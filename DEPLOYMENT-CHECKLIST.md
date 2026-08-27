# Deployment Checklist

Use this checklist before exposing any service to users or connecting it to production data.

## Ownership and licensing

- Confirm ownership or valid redistribution rights for source code, art, audio, fonts, SDKs, and product names.
- Resolve the conflicting MIT, commercial-license, and “All Rights Reserved” language in `License.md`.
- Confirm that all third-party notices and source obligations are satisfied.

## Build and dependencies

- Record the supported Linux distribution, compiler, Tars, Unity, database, Redis, and system-library versions.
- Remove absolute developer-machine paths from `makefile` and configuration.
- Build from a clean environment using documented commands.
- Pin dependencies and retain checksums or a software bill of materials.
- Verify whether the complete Unity `Assets/` directory is available and licensed.

## Secrets and data

- Scan the complete Git history for passwords, private keys, tokens, production endpoints, and personal data.
- Rotate any credential that may have been committed, even if it was later deleted.
- Provide sanitized examples instead of production configuration.
- Define data retention, export, deletion, encryption, and access-control policies.

## Game integrity

- Use a reviewed cryptographic random-number source where randomness affects outcomes.
- Prevent clients from controlling authoritative cards, balances, timing, or settlement.
- Add replay protection, request idempotency, server-side validation, and append-only audit logs.
- Test disconnects, reconnects, duplicate messages, timeouts, concurrent actions, and repeated settlement.
- Conduct independent fairness and security review where required.

## Operations

- Define health checks, structured logs, metrics, alerts, rate limits, backups, restore tests, and rollback procedures.
- Separate development, staging, and production accounts and networks.
- Apply least privilege to databases, queues, storage, and administrative tools.
- Document incident response and vulnerability disclosure ownership.

## Legal and platform review

- Obtain jurisdiction-specific advice on gambling, sweepstakes, prizes, age limits, payments, virtual assets, taxation, and consumer protection.
- Review Apple App Store, Google Play, payment-provider, advertising, and regional distribution policies.
- Do not rely on this checklist as legal, security, or compliance certification.
