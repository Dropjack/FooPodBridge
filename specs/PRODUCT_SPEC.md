# FooPodBridge 产品总规格

- 状态：已批准基线
- 版本：0.1
- 日期：2026-09-08
- 产品目标：[`../docs/PRODUCT_GOAL.md`](../docs/PRODUCT_GOAL.md)
- 架构：[`../docs/ARCHITECTURE.md`](../docs/ARCHITECTURE.md)
- 安全模型：[`../docs/SAFETY_MODEL.md`](../docs/SAFETY_MODEL.md)
- 用户决策：[`../decisions/USER_DECISIONS.md`](../decisions/USER_DECISIONS.md)

本文件是 FooPodBridge 产品边界和用户可见行为的唯一总规格。详细模块在对应任务开始前建立独立规格，但不能与本文件和已批准决策冲突。

## 1. 产品定义

FooPodBridge 是 Windows x64 上的 foobar2000 2.x 组件，为磁盘模式 click-wheel iPod 提供读取、手动音乐导入、删除、播放列表、Smart Playlist、封面、SoundCheck、gapless 和 Audiobook 能力。

产品不是 iTunes 媒体库替代品，不执行自动 Sync；它把 iPod 作为独立设备 namespace，而不是 foobar Playlist Manager 中的一组普通 playlist。

正式用户体验由三种入口共享同一 Core：

1. FooCrate 集成 Devices UI；
2. 独立 Columns UI Device Panel；
3. 简化 Default UI Element。

## 2. 平台与安装

- Windows 10/11 x64；
- 当前验证基线为 foobar2000 2.25.10 stable 与 Columns UI 3.5.0；
- FooPodBridge 以 `FooPodBridge-<version>.fb2k-component` 安装；
- FooCrate 是另一个组件，通过 FooPodBridge 服务连接，不捆绑或自动安装 FooPodBridge；
- FooPodBridge 不要求安装 iTunes、Apple Music 或 Apple Mobile Device Support 才能发现和管理已经由 Windows 暴露为受支持文件系统卷、且本项目已具备所需设备属性和签名能力的 click-wheel iPod；
- 组件包不得包含 foobar2000、Apple DLL、x86 `iTunesCrypt.dll`、用户数据库或未经许可二进制。

## 3. 支持设备

### 验证等级

- `StructureKnown`：固定历史来源能说明数据库家族和目标行为；
- `FixtureRoundTrip`：本项目脱敏或私有 fixture 已通过 Reader/Writer/Validator 往返；
- `DeviceReadVerified`：指定实机已在目标 Windows 环境完成只读识别与 Library 核对；
- `DeviceWriteVerified`：指定实机在外部备份、事务、重启播放和恢复门槛下完成写入验收。

只有 `DeviceWriteVerified` 可以宣传为“已验证可写”。参考支持但尚未达到第四级的已识别型号可以显示 Experimental；是否允许一次受控实验写入由活动设备任务决定，不存在面向任意未知设备的通用强制写入开关。

允许写入不是按产品名称字符串决定，而是按物理设备身份、固件、数据库类型、文件系统、签名输入、能力矩阵和活动任务共同决定。每台设备第一次写入前必须完成对应 `EVID-DEV-*`。

当前数据库家族边界：

- 传统未签名 `iTunesDB`：Photo、Mini、较早 click-wheel 与 Nano 1/2 等候选型号，按证据逐项建立 profile；
- 签名传统 `iTunesDB`：Classic、Nano 3/4 等候选型号，共享 6G 记录骨架与 hash58，但型号能力仍分开验证；
- Shuffle：`iTunesSD`/ShadowDB 独立路线；
- Nano 5 及以后涉及 `iTunesCDB`、SQLite、hash72/CBK 的路径暂不进入当前实现；
- iPhone、iPod touch、Apple Mobile Device 和现代 iOS 数据库路径不支持。

## 4. 设备发现与概览

