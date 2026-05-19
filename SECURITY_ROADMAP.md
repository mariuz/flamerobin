# FlameRobin Security Roadmap

This roadmap tracks security-focused work inspired by [discussion #591](https://github.com/mariuz/flamerobin/discussions/591) about supply-chain and distribution risks.

## Security Checklist

### 1) Threat Model and Risk Assessment
- [ ] Define and publish a lightweight threat model for FlameRobin releases and distribution.
- [ ] Document priority risks (release asset tampering, compromised dependencies, CI/CD compromise, website compromise).
- [ ] Review and update the threat model on each major release cycle.

### 2) Release Integrity and Provenance
- [ ] Publish SHA-256 checksums for all release artifacts.
- [ ] Add detached signatures for release artifacts and document signature verification steps.
- [ ] Ensure release artifacts are published only from protected/reviewed workflows.
- [ ] Evaluate GitHub artifact attestations / provenance for release builds.

### 3) Dependency and Build Supply Chain
- [ ] Keep dependency versions pinned where possible (including GitHub Actions).
- [ ] Establish a routine dependency vulnerability review process (vcpkg and other third-party components).
- [ ] Generate and publish an SBOM for release artifacts.
- [ ] Investigate reproducible-build practices for key release targets.

### 4) CI/CD and Repository Hardening
- [ ] Enforce branch protection and required status checks for release-related branches.
- [ ] Require CODEOWNERS or maintainer review for workflow and release pipeline changes.
- [ ] Minimize GitHub token/workflow permissions to least privilege.
- [ ] Enable and monitor GitHub security features (Dependabot alerts, code scanning, secret scanning where applicable).

### 5) Website and Distribution Channel Security
- [ ] Document the trusted official distribution channels (GitHub Releases, flamerobin.org).
- [ ] Add a user-facing checklist for verifying downloads before installation.
- [ ] Review hosting and publication controls for flamerobin.org and linked assets.

### 6) Vulnerability Handling and Incident Response
- [ ] Add a `SECURITY.md` vulnerability disclosure policy if not already present.
- [ ] Define a response playbook for compromised artifacts or credentials.
- [ ] Define communication steps for security advisories and emergency rebuilds/revocations.

### 7) User Documentation
- [ ] Document the current update model (manual download/install, no in-app auto-updater).
- [ ] Add a "How to verify a release" section to project documentation.
- [ ] Add a short security FAQ based on recurring community questions.
