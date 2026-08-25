# 参考资料与来源边界

- 状态：初始审计记录
- 日期：2026-08-25
- 只读参考根：`D:\Dev\FooPodBridge\Ref`

## 1. 使用原则

参考资料用于回答三类问题：

1. iPod 数据库、设备能力和算法的已知结构；
2. foo_dop 历史上已经实现和修复过的行为；
3. 正式 x64 重建应保留、重写或排除哪些路径。

正式工程不在 `Ref` 内修改代码或生成构建结果。采用任何源码前，先记录许可证、原始文件、采用方式和测试证据。

## 2. foo_dop / iPod manager

- 本地参考：`D:\Dev\FooPodBridge\Ref\ipod_manager`
- 上游：<https://github.com/reupen/ipod_manager>
- `foo_dop` 源码：仓库 README 声明 LGPL；实际采用前以仓库许可证全文和目标项目许可证决定为准。
- `dop-sdk`：仓库 README 声明 0BSD。

当前确认可用于理解的模块包括：

| 参考路径 | 知识用途 | 正式项目方向 |
| --- | --- | --- |
| `foo_dop/itunesdb*` | 传统 iTunesDB 曲目、playlist、字段和 writer | 重新分层为 Reader/Model/Writer/Validator |
| `foo_dop/writer_itunesdb.cpp` | 传统数据库序列化顺序 | x64 安全类型与往返测试重构 |
| `foo_dop/file_adder*` | 导入、格式和元数据处理 | 只保留手动音乐导入所需行为 |
| `foo_dop/gapless_scanner.*`、`mp3.cpp`、`mp4.cpp` | MP3/AAC delay、padding、sample count | Classic 准确 gapless，Photo 能力受限 |
| `foo_dop/smart_playlist_*`、`itunesdb_playlist.cpp` | Apple Smart Playlist 规则和内容 | 原生编辑器与设备能力验证 |
| `foo_dop/photodb.*` | 音乐封面所需 ArtworkDB 知识 | 只管理音乐 artwork，不做 Photo 同步 |
| `foo_dop/mobile_device_*` | iPhone/iPod touch 历史路径 | 正式项目排除 |
| `foo_dop/config_ios.*` | Apple Mobile Device 配置 | 正式项目排除 |
| `foo_dop/panel.cpp` | 旧 Columns UI Panel 行为 | 只作历史行为参考，不移植 UI 架构 |

## 3. `iTunesCrypt.dll`

本地文件：`D:\Dev\FooPodBridge\Ref\ipod_manager\MobileDeviceSign\iTunesCrypt.dll`

已确认事实：

- PE 架构为 x86；
- 未签名；
- 没有 Apple 公司、产品、版本或原始文件名元数据；
- 导出 hash58 与 CBK 相关函数；
- 上游 README 只说明旧构建需要仓库内这份 DLL，没有说明单独来源与授权；
- 它不是从本地 iTunes 9 安装中提取的正式 Apple DLL。

正式决定：

- 不加载、不复制、不打包、不发布；
- 不建立 x64 sidecar 绕过架构；
- Classic hash58 使用可审计、许可证兼容的源码实现；
- Nano 5 的 hash72/CBK 不在当前实机支持范围。

## 4. libgpod

- 上游：<https://github.com/gtkpod/libgpod>
- 用途：交叉验证设备能力、传统 Library/playlist 关系、ArtworkDB、hash58 和设备识别资料。
- 许可证：LGPL；采用具体源码前必须记录文件级版权头、许可证版本和修改。
- 项目不默认把整个 libgpod 作为运行时依赖。优先提取所需、可测试、许可证兼容的算法和格式知识，避免引入未批准的 GLib 等运行时依赖。

特别有用的上游资料：

- `README.overview`：设备数据库、hash58/hash72 与型号矩阵；
- `README.SysInfo`：Classic/Video Nano 的 FireWire ID；
- `src/itdb_hash58.c`：开源 hash58 实现；
- `src/itdb_itunesdb.c`、`src/itdb_device.c`：数据库与设备能力；
- `tests/`：格式和设备工具行为。

## 5. foobar2000 与 UI SDK

- 正式构建只使用许可和仓库策略允许的 foobar2000 SDK、Columns UI SDK 及必要支持库；
- 不从 `Ref\ipod_manager` 直接沿用 2021 年旧 SDK 作为现代基线；
- SDK 版本、来源和仓库布局在工程建立任务中冻结；
- 不提交 foobar2000 程序二进制或用户组件。

## 6. iTunes 9 与 Apple Mobile Device Support

本地 iTunes 9 可能包含 `iTunesMobileDevice.dll`、CoreFoundation 等 iPhone/iPod touch 历史依赖，但这些不提供 foo_dop 的 `iTunesCrypt.dll`，并且不属于 Photo/Classic 磁盘模式目标。正式项目不从 iTunes 安装目录复制运行库。

## 7. 来源记录模板

后续采用参考源码时，在本文件追加：

| 项目 | 必填内容 |
| --- | --- |
| 上游项目与提交 | URL、commit/tag、取得日期 |
| 原始文件 | 精确相对路径 |
| 许可证 | SPDX 或许可证全文位置 |
| 采用方式 | 原样、修改、重写或仅参考格式 |
| 正式文件 | FooPodBridge 中的目标路径 |
| 验证 | fixture、已知向量和实机结果 |

没有完整来源记录的第三方代码不能进入正式组件。
