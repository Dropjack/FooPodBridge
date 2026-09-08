# 参考资料与来源边界

- 状态：任务 001 已验收
- 审计日期：2026-08-28
- 本次实际只读参考根：`D:\dev\foo\FooPodBridge\Ref`
- 项目参考根：`D:\dev\foo\FooPodBridge\Ref`
- 详细证据：[`../tasks/001-完成参考源码与许可证审计/AUDIT_EVIDENCE.md`](../tasks/001-完成参考源码与许可证审计/AUDIT_EVIDENCE.md)
- 历史行为中文蓝图：[`IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md`](IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md)

## 1. 最终审计结论

### 1.1 项目许可证边界

| 范围 | 结论 | 理由与约束 |
| --- | --- | --- |
| FooPodBridge 原创源码 | `LGPL-3.0-or-later` | 与 `DEC-LIC-001` 一致；任务 001 没有发现阻止该选择的正式依赖 |
| libgpod hash58 移植文件 | `BSD-3-Clause` | `src/itdb_hash58.c` 有独立、完整的三条款 BSD 文件头；必须保留作者声明、三项条件和免责声明 |
| FooPodBridge/FooCrate 共享服务合同 | 推荐 `0BSD`，待本任务验收确认 | 合同必须原创，只含 GUID、版本、数据结构与抽象接口；不得复制 LGPL Core、foo_dop `dop-sdk` GUID 或旧接口 |
| FooCrate 原创源码 | MIT | 继续保持现状；只消费 `0BSD` 服务合同和 foobar2000 SDK，不链接、复制或内嵌 FooPodBridge Core |
| foobar2000 SDK | `LicenseRef-foobar2000-SDK` | BSD 风格许可，但正文缺少标准 BSD-3-Clause 的二进制再分发条款，不能错误标成精确 SPDX `BSD-3-Clause` |
| Columns UI SDK | `0BSD` | 当前官方仓库的 `LICENCE` 正文与 SPDX `0BSD` 一致 |

顶层 `LGPL-3.0-or-later` 不能覆盖或改写第三方文件的原许可证。正式源码中的每个第三方文件必须使用自己的 SPDX 标识，不能声称全部文件只有一个许可证。

### 1.2 采用方式

- **允许修改采用**：仅计划采用 libgpod `src/itdb_hash58.c` 的算法和常量，移植到独立 C++ 文件；保留 BSD-3-Clause 文件头，不引入 GLib，SHA-1 使用本项目原创实现或 Windows 系统密码学封装。
- **允许 SDK 方式使用**：任务 002 使用官方 foobar2000 SDK；任务 016 使用当前官方 Columns UI SDK。SDK 只存在于组件适配层，Core 不依赖 SDK。
- **只参考知识后原创实现**：foo_dop 的数据库、导入、SoundCheck、gapless、artwork、设备识别、删除、Audiobook 和 Smart Playlist；libgpod 除 hash58 外的数据库、设备与 artwork 源码。
- **不需要采用**：旧 `dop-sdk` 虽是 0BSD，但它只描述 foo_dop 的旧服务和 GUID；FooPodBridge 必须建立新的版本化服务合同。
- **禁止使用**：`iTunesCrypt.dll/.lib`、`iPhoneCalc.h` 调用链、Apple Mobile Device/iPhone/iPod touch 路径、hash72/CBK、来源不明二进制、旧 panel UI 架构和旧 SDK 副本。

## 2. 固定上游版本

