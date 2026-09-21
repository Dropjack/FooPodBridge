# 006 FooCrate 只读 Devices 工作区：实施准备

- 状态：已验收（FooCrate 1.1.0-beta.2 / FooPodBridge 0.1.0-beta.4 的已批准只读浏览范围）。
- 日期：2026-09-21。
- 准备阶段活动实现为 005；用户随后明确批准 006 实施，005 未验收项仍独立跟踪。
- 用户要求重复项非关键时暂缓，优先推进更多有效工作；本轮提前准备 006，不把此要求当作对尚未展示界面的批准。
- 方案：[`SPEC.md`](SPEC.md)；布局草案：[`WORKSPACE.svg`](WORKSPACE.svg)。

## 目标与输入

使用 FooPodBridge 发布的真实不可变快照，在 FooCrate 左栏添加 Devices，中央浏览设备 Library/playlist。无服务隐藏 Devices，服务失败明确显示状态；不建立 foobar 播放列表、不自动同步、不写设备。

现有证据是用户 beta.3 截图的三栏 FooCrate 布局及约 119 GiB、1862 条记录的只读信息。曲名、艺术家和设备 playlist 名称未提供，草案不得虚构为已读取数据。5.5G 为用户称呼；程序当前只能显示 Classic、未知代际，不能凭用户称呼硬编码型号。

## 本地代码接入点

2026-09-21 已只读核对 FooCrate AGENTS、项目总则、任务入口及当前 023 文档，Git 状态无改动；未修改、构建或启动 FooCrate。

- `D:/dev/foo/FooCrate/src/playback_panel.cpp`：现有 `drawPlaylistBrowser`、`drawPlaylistView`、workspace 切换、键鼠消息和窗口生命周期是接入处；独立设备控制器承载状态，不把解析/磁盘访问塞进绘图函数。
- `D:/dev/foo/FooCrate/src/playlist_browser_model.h`：现有 manual/automatic/reserved 类型代表 foobar playlist，不能添加伪造 GUID 设备行冒充 playlist。
- `include/foopodbridge/service_v1.h` 与 `service_readonly.h`：0BSD 公共接口与 GUID 可独立消费；FooCrate 不链接 LGPL Core 或复制数据库代码。
- 当前接口提供标题、艺术家、专辑、媒体类型、playlist 与成员 ID、状态、容量和快照有效性；没有公开的绝对媒体路径、时长/码率、Artwork 解码或播放解析接口。006 首轮浏览不伪造这些字段，不通过 relative_path 拼盘符或直接访问 iPod_Control。

## 顺序和通过标准

1. 用户核对 SPEC 和草案；批准详细 UI 后才改 FooCrate。
2. 完成服务消费者及独立只读视图模型：用合成服务测试服务缺失、旧 ABI、同名设备、过期快照、playlist 顺序与非音乐分类，不启动应用。
3. 接入原生 sidebar、中央虚拟列表、概览和键盘；绘图只消费内存数据，控件不调用设备 I/O。
4. 检查退出/窗口销毁取消订阅、主线程事件合并、旧回调丢弃；两仓库各自 Debug/Release 和回归。
5. FooCrate 自己的 dist 生成新 prerelease，FooPodBridge 必要时另包；不混装 DLL、不自动安装到 foobar-test。
6. 用户逐项检查设备浏览、返回普通 playlist、不干扰现有播放、服务缺失和设备移除。手动检查全部保持待执行。

## 已知依赖与决定

- DUP-001：一台实机在 beta.3 重插后仍显示两项，用户允许暂缓。不以名称合并不同 token，不隐藏失败项冒充修复；待后续只读拓扑证据确认。进入真实写入前必须解决。
- 005 全局通知会使所有快照失效、跨设备读取仍串行；006 须支持 loading/失效，不能承诺设备独立刷新已完成。
- UI 详细方案需要用户核对，依据 `specs/UI_DIRECTION.md` 第 6 节；本轮准备不代表 006 完成或 005 已验收。

## 2026-09-21 界面方向核对

用户要求顶部保持普通列表样式，左侧只需播放列表与歌曲总数，不显示 Library 内有声读物数量，沿用设备已有 Smart Playlist 浏览有声读物。已同步 DEC-UI-003、总规格、SPEC 与草案；底层分类仍保留。该决定覆盖顶部/左侧方向，不代表全部 UI 已验收。

