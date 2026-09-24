## 2026-09-24 beta.12 刷新后曲库消失的修复候选

用户报告 Classic 保持连接、Windows 资源管理器仍可打开卷，但刷新后第一条设备变为 `Format pending`、Library 不可用，第二条为 `Not mounted`。只读检查当前 FAT32 卷的 `iTunesDB`：4,109,656 字节，`mhbd` header 244、版本 115、1862 条曲目、全部 `mhit` header 624；未观察到 `iTunesCDB` 或 `iTunes Library.itlp`。旧 Hash58 Reader 在首条曲目拒绝 624 字节 header；使用无写能力的 PreserveOnly profile 可解析并验证 1862 条。Ref 原版 `itunesdb.cpp` 读取到偏移 620，支持该 624 字节变体。没有复制数据库或将私有内容写入仓库。

新增纯合成 624 字节回归先复现拒绝，再允许 Hash58 Reader 只读解析并标记 opaque 依赖；语义重写仍明确拒绝，未观察到的 608 字节变体继续拒绝。更新后的 Reader 对当前设备数据库只读解析及 Validator 均通过，返回 1862 首。另对 Windows PnP 做只读查询，确认同一已挂载磁盘父链有两层 Apple ID；挂载别名修正详见任务 005。使用新 Release Core 的非交互 Windows 发现诊断直接读取当前设备，结果为 1 台已挂载、`Ready - read-only`、1862 首；实机 UI 尚待复验。

beta.12 Debug/Release 全构建成功，各 13/13 CTest 通过。交付包 `dist/FooPodBridge-0.1.0-beta.12.fb2k-component` 为 183,458 字节，SHA-256 `9BB4840861BB2700584E9B00E8D6CBD02E108E9865A7FAB251CBCC34536EAE73`；ZIP CRC、x64、ProductVersion、包内仅 `foo_pod_bridge.dll`/`LICENSE.txt`/`THIRD_PARTY_NOTICES.txt` 且 DLL 与 Release 一致。没有部署、启动或操作 foobar2000，也没有设备写入/恢复。下一人工检查：用户在 `foobar-test` 手动安装 beta.12，保持该 Classic 可访问，刷新一次，核对仅一条设备且 Library 1862 首再次出现；之后再单独检查电脑侧恢复状态。

用户随后明确反馈 beta.12 的 Refresh 正常、重复设备项消失；记录为该 Classic 的刷新及单设备显示检查通过。本轮未附截图，不能从反馈单独确认恢复资料字段或完整备份门禁。用户另将 `iPod_Control` 副本置于仓库外的 `D:/Dev/FooPodBridge/iPod_Control`，授权只读使用。初步盘点：`Music` 下 1862 文件；完整 `iPod_Control` 树在副本与当前设备各有 1881 个文件，路径及大小全部匹配；副本与当前设备的 `iTunesDB` SHA-256 相同，Reader/Validator 离线读取 1862 首。`Device/SysInfo` 为 0 字节，`SysInfoExtended` 缺失；这份副本不会自动补齐签名输入。尚未逐文件比较全部约 26 GiB 内容，也未完成稳定身份/恢复门禁绑定，因此只称为备份候选，不称已验证可恢复基线。原始副本不进入 Git 或发布包。

进一步使用同一 Reader/Validator 对副本数据库逐条核对音乐路径：1862 条引用对应 1862 个不同的非空普通文件，不安全路径、缺失文件、非普通文件、重复引用和空文件均为 0。此检查只读副本，没有逐字节比对全部音乐，也没有修改设备或副本。

