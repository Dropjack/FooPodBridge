# 001-完成参考源码与许可证审计

- 状态：已验收
- 日期：2026-08-28
- 前置任务：[`000-确定项目目标与全部产品决策`](../000-确定项目目标与全部产品决策/README.md) 已验收
- 当前产物：文件级来源矩阵、许可证边界、任务 002–014 采用结论、证据与验证清单
- 验收记录：用户于 2026-08-28 确认最终许可证与采用边界，并明确要求进入任务 002

## 1. 任务目标

在任何第三方源码进入正式工程前，逐文件确认 FooPodBridge 后续任务需要的格式知识、算法、版权和许可证边界。最终让每条参考路径都有明确结论：允许采用、只能参考知识后原创实现、需要修改并保留许可证、或禁止使用。

## 2. 为什么先做

FooPodBridge 计划使用 `LGPL-3.0-or-later`，FooCrate 保持 MIT，但这个方向不能代替对实际文件的审计。foo_dop、dop-sdk、libgpod、foobar2000 SDK 和 Columns UI SDK 可能具有不同许可证、版本与分发要求；只有审计完成，任务 002 才能安全建立正式工程和包。

## 3. 输入与边界

- 只读参考根：`D:\dev\foo\FooPodBridge\Ref`；永不修改、格式化、构建或清理；
- 上游项目的官方仓库、具体 commit/tag、许可证全文和文件版权头；
- [`REFERENCE_PROVENANCE.md`](../../docs/REFERENCE_PROVENANCE.md) 的现有初步记录；
- [`USER_DECISIONS.md`](../../decisions/USER_DECISIONS.md) 的 `DEC-LIC-001`；
- 后续任务 002–014 实际需要的数据库、设备、hash58、gapless、artwork 和 Smart Playlist 路径。

本任务不读取或写入真实 iPod，不启动 foobar2000，不创建 CMake/C++，不复制第三方源码到正式目录，也不访问 C 盘日常安装。

## 4. 审计逻辑

每个候选文件按同一流程处理：

1. 记录上游项目、URL、commit/tag、取得日期和本地参考路径；
2. 读取项目许可证全文与文件级版权头，区分 `only`、`or-later` 和多许可证；
3. 说明后续任务为什么需要它，不能用“以后可能有用”代替具体用途；
4. 将采用方式分类为：原样采用、修改采用、只参考格式/行为后原创实现、或禁止使用；
5. 记录正式目标文件、必须保留的声明、源码提供义务和包内 NOTICE/LICENSE 要求；
6. 用另一来源、fixture、已知向量或实机计划说明怎样验证技术结论，而不是把历史实现当作唯一真相；
7. 检查共享服务合同保持原创宽松许可，FooCrate 不包含 LGPL Core、私有头文件或实现代码；
8. 对无法确认来源、许可证冲突、闭源二进制或不需要的历史路径给出明确禁止结论。

## 5. 重点审计范围

- foo_dop：传统 iTunesDB Reader/Writer、普通/Smart Playlist、ArtworkDB、SoundCheck、MP3/AAC gapless 和设备识别；
- dop-sdk：其 0BSD 边界及是否确有必要；
- libgpod：hash58、设备能力、数据库与 SysInfo 资料；
- foobar2000 SDK 与 Columns UI SDK：允许使用的版本、获取方式、版权声明和组件分发边界；
- 明确排除：`iTunesCrypt.dll`、Apple Mobile Device/iPhone 路径、来源不明二进制和当前不支持的 hash72/CBK。

## 6. 永久产物

- 在 [`REFERENCE_PROVENANCE.md`](../../docs/REFERENCE_PROVENANCE.md) 中形成文件级来源矩阵；
- FooPodBridge 最终 SPDX 许可证结论和适用理由；
- FooCrate MIT、FooPodBridge LGPL 与原创共享服务合同的边界说明；
- 正式仓库需要的 LICENSE/NOTICE/第三方声明清单；
- 任务 002–014 每条参考能力的允许、限制或禁止结论；
- 尚需 fixture、已知向量或实机验证的技术证据清单。

## 7. 自动与人工检查

自动检查：

