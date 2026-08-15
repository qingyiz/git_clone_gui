# 实施计划：git-clone-gui

> 阶段：tasks
>
> 状态：执行中
>
> 最近更新：2026-08-15

## 执行策略

- Required 任务构成最小完整交付；按依赖波次顺序执行，每项验证通过后才勾选。
- CMake 已找到 Qt 5.15.2，本机应完成真实编译、CTest 与 bundle 启动证据；Qt 6 原生构建不在本机伪造。
- 每个任务开始前重读引用的需求、设计与当前代码，并先说明修改范围。
- 不自动安装系统级 Qt、不删除用户目录、不引入 shell 执行。

## 任务列表

- [x] TASK-001：建立核心请求校验与基础 CMake 构建图
  - 类型：required
  - 需求：REQ-001、REQ-004
  - 设计：ARCH-001、ARCH-002；BUILD-001～BUILD-003；DEC-003；PROP-001、PROP-003
  - 单一变更原因：建立可独立测试的输入/命令计划契约，作为后续用例唯一数据入口。
  - 模块/构建单元：主要 `git_clone_core`；辅助顶层 CMake/Preset 与 `test_clone_core`。
  - 架构约束：ARCH-001、ARCH-002 / BUILD-001～BUILD-003；core 仅依赖 QtCore，顶层只负责 Qt 发现、选项和子目录编排，不依赖 Widgets/QProcess。
  - 依赖变化：新增 `git_clone_core -> Qt::Core`、`test_clone_core -> git_clone_core + Qt::Test`；无其他边。
  - 平台/交付物：平台无关核心库与测试；建立 macOS 最终 target 所需的输出约定，但本任务不产生 `.app`。
  - 依赖：无
  - 修改范围：根 `CMakeLists.txt`、`CMakePresets.json`、`cmake/CompilerWarnings.cmake`、`src/CMakeLists.txt`、`src/core/**`、`tests/CMakeLists.txt`、`tests/core/**`；不实现状态机、QProcess 或 UI。
  - 产出：请求/校验/命令计划 API；安全相对路径与已有目录校验；显示 quoting；双 Qt 版本构建入口；表驱动测试。
  - 验证：`cmake --list-presets`；`cmake --preset debug`；`cmake --build --preset debug --target test_clone_core && ctest --preset debug -R clone_core --output-on-failure`；`rg` 确认 core 无 Widgets/QProcess/shell。
  - 实施记录：已新增根 CMake/Preset、target 警告规则、`git_clone_core` 与 `test_clone_core`。`cmake --preset debug` 找到 `/Users/qingyizhu/Qt5.15.2`；`cmake --build --preset debug --target test_clone_core` 成功；CTest `clone_core` 1/1 通过（6 个测试函数）；core 的 Widgets/QProcess/shell 静态扫描无命中。FACT-005 已按新证据回写并重新校验全部规格阶段。

- [x] TASK-002：实现可替换进程契约与两阶段控制器
  - 类型：required
  - 需求：REQ-002、REQ-003
  - 设计：ARCH-001、ARCH-003；BUILD-001；DEC-002、DEC-004；PROP-002、PROP-004
  - 单一变更原因：把父成功门控、互斥、失败与取消语义集中为可测试应用用例。
  - 模块/构建单元：主要 `git_clone_application`；测试 `test_clone_controller`。
  - 架构约束：ARCH-001、ARCH-003 / BUILD-001；application 只依赖 core/QtCore 和抽象 `ProcessRunner`，不得 include Widgets 或具体 Git runner。
  - 依赖变化：新增 `git_clone_application -> git_clone_core + Qt::Core`、`test_clone_controller -> git_clone_application + Qt::Test`。
  - 平台/交付物：平台无关静态库和测试，不产生用户交付物。
  - 依赖：TASK-001。
  - 修改范围：`src/application/**`、`tests/application/**` 及对应 CMake；不实现 QProcess 适配和窗口。
  - 产出：`ProcessRunner` 契约、`CloneController` 状态机、3 秒取消超时、fake runner 测试，覆盖成功、父失败、子失败、重复开始、取消。
  - 验证：构建 `test_clone_controller`；`ctest --preset debug -R clone_controller --output-on-failure`；include/link 边审查。
  - 实施记录：已新增 `ProcessRunner` 抽象、`CloneController` 显式状态机和可配置取消超时；fake runner 覆盖父子成功顺序、父失败门控、子失败、重复 start、输出转发、启动错误、terminate/kill 取消。Qt 5.15.2 编译成功；CTest `clone_core`、`clone_controller` 2/2 通过；application 的 Widgets/QProcess/infrastructure include 扫描无命中。

- [x] TASK-003：接入异步 Git QProcess 适配器
  - 类型：required
  - 需求：REQ-002、REQ-003
  - 设计：ARCH-001、ARCH-003；BUILD-001；DEC-001；PROP-003
  - 单一变更原因：将抽象进程契约连接到真实 Git CLI，而不向内层泄漏 QProcess。
  - 模块/构建单元：`git_clone_infrastructure`。
  - 架构约束：ARCH-001、ARCH-003 / BUILD-001；只实现 ProcessRunner，不决定阶段、不校验业务路径、不删除目录、不依赖 presentation。
  - 依赖变化：新增 `git_clone_infrastructure -> git_clone_application + Qt::Core`；无反向边。
  - 平台/交付物：平台无关适配源码；不单独产生用户交付物。
  - 依赖：TASK-002。
  - 修改范围：`src/infrastructure/**` 与其 CMake；不修改 core 状态规则或 UI。
  - 产出：`GitProcessRunner`，使用 `QProcess::MergedChannels`、结构化参数、异步输出/完成/错误转发、terminate/kill。
  - 验证：编译 target；`rg -n 'system\\(|sh -c|bash -c|startCommand' src` 无命中；代码审查 start 调用签名；应用测试回归。
  - 实施记录：已新增 `git_clone_infrastructure` 与 `GitProcessRunner`；进程使用 merged channels、异步 readyRead、结构化 `m_process.start(program, arguments)`、terminate/kill，只有 FailedToStart 转为启动错误。Qt 5.15.2 target 编译成功，CTest 2/2 回归通过；无 shell API 或 presentation 依赖。

- [x] TASK-004：实现克隆表单与 macOS 应用组合根
  - 类型：required
  - 需求：REQ-001、REQ-002、REQ-003、REQ-004
  - 设计：ARCH-001、ARCH-004；BUILD-001～BUILD-003；DEC-001～DEC-004；PROP-001～PROP-004
  - 单一变更原因：把已验证用例暴露为一个完整可操作的桌面用户旅程。
  - 模块/构建单元：主要 `git_clone_presentation`；组合 target `GitCloneGui`。
  - 架构约束：ARCH-001、ARCH-004 / BUILD-001～BUILD-003；presentation 仅调用 core/application 公共契约，不 include QProcess/具体 runner，app 是唯一对象组合根。
  - 依赖变化：新增 `git_clone_presentation -> git_clone_application + git_clone_core + Qt::Widgets`；`GitCloneGui -> presentation + application + infrastructure + Qt::Widgets`。
  - 平台/交付物：macOS arm64 开发产物预期 `build/debug/bin/GitCloneGui.app`；其他平台仅保持 CMake `WIN32`/普通 executable 兼容，不声明已验证。
  - 依赖：TASK-001、TASK-002、TASK-003。
  - 修改范围：`src/presentation/**`、`src/app/**` 及对应 CMake；不增加配置持久化、凭据管理或自动删除。
  - 产出：单窗口表单、目录选择、动态命令预览、实时日志、状态与进度提示、运行中控件门控、取消和安全关闭。
  - 验证：构建 `GitCloneGui`；静态 include 审查；Qt 环境可用时打开 `.app` 人工检查字段、预览、非法路径、取消和真实/测试仓库旅程。
  - 实施记录：已新增 `git_clone_presentation`、`MainWindow`、唯一组合根和 `GitCloneGui` bundle target。界面包含父/子配置、目录选择、安全预览、校验、状态、无限进度、10,000 block 日志、开始/取消及运行时关闭门控；presentation 无 QProcess/infrastructure include。Qt 5.15.2 完整编译成功，CTest 2/2 回归通过；开发产物精确位于 `build/debug/bin/GitCloneGui.app`，Info.plist 身份/版本正确，主 executable 为 arm64。

- [x] TASK-005：补齐使用文档并核验交付契约
  - 类型：required
  - 需求：REQ-004
  - 设计：BUILD-003；平台与交付矩阵；测试策略
  - 单一变更原因：让用户从当前文件夹可复现配置、构建、测试和启动，并如实记录产物证据/阻塞。
  - 模块/构建单元：文档与 `GitCloneGui` 交付验证；不新增运行时代码模块。
  - 架构约束：遵守 BUILD-001～BUILD-003；文档不把未验证平台或自包含部署描述为已支持。
  - 依赖变化：无。
  - 平台/交付物：macOS arm64 `build/debug/bin/GitCloneGui.app`；安装树 `.app` 非自包含；无发布包。
  - 依赖：TASK-004。
  - 修改范围：`README.md`、必要的 `.gitignore`、规格实施记录；仅在验证发现构建错误时最小修复相关 CMake/源码。
  - 产出：中文使用说明、依赖与 Qt 路径示例、Preset/普通 CMake 命令、运行及限制说明、完整验证记录。
  - 验证：`cmake --list-presets`；`cmake --preset debug && cmake --build --preset debug && ctest --preset debug --output-on-failure`；`verify_delivery.py build/debug/bin/GitCloneGui.app --platform macos --kind desktop-app`；`file`/`open` 冒烟；Qt 6 双版本构建标记未验证。
  - 实施记录：已新增中文 README、`.gitignore` 和真实 Git 本地父子仓库集成测试。Debug/Release configure/build 均成功；两个 preset 的 CTest 均为 3/3（core、controller、真实 Git workflow）；开发与全新安装树 `.app` 均通过 `verify_delivery.py` 非自包含检查；连续两次 `cmake --install` 无错误且第二次全部 Up-to-date；Info.plist bundle ID/版本正确；主程序为 Mach-O arm64；`otool` 确认 Qt 5.15.2 @rpath；开发/安装实例均可由 `open -n` 启动且正常响应 TERM。Qt 6 双版本构建、自包含部署、签名、公证和发布包未验证且未宣称支持。

- [x] TASK-006：把核心克隆请求升级为有序子仓库列表
  - 类型：required
  - 需求：REQ-001、REQ-002
  - 设计：DEC-005；ARCH-002；BUILD-001、BUILD-004；PROP-001、PROP-003
  - 单一变更原因：让 core 用一个有序值模型表达 0～N 个子仓库并生成唯一安全目标。
  - 模块/构建单元：`git_clone_core`、`test_clone_core`。
  - 架构约束：ARCH-001、ARCH-002 / BUILD-001、BUILD-004；core 仅依赖 QtCore，不执行 I/O。
  - 依赖变化：无新增 target 边；修改 core 公开数据结构，application/presentation/tests 后续迁移。
  - 平台/交付物：平台无关静态库；不产生 `.app`。
  - 依赖：TASK-001
  - 修改范围：`src/core/CloneRequest.*`、`tests/core/TestCloneRequest.cpp`；不改 controller/UI/store。
  - 产出：children list、child plans、0/N 校验、编号错误、重复目标检查、全部命令预览测试。
  - 验证：构建/运行 `clone_core`；静态确认无 Widgets/QProcess。
  - 实施记录：`CloneRequest/ClonePlan` 已迁移为有序 children 列表，支持 0～N、编号错误、安全路径和重复目标检查；core 测试 14/14 通过，且无 Widgets/QProcess 依赖。

