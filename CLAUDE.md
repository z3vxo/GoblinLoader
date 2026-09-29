# Learning red teaming

Pentester learning red teaming, focused on developing reflective DLLs.
Format: [PIC rDll loader] [any dll] shellcode blob.

## Project Structure

```
src/
├── file.c                              # Test DLL (MessageBoxA on DLL_PROCESS_ATTACH)
├── file.dll                            # Compiled test DLL (appended to loader)
├── makefile                            # Build system
├── rLdr/
│   ├── includes/
│   │   ├── nt.h                        # NT structs (PEB, LDR, UNICODE_STRING), ntdll typedefs, djb2 hashing, PIC memcpy/memset
│   │   └── core.h                      # Hash defines, PE macros, Loader struct, function declarations
│   └── core/
│       ├── loader.c                    # Main loader entry — find DLL, allocate, copy sections, relocate, IAT, permissions, TLS, DllMain
│       └── assembly/
│           ├── asmx64.asm              # NASM entry stub (.text$A) — stack align, shadow space, call/pop location, calls loaderEntry
│           └── utils.c                 # GetPeb(), GetModule() (PEB walk), GetProc() (export table walk by hash)
└── test/
    └── inject.c                        # Test harness — VirtualAlloc → memcpy → VirtualProtect → CreateThread
```

## Build

```bash
cd src && make x64
```

Pipeline: nasm (asm stub) → mingw-gcc (C files) → link → objcopy .text → cat loader.bin + file.dll → shellcode.bin

### Compiler flags
- `-Os` — optimize for size
- `-fno-stack-protector` — no __stack_chk_fail (CRT)
- `-fno-asynchronous-unwind-tables` — no .pdata/.xdata
- `-fno-ident` — no .comment section
- `-fno-exceptions` — no exception tables
- `-mno-red-zone` — Windows x64 has no red zone
- `-nostdlib` — no CRT
- `-fno-builtin` — prevent GCC replacing loops with memcpy/memset calls
- `-masm=intel` — intel assembly syntax in inline asm. **Only affects inline asm, NOT regular C code.** Do not remove C-level logic (like tolower) to "fix" asm issues — they are independent.

### Linker flags (via -Wl,)
- `--no-seh` — no SEH directory
- `-e start` — entry point is asm stub
- `--gc-sections` — remove unused sections
- `-s` — strip symbols

## Loader Flow

1. ASM stub (.text$A): align stack, shadow space, call/pop for location, call loaderEntry
2. Scan forward from location for MZ + PE signature → find appended DLL
3. PEB walk → resolve ntdll and kernel32 modules
4. Hash-based export resolution → NtAllocateVirtualMemory, NtProtectVirtualMemory, LdrLoadDll, LoadLibraryA, GetProcAddress, CreateFileA, NtCreateSection, NtMapViewOfSection, NtContinue
5. Parse PE headers of appended DLL
6. Module overloading: CreateFileA(dbghelp.dll) → NtCreateSection(SEC_IMAGE) → NtMapViewOfSection → get image-backed memory
7. NtProtectVirtualMemory(RW) on entire mapped image
8. Copy headers + sections (overwrites sacrificial DLL)
9. Process base relocations (delta patching)
10. Resolve IAT using LoadLibraryA + GetProcAddress (handles forwarded exports, SxS)
11. Set per-section memory permissions via NtProtectVirtualMemory
12. TLS callbacks (DLL_PROCESS_ATTACH)
13. Call DllMain(DLL_PROCESS_ATTACH)

## Key Design Decisions

- **All code in .text**: no globals, no string literals, no CRT — everything PIC
- **Section ordering**: .text$A (asm stub, byte 0) → .text$B (C code)
- **djb2 hashing**: HashStringA (ANSI, for export names), HashStringW (wide, case-insensitive, for module names from PEB). The tolower in HashStringW is essential — PEB module name casing varies across Windows versions (e.g. "ntdll.dll" vs "NTDLL.DLL"), so hashing must be case-insensitive.
- **volatile in picMemcpy/picMemset**: prevents GCC optimizing byte loops back into CRT calls
- **GetProcAddress for IAT**: avoids reimplementing forwarded export resolution
- **Module overloading**: maps a sacrificial DLL (dbghelp.dll) via CreateFileA → NtCreateSection(SEC_IMAGE) → NtMapViewOfSection, then overwrites the entire mapped image with the payload DLL. Memory appears image-backed by a legitimate file on disk instead of suspicious private unbacked RWX.
- **NtAllocateVirtualMemory/NtProtectVirtualMemory**: ntdll direct calls instead of kernel32 wrappers
- **PEB struct truncated after Ldr field**: only need peb->Ldr, offsets before it are unchanged
- **LDR_DATA_TABLE_ENTRY truncated after BaseDllName**: only access DllBase and BaseDllName

