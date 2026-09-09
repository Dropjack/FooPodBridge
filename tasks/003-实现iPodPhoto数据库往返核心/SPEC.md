# 任务 003：传统 iTunesDB 共同核心实现规格

- 状态：已验收
- 日期：2026-09-09
- 任务入口：[`README.md`](README.md)
- 长期蓝图：[`../../docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md`](../../docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md) 的 `BP-DB-*`、`BP-INIT-001/003`、`BP-FMT-001/002/003`
- 来源边界：[`../../docs/REFERENCE_PROVENANCE.md`](../../docs/REFERENCE_PROVENANCE.md) 的 `FD-DB-01/02`、`FD-PL-01`、`LG-DB-01`

本文把任务 003 从方向性说明收敛为可以直接写测试和 C++ 的合同。除文末用户检查点外，不再为本任务增加新的产品选择；实现发现真实 fixture 与本文冲突时，先记录证据并回到规格核对，不能静默扩大写入能力。

## 1. 交付结果

任务 003 交付一个不依赖 foobar2000、Windows 设备 API、UI 或文件系统事务的 C++20 database Core：

```text
有界字节输入
→ Reader 建立结构树和最小领域模型
→ Validator 独立检查结构与引用
→ Edit 只产生显式变更
→ Writer 按 profile 确定性输出到内存
→ 独立 Reader + Validator 再读
→ Comparator 证明请求修改、允许变化和必须保持项
```

本任务完成后可以：

- 读取未压缩传统 `iTunesDB` 的共同结构；
- 在合成未签名 profile 上生成空 Library、添加/删除虚拟曲目和普通 playlist，并稳定往返；
- 对 Nano 4 的到货非空库与 Restore 后空库两份私有 fixture 读取共同记录、验证结构并在无修改时逐字节保留；
- 明确拒绝截断、越界、计数冲突、重复 ID、悬空引用、错误 profile 和超出任务能力的修改。

本任务完成后仍不能：

- 生成或修改 Nano 4/Classic/Nano 3 可接受的 hash58 数据库；
- 声称任何物理 Photo、Classic 或 Nano 已验证可写；
- 编辑 Smart Playlist 规则、artwork、SoundCheck、gapless、Audiobook 属性或设备能力；
- 打开设备路径、创建 `iPod_Control`、提交数据库或执行恢复；
- 读取 `iTunesCDB`、SQLite、hash72/CBK、Shuffle 数据库或 iOS 数据库。

## 2. 两种能力级别

任务 003 不把“能读共同结构”和“能安全重写某设备数据库”混为一谈。

### 2.1 `TraditionalUnsigned`

这是本任务唯一可修改、可重新序列化的 profile：

- 用于合成 fixture、未签名最小空 Library 和电脑临时目录测试；
- 支持共同 track、master Library、普通 playlist 及其有序成员引用；
- 没有签名输入、hash58、hash72、SQLite 或压缩容器；
- 只有 profile 明确声明的记录和字段可以由 Writer 新建；
- 不因此自动宣称某个真实 iPod 型号可写。

### 2.2 `TraditionalPreserveOnly`

这是 Nano 4 私有 fixture 在任务 003 中使用的能力：

- Reader 可以解析传统共同容器、曲目和 playlist 引用；
- 未理解的 6G header 区、签名区、dataset、record、字段尾部和文件尾部保留原始字节；
- 模型未发生任何变更时，Writer 返回原始输入字节，要求 SHA-256 与输入一致；
- 只要出现曲目、playlist、ID、顺序、字符串、计数或其他语义修改，Writer 返回 `ProfileNotWritable`；
- 任务 004 实现并验证 6G/hash58 后，才可以用可写 6G profile 替换该限制。

这个划分使 Nano 4 fixture 现在就能验证 Reader 和保留机制，同时不会因重新计算长度或时间戳而让现有签名失效。

## 3. 输入合同

Reader 只接受：

- 调用方提供的 `std::span<const std::byte>`；
- 显式的读取选项和资源限制；
- 可选的来源标签，只用于错误定位，不包含真实用户路径。

Reader 不接受路径、盘符、文件句柄或设备对象。读取文件、取消 I/O 和设备生命周期属于以后模块。

固定规则：