随后新增只读卷入口和独立内容审计：入口支持当前 Windows 可访问的可移动卷根，全部创建、追加、Flush、改名和删除调用直接拒绝；审计按完整 `iPod_Control` 树逐文件比较设备与外部副本的大小和 SHA-256，期间定期核对同一物理设备仍映射到当前卷，结束时重新枚举两端文件树并再次核对设备映射。对当前 Classic 与 `D:/Dev/FooPodBridge/iPod_Control` 执行后正常返回：1881/1881 文件、26,102,223,051 字节内容一致，缺失、额外和哈希差异均为 0。设备与副本均通过只读入口访问，未写入任何文件。该单次内容审计不产生 `baseline_proof`，不能替代当前任务授权、完整签名输入、恢复方案和首次实机写入门禁。

只读入口的电脑目录回归覆盖读取成功、五种修改操作全部拒绝、内容不同和挂载代次变化时审计失败；当前可移动卷根的只读枚举实际返回 1881 文件。最终 x64 Debug/Release 完整构建通过，各 13/13 CTest 通过；本轮没有需要用户安装的组件行为变更，未生成或交付新候选包。

## 2026-09-24 beta.11 无设备人工检查

用户在 `foobar-test` 手动安装候选后回复“一切正常”，并提供 FooPodBridge Preferences 与 FooCrate 主界面截图。设置页正常显示 `No iPod devices detected.`，FooCrate 保持普通播放列表、歌曲信息和歌词界面；此时未连接 iPod。记录为 beta.11 无设备加载及设置页检查通过。截图未显示组件版本号，也不证明实机读取、恢复或写入；任务 007 继续实现中。下一检查点为同一台 Classic 的只读连接状态。

## 2026-09-24 Classic 播放与设备列表反馈

用户随后接入 Classic，反馈“能放歌了！显示了！”。FooCrate 截图显示设备 Library 1862 首、`Space Song` 正在播放、专用 `FooPodBridge playback` 列表含 1 首；这支持本次实机只读浏览和单曲播放通过。截图也显示第二条同名 Classic，005 的 DUP-001 仍未解决。此图未包含 FooPodBridge Preferences 的完整设备状态，因此尚不能判定 beta.11 的电脑侧恢复资料关联和签名验证暂缓显示是否正确；下一检查点只读取该状态页。没有实机写入或恢复授权。

## 2026-09-24 缺少签名输入时的只读恢复资料

Classic 的物理身份可用于定位私有恢复仓库，但当前实机没有提供 hash58 签名输入。新增合成回归先在旧实现上复现失败：一份已登记备份和一个已签名 LKG 快照存在时，清除签名输入使仓库查询错误地返回“无法检查”。修正后只读查询仍可报告事务记录和已登记备份，数据库快照/LKG 明确标为验证暂缓，数量不冒充零个有效恢复点；补回签名输入后同一快照通过验证。恢复执行及实机写入仍未开放。

本机原定 Visual Studio Build Tools 路径已不存在；按任务 002 的固定工具链原则核实了现有 Visual Studio Community 的 CMake/CTest 绝对路径和 `3.31.6-msvc6` 版本，并同步 `AGENTS.md` 与 `scripts/build-local.py`。没有使用 PATH 上的裸工具或安装新依赖。

Debug/Release 完整构建和两配置各 13/13 CTest 通过。候选包 `dist/FooPodBridge-0.1.0-beta.11.fb2k-component` 为 181,627 字节，SHA-256 `92AF326B66A075D5303868D181DCFBF3339B0E6617BF92D301F5E465268F5C49`；包内仅 x64 `foo_pod_bridge.dll`、`LICENSE.txt` 和 `THIRD_PARTY_NOTICES.txt`，ProductVersion 为 `0.1.0-beta.11`。本轮没有启动应用、接入或写入 iPod；组件加载、无设备设置页和后续 Classic 只读状态仍待用户逐项检查。任务 007 继续实现中。

## 2026-09-24 身份来源调整

实机 Classic 的只读截图显示设备卷和数据库可读，但 `SysInfo/FireWire GUID` 没有提供 hash58 签名输入。恢复资料定位已改为使用经过设备正向识别和映射校验的物理设备身份哈希；它只用于查找私有恢复记录，不代表签名验证，也不授权写入。这样在身份字段缺失时仍可区分“可定位恢复资料”和“可写数据库身份”。该改动通过 Debug 设备发现测试；完整 beta.9 回归结果如下。

