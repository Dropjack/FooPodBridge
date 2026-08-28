# FooPodBridge 用户决策清单

- 状态：当前路线所需决定已批准；明确延后与正式版前重开项保留
- 日期：2026-08-28
- 当前任务：[`../tasks/002-建立x64组件工程与服务合同/README.md`](../tasks/002-建立x64组件工程与服务合同/README.md)
- 作用：这是影响产品行为、设备数据、UI 或公开发布的唯一决策清单

## 1. 使用方法

不需要一次回复全部项目。当前任务按以下五轮讨论：

1. 实机与直接导入；
2. 元数据、SoundCheck、封面与 Audiobook；
3. playlist、Smart Playlist 与删除；
4. 备份、恢复、取消与多设备；
5. UI、诊断、许可证与发布。

每项都有推荐方案和影响。用户可以回复“采用推荐”，也可以指定其他方案。决定后把状态改为“已批准”并记录日期与理由。实现中若实机证据推翻决定，必须回到本清单重新核对，不能偷偷改变行为。

## 2. 已批准决定

| ID | 决定 | 已批准结果 |
| --- | --- | --- |
| DEC-ARCH-001 | 重建路线 | 建立全新 x64 架构，只从 foo_dop/libgpod 提取已验证知识和许可证兼容算法 |
| DEC-ARCH-002 | Core 边界 | `device/database/media/transaction` 不依赖 foobar 或 UI，可独立测试 |
| DEC-ARCH-003 | 组件边界 | FooPodBridge 与 FooCrate 是两个组件，通过版本化 foobar 服务连接 |
| DEC-DEV-001 | 正式设备 | 优先且只承诺用户实测的 iPod Photo 与 iPod Classic；未知设备拒绝写入 |
| DEC-SCOPE-001 | 管理方式 | 纯手动音乐管理，不提供媒体库镜像、后台同步或一键 Sync |
| DEC-SCOPE-002 | 媒体范围 | Music 与 Audiobooks；排除 Podcasts、Video、Photo、iPhone 和 iPod touch |
| DEC-SCOPE-003 | 播放数据 | 不双向同步播放次数、评分、跳过次数和播放位置 |
| DEC-MEDIA-001 | SoundCheck | 导入必须支持 ReplayGain 到 iPod SoundCheck |
| DEC-MEDIA-002 | Gapless | Classic 自动写准确数据；Photo 不承诺；失败只警告；不写 dummy 数据 |
| DEC-PL-001 | Smart Playlist | 实现原生 iPod Smart Playlist 编辑器，目标是在设备能力范围内做到完整 |
| DEC-SAFE-001 | 事务顺序 | 导入先音频后 DB；删除先 DB 后音频；每批只提交一次 DB |
| DEC-SAFE-002 | 性能 | 大文件只经 USB 写一次；正常音频不完整读回；小型 DB 每次完整验证 |
| DEC-UI-001 | UI 顺序 | FooCrate 集成优先，独立 Columns UI 其次，Default UI 使用简化原生外观 |
| DEC-UI-002 | 视觉稿时机 | 当前只冻结信息结构，详细 mockup 到对应 UI 任务再设计 |
| DEC-TEST-001 | 运行环境 | AI 只用 FooCrate `foobar-dev`；用户只用 `foobar-test`；不碰 C 盘日常安装 |
| DEC-PKG-001 | 交付 | 输出独立 FooPodBridge 与 FooCrate `.fb2k-component`，不混装 DLL |
| DEC-IMP-001 | 正式直接复制格式 | 按设备能力矩阵判断；首批验证 MP3、AAC-LC/M4A 和设备确认支持的 ALAC；其他格式按真实需求与样本后续加入 |
| DEC-IMP-002 | 不受支持格式 | 计划中逐项显示 Unsupported；默认不复制、不改文件，兼容项目可以继续 |
| DEC-IMP-004 | 重复项判断 | 使用强匹配，默认 Skip；允许 Add duplicate；Replace metadata 暂不纳入冻结需求，待测试时重开决定 |
| DEC-IMP-005 | 单曲失败 | 独立单曲问题警告并继续；空间、身份、数据库或事务错误立即停止批次 |
| DEC-IMP-006 | 设备安全余量 | 导入后保留 `max(256 MiB, 总容量的 1%)`；用户可主动调低但不能设为零 |
| DEC-META-001 | 元数据来源与字段范围 | 使用 foobar2000 正式 metadb/file info；写设备实际支持的核心字段，次要字段尽力写入，不反写源文件 |
| DEC-META-002 | 缺少 ReplayGain | Track Gain 优先、Album Gain 次之；都缺失则不写 SoundCheck并提示，不自动扫描 |
| DEC-META-003 | 封面来源 | 使用 foobar artwork service 的 Front Cover；第一张可解码图按设备能力处理，缺图不阻断 |
| DEC-META-004 | Audiobook 分类 | 用户明确选择 Music/Audiobooks 导入目标；不根据 Genre、扩展名或其他字段偷偷改类型，支持时保留章节 |
| DEC-META-005 | 缺少关键标签 | Title 缺失使用文件名；Artist/Album 使用 Unknown；可编辑本批设备值但不反写源文件 |
| DEC-META-006 | Compilation 判定 | 明确标签优先；缺失时按统一 Album Artist + Album 且存在不同 Artist 自动推断，并合并本次、设备与 foobar 曲库证据 |
| DEC-META-007 | Playback Statistics 评分 | `%rating%` 是电脑端唯一来源；导入时写设备 Rating，已有曲目只在明确刷新时单向覆盖，缺失评分清空设备值 |
| DEC-PL-002 | 普通 playlist 创建与加曲 | 设备 UI 明确 New；禁止发送或拖入 foobar playlist 对象；允许把选中曲目批量加入已有设备 playlist |
| DEC-PL-003 | foobar autoplaylist | 禁止发送、拖入、快照或翻译 autoplaylist 对象；可选择其中曲目执行普通导入或加入普通设备 playlist |
| DEC-PL-004 | Smart Playlist 非实时规则 | 只有实机确认的组合显示 Live；其他可表达规则明确标记并由用户主动刷新，不支持规则拒绝 |
| DEC-PL-005 | 删除语义 | Remove from Playlist 只删引用；Delete from iPod 显示影响并先提交 DB、后删文件 |
| DEC-PL-006 | 删除后空 playlist | 保留空普通/Smart Playlist，只有用户明确删除容器才移除 |
| DEC-SAFE-003 | 首次完整备份 | 开发/实机测试阶段每台设备首次写入前在设备外建立并验证一次 `iPod_Control` 基线；正式日常版前重开 |
| DEC-SAFE-004 | 数据库恢复点 | 开发/实机测试阶段每台设备保留最近 10 个验证快照及至少一个 Last Known Good；正式日常版前重开数量 |
| DEC-SAFE-005 | 临时/孤立文件 | 自动清理有操作记录且可证明由本项目创建的临时/orphan；未知未引用文件只报告 |
| DEC-SAFE-006 | 取消后的成功项目 | 提交前取消不更新 DB，已复制文件作为本项目 orphan 清理；提交安全边界开始后完成或恢复 |
| DEC-SAFE-007 | 多设备并发 | 可同时显示多台设备，但全进程一次只执行一个写事务；首版实机测试后复核 |
| DEC-SAFE-008 | 设备弹出边界 | FooPodBridge 不提供或调用 Eject；完成后释放全部句柄，由用户通过 Windows 资源管理器弹出 |
| DEC-UI-004 | 界面语言 | 与当前 FooCrate 一致，首版可见文案使用英文；规格和错误逻辑使用中文记录 |
| DEC-UI-005 | 独立 Columns UI 功能范围 | 功能与 FooCrate 等价，布局与视觉更通用；危险操作和 Smart Playlist 编辑均可达 |
| DEC-UI-006 | Default UI 功能范围 | 提供设备概览、浏览、导入、删除和进度；复杂 Smart Playlist 编辑使用统一管理对话框；不提供 Eject |
| DEC-DIAG-001 | 诊断日志与导出 | 本地有界日志默认遮蔽序列号和用户路径；用户主动导出脱敏诊断包并在分享前预览 |
| DEC-LIC-001 | 公开许可证 | FooCrate 保持 MIT；FooPodBridge 为 LGPL-3.0-or-later；hash58 为 BSD-3-Clause；原创共享合同为 0BSD；其余边界按已验收审计 |
| DEC-PKG-002 | 候选发布节奏 | Core 自动测试持续运行；只有形成可人工验证的永久能力时才同时输出新的 FooPodBridge/FooCrate prerelease 包 |