1. 单个数据库默认最大 512 MiB；超过限制返回 `ResourceLimitExceeded`，不能依赖分配失败处理。
2. 所有长度、偏移和计数先使用无符号 64 位检查，再转换为 `size_t`；任何加法、乘法或向下转换溢出立即失败。
3. record 必须至少容纳自己的共同 header；`header_size <= section_size`，且 section 必须完整落在父范围内。
4. count 必须能由父 section 剩余的最小 record 大小容纳；不能按不可信 count 预分配无限内存。
5. 任务 003 只接受未压缩传统容器。识别到 `iTunesCDB`、SQLite、压缩 payload、未知端序或未知容器版本时返回明确 Unsupported，不尝试猜测或降级。
6. 输入字节在 `DatabaseDocument` 生命周期内由文档拥有；领域对象不保留悬空 span。
7. Reader 不修改输入，不访问网络、注册表、环境变量或当前设备。

## 4. Reader 与结构树

Reader 先建立与原始顺序一致的结构树，再投影最小领域模型。结构树的每个节点至少保存：

- 四字节 marker；
- header/section 声明长度；
- 在输入中的原始范围；
- 已理解的共同字段；
- 未理解的 header 尾部、child record 和 payload 原始字节；
- 原始 child 顺序；
- `unchanged / changed / inserted / removed` 状态。

任务 003 必须识别并有界遍历：

- 数据库根；
- dataset 容器；
- track list 与 track record；
- playlist list、master Library、普通/Smart playlist 容器；
- playlist item 到 track ID 的有序引用；
- 字符串/数据子记录的共同 envelope。

以下内容在任务 003 默认作为 opaque 数据保留，不解释为可编辑业务能力：

- 6G/hash58 和其他签名保留区；
- secondary track/playlist dataset、album/artist index、Genius 和 special dataset；
- Smart Playlist 的规则、限制、排序和嵌套 payload；
- artwork、podcast、video、TV、Photo Library 及未知媒体扩展；
- 已知 record 中超出当前共同字段的尾部。

遇到未知但长度合法的 marker、dataset type 或子记录不是错误；Reader 保存其原始范围并产生 `UnknownPreserved` 诊断。未知结构只有在长度无法验证或与请求修改存在依赖时才阻断。

## 5. 最小稳定领域模型

领域模型不公开 `mh*` marker、原始偏移或裸字节所有权。任务 003 的稳定最小对象为：

### `DatabaseModel`

- 数据库持久 ID；
- 格式观察结果与能力级别；
- 一个且仅一个 master Library；
- 按原始顺序保存的 tracks 和 playlists；
- 只读 diagnostics；
- 与结构树关联但不暴露给 UI 的 preservation token。

### `Track`

- 非零 32 位 track ID；
- 非零 64 位 persistent ID（输入确实提供时）；
- 设备内相对媒体路径；
- Title、Artist、Album 三个基础字符串；
- 未在任务 003 开放修改的其余字段保留 token。

任务 003 的虚拟曲目测试只要求上述最小字段。SoundCheck、gapless、artwork、rating、media kind 和完整元数据将在拥有对应业务规则的后续任务扩展；本任务必须保留现有字节，但不提前定义错误语义。

### `Playlist`

- 非零 64 位 playlist persistent ID；
- 名称；
- `Master / Ordinary / Smart / Opaque` 类型；
- 普通 playlist 的有序 track ID 引用；
- Smart/Opaque playlist 的原始规则和扩展 preservation token。

只有 `Ordinary` 可以在任务 003 编辑成员。Master 成员必须与 Library track 集合一致并由 Writer 生成，不接受调用方手工制造悬空或重复引用。Smart Playlist 在任务 003 只读并逐字节保留，不能增删规则或手工成员。

### 文本规则

- 领域字符串使用 UTF-8；
- 传统 profile 的已知字符串 payload 按明确格式转换为 UTF-16，不使用当前系统代码页；
- 不做 Unicode normalization、大小写折叠或首尾空白修改；
- 无效 UTF-16、奇数字节长度、非法长度或不能无损表示的已知字符串返回 `InvalidText`；
- 未知字符串类型不尝试解码，只保留原始字节；
- 错误和比较报告默认不输出曲名、playlist 名、媒体路径或设备 ID。

## 6. 修改合同

所有修改通过显式 `EditPlan` 执行，不能取得可变容器后任意改写。任务 003 只提供：

- `AddTrack`：向 `TraditionalUnsigned` 添加一个虚拟 track；
- `RemoveTrack`：删除 track，并从 master 和普通 playlist 中移除其引用；
- `AddOrdinaryPlaylist`；
- `RemoveOrdinaryPlaylist`；
- `RenameOrdinaryPlaylist`；
- `ReplaceOrdinaryPlaylistMembers`：传入最终有序 track ID 列表。

规则：

