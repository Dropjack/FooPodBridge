# 002-建立 x64 组件工程与服务合同

- 状态：已验收
- 日期：2026-08-28
- 前置任务：[`001-完成参考源码与许可证审计`](../001-完成参考源码与许可证审计/README.md) 已验收
- 对应路线：[`../TODO.md`](../TODO.md) 的任务 002
- 相关架构：[`../../docs/ARCHITECTURE.md`](../../docs/ARCHITECTURE.md)
- 来源边界：[`../../docs/REFERENCE_PROVENANCE.md`](../../docs/REFERENCE_PROVENANCE.md)
- 当前授权：用户于 2026-08-29 确认三项规格并授权实现、构建、测试、打包和仅限 `foobar-dev` 的开发烟测；仍不授权设备访问、`foobar-test` 自动部署或 C 盘操作

## 1. 任务目标

建立 FooPodBridge 后续所有能力长期使用的 Windows x64 工程底座、组件身份和跨组件服务合同。任务完成后会得到首个可安装候选 `FooPodBridge-0.1.0-beta.1.fb2k-component`：foobar2000 的 Components 页面能确认名称、版本和 x64 身份；FooCrate 能在编译期消费同一份公开合同；产品仍不显示假设备、空 Device Panel 或尚未实现的写入入口。

## 2. 本任务做什么

- 建立只支持 Windows 10/11、MSVC、x64、C++20 的正式 CMake 工程与 presets；
- 建立永久依赖方向：SDK 无关的 Core 边界、foobar2000 适配层、原创公开合同和自动测试；
- 从官方 foobar2000 SDK 2025-03-07 包取得允许的源码依赖并复核原始许可；
- 冻结新组件名称、文件名、版本、Windows 版本资源和全新 GUID；
- 建立不可变的服务 ABI v1、兼容性判断和 FooCrate 编译期消费者检查；
- 建立受约束的打包、包内容审计和仅限 `foobar-dev` 的开发部署脚本；
- 创建任务 001 要求的许可证全文和第三方声明。

## 3. 明确不做

- 不发现、读取、模拟或写入 iPod；不读取任何真实设备样本；
- 不实现 iTunesDB、hash58、媒体导入、事务、播放列表或 artwork；
- 不创建空 Device Panel、Default UI Element、FooCrate Devices 树或假快照；
- 不采用 dop-sdk 的旧 GUID/API，不复制 foo_dop Core，不加载 `iTunesCrypt.dll`；
- 不捆绑 foobar2000、Columns UI、Apple DLL、FooCrate DLL 或 SDK 预编译二进制；
- 不部署到 `foobar-test` 或 C 盘日常安装；候选由用户手动导入 `foobar-test`；
- 不修改只读 `Ref`，不把构建产物写入 FooCrate。

## 4. 中文程序逻辑

### 4.1 构建与依赖边界

1. 配置阶段先确认 Windows、MSVC 和 64 位指针；任一条件不满足就以明确错误停止。
2. 项目源码使用 C++20、UTF-8、动态 MSVC runtime、`/W4 /WX /permissive- /Zc:__cplusplus`；第三方 SDK 单独作为 SYSTEM 依赖编译，其既有警告不能降低项目源码的警告等级。
3. Core 的公开目标不包含 foobar2000、Columns UI、PFC 或 FooCrate 头文件。任务 002 只建立永久 target/依赖边界，不用空函数或假设备填充未来模块。
4. foobar 适配目标依赖 Core 公共边界、服务合同和官方 SDK；反向依赖在 CMake 配置与源文件扫描测试中拒绝。
5. Debug 与 Release 使用同一源码和 x64 ABI；全新构建树必须可重复配置、构建和测试。

### 4.2 服务合同 v1

