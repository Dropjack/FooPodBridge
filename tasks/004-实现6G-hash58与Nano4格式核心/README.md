# 004-实现 6G/hash58 与 Nano 4 格式核心

- 状态：规格核对中
- 日期：2026-09-09
- 前置任务：[`003-实现传统 iTunesDB 共同往返核心`](../003-实现iPodPhoto数据库往返核心/README.md) 已验收
- 对应路线：[`../TODO.md`](../TODO.md) 的任务 004
- 当前授权：允许整理规格、读取 Git 忽略的电脑端私有 fixture、核对已审计来源并做只读分析；开始 C++ 实现和构建前仍需明确批准本任务规格
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

- 仅覆盖传统 6G/hash58 家族候选：Classic、Nano 3、Nano 4；不同型号仍保留各自证据等级。
- Nano 5、iTunesCDB、SQLite、hash72、CBK 和 iOS 明确排除。
- 不加载或分发 `iTunesCrypt.dll`，不采用旧 x86 二进制。
- hash58 允许修改采用固定 libgpod 文件，保留 BSD-3-Clause 版权与免责声明；6G 数据库部分继续原创实现。
- 完整 FireWire GUID/稳定设备 ID 不进入 Git、文档、测试名、日志或错误；公开材料只能显示是否存在和遮蔽摘要。
- 本任务 Writer 仍只输出内存或电脑端测试产物，不访问盘符，不提交设备，不取得 `DeviceWriteVerified`。

## 当前证据与唯一可能的用户动作

- Nano 4 到货非空库与 Restore 后空库私有 fixture 已存在，任务 003 已证明共同结构可读且 no-op 原字节保持。
- 到货库的 hash58 区非零；Restore 后数据库可提供 Apple 生成的空库差异证据。
- 当前 `SysInfo` 为空且没有 `SysInfoExtended`，仓库尚未持有可供任务 004 验证的稳定 16 十六进制字符 FireWire GUID。
- hash58 算法、公开向量和错误输入测试可以先完成；要把 Nano 4 私有 fixture 与正确设备密钥做最终交叉验证时，可能需要用户把 Nano 4 接入一次。届时 Codex 只做明确说明的只读身份采集，不写设备。

## 计划交付

- `TraditionalHash58` 格式 profile 与设备密钥值对象；
- 独立 hash58 生成/验证模块和 BSD 来源声明；
- 6G root/header、记录差异与签名保留区合同；
- 固定向量、错误 ID、错误长度、篡改和错误设备密钥测试；
- Nano 4 两份私有 fixture 的签名差异与脱敏比较报告；
- 正确签名的电脑侧空 Library 产物及独立 Reader/Validator/hash verifier 再读；
- Debug/Release 全量 CTest，不部署组件、不生成用户候选包。

## 下一检查点

先把上述边界收敛成实现级 [`SPEC.md`](SPEC.md)。规格会用“输入是什么、成功证明什么、失败如何拒绝”的方式表达；用户不需要检查算法代码，只需确认：私有 ID 不外泄、本任务不写设备、同家族不等于所有型号都已验证。