用户随后确定右上区域保持歌曲播放信息，Device overview 放在右下歌词区域，作为仅在 iPod 浏览上下文提供的第三页签。设备信息非核心，不自动抢走歌词；切回普通内容隐藏并恢复此前的右下页。此布局方向已批准，后续实现按 SPEC 执行，不再重复请求同一布局批准；任务 005 剩余项与人工验收状态保持真实记录。

## 实施进度与环境断点

用户明确“和我预期完全相同。就这么做了”，据此进入 FooCrate 实现；005 的未验收项与 DUP-001 继续独立跟踪，不伪造为已验收。实现记录在 `D:/dev/foo/FooCrate/tasks/027-集成FooPodBridge只读设备浏览/README.md`。

已接入只读设备树、分批内存曲目投影、原生虚拟表、右下条件第三页签、退出恢复、主题/DPI 与命令隔离，消费者仅使用 0BSD 服务合同。新增页签/ID 索引测试与其余非 ATL 回归共 14 项，在 Debug/Release 均通过；FooCrate 源码独立编译通过但完整 DLL 尚未链接成功。

原 SDK 的 filedialogs_vista.cpp 依赖 ATL，本机 Build Tools 缺少 atlbase.h，首轮全构建失败。用户允许补装但关心并行 Unity 测试；只读查询发现 3 个 Unity 编辑器运行，尚未启动安装器或修改工具链，等待用户告知安装时机。禁止 --force/自动重启，不结束其他项目进程。没有新 FooCrate 包、自动部署、应用操作或设备写入。

## 2026-09-21 ATL 补齐后的完整验证

用户关闭 Unity 并手动安装 ATL，已检测到 MSVC 14.44.35207/atlmfc/include/atlbase.h。此前一次自动补装被已有安装器锁阻挡，未强制关闭或重启任何进程。

FooCrate x64 Debug/Release 全构建成功，各 15/15 测试通过，包括 device_browser 与 artwork_cache。Release 仅有任务 003 已记录的第三方 SDK 五处 C4996 弃用警告，FooCrate 自有代码无新增警告。日志为 build/devices-1.1.0-beta.1/full-debug.log、full-release.log 与各配置 Testing/Temporary/LastTest.log。

候选包：D:/dev/foo/FooCrate/dist/FooCrate-1.1.0-beta.1.fb2k-component，425789 字节。已检查 ZIP CRC、仅含根目录 foo_crate.dll、包内 DLL 与 Release DLL 完全一致、PE x64、ProductVersion 1.1.0-beta.1。包 SHA-256：1246D005C19E2AB8AAFFC7B327CC037FD32D6ABF83A206820F108627C20B50A9。

首次打包已完成归档及结构检查，但最后 Get-FileHash 因嵌套 PowerShell 环境缺失命令而退出；改用 .NET SHA256，既有候选不覆盖，独立 Python 审计已通过。该修改不改变组件二进制。

未部署、启动或操作应用，未访问真实设备文件，未提交或推送。人工组件加载、布局、播放隔离、刷新/移除、服务缺失、主题/DPI 仍待用户逐项验证。第一检查点：在 foobar-test 手动安装 FooCrate 本包及 FooPodBridge 0.1.0-beta.4，点击设备 Library，反馈界面截图；核对真实曲目表与右下条件 Device 入口。DUP-001 仍暂缓。


## 2026-09-21 用户修订：隐藏页面入口

用户明确要求恢复 FooCrate 细滚动条，右下不主动显示页签，沿用鼠标中键切换。此决定覆盖此前可见 Lyrics / Track details / Device 页签设计：普通上下文保持原来的两页；iPod 上下文中键依次切换歌词、歌曲信息、设备概览，歌词不可用时跳过。进入设备浏览不自动切页，退出时仍恢复此前普通页面。无需为隐藏入口预留高度。设备树、曲目表、概览滚动条使用主题色的 3 DIP 细滑块并在空闲时隐藏，保留滚轮、拖动和轨道点击。

## beta.2：细滚动条与隐藏页面入口

用户确认 beta.1 设备内容已显示，截图指出原生粗滚动条，随后明确要求恢复细样式，右下仅用中键切换。此反馈不等同于整体任务验收。

