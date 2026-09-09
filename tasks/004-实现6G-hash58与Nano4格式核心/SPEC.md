# 任务 004：6G/hash58 与 Nano 4 格式核心实现规格

- 状态：规格核对中
- 日期：2026-09-09
- 任务入口：[`README.md`](README.md)
- 前置实现：[`../003-实现iPodPhoto数据库往返核心/SPEC.md`](../003-实现iPodPhoto数据库往返核心/SPEC.md)
- 来源边界：[`../../docs/REFERENCE_PROVENANCE.md`](../../docs/REFERENCE_PROVENANCE.md) 的 `LG-H58-01`、`LG-DEV-01`、`FD-CRYPT-01`

## 1. 交付结果

在任务 003 的纯内存 database Core 上增加 `TraditionalHash58` profile。调用方给出传统 6G 数据库模型、严格解析后的设备密钥和确定生成上下文后，Writer 可以生成带 hash58 的字节；独立 verifier 必须能用同一设备密钥验证，错误密钥或任意受保护字节篡改必须失败。

本任务完成后仍不访问 Windows 设备、不写盘符、不部署组件，也不证明 iPod 固件实际接受新数据库。任务 005 才把生产设备身份和 profile 选择接入只读服务；任务 008 才允许点名 Nano 4 的首次受控写入。

## 2. 格式能力

`profile_kind` 增加 `traditional_hash58`：

- root 使用传统 `mhbd` 容器和 244 字节 header；
- hash58 是 root header 从 `0x58` 开始的 20 字节字段；
- hash 输入是完整数据库字节，计算前只把 hash58 字段置零；
- 现有数据库中 `0x72` 与 `0xAB` 开始的保留区不被解释为 hash72/CBK，未修改节点必须原字节保留；
- 新生成的 hash58-only 数据库不调用或生成 hash72/CBK/SQLite 数据；
- profile 明确声明 6G track/playlist header 尺寸和必需 dataset，不能只因 root header 长 244 就猜测设备家族。

Reader 观察到合法 hash58 envelope 不等于授予可写能力。只有显式 `TraditionalHash58`、有效设备密钥、Validator 通过且无不安全 opaque 依赖时才允许生成。

## 3. 私有设备密钥

公开 API 使用值对象 `hash58_device_key`，只接受恰好 16 个 ASCII 十六进制字符，不接受 `0x`、空格、分隔符、短 ID、长 UDID 或任意文本。

- 解析后立即保存为 8 个字节；比较大小写不影响结果；
- 错误对象只报告 `missing / wrong length / non-hex`，不得回显输入；
- 文档、日志、测试名、比较报告和 Git fixture 不保存完整 ID；
- hash/verifier 不读取注册表、环境变量、SysInfo、USB 或设备路径；
- 任务 005 的设备模块以后负责只读取得身份，并只把解析后的值对象传给 database Core。

任务 004 的 Nano 4 fixture 交叉验证需要一次真实密钥，但密钥只进入 Git 忽略的本机测试输入或当前进程，不写进公开 golden vector。

## 4. hash58 模块

允许修改采用固定 libgpod commit 的 `src/itdb_hash58.c` 算法与常量，正式文件必须：

- 保留 Christophe Fergeau 的 BSD-3-Clause 版权、条件和免责声明；
- 注明 FooPodBridge 的 C++20、无 GLib、span/result API 修改；
- 只负责从 8 字节设备值派生 20 字节 HMAC key；
- SHA-1/HMAC 由独立、可测试的实现提供，不加载第三方 DLL；
- 不导出原始设备值，不提供通用任意算法接口。

公开函数分为三步，便于独立测试：

1. `parse_hash58_device_key`：严格文本输入到私有值对象；
2. `compute_hash58`：设备值 + 已把签名字段归零的数据库字节到 20 字节签名；
3. `verify_hash58`：复制输入、归零签名字段、常量时间比较期望值。

## 5. Reader、Validator 与 Writer

Reader：

- 输入小于 `0x6c`、root/header 越界或错误 marker 按任务 003 错误模型拒绝；
- 保存原始 hash58 与保留区，不把签名内容写入普通 diagnostics；
- 未提供设备密钥时可以结构化读取，但签名状态为 `not_checked`；
- 提供密钥后状态只有 `valid / invalid`，不能把失败降级成可写 warning。

