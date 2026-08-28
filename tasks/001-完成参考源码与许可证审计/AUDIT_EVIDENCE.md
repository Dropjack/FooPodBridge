# 任务 001 审计证据

- 状态：自动检查完成，等待用户核对许可证结论
- 日期：2026-08-28
- 性质：只读来源与许可证审计；没有构建、部署或设备访问

## 1. 本地参考树身份

- 实际路径：`D:\dev\foo\FooPodBridge\Ref\ipod_manager`
- 上游：`https://github.com/reupen/ipod_manager.git`
- HEAD：`08e0657b5ee09bd05cdb60273e1a139205d4d3f6`
- HEAD 日期：2021-04-13
- 审计前后主仓库和六个子模块 `git status --short --untracked-files=all` 均为 `CLEAN`

子模块固定提交：

| 路径 | 提交 |
| --- | --- |
| `foobar2000` | `e7e3f6e82345cbe0cd7b2099f477c5a7d48d2345` |
| `ui_helpers` | `b8ab9af978b074f9165a464c306a406eaf2b8b17` |
| `pfc` | `fe33d087a7ecdb0a824c8ff25ea9fb186c589244` |
| `columns_ui-sdk` | `7b488e267ff9ec96e2ebb4120358618e73124684` |
| `mmh` | `6a3240db5043e10dcf9c1b58356d3450dfa65e6c` |
| `fbh` | `fb6f85d143da42c5206715c627ea54724cc1524d` |

## 2. 本地许可证与关键文件哈希

| 文件 | SHA-256 | 结论 |
| --- | --- | --- |
| `README.md` | `7011F215D8E0DC911A5F161B89C144B8ADF685E9382A9ADA17BC31DD7BBECC36` | 声明 `foo_dop` 为 LGPL、`dop-sdk` 为 0BSD |
| `foo_dop/COPYING` | `0B383D5A63DA644F628D99C33976EA6487ED89AAA59F0B3257992DEAC1171E6B` | GPLv3 全文 |
| `foo_dop/COPYING.LESSER` | `9B21BDD0AE731B11936677B9A1780C13B6BDE518A1DB6AE4074C8F0181FAF4FF` | LGPLv3 全文；仓库未另授予 `or-later` |
| `dop-sdk/LICENCE` | `D82C3E995BD48AF26672368FA72AE397874B707ABB012960EAEF9987932BD6A8` | 0BSD 正文 |
| `foobar2000/sdk-license.txt` | `AB3172654D0F77280AE50760AA2D31AA13A28316E7619A6DF4ED70161A11B9FB` | 2021 BSD 风格 SDK 许可 |
| `foo_dop/vendored/sdk-license.txt` | `5BE9705D3856B0305DA0F24801D422E340EC2DD99FD0F13C53CC42A1880B2C23` | 旧 vendored SDK 许可 |
| `foo_dop/writer_itunesdb.cpp` | `147E8BC1BF36B82D2D45345E077CFA7740567139299AA662E73C18BC98F03C27` | iTunesDB writer 与旧 hash 调用；只参考 |
| `foo_dop/itunesdb_track.cpp` | `C80CAD60F47118DC28FA69A4D761C0689D8F934D05F5A88F6F2C6D9241C092DF` | SoundCheck/track 字段；只参考 |
| `foo_dop/gapless_scanner.cpp` | `139746E4A6065FDDD3C22FFE362FAD4F8F86462FA4EE99B857CA1144583FA6E0` | gapless 调度；只参考 |
| `foo_dop/mp3.cpp` | `CB6C1E6BA2F63AECC43DD593E4DC3C7A21234A486BDCA44DF2B7D59AB53CFE94` | MP3 gapless；只参考 |
| `foo_dop/mp4.cpp` | `F05F5ED4AE6C406D6A145A59F4F224B14E20E2DD6AA7C0DEDCE30A46F9A67261` | MP4/AAC/章节；只参考 |
| `foo_dop/photodb.cpp` | `1505B5480D689620CB86BAC4D3EFE5584CFE02AFEF7C131DAD5B2BF4BAD51EC6` | ArtworkDB；只参考 |
| `foo_dop/smart_playlist_processor.cpp` | `956BBCEEBE1D78ECD5105A7E5325C6FE2815DB9A1B94F90386CF8EA5E52D5718` | Smart Playlist 成员计算；只参考 |
| `foo_dop/device_info.cpp` | `1A2E2C7DB043653F251D01211265E2CC0837AC3A70C18EF62ED0C187D9F55FFB` | 设备信息；只参考 |

完整候选文件名和采用方式记录在 [`../../docs/REFERENCE_PROVENANCE.md`](../../docs/REFERENCE_PROVENANCE.md)；commit 固定了未逐项列出哈希的其余文件内容。

## 3. 官方上游文件证据

### 3.1 libgpod commit `7982c5554f78dde47fd006afbeff659201d6db3d`