- [x] TASK-007：让控制器顺序执行父项目与子仓库队列
  - 类型：required
  - 需求：REQ-002、REQ-003
  - 设计：DEC-002、DEC-005；ARCH-003；BUILD-001、BUILD-004；PROP-002、PROP-004
  - 单一变更原因：把单 child 状态转换升级为 0～N 串行队列和 i/N 状态。
  - 模块/构建单元：`git_clone_application`、`test_clone_controller`、`test_git_clone_workflow`。
  - 架构约束：ARCH-001、ARCH-003 / BUILD-001、BUILD-004；application 只依赖 core/runner port，真实流程测试可依赖 infrastructure。
  - 依赖变化：无新增生产 target 边；测试数据迁移为列表。
  - 平台/交付物：平台无关 controller；不产生 `.app`。
  - 依赖：TASK-006
  - 修改范围：`src/application/CloneController.*`、application/integration tests；不改 UI/store。
  - 产出：0 项直达完成、多项顺序、中间失败门控、当前子项状态和真实父+2 子克隆证据。
  - 验证：`clone_controller`、`git_clone_workflow`；失败历史断言。
  - 实施记录：controller 已用 currentChildIndex 串行推进 0～N 队列并显示 i/N；controller 测试 9/9、真实父+2 子 Git workflow 3/3 通过，中间失败不越过后续项。

- [x] TASK-008：新增可测试的 QSettings 配置存储
  - 类型：required
  - 需求：REQ-007
  - 设计：DEC-006；ARCH-005；BUILD-001、BUILD-004；PROP-005
  - 单一变更原因：提供表单配置的持久化/恢复契约，不让 QSettings 泄漏到 UI。
  - 模块/构建单元：主要 `git_clone_infrastructure`；契约位于 `git_clone_application`；测试 `test_configuration_store`。
  - 架构约束：ARCH-001、ARCH-005 / BUILD-001、BUILD-004；application 只声明 port，infrastructure 实现 QSettings adapter。
  - 依赖变化：application 新增 core 类型契约；infrastructure 保持 application/QtCore 边；新增测试边。
  - 平台/交付物：用户域配置，不改变 `.app` 形态。
  - 依赖：TASK-006
  - 修改范围：`src/application/ConfigurationStore.h`、`src/infrastructure/QSettingsConfigurationStore.*`、对应 CMake 与 `tests/infrastructure/**`；不改 MainWindow。
  - 产出：schemaVersion=1、optional load、0/N 数组保存、sync 结果、临时 INI 往返测试。
  - 验证：`test_configuration_store`；QSettings include 边审查；确认无日志 key。
  - 实施记录：新增 ConfigurationStore port 与 QSettings adapter；临时 INI 测试覆盖首次、0/N、顺序/特殊字符、覆盖和无日志/凭据 key，7/7 通过；QSettings 未泄漏到 application/presentation。

- [x] TASK-009：实现可增删和重编号的子仓库卡片组件
  - 类型：required
  - 需求：REQ-005、REQ-006
  - 设计：DEC-007；ARCH-004、ARCH-006；BUILD-001、BUILD-004；PROP-006
  - 单一变更原因：把单张子仓库的字段与交互从 MainWindow 提取为可复用卡片。
  - 模块/构建单元：`git_clone_presentation`、`test_presentation`。
  - 架构约束：ARCH-004、ARCH-006 / BUILD-001、BUILD-004；card 不访问 controller/store，不拥有列表。
  - 依赖变化：presentation 内部新增 `ChildRepositoryCard`；无新 target link 边。
  - 平台/交付物：Qt Widgets 组件；不单独产生用户产物。
  - 依赖：TASK-006
  - 修改范围：`src/presentation/ChildRepositoryCard.*`、presentation CMake、presentation tests；不重写主窗口布局。
  - 产出：字段读写、序号标题、configurationChanged/removeRequested、对象名和卡片样式属性。
  - 验证：组件增删信号/值/序号测试；Qt offscreen 测试。
  - 实施记录：新增独立 ChildRepositoryCard，支持值读写、序号、编辑门控和 change/remove 信号；组件测试 6/6 通过。旧 MainWindow 仅做列表 API 最小兼容，完整重做留给 TASK-010。

- [x] TASK-010：重做双栏界面并接入自动保存恢复
  - 类型：required
  - 需求：REQ-001、REQ-003、REQ-005、REQ-006、REQ-007
  - 设计：DEC-006、DEC-007；ARCH-004～ARCH-006；BUILD-001、BUILD-003、BUILD-004；PROP-005、PROP-006
  - 单一变更原因：把列表、存储和既有克隆用例组合成紧凑现代的完整页面。
  - 模块/构建单元：主要 `git_clone_presentation`；组合 target `GitCloneGui`。
  - 架构约束：ARCH-001、ARCH-004～ARCH-006 / BUILD-001、BUILD-003、BUILD-004；MainWindow 只依赖 ConfigurationStore port，app 注入具体实现。
  - 依赖变化：presentation 使用 application store port；app 组合 QSettings adapter；无 presentation→infrastructure 边。
  - 平台/交付物：macOS arm64 `build/debug/bin/GitCloneGui.app`。
  - 依赖：TASK-007、TASK-008、TASK-009
  - 修改范围：`src/presentation/MainWindow.*`、`src/app/main.cpp`、相关 CMake/presentation tests；不改 runner 安全实现。
  - 产出：双栏、滚动卡片列表、添加/删除、统一 QSS、紧凑错误摘要、全命令预览、300ms 保存、启动恢复、关闭刷新。
  - 验证：presentation tests、snapshot 视觉检查、GUI launch；include/link 边扫描。
  - 实施记录：MainWindow 已重做为 1160×780 双栏，左侧滚动配置与动态卡片、右侧预览/3 行错误摘要/状态/日志/操作；集中 QSS 覆盖卡片、输入和按钮状态。配置通过 port 在 300ms 保存、启动恢复、关闭刷新，0 张与首次 1 张可区分。presentation 测试 14 项通过（snapshot 未指定路径时跳过），Debug 全 CTest 5/5 通过；布局拆为 MainWindow.cpp 323 行与 MainWindowUi.cpp 265 行，依赖扫描无越界。

- [x] TASK-011：更新说明并完成改版交付验证
  - 类型：required
  - 需求：REQ-004、REQ-006、REQ-007
  - 设计：BUILD-003、BUILD-004；测试策略；平台矩阵
  - 单一变更原因：让多子仓库与配置保存行为可复现，并闭环 macOS 产物和视觉证据。
  - 模块/构建单元：README/Spec 与 `GitCloneGui` 交付验证。
  - 架构约束：BUILD-003、BUILD-004；不扩大平台/发布范围。
  - 依赖变化：无。
  - 平台/交付物：Debug/Release 与安装树 macOS arm64 `.app`，非自包含。
  - 依赖：TASK-010
  - 修改范围：`README.md`、规格实施记录；验证失败时仅最小修复相关文件。
  - 产出：多卡使用说明、QSettings/隐私说明、最终测试/截图/bundle 证据。
  - 验证：Debug/Release build + 全 CTest；delivery script；重复 install；launch；snapshot 人工检查。
  - 实施记录：README 已补齐多子仓库、0 项模式、自动保存恢复、QSettings 隐私边界、Preset 构建和运行说明。Debug/Release 全量 CTest 均为 5/5；父项目 + 2 个子仓库的真实 Git 集成流程通过；最终 snapshot 已生成并人工检查；开发树与安装树 `.app` 均通过 delivery 脚本，连续安装第二次全部 Up-to-date；主程序为 Mach-O arm64，实际 `open -n` 启动稳定并可正常退出。静态扫描确认无 shell 拼接，presentation/application/core 依赖边界无越界。

- [x] TASK-012：增加 macOS 应用图标资源与 Bundle 元数据
  - 类型：required
  - 需求：REQ-008
  - 设计：DEC-008；BUILD-003、BUILD-005
  - 单一变更原因：让 Finder、Dock 和 Bundle 使用与应用视觉一致的正式图标。
  - 模块/构建单元：`GitCloneGui` app target 与 `src/app/resources`。
  - 架构约束：BUILD-003、BUILD-005；资源归 app target，不进入业务模块或顶层 CMake。
  - 依赖变化：无新 link 边；app target 新增 `.icns` resource source。
  - 平台/交付物：Debug/Release macOS `.app` 的 `Contents/Resources/GitCloneGui.icns` 与非空 `CFBundleIconFile`。
  - 依赖：TASK-011
  - 修改范围：`src/app/resources/GitCloneGui.svg`、派生 `.icns`、`src/app/CMakeLists.txt`；不修改 UI/克隆/配置代码。
  - 产出：可维护矢量源、多尺寸 icns、CMake Bundle 资源和图标元数据。
  - 验证：重新配置/构建；`plutil` 检查 `CFBundleIconFile`；检查 Resources 文件；`iconutil` 反解尺寸；`open -n` 启动。
  - 实施记录：新增蓝色渐变 “G + Git 分支” SVG 源与 10 个标准尺寸组成的 1.0 MB `.icns`；app target 已将其复制到 `Contents/Resources` 并设置 `CFBundleIconFile=GitCloneGui.icns`。Debug 重新配置/构建成功，`iconutil` 可完整反解 16～1024 像素资源，`open -n` 启动稳定并可正常退出；未修改任何业务/UI 状态代码。

- [x] TASK-013：生成并验证自包含 Qt 安装 Bundle
  - 类型：required
  - 需求：REQ-004、REQ-008
  - 设计：DEC-003、DEC-009；BUILD-002、BUILD-003、BUILD-005；PROP-007
  - 单一变更原因：把轻量开发 Bundle 转换为携带 Qt Framework/plugin 的最终安装产物。
  - 模块/构建单元：`GitCloneGui` 安装规则、macOS deploy install script、README/Spec。
  - 架构约束：BUILD-003、BUILD-005；使用当前 Qt kit 的 `macdeployqt`，不手工复制、不硬编码用户路径到生成产物。
  - 依赖变化：无运行时 target link 边；新增 install-time 工具调用。
  - 平台/交付物：`build/install/GitCloneGui.app`，macOS arm64 自包含 Bundle；不生成 DMG/签名/公证产物。
  - 依赖：TASK-012
  - 修改范围：`src/app/CMakeLists.txt`、`cmake/DeployMacOS.cmake.in`、`README.md`、规格实施记录；验证失败时仅最小修复打包文件。
  - 产出：幂等 install 部署、Framework/plugins、无开发机 Qt 绝对依赖、开发/部署产物说明。
  - 验证：Release configure/build + 全 CTest；全新 install 两次；`verify_delivery.py --require-self-contained`；`otool -L`/bundle 结构；大小对比；`open -n` 启动。
  - 实施记录：新增 install-time `DeployMacOS.cmake`，从当前 Qt CMake/qmake target 精确定位 `/Users/qingyizhu/Qt5.15.2/bin/macdeployqt`，安装复制 Bundle 后执行 `-always-overwrite`；找不到对应工具会配置失败。Release 与 Debug 全量 CTest 均为 5/5；全新安装和第二次幂等安装成功。开发 Bundle 为 1.3 MB，最终 `build/install/GitCloneGui.app` 为 23 MB，包含 6 个 Qt Framework、Cocoa platform plugin、imageformats/styles 等插件与 `.icns`。`verify_delivery.py --require-self-contained` 通过，递归 Mach-O 检查无开发机 Qt 绝对依赖，主程序为 arm64，`open -n` 实际启动并正常退出。README 已区分开发/部署产物并说明未签名、公证限制。

