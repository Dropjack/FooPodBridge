# 005 实施顺序与验收记录

- 状态：规格已批准；实现中；以下矩阵尚未全部通过
- 日期：2026-09-20

## 1. 实施顺序

| 步骤 | 输入与动作 | 永久产物 | 通过标准 |
| --- | --- | --- | --- |
| 1 | 核对 SPEC 与用户范围 | 批准记录 | 识别、只读边界、信息页、构建和验收范围明确 |
| 2 | 为家族匹配和状态判断先建模拟测试，再实现 | Core 注册表、证据模型、分类器 | 全目标覆盖，矛盾输入不误识别，无写能力 |
| 3 | 为 Windows 适配器与挂载生命周期建测试，再实现 | 只读枚举/通知/卷映射、取消与句柄管理 | 启动已有设备、插拔、同名、盘符复用、错误全部可解释 |
| 4 | 接入现有 Reader/Validator，补只读媒体类型投影 | 不可变 Library、路径边界和代次验证 | 无假数据，失败不发布部分成功曲库 |
| 5 | 增加 ABI 扩展及真实信息页 | 版本化服务、Preferences 只读设备信息 | 旧 ABI 兼容，新消费者可读实际状态 |
| 6 | 构建、回归、开发实例检查和包审计 | Debug/Release 证据、新候选包 | 自动门槛全通过才标记实现完成待验收 |
| 7 | 用户手动安装并核对 | 指定设备的人工验收记录 | 用户明确通过后才标记已验收、转 006 |

## 2. 自动检查矩阵

- 家族：23 条目及未知代际；USB/FireWire；明确排除 touch/iPhone/iPad；同名多设备；卷标改名；普通 U 盘复制 iPod 文件；硬件/文件证据冲突。
- 卷：无卷、无盘符但有有效挂载路径、固定/可移动标志、重复通知、卷映射不唯一、权限不足、不支持文件系统、容量获取失败。
- 数据库：有效空/非空、缺失、零字节、截断、越界、引用非法、未知版本、hash58 通过/失败/无密钥、Shuffle/Nano 5+ FormatPending。
- 快照：Music/Audiobook/其他/未知不误分类；普通/Smart/master/不透明列表计数与顺序；相对路径越界、重解析点、读取中变化、资源上限。
- 生命周期：读前/读中/发布前移除、重新挂载、盘符复用、旧 worker 晚到、多设备、Refresh 合并、取消订阅、关闭信息页、服务关闭、句柄释放。
- 服务：旧 ABI GUID/布局保持；缺少扩展安全降级；新只读接口稳定；所有写入口拒绝；失败结果不伪装成功或空库。
- 恢复：明确模拟输入覆盖 RecoveryRequired，未知 .tmp/.bak 不触发恢复、不清理；生产没有已定义恢复协议时如实报告检测能力边界。
- 隐私与只读：记录所有抽象文件系统调用，断言无写/创建/删除/重命名/Flush 写句柄；设备访问适配器审计只读权限；注入带序列号、路径和曲名的错误，确认日志不泄露。

测试使用电脑临时目录、模拟 Windows 接口和已授权私有 fixture。不得为了制造错误状态改坏实机数据库。模拟测试证明程序分支，不能替代真实设备证据。

## 3. 构建与交付

- 使用 AGENTS.md 指定的 Visual Studio Build Tools CMake/CTest，执行 x64 Debug/Release 与全部相关回归；同一失败最多三次尝试。
- 开发加载和 UI 检查只部署 `D:\dev\foo\FooCrate\.local\foobar-dev`；接触该工作区前先读取它的 AGENTS.md。人工测试实例不自动部署。
- 从现有版本和 dist 清单选择未使用的新 SemVer prerelease；审计 x64 DLL、可见版本、批准包内容、SHA-256 与依赖，然后交付本仓库 dist 下的准确路径。
- 本任务不修改 FooCrate，故不为无改动的 FooCrate 制造候选包；其 Devices 集成包由任务 006 单独产生。

## 4. 人工只读验收

实现完成后补充实际版本、菜单路径和截图。用户只在 `foobar-test` 手动安装候选：

