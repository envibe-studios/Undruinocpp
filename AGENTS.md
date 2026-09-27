# Agent Setup

This project keeps its persistent AI guidance in `.cursor/`. Treat those files
as active project instructions, not editor-only leftovers.

## Startup Context

At the start of meaningful project work, read:

1. `.cursor/FIRST_FIVE_MINUTES.md`
2. `.cursor/rules/00-project-core.mdc`
3. `.cursor/rules/05-collaboration.mdc`
4. `.cursor/rules/10-documentation.mdc`

Then read only the smallest relevant set of docs or playbooks for the task.

## Documentation Map

- Project direction: `.cursor/docs/NORTH_STAR.md`, `.cursor/docs/CURRENT_DIRECTION.md`
- Architecture: `.cursor/docs/PROJECT_ARCHITECTURE.md`, `.cursor/docs/ARCHITECTURAL_DECISIONS.md`
- Engineering standards: `.cursor/docs/ENGINEERING_PHILOSOPHY.md`
- Feature readiness: `.cursor/docs/FEATURE_GATE.md`
- Creative/design direction: `.cursor/design/`
- Task workflows: `.cursor/playbooks/`

## Unreal And MCP

Use UnrealMCP when live editor state, Blueprint structure, level actors, or
asset state matters. Prefer read-only inspection before edits, and ask before
changing production Blueprint assets unless the user has clearly authorized it.

## Scratch Files

Files in `.cursor/` beginning with `_` are generated scratch/output artifacts.
Do not treat them as standing project instructions unless the user explicitly
points to one.
