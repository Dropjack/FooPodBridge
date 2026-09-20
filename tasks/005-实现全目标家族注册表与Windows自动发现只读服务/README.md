# 005-实现全目标家族注册表、Windows 自动发现与只读服务

- 状态：实现中
- 日期：2026-09-20
- 实现规格：[`SPEC.md`](SPEC.md)
- 来源与决策记录：[`EVIDENCE.md`](EVIDENCE.md)
- 实施步骤、自动检查与人工验收：[`VALIDATION.md`](VALIDATION.md)
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

## 当前检查点

2026-09-20 已完成 fetch，远端无新提交，开始时本地工作区 clean；已核对完整任务路线并补齐上述规格、来源和验收计划。

用户于 2026-09-20 回复“推进！没问题！”，已批准 SPEC 第 10 节整体范围，可以实施 C++、电脑侧测试、Debug/Release、foobar-dev 开发检查和候选包交付。该规格无需重复请求批准；设备写入仍不在范围。

用户随后授权“再试试，如果还不行，就换一种方法”。构建环境冲突已解决，Core、Windows 只读适配器、ABI 1.1 扩展和 Preferences 信息页已编译；Debug/Release 构建成功，Debug/Release 各 11 项测试通过。已有 Release DLL 部署至 foobar-dev，但没有完成界面验证，也没有生成本任务的新交付包。

2026-09-20 用户明确禁止 computer-use，要求一切软件使用测试由用户操作，Codex 提供逐步指导。该限制已写入根 AGENTS.md 和 PROJECT_RULES，覆盖此前自动应用检查授权。继续非交互验证与包审计；手动检查保持待完成，不进入任务 006。历史阻断与恢复记录见 [`VALIDATION.md`](VALIDATION.md) 第 6–7 节。

## 存档交接点（2026-09-20）

用户手动打开 foobar-dev 并提供截图：Preferences → Tools → FooPodBridge 页面存在，显示 `No iPod devices detected.` 和只读说明；这只证明加载与无设备页面可用。截图同时暴露右侧 Refresh 按钮及详情区域被裁切的布局问题，尚未修复。

用户未带连接线，要求存档并 commit + push 当前必要内容。任务保持“实现中”，不是验收完成；下一轮先修正页面尺寸/布局并核对规格覆盖，使用新的 prerelease 版本构建和打包，再由用户手动检查。连接线可用后继续 Nano 4 只读发现、数量/签名、Refresh 和插拔验证。所有设备写入仍禁止，所有软件操作交给用户。详细断点见 [`VALIDATION.md`](VALIDATION.md) 第 8 节。