## 3. 必须提供的实机证据

这些不是审美选择，而是允许写入前必须收集的输入。公开文档不记录完整序列号。

### EVID-DEV-001：iPod Photo

- Windows 显示容量与文件系统；
- Apple 型号/容量/固件；
- 是否更换 CF/SD/SSD、容量与转接方式；
- `iPod_Control/Device` 中可公开的型号能力文件；
- 当前 Library 曲目与 playlist 数量；
- 一份位于 iPod 之外、可恢复的 `iPod_Control` 备份；
- 用户允许反复导入/删除的一首测试音频。

状态：尚未采集。阻断 Photo 实机写入，不阻断只读 Core 和 fixture 工作。

### EVID-DEV-002：iPod Classic

要求同上，另需取得 hash58 所需稳定设备 ID，诊断输出必须遮蔽大部分字符。

状态：尚未采集。阻断 Classic 实机写入，不阻断 hash58 已知向量测试。

## 4. 第一轮：直接导入

### DEC-IMP-001：正式直接复制格式

问题：哪些格式/profile 可以不转码直接导入？设备代际能力不同，不能只看扩展名。

- A（推荐）：以设备能力矩阵为准；首批正式验证 MP3、AAC-LC/M4A 与设备确认支持的 ALAC。WAV/AIFF/Audible 在用户有真实需求和样本时再加入。
- B：把 Apple 历史规格列出的所有音频格式一次性纳入首个目标。
- C：只支持 MP3 与 AAC-LC，其他即使设备可能支持也拒绝。