1. 公开合同放在独立 `0BSD` 目录，使用全新命名空间、接口 GUID 和稳定整数类型，不继承 dop-sdk 的类名、GUID 或回调结构。
2. 跨 DLL 边界只使用 foobar2000 service 引用计数、固定宽度整数、GUID、只读接口和调用方提供的字符串输出；禁止传递 STL 容器、异常所有权、Core 私有类型或可由调用方修改的内部对象。
3. ABI major 对应不可变的接口布局和 GUID。已有 v1 虚函数不重排、不删除、不改变语义；需要新增不兼容能力时发布新接口/GUID，而不是让旧消费者猜测布局。
4. 服务发布组件版本、合同 major/minor、运行状态与受支持的扩展接口。任务 002 的真实运行状态是 `ReadyNoDeviceProvider`：表示组件和合同正常，但设备发现能力尚未由任务 005 提供；它不是假设备或错误。
5. FooCrate 查不到服务时保持原功能并隐藏 Devices；major 不兼容时拒绝使用设备能力；minor/扩展未知时只使用已查询成功的接口。缺失服务和版本不兼容都不能使 FooCrate 加载失败。
6. `DeviceSnapshot`、`CapabilityMatrix`、`OperationRequest`、`OperationPlan`、`OperationHandle` 与事件订阅在本任务冻结为版本化接口族和生命周期规则，但任务 002 不制造其实例。后续任务只能实现这些抽象或通过新扩展接口演进，不能把数据库偏移或设备文件写入暴露给 UI。
7. 快照与计划只读；异步回调明确线程归属；订阅和操作句柄可取消且晚到回调可安全丢弃。真实线程与设备行为分别在任务 005 和 007 验证。

### 4.3 组件身份与包

1. 组件稳定名称为 `FooPodBridge`，DLL 为 `foo_pod_bridge.dll`，首个候选可见版本为 `0.1.0-beta.1`，Windows 数字 FileVersion 映射为 `0.1.0.0`。
2. Components 页面只表明组件身份、服务合同版本和当前无设备 provider；不注册无法完成的菜单、设置或 UI。
3. 打包脚本只接受本仓库 Release x64 DLL、固定输出目录 `dist` 和精确 SemVer；拒绝 Debug DLL、错误文件名、仓库外输入与覆盖既有候选。
4. 包内批准内容为 `foo_pod_bridge.dll`、FooPodBridge 许可证和适用的第三方声明；不包含 SDK 源码/二进制、FooCrate DLL、测试文件或用户数据。
5. 部署脚本只接受 `dev`，解析后的目标必须位于 `D:\dev\foo\FooCrate\.local\foobar-dev`；不存在任何 C 盘或自动回退路径。

### 4.4 失败行为

- SDK 文件、许可证、版本或 SHA-256 不匹配：配置前停止，不用 Ref/旧 SDK 回退；
- 工具链不符合仓库固定路径：报告环境不一致，不调用 PATH 上的 `cmake`/`ctest`；
- 服务缺失或 ABI major 不兼容：消费者报告 unavailable，FooCrate 其余功能继续工作；
- 包含非批准条目、非 x64 DLL、错误版本或禁用字符串/导入：打包失败且不生成候选；
- 开发部署目标无法证明是 `foobar-dev`：拒绝复制；
- foobar2000 加载失败：保留构建包和诊断，不改用 `foobar-test` 或日常安装试错。

## 5. 计划永久产物

- `CMakeLists.txt`、`CMakePresets.json` 与 `cmake/`：x64 构建和依赖定义；
- `third_party/`：官方 SDK 的可构建源码、固定来源说明与原许可证，不含预编译 DLL/lib；
- `include/foopodbridge/`：原创 `0BSD` 服务合同 v1；
- `src/core/`：SDK 无关的永久 target 边界；
- `src/foobar/`：组件身份、版本资源和服务实现；
- `tests/`：身份、ABI、依赖方向、消费者兼容和包审计测试；
- `scripts/package-component.ps1`、`scripts/deploy-component.ps1`：受约束打包与开发部署；
- `LICENSES/` 与 `THIRD_PARTY_NOTICES.md`：任务 001 冻结的许可证材料；
- FooCrate 仓库中的公开合同快照/编译测试：只证明可消费，不添加 Devices UI；
- `dist/FooPodBridge-0.1.0-beta.1.fb2k-component`：本任务人工验收候选。

## 6. 实施步骤与检查点

### 步骤 1：解除环境门槛

- 输入：用户提供的官方 SDK 包与解压目录、固定 SHA-256、WinRAR 7.13、已核实的 Visual Studio Build Tools CMake/CTest 绝对路径；
- 动作：复核 SDK 包内 `sdk-license.txt`，核对工具版本与绝对路径；
- 产物：本任务中的环境和许可证证据；
- 通过标准：许可证与任务 001 结论一致，仓库记录的两个固定 CMake 路径存在；否则停止。
- 状态：已完成。官方包哈希、WinRAR 完整性测试、674 个解压文件、必要 SDK 入口和许可证逐字节对照均通过。

