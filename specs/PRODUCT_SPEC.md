# FooPodBridge 产品总规格

- 状态：规格核对中
- 版本：0.1
- 日期：2026-08-26
- 产品目标：[`../docs/PRODUCT_GOAL.md`](../docs/PRODUCT_GOAL.md)
- 架构：[`../docs/ARCHITECTURE.md`](../docs/ARCHITECTURE.md)
- 安全模型：[`../docs/SAFETY_MODEL.md`](../docs/SAFETY_MODEL.md)
- 用户决策：[`../decisions/USER_DECISIONS.md`](../decisions/USER_DECISIONS.md)

本文件是 FooPodBridge 产品边界和用户可见行为的唯一总规格。详细模块在对应任务开始前建立独立规格，但不能与本文件和已批准决策冲突。

## 1. 产品定义

FooPodBridge 是 Windows x64 上的 foobar2000 2.x 组件，为磁盘模式 click-wheel iPod 提供读取、手动音乐导入、删除、播放列表、Smart Playlist、封面、SoundCheck、gapless、Audiobook 和安全弹出能力。

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
- FooPodBridge 不要求安装 iTunes、Apple Music 或 Apple Mobile Device Support 才能管理 Photo/Classic；
- 组件包不得包含 foobar2000、Apple DLL、x86 `iTunesCrypt.dll`、用户数据库或未经许可二进制。

## 3. 支持设备

### 正式可写

- 用户实机验证的 iPod Photo；
- 用户实机验证的 iPod Classic。

允许写入不是按产品名称字符串决定，而是按物理设备身份、固件、数据库类型、文件系统和能力矩阵共同决定。每台设备第一次写入前必须完成 `EVID-DEV-001/002`。

### 非正式设备

- 其他 click-wheel 型号可以被发现并显示 Unsupported；
- 没有实机与完整能力证据时不能切换为 Writable；
- 不支持 iPhone、iPod touch、Apple Mobile Device、Nano 5 hash72/CBK 或现代 iOS 数据库路径。

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
├── Music
├── Audiobooks
└── Playlists
    ├── Normal playlists
    └── Smart playlists
```

- Music/Audiobooks 是设备 Library 的不同媒体视图；
- playlist 是设备 track ID 引用，不是 foobar playlist；
- 所有设备曲目必须存在于设备数据库的 track list；
- UI 可以为设备路径创建 foobar metadb handles 用于播放和属性查看，但不改变设备 namespace 语义。

## 6. 手动导入

用户可以从 foobar 曲目选择、FooCrate Playlist View、普通 playlist 或 autoplaylist 当前结果主动发起导入。

导入必须先生成 Operation Plan，至少说明：

- 目标设备、Music/Audiobooks/playlist；
- 直接复制、拒绝或未来可能的转码结果；
- 预计空间与保留余量；
- 重复、冲突和缺失标签；
- SoundCheck、gapless、封面和 Audiobook 处理；
- 会创建或修改的 playlist；
- 警告和阻断原因。

未经用户批准，FLAC 当前只能明确拒绝，不自动转码。直接复制格式与 profile 由 `DEC-IMP-001` 冻结。

执行遵守安全模型：大文件只写一次、整批只构建/提交一次数据库、数据库完整读回验证。单曲与批次失败策略由 `DEC-IMP-005` 冻结。

## 7. 元数据

正式字段至少覆盖：

- Title、Artist、Album Artist、Album、Composer、Genre；
- Track/Disc number 与总数、Year、BPM；
- Sort Title/Artist/Album/Album Artist/Composer；
- 时长、codec、bitrate、sample rate、channels、文件大小；
- 日期、compilation 和必要设备媒体标志；
- SoundCheck、gapless、封面和 Audiobook 字段。

元数据来源、缺失标签和冲突行为由 `DEC-META-001/005` 冻结。FooPodBridge 默认不反写源音频标签。

## 8. SoundCheck

- 导入必须把可用 ReplayGain 转换为设备 SoundCheck/volume normalization 数据；
- 公式采用有来源记录和已知向量的实现；
- 缺少 ReplayGain 时是否扫描由 `DEC-META-002` 决定；
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
- 原始来源和多图策略由 `DEC-META-003` 冻结；
- 删除曲目后清理不再引用的 artwork 资源，不能影响共享封面。

## 11. Audiobooks

拖到 Audiobooks 的曲目必须在设备能力允许时设置：

- Media Kind = Audiobook；
- Remember Playback Position；
- Skip When Shuffling；
- 支持时保留章节。

目标 namespace 与自动推断的最终优先级由 `DEC-META-004` 冻结。单个文件超过 FAT32 限制时在计划阶段拒绝。

## 12. 普通播放列表

用户可以：

- 新建、重命名、删除和排序设备普通 playlist；
- 添加、移除和重排成员；
- 把 foobar 普通 playlist 当前内容手动发送到设备；
- 在一次操作中导入缺失曲目并建立引用；
- 明确区分删除 playlist 容器、从 playlist 移除和从设备删除。

名称冲突、替换/新建行为由 `DEC-PL-002` 冻结。不提供后台保持同步。

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

foobar autoplaylist 的手动发送行为由 `DEC-PL-003` 冻结；非实时规则行为由 `DEC-PL-004` 冻结。

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

## 15. 事务、恢复与弹出

- 同一设备不能并发写入；
- 写操作始终核对设备身份、挂载代次和数据库指纹；
- 旧数据库在新数据库完整验证前保持正式有效；
- FAT32 提交窗口使用恢复记录和 Last Known Good，不假装多文件原子；
- 取消只在安全边界生效并列出已发生结果；
- 事务中弹出必须等待或安全取消；
- 物理移除进入 Interrupted/Recovery Required，不能重试旧盘符。

备份、清理、多设备和弹出细节由 `DEC-SAFE-003` 至 `DEC-SAFE-008` 冻结。

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
- 视觉和功能可达范围分别由 `DEC-UI-005/006` 冻结。

## 18. Preferences 与诊断

Preferences 至少承载：

- 经批准的导入、SoundCheck、转码和重复策略；
- 数据库备份与清理状态；
- 受支持设备和服务版本；
- 诊断日志/脱敏导出；
- 恢复与 Last Known Good 管理；
- 恢复默认和版本迁移。

设置使用稳定 GUID、版本、合法值校验和迁移。不能靠修改隐藏配置绕过 Unsupported 写入。

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

两台设备分别完成：

1. 识别、读取 Library、普通 playlist、Smart Playlist、容量和 artwork；
2. 导入一批 MP3/AAC，并验证 SoundCheck；
3. Classic 使用连续专辑验证 gapless；
4. 导入 Audiobook 并验证续播/随机播放排除；
5. 新建、编辑、重排和删除普通 playlist；
6. 新建并验证至少一组多规则 Smart Playlist；
7. 从 playlist 移除和从设备删除，确认语义不同；
8. 安全弹出、设备重启后检查所有内容；
9. 再连接并确认数据库可读、空间和状态一致；
10. 执行至少一个可恢复的故障注入/中断演练。

FooCrate、独立 Columns UI 和 Default UI 分别完成其批准范围的日常流程。所有包、来源、许可证、隐私和测试实例边界通过审计。

## 22. 当前批准门槛

本规格只有在以下条件满足后才能从“规格核对中”改为“可实现”：

- 用户完成当前路线所需的 `USER_DECISIONS.md`；
- 用户批准 `tasks/TODO.md` 的永久任务拆分；
- 两台实机证据采集的位置、隐私和备份要求得到确认；
- 公开许可证或暂不公开的决定明确；
- 本规格与架构、安全模型、UI 方向不存在冲突。
