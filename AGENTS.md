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

- The formal project is this directory: `D:\Dev\FooPodBridge\FooPodBridge`.
- The sibling directory `D:\Dev\FooPodBridge\Ref` is read-only reference material. Never modify, format, build into, or clean it.
- `D:\Dev\FooCrate` is a separate repository and the integration/test workspace. Protect its existing changes and follow its own `AGENTS.md` before touching it.
- Never access, install into, launch, or modify the user's daily foobar2000 installation on drive C:.

# Local foobar2000 test instances

- Codex/AI development deployment, automation, diagnostics, and smoke tests may use only `D:\Dev\FooCrate\.local\foobar-dev`.
- User acceptance, clean install, upgrade, rollback, and multi-scenario testing use only `D:\Dev\FooCrate\.local\foobar-test`.
- Do not create a third foobar2000 installation under FooPodBridge.
- A task must explicitly authorize real iPod writes. UI automation, discovery tests, or read-only tests never imply permission to modify an attached device.

# Local build tools

Do not assume CMake or CTest is on `PATH`. Use the Visual Studio bundled executables:

```powershell
$CMake = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$CTest = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
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

- Only explicitly supported and positively identified user-owned iPod Photo and iPod Classic devices may be written.
- Unknown or unverified devices are read-only and must be rejected by every write path.
- Before the first real write to each physical device, require and verify a recoverable backup as defined by `docs/SAFETY_MODEL.md` and the active task.
- UI code never writes device files directly. Every mutation goes through the core transaction service.
- Do not redistribute or load the reference `iTunesCrypt.dll`; the formal x64 project uses source-level, license-compatible implementations.

`docs/PROJECT_RULES.md` is the detailed project charter. This file is only the encoding-safe entry point.
