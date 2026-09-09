# iPod Nano 4 Restore 后纯净 Windows 基线

- 日期：2026-09-09
- 设备：用户自有 16 GB iPod Nano 4
- 状态：只读采集完成
- 到货前基线：[`NANO4_20260908_BASELINE.md`](NANO4_20260908_BASELINE.md)
- Restore 交接：[`NANO4_ITUNES_RESTORE_HANDOFF.md`](NANO4_ITUNES_RESTORE_HANDOFF.md)
- 私有数据：完整备份和 fixture 位于 Git 忽略目录，不随源码发布

## 1. 本次回答的问题

用户已在前一日把 Nano 4 恢复为新状态，并确认“用作磁盘”默认自动开启。本次把同一设备带到一台从未安装 iTunes 的 Windows 电脑，验证：

1. 没有 iTunes/Apple 运行时，Windows 是否仍直接挂载存储卷；
2. `iPod_Control` 和 Restore 后空 Library 是否可通过普通文件系统只读访问；
3. Restore 后状态能否形成电脑侧完整备份和任务 003/004 私有 fixture；
4. 只读检查是否保持设备源文件逐字节不变。

本次不测试 FooPodBridge 设备发现服务，因为任务 005 尚未实现；也不测试导入、初始化、签名写入或任何设备修改。

## 2. 纯净 Windows 环境证据

- 用户确认本机从未安装 iTunes；
- 未发现运行中的 iTunes、Apple Mobile Device、iPod Service 或 Bonjour 相关进程；
- Win32 卸载项未发现 iTunes、Apple Mobile Device Support、Apple Application Support、Bonjour 或 iPod 软件；
- 非管理员 `Get-Service` 未发现 Apple/iPod/Bonjour 服务；
- WMI 服务清单被当前权限拒绝，PnP disk inventory 也不可用，因此不把这两项写成自动证明；
- 不安装驱动、不启动软件、不分配盘符。

## 3. Windows 挂载结果

Windows 直接出现一个可访问的 removable FAT32 volume：

- 总容量约 15.03 GiB，可用空间约 15.01 GiB；
- 根下存在 `iPod_Control`；
- `iPod_Control` 顶层为 `Device`、`iTunes`、`Music`、`Speakable` 和 `Tones`；
- 没有 `Artwork` 目录；
- `Music` 中没有媒体文件；
- 整个 `iPod_Control` 共 15 个文件、21,046,250 字节。

结果分类为 `MountedByDefaultOnCleanWindows`：Restore 后没有依赖当前电脑上的 iTunes/Apple 软件即可获得 FooPodBridge 所需的普通存储卷和文件路径。

## 4. Restore 后数据库

`iPod_Control/iTunes/iTunesDB` 是 14,314 字节的未压缩传统数据库：

- marker 为 `mhbd`；
- header 长 244 字节；
- 根声明总长与文件实际长度一致；
- format 为 1；
- database version 字段为 115；
- 顶层 dataset 数为 5；
- 第一处观察的签名保留区为全零，第二处签名保留区非零；不输出具体字节或完整数据库 hash。

`Device/SysInfo` 仍为 0 字节，没有从卷内取得稳定设备 ID 或 `SysInfoExtended`。因此该 fixture 可以立即服务任务 003 的共同 Reader 和空库结构比较，但任务 004 的 hash58 输入与签名语义仍需独立证据。

## 5. 完整备份与私有 fixture

完整 `iPod_Control` 已只读复制到：

`device-backups/nano4-20260909-restored-clean-windows/iPod_Control`

Device/iTunes 私有 fixture 已复制到：

`tests/private-fixtures/nano4-20260909-restored-clean-windows`

验证结果：

- 源与完整备份：15 个文件逐一比较大小与 SHA-256，零差异；
- 采集前与采集后源设备：15 个文件逐一比较大小与 SHA-256，零差异；
- Device/iTunes 私有 fixture：13 个文件与源对应子集零差异；
- manifest 保存在完整备份的 Git 忽略目录；
- 全程没有设备文件写入、重命名、删除、修复或格式化。

## 6. 与到货旧库的脱敏差异

| 项目 | 2026-09-08 到货状态 | 2026-09-09 Restore 后 |
| --- | ---: | ---: |
| `iPod_Control` 文件数 | 232 | 15 |
| 总字节数 | 4,166,620,499 | 21,046,250 |
| Music 文件数 | 204 | 0 |
| `Artwork` 目录 | 有 | 无 |
| `iTunesDB` 字节数 | 308,628 | 14,314 |
| `mhbd` header | 244 | 244 |
| format | 1 | 1 |
| database version | 49 | 115 |
| 顶层 dataset 数 | 5 | 5 |
| 根声明长度匹配文件 | 是 | 是 |

这证明 Restore 生成了结构合法候选的零音乐传统数据库，而不是简单保留卖家 Library。它不证明 FooPodBridge 已能生成固件接受的签名数据库。

## 7. 对路线的影响

- iTunes/Apple 运行时不再是 Nano 4 日常读写路径的产品依赖；
- 磁盘模式存储卷仍是外部硬前置；没有卷时停在 `NotMounted`；
- 任务 003 现在有“非空到货库”和“Restore 后空库”两份 Nano 4 私有输入，可验证共同结构和 preserve-only 行为；
- 任务 004 可以比较同一设备两种真实数据库 header/签名保留区，但仍需稳定设备 ID 和 hash58 独立验证；
- 第一次真实写入仍由任务 007/008 的事务、备份、唯一测试曲目和明确授权阻断。