设备到达后：

1. Windows 卷事件触发只读识别；
2. 服务确认 iPod 目录、设备文件、数据库与稳定身份；
3. 读取容量、可用空间、型号、固件和能力；
4. 读取 Library、playlist 和 artwork 索引；
5. 发布一个不可变设备快照和代次；
6. UI 显示 Ready、Read-only 或 Unsupported 及原因。

盘符不是稳定设备身份。设备移除或重新挂载后旧快照立即失效。

## 5. 设备 namespace

受支持设备至少包含：

```text
<Device>
├── Library
│   └── Music / Audiobook media kinds
└── Playlists
    ├── Normal playlists
    └── Smart playlists
```

- 第一版可以使用统一 Library 管理 Music 与 Audiobook 两种 Media Kind，不强制独立 Audiobooks 页签；具体筛选和目标入口在 UI 任务冻结；
- playlist 是设备 track ID 引用，不是 foobar playlist；
- 所有设备曲目必须存在于设备数据库的 track list；
- UI 可以为设备路径创建 foobar metadb handles 用于播放和属性查看，但不改变设备 namespace 语义。

## 6. 手动导入

用户可以从 foobar 曲目选择或 FooCrate Playlist View 主动发起导入。普通 playlist/autoplaylist 只可作为用户选择曲目的界面；playlist 对象本身不能发送或拖入设备。

导入必须先生成 Operation Plan，至少说明：

- 目标设备、明确选择的 Music/Audiobooks Media Kind 或 playlist；
- 直接复制、拒绝或未来可能的转码结果；
- 预计空间与保留余量；
- 重复、冲突和缺失标签；
- Playback Statistics `%rating%` 到设备 Rating 的写入、保持或清除结果；
- SoundCheck、gapless、封面和 Audiobook 处理；
- 会创建或修改的 playlist；
- 警告和阻断原因。

直接导入行为已经冻结为：

- 按目标设备能力矩阵检查真实容器、codec 和 profile；首批正式验证 MP3、AAC-LC/M4A 和设备确认支持的 ALAC；
- WAV、AIFF、Audible 等格式等待真实需求与样本，不因文件扩展名或历史规格自动允许；
- 不支持项目逐项显示 Unsupported，默认不复制、不修改源文件或设备，兼容项目仍可继续；
- FLAC 当前只识别并明确拒绝直接导入，不自动转码；
- 重复项使用标准化字段、时长容差和媒体类型强匹配，默认 Skip，并允许明确选择 Add duplicate；Replace metadata 等待后续测试重新决定，在此之前不自动覆盖；
- 导入后至少保留 `max(256 MiB, 总容量的 1%)`，用户可主动调低但不能设为零。

执行遵守安全模型：大文件只写一次、整批只构建/提交一次数据库、数据库完整读回验证。格式、gapless、封面等独立单曲问题记录警告并继续；空间、设备身份、数据库或事务错误立即停止批次。

## 7. 元数据

设备用户元数据分为两层。核心字段至少覆盖：

- Title、Artist、Album Artist、Album；
- Compilation、Year/Date、Track Number 和 Disc Number；
- 路径、时长、codec、文件大小和必要设备媒体标志；
- SoundCheck、gapless、封面和 Audiobook 字段。

Genre、Composer、BPM、Sort、总 Track/Disc 数等次要字段在目标设备明确支持且源数据存在时尽力写入；缺失不阻断，设备不使用的字段忽略。元数据读取 foobar2000 正式 metadb/file info 的原始字段，不使用带回退的 UI 标题格式结果，不反写源音频标签。Title 缺失时使用不含路径的文件名，Artist/Album 缺失显示 Unknown，本批设备值可以在计划中编辑。