- [x] TASK-014：修复应用图标白色方底
  - 类型：required
  - 需求：REQ-008
  - 设计：DEC-008；BUILD-005；PROP-008
  - 单一变更原因：恢复图标圆角外的透明 alpha，消除 Dock/Finder 白色方形画布。
  - 模块/构建单元：`src/app/resources/GitCloneGui.icns` 与 macOS Bundle 资源。
  - 架构约束：BUILD-005；只替换派生图标，不修改业务、UI 或依赖部署逻辑。
  - 依赖变化：无。
  - 平台/交付物：Debug/Release/安装树 macOS `.app` 的透明背景 `.icns`。
  - 依赖：TASK-013
  - 修改范围：重新生成 `src/app/resources/GitCloneGui.icns`，回写 Spec；不修改 SVG 构图和 C++ 源码。
  - 产出：由 `sips` RGBA 基图生成的多尺寸 `.icns`。
  - 验证：基图 `hasAlpha=yes`；四角 alpha=0；反解 Bundle `.icns` 后重复 alpha 检查；Release build/install、self-contained delivery 与启动回归。
  - 实施记录：根因确认为 Quick Look thumbnail 把透明 SVG 合成到白色画布。改用 macOS `sips` 从原 SVG 直接生成 1024×1024 RGBA PNG，再派生 10 个标准尺寸并重建 `.icns`；SVG 构图、C++ 和部署逻辑均未修改。基图与最终安装 Bundle `.icns` 反解图均为 `hasAlpha=yes`，Swift 像素断言确认四角 alpha 为 `[0.0, 0.0, 0.0, 0.0]`。Release 全量 CTest 5/5、自包含 delivery、重新安装和 `open -n` 启动回归均通过。

- [x] TASK-015：实现独立的远程分支查询服务
  - 类型：required
  - 需求：REQ-009
  - 设计：DEC-010；ARCH-001、ARCH-007；BUILD-001、BUILD-006；PROP-009
  - 单一变更原因：建立不占用克隆 runner 的异步远程 refs 查询契约与 Git 适配器。
  - 模块/构建单元：主要 `git_clone_infrastructure`；契约位于 `git_clone_application`；测试 `test_git_remote_branches`。
  - 架构约束：ARCH-001、ARCH-007 / BUILD-001、BUILD-006；application 不依赖 QProcess/Widgets；infrastructure 只负责查询、解析、排序、缓存和超时，不修改工作树或克隆状态。
  - 依赖变化：application 新增 branch service QtCore 契约；infrastructure 保持 `-> application + Qt::Core`；新增测试边，无生产 target 新 link 边。
  - 平台/交付物：平台无关 QtCore 服务，不单独产生用户交付物；在 macOS arm64 原生测试。
  - 依赖：TASK-014。
  - 修改范围：`src/application/RemoteBranchService.*`、`src/infrastructure/GitRemoteBranchService.*`、对应 CMake、`tests/infrastructure/TestGitRemoteBranchService.cpp`；不改窗口、克隆 controller 或配置格式。
  - 产出：request/cancel、15 秒超时、`GIT_TERMINAL_PROMPT=0`、会话缓存、HEAD/heads 解析和默认/常用排序。
  - 验证：本地临时 Git 仓库的 default/多分支查询、并发/取消/无效 URL；静态确认结构化 `QProcess::start` 且无 shell。
  - 实施记录：新增 `RemoteBranchService` port 与 `GitRemoteBranchService` adapter；每个请求使用独立 QProcess、结构化 `ls-remote --symref` 参数、15 秒超时、禁用终端提示、请求取消、URL 会话缓存和隐私友好的通用错误。解析 HEAD/heads 后按默认、常用 exact/namespace、稳定名称排序。新增本地临时 Git 仓库测试，覆盖 5 分支默认/排序、缓存与无效 URL，CTest `git_remote_branches` 1/1 通过；shell API 扫描无命中。

- [x] TASK-016：把父子分支输入升级为可搜索选择器
  - 类型：required
  - 需求：REQ-009、REQ-001、REQ-005、REQ-007
  - 设计：DEC-011；ARCH-004、ARCH-007；BUILD-004、BUILD-006；PROP-005、PROP-006、PROP-009
  - 单一变更原因：让父项目和每张子卡片复用可编辑、URL 驱动、包含式搜索的分支控件。
  - 模块/构建单元：主要 `git_clone_presentation`；组合 target `GitCloneGui`。
  - 架构约束：ARCH-004、ARCH-007 / BUILD-004、BUILD-006；BranchSelector 只依赖 application port，不解析 Git 输出；MainWindow/Card 保持各自页面/单卡职责；app 注入具体 service。
  - 依赖变化：presentation 继续 `-> application/core/Qt::Widgets`；app 组合新增 branch service，无新跨层 link 边。
  - 平台/交付物：macOS arm64 `build/debug/bin/GitCloneGui.app`；不改变 Bundle 结构。
  - 依赖：TASK-015。
  - 修改范围：`src/presentation/BranchSelector.*`、`ChildRepositoryCard.*`、`MainWindow*`、`AppStyle.cpp`、`src/app/main.cpp`、相关 CMake/tests；不改 clone 命令或 QSettings schema。
  - 产出：450ms debounce、editable combo、默认/常用优先下拉、`MatchContains` popup、手工输入保持、过期结果隔离、运行时禁用。
  - 验证：presentation tests 覆盖父/子 URL 查询、结果选择、包含匹配、乱序/失败保持输入、配置往返；Debug 构建与启动。
  - 实施记录：新增 `BranchSelector` 可编辑 QComboBox，提供 450ms URL debounce、默认/常用优先下拉、`QCompleter` 不区分大小写包含匹配、loading/error tooltip 和请求 ID 过期隔离；URL 变化会清除旧 suggestions 但保留手输文本，空分支时自动采用远程 default。父项目与 `ChildRepositoryCard` 均已复用组件，app 注入独立 service，运行时随配置禁用；QSettings 数据模型/schema 未改。新增 default/搜索、乱序/手输保持、子卡 URL 查询测试，presentation/branch/store 相关 CTest 3/3 通过，presentation 无 QProcess/infrastructure include。

- [x] TASK-017：优化目标目录、页内结果与日志布局
  - 类型：required
  - 需求：REQ-003、REQ-006、REQ-010、REQ-004
  - 设计：DEC-012；ARCH-002、ARCH-004、ARCH-006；BUILD-003、BUILD-004；PROP-010、PROP-011
  - 单一变更原因：修正目标目录语义并让任务结果和大段 Git 输出保持清晰可查。
  - 模块/构建单元：`git_clone_core` 与 `git_clone_presentation` 两个受影响构建单元；这是同一用户反馈链的校验/状态/布局竖切。
  - 架构约束：ARCH-002、ARCH-004、ARCH-006 / BUILD-003、BUILD-004；目录不变量仍归 core；页内状态和 splitter 只归 presentation；不引入自动清理或文件操作适配器。
  - 依赖变化：无新增 include/link/target 边。
  - 平台/交付物：Debug/Release 与安装树 macOS arm64 `.app`，形态不变。
  - 依赖：TASK-016。
  - 修改范围：`src/core/CloneRequest.cpp`、`src/presentation/MainWindow.*`/`MainWindowUi.cpp`/`AppStyle.cpp`、core/presentation tests、`README.md`、Spec 实施记录；不改 controller 状态机或部署脚本。
  - 产出：空目录可用/非空明确错误、无 QMessageBox 的 success/error 状态、结果优先保持、纵向 splitter 与默认至少 280px 日志区。
  - 验证：core 空/隐藏文件/普通文件测试；presentation success/failure、无 QMessageBox、splitter/日志高度与 snapshot；Debug/Release 全 CTest、自包含 install/delivery、launch。
  - 实施记录：core 现允许不存在或已存在空父目标，拒绝普通文件以及含隐藏/系统项的非空目录，错误统一包含“父项目目标目录必须为空”；新增三类回归测试。MainWindow 移除完成/失败 QMessageBox，以 success/error 页内状态卡保留任务消息，并验证完成后的非空目录重新校验不会覆盖成功结果。右侧新增纵向 QSplitter，日志卡最小 280px、默认占主要空间、仍为 NoWrap/10,000 blocks；最终 1160×780 snapshot 已人工检查。README 已说明远程分支口径、空目录和新反馈。Debug/Release 全 CTest 均 6/6，自包含 install delivery 通过（Frameworks、Cocoa plugin、RPATH、Bundle 元数据），安装主程序实际启动后正常响应终止；inspect_structure 显示 MainWindow 361 行，未越过约 420 行预算。

- [x] TASK-018：重绘分支选择器折叠外观
  - 类型：required
  - 需求：REQ-006、REQ-009
  - 设计：DEC-011；ARCH-004、ARCH-006、ARCH-007；BUILD-004、BUILD-006；PROP-012
  - 单一变更原因：消除 macOS 原生 QComboBox 子控件叠加产生的黑色直角边框，使父/子分支框与现有输入视觉一致。
  - 模块/构建单元：`git_clone_presentation` 与 `test_presentation`。
  - 架构约束：ARCH-004、ARCH-006、ARCH-007 / BUILD-004、BUILD-006；只改 BranchSelector paint/state 与相关样式，不修改远程查询、配置模型或 clone 流程。
  - 依赖变化：无新增 include/link/target 边；仅新增 QtGui paint API include。
  - 平台/交付物：macOS arm64 `build/debug/bin/GitCloneGui.app` 与安装树 `.app`，Bundle 形态不变。
  - 依赖：TASK-017。
  - 修改范围：`src/presentation/BranchSelector.*`、`AppStyle.cpp`、`tests/presentation/TestPresentation.cpp`、Spec 实施记录；不改 application/infrastructure/core。
  - 产出：统一 8px 圆角外框、无黑色直角子框、浅分隔线、hover/focus/disabled/loading/error 色和 popup 展开箭头翻转。
  - 验证：presentation tests；1160×780 snapshot 与用户截图对照；Debug/Release 全 CTest；安装树 self-contained delivery 与启动回归。
  - 实施记录：`BranchSelector` 已覆盖 QComboBox 折叠态 paint，使用单一 8px 圆角路径绘制背景和 1/1.5px 边框，右侧只保留浅色分隔线与 1.8px 圆头 chevron；normal/hover/focus/disabled/loading/error 均有独立色值，popup 展开时箭头翻转。未改 branch service、model、配置或 clone 流程。新增右边缘暗色像素断言与展开状态测试，presentation CTest 通过；1160×780 snapshot 与用户截图对照确认原生黑色直角框已消失。Debug/Release 全 CTest 均 6/6，自包含 install delivery、Framework/plugin/RPATH 检查和安装主程序启动回归通过；inspect_structure 显示 BranchSelector 253 行，仍只承担单分支输入交互/视觉职责，依赖边不变。

