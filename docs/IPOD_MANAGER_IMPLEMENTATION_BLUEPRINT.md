# FooPodBridge 的 iPod manager 中文实现蓝图

- 状态：跨任务长期蓝图；任务级细节仍在对应任务冻结
- 建立日期：2026-09-01；2026-09-08 由 Nano 4 实机证据修订设备范围
- 参考基线：`reupen/ipod_manager` commit `08e0657b5ee09bd05cdb60273e1a139205d4d3f6`
- 来源与许可证边界：[`REFERENCE_PROVENANCE.md`](REFERENCE_PROVENANCE.md)
- 正式架构：[`ARCHITECTURE.md`](ARCHITECTURE.md)
- 写入安全：[`SAFETY_MODEL.md`](SAFETY_MODEL.md)

## 1. 这份蓝图解决什么问题

这份文档把旧 `foo_dop / iPod manager` 分散在许多 C++ 文件里的历史经验，转换成 FooPodBridge 自己的中文目标方案。以后讨论和实现时，默认先看这里：

1. 先找到对应的目标状态、判断、动作和失败结果；
2. 需要证明“为什么”时，再沿引用查看固定版本的 `Ref\ipod_manager`、libgpod、Apple 官方说明或设备 fixture；
3. 需要写代码时，以本项目架构和安全模型重新实现，不逐行翻译旧源码；
4. 新证据先更新本蓝图和对应任务，再改变实现，避免每次聊天重新总结整个旧项目。

这不是旧源码的中文逐行注释，也不是一次性支持 23 个 iPod 家族的承诺。它按数据库家族描述共同实现，并用验证等级区分“参考支持”和“FooPodBridge 已在实机验证”，让项目不必为了每个旧型号都购买硬件。

## 2. 你先只需要理解这五句话

1. FooPodBridge 管理的不是“USB 上有个 Apple 设备”，而是“Windows 已挂载、可安全访问、身份和数据库能力都已确认的 iPod 存储卷”。
2. iPod 能播放歌曲，不是因为音频文件被复制进磁盘，而是因为音频文件与 `iTunesDB` 中的曲目、路径和 playlist 引用保持一致。
3. 没有 `iTunesDB` 不一定是坏设备；对已确认支持且卷结构健康的设备，它可能是一个需要显式初始化的空 Library。
4. Photo 等早期型号使用未签名传统 `iTunesDB`；Classic 与 Nano 3/4 使用带 6G 记录差异和 hash58 的传统 `iTunesDB`；Shuffle、Nano 5+ 和 iOS 另有额外数据库路径。
5. 历史 iPod manager 告诉我们“哪些思路曾经可行”，FooPodBridge 仍要用 fixture、故障测试和受控实机验收证明自己的实现。

如果只想跟进项目，读到这里后直接看第 5 节总流程和第 13 节任务映射即可。其余章节按问题查阅。

## 3. 怎样使用历史参考

### BP-REF-001：证据标签

| 标签 | 含义 | 能证明什么 | 不能单独证明什么 |
| --- | --- | --- | --- |
| `APPLE` | Apple 官方用户说明或型号资料 | 用户可见概念、官方型号和恢复/磁盘使用语义 | 私有数据库格式和第三方写入安全 |
| `IM` | 固定 commit 的 iPod manager 源码/更新记录 | 历史程序怎样识别、解析、生成和路由 | 我们的 x64 实现正确、所有旧型号都可靠 |
| `LG` | 固定 commit 的 libgpod | 独立格式说明、设备表和 hash58 来源 | Windows UI、我们的事务行为 |
| `FIX` | 脱敏或 Git 忽略的设备私有 fixture | 真实数据库结构和往返差异 | iPod 固件一定接受新写数据库 |
| `TEST` | 自动测试和故障注入 | 边界、损坏拒绝、回滚和重复行为 | 真实固件显示和播放结果 |
| `DEV` | 经授权的实机检查 | 固件接受、重启后 Library、播放和设备特性 | 未测试型号也同样支持 |

### BP-REF-002：采用等级