状态：已批准（2026-08-26），采用 A。首批只把 MP3、AAC-LC/M4A 和目标设备确认支持的 ALAC 纳入正式直接复制验证；WAV、AIFF、Audible 等格式等待真实需求和样本，不因历史规格自动宣称支持。

### DEC-IMP-002：不受支持格式的默认行为

- A（推荐）：计划阶段逐项显示 Unsupported，兼容项目仍可继续；默认不复制、不改文件。
- B：一个不支持项目阻断整个批次。
- C：无提示跳过。

状态：已批准（2026-08-26），采用 A。不支持项目逐项显示 Unsupported，默认不复制、不修改源文件或设备；批次中的兼容项目仍可继续进入计划。

### DEC-IMP-003：FLAC 自动转码

用户已决定稍后再判断。当前正式行为冻结为“识别 FLAC、明确拒绝直接导入、不自动转码”。若以后批准转码，建立独立永久任务并决定编码器、AAC profile、质量、临时空间和元数据保留。

状态：延后决定（2026-08-26 再确认）；当前优先执行 `DEC-IMP-001/002`，FLAC 只识别并明确拒绝直接导入，不自动转码。

### DEC-IMP-004：重复项判断

- A（推荐）：用标准化 Album Artist/Album/Disc/Track/Title、时长容差和媒体类型生成强匹配；默认 Skip，并允许用户在计划中逐项改为 Add duplicate 或 Replace metadata。不同编码版本显示为疑似重复，不自动覆盖。
- B：只按文件名判断。
- C：每次对源文件和设备文件做完整内容哈希。

状态：已批准（2026-08-26），采用经收窄的 A。使用标准化字段、时长容差和媒体类型强匹配，默认 Skip，并允许用户明确选择 Add duplicate。Replace metadata 暂不纳入已冻结需求，待后续重复项测试时重新核对；在此之前不能自动覆盖设备记录。

### DEC-IMP-005：批次中的单曲失败

- A（推荐）：格式、gapless、封面等独立单曲失败记录警告并继续其他曲目；空间、设备身份、数据库或事务错误立即停止批次。
- B：任何一首失败都回滚整批。
- C：任何错误都尽量继续。

