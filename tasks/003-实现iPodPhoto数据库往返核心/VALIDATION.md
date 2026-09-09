# 任务 003 验证记录

- 日期：2026-09-09
- 状态：实现完成待验收
- 规格：[`SPEC.md`](SPEC.md)
- 实现入口：[`../../include/foopodbridge/core/database`](../../include/foopodbridge/core/database)

## 交付边界

本次实现交付独立 C++20 database Core，包括有界 Reader、稳定 Model、独立 Validator、原子 EditPlan、确定性内存 Writer、结构化错误与 Comparator。

- `TraditionalUnsigned` 可以生成和修改电脑侧合成数据库；
- `TraditionalPreserveOnly` 只允许无修改逐字节输出；
- Smart/Opaque playlist 与未知 dataset 不取得编辑能力；
- 任一 opaque 依赖会阻断语义修改；
- Core 不接受路径、盘符、设备或文件句柄，不包含设备 I/O、hash58、部署和实机写入。

## 自动验证

使用 Visual Studio 2022 Build Tools 17.14.37、MSVC 19.44.35228.0，以及 Visual Studio 随附的 CMake 和 Ninja 完成独立 Debug/Release 构建。宿主环境同时注入 `Path` 与 `PATH`，会触发 .NET MSBuild 的环境字典冲突，因此改用相同 MSVC 工具链的 Ninja 生成器；编译器、选项和测试范围没有降级。

| 配置 | 编译要求 | CTest |
| --- | --- | --- |
| Debug | C++20、x64、`/W4 /WX /permissive- /Zc:__cplusplus /utf-8` | 8/8 通过 |
| Release | C++20、x64、`/W4 /WX /permissive- /Zc:__cplusplus /utf-8` | 8/8 通过 |

测试覆盖：

- 空 Library 的生成、独立再读、Validator 和确定性逐字节输出；
- UTF-8/UTF-16 字符串往返、曲目增删、master 自动维护、普通 playlist 增删改与有序成员；
- 全部截断位置、越界长度、超大 count、错误容器、输入/输出资源限制、奇数 UTF-16、重复 ID 和无效 GenerationContext；
- 合成未知 dataset 的 no-op 原字节保留及语义修改拒绝；
- Comparator 对逐字节和领域语义的非允许变化检测；
- 任务 002 的版本、服务合同、组件身份和源码依赖边界回归。

## 私有 fixture 脱敏比较报告

三份测试均只读取仓库内 Git 忽略的电脑端 fixture，没有连接设备。报告不包含名称、媒体路径、设备 ID 或完整 hash。

| 输入 | 观察结构 | 请求变化 | 必须保持 | 结果 |
| --- | --- | --- | --- | --- |
| Nano 4 到货非空库 | 308,628 字节，传统容器，204 tracks | 无 | 全部原字节与 opaque dataset | no-op 精确一致；任意 EditPlan 返回 `ProfileNotWritable` |
| Nano 4 Restore 后空库 | 14,314 字节，传统容器，0 tracks | 无 | 全部原字节与 opaque dataset | no-op 精确一致；任意 EditPlan 返回 `ProfileNotWritable` |
| 用户所称 iPod 5.5G 只读库 | 4,034,040 字节，传统容器，1,862 tracks | 无 | 全部原字节与 opaque dataset | no-op 精确一致；任意 EditPlan 返回 `ProfileNotWritable` |

合成可写场景的允许变化只有 EditPlan 点名对象、新 ID、master 派生成员以及必要父长度/count。写入后由全新 Reader、独立 Validator 和 Semantic Comparator 检查；未请求的对象顺序、ID 或引用变化会令 Writer 整体失败，不返回部分字节。

## 未扩大能力

- 本结果不是 Nano 4、Classic、Photo 或任何真实设备的写入验证；
- hash58、设备稳定身份和真实 6G 可写 profile 仍属于任务 004/005；
- Artwork、SoundCheck、gapless、Audiobook 和 Smart Playlist 规则仍为 opaque 或后续任务能力；
- 本任务没有组件部署、候选包或设备写入产物。
