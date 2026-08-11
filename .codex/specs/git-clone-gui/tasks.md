# 实施计划：git-clone-gui

> 阶段：tasks
>
> 状态：执行中
>
> 最近更新：2026-08-11

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

## 覆盖检查

| 行为 | 实现任务 | 验证任务/证据 | 状态 |
|---|---|---|---|
| REQ-001 | TASK-001, TASK-004, TASK-006, TASK-010 | core 列表 + UI | 已完成 |
| REQ-002 | TASK-002, TASK-003, TASK-006, TASK-007 | controller 队列 + 真实 Git | 已完成 |
| REQ-003 | TASK-002, TASK-003, TASK-007, TASK-010 | controller + UI | 已完成 |
| REQ-004 | TASK-001, TASK-004, TASK-005, TASK-011 | Preset/CTest/bundle/README | 已完成 |
| REQ-005 | TASK-009, TASK-010 | card/presentation tests | 已完成 |
| REQ-006 | TASK-009, TASK-010, TASK-011, TASK-018 | snapshot + 人工检查 | 已完成 |
| REQ-007 | TASK-008, TASK-010, TASK-011 | store 往返 + UI 恢复 | 已完成 |
| REQ-008 | TASK-012, TASK-013, TASK-014 | plist/icon alpha + self-contained delivery + launch | 已完成 |
| REQ-009 | TASK-015, TASK-016, TASK-018 | branch service + selector/presentation tests + snapshot | 已完成 |
| REQ-010 | TASK-017, TASK-019 | core + presentation + snapshot/notification/delivery | 已完成 |
| REQ-011 | TASK-020, TASK-021, TASK-022 | Windows/macOS Actions build/test/deploy、签名门控、artifact/Release | 已完成 |

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
