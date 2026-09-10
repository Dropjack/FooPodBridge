# 003-实现传统 iTunesDB 共同往返核心

- 状态：已验收
- 日期：2026-09-09
- 前置任务：[`002-建立x64组件工程与服务合同`](../002-建立x64组件工程与服务合同/README.md) 已验收
- 对应路线：[`../TODO.md`](../TODO.md) 的任务 003
- 当前授权：用户于 2026-09-09 确认设备已弹出，并批准按 `SPEC.md` 实现、使用电脑端私有 fixture 测试及执行 Debug/Release 构建；不授权设备访问、设备写入、组件部署或候选包制作
- 长期实现蓝图：[`../../docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md`](../../docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md) 的 `BP-DB-*`、`BP-INIT-001/003`、`BP-FMT-001/002/003`
- 实现规格：[`SPEC.md`](SPEC.md)
- 验证记录：[`VALIDATION.md`](VALIDATION.md)
- Nano 4 基线：[`../../docs/device-evidence/NANO4_20260908_BASELINE.md`](../../docs/device-evidence/NANO4_20260908_BASELINE.md)
- Nano 4 Restore 后空库：[`../../docs/device-evidence/NANO4_20260909_RESTORED_CLEAN_WINDOWS.md`](../../docs/device-evidence/NANO4_20260909_RESTORED_CLEAN_WINDOWS.md)
- iPod 5.5G 只读基线：[`../../docs/device-evidence/IPOD55G_20260909_CLEAN_WINDOWS.md`](../../docs/device-evidence/IPOD55G_20260909_CLEAN_WINDOWS.md)
- 磁盘模式验证结论：[`../../docs/device-evidence/NANO4_ITUNES_RESTORE_HANDOFF.md`](../../docs/device-evidence/NANO4_ITUNES_RESTORE_HANDOFF.md)

## 为什么先做共同核心

iPod Photo 的未签名传统 `iTunesDB` 适合定义最小格式 profile；当前到手的 Nano 4 则提供了真实、较复杂的传统 `iTunesDB` 私有 fixture。两者共同驱动 Reader、Model、Writer 和 Validator 骨架：任务 003 先完成所有家族复用的记录安全，任务 004 再增加 6G 记录差异、设备 ID 和 hash58。

## 本轮结论

FooPodBridge 不以“复刻 iTunes、一次支持所有 iPod”为目标。完整兼容还涉及连接方式、文件系统、数据库世代、签名、artwork、固件差异和实机安全验证；仅按“第几代”判断不足以可靠写入。

数据库系统按以下层次扩展：

1. **共同往返核心**：有界解析记录、建立稳定模型、保留未知数据、验证结构并安全重新生成数据库。
2. **数据库格式配置**：描述传统 Photo、Classic 等格式的记录和校验差异，不把所有差异堆进一个型号分支。
3. **设备能力配置**：根据已取得的设备属性、数据库版本和固件证据决定可用能力；营销名称只作为辅助信息。
4. **验证等级**：依次区分“结构已知、fixture 往返通过、实机只读通过、实机写入通过”，不能把较低等级宣传成完整支持。

因此，架构按数据库家族扩展，并按 `StructureKnown / FixtureRoundTrip / DeviceReadVerified / DeviceWriteVerified` 显示证据等级。未知设备仍由服务层拒绝写入；点名的用户自有实验机可以在独立任务、完整备份和事务门槛下受控写入，但在完成实机验收前只能标为 Experimental。Shuffle 与 Nano 5+ 非 touch 是正式目标中的独立路线，iOS/iPod touch 继续排除。

## 历史实现采用方法

本任务不再自行通读并复述整个 iPod manager。共同程序逻辑、源码位置和证据等级统一维护在长期蓝图中。本任务只做以下工作：

1. 从 `BP-DB-*` 采用传统 Reader/Writer、稳定模型和 playlist 引用的目标行为；
2. 从 `BP-INIT-001/003` 增加电脑侧未签名传统空 Library 生成能力，但不连接或初始化实机；
3. 需要核对字段时查看蓝图固定的 `reader.cpp`、`writer_itunesdb.cpp` 和 `itunesdb_playlist.cpp` 位置；
4. 用 libgpod、Nano 4 私有 fixture、合成未签名 profile 和以后可取得的真实早期设备 fixture 分级交叉验证，不能把 foo_dop 当唯一真相；
5. 使用本项目的有界解析、未知数据保留、独立 Validator 和错误模型原创实现；
6. 任何第三方表达复用仍受 [`../../docs/REFERENCE_PROVENANCE.md`](../../docs/REFERENCE_PROVENANCE.md) 限制。