1. 不接 iPod 启动，设备信息页显示无设备；没有假设备或写入命令。
2. 接入 Nano 4，等待真实读取完成；核对家族、容量/可用空间、只读状态、签名检查状态和曲目/playlist 数量。零曲目库应明确显示零。
3. 点击 Refresh，结果来自重新读取；页面保持响应。库名和内容仅本地正常显示，不进入脱敏诊断。
4. 关闭信息页，使用 Windows 资源管理器弹出，再重新连接；设备及时失效并取得新的挂载代次，旧 Library 不继续显示为在线。
5. 已有 5.5G 或其他设备方便时只读补充；型号证据不足应如实显示，不强行升级验证等级。
6. 服务/组件卸载与重新安装正常，FooCrate 原有功能不受影响。

不要求用户格式化、关闭磁盘模式、修改盘符或人为破坏数据库。异常路径先通过模拟验证。实机前后固定设备元数据/数据库的内容哈希对照仅在明确的只读验证范围内进行，不能用全量媒体重复读取代替权限与代码审计。

## 5. 当前已执行

- 2026-09-20：完成 fetch 和工作区状态核对；阅读全任务路线、当前任务、相关总规格、决策、架构、蓝图及固定源码证据。
- 新增 SPEC/EVIDENCE/VALIDATION，更新任务入口；本轮只修改文档。
- 规格准备阶段没有执行 C++、构建或设备访问；用户随后批准后的实施与阻断记录见第 6 节。
- 文档检查通过：本任务四份文档与任务入口均为严格 UTF-8、无 BOM、LF，无替换字符，本地 Markdown 链接全部存在；git diff --check 通过。

## 6. 2026-09-20 实施与三次构建阻断

用户明确回复“推进！没问题！”，批准本任务规格和既定实现/验证/交付范围。无需重新核对已批准的产品范围。

已写入但未经编译验证：

- `include/foopodbridge/core/device/device.h`：设备分类、候选输入、不可变快照、只读后端和发现生命周期接口；
- `src/core/device/device.cpp`：23 代际注册表、正向硬件匹配、传统库只读投影、路径约束、状态分类与异步发现；
- `src/core/device/windows_backend.cpp`：公开 Windows API 枚举/通知、磁盘到卷关系、只读文件打开与取消；
- `tests/device_tests.cpp`：识别、拒绝错误输入、签名、路径和移除晚到结果测试；
- `CMakeLists.txt`：真实 device 静态库与测试目标，保留当前版本号，未生成新候选。

这些代码是进行中的工作，不是实现完成。服务 ABI 扩展、Preferences 信息页、进一步身份/路径/生命周期审查、全量测试、Debug/Release 验证、开发部署与新候选包仍待完成。没有实机扫描、设备写入、FooCrate 修改或部署。

使用指定 Visual Studio bundled CMake/CTest，路径均存在。CMake configure 成功。三次 `foopodbridge_device_tests` Debug 构建均在 CL.exe 启动前失败：

1. 直接由 PowerShell 调用指定 CMake：MSB6001 / System.ArgumentException，环境字典重复键 `Path` 和 `PATH`。
2. Python 子进程先把环境变量键转为统一大写并去重，再调用相同 CMake：仍为相同错误。
3. Python 子进程去掉所有 PATH 变体，只保留一个 `Path` 后调用相同 CMake：仍为相同错误。

第三次之前的独立检查证明，单一 Path 的普通 PowerShell 子进程可读取 .NET ProcessStartInfo.EnvironmentVariables（71 项），但这没有解决 MSBuild 的实际编译任务环境。不能据此声称构建环境已修复，也不能把该错误算作源码编译结果。

根据根 AGENTS.md 三次停止协议，第三次失败后停止进一步构建尝试，保留现有成果。

恢复所需用户动作：允许针对本次 MSBuild 的 Path/PATH 冲突做专项诊断并恢复构建。建议仅检查构建进程的有效环境、MSBuild 节点复用和工具任务环境构造，得到证据后在隔离子进程中修复；不修改系统/用户全局环境，不安装或更换工具，不绕过编译/测试。诊断日志留在本地忽略的 build 目录，公开记录不包含完整环境变量内容或凭据。解除后继续原 005 范围，不重新请求规格批准。

## 7. 恢复构建与人工操作约束

