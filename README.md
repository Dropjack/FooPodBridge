# FooPodBridge

FooPodBridge 是面向 Windows x64、foobar2000 2.x 和磁盘模式 click-wheel iPod 的原生设备管理组件。项目建立全新的 x64 架构，只把 `foo_dop / iPod manager` 作为数据库知识和可观察行为参考，不移植其旧 x86、iOS 或历史 UI 结构。

当前目标按数据库家族实现、按证据等级验证 click-wheel iPod。Nano 4 是首台可牺牲实验机；Photo、Classic 和其他历史支持型号按 `StructureKnown / FixtureRoundTrip / DeviceReadVerified / DeviceWriteVerified` 分级，不把参考支持冒充实机验证，未知设备始终拒绝写入。

FooPodBridge 本身是独立组件，FooCrate 不是安装或运行依赖。项目最终提供三种可选界面适配器：

- FooPodBridge 自带 Columns UI Device Panel，面向不使用 FooCrate 的 Columns UI 用户；
- FooPodBridge 自带简化 Default UI Element，面向 Default UI 用户；
- 可选的 `FooCrate` 通过稳定服务接口提供 FooCrate 风格 Devices 界面，不复制设备与数据库逻辑。

三个界面共享 FooPodBridge 的同一套 Core、服务、事务和安全规则。安装 FooPodBridge 不要求安装 FooCrate；卸载或缺少 FooCrate 也不影响 FooPodBridge 的独立能力。

任务 002 已建立并验收 Windows x64 工程、组件身份和服务合同。当前候选只提供服务底座，不显示假设备或提前注册空 UI；数据库、设备发现和三种真实界面按后续任务逐步实现。继续工作请从 [`tasks/README.md`](tasks/README.md) 开始。

## 工作区边界

- 正式项目：`D:\dev\foo\FooPodBridge\FooPodBridge`
- 只读参考：`D:\dev\foo\FooPodBridge\Ref`
- 可选 FooCrate 集成与隔离测试环境：`D:\dev\foo\FooCrate`
- C 盘日常 foobar2000：严格不在项目范围内

## 当前文档

- [项目规则](docs/PROJECT_RULES.md)
- [产品目标](docs/PRODUCT_GOAL.md)
- [正式架构](docs/ARCHITECTURE.md)
- [iPod manager 中文实现蓝图与学习路径](docs/IPOD_MANAGER_IMPLEMENTATION_BLUEPRINT.md)
- [FooCrate 外部消费者验证记录](docs/FOOCRATE_CONSUMER_VALIDATION.md)
- [设备写入安全模型](docs/SAFETY_MODEL.md)
- [产品总规格](specs/PRODUCT_SPEC.md)
- [需要用户决定的事项](decisions/USER_DECISIONS.md)
- [任务入口](tasks/README.md)
