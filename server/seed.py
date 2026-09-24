#!/usr/bin/env python3
import sqlite3, uuid, os
from datetime import datetime

db_path = os.path.expanduser("~/.local/share/ldr/db/app.db")
conn = sqlite3.connect(db_path)
conn.execute("PRAGMA foreign_keys = ON")
c = conn.cursor()

camp_uuid = str(uuid.uuid4())
c.execute("INSERT INTO campaigns (uuid, name, created_at) VALUES (?, ?, ?)",
          (camp_uuid, "Operation Nightfall", "2026-09-01 09:12:00"))

agents = [
    ("jsmith",   "DESKTOP-A1B2C3",  "corp.local",    "x86_64", "US"),
    ("SYSTEM",   "SRV-DC01",        "corp.local",    "x86_64", "US"),
    ("mwilson",  "LAPTOP-9XZ",      "corp.local",    "x86_64", "US"),
    ("agarcia",  "WS-FINANCE-04",   "corp.local",    "x86_64", "US"),
    ("SYSTEM",   "SRV-FILE01",      "corp.local",    "x86_64", "US"),
    ("ntuser",   "KIOSK-LOBBY",     "workgroup",     "x86",    "US"),
    ("blee",     "DESKTOP-B7K2P",   "corp.local",    "x86_64", "US"),
    ("SYSTEM",   "SRV-SQL01",       "corp.local",    "x86_64", "US"),
    ("rthomas",  "LAPTOP-DEV01",    "corp.local",    "x86_64", "GB"),
    ("jdoe",     "WORKSTATION-22",  "internal.net",  "x86_64", "DE"),
    ("SYSTEM",   "SRV-EXCHANGE",    "internal.net",  "x86_64", "DE"),
    ("cmartinez","DESKTOP-C9F1",    "internal.net",  "x86",    "MX"),
]

for username, hostname, domain, arch, country in agents:
    c.execute("""INSERT INTO agents (uuid, campaign_uuid, username, hostname, domain, architecture, country)
                 VALUES (?, ?, ?, ?, ?, ?, ?)""",
              (str(uuid.uuid4()), camp_uuid, username, hostname, domain, arch, country))

files = [
    ("mimikatz.exe",     2_457_600,  "2026-09-02 11:00:00"),
    ("beacon.bin",         184_320,  "2026-09-03 15:22:00"),
    ("lsass.dmp",       52_428_800,  "2026-09-05 09:45:00"),
    ("svc_implant.dll",    245_760,  "2026-09-11 10:10:00"),
    ("creds.txt",            4_096,  "2026-09-12 16:30:00"),
    ("shellcode.bin",       40_960,  "2026-09-21 08:30:00"),
    ("recon.txt",            8_192,  "2026-09-22 13:15:00"),
    ("privesc.exe",        819_200,  "2026-09-23 17:00:00"),
]

for name, size, created_at in files:
    c.execute("""INSERT INTO files (uuid, campaign_uuid, name, size, created_at)
                 VALUES (?, ?, ?, ?, ?)""",
              (str(uuid.uuid4()), camp_uuid, name, size, created_at))

conn.commit()
conn.close()
print(f"[*] Seeded 1 campaign, {len(agents)} agents, {len(files)} files")