- 用户随后明确授权“再试试，如果还不行，就换一种方法”。通过隔离子进程保留单一 Path、关闭 MSBuild 节点复用并使用单节点工具任务，编译器可正常启动；永久入口为 `scripts/build-local.py`。未修改系统或用户 PATH。
- 修正 Windows SDK 头文件/链接配置；恢复本地缓存中标准 Debug/Release 编译选项后，两种配置均完整构建成功。
- Debug/Release 各 11 项 CTest 全部通过，涵盖注册表、状态、路径、签名、曲库投影、模拟挂载生命周期、原生只读文件访问及既有数据库/合同回归。该结果不等于整个第 2 节矩阵或实机验证已通过。
- Release DLL 已部署到批准的 foobar-dev。曾启动开发实例，但未获取有效界面检查结果；用户按 Esc 停止电脑操作后，未继续 Computer Use。没有生成本任务的新候选包，不宣称加载、UI 或实机检查通过。
- 用户随后要求“不要使用computer-use，禁止使用computer-use，任何需要测试使用方法的，交给我，指导我操作（写进agents.md）”。已落实到根 AGENTS.md 和 PROJECT_RULES；此前自动应用检查授权被覆盖。
- 后续组件加载、界面、Refresh、关闭页面及设备插拔等使用测试均交给用户。Codex 提供准确实例、入口、单步动作、预期结果和反馈要求，等待结果再推进相关下一步；不使用脚本或其他工具绕过禁令。
- 首个人工检查：用户在 `D:\dev\foo\FooCrate\.local\foobar-dev` 中打开 Preferences → Tools → FooPodBridge，反馈页面是否存在及状态文字。不要求连接或修改设备；如果设备已连接，页面仅执行本任务已有的只读发现。

## 8. 用户截图与存档断点（2026-09-20）

用户按指导打开开发实例并提供 Preferences: FooPodBridge 截图，确认：

- Tools 下有 FooPodBridge 页面，组件页面加载成功；
- 显示 `No iPod devices detected.`，设备选择为空，显示只读说明；
- 右侧 Refresh 按钮被裁切，详情区域也延伸到页面右边界；这是待修复缺陷，不能记作完整 UI 验证通过。

用户随后说明没有携带连接线，明确要求“存档，提交必要提交的所有内容，然后 commit+push”。本次保存代码、测试、构建入口、规则与任务证据；不为了存档伪造实机结果，也不额外制作未经完整审计的候选包。用户截图仅保留上述文字证据，不提交临时截图、设备私有数据或本地构建日志。

当前状态：

- 源码/已部署开发 DLL 为 `0.1.0-beta.2`；它已用于用户手动查看，因此后续改变代码再交付须使用 `beta.3` 或更高未用版本，不覆盖之前的候选。
- Debug/Release 均已完整构建，各 11 项 CTest 全通过；源码边界检查通过。
- 本任务尚未生成 `.fb2k-component`；现有历史 dist 包不代表本次实现。
- Nano 4 实时发现、容量/Library/签名、Refresh、插拔、重连和退出释放尚未人工验证。没有新增 DeviceReadVerified 或 DeviceWriteVerified 结论。
- 任务保持“实现中”。缺少连接线仅阻断实机检查，不阻断后续布局修正与电脑侧验证；本次按用户要求暂停推进并保存。

下次继续顺序：

1. 修复 Preferences 页与宿主客户区尺寸适配，检查 Refresh 和详情控件完整可见；界面结果交给用户手动确认。
2. 对照 SPEC/自动矩阵补齐证据与覆盖，重点复查：家族注册项的 profile/来源信息、身份冲突、重解析点、通知引起的全局失效与多设备独立生命周期、数据库指纹、服务订阅/写入拒绝的运行时测试。当前测试通过不代表这些验收项均已完成。
3. 完成新版本 Debug/Release 验证、包内容/架构/依赖审计，生成独立候选包至 dist，并交给用户手动安装。
4. 连接线可用时逐步指导只读实机测试，最后由用户明确验收，才进入 006。

已验证的非交互构建入口（仓库根目录执行，Python 仅属开发工具）：

```powershell
C:\Python314\python.exe scripts/build-local.py --preset vs2022-x64
C:\Python314\python.exe scripts/build-local.py --build --preset debug
C:\Python314\python.exe scripts/build-local.py test --preset debug --output-on-failure
C:\Python314\python.exe scripts/build-local.py --build --preset release
C:\Python314\python.exe scripts/build-local.py test --preset release --output-on-failure
```

本轮发现旧本地 CMake 缓存的配置选项为空，已恢复 Debug `/Zi /Ob0 /Od /RTC1`、Release `/O2 /Ob2 /DNDEBUG`、RelWithDebInfo `/Zi /O2 /Ob1 /DNDEBUG` 和 MinSizeRel `/O1 /Ob1 /DNDEBUG` 后完成上述构建。缓存/SDK/编译产物属于本地环境，不提交 Git。后续复现时需确认这些标准配置有效，不能把空选项构建误记成标准 Debug/Release。
