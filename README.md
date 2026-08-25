# FooPodBridge

FooPodBridge 是面向 Windows x64、foobar2000 2.x 和磁盘模式 click-wheel iPod 的原生设备管理组件。项目建立全新的 x64 架构，只把 `foo_dop / iPod manager` 作为数据库知识和可观察行为参考，不移植其旧 x86、iOS 或历史 UI 结构。

当前正式目标优先服务并验证项目维护者实际拥有的两台设备：iPod Photo 与 iPod Classic。未识别或未经实机验证的型号不允许写入。

项目最终以两个独立组件协作：

- `FooPodBridge` 提供独立 C++ Core、foobar2000 服务接口、Columns UI Device Panel 和简化 Default UI Element；
- `FooCrate` 通过稳定服务接口提供优先维护的 FooCrate 风格 Devices 界面，不复制设备与数据库逻辑。

项目目前处于产品规格核对阶段，尚未建立 C++ 工程。继续工作请从 [`tasks/README.md`](tasks/README.md) 开始。

## 工作区边界

- 正式项目：`D:\Dev\FooPodBridge\FooPodBridge`
- 只读参考：`D:\Dev\FooPodBridge\Ref`
- FooCrate 集成与隔离测试环境：`D:\Dev\FooCrate`
- C 盘日常 foobar2000：严格不在项目范围内

## 当前文档

- [项目规则](docs/PROJECT_RULES.md)
- [产品目标](docs/PRODUCT_GOAL.md)
- [正式架构](docs/ARCHITECTURE.md)
- [设备写入安全模型](docs/SAFETY_MODEL.md)
- [产品总规格](specs/PRODUCT_SPEC.md)
- [需要用户决定的事项](decisions/USER_DECISIONS.md)
- [任务入口](tasks/README.md)