### 步骤 2：冻结身份、合同与测试

- 输入：已批准架构、`0BSD` 合同边界和现代 SDK；
- 动作：先写合同/身份测试，再实现公开头、GUID、组件服务，并用临时 FooCrate 外部消费者验证独立可编译性；
- 产物：不可变 v1 ABI 与缺失/兼容/不兼容状态测试；
- 通过标准：没有 STL/Core 私有类型越过 DLL，FooCrate 缺失服务时仍可编译并安全降级。

### 步骤 3：建立可重复 x64 工程

- 输入：合同和官方 SDK；
- 动作：建立 targets、presets、警告策略、版本资源和依赖方向检查；
- 产物：全新构建树中的 Debug/Release DLL 与测试；
- 通过标准：两种配置构建成功、项目源码零警告、相关测试全部通过。

### 步骤 4：打包与开发加载

- 输入：Release x64 DLL 和批准许可证；
- 动作：生成新候选、审计 ZIP 条目/PE 架构/版本/导入，再只部署到 `foobar-dev` 做加载烟测；
- 产物：`dist/FooPodBridge-0.1.0-beta.1.fb2k-component` 与审计记录；
- 通过标准：Components 页面身份正确，没有 UI/假设备，FooCrate 原功能不回归。

### 步骤 5：用户人工验收

- 用户在 `foobar-test` 手动导入候选；
- 确认 Components 页面显示 `FooPodBridge 0.1.0-beta.1`；
- 确认 FooCrate 正常加载且没有出现假 Devices、空面板或无效菜单；
- 卸载 FooPodBridge 并重启，确认 FooCrate 其余功能不受影响；
- 用户明确确认后，任务 002 才标记“已验收”并进入任务 003。

## 7. 自动验证清单

- CMake 配置拒绝非 Windows、非 MSVC 和非 x64；
- Debug/Release 构建，CTest 全量通过；
- 组件名、DLL 名、SemVer、Windows FileVersion、组件 GUID 与服务 GUID 自动锁定；
- v1 接口签名/布局守卫、GUID 唯一性和 major/minor 兼容矩阵测试；
- Core include/link 依赖扫描，禁止 foobar/Columns UI/FooCrate；
- [FooCrate 外部消费者验证](../../docs/FOOCRATE_CONSUMER_VALIDATION.md)覆盖服务缺失、v1 可用与 major 不兼容；验证夹具不作为 FooCrate 永久依赖；
- PE machine 为 AMD64，禁止导入 `iTunesCrypt`/Apple Mobile Device，禁止 x86 产物；
- 组件包条目白名单、DLL 哈希一致性、许可证与第三方声明存在；
- 部署脚本目标路径保护测试；
- 项目文本严格 UTF-8、无 BOM、LF，本地 Markdown 链接有效。

## 8. 当前环境证据

2026-08-28 的只读探测结果：

- 机器已有 WinRAR 7.13，可通过绝对路径 `C:\Program Files\WinRAR\WinRAR.exe` 读取 LZMA `.7z`；不需要安装 7-Zip，也不需要修改 PATH；
- 仓库原先规定的 Visual Studio Community CMake 与 CTest 绝对路径均不存在；`vswhere` 返回 `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`；
- 该 Build Tools 安装内存在完整的 CMake、CTest 与 Ninja；CMake/CTest 文件版本均为 `3.31.6-msvc6`。下班保存点已把 `AGENTS.md` 固定路径修正为这些实际存在的绝对路径；
- 按 `AGENTS.md` 不允许静默改用其他路径或 PATH 上的裸 `cmake`/`ctest`；
- FooCrate 仓库当前为 clean，现有 CMake/SDK 结构只作合同接入设计证据，本任务尚未修改 FooCrate。
- 用户提供的 staging 位于 `D:\dev\foo\FooPodBridge\.local\downloads`，在正式仓库之外，不会进入 Git；
- `SDK-2025-03-07.7z` SHA-256 为 `CCDA3C5840E66E0E28A7E4FE36407C4E78581AA30C40C362A188FCBAAE799A3E`，与固定值匹配；
- WinRAR 对压缩包执行只读测试返回 0；解压目录有 674 个文件，必要的 `sdk-license.txt`、`sdk-readme.html`、`foobar2000.h`、`component_client.cpp` 和 `pfc.h` 均存在；
- 官方 `sdk-license.txt` SHA-256 为 `2AA8AF2F2A0CCE2DCE4C2A4F422BCBD1752DAB7D1E1B973981B77C971B0B8A32`，与 FooCrate 当前 2025 SDK 副本逐字节一致。