状态：已批准（2026-08-26），采用 A。格式、gapless、封面等独立单曲问题记录警告并继续；空间、设备身份、数据库或事务错误立即停止批次。

### DEC-IMP-006：设备安全余量

- A（推荐）：导入后至少保留 `max(256 MiB, 总容量的 1%)`，用户可以主动调低但不能变为零。
- B：只要文件系统报告能放下就允许写满。
- C：固定保留 1 GiB。

状态：已批准（2026-08-26），采用 A。导入后至少保留 `max(256 MiB, 总容量的 1%)`；用户可以主动调低，但不能设为零。

## 5. 第二轮：元数据、SoundCheck、封面与 Audiobook

### DEC-META-001：元数据来源优先级

- A（推荐）：使用 foobar2000 对目标曲目解析后的正式 metadb/file info；允许标题格式只影响显示，不把 UI 拼接字符串写入设备。缺失字段才使用安全回退。
- B：直接读取文件标签，忽略 foobar 组件提供的字段。
- C：写入用户在 FooPodBridge 单独维护的第二份元数据。

状态：已批准（2026-08-26），采用 A。读取 foobar2000 正式 metadb/file info 的原始字段，不把带回退的 UI 标题格式结果当作设备元数据。核心用户字段至少包含 Title、Artist、Album Artist、Album、Compilation、Year/Date、Track Number 和 Disc Number；设备明确支持的 Genre、Composer、BPM、Sort 等次要字段在源数据存在时尽力写入，缺失不阻断，设备不使用的字段忽略。源文件标签不反写。

### DEC-META-002：缺少 ReplayGain 时的 SoundCheck

- A（推荐）：优先使用 ReplayGain track gain；缺失时使用 album gain；两者都缺失则不写 SoundCheck并在计划中提示，不自动扫描整首音频。
- B：缺失时自动执行 ReplayGain 扫描后再导入。
- C：所有曲目都不看已有值，统一重新扫描。

状态：已批准（2026-08-26），采用 A。优先使用 ReplayGain Track Gain，缺失时使用 Album Gain；两者都缺失则不写 SoundCheck并在计划中提示，不自动扫描整首音频。

### DEC-META-003：封面来源

- A（推荐）：使用 foobar artwork service 的 Front Cover；取第一张可解码图，按设备能力生成缓存，原图不写入项目数据库；缺图不阻断导入。
- B：只使用音频内嵌封面。
- C：支持每曲多张封面和封面类型编辑。

状态：已批准（2026-08-26），采用 A。使用 foobar artwork service 的 Front Cover，取第一张可解码图并按设备能力生成缓存；缺图不阻断导入，不把原图写入项目数据库。

### DEC-META-004：Audiobook 分类

- A（推荐）：拖到 `Audiobooks` 就明确写 Media Kind=Audiobook、Remember Playback Position、Skip When Shuffling；拖到 Music 不根据 Genre 或文件扩展名偷偷改类型。支持时保留章节。
- B：自动根据 M4B/Genre 推断，用户不选目标。
- C：Music 与 Audiobooks 没有行为差异。

状态：已批准（2026-08-26），采用 A 的明确用户意图。用户选择 Music 或 Audiobooks 作为导入目标；选择 Audiobooks 时写 Media Kind=Audiobook、Remember Playback Position、Skip When Shuffling，并在支持时保留章节；选择 Music 时不根据 Genre、`.m4b` 或其他字段偷偷改类型。第一版可以在统一 Library 中管理两种 Media Kind，不要求独立 Audiobooks 页签；具体目标选择/拖放入口留到 UI 任务冻结。

### DEC-META-005：缺少关键标签

- A（推荐）：Title 缺失时使用不含路径的文件名；Artist/Album 显示 Unknown；在计划中可编辑本批设备值，但不反写源文件。
- B：关键标签缺失就拒绝导入。
- C：原样写空字段。

状态：已批准（2026-08-26），采用 A。Title 缺失时使用不含路径的文件名；Artist/Album 缺失显示 Unknown；计划中可编辑本批设备值，但不反写源文件。

### DEC-META-006：Compilation 判定

