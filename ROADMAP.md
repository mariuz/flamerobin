# FlameRobin Roadmap

This document outlines the active roadmap and pending tasks for FlameRobin development.

## 📊 Status Summary
- **Core Feature Roadmap (Phases 1-10):** **100% Completed** (Archived in [ROADMAP_DONE.md](file:///home/ubuntu/work/flamerobin/ROADMAP_DONE.md))
- **Security & Distribution Integrity Roadmap:** **0% Completed** (Outstanding / Active TODOs listed below, including a signed apt repository)

---

## 🔒 Security & Release Integrity Roadmap (Active TODOs)

This section tracks security-focused work inspired by [discussion #591](https://github.com/mariuz/flamerobin/discussions/591) about supply-chain and distribution risks.

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

### 8) Signed apt Repository for Debian and Ubuntu
Today the `.deb` is installed from a file downloaded from GitHub Releases (see [docs/install_linux.md](docs/install_linux.md)), so users have to download and install every new version by hand. [Claude's Debian page](https://code.claude.com/docs/en/desktop-linux) centres on a signed apt repository instead, so updates arrive with the system's regular `apt update && apt upgrade`. FlameRobin could have that too, hosted on GitHub Pages and filled by the release workflow.
- [ ] **Maintainer:** create a dedicated GPG signing key for the repository, publish its fingerprint, and store the private key and passphrase as GitHub Actions secrets.
- [ ] Publish the repository on GitHub Pages (a `gh-pages` branch or Pages deployment): `dists/stable/…` with a signed `InRelease`/`Release.gpg`, the `pool/` of `.deb` files, and the public key as `key.asc`.
- [ ] Extend `release.yml` to add each new `.deb` to the pool, regenerate the indexes (`dpkg-scanpackages` / `apt-ftparchive`) and sign them with the key from the secrets.
- [ ] Optionally let the `.deb` register the repository on install, as Claude Desktop's does, so a `.deb` installed from a downloaded file also receives updates.
- [ ] Document it in `docs/install_linux.md`: add the key, verify its fingerprint, register the repository with `signed-by=`, `apt install flamerobin`; update with `apt upgrade`; uninstall including the repository entry and key.