Build Tools 和官方 SDK 许可证门槛均已解除，不需要安装新的系统工具。当前只剩用户对本任务服务/包范围的规格确认；确认前不进入 C++、构建、测试和打包。

## 9. 当前用户检查点

只需核对以下三点，不必先通读全部技术细节：

1. 首个候选版本、无假设备/无空 UI 的组件身份范围；
2. 原创 `0BSD` 服务 ABI v1、不可变接口/GUID 和 FooCrate 安全降级规则；
3. 包内容、只部署 `foobar-dev`、由用户手动导入 `foobar-test` 的边界；
用户已于 2026-08-29 确认上述三点；任务进入实现，SDK 和工具环境已经通过，不再等待额外安装。

## 10. 验证记录

- 2026-08-28：任务 001 经用户明确验收，任务索引切换到本任务；
- 2026-08-28：严格读取项目规则、路线、架构、产品规格、来源审计和 FooCrate 仓库规则；
- 2026-08-28：只读确认缺少规定的 Visual Studio Community CMake/CTest 与 7-Zip，未执行下载、安装、构建、部署或设备访问。
- 2026-08-28：进一步确认现有 Visual Studio Build Tools 自带 CMake/CTest `3.31.6-msvc6` 与 Ninja；下班保存点已据此更新固定路径，不重复安装 Community。
- 2026-08-28：用户要求建立下班 Git 保存点并推进显然的非产品确认；据此把 `AGENTS.md` 固定工具路径修正为已经核实的 Build Tools 路径。用户明确表示尚未仔细阅读任务 002 文档，因此服务规格仍保持“规格核对中”，没有伪记为已批准。
- 2026-08-29：用户下载并解压官方 SDK；只读确认包 SHA-256 匹配、WinRAR 完整性测试通过、674 个文件与必要入口存在，包内许可证与 FooCrate 当前官方 SDK 副本逐字节一致。7-Zip 安装门槛取消。
- 2026-08-29：用户明确确认三项简明规格；任务状态改为“实现中”。

## 11. 当前构建阻断

2026-08-29，Debug 构建同一操作达到三次失败上限，已停止继续构建：

1. 第一次：官方 SDK `shared/filedialogs_vista.cpp` 需要当前 Build Tools 未安装的 ATL `atlbase.h`。任务 002 没有 UI，决定不为身份组件增加 ATL，而是缩小源码构建边界。
2. 第二次：排除 ATL 文件后，旧 `filedialogs.cpp` 仍引用对应 Vista 实现，导致四个未解析外部符号；随后把整对文件对话框 UI 源从任务 002 的最小 `shared` 目标排除。
3. 第三次：最小 `shared`、PFC、foobar SDK、Core、合同和 component client 均构建成功；`component.cpp` 编译失败。错误收敛为：
   - `timeGetTime`、`interface`、`IUnknown` 和 `IDataObject` 在包含 foobar SDK 前没有由 Windows 前置头声明；
   - `service_factory_single_t` 会通过 SDK 包装类继承实现类型，因此实现类不能声明为 `final`。

最佳恢复方式已经明确，不需要安装新依赖或降低质量：在项目自有适配头中先引入 Windows multimedia/OLE 声明，保持 Core 和公开合同边界不变；移除 service 实现类的 `final`；然后重新配置并开始一轮新的 Debug 构建尝试。

恢复所需用户动作：明确回复“允许按任务 002 阻断记录修正并重新构建”。在收到该确认前不再修改实现或运行构建。

2026-08-29，用户已给出上述明确授权。本轮按最佳恢复方式补齐项目适配层的 Windows multimedia/OLE 前置声明，并移除 service 实现类上与 SDK factory 包装机制冲突的 `final`；新的 Debug 构建从第一次尝试重新计数。

