# iPod Nano 4：iTunes Restore 与无 iTunes 复测交接任务

- 状态：已结束；用户已确认 `MountedByDefault`，产品决定不再继续其他磁盘模式情形的采集
- 建立日期：2026-09-08
- 结束日期：2026-09-09
- 设备：2026-09-08 已建立到货基线的用户自有 16 GB iPod Nano 4
- 到货证据：[`NANO4_20260908_BASELINE.md`](NANO4_20260908_BASELINE.md)
- 本任务授权：历史只读采集授权随任务结束；本文不再授权 Codex 访问当前连接的设备

## 0. 结束记录

用户于 2026-09-09 确认：前一日已完成测试，新状态 Nano 4 接入电脑时默认自动开启“用作磁盘”。本产品将结果记为 `MountedByDefault`。

用户同时冻结了更重要的产品边界：若任何设备默认不开启磁盘使用，或用户、设备、外部软件后续关闭了它，FooPodBridge 不负责配置、启用、恢复或绕过该状态。未获得 Windows 可访问存储卷的设备直接停在 `NotMounted`。FooPodBridge 是音频管理组件，不是 iTunes 的 iPod 配置替代品。

本地 Git 忽略目录中只存在 `nano4-20260908-original` 基线，没有本文原计划的 Restore 后备份或新 fixture。因此，这里只记录用户可见设置的实机结论，不声称本文原定的目录、数据库和 SHA-256 全套采集已完成。任务因产品决定消除了继续采集的必要性而结束；以下步骤仅作历史试验设计保留，不再执行。

## 1. 这次实验要回答什么

一次 Restore 分别观察三个问题，不能混成一个结论：

1. iTunes Restore 为 Nano 4 创建了哪些分区、目录、设备属性和数据库文件？
2. 没有主动选择“作为磁盘使用”时，退出 iTunes并重新连接后，Windows 是否仍暴露存储卷？
3. 若默认不暴露，明确开启“作为磁盘使用”后是否暴露？

在安装了 iTunes 的电脑上退出 iTunes，只能证明“iTunes 进程未运行时可访问”，不能证明“电脑从未安装 Apple 驱动”。真正的恢复后纯净 Windows 复测，需要之后把设备接回 2026-09-08 已验证没有相关 Win32 软件/服务的电脑。

## 2. 为什么现在可以 Restore

到货状态已经完整保存：

- `device-backups/nano4-20260908-original/iPod_Control` 保存完整 232 文件基线；
- 源与备份共 4,166,620,499 字节，逐文件 SHA-256 零差异；
- `tests/private-fixtures/nano4-20260908-original` 保存 Device/iTunes/Artwork 私有 fixture；
- 两个目录均由 Git 忽略，不会发布卖家曲库、设备名称或数据库。

Restore 会擦除设备信息并重新安装 iPod 软件。此处授权的是用户在官方 iTunes UI 中执行 Restore，不是授权 Codex、脚本或 FooPodBridge 手工格式化、删除或改写设备。

## 3. 用户在 iTunes 电脑上的步骤

### A. 在连接 Nano 前阻止自动同步

1. 先不连接 Nano，打开 iTunes。
2. 进入 `Edit > Preferences > Devices`。
3. 勾选 `Prevent iPods, iPhones, and iPads from syncing automatically`，保存设置。
4. 确认 iTunes 当前曲库不会在设备接入时自动同步。

Apple 说明该选项会阻止设备连接时自动与当前 iTunes Library 同步：