问题：foobar2000 曲库，尤其 FLAC，通常只用统一的 Album Artist 管理合辑，没有独立 Compilation 标签；FooPodBridge 需要在不要求用户补标签的前提下生成 iPod Compilation 状态。

批准规则：

1. 明确、可解析的 Compilation/ITUNESCOMPILATION 值优先；明确 false 不被自动推断覆盖。
2. 没有明确值时，以规范化后的原始 Album Artist + Album 作为专辑身份；不能使用会回退到 Artist/Composer/Performer 的显示字段。
3. 同一专辑具有统一且非空的 Album Artist，并存在至少两个不同的 Artist 时，自动推断 Compilation=true；Album Artist 不要求是 `Various Artists`。
4. 推断证据合并本次待导入曲目、设备上已有的同专辑曲目，以及 foobar 曲库中能够无歧义匹配的同专辑曲目。
5. 一旦推断成立，同一事务把设备上同专辑曲目的 Compilation 状态保持一致；保留各曲目 Artist 和原始 Album Artist，不强制改成 `Various Artists`。
6. Operation Plan 显示 `Compilation: inferred`。推断不修改 foobar2000 源文件标签。
7. 已知权衡：固定 Album Artist、但具有不同客串 Artist 的普通专辑也可能被判为 Compilation；用户接受这一自动化规则。

状态：已批准（2026-08-26）。

### DEC-META-007：Playback Statistics 评分导出到设备

背景：FooCrate 不保存私有评分；它通过 foobar2000 正式 metadb 句柄读取 Playback Statistics 提供的 `%rating%`，并调用该组件的 `Rating/1` 至 `Rating/5` 命令写入。FooPodBridge 因此可以直接读取同一个动态字段，不依赖 FooCrate，也不读取音频文件 `RATING` 标签。

- A（推荐）：以 Playback Statistics 的 `%rating%` 为电脑端唯一评分来源。新导入曲目把 1–5 星写成设备原生 Rating；已有设备曲目只在用户明确执行 `Refresh ratings from foobar` 时批量刷新。刷新是带 Operation Plan 的单向快照：缺失评分清除设备 Rating，设备端评分不反写 foobar，下一次明确刷新可以覆盖设备端改动；不后台同步。原生 Smart Playlist 可据此使用 `Rating = N`，并在设备能力允许时与 Artist 等条件组合。
- B：只在曲目首次导入时复制评分；以后不提供批量刷新，评分变化可能长期不反映到设备 Smart Playlist。
- C：不向设备导出评分；Rating 类原生 Smart Playlist 只能依赖用户在设备端维护的评分。

状态：已批准（2026-08-26），采用 A。用户明确确认 Playback Statistics 是电脑端唯一评分事实来源；明确刷新时，缺失 `%rating%` 会清空设备 Rating，设备端评分改动会在下一次明确刷新时被电脑端值覆盖。该功能是用户触发、带 Operation Plan 的单向导出/刷新，不后台运行、不从设备反写电脑，因此不改变 `DEC-SCOPE-003` 禁止双向同步的边界。

## 6. 第三轮：playlist、Smart Playlist 与删除

### DEC-PL-002：设备普通 playlist 创建与曲目加入

- A（推荐）：如果是本次会话中明确选中的同一设备 playlist，则 Replace contents；只按名称撞车时要求选择 Replace、Create new 或 Cancel，不默认 Merge。
- B：总是 Merge 并去重。
- C：总是创建带编号的新 playlist。

状态：已批准（2026-08-26），不采用直接发送 playlist 的 A/B/C 流程。设备普通 playlist 只能通过设备 UI 的明确 New 操作创建；禁止把 foobar 普通 playlist 对象发送或拖入设备，也不根据名称创建、替换或合并容器。用户可以在任意 foobar playlist 中选择大量曲目，并把“曲目选择”加入已存在的设备普通 playlist；已有设备 playlist 仍可重命名、增删成员、调整顺序和删除。

曲目加入规则：

1. 曲目不在设备 Library：按第一轮导入规则写入，成功后加入 playlist。
2. 曲目已在设备 Library、但不在目标 playlist：不复制音频，复用现有设备 track ID 加入 playlist。
3. 曲目已在目标 playlist：默认 Skip，不建立重复成员。
4. `Replace metadata` 继续保持延后决定，不能因 playlist 加曲自动覆盖设备记录。