## Hash Values (djb2, seed 5381)

```
NTDLL_HASH                   0x22d3b5ed  (wide, lowercased)
KERNEL32_HASH                0x7040ee75  (wide, lowercased)
NTALLOCATEVIRTUALMEMORY_HASH 0x6793c34c  (ansi)
NTPROTECTVIRTUALMEMORY_HASH  0x082962c8  (ansi)
LDRLOADDLL_HASH              0x0307db23  (ansi)
LOADLIBRARYA_HASH            0x5fbff0fb  (ansi)
GETPROCADDRESS_HASH          0xcf31bb1f  (ansi)
NTFLUSHINSTRUCTIONCACHE_HASH 0x80183adf  (ansi)
NTCONTINUE_HASH              0x780a612c  (ansi)
NTCREATESECTION_HASH         0xd02e20d0  (ansi)
NTMAPVIEWOFSECTION_HASH      0x231f196a  (ansi)
CREATEFILEA_HASH             0xeb96c5fa  (ansi)
```

## Testing

```bash
# Compile test harness
x86_64-w64-mingw32-gcc src/test/inject.c -o inject.exe -lkernel32

# Run (on Windows)
inject.exe shellcode.bin
```

## Lessons Learned

- **`-masm=intel` only affects inline asm, not C code.** GCC's `-masm=intel` flag reverses operand order in `__asm__()` blocks (AT&T `src,dst` → Intel `dst,src`). Regular C code (comparisons, arithmetic, intrinsics) is unaffected. Don't remove C logic to compensate for asm bugs.
- **Prefer `__readgsqword(0x60)` over inline asm for PEB access.** The compiler intrinsic always generates the correct `mov rax, gs:[0x60]` regardless of `-masm=intel`. Inline asm with `-masm=intel` silently reversed the operand, generating opcode `89` (store) instead of `8b` (load), which corrupted the TEB's PEB pointer.
- **HashStringW must tolower.** PEB module name casing varies by Windows version. Without case-insensitive hashing, GetModule silently returns NULL → GetProc dereferences NULL → thread dies with no visible error.
- **NtFlushInstructionCache is a no-op on x86/x64** (CPU maintains cache coherency), but included for correctness following Havoc's pattern.
- **Patch ImageBase in copied headers** after mapping, before relocations — matches Havoc DllLdr behavior.

## TODO (Stealth Roadmap)

1. **NtContinue for execution transfer** — Build a CONTEXT struct, set RIP to DllMain, call NtContinue instead of calling DllMain directly. Cleans loader stack frames but still missing RtlUserThreadStart → BaseThreadInitThunk base frames (a detection signal). Hash ready: `0x780a612c`.
2. **Indirect syscalls** — Current loader calls ntdll exports (NtProtectVirtualMemory etc.) through the export table, which EDR userland hooks intercept. Find the `syscall` instruction inside ntdll for each function and jump to it.
3. **Skip header copy** — Leave sacrificial DLL's PE headers intact instead of overwriting with payload headers. Minor stealth win against in-memory inspection.

Future considerations:
- **Register unwind tables** — Call RtlAddFunctionTable or RtlInstallFunctionTableCallback for the loaded DLL's code. Without .pdata/.xdata, EDR stack walks hit an unwindable gap (detection signal) and exceptions crash instead of unwinding properly. Do after module stomping.
- Stomp PE headers (zero MZ/PE after loading) — quick win against in-memory PE scanners
- Thread hijacking or APC injection for legitimate base stack frames
- Sleep obfuscation (encrypt payload in memory during sleep)

## Notes

- `nt_temp.h` and `.bak` files are reference only — not used in build
- file.dll in src/ is the DLL that gets loaded by the reflective loader
- GetProc iterates NumberOfNames (not NumberOfFunctions) since NameArray only has named exports

---

# Core DLL (src/ldr)

The payload agent. A Havoc-style modular C2 agent, compiled as a PIC DLL (`src/ldr/file.dll`) that the rLdr reflects into memory. It resolves every API by hash, polls a listener over WinHTTP, and runs tasks (EXE mapping or reflective "module" DLLs).

## Build

```bash
cd /home/kali/loaderProject && make x64-ll      # main mode: LOAD_AND_LISTEN (poll loop)
# x64-le  -> LOAD_AND_EXIT (pull one file, run, exit)
# x64-*-debug -> + DEBUG (AllocConsole + DBGA logs)
```

