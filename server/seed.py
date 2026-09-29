#!/usr/bin/env python3
import sqlite3, uuid, os
from datetime import datetime

db_path = os.path.expanduser("~/.local/share/ldr/db/app.db")
conn = sqlite3.connect(db_path)
conn.execute("PRAGMA foreign_keys = ON")
c = conn.cursor()

camp_uuid = "a5c729ea-d990-48e8-a497-6b59a3f8408a"
c.execute("INSERT INTO campaigns (uuid, name, created_at) VALUES (?, ?, ?)",
          (camp_uuid, "Operation Nightfall", "2026-09-01 09:12:00"))

agent_uuid = "11111111-1111-1111-1111-111111111111"
c.execute("""INSERT INTO agents
             (uuid, campaign_uuid, username, hostname, domain, process, architecture, isElev, country)
             VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)""",
          (agent_uuid, camp_uuid, "administrator", "WIN-TEST01", "TEST",
           "explorer.exe", "x64", 1, "US"))


conn.commit()
conn.close()
print(f"[*] {camp_uuid}")
print(f"[*] {agent_uuid}")