### DEC-PL-003：发送 foobar autoplaylist

- A（推荐）：用户手动发送当前结果，生成/替换普通 iPod playlist 快照；不自动同步，不假装翻译查询。
- B：尝试把所有 foobar 查询自动翻译成 Apple Smart Playlist，翻译失败才快照。
- C：禁止发送 autoplaylist。

状态：已批准（2026-08-26），采用禁止传输对象的方向。foobar autoplaylist 不能发送、拖入、生成普通快照或自动翻译为 iPod Smart Playlist。用户可以打开 autoplaylist 并选择其中的曲目，把这些曲目作为普通批次导入或加入普通设备 playlist；这不传输 autoplaylist 对象。原生 iPod Smart Playlist 只通过 FooPodBridge 规则编辑器明确新建和编辑。

### DEC-PL-004：Smart Playlist 非实时规则

- A（推荐）：编辑器只允许目标设备确认支持的 Live Updating 组合；可保存但需要重新连接刷新的规则必须明确标记 Refresh on next connection，并由用户主动刷新。
- B：只要数据库格式能写就全部允许，并一律显示 Live。
- C：完全禁止非实时规则。

状态：已批准（2026-08-26），采用 A。只有目标设备实机确认支持的规则组合才显示 Live Updating；可保存但不能由设备实时重算的规则明确标记 Refresh on next connection，并由用户主动刷新；不能表达或未经确认的字段/运算符拒绝保存。具体能力矩阵留到参考审计、格式测试和 Photo/Classic 实机验证。

### DEC-PL-005：删除语义

- A（推荐）：`Remove from Playlist` 只移除引用；`Delete from iPod` 显示受影响 playlist 和文件大小并确认，成功提交 DB 后才删文件。
- B：Delete 键总是从设备彻底删除。
- C：组件只允许从 playlist 移除，不允许删设备曲目。

状态：已批准（2026-08-26），采用 A。`Remove from Playlist` 只移除引用；`Delete from iPod` 显示受影响 playlist 和文件大小并确认，成功提交不再引用目标的数据库后才删除文件。

### DEC-PL-006：删除后空 playlist

- A（推荐）：保留用户创建的空普通/Smart Playlist；只有用户明确删除容器才移除。
- B：自动删除空 playlist。

状态：已批准（2026-08-26），采用 A。保留用户创建的空普通/Smart Playlist；只有用户明确删除 playlist 容器时才移除。

## 7. 第四轮：备份、恢复与设备生命周期

### DEC-SAFE-003：第一次完整备份位置

这里的“完整备份”不是 FooPodBridge 日常提供的 iPod 备份服务，也不是每次导入都复制音乐。它只是在 FooPodBridge 第一次对某台实机执行开发/正式写入前，由用户把可恢复的 `iPod_Control` 基线保存到 iPod 之外；目的是在早期 Writer、数据库格式或设备差异判断出错时，仍能恢复写入前状态。

- A（推荐）：用户选择 iPod 之外的目录；FooPodBridge记录备份清单、数据库指纹和验证结果，但不把备份提交 Git。
- B：只备份到同一 iPod 隐藏目录。
- C：不验证完整备份，只保存当前 iTunesDB。

状态：阶段性批准（2026-08-26），开发与实机测试阶段采用 A。每台物理设备首次写入前，由用户在设备外建立并验证一次 `iPod_Control` 基线；不要求日常重复完整备份，也不把 FooPodBridge 扩展成通用 iPod 备份工具。正式日常版本前重新核对是否继续保留该门槛。

### DEC-SAFE-004：每次事务数据库备份保留

这里保存的是电脑端的小型设备数据库恢复点与指纹，不复制音频，也不是 artwork/媒体缓存，不会加速浏览或导入。用途是在本次提交中断，或数次操作后才发现数据库逻辑错误时，找到经过验证的旧数据库版本；占用通常远小于音乐文件。