1. 每项变更先在原模型上完整预检；任一失败时不产生半修改模型。
2. 调用方提供不存在、重复或零 ID 时明确拒绝。
3. `RemoveTrack` 不处理媒体文件；它只修改电脑内存中的数据库模型。
4. 删除 master Library、把 master 改成普通 playlist、编辑 Smart/Opaque playlist 一律拒绝。
5. 普通 playlist 成员可以为空。读取时保留既有重复成员，不把历史数据自动判坏；本任务的新增/替换操作遵守已批准的默认 Skip 语义，调用方给出重复成员时返回 `DuplicatePlaylistMember`。
6. 如果输入包含可能依赖将被修改对象的 opaque index/dataset，而 profile 没有声明可安全重建，返回 `OpaqueDependency`，不能盲目保留一个已失效索引。
7. `TraditionalPreserveOnly` 的任何 EditPlan 都在执行前返回 `ProfileNotWritable`。

## 7. ID、时间与确定性

- 已存在 ID 永远不因无关修改而变化。
- 新 32 位 track ID 从现有非零最大值之后分配；溢出或冲突返回 `IdExhausted`。
- 新 64 位 persistent ID、数据库 ID、playlist ID 和时间由调用方注入的 `GenerationContext` 提供；Core 不读取系统时钟、不使用全局随机数。
- `GenerationContext` 产生的值必须非零且唯一；否则拒绝写入。
- 相同输入模型、profile、EditPlan 和 GenerationContext 必须生成逐字节相同输出。
- 无修改流程不更新时间戳；只有 EditPlan 明确影响的对象和必要父容器可以使用注入时间。
- Comparator 不允许用“所有 ID/时间戳都可变化”掩盖非确定性；允许变化必须来自具体 EditPlan。

## 8. 未知数据保留

保留优先级固定为：

1. 未修改的完整 node 直接复用原始字节；
2. 修改的已知 node 只重写 profile 声明的字段、长度和计数，并拼回原始未知 header 尾部与不受影响 child；
3. 未知 sibling 的相对顺序和字节必须保持；
4. 根或父容器只更新因本次修改必需的长度、计数和 profile 规定字段；
5. 不能证明 opaque 数据与修改无关时拒绝修改。

“语义往返通过”不能替代保留验证。测试同时检查：

- 未修改输入的逐字节一致性；
- 未修改 node 的原始范围 SHA-256；
- 未知 dataset/record/payload 的位置、长度和内容；
- 已知未修改对象的领域语义；
- 只有 EditPlan 声明的对象、父长度/计数和新 ID/时间可以变化。

## 9. Writer

Writer 只接受：

- Validator 已通过的模型；
- 显式 `DatabaseFormatProfile`；
- 可选 EditPlan 产生的新模型；
- `GenerationContext`；
- 最大输出大小。

Writer 输出 `std::vector<std::byte>`，不接受目标路径。任务 003 不实现临时文件、Flush、rename、backup 或设备提交。

流程固定为：

1. 校验 profile 与输入能力；
2. 校验 EditPlan 和 opaque 依赖；
3. 分配新 ID/时间；
4. 计算每个 node 的精确输出大小，全部检查溢出和总大小；
5. 一次性序列化到内存；
6. 用全新 Reader 实例读取输出；
7. 用独立 Validator 验证；
8. 用 Comparator 对照原模型、EditPlan 和输出；
9. 任一步失败都丢弃输出并返回结构化错误。

Writer 不“修复”损坏输入，不默认删除未知记录，不排序原有对象，不改变字符串，不补造 dummy 字段。

## 10. Validator

Validator 与 Reader、Writer 分离，至少检查：

- 根、dataset、record 的 marker、范围、长度和 count 一致；
- 一个且仅一个 master Library，且位于可写 profile 规定的位置；
- track ID 必须非零且唯一；track/playlist persistent ID 是否必需由 profile 决定，已存在的非零 persistent ID 必须在各自命名空间唯一；
- master Library 完整引用每个 track，且没有未知 track；
- 普通 playlist 每个成员引用存在的 track并保持顺序；既有重复成员允许原样生存，但 EditPlan 不新增重复；
- Smart/Opaque playlist 的保留 token 与来源 node 一致；
- profile 需要的 dataset、header 大小、编码、最小空库结构；
- `TraditionalUnsigned` 不包含签名要求或 profile 禁止的新记录；
- `TraditionalPreserveOnly` 只有无修改逐字节输出才通过写后验证。

结构损坏是 fatal error。合法未知内容产生 warning，但不降低为“可写”；写能力只由 profile 和 opaque 依赖检查授予。

## 11. 错误与诊断

错误使用稳定枚举和结构化位置，不依赖英文异常字符串。至少包括：

