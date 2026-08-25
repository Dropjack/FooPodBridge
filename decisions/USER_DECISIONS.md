# FooPodBridge 用户决策清单

- 状态：等待用户逐项核对
- 日期：2026-08-26
- 当前任务：[`../tasks/000-确定项目目标与全部产品决策/README.md`](../tasks/000-确定项目目标与全部产品决策/README.md)
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

状态：待决定。阻断 media capability 规格。

### DEC-IMP-002：不受支持格式的默认行为

- A（推荐）：计划阶段逐项显示 Unsupported，兼容项目仍可继续；默认不复制、不改文件。
- B：一个不支持项目阻断整个批次。
- C：无提示跳过。

状态：待决定。C 不符合项目错误透明原则，不建议。

### DEC-IMP-003：FLAC 自动转码

用户已决定稍后再判断。当前正式行为冻结为“识别 FLAC、明确拒绝直接导入、不自动转码”。若以后批准转码，建立独立永久任务并决定编码器、AAC profile、质量、临时空间和元数据保留。

状态：延后决定；不阻断无转码产品路线。

### DEC-IMP-004：重复项判断

- A（推荐）：用标准化 Album Artist/Album/Disc/Track/Title、时长容差和媒体类型生成强匹配；默认 Skip，并允许用户在计划中逐项改为 Add duplicate 或 Replace metadata。不同编码版本显示为疑似重复，不自动覆盖。
- B：只按文件名判断。
- C：每次对源文件和设备文件做完整内容哈希。

状态：待决定。A 避免 C 的 USB 读回成本，也比 B 可靠。

### DEC-IMP-005：批次中的单曲失败

- A（推荐）：格式、gapless、封面等独立单曲失败记录警告并继续其他曲目；空间、设备身份、数据库或事务错误立即停止批次。
- B：任何一首失败都回滚整批。
- C：任何错误都尽量继续。

状态：待决定。

### DEC-IMP-006：设备安全余量

- A（推荐）：导入后至少保留 `max(256 MiB, 总容量的 1%)`，用户可以主动调低但不能变为零。
- B：只要文件系统报告能放下就允许写满。
- C：固定保留 1 GiB。

状态：待决定。阻断空间预检。

## 5. 第二轮：元数据、SoundCheck、封面与 Audiobook

### DEC-META-001：元数据来源优先级

- A（推荐）：使用 foobar2000 对目标曲目解析后的正式 metadb/file info；允许标题格式只影响显示，不把 UI 拼接字符串写入设备。缺失字段才使用安全回退。
- B：直接读取文件标签，忽略 foobar 组件提供的字段。
- C：写入用户在 FooPodBridge 单独维护的第二份元数据。

状态：待决定。

### DEC-META-002：缺少 ReplayGain 时的 SoundCheck

- A（推荐）：优先使用 ReplayGain track gain；缺失时使用 album gain；两者都缺失则不写 SoundCheck并在计划中提示，不自动扫描整首音频。
- B：缺失时自动执行 ReplayGain 扫描后再导入。
- C：所有曲目都不看已有值，统一重新扫描。

状态：待决定。

### DEC-META-003：封面来源

- A（推荐）：使用 foobar artwork service 的 Front Cover；取第一张可解码图，按设备能力生成缓存，原图不写入项目数据库；缺图不阻断导入。
- B：只使用音频内嵌封面。
- C：支持每曲多张封面和封面类型编辑。

状态：待决定。

### DEC-META-004：Audiobook 分类

- A（推荐）：拖到 `Audiobooks` 就明确写 Media Kind=Audiobook、Remember Playback Position、Skip When Shuffling；拖到 Music 不根据 Genre 或文件扩展名偷偷改类型。支持时保留章节。
- B：自动根据 M4B/Genre 推断，用户不选目标。
- C：Music 与 Audiobooks 没有行为差异。

状态：待决定。

### DEC-META-005：缺少关键标签

- A（推荐）：Title 缺失时使用不含路径的文件名；Artist/Album 显示 Unknown；在计划中可编辑本批设备值，但不反写源文件。
- B：关键标签缺失就拒绝导入。
- C：原样写空字段。

状态：待决定。

## 6. 第三轮：playlist、Smart Playlist 与删除

### DEC-PL-002：发送 foobar 普通 playlist 时名称冲突

- A（推荐）：如果是本次会话中明确选中的同一设备 playlist，则 Replace contents；只按名称撞车时要求选择 Replace、Create new 或 Cancel，不默认 Merge。
- B：总是 Merge 并去重。
- C：总是创建带编号的新 playlist。

状态：待决定。

### DEC-PL-003：发送 foobar autoplaylist

- A（推荐）：用户手动发送当前结果，生成/替换普通 iPod playlist 快照；不自动同步，不假装翻译查询。
- B：尝试把所有 foobar 查询自动翻译成 Apple Smart Playlist，翻译失败才快照。
- C：禁止发送 autoplaylist。

状态：待决定。原生 Apple Smart Playlist 仍由独立编辑器创建。

### DEC-PL-004：Smart Playlist 非实时规则

- A（推荐）：编辑器只允许目标设备确认支持的 Live Updating 组合；可保存但需要重新连接刷新的规则必须明确标记 Refresh on next connection，并由用户主动刷新。
- B：只要数据库格式能写就全部允许，并一律显示 Live。
- C：完全禁止非实时规则。

状态：待决定。

### DEC-PL-005：删除语义