Pipeline: nasm (rLdr stub) → gcc (rLdr) → objcopy .text → gcc (ldr DLL) → merge.py loader.bin + file.dll → shellcode.bin

Build variants are selected with `-D`:
- `LOAD_AND_LISTEN` — poll loop + module loading (the main mode)
- `LOAD_AND_EXIT` — single-file pull-and-run
- `DEBUG` — console alloc + `DBGA()` logging

Module build (separate):

```bash
cd src/modules && make ls   # ls.c -> ls.dll (import-free, entry = ModuleEntry)
```

## Structure

```
src/ldr/
├── includes/
│   ├── api/apis.h          # fn typedefs, hash #defines, Win32/Modules structs
│   ├── comms/comms.h       # WinHTTP wrapper decls
│   ├── core/core.h         # LdrInstance (global `ldr`), Config, hash/GetModule/GetProc decls
│   ├── core/nt.h           # PEB + LDR_DATA_TABLE_ENTRY structs
│   ├── core/utils.h        # inline PIC helpers (GetPeb, LdrStrlen/Memcpy/Memset/...)
│   ├── loader/loader.h     # LdrTask, Module API table, PE macros, task/file/msg defines
│   └── parser/parser.h     # ParserRead/ParserWrite framing
├── src/
│   ├── entry.c             # DllMain(DLL_PROCESS_ATTACH) -> LdrMain
│   ├── core/
│   │   ├── core.c          # LdrMain: init -> ParseConfig -> poll loop -> dispatch
│   │   ├── setup.c         # alloc LdrInstance + resolve APIs by hash
│   │   ├── config.c        # embedded UserId/AgentId(/FileId) blob -> ParseConfig
│   │   └── utils.c         # LdrExitThread (free structs + RtlExitUserThread)
│   ├── api/apiLdr.c        # djb2, GetModule (PEB walk), GetProc (export walk + forwarders)
│   ├── comms/comms.c       # WinHTTP: NwLoadApis, NwInternalDoPost, NwPollServer, NwPostOutput
│   ├── parser/parser.c     # ParserRead/ParserWrite impl
│   └── loader/
│       ├── loader.c        # LdrLoadAndRun dispatch; LdrPullFile/LdrInitPoll/LdrPollServer
│       ├── loader_exe.c    # LdrRunExe -> LdrMapExe (reloc) / LdrHollowExe (no-reloc, spawns svchost)
│       ├── loader_utils.c  # relocs, IAT, sections, perms, ExitProcess hook
│       └── module.c        # LdrRunModule: map module DLL, build Module table, run it
├── file.dll                # compiled payload DLL
└── makefile
```

`src/modules/` holds the loadable modules (`ls.c` + `common.h`); `src/test/` the EXE-injection harness.

## Core Flow (LdrMain)

1. `DllMain(DLL_PROCESS_ATTACH)` → `LdrMain`
2. `LdrAllocateCoreStructsAndLoadApis` — `GetModule(ntdll/kernel32)`, resolve `LocalAlloc/Free/ReAlloc`, `GetProcAddress`, `LoadLibraryA`, `FreeLibrary` + ntdll `NtAllocateVirtualMemory/NtProtectVirtualMemory/NtFlushInstructionCache/NtFreeVirtualMemory/NtDelayExecution` by hash, then alloc `ldr->win32/modules/config`.
3. `ParseConfig` — read `UserId`/`AgentId`(/`FileId`) from the embedded length-prefixed blob in `config.c`.
4. `LOAD_AND_EXIT`: `LdrPullFile()` → `LdrLoadAndRun()`.
   `LOAD_AND_LISTEN`: `LdrInitPoll()` → loop { `NtDelayExecution` sleep → `LdrPollServer()` → switch on `task.code` }.

## Task Wire Format (listener → agent)

```
[code:4][file_type:4][ (if FILE_EXE) has_reloc:4 ][ (if TASK_MODULE) has_args:4 ][data_size:4][data:N][ (if has_args) arg_len:4][arg:N]
```

- `TASK_LOAD=0x1` `TASK_MODULE=0x3` `TASK_NO_TASK=0xff`
- `FILE_EXE=0xac` `FILE_DLL=0xab`
- Args read via `ParserReadString` → NUL-terminated, stored in `LdrTask.args`, passed to the module entry.
- `has_reloc` is keyed off `FileType == FILE_EXE`, `has_args` off `code == TASK_MODULE`. Mutually exclusive (EXE never carries args, module never carries reloc), so no double-read.
- For a no-reloc EXE, `data` is the **prepended blob** (`exemap loader + exe`), not a raw PE — that's why the server sends `has_reloc` (the agent can't parse a blob that starts with shellcode).