- [x] TASK-019：为最终克隆结果增加系统通知
  - 类型：required
  - 需求：REQ-010
  - 设计：DEC-013；ARCH-004、ARCH-008；BUILD-003、BUILD-004、BUILD-007；PROP-013
  - 单一变更原因：在父/子 clone 流程最终成功或失败后增加一次对应的非阻塞系统消息，同时保留页内结果反馈。
  - 模块/构建单元：主要 `git_clone_presentation`；组合 target `GitCloneGui` 与 `test_presentation`。
  - 架构约束：ARCH-004、ARCH-008 / BUILD-003、BUILD-004、BUILD-007；MainWindow 只发请求，DesktopNotifier 独立封装 QSystemTrayIcon，app 组合；不得修改 controller outcome 或使用 shell。
  - 依赖变化：presentation 沿用 QtWidgets 并新增 QSystemTrayIcon API；无新 target/link/package 边。
  - 平台/交付物：macOS arm64 Debug/Release 与 `build/install/GitCloneGui.app`，Bundle 形态不变；系统展示受用户通知权限控制。
  - 依赖：TASK-018。
  - 修改范围：`src/presentation/DesktopNotifier.*`、`MainWindow.*`、presentation CMake/tests、`src/app/main.cpp`、README/Spec；不改 controller/core/infrastructure。
  - 产出：最终 Completed/Failed 各一次对应通知、标题/正文/Information/Critical 级别、蓝色 G 图标、系统能力降级、12 秒自动隐藏 tray item；Cancelled 零通知。
  - 验证：presentation Completed/Failed/Cancelled 信号计数、标题与级别；Debug/Release 全 CTest；install self-contained delivery；macOS 启动与人工通知检查（权限允许时）。
  - 实施记录：新增 `NotificationSeverity` 与独立 `DesktopNotifier`，后者仅依赖 QtWidgets，使用蓝色 G 图标、`QSystemTrayIcon` capability check、Information/Critical 映射和 12 秒 tray item 自动隐藏；不支持系统消息时直接返回，不改变页内状态或 controller outcome。`MainWindow` 仅在最终 `Completed`/`Failed` 各发一次“GitCloneGui · 克隆完成/失败”请求，正文沿用 controller 最终消息（成功含父目录与子仓库数，失败含具体阶段与错误），`Cancelled` 不发；`src/app/main.cpp` 作为唯一组合根连接 notifier。presentation 新增成功/失败/取消计数、标题、正文和级别断言，均通过且不会在测试中实际弹通知。Debug/Release 全 CTest 均 6/6；自包含安装 Bundle 通过 Frameworks、Cocoa plugin、RPATH、Bundle 元数据检查，安装主程序可启动并保持事件循环。`inspect_structure` 显示 MainWindow 372 行，仍低于约 420 行预算；Notifier 67 行，未引入 application/core/infrastructure 反向依赖。系统最终是否展示仍由 macOS 通知权限决定，权限关闭时页内结果为可用降级路径。

- [x] TASK-020：补齐 Windows 原生 target 资源与自包含安装树
  - 类型：required
  - 需求：REQ-004、REQ-011
  - 设计：DEC-003、DEC-014；ARCH-002、ARCH-009；BUILD-003、BUILD-008；PROP-001、PROP-014
  - 单一变更原因：把现有“仅保留 WIN32 编译入口”升级为有图标、UTF-8 文本和 Qt/MSVC 运行时闭包的 Windows x64 便携应用。
  - 模块/构建单元：`GitCloneGui` app target、Windows resource、Windows install deploy script。
  - 架构约束：ARCH-002、ARCH-009 / BUILD-003、BUILD-008；平台资源和部署只归 app/cmake；Windows 原生测试暴露的路径分隔符问题只在 core 路径不变量内做最小修复，不引入新 link 边。
  - 依赖变化：无生产 link 边变化；MSVC target compile options 增加 `/utf-8`；Windows app sources 增加 `.rc/.ico`。
  - 平台/交付物：Windows x64 build-tree `build/ci-windows/bin/GitCloneGui.exe`；安装树 `build/ci-windows/install/bin` 含 exe/DLL/plugins/runtime。
  - 依赖：TASK-019。
  - 修改范围：`cmake/CompilerWarnings.cmake`、`cmake/ConfigureAppDeployment.cmake`、`cmake/DeployWindows.cmake.in`、`src/app/CMakeLists.txt`、`src/app/resources/GitCloneGui.ico`、`src/app/WindowsResources.rc.in`；若 Windows 原生测试暴露平台路径差异，可最小修改 `src/core/CloneRequest.cpp` 及对应既有 core 回归，不改 application/infrastructure/presentation 行为。
  - 产出：正确的 APPLE/WIN32/其他平台 target 分支、Windows icon、同 Qt kit 的 `windeployqt` install-time 部署。
  - 验证：本机 CMake 静态/回归；`windows-2022` Release configure/build/CTest/install；断言 exe、Qt DLL、qwindows plugin、compiler runtime。
  - 实施记录：已实现 APPLE/WIN32/其他平台 target 分支，新增 256×256 RGBA `.ico` 与配置生成的 Windows resource，MSVC 增加 `/utf-8`；部署配置抽到 59 行 `ConfigureAppDeployment.cmake`，Windows install script 使用同 Qt kit 的 `windeployqt` 并断言 qwindows plugin。本机 Debug/Release CMake 与 6/6 CTest 均通过，app CMake 保持 46 行且无业务依赖变化。首轮 Windows runner 暴露 `cleanPath` 正斜杠与 `QDir::separator()` 反斜杠混合导致的合法路径逃逸误判；core 统一分隔符并沿用 Windows case-folded identity 后，run `31506923442` 在 Windows x64/Qt 6.8/MSVC 2022 成功编译、6/6 CTest、windeployqt、MSVC runtime 补齐、ZIP 结构断言和 artifact 上传。

- [x] TASK-021：实现 GitHub Actions 双平台打包与可选签名公证
  - 类型：required
  - 需求：REQ-011
  - 设计：DEC-014；ARCH-009；BUILD-005、BUILD-008；PROP-014、PROP-015
  - 单一变更原因：让 GitHub 原生 runner 自动产生可下载的 macOS DMG/Windows ZIP，并按 Secrets 进入严格签名路径。
  - 模块/构建单元：`.github/workflows/release.yml` 与 `scripts/release`；不属于运行时 C++ target。
  - 架构约束：ARCH-009 / BUILD-005、BUILD-008；YAML 只编排，签名/打包细节下沉脚本；build jobs 只读，tag release job 最小写权限；Secrets 只走环境变量/runner temp。
  - 依赖变化：新增 GitHub Actions checkout/upload/download 与 install-qt action（固定 commit SHA）；无应用 link/runtime 依赖。
  - 平台/交付物：`GitCloneGui-macOS-arm64.dmg` 与 `GitCloneGui-Windows-x64.zip`；普通 run 为 artifact，`v*` 为 Release 附件。
  - 依赖：TASK-020。
  - 修改范围：`.github/workflows/release.yml`、`scripts/release/**`、`cmake/DeployMacOS.cmake.in`；不改 UI/克隆/配置代码。
  - 产出：macOS 证书临时 keychain 导入、`macdeployqt` notarization signing、DMG 生成/公证/staple；Windows PFX/signtool；artifact 与 tag Release 编排。
  - 验证：YAML 解析/actionlint（可用时）、shell 语法、PowerShell parser、无 Secret 静态路径、本机 unsigned DMG/Bundle 回归、GitHub 双 runner run。
  - 实施记录：已新增固定 Action commit SHA 的双平台 workflow、macOS 临时 keychain/Developer ID/notarytool/staple 脚本、Windows PFX/signtool 和 portable ZIP 脚本；build jobs 为 contents read，tag-only release job 单独 contents write。Bash `-n`、Ruby YAML 与 actionlint 均通过；本机 Release 6/6 CTest、自包含 delivery、unsigned DMG 创建/挂载/arm64 结构和安装 app 启动均通过。PR #2 run `31506923442` 的 macOS arm64 与 Windows x64 jobs 均完成 Qt 6.8 Release、6/6 CTest、unsigned 降级、平台部署/打包和 artifact 上传；Windows/macOS artifact API 分别报告约 22.7/23.2 MB。真实签名公证按设计仍待 Secrets，不属于无凭据分支的完成声明。

- [x] TASK-022：补齐发布操作说明并闭环线上交付证据
  - 类型：required
  - 需求：REQ-011
  - 设计：DEC-014；ARCH-009；BUILD-008；PROP-014、PROP-015
  - 单一变更原因：让维护者能取得证书、配置 Secrets、触发 tag Release，并让下载者区分 artifact、unsigned 与正式签名包。
  - 模块/构建单元：README/Spec 与 GitHub Actions 交付验证；不产生运行时模块。
  - 架构约束：ARCH-009 / BUILD-008；文档不把未运行的 Windows/Qt6/签名路径描述为已验证。
  - 依赖变化：无。
  - 平台/交付物：GitHub Actions 两个 artifact、`v*` Release 两个附件；签名证据按 Secrets 实际状态记录。
  - 依赖：TASK-021。
  - 修改范围：`README.md`、Spec 实施记录；仅在原生 run 发现问题时最小修复对应 workflow/CMake/script。
  - 产出：下载入口、手动/标签发版、Apple Developer ID + notarytool Secrets、Windows PFX Secrets、证书限制与排错说明。
  - 验证：README 命令/Secret 名与 workflow 一致；提交推送后的 macOS/Windows jobs、artifact 清单；测试标签 Release；有证书时签名工具验证。
  - 实施记录：README 与 workflow 的触发命令、5 个 Apple Secrets 和 2 个 Windows Secrets 已核对一致。合并后普通 push run `31508314759` 在 macOS arm64/Windows x64 完成 Release 构建、6/6 CTest、自包含打包和两个 artifact 上传。首个标签 `v0.1.0` 暴露 release job 未 checkout 时 `gh` 无法推断仓库，PR #3 通过把 `GH_REPO` 设置为 Actions 当前仓库上下文完成最小修复并由双平台检查验证；标签 `v0.1.1` run `31510019384` 随后自动完成两个平台 job 与 `Publish GitHub Release` job。Release `v0.1.1` 已发布约 24.2 MB DMG 与 22.7 MB ZIP，GitHub API 报告两附件状态均为 `uploaded` 且带 SHA-256 digest。当前未配置签名 Secrets，因此本轮证据明确为 unsigned 降级；Developer ID/notary 与 Authenticode 真实证书路径保持条件式能力，不声明已验证。

- [x] TASK-023：修复无 Developer ID 的 macOS Bundle 失效签名
  - 类型：required
  - 需求：REQ-011 / AC-011.3、AC-011.4；NFR-007
  - 设计：DEC-014、DEC-015；ARCH-009；BUILD-005、BUILD-008；PROP-014、PROP-015
  - 单一变更原因：部署新增 Frameworks/PlugIns/Resources 后只保留链接器主程序签名，导致 Safari quarantine 下被 Gatekeeper 误判为“已损坏”。
  - 模块/构建单元：macOS install-time deployment 与 release packaging；不属于运行时 C++ target。
  - 架构约束：ARCH-009 / BUILD-005、BUILD-008；继续使用当前 Qt kit 的 `macdeployqt` 管理嵌套代码签名顺序，YAML 不承载平台实现，不改变 Windows 或应用业务依赖。
  - 依赖变化：无生产/Action/link 依赖变化；无 Secrets 分支为 `macdeployqt` 增加 ad-hoc identity 参数。
  - 平台/交付物：macOS arm64 安装树 `GitCloneGui.app` 与 `GitCloneGui-macOS-arm64.dmg`；Windows x64 产物保持不变。
  - 依赖：TASK-022。
  - 修改范围：`cmake/DeployMacOS.cmake.in`、`scripts/release/package-macos.sh`、`README.md` 与 Spec 证据；不改 `.github/workflows/release.yml`、C++/Qt 源码或 Windows 脚本。
  - 产出：无 Developer ID 时通过 `macdeployqt -codesign=-` 完整重签；打包前无条件严格验证 Bundle；README 区分“结构有效的 ad-hoc 测试包”和“Developer ID + 公证可信包”。
  - 验证：本机 Qt 5.15.2 全新 install、6/6 CTest、self-contained delivery、严格 `codesign`、DMG 挂载与启动；对带 quarantine 的副本确认不再出现签名损坏；GitHub macOS arm64/Qt 6.8 job 与新标签 Release 原生验证。
  - 实施记录：根因由用户 Safari 下载截图与 `v0.1.0` DMG 原生复现确认：文件 SHA-256 正常且运行时闭包完整，但 `codesign --verify --deep --strict` 报资源未封装，主程序只有 `adhoc,linker-signed`。无证书分支现向同 Qt kit 的 `macdeployqt` 传入 `-codesign=-`，打包脚本改为无条件严格验证 Bundle；README 依据 Apple 官方说明区分 ad-hoc、Developer ID、公证与“仍要打开”。本机 Qt 5.15.2 Release 6/6 CTest、自包含 delivery、DMG 内外严格验证和实际启动通过；签名详情为 `Signature=adhoc`、存在 sealed resources。带 Safari quarantine 属性的副本仍通过严格签名结构验证，`spctl` 仅按预期因缺少 Developer ID 拒绝并要求人工覆盖。PR #5 run `31511847361` 在 macOS arm64/Qt 6.8 日志确认 ad-hoc Bundle、`valid on disk` 与 designated requirement，Windows 回归通过；标签 `v0.1.2` run `31512200305` 的 macOS、Windows、Publish GitHub Release 三个 job 全部成功。Release API 报告 DMG 约 24.2 MB、SHA-256 `0a59c74bb4ca2719fa771d8957b197ed4ff38b234c95ec5c239a2b0c646f4633`，Windows ZIP 约 22.7 MB，附件状态均为 `uploaded`。