- `TruncatedInput`
- `InvalidMarker`
- `HeaderTooSmall`
- `SectionOutOfBounds`
- `LengthOverflow`
- `CountExceedsSection`
- `UnsupportedContainer`
- `UnsupportedEncoding`
- `ResourceLimitExceeded`
- `InvalidText`
- `DuplicateTrackId`
- `DuplicatePersistentId`
- `MissingMasterPlaylist`
- `MultipleMasterPlaylists`
- `DanglingTrackReference`
- `DuplicatePlaylistMember`
- `ProfileMismatch`
- `ProfileNotWritable`
- `OpaqueDependency`
- `IdExhausted`
- `GeneratedOutputInvalid`
- `ComparisonMismatch`

每个错误包含可用时的 marker、无符号 byte offset、结构路径和安全摘要。不得包含原始媒体元数据、真实设备名称、完整路径、数据库 hash 或稳定设备 ID。

Reader/Writer 不让解析异常越过公开 Core API；内存不足仍由标准运行时处理，不能伪装成格式错误。

## 12. Comparator

Comparator 提供三个明确模式：

- `ExactBytes`：用于 preserve-only 和合成 fixture 的 no-op；
- `Semantic`：比较已理解模型、对象顺序、ID 和引用；
- `Preservation`：比较未知区域、未修改 node 和允许变化集合。

比较报告按结构路径列出：

- requested change；
- required derived change（父长度、count、master 成员等）；
- unexpected semantic change；
- unknown-data change；
- byte range change。

私有 fixture 报告默认只显示对象类型、数量、offset/length 和遮蔽摘要，不显示名称、路径或完整 hash。测试失败必须能够定位到首个不允许差异，不能只返回 false。

## 13. 未签名空 Library

任务 003 的空库生成只接受 `TraditionalUnsigned`，并要求调用方提供：

- database persistent ID；
- master Library persistent ID；
- Library 名称；
- 确定时间。

输出只包含该 profile 声明的传统根、空 track dataset 和一个零成员 master Library；不创建 Music 目录、Device 文件、artwork、普通/Smart playlist、签名、媒体文件或设备属性。

空库必须：

1. 由独立 Reader 读取；
2. 由 Validator 通过；
3. 再次用相同 GenerationContext 输出时逐字节一致；
4. 添加一个虚拟 track 后 master 自动包含它；
5. 删除最后一个 track 后回到合法零曲目 Library；
6. 只作为电脑侧产物，不宣称任何实机固件接受。

## 14. 自动测试矩阵

### 14.1 合成公开 fixture

- 最小空 Library；
- 一首/多首 track；
- 空普通 playlist、有序成员、删除成员；
- Smart playlist opaque payload；
- 未知 dataset、未知 child、已知 header 未知尾部；
- root 尾部数据；
- 固定 GenerationContext 下的 golden bytes。

### 14.2 损坏与边界

- 对最小合法 fixture 的每个截断位置验证不会越界；
- header/section 长度小于最小值、超过父级、整数溢出；
- count 与可容纳 record 数不符；
- 未知/错误 marker、容器、编码和压缩标志；
- 零/重复 ID、缺失/重复 master、悬空/重复 playlist 引用；
- 512 MiB 输入限制和配置输出限制；
- 无效 UTF-16、奇数字节字符串和过长 payload；
- GenerationContext 返回零、重复或耗尽 ID；
- opaque dependency 阻断修改。

### 14.3 往返与修改

- no-op 逐字节一致；
- parse → validate → write → parse 的领域语义一致；
- 未知 node 和未修改 node 的 byte range/hash 一致；
- Add/RemoveTrack 正确维护 master 与普通 playlist；
- 新建、重命名、删除和替换普通 playlist 成员；
- 相同上下文的确定性输出；
- Comparator 能抓到每一种非允许变化；
- Writer 生成无效结果时不返回部分字节。

### 14.4 Nano 4 私有 fixture

- 测试仅在 Git 忽略的固定 fixture 存在时注册；当前输入包括 `nano4-20260908-original` 非空库和 `nano4-20260909-restored-clean-windows` 空库；
- Reader 读取共同结构并报告未知保留项；
- Validator 检查共同 ID/引用，不验证任务 004 才实现的 hash58；
- no-op 输出与输入逐字节/SHA-256 一致；
- 任意 EditPlan 返回 `ProfileNotWritable`；
- 测试日志不输出曲名、playlist 名、路径、设备 ID 或完整数据库 hash；
- fixture 缺失不让公开 CI 失败，但公开合成 fixture 的全部测试始终运行。

