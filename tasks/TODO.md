# FooPodBridge 永久任务路线

- 状态：已批准路线
- 日期：2026-09-10（任务 000 按设备范围重开修订）
- 当前任务：`005 实现全目标家族注册表、Windows 自动发现与只读服务`

## 1. 拆分原则

路线按永久能力而不是演示阶段拆分：

- 内部 Core 可以先于 UI 完成，因为它会长期保留并可独立验证；
- UI 只连接真实服务和真实 fixture/设备快照，不展示假设备；
- “只读”是完整、可验收的设备浏览能力，不是写入按钮占位；
- 每个写入任务包含正常、失败、取消、恢复、自动测试和实机检查；
- FooCrate 优先获得真实设备 UI，独立 Columns UI 与 Default UI 后续复用稳定服务；
- 数据库能力按家族拆分，型号支持按四级证据表达；一台实机成功不能自动把同家族其他型号标为已验证，但没有逐台实机也不阻断参考支持和 fixture 实现。
- 所有 Windows 可挂载存储卷模式的非 iPod touch 都必须进入家族注册表和自动发现流程；缺少硬件或尚未实现的格式显示明确的只读/`FormatPending` 状态，不能被静默遗漏。
- 实机与私有 fixture 只验证对应 profile 和能力，不能产生针对当前 Nano、Classic 或稳定 ID 的产品专用分支。

### 1.1 历史实现“抄作业”规则

[`../docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md`](../docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md) 是任务 003–017、021 共同使用的中文目标方案和历史源码导航。任务不再各自重新总结整个 iPod manager，而是：

- 引用对应 `BP-*` 编号，说明采用的历史行为和 FooPodBridge 的差异；
- 只有核对证据时才打开蓝图列出的固定 `Ref\ipod_manager`、libgpod 或 Apple 官方资料；
- 采用历史输入/判断/结果，使用本项目架构原创实现；
- 删除、提交、取消、恢复等历史不安全顺序必须由 `SAFETY_MODEL.md` 替换；
- 新证据与蓝图冲突时，先更新蓝图和来源记录，再改变任务实现。

## 2. 阶段 A：冻结产品与合法来源

### 000 确定项目目标与全部产品决策

- 目标：让聊天决定变成唯一规格、决策清单、安全模型和用户批准的完整路线。
- 输入：当前想法、FooCrate 结构、初始设备范围、foo_dop 初步审计。
- 产物：本仓库文档、两台实机证据计划、所有产品决定和路线批准。
- 用户检查：逐项确认 `USER_DECISIONS.md`，核对没有默认继承 foo_dop 历史功能。
- 通过标准：所有当前阻断项已批准或排除；用户明确宣布产品规格和路线可进入任务 001。
- 不做：不写 C++、不创建工程、不加载组件、不写设备。

### 001 完成参考源码与许可证审计

- 前置：任务 000 已验收。
- 目标：在任何参考源码进入正式工程前，冻结文件级来源、许可证版本和采用方式。
- 产物：foo_dop/libgpod 模块映射、hash58/gapless/Smart Playlist 来源、现代 SDK 来源和最终项目许可证决定。
- 自动检查：许可证文件、版权头、commit/tag 和链接完整；正式依赖不含闭源 Apple DLL 或 `iTunesCrypt.dll`。
- 用户检查：理解“参考知识、修改源码、重新实现”的区别，批准公开许可证。
- 通过标准：任务 002–017 的既有来源都有允许/禁止结论；任务 010/011 的新增来源在采用前完成增量审计。

## 3. 阶段 B：建立 x64 Core 与只读产品

### 002 建立 x64 组件工程与服务合同

- 前置：任务 001 已验收。
- 目标：建立正式 CMake/x64 工程、Core targets、foobar 组件、稳定身份、版本化服务和双仓库合同。
- 产物：可安装 `FooPodBridge-beta.1`，Components 页面能确认名称/版本；没有空 Device Panel。
- 自动检查：x64 Debug/Release、基础测试、ABI/身份测试、包只含批准 DLL、部署路径保护。
- 用户检查：在 `foobar-test` 手动安装/卸载，只检查组件身份和没有破坏 FooCrate。
- 通过标准：全新构建树可重复构建、测试和打包；FooCrate 可在编译期消费公开合同但尚不显示假设备。