- [x] TASK-024：让大型仓库实时输出 Git 进度并记录任务总耗时
  - 类型：required
  - 需求：REQ-003 / AC-003.1、AC-003.6、AC-003.7；NFR-003、NFR-011
  - 设计：DEC-001、DEC-016；ARCH-002、ARCH-003；BUILD-001、BUILD-004；PROP-016
  - 单一变更原因：修复 GUI 管道环境中 Git 默认抑制 clone 进度造成的大仓库长时间无日志，并补充整个父+子任务的最终耗时。
  - 模块/构建单元：`git_clone_core` 与 `git_clone_application`；测试 `test_clone_core`、`test_clone_controller`。
  - 架构约束：ARCH-002、ARCH-003 / BUILD-001、BUILD-004；core 只生成结构化参数，application 只管理任务级单调计时，infrastructure 的 readyRead 适配与 presentation 日志控件保持不变。
  - 依赖变化：无新增 target/link/include 边；application 仅新增 QtCore 已提供的 `QElapsedTimer`。
  - 平台/交付物：平台无关运行时行为；macOS/Windows 既有 app 与发布包形态不变。
  - 依赖：TASK-023。
  - 修改范围：`src/core/CloneRequest.cpp`、`src/application/CloneController.*`、`tests/core/TestCloneRequest.cpp`、`tests/application/TestCloneController.cpp` 与 Spec 实施记录；不改 GitProcessRunner、MainWindow、CMake、部署或发布脚本。
  - 产出：所有父/子 clone 命令显式 `--progress`；现有 readyRead 在阶段完成前即可转发进度；Completed/Failed/Cancelled 最终日志各打印一次一位小数总耗时。
  - 验证：构建 `test_clone_core`/`test_clone_controller`；CTest 定向与 Debug 全量 6/6；参数计数、完成前输出顺序、三种 outcome 耗时格式/次数；静态确认无 shell 和无新增跨层依赖；重跑结构检查与 Spec 校验。
  - 实施记录：根因确认是 `GitProcessRunner` 已经通过 `QProcess::readyRead` 实时转发 merged channels，但 Git 在 stderr 非终端时默认抑制 clone 传输进度。`CloneRequest` 现为父项目和每个子仓库的结构化参数各加入且仅加入一个 `--progress`，不引入 shell；`CloneController` 使用 QtCore `QElapsedTimer` 从有效任务准备启动父阶段时开始计时，在唯一 `finish()` 路径为 Completed/Failed/Cancelled 最终日志各追加一次一位小数的“总耗时：N.N 秒”，校验失败不产生耗时。core 测试覆盖父/子参数与元字符安全，controller fake runner 覆盖完成前输出转发、三种 outcome 的耗时格式/次数及无效输入；Debug 与 Release 全量 CTest 均 6/6 通过，真实 Git 父+2 子流程回归通过，两个 preset 的 `.app` 均重新链接成功。`git diff --check` 与 shell API 静态扫描通过，无新增 target/link 或跨层依赖。

- [x] TASK-025：定义工作区仓库与分支操作契约
  - 类型：required
  - 需求：REQ-013 / AC-013.2、AC-013.4～AC-013.7
  - 设计：DEC-020；ARCH-001、ARCH-011；BUILD-001、BUILD-009；PROP-018、PROP-019
  - 单一变更原因：先建立不依赖 Widgets/QProcess 的工作区值对象、分支差集规则与异步 port，供适配器和页面共同依赖。
  - 模块/构建单元：`git_clone_application`。
  - 架构约束：ARCH-001、ARCH-011 / BUILD-001、BUILD-009；application 不 include Widgets、QProcess 或文件系统遍历实现。
  - 依赖变化：无新增 target link 边；application 增加 QtCore 内部类型。
  - 平台/交付物：平台无关；不产生独立交付物。
  - 依赖：TASK-024。
  - 修改范围：`src/application/WorkspaceService.*`、`src/application/CMakeLists.txt` 与契约测试；不实现扫描、Git 进程或 UI。
  - 产出：RepositoryInfo、BranchCatalog、BranchTarget、scan/load/switch/cancel 契约及可测试远端候选计算。
  - 验证：构建 application；差集覆盖本地同名、`*/HEAD`、多 remote 同名和稳定排序；include/link 审查。
  - 实施记录：新增 `WorkspaceService` port 与 `RepositoryInfo`、`BranchCatalog`、`BranchTarget` 值对象；`remoteBranchCandidates` 按 remote 后短名计算差集，排除 `*/HEAD`、去重并稳定排序，同时保留多 remote 的完整来源名。`test_workspace_contract` 覆盖本地同名过滤、无效远端名、多 remote 同名与顺序；Qt 5.15.2 Debug/Release 均通过，application 未 include Widgets/QProcess/文件系统且无新 target link 边。

- [x] TASK-026：实现可取消仓库扫描与异步 Git 分支适配器
  - 类型：required
  - 需求：REQ-013 / AC-013.1～AC-013.9；NFR-012、NFR-013
  - 设计：DEC-018～020；ARCH-011；BUILD-009；PROP-017～PROP-019
  - 单一变更原因：把文件系统与 Git CLI 行为封装为一个实现 WorkspaceService 的基础设施适配器。
  - 模块/构建单元：`git_clone_infrastructure`；测试 `test_git_workspace`。
  - 架构约束：ARCH-011 / BUILD-009；扫描不跟随 symlink/不进入 `.git`，Git 用结构化参数且不执行 fetch/reset/stash/clean。
  - 依赖变化：`git_clone_infrastructure` 私有新增 `${QT_PACKAGE}::Concurrent`；沿用 infrastructure → application 方向。
  - 平台/交付物：平台无关运行时行为；macOS/Windows 产物形态不变。
  - 依赖：TASK-025。
  - 修改范围：顶层 Qt Concurrent 查找、`src/infrastructure/GitWorkspaceService.*`、infrastructure/test CMake、`tests/infrastructure/TestGitWorkspaceService.cpp`；不改 UI 或克隆 runner。
  - 产出：generation 隔离的扫描 worker、嵌套仓库发现、refs 解析、本地/远端 switch、错误与 busy 状态信号。
  - 验证：临时目录根/嵌套/`.git` 文件/symlink/取消测试；真实本地 Git 多分支/多 remote/switch/冲突测试；10,000 目录容量测试。
  - 实施记录：新增 `GitWorkspaceService`；每次扫描使用独立 QtConcurrent watcher、共享 cancel token 与 generation，快速重扫时旧结果不会覆盖新目录，析构会等待 worker。迭代扫描识别 `.git` 文件/目录，排除 `.git` symlink/元数据目录与目录 symlink，发现父仓库后仍遍历普通子目录；结果规范化、去重、按相对路径稳定排序。单异步 QProcess 先读取当前分支再用 `for-each-ref` 读取 heads/remotes，切换使用 `git switch -- <local>` 或 `git switch --track -- <remote>`，显式结束选项且不执行 fetch/reset/stash/clean。真实 Git 测试覆盖根/嵌套/`.git` 文件仓库、本地 switch、远端 tracking switch 和刷新差集；10,000 目录+2 仓库扫描与 2,000 目录快速重扫隔离通过。Qt Concurrent 仅为 infrastructure 私有 link 边。

- [x] TASK-027：把既有克隆界面迁为独立页面并增加导航壳
  - 类型：required
  - 需求：REQ-012、REQ-001～REQ-010 保持行为
  - 设计：DEC-017；ARCH-004、ARCH-010；BUILD-001、BUILD-009；PROP-006、PROP-011、PROP-013、PROP-020
  - 单一变更原因：在接入第二页前分离窗口壳与既有克隆页，并提供两个稳定导航入口。
  - 模块/构建单元：`git_clone_presentation`。
  - 架构约束：ARCH-004、ARCH-010 / BUILD-009；机械迁移保持旧对象名、controller/store/通知行为，MainWindow 不接管页面字段。
  - 依赖变化：无新增 target/link 边；presentation 内部 MainWindow → ClonePage。
  - 平台/交付物：macOS/Windows GUI 页面结构变化；既有 app 名称与产物路径不变。
  - 依赖：TASK-026。
  - 修改范围：`src/presentation/ClonePage.*`、`MainWindow.*`、`MainWindowUi.cpp`（迁移后移除）、AppStyle、presentation CMake/测试；先用占位 WorkspacePage 或注入 QWidget，不实现工作区行为。
  - 产出：固定左侧导航、默认克隆页、状态保持页面栈、关闭取消协调；既有克隆页全行为回归。
  - 验证：旧 presentation 测试全部通过；新增默认页、按钮选中、往返切换、运行中切页、关闭测试；snapshot。
  - 实施记录：既有 `MainWindow` 的克隆页面机械迁为 `ClonePage`/`ClonePageUi`，保留全部控件 objectName、配置 debounce、通知请求、状态/日志与 controller 连接；新 `MainWindow` 173 行，仅拥有 188px 左侧导航、两个 checkable 入口、页面栈和关闭协调。页面实例常驻，往返切换保持表单/日志状态；关闭时工作区操作立即取消，运行中的 clone 仍按既有 cancel 后 closeReady 退出。原 `test_presentation` 全部通过，新增默认克隆页、选中态、往返状态保持测试；默认窗口 snapshot 显示侧栏未压缩旧双栏到不可用。

- [x] TASK-028：实现仓库树与分支快速切换页面
  - 类型：required
  - 需求：REQ-012、REQ-013
  - 设计：DEC-017～020；ARCH-006、ARCH-010、ARCH-011；BUILD-009；PROP-017～PROP-020
  - 单一变更原因：把已验证的工作区 service 组合为完整可操作的第二个页面。
  - 模块/构建单元：`git_clone_presentation` 与 `GitCloneGui` 组合根。
  - 架构约束：ARCH-010、ARCH-011 / BUILD-009；页面不解析 Git 输出或遍历目录，app 是唯一具体 service 组合根。
  - 依赖变化：无新增 target/link 方向；app 构造 GitWorkspaceService 并注入 MainWindow。
  - 平台/交付物：macOS arm64 `.app` 与 Windows x64 `.exe` 增加工作区页面，路径/部署闭包不变。
  - 依赖：TASK-027。
  - 修改范围：`src/presentation/WorkspacePage.*`、`AppStyle.cpp`、`MainWindow.*`、`src/app/main.cpp`、相邻 CMake/test、README 与 Spec 实施记录；不加入 fetch/pull/push/分支删除或持久化工作区。
  - 产出：目录选择与扫描、层级仓库树、当前/本地/完整远端/远端候选展示、本地与远端切换、刷新和错误/忙碌状态。
  - 验证：fake service UI 测试、真实 service 集成、Debug/Release 全 CTest、snapshot、`.app` 构建/启动、自包含安装回归、结构与 Spec 校验。
  - 实施记录：新增 `WorkspacePage` 与独立 `WorkspacePageUi`，提供目录选择/扫描/取消、按相对路径构建的层级仓库树、仓库计数、当前分支徽标、本地/远端待跟踪/全部远端三个列表、刷新、按钮与双击切换、busy/error/success 状态。页面只调用 `WorkspaceService`，app 组合根注入 `GitWorkspaceService`；README 说明扫描边界、remote-tracking refs、差集与无 fetch/stash/reset 行为。fake service 页面测试覆盖树层级、分支分类与两种 target 请求；视觉 snapshot 通过。Debug/Release 均 9/9 CTest，通过真实父+2 子 clone 回归。`build/release/bin/GitCloneGui.app` 开发 Bundle 与 `build/install-v013/GitCloneGui.app` 自包含 Bundle 均通过 delivery 检查；安装 Bundle 含 QtConcurrent/Widgets/Core/Gui 与 Cocoa plugin，严格 `codesign --deep --strict` 通过并以正常 Cocoa 方式保持事件循环运行。结构检查显示 MainWindow 173、WorkspacePage 316、GitWorkspaceService 323 行，无跨层 include、shell/system 或顶层 CMake 职责漂移。

