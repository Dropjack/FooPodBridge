# FooCrate 外部消费者验证记录

- 验证日期：2026-08-29
- 对应任务：[`tasks/002-建立x64组件工程与服务合同`](../tasks/002-建立x64组件工程与服务合同/README.md)
- 结论：FooCrate 能以独立组件身份编译 FooPodBridge ABI 1.0 公开合同；FooPodBridge 不依赖 FooCrate
- 仓库状态：验证代码已从 FooCrate 工作区移除且从未推送；本文件是保留在 FooPodBridge 仓库中的正式证据

## 为什么做这项验证

FooPodBridge 通过 foobar2000 service 发布设备能力。公开合同只有被另一个独立组件实际包含、编译和链接过，才能证明它不是只在提供方工程中偶然可用。FooCrate 是计划中的可选消费者，因此任务 002 用它做了一次外部编译验证。

这不建立产品依赖：

- 不安装 FooCrate 时，FooPodBridge 仍独立安装、加载并发布服务；
- FooPodBridge 后续自带 Columns UI Device Panel 和 Default UI Element；
- FooCrate 缺失不会改变 FooPodBridge Core、设备服务或写入行为；
- FooPodBridge 缺失时，未来的 FooCrate 集成必须隐藏 Devices，其他功能继续工作。

## 临时验证夹具

验证期间只在本机 FooCrate 工作区建立以下临时内容：

1. 逐字节复制 FooPodBridge 的 `service_v1.h`、`service_guids.cpp` 和 0BSD 许可证；
2. 在 CMake 配置阶段锁定三份快照的 SHA-256，拒绝静默漂移；
3. 建立独立的 `foocrate_foopodbridge_contract_tests` 可执行测试目标；
4. 建立一个纯兼容性分类函数，不加入 `foo_crate.dll` 的源码列表；
5. 分别构建并运行 Debug、Release 测试。

批准快照的 SHA-256：

| 文件 | SHA-256 |
| --- | --- |
| `include/foopodbridge/service_v1.h` | `A39D48C57E53C6C2AA2BE8E75738DFDB76DF88CCC9E0D9D1DD96772090F03DC6` |
| `src/contract/service_guids.cpp` | `C5B4AB4B9B4044930A725DD1CAB0061AD8ADF77826A9171C11EB49CA785F06E0` |
| `LICENSES/0BSD.txt` | `D734C241CFF10A35242A1BF8320C85B1CEAA04DDB935496608198700A7C02EEE` |

## 测试具体证明了什么

编译期和运行期断言覆盖：

- 合同 major/minor 为 `1.0`；
- `service_v1` 与 `device_provider_v1` 继承 foobar2000 `service_base` 且保持抽象接口；
- `service_v1::class_guid` 能从独立 GUID 翻译单元正确链接，并且不是全零 GUID；
- 找不到服务时分类为 `unavailable`，设备 UI 必须隐藏；
- 找到 ABI major 1 时分类为 `available`，允许消费者使用设备能力；
- 遇到未来或不兼容 major 时分类为 `incompatible_major`，设备 UI 必须隐藏。

该验证没有执行服务发现、设备枚举或设备 I/O，也没有构建、覆盖、打包或部署新的 `foo_crate.dll`。

## 构建与结果

FooCrate 原 `build/vs2022-x64` 含旧来源路径缓存，验证保留该目录不动，使用隔离目录 `build/foopodbridge-contract`。使用 Visual Studio Build Tools 2022 自带 CMake/CTest 与 MSVC 19.44：

- Debug `foocrate_foopodbridge_contract_tests`：构建成功；
- Debug CTest `foopodbridge_contract_consumer`：1/1 通过；
- Release `foocrate_foopodbridge_contract_tests`：构建成功；
- Release CTest `foopodbridge_contract_consumer`：1/1 通过。

随后在 `foobar-dev` 的同一 foobar2000 进程中，从批准的 `profile/user-components-x64` 目录同时观察到现有 `foo_crate.dll` 与候选 `foo_pod_bridge.dll`，并通过 `/exit` 正常退出。这证明两个发布 DLL 可以共存，但不表示 FooCrate 已实现 Devices UI。

## 为什么不把测试提交到 FooCrate

任务 002 结束时，FooCrate 的产品代码尚未接入 FooPodBridge，也没有 Devices UI。把只为提前证明 ABI 而建立的快照和测试推入公共 FooCrate 仓库，会让两个项目的发布边界显得比实际更耦合。

因此用户决定：

- FooCrate 本地验证提交未推送，并已把该仓库精确恢复到当时的 `origin/main`；
- 验证方法、哈希和结果只保存在 FooPodBridge 仓库；
- 等真正开始 FooCrate Devices 集成任务时，再由该任务在 FooCrate 中引入已批准合同和永久消费者代码；
- 不使用 FooCrate 的用户路线继续由 FooPodBridge 自带 UI 任务独立推进。

## 后续复验条件

出现以下任一情况时，应重新运行外部消费者验证，不能只引用本记录：

- ABI major、接口 GUID 或现有虚函数布局变化；
- 更换 foobar2000 SDK 基线；
- FooCrate 正式开始发现或调用 FooPodBridge 服务；
- 新增 Columns UI 或 Default UI 消费者并需要验证同一合同。
