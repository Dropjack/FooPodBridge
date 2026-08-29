# Third-party source inventory

`foobar2000/` and `pfc/` are unmodified source directories copied from the official foobar2000 SDK 2025-03-07 archive.

- Official archive: `SDK-2025-03-07.7z`
- Archive SHA-256: `CCDA3C5840E66E0E28A7E4FE36407C4E78581AA30C40C362A188FCBAAE799A3E`
- Acquisition and verification date: 2026-08-29
- Official files retained here: 571; this inventory is the only project-owned file in `third_party`
- SDK license SHA-256: `2AA8AF2F2A0CCE2DCE4C2A4F422BCBD1752DAB7D1E1B973981B77C971B0B8A32`
- PFC license SHA-256: `23963B0CC6A50E505CBB01CB64CAC91A0BD8067EB2410518E5F24DEEEC51DAD4`

The official archive and user-provided extraction staging remain outside the formal repository under `D:\dev\foo\FooPodBridge\.local\downloads`. The archive is not committed. `libPPUI` is not copied because task 002 has no UI dependency. The official archive's three prebuilt `shared-*.lib` files and `foo_input_validator.dll` sample binary are deliberately excluded; FooPodBridge builds required SDK support from source and packages none of it separately.

Do not modify third-party files to make project code compile. Adapt project-owned CMake and adapter code instead.
