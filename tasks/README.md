# FooPodBridge 任务入口

本文件是跨会话继续项目的唯一任务入口。每次按以下顺序阅读：

1. 根目录 [`AGENTS.md`](../AGENTS.md)；
2. [`docs/PROJECT_RULES.md`](../docs/PROJECT_RULES.md)；
3. 本文件；
4. [`TODO.md`](TODO.md)；
5. 下方当前任务；
6. 当前任务链接的规格、决策和参考证据；
7. 当前任务在 [`../docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md`](../docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md) 中映射的章节。无需重新通读整个 `Ref\ipod_manager`，只有追证据时才打开蓝图列出的固定源码位置。

## 当前进度

- 当前任务：[`003-实现传统 iTunesDB 共同往返核心`](003-实现iPodPhoto数据库往返核心/README.md)
- 状态：规格核对中
- 当前阶段：2026-09-08 已完成 Nano 4 纯净 Windows 只读基线、完整外部备份和私有 fixture；已建立 iTunes Restore → 默认磁盘使用 → 必要时启用磁盘使用 → 纯净 Windows 复测的跨会话交接任务
- 当前下一步：用户按 [`Nano 4 iTunes Restore 交接任务`](../docs/device-evidence/NANO4_ITUNES_RESTORE_HANDOFF.md) 在家执行 Restore，Codex 只读采集 Restore 后 fixture；随后按蓝图 `BP-DB-*` 与 `BP-FMT-002/003` 冻结共同核心并等待实现授权
- 当前禁止：不创建假设备或空 UI；不手工修改 Nano 4，不访问 `foobar-test` 与 C 盘日常 foobar2000；`D:\dev\foo\FooPodBridge\Ref` 永久只读

## 状态定义

| 状态 | 含义 |
| --- | --- |
| 待讨论 | 目标或范围尚未共同确认 |
| 规格核对中 | 正在核对产品行为、风险和用户决定 |
| 可实现 | 用户已批准本任务规格与验收，可以开始代码 |
| 实现中 | 正在完成一个完整永久能力 |
| 实现完成待验收 | 构建、自动测试、包审计和允许的开发检查已完成 |
| 已验收 | 用户在指定环境或实机上明确确认通过 |
| 受阻 | 已记录证据、影响和解除条件，不能可靠继续 |

## 任务路线

下列路线已由用户在任务 000 批准为实施基线。路线批准不自动授权后续任务的代码、构建、部署或实机写入；每项任务仍需先解释并核对自己的范围、风险和验收。

| 编号 | 任务 | 状态 | 永久产物 |
| --- | --- | --- | --- |
| 000 | [确定项目目标与全部产品决策](000-确定项目目标与全部产品决策/README.md) | 已验收 | 产品规格、决策、设备证据计划、批准路线 |
| 001 | [完成参考源码与许可证审计](001-完成参考源码与许可证审计/README.md) | 已验收 | 文件级来源矩阵、许可证与允许复用边界 |
| 002 | [建立 x64 组件工程与服务合同](002-建立x64组件工程与服务合同/README.md) | 已验收 | 可安装组件、Core targets、版本化服务 ABI、包审计 |
| 003 | [实现传统 iTunesDB 共同往返核心](003-实现iPodPhoto数据库往返核心/README.md) | 规格核对中 | Reader/Model/Writer/Validator、未签名 profile、Nano 4 私有 fixture 的共同记录输入 |
| 004 | 实现 6G/hash58 与 Nano 4 格式核心 | 待讨论 | Classic/Nano 3/4 签名 profile、hash58 向量和 Nano 4 fixture 往返 |
| 005 | 实现 Windows 设备发现与只读服务 | 待讨论 | 设备身份、能力矩阵、Library 快照、热插拔生命周期 |
| 006 | 实现 FooCrate 只读 Devices 工作区 | 待讨论 | 使用真实服务数据的首个 FooCrate 设备 UI |
| 007 | 实现设备事务、备份与故障恢复核心 | 待讨论 | 文件系统故障注入、Operation Plan、恢复状态机 |
| 008 | 完成 Nano 4 实验性 Music 导入 | 待讨论 | 备份、加一首、删除、重启播放和恢复的第一条实机纵向能力 |
| 009 | 扩展 Photo/Classic 等家族实机验收 | 待讨论 | 可用设备按验证等级补充，Classic 增加准确 gapless |
| 010 | 完成设备曲目删除与孤立资源清理 | 待讨论 | 两种删除语义、DB-first 删除、已批准清理策略 |
| 011 | 完成普通设备播放列表管理 | 待讨论 | 新建/编辑/删除/排序、foobar playlist 手动发送 |
| 012 | 完成 Music artwork 管理 | 待讨论 | Photo/Classic 封面格式、共享引用与安全清理 |
| 013 | 完成 Audiobook 导入与章节属性 | 待讨论 | Audiobook namespace、bookmark、shuffle、章节与 4 GiB 预检 |
| 014 | 完成原生 iPod Smart Playlist 编辑器 | 待讨论 | 设备规则模型、编辑器、初始成员、Live/刷新语义 |
| 015 | 完成 FooCrate 写入与传输体验 | 待讨论 | 正式 Device Workspace、计划、进度、结果、取消、恢复与句柄释放状态 |
| 016 | 完成独立 Columns UI Device Panel | 待讨论 | 不依赖 FooCrate 的完整 Columns UI 入口 |
| 017 | 完成简化 Default UI Element | 待讨论 | Default UI 原生入口与统一管理对话框 |
| 018 | 完成 Preferences、诊断与备份管理 | 待讨论 | 设置迁移、脱敏诊断、Last Known Good 与清理入口 |
| 019 | 完成稳定性、性能与生命周期验证 | 待讨论 | 压力、USB 中断、热插拔、缓存、隐私与性能证据 |
| 020 | 打包并发布首个完整版本 | 待讨论 | 双组件候选、全量回归、安装升级、许可证与发布材料 |

详细目标、依赖、检查点和任务拆分理由见 [`TODO.md`](TODO.md)。

## 推进规则

- 当前任务未获用户明确验收，不进入下一项。
- 每个任务开始前建立独立任务目录与模块规格；本表不能替代实现计划。
- 每个任务产物必须进入最终架构，不提交假数据、空 UI 或随后删除的临时路径。
- 实机写入任务必须单独写明设备、备份、唯一允许动作和恢复方法；前一台设备通过不自动授权另一台。
- AI 组件测试只部署 `foobar-dev`；用户候选只在 `foobar-test` 手动验收；C 盘日常安装永不在范围。
- 任务 002 以后每个交给用户的新组件使用新的 prerelease 版本，旧包不可覆盖。
- 用户改变产品决定时先更新 `USER_DECISIONS.md` 和产品规格，再调整受影响任务。