- [x] TASK-029：重绘仓库树层级、图标与选中状态
  - 类型：required
  - 需求：REQ-013 / AC-013.2、AC-013.10～AC-013.12；NFR-014
  - 设计：DEC-019、DEC-021；ARCH-006、ARCH-010；BUILD-009；PROP-017、PROP-021
  - 单一变更原因：修复用户截图中 `◆` 伪图标、原生 branch 区独立蓝块、层级弱和滚动条偏重造成的仓库树视觉问题。
  - 模块/构建单元：`git_clone_presentation`；测试 `test_workspace_presentation`。
  - 架构约束：ARCH-006、ARCH-010 / BUILD-009；RepositoryTree 不访问 service/I/O，WorkspacePage 只设置节点语义和路径角色，样式与图标不泄漏到 application/infrastructure。
  - 依赖变化：无 target/link/include 方向变化；仅新增 presentation 内部 QWidget/delegate 源文件。
  - 平台/交付物：macOS arm64 `.app` 与 Windows x64 `.exe` 的仓库树视觉更新，产物路径和运行时闭包不变。
  - 依赖：TASK-028。
  - 修改范围：`src/presentation/RepositoryTree.*`、`WorkspacePage.*`、`WorkspacePageUi.cpp`、`AppStyle.cpp`、presentation CMake/测试与 Spec 证据；不改 WorkspaceService、GitWorkspaceService、扫描/分支业务和发布脚本。
  - 产出：纯文本节点、typed node roles、自绘 folder/repository/chevron/guide、连续圆角 hover/selection、38px 行高和 ≤8px 轻量滚动条。
  - 验证：node role/text、sizeHint、展开命中、选中行像素连续性；长列表截图与人工视觉比较；Debug/Release 9/9 CTest、delivery/签名/启动回归、结构与 Spec 校验。
  - 实施记录：根据用户截图定位到 macOS 原生 `QTreeWidget::branch` 与 item 分区绘制造成展开区独立深蓝块，同时 `WorkspacePage` 用 `◆` 文本前缀伪装仓库图标。新增 273 行 `RepositoryTree` 与私有 `QStyledItemDelegate`，关闭原生 branch decoration，统一自绘 40px 行、整行 8px 圆角 hover/selection、轻量 guide、圆角 chevron、folder/root 与 Git branch repository 矢量图标；节点通过 `RepositoryNodeKindRole` 表达语义，文本恢复纯目录名，无 emoji、theme icon 或二进制资产。树专属 QSS 使用 7px 透明轨道滚动条。chevron 命中覆盖原 branch 区并只切换展开状态，不改变 current item 或触发分支读取。自动化验证 Root/Directory/Repository role、无 `◆`、sizeHint ≥38、scrollbar ≤8、选中行左右/中部像素连续、chevron 展开及零 loadBranches 副作用；17 仓库长列表 snapshot 显示选中背景连续、层级线与图标清晰、滚动条轻量。Debug/Release 均 9/9 CTest，通过真实 Git 回归；`build/install-v013/GitCloneGui.app` 重新部署后 self-contained delivery 与 `codesign --deep --strict` 通过。结构检查显示 RepositoryTree 273、WorkspacePage 325 行，无新增 target/link 边或跨层依赖。

- [x] TASK-030：持久化并恢复工作区根目录
  - 类型：required
  - 需求：REQ-013 / AC-013.13；NFR-015
  - 设计：DEC-022；ARCH-001、ARCH-012；BUILD-010；PROP-022
  - 单一变更原因：让工作区目录拥有独立、可测试且不污染 CloneRequest schema 的持久化生命周期。
  - 模块/构建单元：`git_clone_application`、`git_clone_infrastructure`、`git_clone_presentation` 与 app 组合根。
  - 架构约束：ARCH-012 / BUILD-010；presentation 只依赖 store port，QSettings key/schema 只归 infrastructure；恢复路径不自动触发扫描。
  - 依赖变化：既有 target 内新增 port/adapter 源；无新 target、Qt component 或反向依赖。
  - 平台/交付物：macOS/Windows 用户域设置增加 `workspace` namespace；应用产物形态不变。
  - 依赖：TASK-029。
  - 修改范围：`src/application/WorkspaceConfigurationStore.h`、`src/infrastructure/QSettingsWorkspaceConfigurationStore.*`、`WorkspacePage.*`、`MainWindow.*`、`main.cpp`、相邻 CMake/tests；不改扫描/Git/克隆配置 schema。
  - 产出：optional load、300ms 最新值保存、关闭刷新、保存失败非阻塞提示、启动恢复但不自动扫描。
  - 验证：临时 INI 首次/往返/覆盖；fake store 恢复/延迟/无自动扫描；Debug/Release CTest。
  - 实施记录：新增仅包含 optional root path 的 `WorkspaceConfigurationStore` port 与 `QSettingsWorkspaceConfigurationStore` adapter，使用独立 `workspace/schemaVersion=1`、`workspace/rootPath` key，不改变 CloneRequest schema。`WorkspacePage` 在构造时恢复输入但不扫描，文本变化以 300ms debounce 只保存最新值，取消操作/析构前补刷；保存失败显示非阻塞错误且保留输入。app 组合根注入系统用户域 store，兼容构造仍可不启用持久化。临时 INI 测试覆盖首次无值、trim 往返、覆盖和不保存仓库清单；fake store UI 测试覆盖恢复无扫描、连续编辑仅保存最新值和失败不清空。Debug/Release 对应测试及全量 CTest 均通过。

- [x] TASK-031：读取并醒目展示工作树改动风险
  - 类型：required
  - 需求：REQ-013 / AC-013.14～AC-013.17；NFR-012、NFR-015
  - 设计：DEC-023；ARCH-011、ARCH-012；BUILD-009、BUILD-010；PROP-023
  - 单一变更原因：让用户在切换分支前看到实时 clean/dirty 状态和分类数量，避免在不知情时携带或冲突改动。
  - 模块/构建单元：`git_clone_application`、`git_clone_infrastructure`、`git_clone_presentation`。
  - 架构约束：ARCH-011、ARCH-012 / BUILD-010；Git porcelain 解析只归 adapter，UI 只消费 typed status；不新增 stash/reset/clean 或 dirty 禁用逻辑。
  - 依赖变化：无新增 target/link/include 方向；现有异步 Git 状态机增加只读 LoadStatus 阶段。
  - 平台/交付物：macOS/Windows 工作区分支详情新增状态提示卡；产物路径与部署闭包不变。
  - 依赖：TASK-030。
  - 修改范围：`WorkspaceService.h`、`GitWorkspaceService.*`、`WorkspacePage.*`/Ui、`AppStyle.cpp`、application/infrastructure/presentation tests、README/Spec；不改 switch 参数、扫描或发布脚本。
  - 产出：四类状态计数、clean 说明、dirty 高对比警示与谨慎切换文案、切换后重新读取。
  - 验证：真实 Git clean/staged/unstaged/untracked/conflict；UI clean/dirty 文案/属性/计数与切换 enabled 同构；snapshot、Debug/Release 全 CTest、delivery/启动、结构与 Spec 校验。
  - 实施记录：`BranchCatalog` 新增 `WorkingTreeStatus` 四类计数与 `hasChanges()`；`GitWorkspaceService` 在 HEAD/refs 后通过既有异步 QProcess 追加结构化 `git -C <repo> status --porcelain=v1 -z --untracked-files=normal`，解析 staged/unstaged/untracked/conflict，切换参数及禁止 fetch/stash/reset/clean 的边界不变。分支详情在当前分支下始终显示状态：clean 为绿色说明，dirty 为橙色高对比卡、左强调线、粗体标题、非零分类数量和谨慎切换文案；dirty 不改变切换按钮 enabled 规则，切换成功继续自动刷新状态。真实 Git 测试覆盖 clean、已暂存、未暂存、未跟踪和 merge conflict；UI 测试覆盖语义属性、文案、计数及 dirty 可切换。生成 `/tmp/git-clone-gui-worktree-warning.png` 并人工确认警示位置/层级。Debug/Release 10/10 CTest 通过，自包含安装 Bundle、严格签名结构与启动回归通过。

- [x] TASK-032：持久化并恢复左侧导航页
  - 类型：required
  - 需求：REQ-012 / AC-012.5；NFR-015
  - 设计：DEC-024；ARCH-001、ARCH-013；BUILD-011；PROP-024
  - 单一变更原因：让关闭前所在页面作为独立启动状态可靠恢复，并对旧/未知配置安全回退。
  - 模块/构建单元：`git_clone_application`、`git_clone_infrastructure`、`git_clone_presentation` 与 app 组合根。
  - 架构约束：ARCH-013 / BUILD-011；MainWindow 只依赖枚举 store port，QSettings key/schema 只归 infrastructure。
  - 依赖变化：既有 target 内新增 navigation port/adapter 源；无新 target、Qt component 或反向依赖。
  - 平台/交付物：macOS/Windows 用户域设置增加 `navigation` namespace；应用产物形态不变。
  - 依赖：TASK-031。
  - 修改范围：`src/application/NavigationConfigurationStore.h`、`src/infrastructure/QSettingsNavigationConfigurationStore.*`、`MainWindow.*`、`main.cpp`、相邻 CMake/tests；不改 clone/workspace schema 或页面业务。
  - 产出：Clone/Workspace 枚举往返、切页即时保存、启动恢复、未知/损坏配置回退 Clone。
  - 验证：临时 INI store 测试；fake store MainWindow 恢复/保存测试；Debug/Release CTest。
  - 实施记录：新增 `NavigationPage::Clone|Workspace` 与 `NavigationConfigurationStore` port；QSettings adapter 只使用 `navigation/schemaVersion=1` 和稳定字符串 `navigation/currentPage=clone|workspace`。MainWindow 构造完成时恢复页面，每次成功切页即时保存，缺失 schema/未知值回退 Clone；app 组合根注入同用户域的独立 store。临时 INI 测试覆盖首次/未知值/双页面往返，presentation fake store 覆盖恢复 Workspace 与切回 Clone 保存，定向测试通过且未修改 clone/workspace key。