## 2026-09-24 实机只读检查：Classic

用户在指定 `foobar-test` 中连接一台约 119 GiB 的 iPod Classic。设置页显示 `Ready - read-only`、`TraditionalHash58`、容量 119.00 GiB / 可用 94.67 GiB，读取到 1862 首曲目（Music 1834、Audiobooks 27、Other 1），播放列表 2 个普通、6 个 Smart、1 个 Master；FooCrate Devices 也显示对应 Library 与播放列表。Pending/Invalid recovery records 均为 0，说明本次只读发现和数据库读取成功。

该设备代际仍未知，且页面显示 `Identity: Incomplete; current mount only`、`Signature: Not checked (identity unavailable)`、`Stable identity unavailable`。这不是数据库读取失败；设备卷的 SysInfo/FireWire GUID 没提供当前实现所需的稳定签名输入。因此本次证据等级为实机只读读取，继续禁止写入和恢复关联，不能由截图授权实机写入。下一步只修复身份来源/证据记录，再重新执行只读检查；不伪造 hash58 输入。

# 007 验证记录

## 2026-09-23 自动关联实现

服务层现在在后台扫描 foobar 配置目录下的私有 `FooPodBridge/recovery` 仓库。设备完整签名身份经版本化 SHA-256 生成内部仓库键；盘符、显示 token、挂载代次和用户路径都不作为永久键。找到的事务 journal、快照、Last Known Good 和已登记外部备份只读汇总到当前设备状态；没有仓库不会自动创建目录，身份冲突/不完整、代次过期、损坏记录均不关联或明确标为无效。扫描不恢复、不写设备、不把旧备份登记当作当次基线验证。

新增合成测试覆盖重连换盘符、不同身份隔离、重复备份登记、过期基线拒绝、损坏 journal/快照、取消扫描、无写入扫描，以及服务发现结果中的快照/备份数量。Debug CTest 13/13 通过（6.41 秒）。

Release 构建及 CTest 13/13 通过（3.49 秒）。beta.8 包审计仅含 x64 foo_pod_bridge.dll、LICENSE.txt、THIRD_PARTY_NOTICES.txt；SHA-256：CECCC975D456BC6BB08944D81B297EC0FB6DA770D6B2BB242BCAF5DC4D29DE9B。

人工待验证：只在 `D:/dev/foo/FooCrate/.local/foobar-test` 手动导入 `dist/FooPodBridge-0.1.0-beta.8.fb2k-component`，重启并打开 Preferences → Tools → FooPodBridge，先检查不接设备时页面正常。尚未执行应用加载或实机验证。本轮未部署、提交、推送或修改 Ref/FooCrate。

## 2026-09-23 beta.7 原版行为对齐

已按 REFERENCE_RECOVERY.md 核对本地 Ref。移除自行新增的目录恢复表单及入口，保留事务核心、只读恢复发现、基线和离线恢复测试。beta.5/beta.6 相关入口检查仅为历史记录，后续不再执行目录表单验收。用户后续截图显示 beta.6 入口完整，但不构成恢复操作验收。

Debug/Release 构建通过；CTest 各 13/13 通过，分别为 6.02 秒和 3.18 秒。包为 `dist/FooPodBridge-0.1.0-beta.7.fb2k-component`；SHA-256：`F36C0361C518F8A671BB383DF13C78BAF2F692952D49C370D30F5ED1736C54B8`。包审计为 x64，仅包含 foo_pod_bridge.dll、LICENSE.txt、THIRD_PARTY_NOTICES.txt。

