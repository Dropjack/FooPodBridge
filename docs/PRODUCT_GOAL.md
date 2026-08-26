# FooPodBridge 产品目标

- 状态：规格核对中
- 日期：2026-08-25
- 对应总规格：[`../specs/PRODUCT_SPEC.md`](../specs/PRODUCT_SPEC.md)
- 当前决策任务：[`../tasks/000-确定项目目标与全部产品决策/README.md`](../tasks/000-确定项目目标与全部产品决策/README.md)

## 1. 项目为什么存在

现代 foobar2000 x64 无法加载旧版 x86 `foo_dop / iPod manager`。用户希望在 FooCrate 和普通 foobar2000 UI 中重新获得 click-wheel iPod 的手动音乐管理能力，同时避免 iTunes 的媒体库、商店、账户、自动同步和现代 Apple Music 负担。

本项目不是把旧插件勉强编译成 x64，也不是复刻 iTunes 全部历史功能。它建立新的 x64 Core、事务安全模型和现代 UI 适配层，只复用已经被历史项目验证的 iPod 数据格式知识和算法。

## 2. 优先级

优先级从高到低固定为：

1. 项目维护者自己的 iPod Photo 和 iPod Classic 可以可靠日常使用；
2. 任意中断点优先保护设备数据库和可恢复性；
3. FooCrate 集成体验完整、直接、容易理解；
4. 每项开发任务都能独立解释、测试和验收；
5. 独立 Columns UI 和 Default UI 让 FooPodBridge 脱离 FooCrate 仍可使用；
6. 其他用户、其他型号和更大范围兼容只在有实机与维护者时考虑。

项目不会为了 GitHub 上潜在用户牺牲两台真实设备的可靠性，也不会宣称支持没有实机验证的型号。

## 3. 完整产品目标

完整日常设备管理能力包括：

- 发现、识别并显示受支持设备、固件、文件系统、容量与剩余空间；
- 读取设备曲目、Music、Audiobooks、普通播放列表和 Smart Playlist；
- 从 foobar2000/FooCrate 主动拖入或发送设备原生支持的音频；
- 导入前检查设备身份、格式/profile、空间、重复项、目标类别和冲突；
- 将音频、元数据、排序字段、封面、SoundCheck、gapless 和 Audiobook 属性写入正确设备结构；
- 将 Playback Statistics 评分在导入或用户明确刷新时单向写为设备原生 Rating，供原生 Smart Playlist 使用；
- 从设备 Library 删除曲目，或只从某个播放列表移除引用，并让两种行为明确区分；
- 创建、重命名、删除、排序和编辑普通设备播放列表；
- 创建、查看和编辑原生 iPod Smart Playlist，在设备能力范围内支持规则、限制、排序和 Live Updating；
- 显示批次计划、逐项进度、成功、警告、失败、取消和恢复结果；
- 每个写入批次执行备份、暂存、数据库往返验证、提交和中断恢复；
- 每次操作结束后 Flush 并释放全部设备句柄，让用户继续通过 Windows 资源管理器弹出；组件本身不提供 Eject；
- 提供 FooCrate 集成 UI、独立 Columns UI Device Panel 和简化 Default UI Element。

## 4. 已批准的产品行为

- 只正式写入维护者实测的 iPod Photo 和 iPod Classic。
- 纯手动管理；没有后台同步、媒体库镜像或一键 Sync。
- 不双向同步播放次数、评分、跳过次数或播放位置；评分仅按用户明确操作从 Playback Statistics 单向导出，设备端改动不反写电脑。
- 导入支持 ReplayGain 到 SoundCheck。
- Classic 写准确 gapless 数据；Photo 不承诺。扫描失败不阻断批次，不写 dummy 数据。
- 实现真正的原生 iPod Smart Playlist 编辑器，不用普通 playlist 冒充。
- foobar/FooCrate playlist 对象不发送或拖入设备；普通设备 playlist 由用户明确 New 后加入曲目，原生 Apple Smart Playlist 只通过规则编辑器创建和编辑。
- Podcasts、Video、Photo、iPhone、iPod touch 和云内容不在产品范围内。
- 未验证设备显示 Unsupported 并拒绝所有写入。

## 5. 明确排除

- Apple Mobile Device Support、iPhone/iPod touch 通信和越狱路径；
- 读取、加载或发布 x86 `iTunesCrypt.dll`；
- 自动管理 Podcasts、Video 或 Photo；
- 自动同步整个 foobar2000 媒体库或播放列表；
- 后台监控媒体库并静默修改已连接设备；
- 依赖 iTunes、Apple Music 或 Apple 账户才能执行日常导入；
- 修改 foobar2000 核心、C 盘日常安装或只读参考目录；
- 对未实测 Nano、Shuffle、Mini 或其他型号作写入兼容承诺。

## 6. 仍由用户决定的产品方向

所有尚未批准的选择集中在 [`USER_DECISIONS.md`](../decisions/USER_DECISIONS.md)。直接导入、元数据、评分、playlist、删除、测试期备份/恢复、清理、取消、多设备和外部移除边界已经批准；仍待核对的内容包括 FLAC 转码、正式日常版是否继续要求首次完整基线及保留多少数据库快照、UI 信息结构细节、许可证和首轮实机样本。

这些决定不是模糊占位：每项都有推荐方案、替代方案、影响范围和阻断任务。当前任务 000 的完成条件就是逐项批准或明确排除它们。

## 7. 不采用删减式 MVP

项目允许按永久纵向能力推进，例如先完成可验证的只读数据库 Core，再增加安全导入；但每项标记为实现完成的能力必须具备完整错误处理、生命周期、自动检查和人工验收。不会交付：

- 只有树形列表、没有真实设备数据的空 UI；
- 只解析一个成功样本、遇到未知记录就损坏数据库的 reader；
- 能复制音频但不验证数据库的导入；
- 只能创建却不能安全取消、恢复或删除的操作；
- 假进度、假设备、硬编码测试结果或 Release 中的测试替身。

## 8. 交付形态

### FooPodBridge

`FooPodBridge-<version>.fb2k-component` 包含 x64 `foo_pod_bridge.dll`，提供：

- 独立 C++ Core；
- foobar2000 设备服务；
- 独立 Columns UI Device Panel；
- 简化 Default UI Element；
- Preferences 与诊断入口。

### FooCrate

FooCrate 在自己的独立组件中消费 FooPodBridge 服务，提供重点维护的 Devices 体验。FooCrate 不安装或捆绑 FooPodBridge，两个组件独立升级。

## 9. 成功标准

产品达到首个完整可用版本时：

1. 用户能在 FooCrate 中插入 Photo 或 Classic，查看真实设备 Library、容量和播放列表；
2. 用户能把兼容音频手动导入 Music 或 Audiobooks，并看到准确进度与结果；
3. 用户能安全删除曲目、管理普通播放列表和 Smart Playlist；
4. SoundCheck、Classic gapless、封面和 Audiobook 属性在实机上工作；
5. 任意模拟写入失败都不会让旧数据库变成不可启动状态；
6. 两台实机完成添加、删除、播放列表、重启设备和再次连接的回归；
7. 独立 Columns UI 与 Default UI 可以调用相同能力；
8. 所有候选只在 FooCrate 双隔离实例测试，C 盘日常 foobar2000 从未被触碰；
9. 组件包不包含 Apple/foobar2000 闭源文件、来源不明二进制或用户设备数据。