- 新一轮第一次 Debug 构建：原阻断已解除，组件源码成功编译；链接阶段发现只含 `__declspec(selectany)` GUID 定义的合同静态库对象未被抽取，唯一错误为 `service_v1::class_guid` 未解析。按官方 SDK 自身 `guids.cpp` 的模式，把合同 GUID 翻译单元改为强定义后进入第二次构建。
- 新一轮第二次 Debug 构建：组件及其依赖成功生成 `foo_pod_bridge.dll`；失败转移到测试目标。公开合同头对独立消费者尚不自包含 Windows multimedia/OLE 声明，身份/合同测试还把编译期常量写成普通条件，在 `/WX` 下触发 C4127。合同头补齐同一前置声明，纯常量检查改为 `static_assert` 后进入第三次构建。
- 新一轮第三次 Debug 构建：合同头已经提供 multimedia/OLE 声明，但在未定义 `WIN32_LEAN_AND_MEAN` 的合同库翻译单元中，`windows.h` 先引入旧 `winsock.h`，随后 foobar2000 SDK/PFC 引入 `WinSock2.h`，造成 `sockaddr`、`fd_set` 等重复定义。按三次上限停止；本次没有运行 CTest、Release、打包或部署。

第二轮阻断的最佳恢复方式：让公开合同头在任何消费者编译选项下都采用 Windows SDK 兼容的自包含 include 次序（在 `windows.h` 前引入 `WinSock2.h`，再引入 multimedia/OLE 声明），而不是要求 FooCrate 等消费者依赖隐含的 CMake 宏；随后重新配置并从新的 Debug 构建第一次尝试开始计数。

恢复所需用户动作：明确回复“允许按任务 002 第二轮阻断记录调整公开合同头的 Windows include 次序并重新构建”。

2026-08-29，用户已给出上述明确授权。公开合同头现先引入 `WinSock2.h`，再引入 `windows.h` 与 multimedia/OLE 声明；新的 Debug 构建从第一次尝试重新计数。

## 12. 第二轮解除后的验证与当前阻断

- 新一轮第一次 Debug 构建通过；Core、合同、SDK、组件 DLL 和全部测试程序均无新增警告地生成。
- Debug CTest 第二次调用通过 4/4；第一次只因从构建目录调用 `ctest --preset` 而没有找到源码根的 preset，未执行测试。
- Release 完整构建与增量确认通过，Release CTest 4/4 通过。
- 打包第一次因净化子进程没有自动加载 `Get-FileHash` 失败，脚本清理了未完成包；改用 .NET SHA-256 后第二次成功生成 `dist/FooPodBridge-0.1.0-beta.1.fb2k-component`。
- 包审计确认仅含 `foo_pod_bridge.dll`、`LICENSE.txt`、`THIRD_PARTY_NOTICES.txt`；DLL 为 AMD64，ProductVersion 为 `0.1.0-beta.1`，包内/Release DLL SHA-256 均为 `881B74A31A1E5A3FB703F8D5DD6A4CA0B86D6DB0D63B8BBECD3E32668352C32E`，包 SHA-256 为 `4C204B6F13E35B5512ACE60ADF7E0AF13633C806EA3DBBFD5F37CBF8FEB3810A`；导入表与字符串扫描不含禁用 Apple/`iTunesCrypt` 依赖。
- 部署验证第一次发现仓库台账仍写不存在的旧根 `D:\Dev\FooCrate`；只读证明实际工作区为 `D:\dev\foo\FooCrate` 后同步规则和白名单，第二次验证通过。Release DLL 仅部署到 `foobar-dev`，隐藏加载烟测确认进程从准确目录加载 `foo_pod_bridge.dll`，并通过 foobar2000 自身 `/exit` 正常退出。

FooCrate 编译消费者检查开始前，已严格读取其仓库规则和当前任务；工作区起初 clean 且 `main` 与 `origin/main` 一致。其 Community CMake/CTest 台账已失效，实际 Build Tools 17.14.37、CMake/CTest 3.31.6-msvc6 和 Ninja 均经文件证据复核；未安装新工具。验证所需的临时路径记录没有保留到 FooCrate 公共仓库。

当前阻断是复制 0BSD 服务合同快照的同一操作达到三次失败上限：

1. 第一次使用执行环境不存在的 `atob` 解码，未写入文件；
2. 第二次成功建立头文件、GUID 源和许可证快照，但工具输出各多追加一个末尾 LF，三个 SHA-256 均与来源相差；
3. 第三次补丁按错误的许可证单行排版匹配，验证失败且整体未应用。