Validator：

- 检查 `TraditionalHash58` 与 root/header/dataset/6G 记录要求匹配；
- 任何修改输出必须拥有 `valid` 的新签名；
- preserve-only 读取仍可不提供密钥，但不能转成 hash58 可写文档；
- 错误设备密钥、零签名、签名长度错误或受保护字节变化返回稳定错误码。

Writer：

1. 先按任务 003 规则完整生成未签名字节；
2. 保留或生成 profile 明确允许的 6G header/dataset；
3. 把 `0x58..0x6b` 置零；
4. 计算并写入 20 字节 hash58；
5. 用全新 Reader、Validator 和 hash verifier 再读；
6. Comparator 同时检查语义、未知保留区和签名外非允许变化；
7. 失败时丢弃全部输出。

Writer 只返回内存字节。没有路径、临时文件、设备备份、提交或恢复逻辑。

## 6. 空 Library 与现有 fixture

电脑侧空 Library 需要调用方提供 database/master persistent ID、确定时间、`TraditionalHash58` profile 和设备密钥。输出必须包含 profile 要求的五类传统 dataset、零曲目 master Library 和有效 hash58。

两份 Nano 4 私有 fixture 分级使用：

- 到货非空库：验证现有签名、共同 track/playlist 语义、未知/保留区和 no-op 原字节；
- Restore 后空库：验证 Apple 空库的 header、dataset、签名和空 Library 结构；
- 未取得设备密钥前只能完成结构/no-op 检查，不能声称现有签名已验证；
- 新生成空库只做电脑侧结构、签名和差异报告，不能标为 `DeviceWriteVerified`。

用户所称 iPod 5.5G fixture 当前继续 preserve-only；精确型号和 hash58 证据未确认前，不借任务 004 自动升级写能力。

## 7. 错误与隐私

在任务 003 错误枚举上增加至少：

- `MissingDeviceKey`
- `InvalidDeviceKeyLength`
- `InvalidDeviceKeyCharacter`
- `HashFieldOutOfBounds`
- `MissingHash58`
- `Hash58Mismatch`
- `UnsupportedSignedProfile`

错误可包含结构 offset 和安全摘要，不得包含设备密钥、完整数据库 hash、媒体元数据、设备名称或真实路径。

## 8. 自动测试

公开测试：

- 固定设备值与固定字节输入的 key derivation、HMAC 和完整 hash58 golden vectors；
- 大小写等价、长度 0–15/17+、非十六进制、前缀和空白拒绝；
- hash 字段置零规则、错误密钥、单字节篡改、截断和全零签名；
- 相同模型/上下文/密钥逐字节确定；
- 签名后 Reader + Validator + verifier 通过；
- 另一设备密钥不能验证；
- 现有第二保留区 no-op 不变，禁止调用 hash72/CBK 路径；
- x64、无 GLib、无外部运行时 DLL、来源声明和许可证检查。

私有测试：

- 三份现有真实数据库继续完成 preserve-only no-op；
- 密钥存在时验证两份 Nano 4 输入的 hash58；
- 用 Nano 4 profile 生成签名空 Library 并输出脱敏结构差异；
- fixture 或私有密钥缺失时公开 CI 跳过该交叉项并明确记录，不能伪造通过。

Debug/Release 必须继续使用 `/W4 /WX /permissive- /Zc:__cplusplus /utf-8`，任务 002/003 全部回归继续通过。

## 9. 用户检查点

用户只需确认三句话：

1. 完整设备 ID 只在本机私有输入/内存中使用，不进 Git 和日志；
2. 任务 004 只在电脑端生成和验证签名，不写 iPod；
3. Nano 4 fixture 验证通过只代表该 fixture/profile，其他 Classic/Nano 型号仍各自保留证据等级。

批准后开始 C++ 实现、公开向量与私有 fixture 测试。实现进行到必须取得 Nano 4 稳定 ID 时，再给用户一条明确的接入指令；在此之前不需要用户操作设备。
