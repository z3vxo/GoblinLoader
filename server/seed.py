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


conn.commit()
conn.close()
print(f"[*] Seeded 1 campaign, {len(agents)} agents")