## Output Wire Format (agent → listener)

```
[MSG_OUTPUT=0xad][userId:len+37][agentId:len+37][output_type=0xaf][payload...]
```

Header written by `LdrRunModule` into the same `ParserWrite` the module writes into. Module output (e.g. `ls`):

```
[name_len:4][name:N][entry_type:4][size:8] ... repeated, terminated by [END_SIG=0xff]
```

`entry_type`: 1=dir, 2=file, 3=link. `size` is 8 bytes (`ULONGLONG`, little-endian).

## Module System

- `LdrRunModule` maps the module DLL: `NtAllocateVirtualMemory` → copy headers/sections → `LdrProcessRelocs` → `LdrSetSectionPerms`. **No IAT processing** — modules must be import-free (resolve everything through the API table).
- Builds a `Module` struct (function table) and calls `entry(mod, writerCtx, args, argLen)` where `entry = base + AddressOfEntryPoint`.
- Module API: `ModuleWrite4/8/Str` (serialize output), `ModuleGetModule/GetProc` (hash resolve), `ModuleLoadLibraryA/FreeLibrary`.
- Modules compile: `-shared -nostdlib -e ModuleEntry --dynamicbase`, no imports, fully RIP-relative (no `.reloc` needed).

## Key Design Decisions

- **Global `LdrInstance *ldr`** — holds `win32` (all resolved fn pointers), `modules` (ntdll/kernel32/winhttp), `config`.
- **Hash-only resolution** — no runtime import table reliance; ntdll/kernel32 resolved via PEB walk, everything else via `GetProc` hash.
- **`ModuleWriteStr` writes RAW bytes** (`ParserWriteRaw`) — the length is written separately by the module (`ModuleWrite4`). Using `ParserWriteBytes` here double-prefixes (`[len][len][str]`).
- **`ParserWrite4/8`** write raw; **`ParserWriteBytes`** writes `[len][bytes]`; **`ParserReadString`** returns a NUL-terminated allocated string.
- **`NwInternalDoPost`** takes `bReadResponse`; `NwPollServer` reads the response, `NwPostOutput` is fire-and-forget (`bReadResponse=FALSE`).

## Lessons Learned (core DLL)

- **`Module` struct field order MUST match `loader.h` exactly** (`Write4, Write8, WriteStr, GetModule, GetProc, LoadLibraryA, FreeLibrary`). Each module re-declares its own copy of the struct; if `Write8`/`WriteStr` are swapped, the module's `ModuleWrite8` call actually invokes `ModuleWriteStr` with a file size as a pointer → `memcpy` from garbage → access violation.
- **Modules are import-free by contract** — `LdrRunModule` never calls `LdrProcessIAT`, so a module that imports CRT/Win32 will crash on its first real import.
- **The `has_args` read must be conditional on the task code**; mis-keying it desyncs every subsequent field (see Task Wire Format above).
- **ntdll exports `NtWriteVirtualMemory`/`NtReadVirtualMemory`, not `NtWriteProcessMemory`/`NtReadProcessMemory`.** Hashing the wrong name makes `GetProc` return NULL → the call hard-crashes with no returned `NTSTATUS` (so the `NT_SUCCESS` check never fires).
- **Use the NTAPI thread-context calls for hollowing, not the kernel32 wrappers.** `GetThreadContext`/`SetThreadContext`/`ResumeThread` (kernel32) failed in the hollowing path; `NtGetContextThread`/`NtSetContextThread`/`NtResumeThread` (ntdll) work. Same for the memory ops — the ntdll `Nt*` calls are what you want.

## Hash Values

Module/API hashes live in `src/ldr/includes/api/apis.h` (djb2, seed 5381). Key ones:

```
HASHED_ntdll        0x22d3b5ed   (wide, lowercased)
HASHED_kernel32     0x7040ee75   (wide, lowercased)
HASHED_GetProcAddress 0xcf31bb1f (ansi)
HASHED_LoadLibraryA   0x5fbff0fb (ansi)
HASHED_CloseHandle    0x3870ca07 (ansi)
```

Module-local hashes (e.g. `FindFirstFileA 0xae2636cf`, `FindNextFileA 0xf3b43c46`, `GetFullPathNameA 0x3524e9e7`) are defined in each module; verify with `python3 hasher.py`.

---

