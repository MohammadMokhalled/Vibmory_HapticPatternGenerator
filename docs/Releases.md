# Windows builds and draft releases

Every push, pull request, or manual run of `build.yml` produces a tested Windows x64 application when both build jobs succeed. The Linux job tests the standard C++ core with sanitizers and static analysis; it does not build a Linux desktop package. Windows desktop support starts at Windows 10 version 1809. Hardware routing and latency still need testing on the intended output device.

## Downloads and installation

The `Vibmory-windows-x64` artifact contains the portable ZIP, a per-user setup EXE, SHA-256 files, preview images, and installer/signing reports. It is retained for 30 days. The same run's `Vibmory-dependency-sources` artifact contains the pinned dependency sources and provenance described in [Distribution](Distribution.md).

Extract the portable ZIP and run `Vibmory/Vibmory.exe`, keeping its DLLs and plugin directories together. Alternatively run `Vibmory-windows-x64-setup.exe`. Qt and the C++ runtime are bundled; neither Qt nor administrator rights are required on the target machine.

Setup defaults to `%LOCALAPPDATA%\Programs\Vibmory`, registers an uninstall entry for the current user, and creates current-user Start Menu shortcuts. Re-running setup upgrades the same folder. To change the folder, uninstall first. Close Vibmory before upgrading or uninstalling; setup checks the executable before removing files and rejects a busy application. The installer has no automatic updater or background network check.

The uninstaller removes only paths shipped in that version's manifest and removes directories only when empty. It preserves user projects, recovery snapshots, and UI preferences. Keep personal files outside the installation folder to avoid collisions with shipped filenames. CI verifies installation and upgrade file hashes, a path containing spaces and a non-ASCII character, busy-file protection, native launch without Qt on PATH, and uninstall preserving a user project. A separate native UI probe runs Save, Open, and asynchronous WAV Export against real files with only deployed Qt runtime/plugins available. Its Qt Test harness stays outside the shipped package, and its fake audio output keeps device availability separate from file-workflow acceptance.

For unattended installation, use `/S /D=C:\absolute\installation folder`; `/D` must be the last argument and its path is not quoted. Run `Uninstall.exe /S` for unattended removal. `VIBMORY_INSTALLER_TRACE` can point to a writable log file when diagnosing setup; normal installations do not create this trace.

## Optional Authenticode signing

Without signing credentials the workflow produces explicitly reported **unsigned test builds**. This repository does not distribute a private certificate. Configure these repository Actions secrets to enable signing on trusted pushes/manual runs:

- `VIBMORY_SIGNING_CERTIFICATE_BASE64`: base64-encoded PFX containing the code-signing certificate and private key.
- `VIBMORY_SIGNING_CERTIFICATE_PASSWORD`: PFX password, if present.

The optional Actions variable `VIBMORY_TIMESTAMP_URL` overrides the default RFC 3161 timestamp endpoint. The helper imports the certificate into the runner's current-user store, signs the application before its runtime inventory is generated, signs setup after compilation, verifies each signature with Windows SDK SignTool, and removes its temporary PFX and newly imported certificate. Signing is skipped for pull requests. Any configured signing or signature-verification failure stops the build. The signed path requires a valid certificate and timestamp service; unsigned CI does not prove that external credential path.

## Create a reviewable draft

Set the numeric version in `project(Vibmory VERSION ...)` before the release build. An existing tag must be `v<version>` or `v<version>-<prerelease>` and point to the exact built commit. For example, for CMake version `2.0.0`:

```powershell
git tag v2.0.0-rc.1
git push origin v2.0.0-rc.1
```

A version-tag push runs both jobs and then assembles a **draft** release. Once the workflow is present on the repository's default branch, you can also select the tagged commit in the manual run form and set `release_tag` to the existing matching tag. GitHub requires that default-branch presence for [manual dispatch](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow). A manual run with no release tag creates only ordinary build artifacts. Branch push/tag builds work before that merge.

The assembler checks the trusted workflow/repository, successful Linux and Windows jobs, exact source revision and version, peeled Git tag, portable/setup checksums, runtime inventory, dependency source hashes/provenance, all installer acceptance results, and consistent signing reports. It attaches the portable ZIP, setup EXE, matching dependency-source ZIP, individual checksums, and `SHA256SUMS.txt`. Release notes link the source and build and state the signing status and hardware-test boundary.

The release always remains a draft; publication is a separate maintainer action after review. A retry of the same run can replace that run's own draft assets. The assembler refuses to change a published release or a draft owned by a different run. Drafts require repository access to view; ordinary CI artifacts remain available through their build run.