| 等级 | FooPodBridge 的做法 |
| --- | --- |
| 采用目标行为 | 保留旧程序已经证明有意义的输入、判断和结果语义 |
| 原创实现 | 用本项目模型、现代 C++、独立 Reader/Writer 和错误类型重新表达 |
| 安全替换 | 旧程序有直接删除、边写边改或单份备份时，改用 `SAFETY_MODEL.md` 的事务 |
| 仅作反例 | iOS、旧 UI、自动 Sync、dummy gapless 等只用于说明不能继承什么 |
| 允许源码移植 | 当前仅限已审计的 libgpod BSD-3-Clause hash58 文件，并保留声明 |
| 禁止 | `iTunesCrypt.dll/.lib`、`iPhoneCalc`、hash72/CBK、旧 SDK 和 Apple Mobile Device 路径 |

具体文件许可证和采用方式以 [`REFERENCE_PROVENANCE.md`](REFERENCE_PROVENANCE.md) 为准。如果未来想复制尚未批准的第三方代码，必须先重开任务 001，不能只改这份蓝图。

## 4. 设备状态不是“识别/不识别”两个结果

### BP-DEV-001：统一状态分类

| 状态 | 发现的事实 | FooPodBridge 结果 |
| --- | --- | --- |
| `NotMounted` | Windows 看见 USB 设备，但没有可访问卷 | 不读取、不写入；说明需要先让 Windows/Apple 工具恢复或暴露存储卷 |
| `UnsupportedFileSystem` | 有卷，但 Windows 不能可靠读写，例如 MacPod HFS/HFS+ | 显示 Unsupported，不建议 Windows 格式化 |
| `UnidentifiedVolume` | 有卷，但不能证明它是受支持的用户 iPod | 最多显示只读诊断；服务层拒绝写入 |
| `Initializable` | 已确认是具有格式 profile 和活动实验/验证任务的设备，卷健康，但 `iPod_Control` 或主数据库不存在 | 显示“可初始化”，连接本身不创建任何文件 |
| `ReadyReadOnly` | 数据库有效，但设备尚未取得写入验收或当前能力不足 | 发布只读 Library 快照，拒绝写命令 |
| `ReadyWritable` | 身份、格式、能力、备份和实机门槛全部满足 | 允许生成 Operation Plan；仍不自动写入 |
| `DatabaseCorrupt` | 数据库存在但截断、越界、签名或引用非法 | 不把它当空设备覆盖；进入诊断/恢复路径 |
| `RecoveryRequired` | 发现 FooPodBridge 有记录的未完成提交 | 按事务记录恢复，恢复前拒绝普通写入 |
| `RestoreRequired` | 分区、固件或基础卷不健康 | 超出 FooPodBridge 首版范围，交给 Apple Restore/专门恢复工具 |

### BP-DEV-002：存储卷是硬门槛

iPod manager 的 Windows 路径先按 Apple USB ID 识别磁盘设备，再把磁盘关系映射到 Windows volume；最后只接受已挂载的 removable drive。没有 volume 时，它也没有后续数据库路径。

FooPodBridge 采用同一事实边界，但使用现代 Windows 设备/卷 API 原创实现。它不负责打开 iTunes 的“Enable disk use”，不向未知设备发送私有控制命令，也不把充电状态冒充可管理状态。

用户概念参考：