- 新增 device_scrollbars.h：设备树、虚拟表与概览的原生滚动区域使用共享 3 DIP 滑块宽度、1000 ms 隐藏时限和主题色；保留滚轮/键盘滚动，单独处理滑块拖动、轨道翻页、捕获释放、DPI、销毁与空滚动范围。原生非客户区命中宽度保留，视觉去除粗轨道及箭头。
- 删除右下 TabControl 和 28 DIP 高度预留；中键经原歌词宿主、父面板或概览文本入口统一轮换，普通上下文仍是原两页。新增模型测试覆盖三页轮换、歌词缺失跳过、退出恢复与普通两页。
- FooCrate 1.1.0-beta.2：Debug/Release 全构建成功，各 15/15 测试通过，无新增编译警告。沿用现有构建树 build/devices-1.1.0-beta.1；日志 beta2-debug-final.log / beta2-release-final.log。包在 D:/dev/foo/FooCrate/dist/FooCrate-1.1.0-beta.2.fb2k-component，ZIP CRC、仅含 foo_crate.dll、x64、与 Release 字节一致均通过。旧 beta.1 保留。
- 没有更新 FooPodBridge 二进制，没有部署/启动应用、访问或写入设备。待用户在 foobar-test 手动更新 FooCrate，先检查 Library 界面已无可见右下页签及粗滚动条；之后再逐项检查中键轮换和滚动拖动。不能用模型测试替代实际 UI 验证。

## beta.2 人工外观检查通过

2026-09-21：用户提供 beta.2 的 Library 界面截图并明确表示满意。截图确认真实设备列表/曲目显示、滚动条细样式、右下不显示页签栏。此项外观检查通过；中键轮换、退出设备上下文恢复、滚动条拖动和其他生命周期检查尚未由本次截图验证，整体任务仍待验收。下一检查：在 foobar-test 保持设备 Library 上下文，在右下区域用中键轮换，确认能进入并离开 Device overview。

## beta.2 人工中键轮换检查通过

2026-09-21：用户在要求检查设备 Library 下右下区域中键来回切换后回复没有任何问题，并提供 Device overview 截图。确认设备上下文中键轮换通过，概览显示 Read-only、119.00 GiB 容量和 94.67 GiB 可用空间。未据此扩展为播放隔离、退出恢复、移除或服务缺失检查通过。下一检查：概览显示时点击左上普通 Library (full)，确认恢复歌曲信息/歌词，且普通上下文中键不再进入设备概览。整体任务仍待剩余人工验收。

## beta.2 人工退出设备上下文检查通过

2026-09-21：用户按上一检查要求，从显示 Device overview 的状态点击普通 Library (full)，并确认没有任何问题。退出设备浏览恢复歌曲信息/歌词、普通上下文中键不再进入 Device overview，均记录为通过。下一检查：播放测试实例中已有的普通曲目时浏览设备 Library/playlist 并切换右下概览，确认播放不中断、不换曲，右上仍显示原播放歌曲。播放隔离及其他未执行检查仍待验收。

## beta.2 人工播放隔离检查通过

2026-09-21：用户确认播放普通本地歌曲期间浏览设备 Library/playlist、切换右下设备概览的检查已经一起测试通过。记录播放不中断、不换曲、右上维持原歌曲信息。此前已通过外观、中键轮换和退出设备上下文恢复；不把此确认扩大为尚未执行的滚动条拖动、设备移除、服务缺失和主题/DPI 检查。下一检查为设备 Library 纵向细滚动条拖动，确认曲目跟随且松开后停止。

## beta.2 人工纵向滚动条拖动检查通过

2026-09-21：用户针对设备 Library 中央列表纵向细滚动条上下拖动、曲目跟随及松开停止的检查回复没有任何问题，记录为通过。下一检查：在 foobar-test 保持设备 Library 打开，通过 Windows 安全弹出设备并断开，确认旧曲目清除、状态反映断开且普通播放列表可用。设备移除、服务缺失和主题/DPI 等未执行项仍待验证，整体任务不标记已验收。本轮记录在 FooPodBridge；FooCrate 文档尚未同步此条。

## beta.2 人工安全弹出与拔线状态检查通过