### 14.5 iPod 5.5G 私有 fixture

- 输入为 `ipod55g-20260909-clean-windows-readonly` 的 Device/iTunes/Artwork 子集，测试不访问设备；
- 在精确型号和 profile 证据完成前使用 `TraditionalPreserveOnly`；
- Reader 必须在 4,034,040 字节真实数据库上有界完成，不输出媒体元数据；
- no-op 输出与 `iTunesDB` 输入逐字节/SHA-256 一致；
- 任意 EditPlan 返回 `ProfileNotWritable`；
- Artwork 只作为 opaque 外部 fixture 保存，不由任务 003 解析；
- fixture 缺失不让公开 CI 失败，私有运行结果单独记录。

### 14.6 架构与回归

- database Core 不能 include/link foobar2000、PFC、Columns UI、FooCrate、Windows 设备 API 或文件系统事务；
- Debug/Release 使用 `/W4 /WX /permissive- /Zc:__cplusplus /utf-8`；
- 任务 002 的版本、合同、组件身份和 source boundary 测试继续通过；
- 不访问 `Ref`、设备盘符、`foobar-dev`、`foobar-test` 或 C 盘日常安装；
- 本任务没有用户可见组件能力，因此不生成新的人工候选包。

## 15. 计划源码边界

建议永久文件布局：

```text
include/foopodbridge/core/database/
  error.h
  format_profile.h
  model.h
  reader.h
  edit.h
  writer.h
  validator.h
  comparator.h
src/core/database/
  reader.cpp
  edit.cpp
  writer.cpp
  validator.cpp
  comparator.cpp
  traditional_unsigned_profile.cpp
tests/
  database_reader_tests.cpp
  database_writer_tests.cpp
  database_corruption_tests.cpp
  database_private_fixture_tests.cpp
```

`foopodbridge_core_database` 从任务 002 的依赖边界变为真实静态库；它可以依赖通用 `foopodbridge_core`，不能反向依赖 device/media/transaction/foobar。具体文件可以在不改变 API 和测试边界的前提下合并，不能把 Reader/Writer/Validator 做成一个无法独立验证的类。

## 16. 实施顺序

1. 先建立 error、bounded cursor、profile 和最小合成 fixture 测试；
2. 实现结构树 Reader 与损坏输入测试；
3. 实现领域投影和独立 Validator；
4. 实现 preservation/no-op exact bytes；
5. 实现 `TraditionalUnsigned` 空 Library Writer；
6. 实现 EditPlan、ID/时间注入和普通 playlist 修改；
7. 实现 Comparator 和未知数据保留测试；
8. 接入 Nano 4 私有 fixture 的可选只读/preserve-only 测试；
9. 完成 Debug/Release 全量构建与 CTest；
10. 更新任务证据并交给用户检查结构/比较报告。

实现期间不部署组件、不启动 foobar2000、不访问真实设备。

## 17. 完成与用户验收

“实现完成待验收”要求：

- 第 14 节全部适用自动测试通过；
- Nano 4 私有 fixture 存在时完成 preserve-only 检查，不存在时明确记录而不伪造；
- Debug/Release 全量构建和 CTest 通过，无新增警告；
- 来源只使用 `REFERENCE_PROVENANCE.md` 已批准的“知识参考后原创实现”路径；
- 没有设备 I/O、签名写入、组件 UI、包或部署；
- 提供一份脱敏比较报告，用户能看懂“请求变化、派生变化、必须保持和拒绝原因”。

用户只需检查以下五个边界：

1. 任务 003 唯一可写的是合成未签名 profile，不宣称真实 Photo/Nano 可写；
2. Nano 4 本任务只读共同结构、no-op 原字节保留，任何修改都拒绝；
3. Smart Playlist、artwork、SoundCheck、gapless 等数据保留但不在本任务编辑；
4. opaque 数据可能依赖修改对象时宁可拒绝，不冒险留下失效索引；
5. 所有输出只在内存/电脑测试目录验证，不连接设备。

用户明确批准这五点并授权实现后，任务状态改为“可实现/实现中”，开始 C++、构建和测试。磁盘模式、iTunes 配置和 Restore 已由 `DEC-DEV-002` 排除，不再作为任务 003 的检查点。

2026-09-09，用户在确认设备已弹出后明确批准“任务 003 按 SPEC 实现”，并将规格核对交由 Codex 负责。本次授权覆盖 C++ 实现、电脑端私有 fixture 测试以及 Debug/Release 构建与 CTest；不覆盖设备访问、设备写入、组件部署或候选包制作。
