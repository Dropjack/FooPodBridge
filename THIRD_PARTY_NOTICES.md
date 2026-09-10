# FooPodBridge third-party notices

This file records both the third-party source in the current build and audited source planned for later tasks. A planned entry is not part of the current binary until its status says otherwise.

## foobar2000 SDK 2025-03-07

- Status: used to build the foobar2000 adapter and component entrypoint.
- Copyright: Copyright (c) 2002-2025, Peter Pawlowski. All rights reserved.
- Source: <https://www.foobar2000.org/SDK>
- Audited archive SHA-256: `CCDA3C5840E66E0E28A7E4FE36407C4E78581AA30C40C362A188FCBAAE799A3E`
- License: [`LICENSES/LicenseRef-foobar2000-SDK.txt`](LICENSES/LicenseRef-foobar2000-SDK.txt)
- Distribution: SDK source remains in `third_party`; SDK binaries are not packaged. The component package contains only the FooPodBridge DLL and notices.

## PFC from foobar2000 SDK 2025-03-07

- Status: used as a source-built SDK support library.
- Copyright: Copyright (C) 2002-2024 Peter Pawlowski.
- Source: included in the official foobar2000 SDK archive above.
- License: zlib-style terms in [`LICENSES/Zlib.txt`](LICENSES/Zlib.txt).
- Distribution: source notice is retained in `third_party/pfc/pfc-license.txt`; no standalone PFC binary is packaged.

## libgpod hash58

- Status: used by the task 004 Core database target; the current component module does not yet link that target.
- Copyright: Copyright (C) 2007, Christophe Fergeau.
- Source: `gtkpod/libgpod`, commit `7982c5554f78dde47fd006afbeff659201d6db3d`, `src/itdb_hash58.c`.
- License: [`LICENSES/BSD-3-Clause.txt`](LICENSES/BSD-3-Clause.txt).
- Local adaptation: `src/core/database/hash58.cpp`; rewritten in C++20 without GLib and retaining the complete BSD-3-Clause notice.

## Columns UI SDK

- Status: audited and planned for task 016; not present in the task 002 binary.
- Source: `reupen/columns_ui-sdk`, audited commit `2ac32c02dcf4685c120e4a3a8eaf2ea58a7df89d`.
- License: `0BSD`; see [`LICENSES/0BSD.txt`](LICENSES/0BSD.txt).

No foo_dop source, Apple Mobile Device code, `iTunesCrypt.dll`, user device data, or foobar2000 program binary is included in FooPodBridge.
