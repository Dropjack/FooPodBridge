# FooPodBridge 项目总则

本项目实现一个 foobar2000 x64 组件，用现代 C++ 重建磁盘模式 click-wheel iPod 的读取、手动音乐管理和安全数据库写入能力。`foo_dop / iPod manager` 只作为经过历史实机验证的知识与行为参考；正式工程不继承其 x86 二进制依赖、iPhone/iPod touch 路径或旧 UI 架构。

## 1. 沟通与用户参与

- 面向用户的目标、规格、任务、原理、风险、验证步骤和决策记录使用中文。
- 用户不需要审查 C++ 写法，但在每项实现前必须能理解：输入来自哪里、经过什么判断、状态怎样变化、失败时怎样恢复、为什么这样拆分。
- 每项任务是可独立检查的永久成果，不是写完即丢的临时代码。可以按模块逐步推进，但不能提交假数据、空 UI、只有成功路径或无法验证的占位功能。
- 每项任务写清输入、动作、产物、自动检查、人工检查和通过标准。
- 自动检查通过只能称为“实现完成待验收”。只有用户在指定隔离环境或实机上明确确认后，才能标记为“已验收”。
- 原理说明不要求机械地为每个任务复制一篇独立文章；必须在任务设计、规格和讨论中把用户关心的逻辑讲清楚。

## 2. 文本编码与启动协议

- 项目维护的文本统一使用 UTF-8 无 BOM 和 LF；第三方文件保持原样。
- 新会话先严格解码并完整阅读根目录 `AGENTS.md`、本文件、`tasks/README.md` 和当前任务。
- PowerShell 不依赖默认编码。中文文本使用严格 UTF-8 解码或 `Get-Content -Encoding utf8`。
- 乱码是无效显示，不是文件内容。修复读取方法后重读，不能因为终端乱码而重写原文件。
- 修改项目文本后验证严格 UTF-8、无 BOM、LF 和中文完整性。

## 3. 正式产品边界

- 初始平台：Windows 10/11 x64。
- 当前验证基线：foobar2000 2.25.10 stable、Columns UI 3.5.0，以及 FooCrate 已维护的双隔离实例。
- 设备兼容按数据库家族和证据等级管理，不再把“项目维护者是否刚好把每个型号带在身边”作为实现门槛。公开支持表必须区分参考支持、fixture 往返、实机只读和实机写入验证。
- 未识别或能力不明的设备只能只读诊断并由服务层拒绝写入。参考实现支持但尚未完成 FooPodBridge 写入验收的型号可以标为 Experimental；只有活动实机任务明确点名、正向识别、备份与恢复门槛齐全的用户自有实验机才能进入受限写入路径，不能用通用开关绕过。
- 产品只管理音乐和 Audiobooks；不管理 Podcasts、Video、Photo、iPhone、iPod touch 或云端内容。
- 产品只执行用户主动发起的导入、删除、播放列表和设备维护操作；不提供媒体库镜像、后台同步或自动 Sync。
- 不双向同步播放次数、评分、跳过次数或播放位置。
- 导入必须支持 ReplayGain 到 SoundCheck 的设备音量修正。
- iPod Classic 导入 MP3/AAC 时生成准确 gapless 信息；iPod Photo 不承诺使用这些字段。扫描失败警告单曲但不阻断整批导入，不写虚假的 dummy gapless 数据。
- 实现原生 iPod Smart Playlist 编辑器。设备不支持的规则必须明确拒绝，不能静默降级为普通列表。
- FLAC 等不受支持格式是否自动转码由 `decisions/USER_DECISIONS.md` 的明确决定控制；在决定批准前不得偷偷转码。

## 4. 架构硬边界

