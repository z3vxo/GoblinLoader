# Registration / Licensing Flow

Design for replacing the current `setup → register → run` flow. The goal is a
licensing gate that survives a motivated operator, with the **buyer's binary
treated as untrusted** and all real authority held by a backend we control.

## Threat model

The operator/buyer receives the compiled binary. They can read it, patch it,
edit every file it writes, and run it anywhere. Therefore:

- Any check performed *only* on the client can be patched or bypassed.
- Any secret compiled into the binary is recoverable by the buyer.
- The only trustworthy decision is one made on our backend.

Client-side crypto does not give control; it only raises the cost of copying a
token from one machine to another. Control lives in the backend's seat DB and
in the signature on the license token.

## Current flow and why it fails

```
sudo ./server setup                  # root: collect machine fingerprint
./server register <user> <pass> <domain>   # user: read fingerprint, POST, write config+DB
./server run                         # serve
```

Problems, in order of severity:

1. **`run` never checks the fingerprint.** `server.New()` only calls
   `database.NewDB()` (`backend/internal/database/database.go`). Neither
   `/etc/ldr/fingerprint.json` nor `~/.local/share/ldr/config/config.json` is
   read at runtime. The token returned by `domain/register` is written and
   never used.
2. **Secrets are in the shipped binary.** `internal/auth/secret.go` (JWT HS256
   key) and `internal/utils/salt.go` (`FingerprintSalt`) are compiled in. Any
   buyer can mint tokens and forge fingerprints.
3. **`install_id` is a build constant.** `internal/setup/install_id.go` is
   `go generate`d once, so every install from that build shares it.
4. **Client hashing with a public salt is cosmetic.** `hashField` is
   `sha256(public_salt + value)`.
5. **Portable, world-readable artifact.** `/etc/ldr/fingerprint.json`, mode
   `0644`, unsigned, copyable.
6. **Long-lived, non-revocable tokens.** 30-day JWT with no server check.

## Design principles

- The signing **private key never leaves the backend**.
- The binary embeds only a **public** key.
- The token is **bound to the hardware** server-side.
- Tokens are **short-lived** and refreshed via `checkin`, so revocation works.
- Seat enforcement happens in the backend, not the binary.
- One root-owned, non-portable local artifact; the user-writable config gates
  nothing.

## Keys

Two independent keypairs. Do not conflate them.

| Keypair | Private held by | Lifetime | Purpose |
|---|---|---|---|
| **Signing key** | license backend only | long-lived, product-wide | backend signs license tokens; binary verifies with embedded public key |
| **Install key** | the install (sealed on disk / TPM / DPAPI) | per-install, created at setup | authenticates the install's `checkin`; public half registered at `activate` |

Key handling:

- Generate the signing keypair once. Embed the public key as a constant
  (`internal/setup/license_pubkey.go`). It is public; safe to commit.
- Embed **two** public keys (current + next) so the signing key can be rotated
  without reissuing every field binary.
- Keep the signing private key in a secret manager / HSM, never in this repo.
- Generate `installID` at setup from `crypto/rand`, not at build time.

## Buyer's binary

The distributed binary runs two subcommands over the backend.

### `setup` (runs as root, one time)

Root is required only to read root-only hardware IDs
(`/sys/class/dmi/id/product_uuid`, `board_serial`).

1. Collect hardware IDs: `product_uuid`, `board_serial`, `machine-id`, cpu
   model+cores.
2. Generate a random per-install ID (`crypto/rand`).
3. Generate the install keypair; seal the private half (TPM / DPAPI /
   at minimum `0600 root:root`).
4. `POST <license-backend>/activate` over pinned TLS:
   `{ user, credential, hw_ids, install_pub, install_id }`.
5. Receive `{ license_token }`. Verify the Ed25519 signature with the embedded
   **public** key. Reject on failure.
6. Write `/etc/ldr/license.json` (`0600 root:root`) containing the token,
   install_id, and install key reference.

Deletes the old three-way split: there is no separate `register` command and no
gate material in `~/.local/share/ldr/`.

### `run` (service user, every start)