### 003 实现传统 iTunesDB 共同往返核心

- 前置：任务 002 已验收；Nano 4 私有 fixture 和验证外部基线已取得，真实 Photo fixture 不再阻断共同核心。
- 目标：建立可扩展的传统 `iTunesDB` Reader/Model/Writer/Validator，以未签名 profile 定义早期家族最小结构，并用 Nano 4 私有 fixture 提供真实共同记录输入；不在本任务生成签名 Nano 4 数据库。
- 产物：共同 Reader/Model/Writer/Validator、格式配置边界、二进制比较器、私有 Nano 4 读取 fixture、合成未签名 fixture，以及仅含合法 master Library 的未签名空库产物。
- 自动检查：原始读取、无修改语义等价、未知字段/6G 签名区保留、损坏/截断拒绝、增删虚拟曲目/playlist 往返、未签名空 Library 生成与再读取。
- 用户检查：查看数据库结构说明和比较报告，不写实机。
- 通过标准：在电脑临时目录反复往返稳定；没有任何设备写入。
- 蓝图：`BP-DB-*`、`BP-INIT-001/003`、`BP-FMT-001/002/003`。

### 004 实现 6G/hash58 与 Nano 4 格式核心

- 前置：任务 003 已验收；Nano 4 私有 fixture 已取得；稳定设备 ID/FireWire GUID 以遮蔽形式取得批准。
- 目标：为 Classic、Nano 3/4 的候选 6G 家族扩展记录 profile，并用允许的 BSD-3-Clause 源码实现 hash58，不依赖 x86 DLL 或 CBK/hash72。
- 产物：6G Reader/Writer/Validator profile、hash58 已知向量、设备 ID 处理、Nano 4 差异报告和签名正确的空库产物。
- 自动检查：上游/自建向量、错误 ID、篡改检测、Nano 4 完整数据库往返、空 Library 生成、第二签名保留区行为对比、x64 架构审计。
- 用户检查：理解 hash58 输入、私有 ID 边界以及“同家族不等于同型号已验证”；仍不写实机。
- 通过标准：生成数据库在结构、hash 验证器和 Nano 4 fixture 比较中通过，等待任务 008 实机接受测试。
- 蓝图：`BP-DB-*`、`BP-INIT-003`、`BP-FMT-001/003`。
- 验收：2026-09-10 用户确认通过；该结论只覆盖电脑侧 profile/签名/fixture 能力，不授权实机写入。

### 005 实现全目标家族注册表、Windows 自动发现与只读服务

- 前置：任务 003/004 已验收；`EVID-DEV-001/002` 的只读部分可采集。
- 目标：对所有 Windows 可挂载存储卷模式的非 iPod touch 自动发现并正向识别，以家族注册表发布数据库 profile、能力、证据等级和 Library 快照；尚未实现的早期/Shuffle/Nano 5+ profile 也必须明确分类，正确处理热插拔与盘符变化。
- 产物：device 模块、全目标家族注册表、证据门禁、只读服务、完整设备状态分类、Preferences/诊断设备信息和真实快照测试。输入硬前置是 Windows 已暴露可访问存储卷；本模块不配置 iTunes 或开启 iPod 磁盘使用。
- 自动检查：无存储卷直接进入 `NotMounted` 且不发送任何配置命令，并覆盖临时卷、盘符复用、同名设备、Unidentified、FormatPending、Unsupported、Initializable、损坏 DB、恢复遗留、移除和生命周期；型号名称不能单独授予 Writer。
- 用户检查：先用 Nano 4 查看识别、容量、曲目与 playlist 数量；其他设备可用时按型号补充验证等级。
- 通过标准：目标家族没有因缺少当前实机而从注册表消失；读结果与设备现状一致；任何命令都不能修改设备。
- 蓝图：`BP-DEV-*`、`BP-FLOW-001/003`、`BP-SUP-*`。

### 006 实现 FooCrate 只读 Devices 工作区

