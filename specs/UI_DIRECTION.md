# FooPodBridge UI 方向

- 状态：信息结构方向已批准，详细视觉与交互等待对应 UI 任务
- 日期：2026-08-26
- 产品总规格：[`PRODUCT_SPEC.md`](PRODUCT_SPEC.md)

## 1. 三个入口

### FooCrate 集成界面

第一优先级。FooCrate 保持既有 `Playlist Browser | Playlist View | Right Side Panel` 和主题体系，在 Playlist Browser 下方增加独立 Devices namespace。设备内容不伪装成 foobar playlist，也不写入 Playlist Manager。

初步信息结构：

```text
Playlists
├── Active Playlist
├── Favorites
└── ...

Devices
└── <Device Name>
    ├── Library
    │   └── Music / Audiobook media kinds
    └── Playlists
        ├── <Normal Playlist>
        └── <Smart Playlist>
```

选择设备节点后进入 Device Workspace。第一版允许在统一 Library 中管理 Music 与 Audiobook，不强制独立 Audiobooks 页签；用户仍必须明确选择导入 Media Kind。详细决定包括筛选/目标入口、中央曲目列表、设备概览、容量条、传输队列、右栏作用和 Smart Playlist 编辑器入口；用户已明确要求到 UI 任务再看 mockup 后决定。

### 独立 Columns UI Device Panel

面向不用 FooCrate 的 Columns UI 布局。它必须使用同一 FooPodBridge 服务，不维护第二套设备状态。功能范围由 `DEC-UI-005` 冻结；视觉遵循 Columns UI 主题而不是复制完整 FooCrate 外壳。

### Default UI Element

使用 Default UI 原生、简洁的列表和命令外观。功能范围由 `DEC-UI-006` 冻结。简化视觉不允许简化事务、安全提示或错误结果。

## 2. 共享状态

三个入口必须表示相同状态：

| 状态 | 必须表达的含义 |
| --- | --- |
| Service missing | FooPodBridge 服务不存在；FooCrate 隐藏 Devices，独立入口不可创建 |
| No device | 服务正常，但没有设备；不显示伪设备 |
| Discovering | 正在识别真实卷、设备和能力 |
| Unsupported | 已发现设备但不能安全写入，显示原因 |
| Read-only | 能读取 Library，但当前条件禁止写入 |
| Ready | 快照有效，可以计划操作 |
| Planning | 正在分析格式、空间、重复、SoundCheck、gapless 和目标 |
| Transferring | 正在写音频；显示真实项目与字节进度 |
| Updating library | 音频已准备，正在生成/验证/提交数据库 |
| Cleaning | 正在完成删除或临时文件清理 |
| Recovery required | 检测到中断事务，需要明确恢复 |
| Completed | 操作结束且设备句柄已释放；需要时由用户通过 Windows 资源管理器弹出 |

UI 不能只用颜色区分 Ready、Unsupported、错误或当前操作。

## 3. 共享用户意图

UI 只能发出以下高层意图：

- 浏览某个设备 namespace 或 playlist；
- 明确选择 Music、Audiobooks Media Kind 或指定 playlist 后导入选中曲目；
- 从 playlist 移除引用；
- 从设备删除曲目；
- 新建、重命名、删除或编辑普通 playlist；
- 新建、查看或编辑 Smart Playlist；
- 明确执行 `Refresh ratings from foobar`，在计划中检查将写入、保持、清除或覆盖的设备 Rating；
- 查看操作计划、进度、结果和恢复建议；
- 取消、重试或清理由本项目产生的临时文件；不提供 Eject 命令。

UI 不能发出“覆盖 iTunesDB”“删除 F12 文件”或“忽略 hash58”之类底层命令。

## 4. 拖放方向

- 选择 Music 目标：按设备音乐能力检查并写 Music 类型，不根据 Genre、`.m4b` 或其他字段偷偷改类；
- 选择 Audiobooks 目标：写 Audiobook、Remember Playback Position、Skip When Shuffling，并在支持时保留章节；
- 拖选中的曲目到普通设备 playlist：缺失曲目导入和 playlist 引用属于同一批事务；设备已有曲目复用 track ID，目标 playlist 已有成员默认 Skip；
- foobar 普通/autoplaylist 对象：禁止发送或拖入设备；用户必须先在设备 UI 明确 New，再拖入曲目选择；
- Smart Playlist：不能拖入手工成员；只能通过原生规则编辑器明确新建或编辑；
- FLAC：当前显示 Unsupported format，不自动转码；未来只有 `DEC-IMP-003` 重开后才改变；
- 不支持目标、空间不足或事务进行中时提供明确禁止反馈。

## 5. 传输显示原则

- 显示正在处理的真实文件/曲目名称；
- 音频阶段显示项目和字节进度；
- 数据库阶段单独显示 Updating Library/Verifying，不把它伪造成复制进度；
- 批次结果区分成功、警告、跳过、失败、取消和待清理；
- UI 关闭不能取消 Core 事务对象的生命周期管理；重新打开能订阅当前操作；
- 不使用模态进度框阻止用户查看其他 foobar 内容，但写冲突命令必须禁用。

## 6. 视觉设计时机

用户决定暂不制作详细 UI mockup。对应任务开始时再：

1. 读取当时 FooCrate 实际布局、主题和可用宽度；
2. 用真实设备快照而不是假 dashboard 设计状态；
3. 比较 Device Workspace 的中央/右栏布局方案；
4. 设计容量、传输、错误、Smart Playlist 编辑和窄窗口状态；
5. 由用户选择后写正式模块规格；
6. 只在规格批准后实现。

当前文档不冻结像素、颜色、图标或右栏用途。