1. Load and verify `/etc/ldr/license.json` signature with the embedded public
   key. No valid signature → refuse.
2. Recompute the hardware IDs; compare against the token claims. Mismatch →
   refuse (this is what catches a copied `license.json`).
3. `POST <license-backend>/checkin`, authenticated by signing a backend nonce
   with the install key. Expect `{ status, license_token }`.
   - `active` → continue, and store the refreshed short-TTL token.
   - `revoked` → shut down.
4. If the backend is unreachable, allow a bounded grace window (configurable;
   decide the value knowingly), then refuse.
5. Start the server.
6. Background **heartbeat** every N minutes; on `revoked`, terminate.

Local DB (`~/.local/share/ldr/db/app.db`) remains the operators/agents data
store, but it is **not** the license gate — it must never be sufficient on its
own to make the binary run.

## Backend (we control)

Separate from the C2 server being gated — a service it needs cannot be served by
the thing it protects. Always-on.

### Endpoints

- `POST /activate`
  - Authenticate the operator (credential).
  - Look up the license; enforce seat quota for the account.
  - If `hw_ids` already seated → allow only the same install, else reject
    unless the operator releases the seat.
  - Upsert `{ license_id, hw_ids, install_pub, install_id, status=active, exp }`.
  - Return an Ed25519-signed `license_token`.
- `POST /checkin`
  - Authenticate via the install key (nonce signature).
  - Verify the record is still `active` and not revoked.
  - Return `{ status, license_token }` with a fresh short TTL (~24h).
- `POST /release` (optional)
  - Free a seat so the license can be moved.

### Token

```
license_token = Ed25519_sign(private_key,
    license_id ‖ hw_ids ‖ install_pub ‖ issued_at ‖ exp)
```

~24h TTL. Short TTL is what makes revocation effective; the current 30-day JWT
cannot be revoked within a reasonable window.

### State

- Seat/license table keyed by `license_id` / account.
- Per seat: hardware IDs, install public key, status
  (`active` / `revoked`), expiry, last checkin.
- Revocation is a DB flip; the next `checkin` (within the TTL) enforces it.

## Wire / flow summary

```
setup (root)                              license backend
  collect hw ids (root-only)   ──────▶
  gen install keypair                    authenticate operator
  POST /activate                         seat check + upsert record
    {user, cred, hw_ids,                 sign Ed25519_priv(token)
     install_pub, install_id}   ◀──────  {license_token}
  verify sig (embedded pub)
  write /etc/ldr/license.json (0600)

run (service user)                        license backend
  verify license.json sig    ────────▶
  recompute hw ids, compare              verify checkin (install-key nonce)
  POST /checkin               ◀──────  {status, fresh token}
  grace window if offline
  start C2                               heartbeat → revoke terminates
```

## What each side can and cannot do

- Buyer/binary **can**: read the public key, edit local files, patch the binary,
  re-run `setup`.
- Buyer/binary **cannot**: forge a token (no private key), activate past the
  seat quota, keep running after revocation, or move a token to new hardware
  and pass the recomputed-hardware check.

## Repo changes

- Add `internal/setup/license_pubkey.go` (public key constant, generated).
- Add a `VerifyLicense` used by both `setup` and `run`.
- Add an activation client behind an interface so the real backend can be
  swapped in later (stub it now).
- Replace `register` with activation inside `setup`.
- `run`: verify token + recompute hardware + `checkin` + heartbeat.
- Delete reliance on `FingerprintSalt` / `secret` as the distributed trust root.
- Move gate artifact to `/etc/ldr/license.json` `0600 root:root`; stop writing
  gate material under `~/.local/share/ldr/config/`.
- Pin the activation endpoint TLS certificate.

## Remaining limits (accepted)

No client-side scheme is foolproof. The above reduces the cheapest bypass
(copy a file, run anywhere) to an expensive one, makes revocation and seat
enforcement real, and keeps the only unbypassable decision on the backend. A
determined attacker with a patched binary can still run a client that skips its
own local checks — but it still cannot obtain a valid token for hardware that
is not seated, which is the point.