# Process Hollowing (src/exemap)

For EXEs without `.reloc` (fixed-base images), the agent can't map in-process — they must sit at their preferred `ImageBase` (`0x140000000` on x64), which isn't free. So the agent hollows a fresh `svchost.exe`.

## Agent side — `LdrHollowExe` (`src/ldr/src/loader/loader_exe.c`)

1. `CreateProcessA(svchost.exe, CREATE_SUSPENDED)`
2. `NtAllocateVirtualMemory` (remote, RWX) for the blob
3. `NtWriteVirtualMemory` the blob in
4. `NtGetContextThread` → `ctx.Rip = blob` → `NtSetContextThread` → `NtResumeThread`

The blob is `exemap loader.bin + payload.exe` (prepended). The server sends `has_reloc` at upload time because a no-reloc payload arrives as the prepended blob (starts with shellcode, not `MZ`), so the agent can't cheaply parse its PE headers.

## Injected stub — `src/exemap/core/loader.c` (PIC)

1. Scan forward from `location` for `MZ`+PE → find appended EXE
2. Resolve ntdll/kernel32 by hash
3. `NtUnmapViewOfSection(peb->ImageBaseAddress)` — self-unmap svchost
4. `NtAllocateVirtualMemory` at the EXE's preferred base; bail if it lands elsewhere
5. Copy headers (patch `ImageBase`) + sections
6. Fix IAT via `LoadLibraryA` + `GetProcAddress`
7. Set per-section perms via `NtProtectVirtualMemory`
8. Set up TLS: `TlsAlloc` → write index to `AddressOfIndex` → alloc block + copy template → `TlsSetValue`
9. Run TLS callbacks (`DLL_PROCESS_ATTACH`)
10. `((void (*)(void))entry)()`

Build: `cd src/exemap && make x64` → `shellcode.bin` = `loader.bin` + `payload.exe` (no-reloc: `--disable-dynamicbase` + `objcopy --remove-section=.reloc`). `listener.py` serves this on `PULL_FILE` as `[TASK_LOAD][FILE_EXE][has_reloc=0][data_size][data]`.

---

# C2 Server (src/server)

Operator-facing web server. Go backend (chi) + React SPA frontend (Vite), served as a single self-contained binary via `//go:embed`.

## Structure

```
src/server/
├── Makefile                         # build system (see below)
├── server                           # compiled binary (output of make build/all)
├── backend/
│   ├── cmd/main.go                  # entry point — setup / register / run commands
│   ├── gen_secrets.py               # go:generate script — writes secret.go, salt.go, install_id.go
│   └── internal/
│       ├── assets/
│       │   ├── assets.go            # //go:embed static
│       │   └── static/              # React build output (gitignored); embedded at go build time
│       ├── auth/
│       │   ├── auth.go              # CreateJWT / VerfiyJWT (HS256, 30-day)  (note: "Verfiy" is a typo in source)
│       │   └── secret.go            # GENERATED — var secret = []byte("...")
│       ├── database/
│       │   ├── database.go          # DB struct, GetDbPath, SetupDB (schema, no migrations), NewDB, InsertUser
│       │   ├── db_users.go          # VerifyCreds, GetAgents
│       │   ├── db_campaigns.go      # GetCampaigns, InsertCampaign, DeleteCampaign
│       │   ├── db_files.go          # InsertFile, ListFileMetadata, GetFiles, DeleteFile
│       │   ├── db_tasks.go          # GetTasks (pending tasks for an agent)
│       │   └── types.go             # Campaign, Agent, File/Files, FileResp/FileListResp, Task(s)
│       ├── parser/
│       │   └── parser.go            # binary reader: GetCodeAndAgentID, Read4/ReadBytes/ReadString, Err
│       ├── server/
│       │   ├── server.go            # Server struct (DB + WS hub), New(), Parse() (pflag), Start() (chi + static SPA)
│       │   ├── auth_handlers.go     # LoginHandler, AuthMiddleware
│       │   ├── campaing_handlers.go # GetCampaigns, CreateCampaign, DeleteCampaign  (note: filename is misspelled "campaing")
│       │   ├── agent_handlers.go    # HandleAgentCheckin, GetAgents, DeleteAgent
│       │   ├── file_handlers.go     # UploadFile, ListFiles, DeleteFile + parsePE
│       │   └── types.go             # LoginReq, LoginResp, NewCampaignReq
│       ├── setup/
│       │   ├── setup.go             # RunSetup() (root), RunRegister(user,pass,domain) (user)
│       │   └── disk.go              # SetupFolder() — creates ~/.local/share/ldr/{db,config}/
│       ├── utils/
│       │   ├── httputils.go         # writeJson, Return400/401/500
│       │   └── salt.go              # GENERATED — var FingerprintSalt = "..."
│       └── ws/
│           ├── ws.go                # operator WS hub — Clients set, Broadcast, req/res Handler, one writer/conn
│           └── events.go            # event name constants (agent.new/checkin/output, file.new)
└── frontend/
    ├── index.html                   # Vite entry (loads Google Fonts: Inter + JetBrains Mono)
    ├── src/
    │   ├── main.jsx                 # React root (StrictMode)
    │   ├── App.jsx                  # BrowserRouter + providers — / → Login, /dashboard → Dashboard (nested tabs), * → /
    │   ├── index.css                # :root design tokens + font vars + global reset (single source of truth)
    │   ├── context/
    │   │   ├── SocketContext.jsx    # single persistent WS: send(req)→promise, on(event), reconnect/backoff
    │   │   ├── CampaignContext.jsx  # currentCampaign — persists across tab switches
    │   │   └── TerminalContext.jsx  # activeAgent — which agent the terminal is attached to
    │   └── pages/
    │       ├── Login.jsx / Login.css # login form — POST /rest/login, stores JWT, navigates /dashboard
    │       ├── Dashboard.jsx / Dashboard.css    # topbar + nav tabs + campaign dropdown + <Outlet/>
    │       └── tabs/
    │           ├── UsersTab.jsx / UsersTab.css     # agent table + terminal (uses shared socket)
    │           ├── CampaignTab.jsx / CampaignTab.css # campaign CRUD
    │           └── FilesTab.jsx / FilesTab.css     # upload modal (Type/Arch) + file table
    └── package.json                 # react, react-dom, react-router-dom, vite, oxlint
```