Compilation 明确标签优先；没有明确值时，使用规范化的原始 Album Artist + Album 识别专辑。统一且非空的 Album Artist 下出现至少两个不同 Artist 时自动推断 Compilation=true。证据合并本次导入、设备已有同专辑曲目和 foobar 曲库中无歧义的同专辑匹配；推断成立时，同一事务保持设备上整个专辑的 Compilation 状态一致，保留原始 Album Artist 和每曲 Artist，不修改源标签。计划必须显示推断结果与来源。

评分是 `DEC-META-001` 原始文件字段规则的明确例外：FooPodBridge 通过目标 foobar2000 metadb 句柄读取 Playback Statistics 动态提供的 `%rating%`，不读取或写入音频文件 `RATING` 标签，也不依赖 FooCrate。新导入曲目把有效 1–5 星写为目标设备的原生 Rating；Playback Statistics 缺失或值无效时按未评分处理。

已有设备曲目只有在用户明确执行 `Refresh ratings from foobar` 后才刷新。刷新必须生成 Operation Plan，逐项显示写入、保持和清除；缺失电脑端评分会清空设备 Rating，设备端改动会被电脑端值覆盖。电脑端 Playback Statistics 是唯一事实来源，设备评分不反写电脑，不提供后台或双向同步。

## 8. SoundCheck

- 导入必须把可用 ReplayGain 转换为设备 SoundCheck/volume normalization 数据；
- 公式采用有来源记录和已知向量的实现；
- 优先使用 ReplayGain Track Gain，缺失时使用 Album Gain；两者都缺失则不写 SoundCheck并提示，不自动扫描；
- SoundCheck 失败属于可见项目结果，不能写虚假默认值；
- 实机验收比较开启 Sound Check 后的明显音量差异，不只检查数据库字段存在。

## 9. Gapless

- Classic 的 MP3/AAC 导入提取 encoder delay、padding/drain、sample count 和必要 resync 信息；
- 只有准确数据才标记为准确 gapless；
- Photo 不承诺固件使用这些字段；
- 找不到数据时记录警告并继续，不写 dummy gapless；
- 实机验收使用真正连续的相邻曲目，不能只检查普通独立歌曲。

## 10. Artwork

- 只处理音乐/Audiobook artwork，不实现 Photo Library 同步；
- 按设备能力生成对应缩略图与 ArtworkDB/索引记录；
- 缺图或单张坏图不阻断音频导入；
- 设备不支持的尺寸/格式不能硬写；
- 封面使用 foobar artwork service 的 Front Cover，取第一张可解码图按设备能力处理；原图不写入项目数据库；
- 删除曲目后清理不再引用的 artwork 资源，不能影响共享封面。

## 11. Audiobooks

用户明确选择 Audiobooks 作为导入目标时，必须在设备能力允许时设置：

- Media Kind = Audiobook；
- Remember Playback Position；
- Skip When Shuffling；
- 支持时保留章节。

用户选择 Music 时不根据 Genre、`.m4b`、文件扩展名或其他字段偷偷改为 Audiobook。第一版可以在统一 Library 中查看、筛选和管理两种 Media Kind，不强制独立 Audiobooks 页签；导入目标的具体菜单、选择器或拖放入口在 UI 任务冻结。单个文件超过 FAT32 限制时在计划阶段拒绝。

## 12. 普通播放列表

用户可以：

- 通过设备 UI 的明确 New 操作新建普通 playlist，并重命名、删除和排序；
- 添加、移除和重排成员；
- 在任意 foobar playlist 中选择大量曲目，并把“曲目选择”加入已存在的设备普通 playlist；禁止发送或拖入 foobar playlist 对象，也不按名称自动创建、替换或合并容器；
- 在一次操作中导入缺失曲目并建立引用；
- 明确区分删除 playlist 容器、从 playlist 移除和从设备删除。

加入成员时：设备 Library 中不存在的曲目先按导入规则写入；已存在但尚未属于目标 playlist 的曲目复用现有 track ID；已在目标 playlist 的成员默认 Skip。`Replace metadata` 保持延后，playlist 操作不能自动覆盖设备曲目记录。不提供后台保持同步。