- 前置：任务 005 已验收；FooCrate UI 模块规格与 mockup 获用户批准。
- 目标：用真实 FooPodBridge 服务数据把 Devices namespace 和只读 Device Workspace 加入 FooCrate。
- 产物：FooCrate 服务消费者、真实设备树、统一 Library 中的 Music/Audiobook Media Kind、playlist 浏览和 Unsupported/移除状态。
- 自动检查：服务缺失隐藏、版本不兼容、热插拔、快照代次、窗口销毁、DPI/主题。
- 用户检查：在 `foobar-test` 浏览当前 Nano 4；其他设备可用时补充，确认 FooCrate 原有 playlist/album/lyrics 不回归。
- 通过标准：没有假数据和写入按钮占位；只读能力完整可日常浏览。
- 蓝图：`BP-FLOW-001`、`BP-SUP-*`；禁止移植旧 `panel.cpp`。

## 4. 阶段 C：安全写入与分级实机导入

### 007 实现设备事务、备份与故障恢复核心

- 前置：任务 005 已验收；`DEC-SAFE-*` 全部批准。
- 目标：实现 Operation Plan、单设备写锁、批次暂存、数据库备份、恢复记录、取消和故障注入，不写真实设备；落实测试期每设备 10 个验证快照和 Last Known Good 保护。
- 产物：transaction 模块、抽象文件系统、显式 `InitializeLibrary` Operation Plan、首次外部基线清单、恢复工具、数据库快照保留模型和完整故障矩阵。
- 自动检查：空卷初始化、已有数据库不得误初始化、磁盘满、短写、Flush、重命名、权限、代次变化、取消各阶段、恢复组合、10 份轮换和 Last Known Good 不被自动删除。
- 用户检查：查看每个中断点留下的临时目录，确认旧测试 DB 可恢复。
- 通过标准：所有模拟故障满足 `SAFETY_MODEL.md`，才允许首轮实机写入任务。
- 蓝图：`BP-FLOW-002/003`、`BP-INIT-*`、`BP-TXN-*`。

### 008 完成 Nano 4 实验性 Music 导入

- 前置：任务 007 已验收；Nano 4 外部备份已验证；稳定身份、唯一测试曲目和本次唯一写入动作已冻结；直接格式/元数据决定已批准。
- 目标：先在可牺牲 Nano 4 上完成一条可长期使用的 Music 导入纵向能力，不格式化、不清空现有库、不修改 artwork/游戏。
- 产物：预检、重复/空间、批准格式、元数据、Playback Statistics Rating、SoundCheck、Nano 4 DB 写入、FooCrate 计划/进度/结果和恢复。
- 自动检查：兼容/不兼容、空间、FAT32、标签缺失、Rating、SoundCheck、单曲/批次失败、数据库读回。
- 用户检查：先在已有 Library 增加并删除一首批准测试曲，确认组件释放句柄后用 Windows 资源管理器弹出，重启、播放、再连接；成功后才另行批准“移开旧 DB → 最小 Library → 恢复基线”的初始化测试。
- 通过标准：Nano 4 能浏览/播放测试曲并恢复；备份和恢复证据完整；验证等级从 Experimental 提升为 `DeviceWriteVerified`。
- 蓝图：`BP-INIT-003`、`BP-IMP-*`、`BP-FLOW-002`。

### 009 实现早期与传统 iTunesDB 家族 profile

- 前置：任务 003/005 已验收；固定来源、格式矩阵和 fixture 取得合法使用结论。
- 目标：为早期全尺寸 iPod、Mini、Photo/Color/Video 和 Nano 1/2 建立可审计的传统数据库 profile；先查明记录版本、端序、路径、数据库和固件差异，禁止把它们简化为一个营销型号配置开关。
- 产物：家族/代际映射、独立 profile 描述、Reader/Writer/Validator 扩展、公开或私有 fixture 往返和证据缺口表。
- 自动检查：各 profile 的合法最小库、损坏拒绝、未知记录保留、跨 profile 误用拒绝、确定性输出和 round-trip；无实机时只提升到 `StructureKnown`/`FixtureRoundTrip`。
- 用户检查：不要求购买设备；有设备或脱敏样本时才补充只读/写入验收，不因缺少样本伪造结论。
- 通过标准：已知早期/传统目标变体都有明确 profile 或 `FormatPending` 原因，不存在“都按 Photo/当前 IPC 配置写”的通用路径。
- 蓝图：`BP-DB-*`、`BP-FMT-001/002/004`、`BP-SUP-*`。

