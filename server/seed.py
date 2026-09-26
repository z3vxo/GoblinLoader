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


conn.commit()
conn.close()
print(f"[*] {camp_uuid}")
