# 007 验证记录

2026-09-22：本轮无 iPod 的实现与测试完成。没有实机、UI 或应用加载验证；不得标记整个任务已验收。

## 构建与回归

- 使用仓库 scripts/build-local.py 调用指定 Visual Studio CMake/CTest，未回退到 PATH 中未知工具。
- x64 Debug、Release ALL_BUILD 均成功，自有 transaction 代码使用 /W4 /WX，无新增警告。
- Debug：13/13 CTest 通过；Release：13/13 CTest 通过。最后输出分别为 5.53 秒与 2.85 秒。
- 既有 device、数据库/hash58、私有 fixture、服务合同、组件身份和源码边界测试全部通过。未启动 foobar2000。

## 新增验证

transaction_core 使用仓库构建目录、Windows 文件 API 和真实 Reader/Writer 生成的合成 iTunesDB，验证正常导入、删除、执行前取消、恢复幂等、历史事务拒绝、自动前/后快照、最近 10 份 + LKG/活动恢复保护、源/备份清单比较和路径拒绝。音频是仅用于文件传输校验的合成字节，不称为可播放 MP3。

transaction_faults 在替换文件系统上运行相同 Core 和真实数据库验证器：

- 三类操作：已有库导入、DB-first 删除、显式初始化。
- 238 个文件变更前/后中断点，包含创建、追加、Flush、重命名、清理与电脑侧恢复证据。
- 970 次恢复过程再次中断的检查；复建 engine 后重复恢复，检查有效库引用不缺文件、旧库证据不丢失、旧音频不提前删除。
- 准备、复制、暂存、提交边界、清理阶段取消；进入重命名后的取消延迟处理。取消初始化之后恢复不会提交新库。
- 计划过期、复制中挂载代次变化、错误设备恢复、全进程写锁、空间不足、短写、创建/写入/Flush/重命名失败、清理失败、数据库及恢复记录损坏、已有库禁止初始化、只读 profile 和 hash58 密钥门禁。
- 正常导入验证媒体只写一次，目标媒体不额外完整读回；数据库仍完整读回验证。

## 保留的电脑目录

Release 的正常测试样本：

`D:/dev/foo/FooPodBridge/FooPodBridge/build/vs2022-x64/transaction-fixture-12984-5214203`

其中 device 是合成设备目录，host 保存事务与快照，source/backup 是合成音频源和对照。目录受 build 忽略规则保护，不进入 Git。之后再次运行测试会创建新的独立目录，不覆盖旧样本。

复现命令（在仓库根目录运行）：

```powershell
python scripts/build-local.py --build build/vs2022-x64 --config Debug --target ALL_BUILD
python scripts/build-local.py test --test-dir build/vs2022-x64 -C Debug --output-on-failure
python scripts/build-local.py --build build/vs2022-x64 --config Release --target ALL_BUILD
python scripts/build-local.py test --test-dir build/vs2022-x64 -C Release --output-on-failure
```

## 处理过的问题

- 首次直接构建遇到 Path/PATH 环境键重复；改用仓库现有 build-local.py 后解决，未修改全局环境。
- C++20 的 u8path 弃用警告通过 char8_t 路径构造解决，未屏蔽警告。
- 最初系统临时目录测试两次出现文件访问拒绝，第二次取得 Win32 错误 5。改在仓库 build 下新建目录后同一文件系统实现正常工作；没有降低重解析点/祖先目录锁定检查。第三次已通过目录访问，后续测试暴露的是独立的历史恢复期望问题。
- 历史恢复测试促使完成标记绑定选定指纹，避免在后续事务之后再次执行旧清理。
- 自动事务快照接入后，独立轮换测试与事务快照发生序号冲突；按设备键分离独立测试数据后通过。

## 证据限制与下一步

内存中断注入保留的是模拟时点文件状态，不模拟真实 FAT32 的扇区撕裂、目录持久顺序、USB 控制器缓存或固件播放；这些必须在独立授权实机任务中验证。损坏/未知记录、部分恢复临时文件等无法证明安全的组合保持 RecoveryRequired 或 CleaningRequired，测试通过不表示所有故障均能自动恢复。

运行中组件仍不消费该写入核心；设备恢复发现器、终端用户恢复入口、完整外部基线枚举/授权绑定和组件包交付尚未完成。当前没有新组件包，不需要用户安装或连接设备。本轮没有提交、推送、部署或改动 FooCrate/Ref。
