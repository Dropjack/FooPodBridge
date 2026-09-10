# 任务 004：电脑侧验证与脱敏差异报告

- 日期：2026-09-10
- 范围：内存生成、公开向量、Git 忽略的 Nano 4 数据库副本，以及用户授权的一次 Windows PnP 身份只读查询
- 明确未做：读取或写入设备文件、部署 foobar2000 组件、生成用户候选包

## 公开 hash58 证据

公开测试用固定的虚构 16 字符设备值和固定 512 字节输入建立两级 golden vector：

- key derivation 固定结果用于锁定 LCM、两张映射表和 SHA-1 输入；
- 完整 HMAC 固定结果用于锁定 database ID、预哈希块、scheme 和 hash58 字段的规范化规则。

测试还覆盖大小写等价、空值、错误长度、非十六进制、前缀、空白、错误 scheme、全零签名、错误设备值和单字节篡改。生产错误与日志不包含输入设备值或签名字节。

## 三类电脑侧输入比较

下表只记录结构和“零/非零”状态，不包含设备 ID、签名、媒体元数据或真实设备路径。

| 输入 | 大小 | root / 版本 | dataset（类型:数量） | hash58 | `0x72` 起 46 字节 | `0xAB` 起 57 字节 |
| --- | ---: | --- | --- | --- | --- | --- |
| Nano 4 到货库私有副本 | 308,628 | 244 / 49 | `4:200, 1:204, 3:1, 2:1, 5:5` | 当前设备身份验证通过 | 45 字节非零，原样保留 | 全零，原样保留 |
| Nano 4 Restore 空库私有副本 | 14,314 | 244 / 115 | `4:0, 1:0, 3:1, 2:1, 5:5` | 当前设备身份验证通过 | 45 字节非零，原样保留 | 全零，原样保留 |
| FooPodBridge 公开签名空库 | 1,472 | 244 / 115 | `4:0, 1:0, 3:1, 2:1` | 公开虚构密钥验证通过 | 全零 | 全零 |

三者的 dataset/list header 分别为 96/92 字节。到货库的 track header 是 584 字节、playlist header 是 140 字节；Restore 输入的 playlist header 是 184 字节。FooPodBridge 公开输出显式选择 184 字节 playlist 版本，并生成语义一致的 type 3/type 2 零曲目 master Library 视图。

Apple 私有输入中的 type 5 包含未建模的 special playlists。公开空库不复制这些名称、规则或成员，因此只生成固定历史 Writer 支持的四类基础 dataset。`0x72` 保留区的非零数据也不被推断成另一签名方案；hash58-only 新输出保持这些排除区为零。

## 当前证据结论

- 两份 Nano 4 私有数据库均已通过 `TraditionalHash58` 的无密钥结构读取和当前设备身份签名验证，并继续通过 preserve-only 原字节 no-op；iPod 5.5G 输入仍保持 preserve-only。
- 公开签名空库通过全新 Reader、Validator、直接 verifier 和逐字节确定性检查。
- Windows PnP 中唯一匹配的 Apple iPod 身份尾部恰为 16 个十六进制字符，并同时验证两份 Apple fixture；完整值只存在于查询/测试进程内，未输出、未落盘、未进入 Git，验证后已清除进程环境。
- 电脑侧通过不等于固件接受，不提升任何型号到 `DeviceWriteVerified`；首次实机写入仍属于后续任务并需要单独批准与备份。
- 2026-09-10 重开任务 000 后复核生产边界：`hash58` 与 Writer API 只接收数据库 profile 和解析后的 8 字节设备密钥，不接收 Nano/Classic 营销型号，不读取私有 fixture，也没有针对当前稳定 ID 的条件分支。固定的 header/dataset/版本要求属于当前 `TraditionalHash58` profile 证据；其他候选设备不匹配时安全拒绝并留给任务 005 的 `FormatPending`，不能猜写。
- Nano 5+ 非 touch 已进入正式产品目标，但它需要任务 011 的 CDB/SQLite/签名独立 profile；任务 004 没有把 hash58 外推到该家族。

私有测试支持从当前进程环境或忽略目录中的 `nano4-hash58-device-key.txt` 接收密钥；两种方式都不打印输入。公开 CI 环境中缺失私有输入时明确报告跳过，绝不使用虚构值代替。

## 构建与边界检查

- MSVC x64 Debug：9/9 CTest 通过；
- MSVC x64 Release：9/9 CTest 通过；
- 当前 Nano 4 身份在 Debug 与 Release 私有测试中均验证两份 fixture 通过；
- 数据库 Core 与测试编译参数包含 `/W4 /WX /EHsc /permissive- /Zc:__cplusplus /utf-8`；本轮重建发现并补齐显式 `/EHsc`，避免标准容器异常展开警告 C4530 在 `/WX` 下阻断编译；
- Release 测试映像为 PE32+ x64，只依赖 Windows 与项目既有 MSVC/UCRT 运行库，没有引入 GLib 或额外密码学 DLL；
- source-boundaries、`git diff --check`、严格 UTF-8、无 BOM、LF-only 检查通过。
- 任务 000 范围修订后再次执行 MSVC x64 Debug/Release 全量构建与 CTest，仍为两套 9/9 通过；建立任务 005 入口后共 31 个 Markdown 文件通过严格 UTF-8、无 BOM、LF-only 和本地链接检查，任务索引为唯一连续 000–023。

## 用户验收

用户于 2026-09-10 明确表示“004通过”；该任务主要是电脑侧 Core、签名与 fixture 证据，没有额外实机人工步骤。任务因此标记为已验收。此验收不授权设备写入，也不把 Nano 4 证据外推到其他型号。