当前人工检查已通过：用户按 beta.7 检查点反馈“没问题了”，并提供 Preferences → Tools → FooPodBridge 截图；设置页显示正常，Directory recovery 按钮已移除，当前显示未发现 iPod。此确认仅覆盖本次组件设置页检查，不代表恢复操作或实机写入验收。本轮未运行应用、部署、提交、推送或修改 Ref/FooCrate。

## beta.6 布局修正

用户截图确认 beta.5 目录恢复窗口能完整打开，但 Preferences 页入口按钮右侧被裁切。根因是新增按钮未参加 layout() 的客户区动态布局；beta.6 将其宽度映射为 110 DLU，并按客户区右边距定位、限制最大宽度。没有改动恢复业务逻辑。Debug/Release 构建通过，两配置的组件身份和源码边界检查通过；实际布局仍待用户检查。

新包 `dist/FooPodBridge-0.1.0-beta.6.fb2k-component`，包审计确认 x64 和版本；SHA-256：`399745CFCE5CA30F5A4F149AF6D96EF1BA52E7095265F11C0C40B5FF3D34672E`。旧 beta.5 保留。

## 2026-09-23 恢复集成

- Windows 只读发现检查 journal/完成标记，区分待恢复、已完成和损坏；原生目录测试覆盖损坏记录保留、有效待恢复记录和不修改目录。
- Core 枚举完整 iPod_Control 文件树，比较相对路径、大小和 SHA-256，前后复核内容和目录集合；绑定任务、身份、挂载代次、能力版本与备份目录。重叠目录、漏备份和过期代次拒绝。
- 离线恢复会话读取设备副本及电脑仓库记录、验证 LKG，并复用事务引擎。测试实际提交一个不同的 LKG 数据库，并模拟提交前中断，再从会话发现和恢复，校验恢复字节。
- 设置页新增目录副本恢复入口，后台操作和关闭取消；当前仅 NTFS 普通目录副本，不提供真实卷写入。组件 UI、加载、生命周期的人工检查仍未执行。
- 任务 007 保持实现中：真实卷身份/备份适配、活动任务授权提供器、FooCrate 可消费的恢复合同仍不能由这轮离线能力替代。
- 旧包 beta.3/beta.4 保留。新候选 beta.5 独立打包，不部署，不提交/推送。
- 最终 Debug/Release 全构建通过，CTest 各 13/13（6.16 秒 / 3.22 秒）；既有 238 个提交中断点与 970 次恢复中断回归继续通过。
- beta.5 包审计通过：x64 DLL、产品版本 0.1.0-beta.5，包内仅 foo_pod_bridge.dll、LICENSE.txt、THIRD_PARTY_NOTICES.txt。
- 包 SHA-256：`B804429D86FE15732D932492B31730504D0EBDA4CEA61B12FC74F6D745EB2AC8`。
- DLL SHA-256：`17C446ECB583B5E336C59C56BBA1E85531FBCA32911B2372478C24BB498A4EB9`。

### beta.5 人工检查点（历史记录，beta.7 起停用）

不连接 iPod。只在 `D:/dev/foo/FooCrate/.local/foobar-test` 手动导入本仓库 `dist/FooPodBridge-0.1.0-beta.5.fb2k-component`，重启后打开 Preferences → Tools → FooPodBridge → Directory recovery。预期版本为 beta.5，能打开目录恢复窗口且说明真实 iPod 写入禁用。暂不填写路径或执行恢复；反馈能否打开、窗口是否完整显示及任何错误。通过后再逐项指导目录 fixture 恢复、取消与关闭检查。

2026-09-22：本轮无 iPod 的实现与测试完成。没有实机、UI 或应用加载验证；不得标记整个任务已验收。

## 构建与回归

- 使用仓库 scripts/build-local.py 调用指定 Visual Studio CMake/CTest，未回退到 PATH 中未知工具。
- x64 Debug、Release ALL_BUILD 均成功，自有 transaction 代码使用 /W4 /WX，无新增警告。
- Debug：13/13 CTest 通过；Release：13/13 CTest 通过。最后输出分别为 5.53 秒与 2.85 秒。
- 既有 device、数据库/hash58、私有 fixture、服务合同、组件身份和源码边界测试全部通过。未启动 foobar2000。