## Database schema (SQLite, `~/.local/share/ldr/db/app.db`)

`SetupDB` uses `CREATE TABLE IF NOT EXISTS` — there are **no migrations**. Schema changes require deleting the DB file.

| Table | Columns |
|-------|---------|
| `users` | `id, username, password` (sha256(salt+pass)) |
| `campaigns` | `id, uuid UNIQUE, name, created_at` |
| `agents` | `id, uuid UNIQUE, campaign_uuid→campaigns, username, hostname, domain, architecture, country, last_seen` |
| `files` | `id, uuid UNIQUE, campaign_uuid→campaigns, name, size, kind, arch, has_reloc, sha256, created_at` |
| `tasks` | `id, agent_uuid→agents, code, file_type, has_reloc, has_args, file_uuid→files, args, status, created_at` + `idx_tasks_agent_status` |

FKs need `PRAGMA foreign_keys = ON` and a **UNIQUE** column on the parent — keep `agents.uuid`/`files.uuid` unique or the `REFERENCES` break at insert time.

## File uploads & PE classification

`POST /rest/files/{campaignID}` (multipart field `file`, optional `kind`/`arch`):

- Bytes are written to `~/.local/share/ldr/files/<uuid>`; the `files` row holds metadata only (no BLOBs — payloads can be large). `sha256` is computed while streaming.
- `parsePE` classifies: valid PE + `IMAGE_FILE_DLL` → `dll`, PE otherwise → `exe`, no `MZ` → `shellcode`. `arch` from `IMAGE_FILE_MACHINE` (`0x8664` x64 / `0x14c` x86 / `0xaa64` arm64).
- `has_reloc` = a non-empty `.reloc` **section**, not the BaseReloc data directory — `objcopy --remove-section=.reloc` leaves a stale directory entry that would false-positive.
- Operator `kind` wins: `shellcode` forces it; `exe`/`dll` require a matching valid PE; `auto`/empty uses detection.
- Unsupported arch (`≠ x64/x86`) → `400 "<arch> unsupported"` and the staged file is deleted.
- `DELETE /rest/files/{id}` removes the DB row **and** the disk file.

## Build

```bash
cd src/server

make build       # Go only — embeds whatever is currently in internal/assets/static/
make all         # go generate (fresh secrets) + npm build → copy dist → go build
make secrets     # go generate (fresh JWT secret/salt/installID) + go build
make run         # make build + ./server run [ARGS]
make clean       # remove binary + internal/assets/static/
```

Pass flags via `ARGS`: `make run ARGS="--addr 0.0.0.0 --port 8081"`

## Setup flow (first install)

```bash
sudo ./server setup                              # root: reads DMI, writes /etc/ldr/fingerprint.json
./server register <user> <pass> <domain>         # user: dirs + API reg + DB + config
./server run --addr 0.0.0.0 --port 8081
```

