# 004-实现 6G/hash58 与 Nano 4 格式核心

- 状态：已验收
- 日期：2026-09-09
- 前置任务：[`003-实现传统 iTunesDB 共同往返核心`](../003-实现iPodPhoto数据库往返核心/README.md) 已验收
- 对应路线：[`../TODO.md`](../TODO.md) 的任务 004
- 当前授权：用户已于 2026-09-10 批准规格、C++ 实现、构建和电脑侧测试，并于同日确认任务 004 通过；设备写入、部署与候选包发布仍未授权
- 长期蓝图：[`../../docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md`](../../docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md) 的 `BP-DB-*`、`BP-INIT-003`、`BP-FMT-001/003`
- 来源边界：[`../../docs/REFERENCE_PROVENANCE.md`](../../docs/REFERENCE_PROVENANCE.md) 的 `LG-H58-01`、`LG-DEV-01`、`FD-CRYPT-01`
- Nano 4 私有证据：[`../../docs/device-evidence/NANO4_20260908_BASELINE.md`](../../docs/device-evidence/NANO4_20260908_BASELINE.md)、[`../../docs/device-evidence/NANO4_20260909_RESTORED_CLEAN_WINDOWS.md`](../../docs/device-evidence/NANO4_20260909_RESTORED_CLEAN_WINDOWS.md)

## 人类语言目标

任务 003 已经能安全理解和生成传统 iPod 曲库，但 Nano 4、Nano 3 和部分 Classic 不会接受一个只有正确结构的数据库；数据库还必须用该台设备自己的稳定 ID 生成正确签名。

任务 004 就是在任务 003 的发动机上增加这把“设备专属封条”：

1. 把 6G 家族与早期未签名 iPod 明确分成不同 profile；
2. 用已审计、允许采用的 BSD-3-Clause hash58 算法生成和验证封条；
3. 对错误设备 ID、被篡改数据库和错误签名明确拒绝；
4. 在电脑内存与私有 fixture 上证明输出结构和签名一致；
5. 仍然不把任何内容写回 iPod。

这不是用户界面任务，也不是让用户学习密码学。用户只需确认隐私和设备写入边界；算法、格式、测试和来源声明由 Codex 负责。

## 为什么不能跳到任务 005

任务 005 要把设备识别成“可读、可初始化、只读或不支持”等状态。对于 Nano 4/Classic，判断能力必须知道它使用 hash58、需要哪种稳定身份，以及现有数据库签名是否匹配。跳过任务 004 会让任务 005 只能看见一个磁盘，不能可靠发布数据库能力。

## 固定边界

- 本任务仅覆盖传统 6G/hash58 profile：Classic、Nano 3、Nano 4 是当前来源支持的候选映射，不同型号仍保留各自证据等级。
- Nano 5、iTunesCDB、SQLite、hash72、CBK 和 iOS 不进入任务 004；其中 Nano 5+ 非 touch 仍属于产品强制目标，由任务 011 的独立 profile 和增量来源审计处理，iOS/iPod touch 继续排除。
- Nano 4 fixture 与稳定设备 ID 只验证 `TraditionalHash58` 算法、记录版本和签名输入，生产代码不得用设备名称、私有字节或该 ID 选择通用 Writer。
- 不加载或分发 `iTunesCrypt.dll`，不采用旧 x86 二进制。
- hash58 允许修改采用固定 libgpod 文件，保留 BSD-3-Clause 版权与免责声明；6G 数据库部分继续原创实现。
- 完整 FireWire GUID/稳定设备 ID 不进入 Git、文档、测试名、日志或错误；公开材料只能显示是否存在和遮蔽摘要。
- 本任务 Writer 仍只输出内存或电脑端测试产物，不访问盘符，不提交设备，不取得 `DeviceWriteVerified`。

## 当前证据与唯一可能的用户动作

- Nano 4 到货非空库与 Restore 后空库私有 fixture 已存在，任务 003 已证明共同结构可读且 no-op 原字节保持。
- 到货库的 hash58 区非零；Restore 后数据库可提供 Apple 生成的空库差异证据。
- 当前 `SysInfo` 为空且没有 `SysInfoExtended`，仓库尚未持有可供任务 004 验证的稳定 16 十六进制字符 FireWire GUID。
- hash58 算法、公开向量和错误输入测试可以先完成；要把 Nano 4 私有 fixture 与正确设备密钥做最终交叉验证时，可能需要用户把 Nano 4 接入一次。届时 Codex 只做明确说明的只读身份采集，不写设备。

2026-09-10 实现前复核纠正了两项技术合同：hash58 规范化还必须临时清零 database ID 与 `0x32` 预哈希块并强制 scheme `1`，不能只清零 `0x58`；Nano 4 私有 fixture 的五个 dataset 中，type `4/1/3/2` 是固定历史 Writer 证明的 6G 基础结构，type `5` 只在 special playlists 有效时出现，公开空库不能从私有 fixture 伪造模板。修订已写入 [`SPEC.md`](SPEC.md)。用户已于 2026-09-10 明确批准该规格，当前进入 C++ 实现；批准不包含设备访问、设备写入、部署或候选包发布。

## 计划交付

- `TraditionalHash58` 格式 profile 与设备密钥值对象；
- 独立 hash58 生成/验证模块和 BSD 来源声明；
- 6G root/header、记录差异与签名保留区合同；
- 固定向量、错误 ID、错误长度、篡改和错误设备密钥测试；
- Nano 4 两份私有 fixture 的签名差异与脱敏比较报告；
- 正确签名的电脑侧空 Library 产物及独立 Reader/Validator/hash verifier 再读；
- Debug/Release 全量 CTest，不部署组件、不生成用户候选包。

实现和脱敏结构比较记录见 [`REPORT.md`](REPORT.md)。

## 下一检查点

实现、公开向量、两份 Nano 4 私有签名交叉验证、全非 touch 范围复评和 Debug/Release 全量回归均已完成，结果见 [`REPORT.md`](REPORT.md)。用户于 2026-09-10 明确确认任务 004 通过；下一任务是 [`005-实现全目标家族注册表与Windows自动发现只读服务`](../005-实现全目标家族注册表与Windows自动发现只读服务/README.md)。设备仍未获得任何写入授权。