## 13. Smart Playlist

产品实现原生 Apple/iPod Smart Playlist 编辑器，而不是普通 playlist 快照伪装。

编辑器必须：

- 根据目标设备能力列出字段、运算符、组合、限制和排序；
- 支持全部满足/任一满足和可表达的嵌套规则；
- 支持数量、时长或容量限制及支持的选择方式；
- 明确 Live Updating 或需要重新连接/手动刷新的语义；
- 写入规则和设备所需的初始成员；
- 读取并编辑已有 iTunes Smart Playlist，同时保留未修改的有效字段；
- 对不能表达的 foobar 查询明确说明，不能声称无损转换。

Smart Playlist 成员完全由规则计算，不接受手工拖入曲目。foobar autoplaylist 对象不能发送、拖入、快照或翻译；用户仍可选择其中曲目执行普通导入或加入普通设备 playlist。只有实机确认的规则组合显示 Live Updating；可保存但不能实时重算的规则明确标记 Refresh on next connection 并由用户主动刷新；不支持规则拒绝保存。

Rating 规则使用设备原生 Rating，而不是在设备端执行 foobar 查询。首要用户场景包括 `Rating = 2/3/4/5`，以及设备能力允许时的 `Rating = N AND Artist = value`；每个目标型号对字段、运算符和 Live Updating 的准确支持仍须通过任务 014 的数据库往返与实机验证冻结。

## 14. 删除

安全顺序固定为：

```text
预检引用
→ 生成并验证不再引用目标的数据库
→ 备份并提交数据库
→ 删除无引用音频/artwork
→ 报告清理结果
```

数据库提交失败时绝不能先删音频。文件删除失败时报告孤立文件，不回滚已经有效的新数据库。用户可见命令和确认范围由 `DEC-PL-005/006` 冻结。

## 15. 事务、恢复与外部移除

- 同一设备不能并发写入；
- 写操作始终核对设备身份、挂载代次和数据库指纹；
- 旧数据库在新数据库完整验证前保持正式有效；
- FAT32 提交窗口使用恢复记录和 Last Known Good，不假装多文件原子；
- 取消只在安全边界生效并列出已发生结果；
- FooPodBridge 不提供 Eject、不调用系统弹出；操作结束后必须 Flush 并释放所有设备文件句柄；
- 用户通过 Windows 资源管理器弹出，事务中发生外部弹出或物理移除按 Interrupted/Recovery Required 处理；
- 物理移除进入 Interrupted/Recovery Required，不能重试旧盘符。

开发与实机测试阶段，每台物理设备首次写入前必须由用户在设备外建立并验证一次 `iPod_Control` 基线；每台设备在电脑端保留最近 10 个验证通过的数据库快照，最后一个 Last Known Good 不自动删除。两项策略在正式日常版本前重新核对，但当前测试路线不得提前取消。

自动清理只作用于具有有效操作记录、能够证明由 FooPodBridge 创建且不再被数据库引用的 `.tmp`/orphan；未知孤立音频只报告，不自动删除。

备份、清理、多设备和外部移除边界由 `DEC-SAFE-003` 至 `DEC-SAFE-008` 冻结。

## 16. FooCrate 集成

- FooCrate 检测 FooPodBridge 版本化服务；
- 服务缺失时隐藏 Devices，不影响原有播放、playlist、album、歌词和设置；
- 有设备时在 Playlist Browser 下方显示独立 Devices namespace；
- 设备选择进入 Device Workspace，不覆盖或污染 foobar Playlist Manager；
- 使用 FooCrate 已有主题、DPI、输入和状态模型；
- 不复制 Core、hash58、数据库和事务源码。

详细布局按 `DEC-UI-003` 在对应 UI 任务设计和批准。

## 17. 独立 Columns UI 与 Default UI

两个入口：