## 新增验证

transaction_core 使用仓库构建目录、Windows 文件 API 和真实 Reader/Writer 生成的合成 iTunesDB，验证正常导入、删除、执行前取消、恢复幂等、历史事务拒绝、自动前/后快照、最近 10 份 + LKG/活动恢复保护、源/备份清单比较和路径拒绝。音频是仅用于文件传输校验的合成字节，不称为可播放 MP3。

transaction_faults 在替换文件系统上运行相同 Core 和真实数据库验证器：

- 三类操作：已有库导入、DB-first 删除、显式初始化。
- 238 个文件变更前/后中断点，包含创建、追加、Flush、重命名、清理与电脑侧恢复证据。
- 970 次恢复过程再次中断的检查；复建 engine 后重复恢复，检查有效库引用不缺文件、旧库证据不丢失、旧音频不提前删除。
- 准备、复制、暂存、提交边界、清理阶段取消；进入重命名后的取消延迟处理。取消初始化之后恢复不会提交新库。
- 计划过期、复制中挂载代次变化、错误设备恢复、全进程写锁、空间不足、短写、创建/写入/Flush/重命名失败、清理失败、数据库及恢复记录损坏、已有库禁止初始化、只读 profile 和 hash58 密钥门禁。
- 正常导入验证媒体只写一次，目标媒体不额外完整读回；数据库仍完整读回验证。

## 保留的电脑目录

Release 的正常测试样本：

`D:/dev/foo/FooPodBridge/FooPodBridge/build/vs2022-x64/transaction-fixture-12984-5214203`

其中 device 是合成设备目录，host 保存事务与快照，source/backup 是合成音频源和对照。目录受 build 忽略规则保护，不进入 Git。之后再次运行测试会创建新的独立目录，不覆盖旧样本。

复现命令（在仓库根目录运行）：

```powershell
python scripts/build-local.py --build build/vs2022-x64 --config Debug --target ALL_BUILD
python scripts/build-local.py test --test-dir build/vs2022-x64 -C Debug --output-on-failure
python scripts/build-local.py --build build/vs2022-x64 --config Release --target ALL_BUILD
python scripts/build-local.py test --test-dir build/vs2022-x64 -C Release --output-on-failure
```

## 处理过的问题

- 首次直接构建遇到 Path/PATH 环境键重复；改用仓库现有 build-local.py 后解决，未修改全局环境。
- C++20 的 u8path 弃用警告通过 char8_t 路径构造解决，未屏蔽警告。
- 最初系统临时目录测试两次出现文件访问拒绝，第二次取得 Win32 错误 5。改在仓库 build 下新建目录后同一文件系统实现正常工作；没有降低重解析点/祖先目录锁定检查。第三次已通过目录访问，后续测试暴露的是独立的历史恢复期望问题。
- 历史恢复测试促使完成标记绑定选定指纹，避免在后续事务之后再次执行旧清理。
- 自动事务快照接入后，独立轮换测试与事务快照发生序号冲突；按设备键分离独立测试数据后通过。

## 证据限制与下一步

内存中断注入保留的是模拟时点文件状态，不模拟真实 FAT32 的扇区撕裂、目录持久顺序、USB 控制器缓存或固件播放；这些必须在独立授权实机任务中验证。损坏/未知记录、部分恢复临时文件等无法证明安全的组合保持 RecoveryRequired 或 CleaningRequired，测试通过不表示所有故障均能自动恢复。

当前已有只读恢复发现和离线完整基线枚举，beta.7 已交付。正式恢复操作的服务接口、设备与私有仓库自动关联、真实卷门禁及授权绑定仍未完成；beta.7 设置页人工检查已通过，正式恢复操作仍待实现和验收。007 保持实现中。