2026-09-21：用户提供两张截图，明确第一张为 Windows 安全弹出后、第二张为拔线后，并反馈有效设备项消失及时。第一张旧曲目已清除，剩余设备项显示 Not mounted / Windows has not exposed an accessible storage volume；第二张设备树显示 No iPod devices detected，中央显示 Device disconnected。此状态响应与清除失效曲目检查通过。安全弹出后仍留一项的具体来源未证明，不能据此认定 DUP-001 已修复。截图不证明普通 playlist 后续操作、重新连接恢复或服务缺失场景通过。下一检查：重新连接同一 iPod，等待读取后选择 Library，确认真实曲目及播放列表恢复，无需重启 foobar。本条仅更新 FooPodBridge 记录。

## beta.2 人工重新连接恢复检查通过

2026-09-21：用户确认重新插入同一设备后，资源管理器出现时 foobar 也及时恢复设备内容，无需重启；截图显示设备播放列表选中及其真实曲目，右上歌曲信息与右下歌词仍显示。重新连接恢复检查通过，不把用户所说的出现推断为程序主动抢焦点。已通过外观、中键轮换、退出恢复、播放隔离、纵向滚动条拖动、安全弹出/拔线及重连恢复。服务缺失、主题/DPI 等未执行项仍待验证，DUP-001 仍暂缓。下一检查优先验证 FooCrate 无 FooPodBridge 时可独立使用：仅在 foobar-test 手动移除 FooPodBridge 并重启，确认 Devices 消失且普通播放/歌词可用；不能操作日常安装。本条仅更新 FooPodBridge 记录。

## beta.2 人工服务缺失与独立使用检查通过

2026-09-21：用户针对 foobar-test 移除 FooPodBridge、保留 FooCrate 并重启后的检查明确确认没有问题。记录 Devices 区域隐藏、普通播放列表/播放/歌词正常，FooCrate 可不安装 FooPodBridge 独立使用的原则已通过人工验证。已确认的其他项目保持原记录；主题/DPI 检查仍未执行，不将整体任务提前标记已验收。下一检查：恢复 FooPodBridge beta.4 后，在测试实例使用 FooCrate 深色主题检查设备 Library、细滚动条与右下概览的可读性。本条仅更新 FooPodBridge 记录，FooCrate 文档尚待同步。

## beta.2 人工深色主题检查通过

2026-09-21：用户针对恢复 FooPodBridge beta.4 后切换 FooCrate 深色主题、检查设备 Library 文字/选中行/细滚动条可读性的要求回复没有问题。按用户报告记录深色主题检查通过；本条没有附加截图，不声称已目视核验。DPI 缩放检查仍待执行，整体任务尚未标记已验收。下一检查：若有不同缩放比例的显示器，将 foobar-test 移至该屏，检查设备列表、滚动条与概览无裁切或错位；没有该环境则明确记录未测，不要求更改系统缩放。本条仅更新 FooPodBridge 记录。

## beta.2 人工 DPI 检查与本轮验收结论

2026-09-21：用户针对不同缩放显示器下设备列表、细滚动条和右下概览无裁切/错位的检查回复没有问题，按用户报告记录 DPI 检查通过。

已逐项获用户确认：外观、隐藏入口中键轮换、退出设备上下文恢复、播放隔离、纵向滚动条拖动、安全弹出与拔线响应、重新连接恢复、无 FooPodBridge 时 FooCrate 独立使用、深色主题、DPI。任务 006 的本轮已批准只读浏览范围据此标记已验收，候选为 FooCrate 1.1.0-beta.2 与 FooPodBridge 0.1.0-beta.4。不声称所有硬件、系统主题或任意故障场景均已覆盖。

任务 005 的 DUP-001（一台设备重复显示）按用户决定继续暂缓；本结论不代表 005 全部验收、写入授权或任务 007 实施授权。未生成稳定版、未提交或推送。FooCrate 仓库当前不在可写范围，本轮后续人工记录保存在此处，其任务文档尚待同步。

## 提交留档

2026-09-21：用户明确授权两仓库 commit + push。FooCrate 人工验收记录已同步；本次提交源码、自动测试代码、规格和验收文档，不提交新 beta 组件包、不创建 Release，不改变仓库可见性。