- 通过同一服务读取同一快照；
- 发出同一高层 Operation Request；
- 显示同一预检、进度、警告、失败和恢复语义；
- 不允许独立 UI 绕过服务层设备支持判断；
- 独立 Columns UI 功能与 FooCrate 等价，采用更通用的 Columns UI 布局和视觉，所有危险操作与 Smart Playlist 编辑均可达；
- Default UI 提供设备概览、Library/playlist 浏览、导入、删除和进度，复杂 Smart Playlist 编辑通过共享管理对话框打开；
- 所有入口均不提供 Eject。

## 18. Preferences 与诊断

Preferences 至少承载：

- 经批准的导入、SoundCheck、转码和重复策略；
- 数据库备份与清理状态；
- 受支持设备和服务版本；
- 诊断日志/脱敏导出；
- 恢复与 Last Known Good 管理；
- 恢复默认和版本迁移。

设置使用稳定 GUID、版本、合法值校验和迁移。不能靠修改隐藏配置绕过 Unsupported 写入。

诊断日志必须本地有界并默认遮蔽设备序列号和用户路径。诊断包只由用户主动导出，导出后可以预览，再由用户决定是否分享。

## 19. 性能与响应

- UI 线程不执行 USB、数据库、封面或源文件阻塞 I/O；
- 正常音频只经 USB 写一次；
- 一个批次只重建一次数据库；
- 元数据、封面、SoundCheck 和 gapless 可以在电脑侧流水准备；
- 数据库完整读回验证，音频默认使用写入结果、Flush 和大小校验；
- 进度按 Copying、Updating Library、Verifying、Cleaning 分阶段显示真实工作；
- 取消、关闭 UI 和快速设备变化不泄漏后台对象或回调。

## 20. 测试边界

- Core 测试使用脱敏 fixture 和临时目录；
- 故障注入覆盖短写、磁盘满、权限、Flush、重命名、数据库验证、hash 和设备代次变化；
- 组件自动化与两组件通信只使用 `D:\Dev\FooCrate\.local\foobar-dev`；
- 用户候选测试只使用 `D:\Dev\FooCrate\.local\foobar-test`；
- C 盘日常 foobar2000 永远不在测试范围；
- 实机写入由当前任务逐次授权，第一次写入前验证外部备份。

## 21. 首个完整版本验收

每个声明 `DeviceWriteVerified` 的数据库家族至少选择一台代表设备完成：

1. 识别、读取 Library、普通 playlist、Smart Playlist、容量和 artwork；
2. 导入一批 MP3/AAC，并验证 SoundCheck；
3. 声明 Classic gapless 能力时，必须在 Classic 上使用连续专辑单独验证；
4. 导入 Audiobook 并验证续播/随机播放排除；
5. 新建、编辑、重排和删除普通 playlist；
6. 新建并验证至少一组多规则 Smart Playlist；
7. 从 playlist 移除和从设备删除，确认语义不同；
8. 确认 FooPodBridge 已结束操作并释放句柄，通过 Windows 资源管理器弹出，设备重启后检查所有内容；
9. 再连接并确认数据库可读、空间和状态一致；
10. 执行至少一个可恢复的故障注入/中断演练。

FooCrate、独立 Columns UI 和 Default UI 分别完成其批准范围的日常流程。所有包、来源、许可证、隐私和测试实例边界通过审计。

## 22. 当前批准门槛

以下门槛已于 2026-08-28 满足，本规格成为任务 001 以后使用的已批准基线。后续实机证据或任务级发现可以显式重开受影响决定，但不能静默改变本规格。

本规格的批准条件为：

- 用户完成当前路线所需的 `USER_DECISIONS.md`；
- 用户批准 `tasks/TODO.md` 的永久任务拆分；
- 两台实机证据采集的位置、隐私和备份要求得到确认；
- 公开许可证或暂不公开的决定明确；
- 本规格与架构、安全模型、UI 方向不存在冲突。