- `setup` errors out if not run as root (`os.Getuid() != 0`)
- `register` errors with a clear message if `/etc/ldr/fingerprint.json` is missing
- If dirs under `~/.local/share/ldr/` are root-owned from a prior run: `sudo rm -rf ~/.local/share/ldr/`

## Build-time secrets (`go generate`)

Three values are generated once and committed as Go source files — stable across builds until you explicitly regenerate:

| File | Variable | How generated |
|------|----------|---------------|
| `internal/auth/secret.go` | `var secret = []byte("...")` | `secrets.token_hex(32)` |
| `internal/utils/salt.go` | `var FingerprintSalt = "..."` | `secrets.token_hex(16)` |
| `internal/setup/install_id.go` | `var installID = "..."` | `uuid.uuid4()` |

Run `go generate ./cmd/` (or `make secrets`) to produce fresh values for a new deployment. Each file carries `// Code generated by gen_secrets.py; DO NOT EDIT.`

## Embedded frontend

The React build output is embedded into the Go binary at compile time via `//go:embed static` in `internal/assets/assets.go`. The server uses `fs.Sub` to strip the `static/` prefix and serves files at `/`. Any path not matching a real file falls back to `index.html` (SPA routing).

`internal/assets/static/` is gitignored — populated by `make all` from `frontend/dist/`.

## Frontend

- **Framework**: React + Vite
- **Router**: react-router-dom (`BrowserRouter`); `/dashboard` nests `users`, `campaign`, `builder`, `files`, `settings` tabs (builder/settings are stubs)
- **Linter**: Oxlint
- **Fonts**: Inter (UI), JetBrains Mono (wordmark/mono values) — loaded from Google Fonts in `index.html`
- **Style**: Midnight Emerald design system (see UI skill below). Tokens live once in `src/index.css` `:root`; all other CSS files consume them (no duplicated `:root` blocks)
- **State**: app-root providers — `SocketContext` (single persistent WS), `CampaignContext` (current campaign), `TerminalContext` (active agent). Selection and the socket survive navigating away from a tab and back.

## UI Style

All frontend UI follows the **Midnight Emerald** design system — dark navy-black surfaces, hairline borders, emerald accent only, monospace numerals. Full token set and component rules are in the skill file:

```
~/.claude/skills/midnight-emerald-ui.md
```

Load it when building any frontend component for this project. The full token set (surfaces, lines, text, accent, secondary data hues, error/warning) plus `--font-sans` / `--font-mono` lives in `src/index.css` `:root`:

```css
--bg: #070b14  --surface: #0b111d  --surface-2: #111827  --surface-3: #1f2937
--border: #1a2230  --divider: #1e2633
--text: #f1f5f9  --text-muted: #94a3b8  --text-subtle: #64748b  --text-disabled: #3b4556
--accent: #10b981  --accent-strong: #34d399  --accent-tint: rgba(16,185,129,0.14)
--error: #f87171  --error-tint: rgba(248,113,113,0.12)
```

Rules to respect: no drop shadows / glows, no pure `#fff`/`#000`, no borders > 1px, focus = `outline: 2px solid rgba(16,185,129,0.5); outline-offset: 2px`, mono for all numbers/ids.

## API routes

```
POST   /rest/login                  # LoginHandler — no auth
GET    /rest/campaigns              # GetCampaigns — AuthMiddleware required
POST   /rest/campaigns              # CreateCampaign — AuthMiddleware required
DELETE /rest/campaigns/{id}         # DeleteCampaign — AuthMiddleware required
GET    /rest/agents/{id}            # GetAgents (id = campaign uuid) — AuthMiddleware required
DELETE /rest/agents/{id}            # DeleteAgent — AuthMiddleware required
GET    /rest/files/{campaignID}     # ListFiles — AuthMiddleware required
POST   /rest/files/{campaignID}     # UploadFile (multipart "file" + kind + arch) — AuthMiddleware required
DELETE /rest/files/{id}             # DeleteFile — AuthMiddleware required
POST   /rest/checkin                # HandleAgentCheckin (agent → server) — NOTE: currently inside the JWT group
GET    /rest/ws                     # operator WebSocket hub — AuthMiddleware (?token=)
GET    /*                           # SPA static file server (fallback → index.html)
```