- A（推荐）：按物理设备保存最近 10 个验证通过的数据库快照，并永不自动删除最后一个 Last Known Good；Preferences 可查看占用和手动清理。
- B：只保留上一次。
- C：永久保留全部。

状态：阶段性批准（2026-08-26），开发与实机测试阶段采用 A：按物理设备保留最近 10 个验证通过的数据库快照，最后一个 Last Known Good 永不自动删除，用户可查看占用并手动清理较旧快照。正式日常版本前重新核对保留数量；即使以后取消 10 份历史，也不能取消事务中的临时回滚保护和至少一个 Last Known Good。

### DEC-SAFE-005：FooPodBridge 临时/孤立文件清理

这是事故收尾而不是“清理 iPod”功能：取消、崩溃或数据库提交失败后，可能留下未被数据库引用的临时音频。FooPodBridge 只对拥有有效操作记录、能够证明由自己创建的文件自动处理；对来源不明的未引用文件没有所有权。

- A（推荐）：自动清理有有效操作记录、明确由本项目创建且不被 DB 引用的 `.tmp`；未知孤立音频只报告并让用户选择。
- B：自动删除所有 DB 未引用文件。
- C：从不清理，只报告路径。

状态：已批准（2026-08-26），采用 A。自动范围永久限于具有有效操作记录、能够证明由 FooPodBridge 创建且不被数据库引用的 `.tmp`/orphan；未知孤立音频只报告并由用户明确选择，不把 FooPodBridge 做成通用磁盘清理器。

### DEC-SAFE-006：取消后的成功项目

- A（推荐）：提交前取消则不更新 DB，成功复制文件作为本项目 orphan 清理；提交阶段进入安全边界后完成或恢复，不产生“半个 playlist”。
- B：取消时把已复制曲目提交到 DB。

状态：已批准（2026-08-26），采用 A。提交前取消不更新设备数据库，已经复制的文件作为有操作记录的本项目 orphan 进入清理；数据库提交安全边界开始后不强行打断关键序列，而是完成到有效状态或按恢复记录恢复，不能留下半个 playlist。

### DEC-SAFE-007：多设备并发

- A（推荐）：可以同时显示多台设备，但全进程一次只允许一个写事务；第一版避免两个 USB/FAT 事务和两套进度同时竞争。
- B：每台设备各自并行写入。
- C：只显示第一台设备。

状态：已批准（2026-08-26），采用 A。可以同时发现和浏览多台设备，但 FooPodBridge 全进程一次只允许一个写事务，其他设备的写请求排队或明确拒绝；首版在两台实机同时连接场景复核后再决定是否有必要放宽。

### DEC-SAFE-008：完成后弹出

- A（推荐）：永不自动弹出；显示完成状态和明确 Eject 按钮。
- B：每个成功批次自动弹出。
- C：Preferences 可设置自动弹出。

状态：已批准（2026-08-26），不采用 A/B/C 中由组件执行弹出的设计。FooPodBridge 不提供 Eject 按钮、不调用系统弹出，也不把整台 iPod 的移除生命周期当作产品职责；每次操作结束后必须及时 Flush 并释放所有设备文件句柄，由用户使用 Windows 资源管理器的 Eject。事务中发生外部弹出或物理移除仍按中断/恢复模型处理。

## 8. 第五轮：UI、诊断、许可证与发布

### DEC-UI-003：FooCrate Devices 信息结构

初步方向是 `Playlists` 下方增加 `Devices`，设备下显示 Music、Audiobooks 和 Playlists；选择后进入独立 Device Workspace。中央/右栏的最终分工、传输队列位置、容量条和 Smart Playlist 编辑入口在 FooCrate UI 规格任务用真实布局共同决定。

状态：明确延后到 UI 任务；不阻断 Core，UI 任务不得在未核对前实现。

### DEC-UI-004：界面语言

- A（推荐）：与当前 FooCrate 一致，首版可见文案使用英文；规格和错误逻辑用中文记录。
- B：首版中文。
- C：首版即中英本地化。

状态：已批准（2026-08-28），采用 A。与当前 FooCrate 一致，首版用户可见文案使用英文；中文继续作为规格、程序逻辑、错误语义和用户沟通的记录语言。