| 文件 | SHA-256 | 文件级许可证/用途 |
| --- | --- | --- |
| `src/itdb_hash58.c` | `796AAC0573EA3637635015A0A4050D79AE9EEA69117EAADE99736C8695779652` | BSD-3-Clause；唯一计划修改采用的算法 |
| `src/itdb_itunesdb.c` | `1562931AA7D57EC09F3F7E540CF759D9A0C938F62EC12FB85213FC376EFFB37C` | LGPL-2.1-or-later；数据库/Smart/SoundCheck/gapless 字段 |
| `src/itdb_playlist.c` | `F50A48E97574F07D7EBA07F542080B0DB529EA9CEEFC16F5575C4ED807E8E6AD` | LGPL-2.1-or-later；部分 Smart 段另称 FreeBSD license，但无精确正文 |
| `src/itdb.h` | `732108CA544D313940D64DEDCF254B930B1B7E2CE4F40C676ED7C65F963AB846` | LGPL-2.1-or-later；字段和公式说明 |
| `src/itdb_device.c` | `3D56150F89E1982E39612014EA526534DB64528842D3E24464E656C2FF6FAD18` | LGPL-2.1-or-later；设备表与能力 |
| `src/itdb_sysinfo_extended_parser.c` | `D90784AD39B1267D692558B8ED7E5D7C6AE309307CB0E08F9AA4FBC0560DB124` | LGPL-2.1-or-later；SysInfoExtended |
| `src/itdb_artwork.c` | `0E32C8C742FA04949DD1F3860039458A228B1A4C08D46CD6A61C98D7D10AA155` | LGPL-2.1-or-later；artwork 模型 |
| `src/db-artwork-parser.c` | `EE59AD39EB4A8E767B63362F4951011F11D4F6F9BFFE2B8BB2ED3544C8B2B169` | LGPL-2.1-or-later；ArtworkDB parser |
| `src/db-artwork-writer.c` | `91A53F3DCAE0D6F93EFA7C335297679A08359AEBD62E5FB5F2C692C07D9ED986` | LGPL-2.1-or-later；ArtworkDB writer |
| `README.overview` | `05EB1BDEA450F36938EA16A46A7AF71AC56548EAE37586A061CF55D97C38FD02` | hash58/hash72/SQLite 流程说明 |
| `README.SysInfo` | `434B5D0A03FD6A49F3F00AA148659F641E2C095C040F4662136C429DD16020B0` | FireWire ID 与 SysInfo 说明 |

关键交叉证据：

- `itdb_hash58.c` 的 BSD 文件头要求源码保留版权、条件和免责声明，二进制材料复制相同声明，并禁止作者名背书；
- `itdb.h` 第 1356–1367 行给出与 foo_dop 相同的 SoundCheck 公式；
- `itdb_itunesdb.c` 同时读写 SoundCheck、pregap、samplecount、postgap 和 Smart Playlist MHOD 50/51；
- 上游 `tests/` 没有独立 hash58 已知向量，因此任务 004 必须自建并交叉验证。

### 3.2 SDK

| 来源 | 身份 | SHA-256/结论 |
| --- | --- | --- |
| 官方 foobar2000 SDK 下载页 | 2025-03-07 | 官方包 SHA-256 `CCDA3C5840E66E0E28A7E4FE36407C4E78581AA30C40C362A188FCBAAE799A3E` |
| `reupen/foobar2000-sdk-modified` | commit `85f56870d5e8e5dc7bb8cd07179a44e3a6e5d834` | `sdk-license.txt` SHA-256 `99D689109655CE4B5858C000DCA0F616816CECDA812DE1073820F0E3EC557CD4`；只作许可交叉证据 |
| 官方 Columns UI SDK | commit `2ac32c02dcf4685c120e4a3a8eaf2ea58a7df89d` | `LICENCE` SHA-256 `3662A9ADFE6FEDDB077F96DFAD3CB30408540A2A3EAE5A656B7FF4375E622BE0`；0BSD |

官方 foobar2000 SDK 包提取在三次上限后停止：

1. curl 失败：`CRYPT_E_NO_REVOCATION_CHECK`；
2. curl 加 `--ssl-no-revoke` 后没有返回可验证输出；
3. .NET HTTPS 下载成功并确认包哈希，但 `bsdtar 3.5.2` 报 `LZMA codec is unsupported`，无法提取 `sdk-license.txt`。

继续该精确检查需要可用的 7-Zip，或用户提供从官方包解出的 `sdk-license.txt`。任务 002 首次构建前必须补做；本任务不再重试相同环境条件。

## 4. 禁止二进制证据

| 文件 | SHA-256 | 证据 |
| --- | --- | --- |
| `MobileDeviceSign/iTunesCrypt.dll` | `727C4C3A078145884E9ED8F906BD5C9292CE89E209727E90300B540BFA26D3E3` | 460,800 字节；PE machine `0x014C`（x86）；Authenticode `NotSigned`；版本/公司/产品/原始文件名为空 |
| `MobileDeviceSign/iTunesCrypt.lib` | `CA8AEFD856FC5CF3C0E66714EC1360DE440761423495886B7FD61EF9C7BCEA7B` | x86 DLL 导入库；没有独立授权 |
| `MobileDeviceSign/stdafx.h` | `96208A85A8E69F11BB66CD1EE75CAB2BE12965F81B4CE55AB780B27A2CD7706D` | 声明 CBK/hash58 DLL exports；没有独立授权 |

三者均不得加载、复制、链接、打包或发布。

## 5. 自动检查结果

- `Ref/ipod_manager` 主仓库：clean；
- 六个已检出的子模块：全部 clean；
- 没有在 `Ref` 创建、修改、构建、格式化或清理文件；
- 没有访问 foobar2000、FooCrate、真实设备或 C 盘日常安装；
- 没有创建 CMake/C++、组件包或 DLL；
- 项目文档修改后的 UTF-8/LF/链接检查记录在任务 README 的验证区。