- `device`、`database`、`media` 和 `transaction` 属于独立 C++ Core，不依赖 foobar2000、Columns UI、Default UI 或 FooCrate。
- UI 不直接访问 `iPod_Control`，不直接解析或写入 `iTunesDB`，不自行计算 hash58。
- FooPodBridge 组件通过版本化 foobar2000 服务接口发布设备快照、能力、异步操作和结果。
- FooCrate 是独立组件，只消费该服务并绘制 FooCrate 风格 Devices 界面；它不链接或复制 FooPodBridge Core 源码。
- 独立 Columns UI Device Panel 和 Default UI Element 与 FooCrate 共享同一个服务、状态语义和写入行为。
- FooPodBridge 缺失时 FooCrate 隐藏 Devices；FooCrate 缺失时 FooPodBridge 的独立 UI 仍可使用。
- UI 回调、设备 I/O、后台任务和绘图必须分离；绘图线程不得执行阻塞式磁盘或 USB I/O。

## 5. 设备写入硬规则

- 所有写入必须经过 `docs/SAFETY_MODEL.md` 定义的预检、备份、批次暂存、验证、数据库提交和恢复流程。
- 音频大文件每次批次最多通过 USB 写入一次；不能用无条件完整读回让正常导入时间翻倍。
- 数据库每个用户批次只生成和提交一次，不能每复制一首歌就重建一次。
- 导入先写并确认音频，再让数据库引用它；删除先提交不再引用音频的数据库，再删除音频文件。
- 关闭、取消和外部设备移除必须有明确状态机。FooPodBridge 不提供 Eject；操作结束后释放全部句柄，由用户通过 Windows 资源管理器弹出。任何 UI 都不能绕过事务服务并发写入。
- 第一次对每台实机执行写入前，当前任务必须点名物理设备，并验证可恢复备份、稳定身份、格式/签名能力和测试曲目；普通自动化、UI 测试或另一台同型号设备的通过记录不构成实机写入授权。
- 不把序列号、用户路径、曲库内容或设备数据库样本写入 Release 日志、公开 fixture 或 Git。

## 6. 测试与 foobar2000 边界

- 纯 Core 单元测试、数据库 fixture、格式往返和故障注入从 FooPodBridge 构建目录运行，不需要启动 foobar2000。
- 真实组件加载、两组件通信、自动化、诊断和冒烟测试只使用 `D:\dev\foo\FooCrate\.local\foobar-dev`。
- 用户的干净安装、升级、回退和多场景验收只使用 `D:\dev\foo\FooCrate\.local\foobar-test`。
- 不在 FooPodBridge 建立第三套 foobar2000，不接触 `D:\Dev\foobar2000` 参考实例，也绝不接触 C 盘日常安装。
- Codex 可以按已批准任务自动部署开发构建到 `foobar-dev`；交给用户的候选包由用户手动导入 `foobar-test`。
- 实机测试分设备、分任务授权；自动测试默认使用脱敏 fixture、临时目录和故障注入，不默认写入已连接设备。

## 7. 参考资料、许可证与来源

- `D:\dev\foo\FooPodBridge\Ref` 永久只读，不在其中编辑、格式化、生成、构建或清理文件。
- 任何从 foo_dop 或 libgpod 采用的源码级实现必须在 `docs/REFERENCE_PROVENANCE.md` 记录文件、用途、许可证和修改方式。
- 可以理解并重新设计数据格式与算法，但不能把不明来源的二进制当作正式依赖。
- `Ref\ipod_manager\MobileDeviceSign\iTunesCrypt.dll` 是 x86、未签名且来源/单独授权不明确的参考二进制。正式组件禁止加载、复制、打包或发布它。
- hash58 等能力使用可审计、可测试、许可证兼容的源码实现。
- 不提交 foobar2000 二进制、Apple 闭源 DLL、用户设备数据库或未经许可的第三方组件。
- FooCrate 保持 MIT；FooPodBridge 计划采用 `LGPL-3.0-or-later`，但最终 SPDX 与允许复用的源码边界必须由任务 001 文件级审计确认。任务 001 完成前不公开发布 FooPodBridge 源码或包含第三方源码的组件；这不阻止只包含原创文档的本地规格工作。

