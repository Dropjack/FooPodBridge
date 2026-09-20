# 005 来源与判断记录

- 日期：2026-09-20
- 性质：规格依据与阶段证据；不代表实机验收

## 已核对的本地事实

| 证据 | 观察 | 对 005 的约束 |
| --- | --- | --- |
| IM：固定参考 device_info.cpp 第 362–363 行 | 列出 23 个非 touch 代际 | 注册表必须覆盖这些代际，不能只做 Photo/Classic/Nano 4 |
| IM：ipod_manager.cpp 第 216–300 行 | USB 前缀判断中也出现 touch/iPhone/iPad；另有 1394 路径 | 不能复用宽泛 Apple USB 前缀作正向非 touch 识别，不能遗漏 FireWire |
| IM：ipod_manager.cpp 第 420 行起 | 先枚举磁盘设备并查父节点，再建立卷关系 | 本项目原创实现物理设备到卷的核对，不以盘符作为身份 |
| 本地 service_v1.h | ABI 1.0；device_kind 只有 unknown/photo/classic，快照没有 Library 字段 | 用独立扩展接口补齐，保护既有 ABI |
| 本地 database/model.h | 已有 track/playlist 与 hash58 状态；track 尚无 Media Kind 投影 | 在只读模型中补齐必要字段，保留未知值 |
| 本地 database/format_profile.h | unsigned、preserve-only、hash58 三种 profile | 不假装已实现 Shuffle/Nano 5+ |
| FIX/TEST：任务 004 REPORT | 两份 Nano 4 fixture 通过结构/身份签名验证；5.5G 仍 preserve-only | 复用算法证据，不外推生产扫描、实机只读或可写结论 |

参考目录为 `D:\dev\foo\FooPodBridge\Ref`，本轮仅读取。采用范围遵守 [`REFERENCE_PROVENANCE.md`](../../docs/REFERENCE_PROVENANCE.md) 的 FD-DEV-01、LG-DEV-01：理解行为后原创实现，不复制参考表达或二进制。

## Windows 官方依据

查询日期：2026-09-20。

- Microsoft 的[设备接口到达/移除通知说明](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/registering-for-notification-of-device-interface-arrival-and-device-removal)要求注册通知后枚举已存在的接口，并指出两者之间到达的接口可能重复出现；本规格据此安排注册、枚举、去重以及移除时释放句柄。
- [IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddvol/ni-ntddvol-ioctl_volume_get_volume_disk_extents)返回卷所在物理磁盘范围，可能涉及多个磁盘；本项目不能把每个卷未经核对就假定为一台 iPod。

这些资料证明 API 的用途，不证明本项目已正确实现，也不证明当前用户设备可被识别。

## 实现与实机尚需验证

- 为注册表每个实际匹配条件补来源和去歧义测试；23 个名称齐全不等于匹配已完成。
- 早期 FireWire、固定磁盘标志、空 SysInfo、未知/矛盾身份、Nano/Shuffle 后续家族必须有模拟输入覆盖。
- 真实 Windows 权限、磁盘到卷映射、热插拔与不阻塞系统弹出，需要批准环境验证。
- 当前 Nano 4 的实时 Library、容量与签名核对，以及其他设备可用时的补充，尚未执行。
- 规格准备阶段没有检查已连接硬件；实施阶段曾启动只读发现的开发实例，后续用户截图只确认无设备页面，没有增加 DeviceReadVerified/DeviceWriteVerified 记录。

## 决策记录

- 2026-09-20：用户要求 fetch、检查全部任务路线并逐项继续。fetch 成功，HEAD 与 origin/main 相差 0/0，工作区开始时 clean。
- 本轮完成 005 规格、来源和验收方案整理；状态从“待讨论”推进为“规格核对中”。
- 用户随后回复“推进！没问题！”，明确批准规格；本轮已开始实现。构建遇到三次相同环境阻断，详见 VALIDATION 第 6 节；未取得新的实机或自动通过证据。
- 上条为阻断时的历史记录。用户再次授权后，构建环境已恢复，Debug/Release 各 11 项自动测试通过，详见 VALIDATION 第 7–8 节。
- 用户明确禁止 computer-use，所有软件操作验证交由用户；已同步根 AGENTS.md 和 PROJECT_RULES。
- 用户截图确认信息页加载与无设备状态，发现右侧控件裁切。用户未带连接线，要求 commit + push 保存当前实现；实机验证与完整任务验收仍未完成。