当前三份快照内容已存在且只比来源多一个末尾 LF；尚未加入 FooCrate CMake/测试，也未构建或打包 FooCrate。最佳恢复方式是按实际多行许可证尾部精确移除三个额外 LF，复核来源哈希完全一致，再继续建立缺失/v1/major 不兼容编译测试。

恢复所需用户动作：明确回复“允许按任务 002 第三轮阻断记录规范化 FooCrate 合同快照末尾换行并继续编译检查”。

2026-08-29，用户已给出上述明确授权，第三轮阻断现已解除：

- 精确移除 FooCrate 三份快照各自多出的一个末尾 LF 后，头文件、GUID 定义和 0BSD 许可证与 FooPodBridge 来源逐字节一致；SHA-256 分别为 `A39D48C57E53C6C2AA2BE8E75738DFDB76DF88CCC9E0D9D1DD96772090F03DC6`、`C5B4AB4B9B4044930A725DD1CAB0061AD8ADF77826A9171C11EB49CA785F06E0` 和 `D734C241CFF10A35242A1BF8320C85B1CEAA04DDB935496608198700A7C02EEE`。
- 临时 FooCrate 独立测试目标只编译合同消费者和 GUID 定义，不链接进现有 `foo_crate.dll`；CMake 配置阶段用上述哈希拒绝快照静默漂移。
- FooCrate 原 preset 构建目录含来自 `D:\Dev\Refrain` 的旧缓存，因此保留不动，改用隔离目录 `build/foopodbridge-contract`。配置前两次分别被旧缓存与进程环境重复的 `Path`/`PATH` 阻断；第三次以大小写无关去重后的子进程环境配置成功。
- Debug 构建第一次因外层日志管道超时而没有形成可验证产物；第二次明确发现新增测试目标缺少 `/EHsc`，在 `/WX` 下触发标准库 C4530；补齐开关后第三次构建成功。Debug CTest `foopodbridge_contract_consumer` 通过 1/1。
- Release 第一次构建成功，Release CTest 同一测试通过 1/1。测试覆盖 ABI 1.0 常量、入口服务 GUID、服务缺失、兼容 v1 和未来 major 不兼容；后两种不可用状态都明确隐藏设备 UI。
- 本轮没有构建、覆盖、打包或部署 `foo_crate.dll`，没有新增网络或设备代码，也没有访问设备、`foobar-test` 或 C 盘日常安装。
- 最终 `foobar-dev` 隐藏烟测在同一进程中从批准的 `profile/user-components-x64` 目录同时观察到 `foo_pod_bridge.dll` 与现有 `foo_crate.dll`；没有模块读取错误，并通过 foobar2000 自身 `/exit` 正常退出。
- 用户随后决定不把提前验证用的合同快照和测试推入公共 FooCrate 仓库。未推送的 FooCrate 本地提交已销毁，仓库精确恢复到其 `origin/main`；完整验证方法、哈希与结果改由 [`docs/FOOCRATE_CONSUMER_VALIDATION.md`](../../docs/FOOCRATE_CONSUMER_VALIDATION.md)永久保存。该清理不改变已经取得的构建/共存证据，也进一步明确 FooCrate 只是可选消费者。

任务 002 的自动实现门槛现已全部通过，状态转为“实现完成待验收”。下一步只由用户在 `foobar-test` 手动导入候选并完成第 6 节步骤 5；未授权自动部署到该实例。

## 13. 用户人工验收

2026-08-29，用户在 `foobar-test` 手动检查候选并反馈：

- 安装时弹出 LGPL 许可证正文；核对确认该内容来自候选包批准的 `LICENSE.txt`，不是功能界面或联网提示；
- 安装后没有假设备入口，也没有空白或无效的 Devices 界面，符合任务 002 明确的无 UI 范围；
- 用户未观察到额外视觉界面，这是预期行为；共存性由此前 `foobar-dev` 同一进程同时加载准确路径下 `foo_crate.dll` 和 `foo_pod_bridge.dll` 的自动证据补足；
- 卸载正常。

人工反馈与自动证据合并覆盖安装、无假 UI、双组件共存和卸载降级标准。任务 002 标记为“已验收”，下一项为任务 003；本次验收不构成任务 003 的实现授权或任何实机访问授权。
