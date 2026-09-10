# 005-实现全目标家族注册表、Windows 自动发现与只读服务

- 状态：待讨论
- 日期：2026-09-10
- 前置任务：[`003-实现传统 iTunesDB 共同往返核心`](../003-实现iPodPhoto数据库往返核心/README.md) 与 [`004-实现 6G/hash58 与 Nano 4 格式核心`](../004-实现6G-hash58与Nano4格式核心/README.md) 已验收
- 对应路线：[`../TODO.md`](../TODO.md) 的任务 005
- 产品决定：[`../../decisions/USER_DECISIONS.md`](../../decisions/USER_DECISIONS.md) 的 `DEC-DEV-001/002/003`
- 长期蓝图：[`../../docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md`](../../docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md) 的 `BP-DEV-*`、`BP-FLOW-001/003`、`BP-SUP-*`

## 人类语言目标

这一任务让 FooPodBridge 在 Windows 出现存储卷时自动判断：它是否是非 iPod touch、属于哪个候选家族、数据库能力是否已经实现，以及当前证据只允许诊断、读取还是未来可以进入实验写入。

目标范围不是当前两台实机。早期全尺寸 iPod、Mini、Photo/Color/Video/Classic、Nano 和 Shuffle 都必须进入家族注册表；尚未实现的格式显示 `FormatPending` 或明确只读原因，不能从 UI 和服务中静默消失。

## 本任务必须冻结的逻辑

1. Windows 卷事件、设备节点与稳定物理身份怎样建立一次挂载代次；
2. 怎样正向证明候选卷是非 iPod touch，而不是只按盘符、卷标或显示名称猜测；
3. 家族注册表怎样把型号证据、数据库 profile、签名需求、文件系统和四级证据分开；
4. `NotMounted / UnsupportedFileSystem / UnidentifiedVolume / FormatPending / Initializable / ReadyReadOnly / DatabaseCorrupt / RecoveryRequired` 的输入与输出；
5. 已实现的传统/Hash58 Reader 怎样生成不可变 Library 快照，尚未实现的 Shuffle/Nano 5+ 怎样保持安全可见；
6. 热插拔、盘符复用、同名设备、重新挂载和服务生命周期怎样让旧快照失效；
7. 诊断怎样遮蔽稳定 ID、序列号、用户路径和媒体内容。

## 固定边界

- 本任务只读；生产代码、自动测试和实机检查都不得修改设备文件。
- 自动识别不等于默认写入。任务 005 不提供写命令、初始化或通用强制开关。
- 不启动或控制 iTunes，不开启磁盘模式，不发送私有设备配置命令。
- 不使用设备名称、当前 Nano/Classic fixture 或某台稳定 ID 硬编码 profile。
- 不实现 Shuffle、Nano 5+ 的 Writer；只在注册表中保留正式目标和准确的 `FormatPending`/证据状态。
- 不创建假设备 UI；服务和测试使用真实只读快照、脱敏 fixture 或明确的模拟 Windows 接口。
- 不访问 `foobar-test`、C 盘日常 foobar2000 或只读 `Ref` 目录中的任何可修改路径。

## 下一次开始时

先核对任务 005 的完整 SPEC、家族注册表最小字段、Windows 设备/卷 API 边界、状态转换和自动/人工验收步骤。只有用户批准规格后才进入 C++ 实现、构建和只读设备检查；任何设备写入仍由任务 007/008 之后的独立授权阻断。
