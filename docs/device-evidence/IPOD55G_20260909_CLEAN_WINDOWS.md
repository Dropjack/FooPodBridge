# iPod 5.5G 纯净 Windows 只读基线

- 日期：2026-09-09
- 设备：用户称为 iPod 5.5G 的自有实机
- 状态：存储卷、目录、数据库 header 和私有 fixture 只读采集完成
- 当前分类：真实传统 `iTunesDB` 输入；精确型号、固件、存储改装和可写 profile 待后续核实
- 私有数据：fixture 位于 Git 忽略目录，不随源码发布

## 1. 本次目标与授权

用户把 Nano 4 与其称为 iPod 5.5G 的设备带到一台从未安装 iTunes 的 Windows 电脑，用于还原并完成此前计划的纯净 Windows 只读测试。本轮只回答：

1. 没有 iTunes/Apple 运行时是否仍直接出现存储卷；
2. 是否存在可读的 `iPod_Control` 和传统数据库；
3. 数据库能否成为任务 003 的真实共同 Reader 输入；
4. 能否只复制 Device/iTunes/Artwork 私有 fixture 且保持源文件不变。

本轮没有授权或执行设备写入、数据库修复、盘符分配、格式化、完整媒体备份或真实导入。

## 2. 纯净 Windows 与挂载结果

环境证据沿用同日 Nano 4 复测：

- 用户确认本机从未安装 iTunes；
- 未发现运行中的 iTunes/Apple 相关进程；
- Win32 卸载项未发现 iTunes、Apple Mobile Device Support、Apple Application Support、Bonjour 或 iPod 软件；
- 非管理员 `Get-Service` 未发现 Apple/iPod/Bonjour 服务；
- WMI 服务与 PnP disk inventory 因当前权限不可用，不把它们写成自动证明。

设备直接出现一个可访问的 removable FAT32 volume：

- 总容量约 119.00 GiB，可用空间约 94.67 GiB；
- 根下存在 `iPod_Control`；
- 顶层目录为 `Artwork`、`Device`、`Games_RO`、`iTunes`、`Music` 和 `Tones`；
- `iPod_Control` 共 1,882 个文件、26,102,199,731 字节；
- `Music` 共 1,862 个媒体文件、25,631,796,892 字节；
- 不记录卷标、设备名称、曲目名、playlist 名、完整路径、序列号或数据库 hash。

119 GiB 容量与存储介质/转接方式需要用户后续确认；本轮不根据容量猜测硬件改装，也不把营销名称直接映射为数据库 Writer。

## 3. 数据库与相关结构

主数据库为 `iPod_Control/iTunes/iTunesDB`：

- 文件大小 4,034,040 字节；
- marker 为 `mhbd`；
- header 长 244 字节；
- 根声明总长与文件实际长度一致；
- format 为 1；
- database version 字段为 49；
- 顶层 dataset 数为 5；
- 第一处观察的签名保留区为全零，第二处签名保留区非零；
- 没有 `iTunesCDB`。

`Device/SysInfo` 为 0 字节。当前没有从卷内取得精确型号、固件或稳定设备 ID，因此上述 header 只能证明“传统未压缩数据库结构可读”，不能单独证明数据库无需签名或设备已经可写。

Artwork 目录存在 `ArtworkDB` 和四个 `.ithmb` 文件；本轮只保存原始 fixture，不解析或修改 artwork。

## 4. 私有 fixture

以下目录已只读复制到：

`tests/private-fixtures/ipod55g-20260909-clean-windows-readonly`

范围仅包含 Device、iTunes 和 Artwork：

- 19 个文件；
- 470,402,770 字节；
- 源与 fixture 逐文件大小/SHA-256 零差异；
- 采集前与采集后源子集逐文件大小/SHA-256 零差异；
- 复制 Music 文件数为 0；
- manifest 保存在同一 Git 忽略目录。

本轮没有建立完整 `iPod_Control` 备份。若以后考虑对该设备进行任何写入，必须先确认精确型号、固件、119 GiB 存储实现、稳定身份、恢复方法和足够空间的设备外完整备份。

## 5. 对任务 003/004 的价值

- 任务 003 新增一份真实、大型、非空的传统 `iTunesDB`，可验证有界 Reader、引用规模、未知数据保留和 no-op 逐字节一致；
- 在精确 profile 证据完成前，该 fixture 使用 `TraditionalPreserveOnly`，任何修改必须返回 `ProfileNotWritable`；
- Reader/Validator 完成后，可以在电脑副本上判断共同 track/playlist 结构是否满足未签名 profile，不连接设备；
- 任务 004 可利用“第一处全零、第二处非零”的观察核对签名保留区语义；非零保留区本身不能直接等同于 hash58 要求；
- 该设备不因 fixture 采集获得 `DeviceReadVerified` 或 `DeviceWriteVerified`，因为正式 Reader、设备身份和用户 Library 对照尚未完成。

## 6. 后续最小检查

当前业务主线不需要立刻做完整备份或实机写入。后续只需在相关任务确认：

1. 用户确认设备的准确 Apple 代际、标称容量、固件和 119 GiB 存储改装方式；
2. 任务 003 Reader 对该 fixture 给出脱敏结构/引用报告；
3. 若它确属未签名传统家族，先只在电脑副本验证可写 profile；
4. 只有未来明确选择该设备做实机实验时，才建立完整外部备份、恢复计划和唯一写入动作。
