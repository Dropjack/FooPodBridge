# Mandatory startup protocol

This file is intentionally ASCII-only so it remains readable before terminal encoding is configured.

Before inspecting, inferring from, or modifying this repository:

1. Configure Windows PowerShell for UTF-8 before displaying repository text:

   ```powershell
   [Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)
   [Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
   $OutputEncoding = [Console]::OutputEncoding
   ```

2. Read repository text with explicit UTF-8 decoding. Read the mandatory startup files from raw bytes with strict UTF-8 error detection:

   ```powershell
   $Utf8Strict = [System.Text.UTF8Encoding]::new($false, $true)
   $Text = $Utf8Strict.GetString([System.IO.File]::ReadAllBytes($Path))
   ```

3. Fully read `docs/PROJECT_RULES.md`, then `tasks/README.md`, then the current task named by that index.

4. Treat mojibake as invalid output. Fix decoding and reread the original bytes; never rewrite a source file because a terminal rendered it incorrectly.

# Repository and reference boundaries

- The formal project is this directory: `D:\dev\foo\FooPodBridge\FooPodBridge`.
- The sibling directory `D:\dev\foo\FooPodBridge\Ref` is read-only reference material. Never modify, format, build into, or clean it.
- `D:\dev\foo\FooCrate` is a separate repository and the integration/test workspace. Protect its existing changes and follow its own `AGENTS.md` before touching it.
- Never access, install into, launch, or modify the user's daily foobar2000 installation on drive C:.

# User-operated application testing

- Computer Use is prohibited. Do not invoke the computer-use skill, sky, cua, or other desktop/browser UI-control tools for this project.
- Do not substitute shell scripts, accessibility APIs, simulated input, automated screenshots, or another automation method for prohibited UI control.
- All tests of application usage, component loading, UI behavior, and interactive device workflows are performed by the user. Do not launch or operate applications to perform these checks.
- Provide concise Chinese instructions with the exact test instance, entry point, action, expected result, and feedback needed. Guide one check at a time and wait for the user's result before the next dependent check.
- Code inspection, command-line builds, non-interactive unit/fixture tests, and package audits remain allowed within the approved task. They do not replace user-operated application verification.
- Record unperformed manual checks as pending. Never claim UI or usage verification based only on compilation or automated tests.
- This restriction overrides earlier task text authorizing automated application/UI checks, including foobar-dev smoke tests.

# Local foobar2000 test instances

- Authorized development deployment and non-interactive diagnostics may use only `D:\dev\foo\FooCrate\.local\foobar-dev`. Application loading and usage checks are user-operated under the rule above.
- User acceptance, clean install, upgrade, rollback, and multi-scenario testing use only `D:\dev\foo\FooCrate\.local\foobar-test`.
- Do not create a third foobar2000 installation under FooPodBridge.
- A task must explicitly authorize real iPod writes. Discovery tests or read-only tests never imply permission to modify an attached device.

# Real iPod and iPod_Control copy read budget

- Do not scan, enumerate, hash, compare, or test an entire user `iPod_Control` copy or attached iPod by default. The presence of a copy, a connected device, or general permission to continue read-only work is not authorization for a bulk scan, including on any iPods connected in the future.
- Do not read files under `iPod_Control/Music` unless a specific, current problem requires those bytes. Prefer reference code, synthetic fixtures, the small device/database files needed for the question, and the minimum targeted metadata. Do not collect or print song titles, paths, identifiers, or database contents in public artifacts.
- Before any necessary bulk read of a user copy or real device, explain the exact question it answers, why a smaller check is insufficient, which files and approximate total bytes will be read, and the expected time/cost. Obtain the user's explicit approval for that individual scan. Previous approval does not carry over to another scan or device.
- Automated builds, unit tests, diagnostics, and newly connected devices must never trigger a bulk scan of user data. Run routine regression tests only against synthetic or approved small fixtures. Stop when enough evidence answers the question; do not repeat a successful scan for reassurance.
- Keep backup verification required before a separately authorized real-device write, but schedule any large verification only after the user approves that specific read. A successful read-only comparison never grants write permission.

# Local build tools

Do not assume CMake or CTest is on `PATH`. Use the Visual Studio bundled executables:

```powershell
$CMake = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$CTest = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
```

If either path does not exist, report the environment mismatch and consult the approved development setup task; do not fall back to bare `cmake` or `ctest`.

# Three-attempt stop protocol

For the same operation or blocking condition, make at most three total attempts. The first failure counts as attempt one. After the third failure, stop and tell the user:

- what failed and the relevant error;
- what each attempt did;
- what exact permission, device state, file, screenshot, credential, or user action is required to resume with the best approach.

Do not silently keep retrying or switch to a lower-quality workaround.

# Component build and handoff

- The primary deliverable is `FooPodBridge-<version>.fb2k-component`, containing the expected x64 `foo_pod_bridge.dll` and only approved package content.
- FooCrate integration changes produce a separate FooCrate prerelease component under the FooCrate repository's rules; never bundle `foo_crate.dll` into FooPodBridge.
- Every package handed to the user for testing receives a new SemVer prerelease version. Never overwrite a previous stable or prerelease package.
- Put FooPodBridge packages in this repository's `dist` directory, verify package contents, and report the exact path.
- The user manually imports acceptance candidates into `foobar-test`. Automated development deployment is limited to `foobar-dev`.

# Device-write safety

- The target device range is every non-iPod-touch iPod that Windows exposes as an accessible storage volume, including early full-size iPod, mini, photo/video/classic, nano, and shuffle families. Discovery is automatic; missing local hardware or an unimplemented database profile must reduce capability/evidence, not silently remove a family from the product target.
- Real-device support is evidence-tiered. Only a positively identified user-owned device named by the active device task may enter an experimental write path; public support claims must distinguish reference-backed, fixture-verified, read-verified, and write-verified models. A real device validates its format/profile variant and must not become a device-specific product configuration.
- Unknown devices remain read-only. A reference-backed but not write-verified model may be written only in an explicitly scoped experiment after its format profile, stable identity, recovery plan, and external backup are verified; there is no generic UI override.
- Before the first real write to each physical device, require and verify a recoverable backup as defined by `docs/SAFETY_MODEL.md` and the active task.
- UI code never writes device files directly. Every mutation goes through the core transaction service.
- Do not redistribute or load the reference `iTunesCrypt.dll`; the formal x64 project uses source-level, license-compatible implementations.

`docs/PROJECT_RULES.md` is the detailed project charter. This file is only the encoding-safe entry point.