- [Change Devices preferences in iTunes on PC](https://support.apple.com/guide/itunes/change-devices-preferences-in-itunes-on-pc-itns34267c28/windows)

### B. 执行 Apple Restore

1. 使用稳定的主板 USB 口和数据线连接本次点名的 Nano 4。
2. 在 iTunes 中选择设备，进入 `Summary`。
3. 可记录型号、容量和 Software Version；不要把完整序列号写进聊天或项目文档。
4. 点击 `Restore iPod` 并确认。
5. 等待下载、擦除、固件安装、设备重启和 iTunes 最终完成；中途不拔线、不退出 iTunes。
6. 若出现新 iPod 设置页，选择设置为新设备；名称使用不含真人信息的实验名称，例如 `FooPodBridge Nano4 Lab`。
7. 不同步音乐、视频、Podcast、照片或游戏，不恢复卖家的 Library。

只能使用 iTunes 的 `Restore iPod`，不能使用 Windows 的“格式化”命令。Apple 对 Restore 的定义是擦除设备并重新安装 iPod 软件：

- [Restore your iPhone, iPad, or iPod to factory settings using a computer](https://support.apple.com/en-us/118107)

### C. 保留“磁盘使用”默认值

这是本实验最重要的控制变量：

1. 在首次设置页或 Summary 中，记录 `Enable disk use`/`作为磁盘使用` 当前是勾选、未勾选还是灰色不可改。
2. 第一轮不要主动改变它。
3. `Manually manage music` 也不要主动开启，因为它可能联动磁盘使用；若 iTunes 强制或默认开启，记录真实状态，不要为了符合预期反复切换。
4. 使用 iTunes 的 Eject 按钮安全弹出。
5. 完全退出 iTunes；等待窗口和 `iTunes.exe` 进程结束。
6. 重新插入 Nano，保持 iTunes 关闭；若系统自动启动 iTunes，立即退出，期间不要点击 Sync/Apply。

## 4. 第一轮交给 Codex 的固定提示

恢复并重新连接后，把下面文字原样或等价地发给 Codex；如果实际复选框状态不同，补充真实状态：

> Nano 4 已由 iTunes 执行 Restore，并设置为新 iPod；没有同步任何内容，也没有主动修改“作为磁盘使用”或“手动管理音乐”。iTunes 已完全退出。允许只读检查当前 Windows 卷、设备目录和数据库，并把恢复后完整基线及 Device/iTunes/Artwork 私有 fixture 复制到项目的 Git 忽略目录；禁止任何设备写入、格式化、删除、重命名或修复。

## 5. 第一轮 Codex 操作规范

Codex 必须按仓库启动协议读取规则，然后执行：

1. 检查 `iTunes.exe` 是否仍在运行，只报告进程状态；不结束系统服务、不卸载软件。
2. 只读枚举 Windows 文件系统卷，确认是否出现含 `iPod_Control` 的 FAT32 removable volume。
3. 若没有卷，记录为 `NotMountedAfterRestoreDefault` 并停止；禁止自行分配盘符、格式化、启动 iTunes或打开私有设备控制命令。
4. 若有卷，只读记录容量、文件系统、`iPod_Control` 顶层目录，以及 Device/iTunes/Artwork 文件名与大小。
5. 检查主数据库是不存在、零长度、传统 `iTunesDB`、`iTunesCDB` 还是损坏候选；只解析公开安全的 header，不在聊天输出设备 ID、数据库 hash、名称和媒体元数据。
6. 将完整恢复后基线复制到：
   `device-backups/nano4-20260908-itunes-restored-default/iPod_Control`
7. 将 Device/iTunes/Artwork 私有 fixture 复制到：
   `tests/private-fixtures/nano4-20260908-itunes-restored-default`
8. 对源与完整备份逐文件进行大小和 SHA-256 比较，要求零差异；生成的 manifest 留在 Git 忽略目录。
9. 与 `nano4-20260908-original` 私有 fixture 做电脑侧差异报告，只公开文件存在性、大小、header 结构和计数差异。
10. 再次确认设备文件数、总字节数和主数据库指纹未因 Codex 检查而改变。

## 6. 如果第一轮没有存储卷

只有 Codex 明确记录 `NotMountedAfterRestoreDefault` 后，用户才执行第二轮变量切换：

1. 打开 iTunes并连接 Nano。
2. 在 Summary/Settings 中勾选 `Enable disk use`/`作为磁盘使用`；点击 Apply。
3. 不开启任何内容同步，不添加音乐。
4. 使用 iTunes Eject，完全退出 iTunes，再重新连接 Nano。
5. 发送以下提示：

> Nano 4 在 Restore 后默认没有暴露存储卷；我已在 iTunes 中只开启“作为磁盘使用”并 Apply，没有同步内容。iTunes 已完全退出。允许只读重复挂载、目录、数据库和私有 fixture 检查；禁止任何其他设备写入。

第二轮 Codex 使用不同目录：

- `device-backups/nano4-20260908-itunes-restored-disk-use/iPod_Control`
- `tests/private-fixtures/nano4-20260908-itunes-restored-disk-use`

Apple 的旧 iPod 文档说明：该选项用于让 iPod classic、Nano 或 Shuffle 作为磁盘显示；若复选框为灰色，设备可能已经处于可作磁盘使用的状态：

- [Set up iPod as a hard disk in iTunes on PC](https://support.apple.com/en-lamr/guide/itunes/itns3114/windows)

## 7. 回到纯净 Windows 后的第三轮

若 Restore 发生在另一台装有 iTunes 的电脑，之后把 Nano 带回 2026-09-08 的纯净 Windows 电脑，再发送：

> 这是已由另一台电脑上的 iTunes Restore 的同一台 Nano 4；当前电脑仍未安装 iTunes/Apple Device 软件。允许只读重复纯净 Windows 挂载、目录和数据库检查，并复制独立私有 fixture；禁止任何设备写入。

第三轮只需在设备状态未变化的前提下确认：

- Windows 是否直接挂载卷；
- 是否仍需要磁盘使用设置；
- 主数据库与 Restore 后 fixture 是否相同；
- 没有 iTunes 进程和 Apple 软件时，FooPodBridge 需要的只读路径是否完整存在。

## 8. 结果分类

| 结果 | 含义 | 下一步 |
| --- | --- | --- |
| `MountedByDefault` | Restore 后未主动启用磁盘使用也有卷 | 保存默认 fixture；无需第二轮开关测试 |
| `RequiresDiskUse` | 默认无卷，开启磁盘使用后有卷 | 产品说明把存储卷列为前置，不能声称能绕过该设置 |
| `NoVolumeAfterDiskUse` | 开启后仍无卷 | 停止；记录 USB、驱动或 Restore 异常，不用 Windows 格式化 |
| `InitializableNoDatabase` | 卷健康但没有主数据库 | 为任务 005/007 保存真正的 Apple Restore 后初始化样本 |
| `AppleEmptyDatabase` | Restore 创建了合法零曲目数据库 | 作为 Apple 生成的 golden fixture，与 FooPodBridge 空库比较 |
| `UnexpectedContent` | 设置为新设备后仍出现媒体或复杂 Library | 只读保存证据，先查 iTunes 是否自动同步，不覆盖或删除 |

## 9. 通过标准与停止条件

本交接任务完成要求：

- iTunes Restore 成功且设备能正常启动；
- 第一轮默认磁盘使用状态被准确记录；
- 至少得到 `MountedByDefault` 或 `RequiresDiskUse` 的明确结论；
- Restore 后源与电脑备份逐文件验证通过；
- 原始和 Restore 后私有 fixture 使用不同目录并均被 Git 忽略；
- 全程没有 Codex 发起的设备写入。

若 Restore 报错、设备循环重启、Windows 要求格式化、容量异常或磁盘使用开启后仍无卷，立即停止。保留错误原文和 iPod 屏幕状态，不连续尝试超过仓库规定的三次。

## 10. 本任务不做什么

- 不要求在 iTunes 中加入测试歌曲；
- 不把“退出 iTunes”误写成“电脑没有安装 iTunes/驱动”；
- 不把完整卖家曲库重新复制回设备；
- 不让 Codex 手工创建 `iPod_Control` 或 `iTunesDB`；
- 不提前执行 FooPodBridge 的空库初始化、导入、删除、artwork 或 playlist 写入测试。
