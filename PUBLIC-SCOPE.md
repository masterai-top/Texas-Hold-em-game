# Public Repository Scope

This document separates files visible in the public repository from capabilities shown in product material.

## Verified in the public tree

- C++ Match and Order service source files.
- Tars interface definitions and servant implementations.
- Game-start, action, calculation, end, timeout, and message-handling headers.
- Shared poker logic under `common/` and `core/`.
- Configuration-related headers under `config/`.
- Unity `Packages/` and `ProjectSettings/` metadata.
- Product screenshots and technical Markdown documentation.

## Not verified as complete

- Unity `Assets/` and a buildable full client.
- Complete database schema, migration, seed, backup, and rollback assets.
- Complete production backend, operations console, payment, wallet, and settlement services.
- Reproducible dependency versions for the C++ server and Tars runtime.
- Automated unit, integration, load, reconnect, settlement, and security tests.
- CI builds, signed releases, software bills of materials, or independent audits.
- AI implementation corresponding to the `poker-ai` marketing topic.

## How to describe the repository

Recommended: “C++ Texas Hold'em match/order server code, Tars protocols, game-flow components, Unity project metadata, documentation, and product screenshots.”

Avoid stating that the public repository is a complete, audited, production-ready system until every required component can be built and tested from a clean environment.

## Product screenshots

Screenshots demonstrate intended product design and historical product states. They do not establish that every displayed feature is implemented by the files currently available in the public tree.