### DEC-UI-005：独立 Columns UI 功能范围

- A（推荐）：功能与 FooCrate 等价，布局与视觉更通用；所有危险操作和 Smart Playlist 编辑均可达。
- B：只读浏览，写入必须去 FooCrate。
- C：只提供设备树，打开独立管理窗口执行其余操作。

状态：已批准（2026-08-28），采用 A。独立 Columns UI 功能与 FooCrate 等价，布局和视觉采用更通用的 Columns UI 形式；所有危险操作、完整结果和 Smart Playlist 编辑均必须可达。

### DEC-UI-006：Default UI 功能范围

- A（推荐）：设备概览、Library/playlist 浏览、导入、删除和进度；复杂 Smart Playlist 编辑通过统一管理对话框打开。所有入口都不提供 Eject。
- B：与 FooCrate 完全同布局。
- C：只读设备信息。

状态：已批准（2026-08-28），采用 A。Default UI 提供设备概览、Library/playlist 浏览、导入、删除和进度；复杂 Smart Playlist 编辑通过三个入口共享的统一管理对话框打开。所有入口均不提供 Eject。

### DEC-DIAG-001：诊断日志与导出

- A（推荐）：本地有界日志默认遮蔽序列号和用户路径；用户主动导出脱敏诊断包，预览后再分享。
- B：只在 Debug 构建记录日志。
- C：Release 记录完整路径和设备信息。

状态：已批准（2026-08-28），采用 A。本地日志有明确容量/轮换边界，默认遮蔽设备序列号和用户路径；诊断包只由用户主动导出，导出后可以预览，再由用户决定是否分享。

### DEC-LIC-001：公开许可证

- A（推荐）：完成文件级审计后采用与实际复用代码兼容的 LGPL 许可证；若采用 LGPLv3 参考实现，优先 LGPL-3.0-or-later。
- B：完全不采用参考源码，只依据格式知识重写，再评估更宽松许可证。
- C：暂不公开源码并保留全部权利。

状态：已批准并完成审计验收（2026-08-28），采用 A。FooCrate 保持 MIT；FooPodBridge 原创源码使用 `LGPL-3.0-or-later`；唯一计划修改采用的第三方算法是 libgpod `src/itdb_hash58.c`，该文件保持 `BSD-3-Clause`；FooPodBridge/FooCrate 共享服务合同使用本项目原创 `0BSD`；foo_dop 与 libgpod 其余候选源码只作知识参考后原创实现。`iTunesCrypt.dll`、旧 SDK、iOS 与 hash72/CBK 路径保持禁止。文件级证据、许可证版本、保留声明和任务 002–014 采用边界以 [`../docs/REFERENCE_PROVENANCE.md`](../docs/REFERENCE_PROVENANCE.md) 为准；若后续需要复制未获批准的第三方表达，必须先重开任务 001。

### DEC-PKG-002：候选发布节奏

- A（推荐）：Core 自动测试持续运行；只有形成可人工验证的永久能力时才同时输出新的 FooPodBridge/FooCrate prerelease 包。
- B：每个内部提交都生成并交给用户安装。
- C：直到全部功能结束才给任何候选。

状态：已批准（2026-08-28），采用 A。Core 自动测试持续运行；只有形成可由用户人工验证的永久能力时，才同时输出新的 FooPodBridge 与 FooCrate prerelease 包，不为每个内部提交要求用户安装候选。

## 9. 路线批准

状态：已批准（2026-08-28）。用户默认认同 [`tasks/TODO.md`](../tasks/TODO.md) 的建议路线，并选择在进入具体任务时按需提问；后续新证据仍可在受影响任务开始前显式重开对应决定。

完成上述决定后，用户还需核对 [`tasks/TODO.md`](../tasks/TODO.md) 中每个永久任务的目标、依赖和实机检查点。任务 000 只有在以下条件全部满足时才能标记“已验收”：

- 所有阻断当前路线的“待决定”项目变为“已批准”或“明确排除”；
- 两台实机证据采集任务的位置与隐私边界明确；
- 用户批准完整任务路线；
- 产品规格、架构、安全模型和任务索引没有冲突。