- 本地 Markdown 链接、UTF-8 无 BOM 和 LF；
- 每个采用项都有上游、commit/tag、文件、许可证、采用方式、正式目标和验证；
- 禁止项没有进入正式源码、依赖或组件包；
- 参考目录没有任何修改或生成文件。

用户检查：

- 理解“采用源码”“修改源码”“只参考格式知识后原创实现”和“禁止使用”的差别；
- 确认 FooCrate 继续使用 MIT；
- 批准 FooPodBridge 最终 SPDX、第三方声明和公开范围。

## 8. 通过标准

- 任务 002–014 所需参考路径都有文件级允许、限制或禁止结论；
- `LGPL-3.0-or-later` 与实际采用文件兼容，或已回到 `DEC-LIC-001` 重新决定；
- `iTunesCrypt.dll` 和其他来源不明二进制保持禁止；
- foobar2000/Columns UI SDK 的使用与分发边界明确；
- 用户核对最终许可证结论后，本任务才能标记已验收并进入任务 002。

## 9. 用户验收结论

用户已于 2026-08-28 明确确认以下最终结论：

1. FooPodBridge 原创源码使用 `LGPL-3.0-or-later`；
2. 唯一计划修改采用的第三方算法是 libgpod `src/itdb_hash58.c`，该文件保持 `BSD-3-Clause`；
3. FooPodBridge/FooCrate 共享服务合同使用原创 `0BSD`，FooCrate 保持 MIT；
4. foo_dop 和 libgpod 其余候选源码只作知识参考后原创实现，`iTunesCrypt.dll`、旧 SDK、iOS、hash72/CBK 路径保持禁止。

本任务因此标记为“已验收”。原定由任务 002 补做的官方 foobar2000 SDK 包内 `sdk-license.txt` 复核已于 2026-08-29 完成，正文与本审计结论一致，不需要重开任务 001。

## 10. 审计结果

- 永久来源矩阵：[`../../docs/REFERENCE_PROVENANCE.md`](../../docs/REFERENCE_PROVENANCE.md)；
- 固定证据和 SHA-256：[`AUDIT_EVIDENCE.md`](AUDIT_EVIDENCE.md)；
- foo_dop 普通候选文件没有逐文件授权头，仓库只指向 LGPLv3 全文且未写 `or-later`，因此不直接复制；
- libgpod `itdb_hash58.c` 有独立 BSD-3-Clause 文件头，允许修改移植并保留声明；
- libgpod 其他候选文件是 LGPL-2.1-or-later，但本项目不复制，也不引入 GLib/GdkPixbuf；
- dop-sdk 与 Columns UI SDK 的当前许可证为 0BSD；旧 dop-sdk 因不需要而不采用；
- foobar2000 SDK 使用自定义 BSD 风格许可，记录为 `LicenseRef-foobar2000-SDK`，不误标标准 BSD-3-Clause；
- 任务 002–014 的每条参考能力都有允许、限制或禁止结论及独立验证方法。

## 11. 验证记录

- 本地 `ipod_manager` 主仓库及六个子模块在审计后均为 clean，`Ref` 没有任何生成或修改；
- `iTunesCrypt.dll` 复核为 x86、未签名、无版本/公司元数据，DLL、导入库和声明头全部禁止；
- 官方 foobar2000 SDK 2025-03-07 包已下载到安全临时目录并确认 SHA-256，临时文件随后删除；当前 `bsdtar` 不支持其 LZMA codec，包内许可证复核按三次上限停止并转为任务 002 的明确前置检查；
- 审计没有创建或修改 C++、CMake、组件包、foobar2000 实例、FooCrate 或真实设备；
- Markdown 本地链接、严格 UTF-8 无 BOM、LF 和来源矩阵完整性检查在本轮交付前执行。
- 2026-08-28：用户确认 FooPodBridge `LGPL-3.0-or-later`、hash58 `BSD-3-Clause`、原创共享合同 `0BSD`、FooCrate MIT，以及其余源码只参考/禁止边界；任务 001 验收通过。
- 2026-08-29：任务 002 使用用户提供的官方包与解压目录补做许可复核；包哈希匹配、WinRAR 完整性测试通过，官方许可与 FooCrate 的 2025 SDK 副本逐字节一致。
