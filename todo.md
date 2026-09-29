# TODO

## Tasking

- [ ] **Client-side terminal commands** — add `ls`, `whoami`, `domain-info`, `run <file>`.
  - `ls` / `whoami` / `domain-info` are built-in agent-side actions (likely modules or inline tasks).
  - `run <file>` takes a file UUID from the campaign's `files` and queues it for the active agent.
- [ ] **Server-side command → task** — map a terminal command into a `tasks` row:
  - resolve `run <file>` → `file_uuid` + `code`/`file_type`/`has_reloc` from `files`, `agent_uuid` = active agent.
  - validate the file belongs to the same campaign as the agent.
- [ ] **Cleanup for mapped EXE images** — the in-process map path now leaks the image (the threadpool callback was removed because host indirect calls into the image-backed agent mapping trip CFG). Figure out a cleanup method that doesn't rely on a host callback — e.g. reap the thread from the agent's own poll loop with `WaitForSingleObject(hThread, 0)` (needs `HASHED_WaitForSingleObject`).

## Agent protocol

- [ ] **Handle agent registration** — `CODE_REGISTER`: parse agent metadata (username, hostname, domain, arch, country) → `InsertAgent`; broadcast `agent.new`.
- [ ] **Handle agent checkin** — finish `HandleAgentCheckin`: update `last_seen`, fetch pending tasks, respond. (Currently only `GetTasks` + `agent.checkin` broadcast; result discarded.)
- [ ] **Handle agent output** — parse agent → server output frames and broadcast `agent.output` (route by agent uuid) so the terminal renders it.
- [ ] **Task completion → server → frontend** — finish alerting the server once a task is executed: agent echoes the task id back, server marks the row done (`MarkTaskDone(taskId, agentId)`), then broadcasts to the frontend so the terminal/task list updates.
- [ ] **Send tasks back** — serialize the `task` struct into the agent's binary task wire format (`[code][file_type][has_reloc?][has_args?][data_size][data][arg?]`, see *Task Wire Format (listener → agent)* in `CLAUDE.md`):
  - add `parser.CraftTask`/`CraftTasks` mirroring the reader.
  - read each task's `file_uuid` bytes from `~/.local/share/ldr/files/<uuid>` at checkin.
  - mark dispatched rows `status=1` atomically so a retried checkin doesn't re-send.

## Frontend

- [ ] **Build the Builder panel** — replace the `builder` stub tab (payload/config builder for generating agent blobs).

## Licensing

- [ ] **Implement the better licensing system** — see `reg-flow.md`: Ed25519-signed, hardware-bound tokens; backend-held signing key; short-lived tokens refreshed via `/checkin`; seat enforcement + revocation server-side.