### 010 实现 Shuffle 独立数据库家族

- 前置：任务 005 已验收；任务 001 对拟采用来源完成必要的增量审计。
- 目标：把 Shuffle 作为独立设备/数据库家族实现，不套用 Classic 或普通 `iTunesDB` Writer；按证据区分 `iTunesSD`、ShadowDB 及代际差异。
- 产物：Shuffle profile 注册表、Reader/Writer/Validator、路径/顺序/播放能力模型、fixture 往返和明确证据等级。
- 自动检查：各变体的记录边界、曲目顺序、文件路径、空库、损坏拒绝、跨代误用拒绝和确定性输出。
- 用户检查：没有实机时只核对结构/fixture 证据；任何首轮实机写入另建点名实验并验证外部备份与恢复。
- 通过标准：所有已知 Shuffle 代际都被识别为具体 profile 或 `FormatPending`，不能因没有屏幕或没有当前硬件而从目标范围消失。
- 蓝图：`BP-DB-*`、`BP-FMT-005`、`BP-SUP-*`。

### 011 实现 Nano 5+ 非 touch 独立数据库家族

- 前置：任务 005 已验收；任务 001 必须先对 `iTunesCDB`、SQLite、hash72/CBK 的合法来源与原创实现边界完成增量审计。
- 目标：为 Windows 可挂载的 Nano 5+ 非 touch 设备建立独立 `iTunesCDB`/SQLite/签名 profile，不套用任务 004 的 hash58 Writer，不加载或复制 `iTunesCrypt.dll`。
- 产物：代际/能力矩阵、CDB/SQLite Reader/Model/Writer/Validator、合法签名接口、fixture 往返和缺少证据时的 `FormatPending` 状态。
- 自动检查：数据库关系、签名规范化、错误身份、篡改、跨 profile 误用、确定性生成、无闭源 DLL/Apple Mobile Device 依赖和许可证扫描。
- 用户检查：无实机不阻断结构与 fixture 实现；首次写入任何 Nano 5+ 必须由独立设备任务点名、备份、恢复并完成重启播放验收。
- 通过标准：目标 Nano 5+ 代际都有可审计的 profile/证据状态；未解决合法签名能力时保持只读，不能退回 hash58 猜写或通用强制开关。
- 蓝图：`BP-DB-*`、`BP-FMT-006`、`BP-SUP-*`。

### 012 扩展跨家族实机验收与能力矩阵

- 前置：任务 008–011 中目标 profile 已达到允许受控实验的证据；目标型号可用时，其外部备份、身份和测试曲已验证。
- 目标：把 Nano 4 已验收纵向能力扩展到可取得的早期/传统、Classic、Shuffle 和 Nano 5+ 设备；Classic 加入准确 gapless。没有到手的型号保持较低验证等级，但其格式与识别任务不被取消。
- 产物：每台可用实机的设备级接受记录、验证等级、Playback Statistics Rating；Classic 可用时增加连续专辑 gapless 测试。
- 自动检查：profile/签名输入、连续 MP3/AAC、gapless 无数据警告、错误签名恢复、批次性能和跨家族门禁。
- 用户检查：在设备当前状态允许时验证首次初始化或已有 Library 导入；只对点名设备执行批准动作并在重启、播放、再连接后提升等级。
- 通过标准：每个已执行型号的 Library、SoundCheck 和再次连接通过；Classic 只有在 gapless 也通过后才获得对应能力标记，未执行型号不冒充已验证。
- 蓝图：`BP-INIT-003`、`BP-FMT-*`、`BP-IMP-*`。

## 5. 阶段 D：完整手动管理

### 013 完成设备曲目删除与孤立资源清理

- 前置：任务 008/012 中至少一个写入 profile 已验收；删除与清理决定已批准。
- 目标：实现 Remove from Playlist 与 Delete from iPod 的不同语义，先 DB 后音频并安全清理。
- 产物：引用分析、确认模型、删除事务、清理报告和恢复。
- 用户检查：每个声明已验证的数据库家族至少一台设备测试只移除引用、彻底删除、共享 playlist、删除失败模拟。
- 通过标准：不产生 DB 指向缺失文件；未知孤立文件不被误删。
- 蓝图：`BP-DEL-001`，必须替换历史 file-first 删除顺序。