| 项目 | 固定版本 | 取得日期 | 用途 |
| --- | --- | --- | --- |
| [reupen/ipod_manager](https://github.com/reupen/ipod_manager/tree/08e0657b5ee09bd05cdb60273e1a139205d4d3f6) | commit `08e0657b5ee09bd05cdb60273e1a139205d4d3f6`，2021-04-13 | 2026-08-28 | 本地 foo_dop/dop-sdk 历史实现 |
| [gtkpod/libgpod](https://github.com/gtkpod/libgpod/tree/7982c5554f78dde47fd006afbeff659201d6db3d) | commit `7982c5554f78dde47fd006afbeff659201d6db3d`，2012-05-04 | 2026-08-28 | hash58、格式、设备、SysInfo 与交叉验证 |
| [foobar2000 SDK](https://www.foobar2000.org/SDK) | 官方发布 `2025-03-07`；包 SHA-256 `CCDA3C5840E66E0E28A7E4FE36407C4E78581AA30C40C362A188FCBAAE799A3E` | 2026-08-28 | 任务 002 的 x64 组件适配层基线 |
| [reupen/foobar2000-sdk-modified](https://github.com/reupen/foobar2000-sdk-modified/tree/85f56870d5e8e5dc7bb8cd07179a44e3a6e5d834) | commit `85f56870d5e8e5dc7bb8cd07179a44e3a6e5d834` | 2026-08-28 | 只用于交叉核对 2025 SDK 许可正文，不作为正式 SDK 来源 |
| [reupen/columns_ui-sdk](https://github.com/reupen/columns_ui-sdk/tree/2ac32c02dcf4685c120e4a3a8eaf2ea58a7df89d) | commit `2ac32c02dcf4685c120e4a3a8eaf2ea58a7df89d`，2026-07-13 | 2026-08-28 | 任务 016 的 Columns UI 适配层基线 |

libgpod 官方镜像没有标签，因此使用审计日 `main` 的精确 commit。foobar2000 SDK 使用官方日期版本和包哈希，不使用 `Ref` 中 2021 年旧 SDK 作为正式基线。

## 3. 文件级来源矩阵

### 3.1 foo_dop

上游 README 将整个 `foo_dop` 目录声明为 LGPL，并附 GPLv3 与 LGPLv3 全文。候选普通源文件没有逐文件授权头，仓库也没有明确写出“or any later version”。因此，若复制这些文件，必须保守按 `LGPL-3.0-only` 处理；本项目决定不复制，统一采用“格式/行为知识 + 原创实现 + 独立测试”。

| ID | 精确参考文件 | 后续用途 | 采用结论 | 计划正式目标 | 独立验证 |
| --- | --- | --- | --- | --- | --- |
| FD-DB-01 | `foo_dop/itunesdb.h`、`itunesdb.cpp`、`itunesdb_helpers.cpp`、`reader.cpp`、`reader.h`、`writer_itunesdb.cpp` | iTunesDB 记录、端序、Reader/Writer | 只参考知识；不复制 | `src/core/database/*` | 脱敏/私有家族 fixture、损坏/截断测试、无修改往返 |
| FD-DB-02 | `foo_dop/itunesdb_track.cpp`、`itunesdb_mappings.cpp` | 曲目字段、Media Kind、SoundCheck、gapless 字段 | 只参考知识；不复制 | `src/core/database/*`、`src/core/media/*` | 与 libgpod 字段说明交叉核对；模型 round-trip |
| FD-PL-01 | `foo_dop/itunesdb_playlist.cpp` | master/普通/Smart playlist 关系 | 只参考知识；不复制 | `src/core/database/*` | fixture 引用完整性、排序、空列表测试 |
| FD-IMP-01 | `foo_dop/file_adder.cpp`、`file_adder.h`、`file_adder_helpers.cpp` | 历史导入检查、元数据与失败行为 | 只参考行为；不继承同步/转码架构 | `src/core/media/*`、`src/core/transaction/*` | 规格驱动的格式、空间、重复、故障注入测试 |
| FD-SC-01 | `foo_dop/itunesdb_track.cpp` 第 337–357 行 | ReplayGain 到 SoundCheck：`1000 * 10^(-0.1 * gain_dB)` | 只参考公式；原创实现 | `src/core/media/soundcheck.*` | libgpod `itdb.h` 第 1356–1367 行为第二来源；数值向量和边界测试 |
| FD-GAP-01 | `foo_dop/gapless_scanner.cpp/.h`、`gapless.cpp/.h`、`mp3.cpp`、`mp4.cpp/.h` | MP3/AAC delay、padding、sample count | 只参考知识；不复制 | `src/core/media/gapless/*` | LAME/iTunSMPB 样本、连续专辑、foobar 解码 sample count、损坏文件 |
| FD-GAP-02 | `foo_dop/vendored/mp3_utils.cpp/.h`、`vendored/sdk-license.txt` | 旧 foobar SDK MP3 helper | 禁止从旧副本采用 | 无 | 任务 002 只允许官方现代 SDK；Core 自有解析器保持 SDK 无关 |
| FD-SPL-01 | `foo_dop/smart_playlist_editor.cpp/.h`、`smart_playlist_processor.cpp/.h`、`itunesdb_playlist.cpp` | Smart Playlist 字段、运算符、编辑与成员计算 | 只参考知识；不复制 UI/算法 | `src/core/database/smart_playlist/*` 与共享编辑模型 | libgpod 交叉验证、格式往返、未知规则保留、型号级实机能力矩阵 |
| FD-ART-01 | `foo_dop/photodb.cpp/.h` | ArtworkDB、图像记录和 `.ithmb` | 只参考知识；不复制 | `src/core/database/artwork/*`、`src/core/media/artwork/*` | libgpod artwork 文件、脱敏 fixture、图像尺寸/端序与共享引用测试 |
| FD-DEV-01 | `foo_dop/device_info.cpp`、`plist.cpp/.h`、`ipod_manager.cpp/.h`、`ipod_scanner.cpp/.h` | Windows 磁盘设备、SysInfo、能力属性 | 只参考行为；原创 Windows 实现 | `src/core/device/*` | Windows 卷 fixture、真实设备只读证据、libgpod `itdb_device.c` 交叉验证 |
| FD-DEL-01 | `foo_dop/file_remover.cpp/.h`、`remove_files.h`、`maintenance.h` | 删除与引用关系 | 只参考行为；不继承历史直接写路径 | `src/core/transaction/*` | DB-first 故障注入、playlist 引用、未知 orphan 不删除 |
| FD-AUD-01 | `foo_dop/chapter.h`、`mp4.cpp`、`itunesdb_track.cpp` | Audiobook、章节、bookmark、shuffle 字段 | 只参考知识；不复制 | `src/core/media/audiobook/*` | M4B/MP4 章节样本、Media Kind 往返、设备能力测试 |
| FD-UI-01 | `foo_dop/panel.cpp` | 旧 Columns UI panel | 禁止移植；仅历史 UX 参考 | 无 | 新 UI 只消费版本化服务 |
| FD-IOS-01 | `foo_dop/config_ios.*`、`mobile_device_*`、`mobile_device_v2.*` | iPhone/iPod touch/Apple Mobile Device | 产品范围外，禁止使用 | 无 | 源码、构建依赖和包内容扫描 |
| FD-CRYPT-01 | `foo_dop/iPhoneCalc.h`、`writer_sqlite.cpp` 的 hash72/CBK、`MobileDeviceSign/*` | 旧签名桥和闭源二进制 | 禁止使用 | 无 | x64 依赖/导入表/包内容扫描 |

历史配置 `use_dummy_gapless_data=true` 与本项目 `DEC-MEDIA-002` 冲突，明确不得继承。历史同步、自动弹出、iPhone、Video、Podcast 和转码路径同样不能作为默认行为进入正式项目。

### 3.2 dop-sdk

`dop-sdk/LICENCE` 与 SPDX `0BSD` 正文一致，但这些文件只定义旧 foo_dop API：`dop.h`、`device_identifiers.*`、`ipod_manager.*`、`playback_data.*`。任务 002 不采用任何旧 GUID、版本号、类名或回调数据结构；共享服务合同从产品规格原创设计并使用新的身份。因“不需要”而不采用，不是许可证不兼容。

### 3.3 libgpod

libgpod 根 `COPYING` 是旧 LGPLv2 全文，但实际候选文件的文件头优先：数据库、device 和 artwork 文件明确授予 `LGPL-2.1-or-later`；hash58 文件独立授予 BSD-3-Clause。

| ID | 精确上游文件 | 文件许可证 | 采用结论 | 计划正式目标 | 验证 |
| --- | --- | --- | --- | --- | --- |
| LG-H58-01 | [`src/itdb_hash58.c`](https://github.com/gtkpod/libgpod/blob/7982c5554f78dde47fd006afbeff659201d6db3d/src/itdb_hash58.c) | `BSD-3-Clause`；Christophe Fergeau，基于 wtbw proof-of-concept | **修改采用**；保留完整文件头，去除 GLib/API 耦合 | `src/core/database/hash58.cpp`、`.h` | 上游交叉实现、自建已知向量、错误 FireWire ID、DB 篡改、实机 Classic 接受 |
| LG-DB-01 | `src/itdb_itunesdb.c`、`src/itdb.h` | `LGPL-2.1-or-later`；并注明部分知识来自 gnupod `mktunes.pl` | 只参考格式知识；不复制 | `src/core/database/*` | 与 foo_dop 和 fixture 三方交叉验证 |
| LG-PL-01 | `src/itdb_playlist.c`、`src/itdb.h` Smart Playlist 段 | 文件整体 `LGPL-2.1-or-later`；部分段落称可用 “FreeBSD license” 但未附精确正文/SPDX | 不依赖含糊的额外授权；只按 LGPL 来源作知识参考，不复制 | `src/core/database/smart_playlist/*` | 规则往返、成员计算、实机 Live 能力 |
| LG-DEV-01 | `src/itdb_device.c`、`src/itdb_sysinfo_extended_parser.c`、`README.SysInfo`、`README.overview` | 源码 `LGPL-2.1-or-later`；README 为文档证据 | 只参考型号、FireWire ID 与能力知识 | `src/core/device/*` | 官方设备文件、点名设备只读证据、隐私遮蔽 |
| LG-ART-01 | `src/itdb_artwork.c`、`src/db-artwork-parser.c`、`src/db-artwork-writer.c`、`src/itdb_photoalbum.c` | `LGPL-2.1-or-later` | 只参考格式知识；不引入 GLib/GdkPixbuf | `src/core/database/artwork/*` | foo_dop、fixture 和实机 artwork 三方验证 |

hash58 文件调用 GLib `GChecksum` 只是实现依赖，不改变其独立 BSD-3-Clause 授权。正式移植只采用该文件的算法和常量，替换内存、错误和 SHA-1 调用，不把 libgpod 或 GLib 作为运行时依赖。

### 3.4 foobar2000 SDK 与 Columns UI SDK

| 项目 | 允许使用 | 禁止/限制 | 分发要求 |
| --- | --- | --- | --- |
| foobar2000 SDK 2025-03-07 | 组件适配层、服务接口、metadb/artwork/async API；x64 构建 | 不进入 Core；不使用 `Ref` 的 2021 修改副本作正式基线；不分发 foobar2000 程序二进制 | 源码分发保留 SDK copyright、条件和免责声明；不得用作者名背书；源码清单用 `LicenseRef-foobar2000-SDK` |
| Columns UI SDK commit `2ac32c0…` | 任务 016 的独立 panel/toolbar 适配层 | 不复制旧 `panel.cpp`；不进入 Core/FooCrate | `0BSD` 无保留条件；仍在第三方清单记录版本和来源 |
| dop-sdk | 无正式用途 | 不采用旧 GUID/API | 不进入源码和包，因此没有分发义务 |

官方 foobar2000 SDK 包已固定版本和 SHA-256。2026-08-29，用户把官方 `.7z` 与解压目录放入仓库外的本地 staging；包 SHA-256 为 `CCDA3C5840E66E0E28A7E4FE36407C4E78581AA30C40C362A188FCBAAE799A3E`，与任务 001 固定值完全一致。WinRAR 7.13 对压缩包执行只读完整性测试返回 0；解压目录包含 674 个文件和全部必要 SDK 入口。官方 `sdk-license.txt` SHA-256 为 `2AA8AF2F2A0CCE2DCE4C2A4F422BCBD1752DAB7D1E1B973981B77C971B0B8A32`，与 FooCrate 当前官方 SDK 副本逐字节一致，许可证结论无需重开。

## 4. 任务 002–014 采用边界

| 任务 | 允许来源 | 正式采用方式 | 禁止项 |
| --- | --- | --- | --- |
| 002 工程与服务合同 | 官方 foobar2000 SDK；共享合同产品规格 | SDK 适配 + 原创 `0BSD` 合同 | 旧 SDK、dop-sdk GUID/API、空 UI |
| 003 Photo iTunesDB | FD-DB-01/02、LG-DB-01 | 原创 Reader/Model/Writer/Validator | 复制旧 parser/writer、引入 GLib |
| 004 Classic/hash58 | LG-H58-01、LG-DEV-01、FD-CRYPT-01 反例 | BSD-3-Clause hash58 移植 + 原创 Classic DB | `iTunesCrypt.dll/.lib`、hash72/CBK |
| 005 设备发现 | FD-DEV-01、LG-DEV-01 | 原创 Windows 只读服务 | Apple Mobile Device/iOS 路径、未识别设备写入 |
| 006 FooCrate 只读 UI | 原创 `0BSD` 服务合同、FooCrate MIT | 只消费快照 | LGPL Core/私有头复制到 FooCrate |
| 007 事务与恢复 | 产品安全模型 | 完全原创 | 继承 foo_dop 直接文件写路径 |
| 008 Photo Music 导入 | FD-IMP-01、FD-SC-01、数据库来源 | 规格驱动原创实现 | sync、自动转码、dummy gapless |
| 009 Classic Music 导入 | FD-GAP-01、LG-H58-01 | 原创 gapless + BSD hash58 移植 | 用历史成功路径代替实机接受测试 |
| 010 删除 | FD-DEL-01、数据库引用知识 | 原创 DB-first 事务 | 自动删除未知 orphan |
| 011 普通 playlist | FD-PL-01、LG-DB-01 | 原创模型与 writer | 发送 foobar playlist/autoplaylist 对象 |
| 012 artwork | FD-ART-01、LG-ART-01 | 原创、无 GLib/GdkPixbuf 的实现 | Photo Library 同步、复制旧 GDI+/GLib 架构 |
| 013 Audiobook | FD-AUD-01、数据库字段知识 | 原创 Media Kind/章节实现 | 扩展名/Genre 偷猜类型 |
| 014 Smart Playlist | FD-SPL-01、LG-PL-01 | 原创规则模型、writer、编辑器和成员计算 | 复制旧 UI/processor；把未验证规则宣称为 Live |

## 5. 正式仓库与包所需许可证文件

任务 002 建立工程时必须创建并自动检查：

1. `LICENSES/LGPL-3.0-or-later.txt`：FooPodBridge 原创文件许可证全文；
2. `LICENSES/BSD-3-Clause.txt`：hash58 文件许可证全文；
3. `LICENSES/0BSD.txt`：原创共享服务合同和 Columns UI SDK 的许可证全文；
4. `LICENSES/LicenseRef-foobar2000-SDK.txt`：来自官方 SDK 的原始许可正文；
5. `THIRD_PARTY_NOTICES.md`：至少记录 libgpod hash58、foobar2000 SDK、Columns UI SDK 的上游、版本、文件和版权；
6. 每个源码文件的 SPDX 头；修改采用的 hash58 文件保留原始版权/免责声明并追加本项目修改日期与说明；
7. `.fb2k-component` 至少包含 FooPodBridge 许可证和 hash58 的 BSD 二进制再分发声明；发布页提供对应源码和可重建说明。

foo_dop、libgpod 的 LGPL 数据库/artwork/device 文件因为没有复制，不进入二进制第三方声明；但必须永久保留在本来源文档中。若后续实现复制了受保护表达、表或大段结构，必须先重开任务 001。

## 6. 明确禁止清单

- `MobileDeviceSign/iTunesCrypt.dll`：x86 `PE_MACHINE=0x014C`、未签名、无公司/产品/版本/原始文件名、无单独来源和授权；
- `MobileDeviceSign/iTunesCrypt.lib` 与 `stdafx.h`：只为上述 DLL 提供导入/声明，同样禁止；
- `foo_dop/iPhoneCalc.h` 及 hash72/CBK 调用链；
- `foo_dop/config_ios.*`、`mobile_device_*`、Apple Mobile Device Support、iTunes 安装目录 DLL；
- Nano 5 hash72/CBK、iPhone/iPod touch、Video/Photo/Podcast 管理；
- `Ref` 中 foobar2000/Columns UI SDK 旧副本作为现代正式依赖；
- 来源不明二进制、用户设备数据库、完整序列号、FireWire ID 或未脱敏 fixture。

## 7. 技术证据仍需后续任务完成

- hash58：上游没有独立已知向量；任务 004 必须建立公开/自建向量、错误 ID、篡改检测，并最终由脱敏 Classic fixture 与实机接受交叉验证；
- iTunesDB：任务 003/004 用两类脱敏 fixture 证明未知字段保留、损坏拒绝和无修改往返；
- gapless：任务 009 使用 MP3/AAC 连续曲目和编码器元数据验证，扫描失败不得写 dummy；
- Smart Playlist：任务 014 只把目标型号实机验证过的规则组合标记为 Live；
- SDK：官方 `SDK-2025-03-07.7z` 的包哈希、WinRAR 完整性测试、必要文件和包内 `sdk-license.txt` 已于任务 002 复核通过；
- 设备能力与 artwork：任务 005/012 以只读设备证据和脱敏 fixture 校正历史型号表，历史源码不能单独证明支持。

## 8. 审计规则

后续任何第三方源码进入正式目录前，必须同时具备：精确上游 URL、commit/tag、原始文件、文件许可证、采用方式、正式目标、保留声明和独立验证。缺少任一项就拒绝进入构建与组件包。

本文件继续作为许可证和文件级采用方式的唯一依据；中文蓝图负责把已批准的“只参考知识后原创实现”转换成跨任务目标流程和源码导航，不能反向扩大本文件允许的复用范围。
