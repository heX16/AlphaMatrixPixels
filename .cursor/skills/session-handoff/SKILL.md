---
name: session-handoff
description: Summarizes the current work session
disable-model-invocation: true
---

Your work session is ending. Write a short summary of what you did in this session so the user can pass this information to another GPT agent.

- Produce a brief handoff with:
  - **Summary**: goal + what was done.
  - **Changes made**: key changes + relevant `files/paths` and important functions/classes.
  - **Current state**: works vs incomplete + key decisions/assumptions.
  - **Next steps**: concrete actions.
  - **Notes**: commands/tests if known + blockers/questions/risks.