### 014 完成普通设备播放列表管理

- 前置：任务 013 已验收；playlist 冲突/快照决定已批准。
- 目标：完整管理普通 iPod playlist；设备 UI 明确 New 后加入曲目选择，不传输 foobar playlist/autoplaylist 对象。
- 产物：新建、重命名、删除、排序、成员编辑、缺失曲目同批导入、已有 track ID 复用和目标成员去重。
- 用户检查：每个声明已验证的数据库家族至少一台设备创建/编辑/删除，重启后顺序与成员正确；确认没有后台同步。
- 通过标准：master Library 引用完整，普通/Smart 类型不混淆。
- 蓝图：`BP-PL-001`。

### 015 完成 Music artwork 管理

- 前置：任务 008/012 已有音频导入；封面来源决定已批准。
- 目标：按设备 artwork capability 完整处理音乐封面、共享引用、更新和删除清理，先以 Nano 4 建立实机基线。
- 产物：设备能力格式、缩放、ArtworkDB/索引、缓存、无图/坏图和共享资源生命周期。
- 用户检查：按型号 artwork capability 分级验证有图、无图、不同封面、更新和删除后的设备显示。
- 通过标准：封面正确且不破坏音乐 DB；不包含 Photo Library 同步。
- 蓝图：`BP-ART-001`。

### 016 完成 Audiobook 导入与章节属性

- 前置：任务 015 已验收；Audiobook 决定与样本获批准。
- 目标：把用户明确选择 Audiobooks 目标的内容正确标记，并处理章节和 FAT32 大文件限制；不靠扩展名或 Genre 偷猜类型。
- 产物：统一 Library 中的 Audiobook media kind、bookmark、shuffle、章节、封面和专属预检。
- 用户检查：按型号 capability 验证播放、暂停、返回续播、随机播放排除和章节；不支持项明确显示。
- 通过标准：不能靠扩展名误分类 Music；超限文件在复制前拒绝。
- 蓝图：`BP-AUD-001`。

### 017 完成原生 iPod Smart Playlist 编辑器

- 前置：任务 014 已验收；Smart Playlist 规则与刷新决定批准。
- 目标：在两台目标设备能力范围内完整创建、读取和编辑 Apple Smart Playlist；不接受手工成员或 foobar autoplaylist 转换，并提供已有设备曲目的明确单向 Rating 刷新。
- 产物：规则模型、设备能力字段/运算符、嵌套、限制、排序、初始成员、Live/刷新语义、Rating 刷新计划和编辑 UI。
- 自动检查：规则往返、未知字段、嵌套、日期/数字/playlist 引用、Rating 写入/清除/覆盖、设备不支持拒绝、成员计算。
- 用户检查：按目标型号建立 Rating 2–5 的列表和 Rating + Artist 组合；修改电脑端评分并明确刷新，验证写入、清除、设备端改动覆盖和批准的 Live/手动更新语义。
- 通过标准：不以普通快照冒充；已有 iTunes Smart Playlist 不损坏。
- 蓝图：`BP-SPL-001`。

## 6. 阶段 E：完成三套 UI

### 018 完成 FooCrate 写入与传输体验

- 前置：任务 008、013–017 的纵向能力已验收；目标数据库 profile 的证据门禁由任务 005/009–012 提供；FooCrate 完整 UI mockup 和模块规格批准。
- 目标：把所有正式手动管理能力以 FooCrate 风格整合到 Device Workspace。
- 产物：容量、计划、拖放、进度、结果、错误、取消、恢复和 Smart Playlist 编辑；完成后显示句柄已释放，不提供 Eject。
- 自动检查：主题、DPI、窗口生命周期、服务消失、操作中关闭/重开、键盘与拖放。
- 用户检查：在 `foobar-test` 以 FooCrate 完成完整日常流程和多场景回归。
- 通过标准：FooCrate 是首要、完整管理入口，原有功能无回归。