- `APPLE`：[Set up iPod as a hard disk in iTunes on PC](https://support.apple.com/en-ae/guide/itunes/itns3114/windows)：启用后可在 Windows 看到磁盘；若复选框不可用，设备可能已经可作硬盘使用。
- `IM`：[`foo_dop/ipod_manager.cpp`](../../Ref/ipod_manager/foo_dop/ipod_manager.cpp) 第 216–300 行识别 USB/FireWire 型号，第 420–575 行建立 volume 关系，第 596–690 行扫描盘符。

### BP-DEV-003：型号名称只是证据之一

正式写入能力由稳定设备身份、文件系统、数据库版本、签名类型、固件/属性证据和实机验证共同决定。营销名称只用于显示和辅助判断。

- `APPLE`：[Identify your iPod model](https://support.apple.com/en-ie/103823) 用于核对 A 型号、代际、容量和导航方式。
- `IM`：[`foo_dop/device_info.cpp`](../../Ref/ipod_manager/foo_dop/device_info.cpp) 第 362–363 行列出 23 个非 touch 代际家族，第 520–1013 行按设备属性和产品代码细分。
- `LG`：libgpod 的设备表只作为第二来源；没有本项目 `DEV` 证据时只能获得参考/fixture 等级。受控实验写入还必须由活动设备任务点名并满足安全门槛。

## 5. 从插入设备到完成操作的总流程

### BP-FLOW-001：只读连接

```text
Windows volume 到达
→ 建立本次挂载代次
→ 只读取得卷、文件系统和容量
→ 识别设备属性与数据库家族
→ 分类为 Unsupported / Initializable / Ready / Corrupt / RecoveryRequired
→ 有有效数据库时由 Reader 解析并由 Validator 检查
→ 生成不可变 DeviceSnapshot
→ 服务发布快照，UI 只显示结果
```

连接、浏览和发布快照不创建目录、不修复数据库、不改设备名称，也不生成“顺手试一下”的数据库。

### BP-FLOW-002：所有写操作的共同入口

```text
用户明确发起高层意图
→ 核对设备身份、挂载代次和当前数据库指纹
→ media/database 把意图转换成候选模型
→ transaction 生成 OperationPlan
→ 显示影响、空间、冲突、警告和恢复条件
→ 用户确认或执行无争议计划
→ 在电脑侧生成并验证全部数据库产物
→ 按安全模型复制/备份/暂存/读回/提交
→ 从正式设备路径重新读取
→ 发布新快照和最终结果
```

历史 iPod manager 的操作流程是行为参考；提交顺序必须由 FooPodBridge 的 transaction 服务统一替换。

### BP-FLOW-003：外部状态变化

只要设备身份、挂载代次、盘符映射、数据库指纹或能力版本在计划后改变，旧计划立即失效。物理移除、系统弹出、短写或 Flush 失败进入明确的失败/恢复状态，不用旧盘符重试。

## 6. 数据库共同核心

### BP-DB-001：Reader

Reader 的输入是有长度边界的字节序列，不是任意裸指针。它需要：

1. 识别 `iTunesCDB` 或 `iTunesDB`；
2. 校验每个 header/section/record 的声明长度和父级范围；
3. 建立 track、master Library、普通 playlist、Smart Playlist 和引用模型；
4. 保留尚不理解但必须原样生存的记录和尾部数据；
5. 对截断、重叠、溢出、重复稳定 ID 和悬空引用返回具体错误。

历史参考：`IM` [`foo_dop/reader.cpp`](../../Ref/ipod_manager/foo_dop/reader.cpp) 第 131–379 行展示 CDB/DB 选择和传统数据库读取；[`foo_dop/itunesdb_playlist.cpp`](../../Ref/ipod_manager/foo_dop/itunesdb_playlist.cpp) 展示 playlist/Smart Playlist 记录。我们采用格式知识，不复制其无界类型和异常结构。

### BP-DB-002：稳定模型

内存模型必须把“已理解的业务字段”和“需要保留的原始数据”分开。UI 只能接触领域对象，不能持有 `mh*` 原始记录或数据库偏移。track ID、persistent ID、playlist ID 和引用关系由 database 模块统一分配和验证。

### BP-DB-003：Writer

Writer 只接受已经通过能力检查的模型，并执行确定性序列化：

1. 按目标格式配置选择记录版本、必需 section 和字段；
2. 为新增对象分配不冲突的稳定 ID；
3. 重新计算记录长度、计数、引用和必要签名；
4. 保留未修改未知数据；
5. 输出到内存或电脑临时目录，不直接打开真实设备正式文件。

历史参考：`IM` [`foo_dop/writer.cpp`](../../Ref/ipod_manager/foo_dop/writer.cpp) 第 60–151 行展示数据库家族路由；[`foo_dop/writer_itunesdb.cpp`](../../Ref/ipod_manager/foo_dop/writer_itunesdb.cpp) 展示传统数据库生成。设备提交部分不采用。

### BP-DB-004：Validator 与比较器

Writer 输出必须由独立 Reader 再读一次。Validator 检查结构、计数、唯一 ID、master Library、playlist 引用、设备能力和签名。比较器至少能区分：

- 允许变化：时间戳、随机/新分配 ID、用户请求修改的对象、重新计算的长度和签名；
- 必须保持：未修改曲目语义、playlist 顺序、未知字段/记录、artwork 引用；
- 必须拒绝：输入截断、越界、悬空引用、错误签名、设备格式不允许的记录。

历史程序的成功读取不能替代这些检查；`FIX + TEST` 是任务 003/004 的主要证据。

## 7. 空 Library 与首次初始化

### BP-INIT-001：先区分“空”与“坏”

只有以下条件同时成立，设备才进入 `Initializable`：

1. Windows 已挂载健康、受支持的文件系统卷；
2. 设备被正向识别为活动设备任务点名的物理实验机或验证机；
3. 已取得足以选择数据库格式和签名方式的能力证据；
4. 主数据库文件确实不存在，而不是存在但打不开、截断或签名错误；
5. 没有未完成事务或需要恢复的旧数据库证据。

数据库存在但损坏时禁止用新空库覆盖。

### BP-INIT-002：历史方案

iPod manager 的 `preparer_t` 会创建数据库目录；当 `iTunesCDB` 和 `iTunesDB` 都不存在时，生成 database ID、master Library playlist ID、另一个数据库随机值、Library 名称、编码和格式。首次 Send Files 随后在同一操作中添加曲目并写出数据库。

- `IM`：[`foo_dop/prepare.cpp`](../../Ref/ipod_manager/foo_dop/prepare.cpp) 第 12–57 行。
- `IM`：[`foo_dop/send_files.cpp`](../../Ref/ipod_manager/foo_dop/send_files.cpp) 第 5–56 行。
- `IM` 更新记录：[`CHANGELOG.md`](../../Ref/ipod_manager/CHANGELOG.md) 的 0.6.5.7 记录了“blank Nano 5G”写库修复，证明空库是历史真实场景，但 Nano 5 的 SQLite/hash72 路径不进入本项目。

### BP-INIT-003：FooPodBridge 目标方案

FooPodBridge 保留“缺库时建立空模型”的思路，改成显式、安全的四段能力：

1. **任务 003/004，电脑侧数据库能力**：给定格式配置，可以在临时目录生成零曲目、仅含合法 master Library 的传统或 6G/hash58 数据库并往返验证。
2. **任务 005，只读设备分类**：识别 `Initializable`，发布原因和所需能力；不创建目录。
3. **任务 007，初始化事务**：提供 `InitializeLibrary` Operation Plan，生成恢复记录、电脑侧产物、设备暂存和读回验证。
4. **任务 008/009，实机接受**：先在有外部备份、明确授权的 Nano 4 实验机建立完整纵向能力；Photo/Classic 等其他型号按可用硬件和验证等级补充，不阻断共同能力实现。

首次导入可以包含初始化，但“仅仅插入设备”永远不会初始化。没有存储卷或需要 Apple Restore 的设备不进入此流程。

### BP-INIT-004：不拿 Windows 格式化冒充新机

Windows 格式化卷不等于 Apple Restore，也不能可靠模拟出厂状态。恢复会擦除信息并重新安装设备软件；FooPodBridge 首版不实现分区或固件恢复。

- `APPLE`：[Restore your iPhone, iPad, or iPod to factory settings using a computer](https://support.apple.com/en-us/118107)。该页面说明 Restore 是擦除并安装设备软件；具体旧 click-wheel 型号的可用工具仍需在实机任务中核对。

## 8. Photo、Classic 和其他数据库家族

### BP-FMT-001：格式配置，而不是巨大型号 switch

共同 Reader/Model/Writer 使用显式 `DatabaseFormatProfile`，至少描述：

- 记录版本、header 大小和必需 section；
- 字段存在条件和默认值；
- 未知数据保留策略；
- 是否压缩、是否需要签名及签名输入；
- playlist、Smart Playlist、artwork 和媒体能力；
- 空库最小合法结构。

设备能力配置选择格式配置；营销名称不能直接调用某个 Writer。

### BP-FMT-002：Photo

Photo 先建立传统 `iTunesDB` 的共同往返核心。任务 003 只在电脑 fixture/临时目录完成读取、无修改往返、损坏拒绝、虚拟增删和空库生成，不连接实机。

### BP-FMT-003：6G/hash58 签名传统数据库

Classic 与 Nano 3/4 在共同核心上增加 6G 记录差异和 hash58。历史 iPod manager 的 `is_6g_format()` 明确包含 Classic、Nano 3G 和 Nano 4G，并在数据库 header 上根据 FireWire GUID 生成 hash58。libgpod 的设备矩阵也把三者列为需要 SysInfoExtended 与 hash58、但不需要 hash72/SQLite 的型号。

2026-09-08 的用户自有 Nano 4 私有 fixture 提供了第一份本项目实机输入：未压缩 `iTunesDB`，`mhbd` header 244 字节、版本字段 49、五个顶层 dataset、hash58 区非零。header 的另一签名保留区也非零，但不能只按偏移把它定性成 Nano 5 式 hash72；任务 004 必须分别验证“固件实际要求”和“历史 writer 额外生成”的字段，禁止因为参考代码写过 CBK 就引入未获许可的 DLL/算法。

- `IM`：[`foo_dop/ipod_manager.h`](../../Ref/ipod_manager/foo_dop/ipod_manager.h) 第 442–450 行的格式判断。
- `IM`：[`foo_dop/writer_itunesdb.cpp`](../../Ref/ipod_manager/foo_dop/writer_itunesdb.cpp) 第 1819–1889 行的历史签名路径，仅参考行为；其中 `iTunesCrypt` 调用禁止采用。
- `LG`：[`libgpod/src/itdb_hash58.c`](https://github.com/gtkpod/libgpod/blob/7982c5554f78dde47fd006afbeff659201d6db3d/src/itdb_hash58.c) 是允许修改采用的 BSD-3-Clause 算法来源。
- `LG`：[`libgpod/README.overview`](https://github.com/fadingred/libgpod/blob/master/README.overview) 的设备矩阵把 Classic、Nano 3G、Nano 4G 列为 hash58-only，把 Nano 5G 单独列为 hash58 + hash72 + SQLite + iTunesCDB。
- `DEV`：[`../docs/device-evidence/NANO4_20260908_BASELINE.md`](device-evidence/NANO4_20260908_BASELINE.md) 记录纯净 Windows 挂载、私有 fixture 和验证备份；完整标识与数据库内容不进入 Git。

### BP-FMT-004：Shuffle、Nano 5+ 与 iOS 路径

Shuffle 不是“没有屏幕的普通 Classic”。历史代码除主数据库外还按能力写 `iTunesSD` 或 ShadowDB：[`foo_dop/writer_itunessd.cpp`](../../Ref/ipod_manager/foo_dop/writer_itunessd.cpp) 第 11–164、166–449 行。Shuffle 只有在新任务取得合法证据、硬件和独立格式配置后才能加入。

历史 SQLite writer 位于 [`foo_dop/writer_sqlite.cpp`](../../Ref/ipod_manager/foo_dop/writer_sqlite.cpp)，涉及本项目禁止的 Nano 5 hash72/CBK 与旧 iOS 路径，只作为范围反例，不作为当前待办实现。Nano 4 不属于这条 SQLite 路线。

## 9. 音乐导入与元数据

### BP-IMP-001：历史有效思路

iPod manager 的 Send Files 大体执行：扫描设备、准备/读取数据库、刷新缓存、检查重复、复制文件、更新模型/Smart Playlist、写数据库。入口见 [`foo_dop/send_files.cpp`](../../Ref/ipod_manager/foo_dop/send_files.cpp)；文件处理见 [`foo_dop/file_adder.cpp`](../../Ref/ipod_manager/foo_dop/file_adder.cpp) 第 160 行起。

FooPodBridge 采用“先建立完整批次模型、最后写一次数据库”，但不继承旧程序的同步、自动转码、直接设备修改或取消后补写数据库行为。

### BP-IMP-002：FooPodBridge 批次

```text
规范化源曲目
→ 检查 codec/container/profile/FAT32/空间
→ 检查重复和用户目标 Music/Audiobook/playlist
→ 计算元数据、SoundCheck、Classic gapless
→ 形成一次 OperationPlan
→ 音频每首经 USB 写一次，数据库尚不引用
→ 全批只生成、验证和提交一次数据库
```

字段规则、重复策略、Compilation、Rating 和失败语义以 [`PRODUCT_SPEC.md`](../specs/PRODUCT_SPEC.md) 为准；旧映射只用于格式证据。

### BP-IMP-003：SoundCheck

历史公式位置：`IM` [`foo_dop/itunesdb_track.cpp`](../../Ref/ipod_manager/foo_dop/itunesdb_track.cpp) 第 337–357 行。libgpod 是第二来源。FooPodBridge 原创实现并建立数值向量；缺少 ReplayGain 时不写虚假默认值。

### BP-IMP-004：Gapless

历史入口和格式线索：

- `IM` [`foo_dop/gapless_scanner.cpp`](../../Ref/ipod_manager/foo_dop/gapless_scanner.cpp) 第 17 行起；
- `IM` [`foo_dop/mp3.cpp`](../../Ref/ipod_manager/foo_dop/mp3.cpp) 的 MPEG frame/resync 查找；
- `IM` [`foo_dop/mp4.cpp`](../../Ref/ipod_manager/foo_dop/mp4.cpp) 第 238、631、1091 行附近的 AAC/MP4 信息。

FooPodBridge 不复制旧解析器，也不继承 dummy gapless。扫描失败只警告单曲；Classic 只有准确数据才写准确标志。

## 10. 删除、playlist、artwork 与 Audiobook

### BP-DEL-001：删除必须安全替换历史顺序

历史 `file_remover.cpp` 在部分路径中先删除设备音频，再从内存模型移除引用，之后上层才写数据库：[`foo_dop/file_remover.cpp`](../../Ref/ipod_manager/foo_dop/file_remover.cpp) 第 6–93、96–175 行；调用链见 [`foo_dop/remove_files.h`](../../Ref/ipod_manager/foo_dop/remove_files.h)。这个顺序只作反例。

FooPodBridge 固定为：先提交不再引用曲目的数据库，再删除无引用音频。数据库失败不删音频；文件删除失败只留下可报告的 orphan。

### BP-PL-001：普通 playlist

普通 playlist 是设备 track ID 的有序引用。新增成员先复用 Library 已有 track ID；缺失曲目与导入组成一个 Operation Plan；重复成员默认 Skip。历史结构证据见 [`foo_dop/itunesdb_playlist.cpp`](../../Ref/ipod_manager/foo_dop/itunesdb_playlist.cpp) 和传统 writer 的 playlist section。旧 UI 行为不采用。

### BP-SPL-001：Smart Playlist

Smart Playlist 需要同时保存规则、限制/排序信息和设备所需初始成员：

- 规则读写：`IM` [`foo_dop/itunesdb_playlist.cpp`](../../Ref/ipod_manager/foo_dop/itunesdb_playlist.cpp) 第 211–398 行；
- 历史字段/运算符：`IM` [`foo_dop/smart_playlist_editor.cpp`](../../Ref/ipod_manager/foo_dop/smart_playlist_editor.cpp) 第 95 行起；
- 历史成员计算：`IM` [`foo_dop/smart_playlist_processor.cpp`](../../Ref/ipod_manager/foo_dop/smart_playlist_processor.cpp) 第 115、211、306、398 行起。

FooPodBridge 原创规则模型和编辑器。只有目标型号实机确认的字段、运算符和 Live 语义才显示为支持；同数据库家族的其他型号不能自动继承，foobar autoplaylist 不转换。

### BP-ART-001：Artwork

历史 `photodb.cpp` 同时包含 ArtworkDB 记录、缩略图文件和共享引用思路：[`foo_dop/photodb.cpp`](../../Ref/ipod_manager/foo_dop/photodb.cpp) 第 277、389、744、827、974、1106 行起。FooPodBridge 只处理 Music/Audiobook artwork，使用设备能力格式、独立图像实现和引用计数；不复制旧 GDI+ 架构，也不实现 Photo Library 同步。

### BP-AUD-001：Audiobook

历史元数据线索位于 [`foo_dop/itunesdb_track.cpp`](../../Ref/ipod_manager/foo_dop/itunesdb_track.cpp) 第 261–329 行，MP4 章节位于 [`foo_dop/mp4.cpp`](../../Ref/ipod_manager/foo_dop/mp4.cpp) 第 879 行起。FooPodBridge 只有在用户明确选择 Audiobooks 时设置 media kind、remember position、skip when shuffling 和受支持章节；不按扩展名或 Genre 偷猜。

## 11. 事务：哪些历史思路保留，哪些必须换掉

### BP-TXN-001：可以保留的思路

- 正式文件之外先写临时文件；
- 旧数据库保留一个备份名称；
- 写完数据库后更新缓存和设备视图；
- 一次用户操作只写一次主数据库。

历史证据：[`foo_dop/writer_itunesdb.cpp`](../../Ref/ipod_manager/foo_dop/writer_itunesdb.cpp) 第 46、1895–1909 行使用 `.dop.temp` 和 `.dop.backup`。

### BP-TXN-002：必须由本项目替换的部分

- 不允许功能模块直接删除/移动真实设备文件；
- 不允许取消后为了“保持当前状态”无条件补写数据库；
- 不用单个 `.backup` 冒充可恢复事务；
- 不假设 FAT32 多文件重命名原子；
- 不让 UI 或旧同步对象决定恢复策略。

替代方案完全以 [`SAFETY_MODEL.md`](SAFETY_MODEL.md) 为准：不可变计划、稳定身份/代次、电脑侧生成验证、Last Known Good、设备暂存、Flush、完整数据库读回、恢复记录和逐阶段故障注入。

## 12. 兼容范围怎样表达

### BP-SUP-001：四个验证等级

1. `StructureKnown`：历史来源说明结构存在；
2. `FixtureRoundTrip`：本项目 fixture 读取/修改/往返通过；
3. `DeviceReadVerified`：指定实机只读结果正确；
4. `DeviceWriteVerified`：指定实机在备份与恢复门槛下接受写入并重启验证。

只有第四级可以宣传“已验证可写”。iPod manager 识别过某个型号，只能提高 `StructureKnown` 的可信度，不能自动把 FooPodBridge 提升到第四级。活动任务可以在前三级设备上批准一次受控实验写入，但 UI 和文档必须继续显示 Experimental，直到重启播放和恢复验收完成。

### BP-SUP-002：当前范围

- 传统未签名家族：任务 003 建立共同核心；Photo、Mini、早期 click-wheel、Nano 1/2 按参考与后续 fixture 分级；
- 6G/hash58 家族：任务 004 建立签名 profile；Nano 4 是当前点名实验机，Classic 与 Nano 3 使用相同家族但保持各自验证等级；
- 其他已识别 click-wheel：有固定参考和匹配 profile 时可显示 Experimental/ReadOnly，没有足够证据时 Unsupported；
- Shuffle：独立数据库家族，未来另建任务；
- Nano 5、iPod touch、iPhone、iPad：当前明确排除。

## 13. 永久任务映射

| 任务 | 从历史参考吸收的思路 | FooPodBridge 必须补上的永久能力 | 主要蓝图编号 |
| --- | --- | --- | --- |
| 003 传统数据库共同核心 | 传统 Reader/Writer、playlist 结构、空库准备 | 有界解析、未知数据保留、Validator、比较器、未签名 profile；Nano 4 fixture 同时提供共同记录输入 | `BP-DB-*`、`BP-INIT-003`、`BP-FMT-002/003` |
| 004 6G/hash58 与 Nano 4 | 6G 格式差异、FireWire GUID/hash58 输入 | BSD hash58 移植、已知向量、Nano 4 fixture 往返、空库和签名验证 | `BP-FMT-003`、`BP-INIT-003` |
| 005 设备发现/只读 | USB/volume 映射、设备属性、DB 家族路由 | 现代 Windows API、稳定身份、挂载代次、完整状态分类 | `BP-DEV-*`、`BP-FLOW-001` |
| 006 FooCrate 只读 UI | 只参考可观察信息 | 真实服务快照、无服务隐藏、无旧 panel 代码 | `BP-FLOW-001`、`BP-SUP-*` |
| 007 事务/恢复 | temp + backup 的基本动机 | 完整安全状态机、故障注入、初始化事务 | `BP-TXN-*`、`BP-INIT-003` |
| 008 Nano 4 实验导入 | Send Files 的批次骨架、字段线索 | Operation Plan、一次 USB 写入、Nano 4 已有库加一首/删除/重启/恢复验收 | `BP-IMP-*`、`BP-FLOW-002` |
| 009 家族扩展验收 | Photo/Classic writer、gapless 线索 | 可用实机按等级补充；Classic 加准确 gapless，未到手型号不阻断 Nano 4 主线 | `BP-FMT-002/003`、`BP-IMP-004` |
| 010 删除 | 引用清理线索 | DB-first 删除与 orphan 报告 | `BP-DEL-001` |
| 011 普通 playlist | playlist 数据关系 | 统一事务、复用 track ID、明确三种删除语义 | `BP-PL-001` |
| 012 Artwork | ArtworkDB/ithmb/共享引用线索 | 原创图像实现、能力格式、引用安全 | `BP-ART-001` |
| 013 Audiobook | media kind、bookmark、chapter 字段 | 用户明确分类、FAT32 预检、设备能力 | `BP-AUD-001` |
| 014 Smart Playlist | 规则、编辑和成员计算线索 | 原创模型、能力拒绝、实机 Live 语义 | `BP-SPL-001` |
| 018 诊断/备份 | 历史属性和单备份仅作线索 | 脱敏诊断、Last Known Good、恢复入口 | `BP-TXN-*`、`BP-REF-001` |

每个任务开始时只需读取本表对应章节和引用，不要求重新通读整个 iPod manager。若任务发现蓝图错误，先记录新证据并更新本蓝图，再修改任务结论。

## 14. 给项目维护者的学习顺序

### 第一步：先理解“卷”和“Library”

阅读 `BP-DEV-001/002`、`BP-FLOW-001`。你应该能回答：为什么能充电不等于能管理；为什么出现盘符仍不等于允许写入。

### 第二步：理解音频文件为什么不能直接拖进去播放

阅读 `BP-DB-001/002`、`BP-PL-001`。重点只看 track、路径、master Library 和 playlist 引用，不需要先学二进制偏移。

### 第三步：理解 Reader/Writer/Validator

阅读 `BP-DB-003/004`。重点是“读取”和“重新生成”为什么必须由独立验证器夹住，以及 fixture 能证明什么。

### 第四步：理解 Photo 与 Classic 的差别

阅读 `BP-FMT-002/003`。只需知道共同骨架与 hash58 的边界，不需要手算 SHA-1。

### 第五步：理解为什么安全事务比旧代码复杂

阅读 `BP-FLOW-002/003`、`BP-TXN-*`，再看 `SAFETY_MODEL.md`。重点是中途拔线时旧数据库、音频和恢复记录分别处于什么状态。

### 第六步：按兴趣学习功能

导入看 `BP-IMP-*`，删除看 `BP-DEL-001`，播放列表看 `BP-PL-001/BP-SPL-001`，封面看 `BP-ART-001`，有声书看 `BP-AUD-001`。这些可以独立提问，不需要按 C++ 文件顺序阅读。

## 15. 固定参考入口

- iPod manager 固定上游：[commit `08e0657`](https://github.com/reupen/ipod_manager/tree/08e0657b5ee09bd05cdb60273e1a139205d4d3f6)
- 本地只读参考：`D:\dev\foo\FooPodBridge\Ref\ipod_manager`
- libgpod 固定上游：[commit `7982c55`](https://github.com/gtkpod/libgpod/tree/7982c5554f78dde47fd006afbeff659201d6db3d)
- Apple 型号表：[Identify your iPod model](https://support.apple.com/en-ie/103823)
- Apple 磁盘使用说明：[Set up iPod as a hard disk in iTunes on PC](https://support.apple.com/en-ae/guide/itunes/itns3114/windows)
- Apple 恢复概念：[Restore your iPhone, iPad, or iPod to factory settings using a computer](https://support.apple.com/en-us/118107)
- 文件级采用与许可证：[`REFERENCE_PROVENANCE.md`](REFERENCE_PROVENANCE.md)

Apple 在线页面会更新；涉及旧型号的具体行为必须记录查询日期，并由固定的历史源码、fixture 或实机证据交叉验证。Ref 永久只读，任何结论变化都写入本项目文档，不能修改参考源码来“配合”结论。
