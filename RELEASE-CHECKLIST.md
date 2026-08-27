# Release Checklist

- Tag a commit that has been reviewed and built from a clean environment.
- State exactly which server, client, database, admin, and infrastructure components are included.
- Publish supported compiler, Tars, Unity, database, and Redis versions.
- Include dependency locks or an SBOM and checksums for release artifacts.
- Remove secrets, personal data, production endpoints, logs, caches, object files, and build dependency files.
- Record database migrations, compatibility, backup requirements, and rollback steps.
- Publish test results for game flow, reconnects, duplicate messages, timeouts, settlement, and concurrency.
- Document known limitations, breaking protocol changes, and upgrade steps.
- Resolve licensing and third-party notices before distribution.
- Never call a release production-ready solely because screenshots or binaries exist.