- A（推荐）：`Remove from Playlist` 只移除引用；`Delete from iPod` 显示受影响 playlist 和文件大小并确认，成功提交 DB 后才删文件。
- B：Delete 键总是从设备彻底删除。
- C：组件只允许从 playlist 移除，不允许删设备曲目。

状态：待决定。

### DEC-PL-006：删除后空 playlist

- A（推荐）：保留用户创建的空普通/Smart Playlist；只有用户明确删除容器才移除。
- B：自动删除空 playlist。

状态：待决定。

## 7. 第四轮：备份、恢复与设备生命周期

### DEC-SAFE-003：第一次完整备份位置

- A（推荐）：用户选择 iPod 之外的目录；FooPodBridge记录备份清单、数据库指纹和验证结果，但不把备份提交 Git。
- B：只备份到同一 iPod 隐藏目录。
- C：不验证完整备份，只保存当前 iTunesDB。

状态：待决定。B/C 无法充分覆盖文件系统或设备故障。

### DEC-SAFE-004：每次事务数据库备份保留

- A（推荐）：按物理设备保存最近 10 个验证通过的数据库快照，并永不自动删除最后一个 Last Known Good；Preferences 可查看占用和手动清理。
- B：只保留上一次。
- C：永久保留全部。

状态：待决定。

### DEC-SAFE-005：FooPodBridge 临时/孤立文件清理

- A（推荐）：自动清理有有效操作记录、明确由本项目创建且不被 DB 引用的 `.tmp`；未知孤立音频只报告并让用户选择。
- B：自动删除所有 DB 未引用文件。
- C：从不清理，只报告路径。

状态：待决定。

### DEC-SAFE-006：取消后的成功项目

- A（推荐）：提交前取消则不更新 DB，成功复制文件作为本项目 orphan 清理；提交阶段进入安全边界后完成或恢复，不产生“半个 playlist”。
- B：取消时把已复制曲目提交到 DB。

状态：待决定。

### DEC-SAFE-007：多设备并发

- A（推荐）：可以同时显示多台设备，但全进程一次只允许一个写事务；第一版避免两个 USB/FAT 事务和两套进度同时竞争。
- B：每台设备各自并行写入。
- C：只显示第一台设备。

状态：待决定。

### DEC-SAFE-008：完成后弹出

- A（推荐）：永不自动弹出；显示完成状态和明确 Eject 按钮。
- B：每个成功批次自动弹出。
- C：Preferences 可设置自动弹出。

状态：待决定。

## 8. 第五轮：UI、诊断、许可证与发布

### DEC-UI-003：FooCrate Devices 信息结构

初步方向是 `Playlists` 下方增加 `Devices`，设备下显示 Music、Audiobooks 和 Playlists；选择后进入独立 Device Workspace。中央/右栏的最终分工、传输队列位置、容量条和 Smart Playlist 编辑入口在 FooCrate UI 规格任务用真实布局共同决定。

状态：明确延后到 UI 任务；不阻断 Core，UI 任务不得在未核对前实现。

### DEC-UI-004：界面语言

- A（推荐）：与当前 FooCrate 一致，首版可见文案使用英文；规格和错误逻辑用中文记录。
- B：首版中文。
- C：首版即中英本地化。

状态：待决定。

### DEC-UI-005：独立 Columns UI 功能范围

- A（推荐）：功能与 FooCrate 等价，布局与视觉更通用；所有危险操作和 Smart Playlist 编辑均可达。
- B：只读浏览，写入必须去 FooCrate。
- C：只提供设备树，打开独立管理窗口执行其余操作。

状态：待决定。

### DEC-UI-006：Default UI 功能范围

- A（推荐）：设备概览、Library/playlist 浏览、导入、删除、进度和 Eject；复杂 Smart Playlist 编辑通过统一管理对话框打开。
- B：与 FooCrate 完全同布局。
- C：只读设备信息。

状态：待决定。

### DEC-DIAG-001：诊断日志与导出

- A（推荐）：本地有界日志默认遮蔽序列号和用户路径；用户主动导出脱敏诊断包，预览后再分享。
- B：只在 Debug 构建记录日志。
- C：Release 记录完整路径和设备信息。

状态：待决定。C 不符合隐私规则。

### DEC-LIC-001：公开许可证

- A（推荐）：完成文件级审计后采用与实际复用代码兼容的 LGPL 许可证；若采用 LGPLv3 参考实现，优先 LGPL-3.0-or-later。
- B：完全不采用参考源码，只依据格式知识重写，再评估更宽松许可证。
- C：暂不公开源码并保留全部权利。

状态：待决定。阻断公开 GitHub 发布和第三方源码进入正式工程，不阻断原创规格文档。

### DEC-PKG-002：候选发布节奏

- A（推荐）：Core 自动测试持续运行；只有形成可人工验证的永久能力时才同时输出新的 FooPodBridge/FooCrate prerelease 包。
- B：每个内部提交都生成并交给用户安装。
- C：直到全部功能结束才给任何候选。

状态：待决定。

## 9. 路线批准

完成上述决定后，用户还需核对 [`tasks/TODO.md`](../tasks/TODO.md) 中每个永久任务的目标、依赖和实机检查点。任务 000 只有在以下条件全部满足时才能标记“已验收”：

- 所有阻断当前路线的“待决定”项目变为“已批准”或“明确排除”；
- 两台实机证据采集任务的位置与隐私边界明确；
- 用户批准完整任务路线；
- 产品规格、架构、安全模型和任务索引没有冲突。
