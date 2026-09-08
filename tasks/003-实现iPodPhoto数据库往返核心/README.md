# 003-实现传统 iTunesDB 共同往返核心

- 状态：规格核对中
- 日期：2026-09-08
- 前置任务：[`002-建立x64组件工程与服务合同`](../002-建立x64组件工程与服务合同/README.md) 已验收
- 对应路线：[`../TODO.md`](../TODO.md) 的任务 003
- 当前授权：允许记录设计结论和对点名 Nano 4 做只读采集/电脑端备份；尚未授权 C++ 实现、构建或任何设备写入
- 长期实现蓝图：[`../../docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md`](../../docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md) 的 `BP-DB-*`、`BP-INIT-001/003`、`BP-FMT-001/002/003`
- Nano 4 基线：[`../../docs/device-evidence/NANO4_20260908_BASELINE.md`](../../docs/device-evidence/NANO4_20260908_BASELINE.md)
- Restore 交接任务：[`../../docs/device-evidence/NANO4_ITUNES_RESTORE_HANDOFF.md`](../../docs/device-evidence/NANO4_ITUNES_RESTORE_HANDOFF.md)

## 为什么先做共同核心

iPod Photo 的未签名传统 `iTunesDB` 适合定义最小格式 profile；当前到手的 Nano 4 则提供了真实、较复杂的传统 `iTunesDB` 私有 fixture。两者共同驱动 Reader、Model、Writer 和 Validator 骨架：任务 003 先完成所有家族复用的记录安全，任务 004 再增加 6G 记录差异、设备 ID 和 hash58。

## 本轮结论

FooPodBridge 不以“复刻 iTunes、一次支持所有 iPod”为目标。完整兼容还涉及连接方式、文件系统、数据库世代、签名、artwork、固件差异和实机安全验证；仅按“第几代”判断不足以可靠写入。

数据库系统按以下层次扩展：

1. **共同往返核心**：有界解析记录、建立稳定模型、保留未知数据、验证结构并安全重新生成数据库。
2. **数据库格式配置**：描述传统 Photo、Classic 等格式的记录和校验差异，不把所有差异堆进一个型号分支。
3. **设备能力配置**：根据已取得的设备属性、数据库版本和固件证据决定可用能力；营销名称只作为辅助信息。
4. **验证等级**：依次区分“结构已知、fixture 往返通过、实机只读通过、实机写入通过”，不能把较低等级宣传成完整支持。

因此，架构按数据库家族扩展，并按 `StructureKnown / FixtureRoundTrip / DeviceReadVerified / DeviceWriteVerified` 显示证据等级。未知设备仍由服务层拒绝写入；点名的用户自有实验机可以在独立任务、完整备份和事务门槛下受控写入，但在完成实机验收前只能标为 Experimental。Shuffle、Nano 5+ 与 iOS 仍是独立路线。

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

## 任务 003 的实际边界

本任务只完成共同往返核心和未签名传统格式 profile，以私有/合成 fixture 验证：

- 正常数据库可读取并通过结构验证；
- 无修改往返保持语义和未知数据；
- 截断、越界、长度冲突和非法引用被明确拒绝；
- 在内存模型中增删虚拟曲目与普通 playlist 后可以稳定往返；
- 可以从空模型生成仅含合法 master Library 的未签名传统数据库，并由独立 Reader/Validator 再读通过；
- 可以读取 Nano 4 fixture 的共同记录并原样保留本任务尚未理解的 6G/签名区域，但不在本任务声称能生成 Nano 4 可接受数据库；
- 所有测试只操作仓库 fixture 和电脑临时目录，不连接或写入实机。

Classic、Nano 3/4 的 6G 格式差异与 hash58 留在任务 004。真实 Photo fixture 到手后可以把该型号从 `StructureKnown` 提升到 `FixtureRoundTrip`，但不再阻断共同核心实现。

## 下一检查点

下次按蓝图 `BP-DB-*`、`BP-INIT-003` 和 `BP-FMT-002/003`，冻结私有 Nano 4 fixture 的使用方式、未签名最小空 Library、未知记录保留和自动比较标准；核对完成并获得明确实现授权后，任务才能转为“可实现”。
