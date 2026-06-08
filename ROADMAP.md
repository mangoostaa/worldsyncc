# Roadmap

This roadmap is a practical direction for WorldSync. It is not a strict promise of dates or release contents.

## v0.1.x - Adoption And Stability

- Improve release packaging and installation documentation.
- Add minimal examples for first-time users.
- Add version natives and clearer startup logs.
- Expand tests for core entities, storage, crops, vehicles and pathfinding.
- Harden native input validation for invalid IDs, empty strings and small buffers.
- Keep the existing public Pawn API stable unless a change is required to fix a bug.

## v0.2.x - Server Systems

- Add higher-level helpers for houses and properties.
- Add business and resource-node primitives.
- Improve persistent vehicle workflows.
- Add more practical guides for roleplay and survival gamemodes.
- Add schema migration support for future storage changes.

## v0.3.x - Production And World Logic

- Add production chains for farms, resources and businesses.
- Add zone or territory support.
- Improve route and patrol tooling for NPC-driven systems.
- Add richer debug commands and inspection helpers.

## Longer-Term Ideas

- Linux release automation.
- GitHub Actions for build, test and release packaging.
- Config file support for log level, autosave interval and storage behavior.
- More sample gamemodes that demonstrate complete systems.

## Priorities For Contributors

The most useful contributions are:

- Reproducible bug reports with logs and minimal Pawn snippets.
- Tests for existing behavior.
- Small examples that demonstrate one system at a time.
- Documentation improvements that reduce setup friction.
- Storage and migration improvements that preserve existing server data.