Auth: JWT (HS256), 30-day expiry. Token stored in `localStorage` as `jwt`, sent as `Authorization` header. For WebSocket (browsers can't set the header), `AuthMiddleware` falls back to the `?token=` query param.

## Realtime (operator WebSocket hub)

One persistent socket per operator at `GET /rest/ws` (JWT via `?token=`). It carries both commands (request/response) and unsolicited pushes.

```
client → server : { "type":"req",   "id":"<uuid>", "code":N, "agent_id":"", "campaign_id":"", "payload":{} }
server → client : { "type":"res",   "id":"<uuid>", "code":N, "ok":true, "data":{} }   // matched by id
server → client : { "type":"event", "event":"agent.checkin", "data":{} }              // unsolicited push
```

- `ws.WS` is a hub (`Clients map[*Client]struct{}`); each `Client` has a `send chan []byte` fed to a single `writeLoop` goroutine — **one writer per conn**, never write to a `*websocket.Conn` from two goroutines.
- `Broadcast(event, data)` fans out (non-blocking; drops on full buffer). `events.go` defines `agent.new`, `agent.checkin`, `agent.output`, `file.new`.
- Commands: `CodeListFiles=1` → `ListFileMetadata(campaign_id)`; unknown code → `{ok:false,msg:"unknown code"}`.
- Emitters today: `HandleAgentCheckin` broadcasts `agent.checkin`. `agent.output`/`agent.new`/`file.new` are defined but **not emitted yet**.
- Uses `github.com/gorilla/websocket`; `CheckOrigin: true` — fine for a single-operator local tool, tighten before external exposure.

### Frontend socket + terminal

1. `SocketContext` (app root) owns the single socket: `send(req)` returns a promise resolved by request `id`; `on(event, handler)` subscribes; reconnects with exponential backoff.
2. Opening a terminal only stores the agent in `TerminalContext.activeAgent`; closing clears it — **no per-terminal socket**.
3. `Terminal` subscribes to `agent.output` (filtered by `activeAgent.uuid`) and, for `files`, sends `{code:1, agent_id, campaign_id}` then prints `res.data.files`.
4. `UsersTab` subscribes to `agent.checkin` and reloads the agent table (debounced 1s).

## Tasking & agent checkin (WIP)

- Agent → server frames share the header `[code:4][agent_len:4][agent_str]`; `parser.GetCodeAndAgentID()` decodes it. `CHECK_IN=0xac` (`parser.CHECK_IN`).
- `HandleAgentCheckin` parses the header, calls `GetTasks(agentID)`, and broadcasts `agent.checkin`. The task result is currently discarded — **server → agent binary task serialization is not implemented yet**.
- To send tasks the handler must: read each task's `file_uuid` bytes from disk, emit the task wire format (see *Task Wire Format (listener → agent)* above), and mark rows `status=1`.

## Licensing / registration

The current `setup → register → run` flow is **cosmetic** — `run` never reads `/etc/ldr/fingerprint.json` or the token, and `secret`/`FingerprintSalt` are compiled into the distributed binary. A redesigned flow (Ed25519-signed tokens bound to hardware, revocable via online checkin, all authority on a backend we control) is documented in `reg-flow.md`.

## Key Design Decisions

- **`//go:embed`** over external file path — binary is fully self-contained, no `/var/www/html/` needed at runtime.
- **`fs.Sub` strips `static/` prefix** — files in the embed are at `static/foo.js` but served at `/foo.js`.
- **SPA fallback** — unknown paths return `index.html` so react-router client routes (e.g. `/dashboard`) don't 404.
- **Three-command setup** isolates privileges: `setup` (root, DMI only) → `register` (user, everything else) → `run`.
- **`go generate` for secrets** — `secret`, `FingerprintSalt`, `installID` are build-time constants, not runtime-generated, so re-running setup doesn't produce a different identity.
- **WS auth via query param** — browsers can't send the `Authorization` header on a WebSocket handshake, so `AuthMiddleware` checks `?token=` when the header is empty.
- **One writer per WebSocket** — each `Client` has its own `send` channel + `writeLoop`; `Broadcast`/`reply` only enqueue. Concurrent `WriteMessage` calls on the same `*websocket.Conn` race/panic in gorilla.
- **Terminal state lives in `TerminalContext`, not `UsersTab`** — `UsersTab` unmounts when you switch tabs; local state would be destroyed. The app-root provider keeps `activeAgent` alive across tab navigation, and the socket is app-wide (`SocketContext`) so commands and push events share one connection.
- **Payloads live on disk, not in the DB** — `tasks.file_uuid → files.uuid`; bytes are read from `~/.local/share/ldr/files/<uuid>` at checkin. (An earlier `tasks.data BLOB` was removed.)