- [x] TASK-033：恢复有效工作目录后自动扫描
  - 类型：required
  - 需求：REQ-013 / AC-013.13；NFR-012、NFR-016
  - 设计：DEC-025；ARCH-010、ARCH-013；BUILD-011；PROP-025
  - 单一变更原因：把已保存的有效工作目录直接恢复为可用仓库树，同时保持 UI 首帧和既有取消/重扫语义。
  - 模块/构建单元：`git_clone_presentation` 与 `test_workspace_presentation`。
  - 架构约束：ARCH-010、ARCH-013 / BUILD-011；只排队调用 service，不在 UI 线程遍历目录。
  - 依赖变化：无新增 link/include/target 边。
  - 平台/交付物：macOS/Windows 工作区启动行为变化；产物路径和 Bundle 形态不变。
  - 依赖：TASK-032。
  - 修改范围：`WorkspacePage.*` 和 presentation tests；不修改存储格式、扫描算法或分支状态。
  - 产出：有效恢复路径事件循环后自动 scan 一次；空/无效路径零次并可手工修正；手工扫描和取消行为保持。
  - 验证：QTemporaryDir + fake service 时序、调用次数、busy/cancel 回归；Debug/Release presentation tests。
  - 实施记录：`WorkspacePage::restoreRootPath()` 恢复输入并仅对 exists+isDir+isReadable 返回自动扫描资格；全部信号连接和初始 UI 状态就绪后用 `QTimer::singleShot(0)` 复用 `startScan()`，继续进入既有 worker/busy/cancel/generation 流。临时有效目录 fake service 测试确认事件循环后恰好一次 scan，原无效 `/workspace/restored` 测试确认零次且仍可编辑保存；定向 presentation 回归通过。

- [x] TASK-034：降低递归扫描的逐目录文件系统开销
  - 类型：required
  - 需求：REQ-013 / AC-013.1～AC-013.3、AC-013.18；NFR-013、NFR-016
  - 设计：DEC-026；ARCH-011、ARCH-013；BUILD-009、BUILD-011；PROP-017、PROP-026
  - 单一变更原因：消除每目录 canonical 路径解析与完整 QFileInfoList 分配，在不漏嵌套仓库的前提下降低大树扫描耗时。
  - 模块/构建单元：`git_clone_infrastructure` 与 `test_git_workspace`。
  - 架构约束：ARCH-011、ARCH-013 / BUILD-011；只优化 worker 内部，port/signals/cancel/generation 不变，不默认忽略普通目录、不用 shell。
  - 依赖变化：无新增 link/include/target 边；移除 worker 对 `QSet` 的需求（若无其他使用）。
  - 平台/交付物：平台无关扫描行为；macOS/Windows 应用产物形态不变。
  - 依赖：TASK-033。
  - 修改范围：`GitWorkspaceService.cpp`、`TestGitWorkspaceService.cpp`、README/Spec；不改页面树、Git refs/switch 或部署脚本。
  - 产出：单 worker 低开销 DFS、结果等价回归、10,000 目录 ≤1.5 秒且较约 1.78 秒基线中位数降低 ≥20%、真实 57,327 目录前后计时证据。
  - 验证：正确性/取消/快速重扫测试；三次 QElapsedTimer 性能中位数；真实工作区计时；Debug/Release 全 CTest、delivery/启动、结构与 Spec 校验。
  - 实施记录：基线确认原 worker 在 10,000 目录夹具三次总测试约 1.76～2.26s，真实 `/Users/qingyizhu/workspace` 57,327 目录扫描三次为 18.023/18.013/18.019s、均发现 17 仓库。移除逐目录 canonical/visited；macOS/Linux 私有 helper 通过 `opendir/readdir` 复用 `d_type`，仅 `DT_UNKNOWN` 回退 `lstat`，一次读取同时识别子目录与 `.git` 文件/目录；Windows/其他平台保留 Qt `entryList`。不使用 shell、不忽略普通目录，cancel、不可读计数、symlink 排除、嵌套发现和最终稳定排序不变。优化后 10,000 目录三次扫描中位数 146ms，较约 1.78s 基线降低约 91.8% 且低于 1.5s；真实工作区三次为 5.229/5.052/5.032s，中位数 5.052s，较 18.019s 降低约 72.0%，仍发现相同 17 仓库。完整正确性、Debug/Release 与交付证据见最终验证。

- [x] TASK-035：统一 `0.1.4` 版本并发布带说明的 GitHub Release
  - 类型：required
  - 需求：REQ-011 / AC-011.8～AC-011.9
  - 设计：DEC-027；ARCH-009；BUILD-003、BUILD-005、BUILD-008；PROP-027
  - 单一变更原因：让应用内版本、平台元数据、标签和 GitHub Release 说明一致，并交付当前已验收功能。
  - 模块/构建单元：根/app/presentation、release workflow、release 文档和既有 presentation tests。
  - 架构约束：ARCH-009 / BUILD-003、BUILD-005、BUILD-008；版本只由 CMake project 提供；Release 正文位于文档文件，YAML 不硬编码某一版本正文；不改变业务 target 依赖方向。
  - 依赖变化：app 新增一个私有编译定义；release job 新增既有 checkout action，无新库或 target 边。
  - 平台/交付物：macOS arm64 DMG、Windows x64 ZIP、GitHub Release `v0.1.4`。
  - 依赖：TASK-034。
  - 修改范围：CMake/app/MainWindow/presentation test、README、release workflow、`docs/releases/v0.1.4.md` 和 Spec；不改克隆/扫描/分支业务行为。
  - 产出：侧栏“版本 0.1.4”、一致 Bundle/runtime 版本、中文 Release 正文、提交/main push/tag push 与两平台 Release。
  - 验证：Debug/Release CTest、运行时 UI、Info.plist、自包含交付、workflow YAML/脚本审查、GitHub Actions 三个 job、Release API 正文与附件。
  - 实施记录：根 CMake project 已设为 0.1.4 并以 app 私有编译定义注入运行时；MainWindow 读取 applicationVersion，在侧栏显示“版本 0.1.4”；Debug/Release 编译定义与 Info.plist 的 short/build version 均为 0.1.4。新增 `docs/releases/v0.1.4.md`，release job checkout 标签并优先使用同名说明文件，未来标签缺失文件时回退自动说明。Ruby YAML、Bash 语法和 diff check 通过；Qt 5.15.2 Debug/Release 全部 11/11 CTest 通过，自包含安装 Bundle、ad-hoc 严格签名与 0.1.4 plist 验证通过。首轮标签 run `31720507955` 暴露 Qt 6.8 对 `findChild<RepositoryTree *>` 的元对象静态断言，专用控件补充 `Q_OBJECT` 后本机两套 11/11 回归通过。修复标签 run `31720858555` 的 macOS arm64、Windows x64 与 Publish GitHub Release 三个 job 全部成功；公开 Release `v0.1.4` 正文与说明文件一致。Release API 报告 DMG 23,953,384 bytes、SHA-256 `ef78bea4e1a890c277e3b7e7351327c8c1eae9900564eefd94dcf83cd16bbfb4`，Windows ZIP 22,786,291 bytes、SHA-256 `112a3254c66b8624f1ee41e0f301cdbb7cb62d08a44cd6871ec987bc0fbbcd1a`，两附件状态均为 `uploaded`。

- [x] TASK-036：精简分支页签并增加千级列表搜索
  - 类型：required
  - 需求：REQ-013 / AC-013.4～AC-013.7、AC-013.19～AC-013.20；NFR-017
  - 设计：DEC-020、DEC-028；ARCH-010、ARCH-011；BUILD-009；PROP-018、PROP-019、PROP-028
  - 单一变更原因：移除没有操作价值的完整远端展示，并让数百至上千个可切换分支可通过关键词直接定位。
  - 模块/构建单元：`git_clone_presentation` 与 `test_workspace_presentation`。
  - 架构约束：ARCH-010、ARCH-011 / BUILD-009；只改 WorkspacePage 表示与本地筛选，不修改 WorkspaceService、BranchCatalog、Git refs 读取或 switch 参数。
  - 依赖变化：无新增或移除 target/link/include 方向；presentation 删除一个 QListWidget 成员并增加现有 QtWidgets QLineEdit 交互。
  - 平台/交付物：macOS arm64 `.app` 与 Windows x64 `.exe` 的工作区分支详情交互更新；产物路径、Bundle/ZIP 结构与运行时闭包不变。
  - 依赖：TASK-035。
  - 修改范围：`WorkspacePage.h/.cpp`、`WorkspacePageUi.cpp`、`TestWorkspacePresentation.cpp`、README 与 Spec；不改 application/infrastructure/core、CMake target、部署或发布脚本。
  - 产出：仅本地/远端待跟踪两个页签；共享搜索框、大小写不敏感包含筛选、清空恢复、无匹配禁用切换、新 catalog 沿用筛选词。
  - 验证：presentation 功能测试和 1,000 项 ≤250ms 容量测试；Debug/Release 全 CTest；snapshot；结构、Spec、无跨层依赖审查。
  - 实施记录：`WorkspacePageUi.cpp` 已移除只读“全部远端”列表和页签，分支详情仅保留本地/远端待跟踪两个可操作页签；`BranchCatalog.remoteBranches`、Git refs 读取与候选差集保持不变。新增共享 `workspaceBranchSearch`，按 trim 后关键词对两个列表的原始 `BranchNameRole` 做大小写不敏感包含匹配；隐藏当前项会清除选择并禁用切换，新 catalog 仍复用当前关键词，清空恢复全部项，筛选不调用 service。测试覆盖两页签/旧控件不存在、大小写与空白、无匹配、清空、页签切换、隐藏项不可切换、远端 target 保持、新 catalog 沿用关键词，以及本地/远端各 1,000 项在 250ms 门槛内完成且零 Git 读取。Qt 5.15.2 Debug/Release 全量 CTest 均 11/11 通过；`/tmp/git-clone-gui-branch-search.png` 人工确认搜索框、双页签、列表和底部操作区布局完整。结构检查为 56 个源文件，WorkspacePage 415 行仍低于约 420 行预算；无 CMake、target/link、跨层 include 或发布产物契约变化。

- [x] TASK-037：为分支搜索增加有限错字容忍
  - 类型：required
  - 需求：REQ-013 / AC-013.20；NFR-017
  - 设计：DEC-028、DEC-029；ARCH-010；BUILD-009；PROP-028、PROP-029
  - 单一变更原因：让记忆不完整或误输一至数个字母的用户仍能定位分支，同时限制短词误命中和千级列表计算成本。
  - 模块/构建单元：`git_clone_presentation` 与 `test_workspace_presentation`。
  - 架构约束：ARCH-010 / BUILD-009；匹配算法为 presentation 内无状态纯 helper，WorkspacePage 只管理列表/选择；不访问 WorkspaceService、Git、文件系统或持久化。
  - 依赖变化：现有 presentation target 新增 `BranchNameMatcher.*` 源文件；无新 target、link、Qt component、第三方库或跨层 include。
  - 平台/交付物：macOS arm64 `.app` 与 Windows x64 `.exe` 的工作区搜索交互更新；产物路径、Bundle/ZIP 结构与运行时闭包不变。
  - 依赖：TASK-036。
  - 修改范围：`src/presentation/BranchNameMatcher.*`、`WorkspacePage.cpp`、`WorkspacePageUi.cpp`、presentation CMake、`TestWorkspacePresentation.cpp`、README 与 Spec；不改 WorkspaceService/BranchCatalog、Git refs/switch、core/infrastructure、部署或发布脚本。
  - 产出：包含匹配优先；3～4/5～8/9+ 字符分别容忍 1/2/3 次插入、删除、替换或相邻颠倒；1～2 字符只精确包含；分支名任意连续区域可命中。
  - 验证：matcher 表驱动阈值/编辑类型测试；本地/远端各 1,000 项错字筛选 ≤250ms、零 Git 调用、切换 target 回归；Debug/Release 全 CTest；结构、Spec 和依赖审查。
  - 实施记录：新增 75 行纯 presentation helper `BranchNameMatcher.cpp`，先对 trim/case-fold 文本做包含匹配，未命中且关键词至少 3 字符时，以三行滚动动态规划计算查询对分支名任意连续区域的 Damerau-Levenshtein 最小距离；3～4/5～8/9+ 字符阈值分别为 1/2/3，覆盖插入、删除、替换与相邻颠倒，短词不启用容错。`WorkspacePage::applyBranchFilter()` 只把原 `contains` 替换为 helper 调用，页面选择/隐藏/按钮状态、BranchKind/BranchName、service 与 Git 行为不变；搜索占位文案和 README 已说明有限错字支持。表驱动测试覆盖 trim/大小写、连续区域、四类编辑、短词与三档阈值内外；既有本地/远端各 1,000 项测试改用 `targat`/`TARGTE` 错字，验证仅目标分支可见、远端 target 正确、刷新沿用关键词、250ms 门槛和零 Git 读取。Qt 5.15.2 Debug/Release 全量 CTest 均 11/11 通过，`git diff --check` 通过。结构检查为 58 个源文件，WorkspacePage 416 行仍低于约 420 行，matcher cpp 75 行；presentation CMake 仅登记两个源文件，无新 target/link/component/第三方依赖或跨层 include。