## 2026-09-08 Nano 4 到货证据

- 纯净 Windows 直接挂载 FAT32 removable volume，不需要安装 iTunes 才能读取现有 Library；
- 原始 Device/iTunes/Artwork 已进入 Git 忽略的私有 fixture；
- 完整 `iPod_Control` 设备外备份经 232 个文件逐一 SHA-256 验证，零差异；
- 当前数据库为传统未压缩 `iTunesDB`，可为共同 Reader 提供真实输入；
- 设备的 `SysInfo` 为空且没有 `SysInfoExtended`，说明任务 005 仍必须自己取得稳定身份和 hash58 输入；
- 本任务不因备份完成而写入设备。

## 2026-09-09 磁盘模式结论

- 用户已完成新状态 Nano 4 测试，并确认设备接入电脑时默认自动开启“用作磁盘”，记为 `MountedByDefault`；
- 已在从未安装 iTunes 的电脑只读确认 Restore 后设备仍直接挂载 FAT32 volume；完整备份、Device/iTunes 私有 fixture 和采集前后逐文件 SHA-256 对照零差异；
- Restore 后空 `iTunesDB` 为 14,314 字节、header 244、format 1、version 115、五个 dataset，可与到货非空库共同驱动任务 003 的 Reader/preserve-only 测试；
- 产品边界已通过 [`DEC-DEV-002`](../../decisions/USER_DECISIONS.md) 冻结：FooPodBridge 只管理 Windows 已暴露的可访问存储卷。如果“用作磁盘”未默认开启或后续被关闭，不配置 iPod、不控制 iTunes、不发送私有启用命令；
- 该结论消除了继续验证“如何帮设备开启磁盘使用”的任务前置，不改变任务 003 的纯电脑 fixture 与临时目录边界。

## 任务 003 的实际边界

本任务只完成共同往返核心和未签名传统格式 profile，以私有/合成 fixture 验证：

- 正常数据库可读取并通过结构验证；
- 无修改往返保持语义和未知数据；
- 截断、越界、长度冲突和非法引用被明确拒绝；
- 在内存模型中增删虚拟曲目与普通 playlist 后可以稳定往返；
- 可以从空模型生成仅含合法 master Library 的未签名传统数据库，并由独立 Reader/Validator 再读通过；
- 可以读取 Nano 4 fixture 的共同记录并原样保留本任务尚未理解的 6G/签名区域，但不在本任务声称能生成 Nano 4 可接受数据库；
- 所有测试只操作仓库 fixture 和电脑临时目录，不连接或写入实机。

2026-09-09 又取得用户所称 iPod 5.5G 的真实传统数据库私有 fixture：4,034,040 字节、header 244、format 1、version 49、五个 dataset，并带有较大 Library 与 Artwork 数据。精确型号/profile 核实前使用 `TraditionalPreserveOnly`；它增强 Reader、规模和未知数据保留测试，但不扩大本任务的设备写入范围。

Classic、Nano 3/4 的 6G 格式差异与 hash58 留在任务 004。真实 Photo fixture 到手后可以把该型号从 `StructureKnown` 提升到 `FixtureRoundTrip`，但不再阻断共同核心实现。

## 下一检查点

[`SPEC.md`](SPEC.md) 已把私有 Nano 4 fixture 的 preserve-only 使用方式、未签名最小空 Library、Reader/Model/Writer/Validator、未知记录保留、错误分类、资源限制和自动比较标准收敛为实现合同。

用户已批准 `SPEC.md` 第 17 节的五个业务边界并授权实现。C++ Core、三份私有 fixture preserve-only 验证及 Debug/Release 全量构建与 CTest 已完成，详见 [`VALIDATION.md`](VALIDATION.md)。用户于 2026-09-09 明确验收任务 003；磁盘模式与 iTunes 配置不再是后续数据库任务的检查点。