## 8. 规格与任务

- `specs/PRODUCT_SPEC.md` 是产品边界和用户可见行为的唯一总规格。
- `decisions/USER_DECISIONS.md` 是产品决策的唯一清单；规格通过决策 ID 引用它，不维护互相冲突的副本。
- `tasks/README.md` 是跨会话唯一任务入口，必须标明当前任务、状态、下一步和用户检查点。
- 任务 000 已完成阻断性产品决定并批准路线；任务 001 完成参考源码与许可证审计前，仍不创建 CMake、C++、组件 DLL 或 UI 脚手架。
- 后续每项任务使用独立目录，至少包含目标、范围、程序逻辑、步骤、验证记录和决策记录。
- 状态统一为：待讨论、规格核对中、可实现、实现中、实现完成待验收、已验收、受阻。
- 用户可见行为先更新规格并得到用户核对，再进入代码实现。

## 9. 必须执行的实现工作流

1. 严格读取启动文档、当前任务和相关规格，检查 Git 状态并保护用户修改。
2. 用中文说明本轮目标、会查看或修改的范围，以及明确不做的内容。
3. 只读检查参考资料，把已证实行为、源码推断和待实机验证分开记录。
4. 先冻结本任务程序逻辑、失败行为和验收标准；需要用户决定的行为先停在规格核对。
5. 以测试驱动实现一个完整永久能力，不提交假数据或无法使用的半套行为。
6. 构建适用的 x64 Debug 与 Release，消除新增警告或记录获批原因。
7. 运行 Core、故障注入、服务和回归测试；涉及 UI 时只部署到 `foobar-dev`。
8. 生成新的 prerelease `.fb2k-component`，审计包内容并放入对应仓库 `dist`。
9. 向用户提供可重复的 `foobar-test` 或实机人工验收步骤。
10. 用户明确验收后同步规格、任务和证据；未经确认不标记“已验收”。

## 10. 组件与版本交付

- FooPodBridge 包命名为 `FooPodBridge-<SemVer>.fb2k-component`，核心文件为 `foo_pod_bridge.dll`。
- FooCrate 集成变更遵守 FooCrate 仓库规则，独立产生 `FooCrate-<SemVer>.fb2k-component`；两个 DLL 不混装。
- 每个交给用户的新测试二进制使用新的 `beta.N`，旧稳定版和候选包不可覆盖。
- FooPodBridge 包放在本仓库 `dist`；FooCrate 包放在 FooCrate 仓库 `dist`。
- 交付前验证包存在、架构为 x64、可见版本正确、包内只有批准内容。
- 用户手动安装验收候选。除非用户另行要求，不把候选部署到 `foobar-test`，更不能部署到日常安装。

## 11. 实现完成与验收

“实现完成待验收”要求：

- 本任务规格的正常、空数据、失败、中断、取消、恢复和生命周期行为全部实现；
- Debug/Release、相关自动测试、包审计和允许的开发实例检查通过；
- 中文程序逻辑、规格和实际行为一致；
- 没有新增未解释警告、隐私日志、未授权依赖或真实设备风险；
- 提供面向非 C++ 用户的人工检查步骤和预期结果。

“已验收”还要求用户在 `foobar-test` 或指定实机上完成检查并明确确认通过。

## 12. 当前工作区

- 正式项目：`D:\dev\foo\FooPodBridge\FooPodBridge`
- 只读参考：`D:\dev\foo\FooPodBridge\Ref`
- FooCrate 集成：`D:\dev\foo\FooCrate`
- AI 开发实例：`D:\dev\foo\FooCrate\.local\foobar-dev`
- 用户验收实例：`D:\dev\foo\FooCrate\.local\foobar-test`
- 日常 foobar2000：C 盘，严格禁止触碰
