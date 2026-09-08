# iPod Nano 4 纯净 Windows 与到货基线

- 日期：2026-09-08
- 设备角色：用户自有、允许破坏性实验的 16 GB iPod Nano 4
- 当前结论：外部基线已验证；只读结构证据已采集；尚未授权任何具体设备写入动作
- 私有数据：完整备份和原始 fixture 位于 Git 忽略目录，不随源码发布
- 下一次人工交接：[`NANO4_ITUNES_RESTORE_HANDOFF.md`](NANO4_ITUNES_RESTORE_HANDOFF.md)

## 1. 本次要回答的问题

1. 没有安装 iTunes/Apple Device 软件时，Windows 能否把 Nano 4 暴露成普通存储卷？
2. 能否只通过该卷读取现有 `iPod_Control` 与传统 `iTunesDB`？
3. 在任何写入前，是否已经得到可恢复并经过验证的设备外基线？
4. 这台设备应怎样进入 FooPodBridge 的 Reader、hash58、事务和实机验收路线？

## 2. 已证实事实

- Windows 将设备直接挂载为 removable volume；文件系统为 FAT32，容量约 15.03 GiB，可用空间约 11.15 GiB。
- Win32 卸载项中未发现 iTunes、Apple Mobile Device Support、Apple Application Support 或 iPod 软件；未发现 Apple/iPod/Bonjour 服务。当前受限环境不能加载 Appx 模块，因此不把“Microsoft Store 包不存在”列为自动证明；用户确认本机没有相关 iPod 软件。
- 卷中已经存在 `iPod_Control`，所以这是“有现存 Library 的二手机”，不是空白或未初始化设备。
- `iPod_Control/Music` 有 50 个目录、204 个媒体文件，共 4,101,971,789 字节；不记录或公开曲名和标签。
- `iPod_Control/iTunes/iTunesDB` 是 308,628 字节的未压缩传统数据库。`mhbd` 声明总长与实际文件相同，header 长 244 字节，数据库版本字段为 49，顶层 dataset 数为 5，hash58 区非零。
- `iPod_Control/Device/SysInfo` 存在但为 0 字节；没有 `SysInfoExtended`。这说明“Windows 能读存储卷”不等于“卷里已经保存了写库所需的全部设备属性”。稳定设备 ID、固件和 FireWire GUID/hash58 输入仍需由任务 005 的 Windows 设备查询能力取得。
- FAT 时间戳出现远未来年份，不能把时间戳作为恢复时挑选数据库版本的依据。

## 3. 已完成的安全基线

- 完整 `iPod_Control` 只读复制到 `device-backups/nano4-20260908-original/iPod_Control`；
- 共 232 个文件、4,166,620,499 字节；
- 源设备与电脑备份逐文件计算 SHA-256，零缺失、零大小差异、零 hash 差异；
- Device、iTunes 和 Artwork 原始文件另存到 `tests/private-fixtures/nano4-20260908-original`，共 22 个文件、64,590,541 字节；
- 上述目录均由仓库 `.gitignore` 排除；公开文档不保存卷标、完整数据库 hash、序列号、曲目名或 playlist 名。

设备在采集和备份过程中没有发生任何文件写入、重命名、删除、恢复或格式化。

## 4. 参考证据怎样解释 Nano 4

- iPod manager 的 `is_6g_format()` 把 Nano 4 与 Classic、Nano 3 放入同一 6G 记录路径；其更新记录另有 Nano 4 compatibility 和 artwork 修复。
- libgpod 的设备矩阵把 Nano 4 标为需要 `SysInfoExtended` 与 hash58，但不需要 hash72、SQLite 或 `iTunesCDB`。
- 当前数据库 header 的另一签名保留区也有非零数据；这只能证明历史 writer 写入过该区域，不能单凭偏移证明 Nano 4 固件要求 Nano 5 式 hash72。任务 004 必须用独立 hash58 验证、电脑副本差异和受控实机接受测试区分“必须字段”与“历史实现附带字段”。

## 5. 分阶段测试方案

### 阶段 A：到货只读基线（已完成）

确认纯净 Windows 挂载、FAT32、目录结构、数据库头、私有 fixture 和完整可恢复备份。设备保持原样。

### 阶段 B：电脑副本上的 Reader/Writer（任务 003/004）

1. Reader 有界解析私有 `iTunesDB`，建立曲目、master Library、普通/Smart playlist 和未知记录模型；
2. Validator 拒绝截断、越界、长度冲突和悬空引用；
3. 无修改往返只写电脑临时目录，比较允许变化与必须保持的数据；
4. 取得并遮蔽稳定设备 ID，验证 hash58 已知向量与当前数据库；
5. 生成签名空 Library 和“增加一首虚拟曲目”的电脑侧候选，独立再读，不写设备。

阶段 B 完成前，设备保持只读。

### 阶段 C：事务与故障注入（任务 007）

用抽象文件系统和电脑临时目录覆盖短写、磁盘满、Flush、重命名、拔线代次变化、取消和恢复组合。只有所有模拟故障都保留可恢复旧数据库，才进入实机。

### 阶段 D：Nano 4 第一次受控写入（任务 008）

第一次不格式化、不清空卖家 Library，也不修改 artwork/游戏：

1. 点名本物理设备，重新核对挂载代次、数据库指纹、备份和可用空间；
2. 选择一首用户允许导入和删除的短 MP3；
3. 生成只包含“一首新增曲目”的不可变 Operation Plan；
4. 音频以临时名写入并 Flush，再改最终名；
5. 新数据库先在电脑生成、再读和比较；设备旧数据库建立 Last Known Good；
6. 新数据库以临时名写入、完整读回、验证后提交；
7. 用户通过 Windows 资源管理器弹出，Nano 4 重启后确认 Library 和播放；
8. 再连接后由 FooPodBridge 删除该测试曲并再次重启验证；必要时从完整基线恢复。

### 阶段 E：空 Library 初始化（首次增删成功之后）

不使用 Windows 格式化。事务服务先保存并移开现有数据库引用，让已有媒体暂时成为未知 orphan（只报告、不删除），再写入最小签名 Library 和一首测试曲。设备重启验收后恢复到已验证基线。该阶段专门回答“没有 iTunesDB 时 FooPodBridge 能否初始化”。

## 6. 当前阻断项

- 任务 003 Reader/Model/Writer/Validator 尚未实现；
- 任务 004 hash58 与 Nano 4 格式 profile 尚未实现；
- Windows 稳定设备身份、固件与签名输入尚未取得；
- 任务 007 事务和故障注入尚未实现；
- 尚未点名唯一测试音频和阶段 D 唯一写入动作。

因此，完整备份已经让未来写入具备恢复前提，但它本身不构成“现在可以手工改 F 盘”的许可。