- [ ] TASK-038：发布包含分支搜索改进的 `v0.1.5`
  - 类型：required
  - 需求：REQ-011 / AC-011.10
  - 设计：DEC-027、DEC-030；ARCH-009；BUILD-003、BUILD-005、BUILD-008；PROP-027
  - 单一变更原因：把已验收的双页签与模糊搜索功能以版本一致、说明完整、可下载的双平台 Release 正式交付。
  - 模块/构建单元：根/app/presentation、release 文档、既有测试与 GitHub Actions 发布流水线。
  - 架构约束：ARCH-009 / BUILD-003、BUILD-005、BUILD-008；版本继续只来自 CMake project；不修改发布脚本、业务 target 依赖或平台产物契约。
  - 依赖变化：无新 target、link、Qt component、第三方库或 Actions；仅新增同名 Release 说明。
  - 平台/交付物：`v0.1.5`、macOS arm64 DMG、Windows x64 ZIP 与 GitHub Release。
  - 依赖：TASK-037。
  - 修改范围：根 CMake、README、版本 UI 测试、`docs/releases/v0.1.5.md` 与 Spec；外部操作为提交、PR 合并、标签推送及 Release 验证。
  - 产出：项目/运行时/Bundle/侧栏版本 0.1.5，中文更新说明，main 合并提交、标签和带两个平台附件的最新公开 Release。
  - 验证：Debug/Release 全 CTest、版本字符串与 Info.plist、diff/Spec 检查、PR/main/标签关系、GitHub Actions 三个 job、Release 正文/附件/API。
  - 实施记录：执行中；本地版本、说明与回归完成后提交 PR，线上证据待标签流水线完成后回写。

## 执行波次

| 波次 | 任务 | 并行性 | 完成后仓库状态 |
|---|---|---|---|
| 1 | TASK-001 | 顺序 | 核心规则与基础构建图可测试 |
| 2 | TASK-002 | 顺序 | 两阶段用例可由 fake runner 完整验证 |
| 3 | TASK-003 | 顺序 | 真实 Git 适配可编译并符合安全边界 |
| 4 | TASK-004 | 顺序 | 完整 GUI target 与用户旅程就绪 |
| 5 | TASK-005 | 顺序 | 文档、产物和规格证据闭环 |
| 6 | TASK-006 | 顺序 | core 可表达 0～N 子仓库 |
| 7 | TASK-007, TASK-008, TASK-009 | 可并行但本次顺序 | 队列、存储和卡片组件分别可测 |
| 8 | TASK-010 | 顺序 | 完整改版 GUI 可用 |
| 9 | TASK-011 | 顺序 | 新需求交付证据闭环 |
| 10 | TASK-012 | 顺序 | 开发 Bundle 具备正式 macOS 图标 |
| 11 | TASK-013 | 顺序 | 安装 Bundle 自包含 Qt 运行时并完成交付闭环 |
| 12 | TASK-014 | 顺序 | Dock/Finder 图标无白色方底且交付回归通过 |
| 13 | TASK-015 | 顺序 | 远程分支查询 port/adapter 可独立验证 |
| 14 | TASK-016 | 顺序 | 父子分支均可选择、手输和搜索 |
| 15 | TASK-017 | 顺序 | 目录、页内结果与大日志体验完成并交付回归 |
| 16 | TASK-018 | 顺序 | 分支选择器无原生黑色直角边框且视觉回归通过 |
| 17 | TASK-019 | 顺序 | 最终成功/失败各产生一次对应系统通知且取消不误报 |
| 18 | TASK-020 | 顺序 | Windows app target 与自包含安装树契约就绪 |
| 19 | TASK-021 | 顺序 | 双平台 artifact 与 tag Release 流水线就绪 |
| 20 | TASK-022 | 顺序 | 发布文档与 GitHub 原生交付证据闭环 |
| 21 | TASK-023 | 顺序 | ad-hoc Bundle 签名结构有效且新 Release 不再被判为“已损坏” |
| 22 | TASK-024 | 顺序 | 大仓库父/子 clone 实时产出进度且三种最终结果均记录一次总耗时 |
| 23 | TASK-025 | 顺序 | 工作区跨层数据与操作契约稳定且分支差集可测试 |
| 24 | TASK-026 | 顺序 | 嵌套扫描与真实 Git 分支操作可独立验证 |
| 25 | TASK-027 | 顺序 | 主窗口成为导航壳且既有克隆旅程无回归 |
| 26 | TASK-028 | 顺序 | 仓库工作区页面完整接线并通过交付回归 |
| 27 | TASK-029 | 顺序 | 仓库树图标、层级和选择视觉统一且无原生 branch 割裂 |
| 28 | TASK-030 | 顺序 | 工作目录可在重启后恢复；当时不自动扫描，后由 TASK-033 扩展 |
| 29 | TASK-031 | 顺序 | 分支详情实时显示 clean/dirty 风险并完成回归交付 |
| 30 | TASK-032 | 顺序 | 左侧导航页可持久化并安全恢复 |
| 31 | TASK-033 | 顺序 | 有效恢复目录在启动后自动扫描一次 |
| 32 | TASK-034 | 顺序 | 大目录扫描额外开销降低且保持完整发现 |
| 33 | TASK-035 | 顺序 | `v0.1.4` 版本、说明和两平台附件在 GitHub Release 一致交付 |
| 34 | TASK-036 | 顺序 | 分支详情只保留两个可操作页签，千级分支可直接搜索并安全切换 |
| 35 | TASK-037 | 顺序 | 分支搜索容忍有限错字且保持短词精度和千级响应性 |
| 36 | TASK-038 | 顺序 | `v0.1.5` 版本、说明和两平台附件在最新 GitHub Release 一致交付 |

## 覆盖检查

| 行为 | 实现任务 | 验证任务/证据 | 状态 |
|---|---|---|---|
| REQ-001 | TASK-001, TASK-004, TASK-006, TASK-010 | core 列表 + UI | 已完成 |
| REQ-002 | TASK-002, TASK-003, TASK-006, TASK-007 | controller 队列 + 真实 Git | 已完成 |
| REQ-003 | TASK-002, TASK-003, TASK-007, TASK-010, TASK-024 | core 参数 + controller + UI | 已完成 |
| REQ-004 | TASK-001, TASK-004, TASK-005, TASK-011 | Preset/CTest/bundle/README | 已完成 |
| REQ-005 | TASK-009, TASK-010 | card/presentation tests | 已完成 |
| REQ-006 | TASK-009, TASK-010, TASK-011, TASK-018 | snapshot + 人工检查 | 已完成 |
| REQ-007 | TASK-008, TASK-010, TASK-011 | store 往返 + UI 恢复 | 已完成 |
| REQ-008 | TASK-012, TASK-013, TASK-014 | plist/icon alpha + self-contained delivery + launch | 已完成 |
| REQ-009 | TASK-015, TASK-016, TASK-018 | branch service + selector/presentation tests + snapshot | 已完成 |
| REQ-010 | TASK-017, TASK-019 | core + presentation + snapshot/notification/delivery | 已完成 |
| REQ-011 | TASK-020, TASK-021, TASK-022, TASK-023, TASK-035, TASK-038 | 版本一致性、Windows/macOS Actions build/test/deploy、签名门控、artifact/Release 正文与附件、ad-hoc Bundle 严格验证 | 执行中 |
| REQ-012 | TASK-027, TASK-028, TASK-032 | navigation store/presentation、snapshot、运行中切页/关闭 | 已完成 |
| REQ-013 | TASK-025, TASK-026, TASK-028～TASK-031, TASK-033～TASK-034, TASK-036～TASK-037 | contract、扫描/Git 集成与性能、工作目录存储/自动扫描、工作树风险、容错分支搜索、页面 fake service、自绘树与全量交付回归 | 已完成 |

## 完成门槛

- [x] TASK-006～TASK-011 全部完成。
- [x] REQ-001～REQ-007 与 PROP-001～PROP-006 均有证据。
- [x] Debug/Release 全测试、真实父+2 子 Git 流程通过。
- [x] 新 UI snapshot、默认/最小尺寸和 macOS `.app` 启动通过。
- [x] 配置 0/N 往返、关闭刷新和隐私说明已验证。
- [x] TASK-012～TASK-013 全部完成。
- [x] REQ-008 / PROP-007 有 icon、依赖闭包与启动证据。
- [x] 安装树 `.app` 通过 `--require-self-contained`，且 README 区分开发与部署产物。
- [x] TASK-014 完成，图标基图与 Bundle `.icns` 的四角 alpha 均为 0。
- [x] TASK-015～TASK-017 完成，REQ-009～REQ-010 与 PROP-009～PROP-011 均有证据。
- [x] 分支查询、可搜索选择、空目录、页内结果和日志高度回归通过。
- [x] TASK-018 完成，PROP-012 有自动化与 snapshot 证据。
- [x] TASK-019 完成，PROP-013 与系统通知降级行为有证据。
- [x] TASK-020～TASK-022 全部完成。
- [x] Windows x64 与 macOS arm64 GitHub runner 均完成 Release 构建、全 CTest 和自包含产物检查。
- [x] 普通 run 提供两个 artifact，测试 `v*` 标签提供包含 DMG/ZIP 的 GitHub Release。
- [x] 无 Secrets 的 unsigned 降级有明确证据；真实 Developer ID/notary 与 Authenticode 验证保持为配置对应 Secrets 后的条件式路径。
- [x] TASK-023 完成，macOS ad-hoc Bundle 严格签名验证、quarantine 等价检查和新标签 Release 原生验证均通过。
- [x] TASK-024 完成，PROP-016 有父/子 `--progress`、完成前输出转发和三种 outcome 总耗时证据。
- [x] TASK-025～TASK-028 全部完成。
- [x] REQ-012～REQ-013 与 PROP-017～PROP-020 均有自动化和交付证据。
- [x] 既有 REQ-001～REQ-011 全量回归通过，克隆页面对象名、配置、通知、运行中关闭语义无漂移。
- [x] TASK-029 完成，AC-013.10～AC-013.12 / PROP-021 有交互、像素与 snapshot 证据。
- [x] TASK-030～TASK-031 完成，AC-013.13～AC-013.17 / PROP-022～PROP-023 有存储、真实 Git、UI 语义与交付证据。
- [x] TASK-032～TASK-034 完成，AC-012.5、AC-013.13、AC-013.18 / PROP-024～PROP-026 有导航存储、启动自动扫描、性能与交付证据。
- [x] TASK-035 完成，AC-011.8～AC-011.9 / PROP-027 有版本、UI、Bundle、Actions 与 Release API 证据。
- [x] TASK-036 完成，AC-013.19～AC-013.20 / PROP-028 有双页签、筛选正确性、千级容量与 snapshot 证据。
- [x] TASK-037 完成，AC-013.20 / PROP-029 有编辑类型、阈值内外、短词精度和 2,000 项模糊容量证据。
- [ ] TASK-038 完成，AC-011.10 / PROP-027 有 0.1.5 版本、main、标签、Actions 与 Release API 证据。
