# LinnStrument firmware: pointer for AI coding assistants

The brief for AI coding assistants working in this repository is `CLAUDE.md`.
It's written for Claude Code, which reads it automatically, but everything in
it applies to any assistant: how the firmware is built and tested, its hard
constraints, and the rules for changes, flashing, publishing and commits.

Read `CLAUDE.md` in full before you change anything, and follow it as if it
were addressed to you. Where it names something specific to Claude Code, such
as the MCP servers in `.mcp.json` or the command allowlist in
`.claude/settings.json`, use your own tool's equivalent, and ask the person
when there isn't one. Its rules hold whatever the tool: never flash a
connected instrument or commit without the person's word.

Humans should read `CONTRIBUTING.md`.