### 019 完成独立 Columns UI Device Panel

- 前置：任务 018 已验收；独立功能范围批准。
- 目标：为非 FooCrate Columns UI 用户提供批准的完整入口。
- 产物：可加入普通 CUI 布局的 Device Panel 和共享管理对话框。
- 用户检查：不加载 FooCrate 布局也能完成批准范围，主题/布局保存正常。
- 通过标准：不复制业务逻辑，结果与 FooCrate 相同。

### 020 完成简化 Default UI Element

- 前置：任务 019 已验收；Default UI 范围批准。
- 目标：使用 Default UI 原生外观提供批准的管理能力。
- 产物：DUI Element、基础浏览/命令和共享复杂编辑对话框。
- 用户检查：Default UI 配置中添加/删除元素，完成批准日常流程。
- 通过标准：视觉简洁但安全、错误和事务无降级。

## 7. 阶段 F：设置、质量与发布

### 021 完成 Preferences、诊断与备份管理

- 前置：任务 018–020 已验收；诊断/备份决定批准。
- 目标：提供可迁移设置、Last Known Good、占用/清理和脱敏诊断导出。
- 正式化门槛：在本任务验收前重新核对 `DEC-SAFE-003/004`，决定日常版是否继续要求首次完整基线以及数据库快照保留数量；事务回滚保护和至少一个 Last Known Good 不可取消。
- 产物：Preferences、稳定 GUID、配置迁移、备份查看/恢复、日志边界。
- 用户检查：Apply/Cancel/Reset、升级、损坏配置、备份恢复和诊断预览。
- 通过标准：不能通过设置绕过设备支持；隐私内容默认遮蔽。
- 蓝图：`BP-TXN-*`、`BP-REF-001`。

### 022 完成稳定性、性能与生命周期验证

- 前置：任务 021 已验收。
- 目标：证明长时间、大批次、热插拔、取消、双 UI 和设备异常下可靠且性能没有人为翻倍。
- 产物：性能基线、压力/故障矩阵、资源和线程审计、隐私日志审计。
- 用户检查：每个声明已验证的数据库家族至少一台设备执行批准的压力流程，验证 UI 响应、复制速度、重启与恢复。
- 通过标准：无已知数据库破坏、无限缓存、悬挂事务、隐私日志或 C 盘访问。

### 023 打包并发布首个完整版本

- 前置：任务 000–022 全部已验收；许可证和公开范围批准。
- 目标：从干净提交构建、测试并交付 FooPodBridge 与 FooCrate 正式组件。
- 产物：两个 `.fb2k-component`、SHA-256、安装/升级/回退、支持矩阵、已知限制和发布说明。
- 用户检查：在 `foobar-test` 做干净安装与升级；两台实机执行完整日常流程。
- 通过标准：包内容、版本、许可证、来源、双组件缺失行为和所有回归通过，用户明确批准发布。

## 8. 任务 000 路线决定记录

- 任务 008/012 的首轮实机导入不新增 artwork，必须保留设备已有 artwork 数据和引用；完整新增、更新、共享引用与清理由任务 015 一次完成；
- SoundCheck 是首轮导入硬要求，已按当前目标放入任务 008/012；
- FooCrate 只读 UI 放在真实写入前，符合“先把 FooCrate 体验设计正确”的优先级；
- Smart Playlist 独立成任务 017，避免与普通 playlist 混成无法单独验收的大任务；
- 独立 CUI/DUI 等 Core 和 FooCrate 行为稳定后实现，避免三份 UI 同时变化；
- FLAC 转码当前没有路线任务，用户重开 `DEC-IMP-003` 后才增加。

用户于 2026-08-28 表示默认认同建议路线，并将在具体任务中按需提问。路线因此批准为当前实施基线；任何后续疑问或新证据都可以在进入受影响任务前重开对应决定，不能静默改变产品行为。

用户于 2026-09-10 重开任务 000，明确批准“全非 touch 自动识别、按证据开放写入、实机只验证 profile”的强制标准。路线新增任务 009–011 分别覆盖早期/传统、Shuffle 和 Nano 5+，原后续任务顺延至 012–023；缺少实机不允许删除这些目标，只影响证据等级和是否允许写入。
