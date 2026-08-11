# 设计文档：git-clone-gui

> 阶段：design
>
> 工作流：requirements-first
>
> 设计深度：high
>
> 状态：已更新
>
> 最近更新：2026-08-11

## 设计摘要

- 目标：在既有可保存、多仓库、自包含 macOS 应用上增加远程分支搜索，并优化目录、结果和日志体验。
- 覆盖行为：REQ-001～REQ-010。
- 核心方案：application 新增独立远程分支查询 port；infrastructure 用独立 `QProcess` 并发执行 `git ls-remote --symref`；presentation 用可编辑 `BranchSelector` 提供下拉和包含式搜索；core 允许已存在空目标目录；执行区改为可拖动纵向 splitter 与页内结果状态。
- 模块/构建边界：ARCH-001～ARCH-008 / BUILD-001～BUILD-007。

## 代码库调查

| 证据类型 | 证据 | 已验证事实 | 对设计的影响 |
|---|---|---|---|
| 当前结构 | `inspect_structure.py` | 实施后 29 个源文件；MainWindow 361 行、UI 构建 284 行；五层 target 保持 | 新 service/selector 未使 MainWindow 越过约 420 行预算，资源/部署仍归 app target |
| 当前 core | `CloneRequest.h/.cpp` | 已使用 `QList<ChildRepositoryRequest/Plan>` | 本轮不修改 |
| 当前 application | `CloneController.cpp` | 已用 currentChildIndex 串行推进 0～N 子队列 | 本轮不修改 |
| 当前 presentation | snapshot、`MainWindow*.cpp` | 双栏卡片界面与集中 QSS 已完成 | 图标沿用其蓝色 “G” 视觉语言 |
| 当前 infrastructure | `GitProcessRunner`、`QSettingsConfigurationStore` | Git 与配置适配均已完成 | 本轮不修改 |
| 工具链 | CMake cache/既有回归 | Qt 5.15.2、macOS arm64 可构建 | 使用 Qt 5.15/6 公共 API |
| 当前 Bundle | `du`、`plutil`、`find`、`otool` | Debug 约 524 KB；`CFBundleIconFile` 为空；无 Resources/Frameworks/PlugIns | 现状只是开发构建物，不是最终部署产物 |
| 部署工具 | `/Users/qingyizhu/Qt5.15.2/bin/macdeployqt -h` | 当前 kit 的官方工具可收集 Framework 与插件 | 安装阶段调用对应 kit 工具生成自包含 Bundle |
| 分支输入 | `MainWindowUi.cpp`、`ChildRepositoryCard.cpp` | 父/子分支均为普通 `QLineEdit`，URL 变化不触发远程查询 | 新增独立可编辑选择组件，父/子复用 |
| 远程查询 | `ProcessRunner`、`GitProcessRunner` | 克隆 runner 是单进程互斥状态机，不能承载并发分支查询 | 新建 service port/adapter，每次查询独立 QProcess，不改变克隆状态 |
| 目录校验 | `CloneRequest.cpp` | `QFileInfo::exists(parentTarget)` 一律拒绝，包括空目录 | 改为仅拒绝文件和非空目录 |
| 结果与布局 | `MainWindow.cpp`、`MainWindowUi.cpp` | 完成使用 `QMessageBox`；右栏顺序固定，preview 最高 170px 后日志获得余量 | 页内状态卡 + 纵向 splitter，日志默认至少 280px |

### 工具链与兼容性基线

| 项目 | 已验证值 | 证据 | 设计结论 |
|---|---|---|---|
| OS/架构 | macOS 15.7.5 arm64 | `sw_vers`、`uname -m` | required 产物仍为 arm64 `.app` |
| 构建 | CMake 3.27.1、Ninja 1.11.1、Clang 17 | 版本命令 | Preset/输出路径不变 |
| Qt | Qt 5.15.2 已验证；Qt 6 兼容未原生验证 | CMake cache | 仅用 Core/Widgets/QSettings 公共 API |
| macOS 部署 | `macdeployqt` 来自当前 Qt 5.15.2 kit | 工具帮助与 qmake query | Release 安装树可原生部署并验证 self-contained |
| Git 分支广告 | Git 2.44.0、本地 `ls-remote --symref`、官方文档 | HEAD symref 和 heads 可读取；未 fetch 的对象不能按 `committerdate` 排序 | 默认与常用工作分支族优先，不做虚假的最近提交排序 |

## 约束与设计原则

- 保留 `program + QStringList arguments`，预览与执行分离。
- 子仓库列表是有序值对象；运行时复制快照，UI 后续变化不影响任务。
- presentation 只依赖 application/core 契约，不直接 include QProcess 或具体 QSettings 类。
- QSettings 只保存表单字段和 schemaVersion，不保存日志、任务状态或凭据对象。
- MainWindow 负责页面级布局/状态，单卡字段、标题、删除事件归 `ChildRepositoryCard`。
- 不引入 QML、第三方主题库、拖拽排序或破坏性目录清理。
- 图标保留 SVG 源文件并提交生成的 `.icns`；开发 Bundle 可保持轻量，安装树才执行 Qt runtime 部署。
- 分支查询不复用克隆 runner、不触发 shell、不持久化分支清单；请求 ID、超时和 URL 快照共同隔离过期结果。
- 结果反馈优先于克隆完成后自动发生的重新校验；用户再次编辑任一配置时才恢复校验视觉状态。

## 方案比较

| 方案 | 需求覆盖 | 优点 | 代价与风险 | 结论 |
|---|---|---|---|---|
| A：Qt Widgets 自定义卡片 + QSS + QSettings | 全覆盖 | 不增依赖、Qt 5/6 兼容、可复用既有逻辑 | QSS 需要视觉回归 | 采用 |
| B：迁移 QML | 全覆盖 | 动态列表和动画方便 | 重写表示层、部署增加 QtQuick，范围过大 | 否决 |
| C：MainWindow 内动态创建全部字段并直接 QSettings | 功能覆盖 | 初始文件少 | MainWindow 膨胀、依赖反转、难测试 | 否决 |
| D：每次 POST_BUILD 都运行 `macdeployqt` | REQ-008 | build tree 立即自包含 | 每次增量构建都复制/改写 Framework，慢且污染开发产物 | 否决 |
| E：安装阶段运行当前 kit 的 `macdeployqt` | REQ-008 | 区分开发与部署产物，符合 CMake install 语义 | 最终交付需多执行一次 install | 采用 |
| F：通用 `git ls-remote --symref` + 默认/常用排序 | REQ-009 | 支持 SSH/HTTPS/本地等 Git URL，不下载对象，不绑定托管平台 | 不能获知最近提交时间 | 采用 |
| G：GitHub/GitLab API | REQ-009 | 可取得平台特有活跃度元数据 | URL 识别、Token、分页和多平台兼容显著扩大范围 | 否决 |
| H：浅抓取所有分支后按提交时间排序 | REQ-009 | 可得到真实提交时间 | 大仓库/多分支网络与临时磁盘成本高，违背轻量查询 | 否决 |

### DEC-001：以结构化 QProcess 参数执行 Git

- 上下文与需求：REQ-002、REQ-003、NFR-001。
- 决策：沿用 `ProcessRunner::start(ProcessCommand)` 和 `QProcess::start(program, arguments)`。
- 理由：多子仓库只增加命令数量，不改变安全边界。
- 代价：预览单独 quoting。
- 被否决方案：shell/system/单字符串执行。

### DEC-002：状态机使用父阶段加有序子队列

- 上下文与需求：REQ-002、REQ-003、NFR-002。
- 决策：状态仍为 Idle/CloningParent/CloningChild/Cancelling；controller 额外持有 `currentChildIndex`，每次成功推进一项，0 项直接完成。
- 理由：避免状态枚举随子仓库数量扩张，且易用 fake runner 测试。
- 代价：状态文本必须携带 i/N 上下文。
- 被否决方案：为每个子仓库创建 controller 或并行克隆。

### DEC-003：Qt 6 优先、Qt 5.15 回退

- 上下文与需求：REQ-004、NFR-004。
- 决策：保留 versionless CMake 查找和公共 API。
- 理由：现有 Qt 5.15.2 验证环境稳定。
- 代价：不用 Qt 6 专属 UI/部署 API。
- 被否决方案：硬编码单一 Qt 路径。

### DEC-004：失败保留磁盘内容

- 上下文与需求：REQ-003。
- 决策：队列任一项失败/取消都停止且不删除文件。
- 理由：避免数据丢失。
- 代价：用户需手工处理部分目录。
- 被否决方案：自动递归清理。

### DEC-005：核心请求与计划采用有序列表

- 上下文与需求：REQ-001、REQ-002、REQ-005。
- 决策：`CloneRequest.children` 为 `QList<ChildRepositoryRequest>`；`ClonePlan.children` 为 `QList<ChildClonePlan>`，每项拥有 request 对应命令与绝对目标。
- 理由：保持输入顺序、单次校验、预览和队列执行使用同一事实源。
- 代价：现有单 child API 与测试需迁移。
- 被否决方案：在 UI 循环多次调用单 child controller。

### DEC-006：通过 application 存储契约注入 QSettings

- 上下文与需求：REQ-007、NFR-005、NFR-006。
- 决策：application 定义 `ConfigurationStore`（load optional / save bool）；infrastructure 实现 `QSettingsConfigurationStore`；composition 注入 MainWindow。
- 理由：UI 可用 fake store 测试，QSettings 不跨层泄漏。
- 代价：增加一个契约和适配器。
- 被否决方案：MainWindow 直接构造 QSettings、JSON 文件路径硬编码。

### DEC-007：双栏卡片式 Widgets 视觉系统

- 上下文与需求：REQ-005、REQ-006。
- 决策：左栏是可滚动配置区（父卡、子卡列表、添加按钮、目标卡）；右栏是固定执行区（摘要、预览、状态、日志、操作按钮）。统一浅色背景、白卡、12px 圆角、蓝色主色和等宽输出。
- 理由：控制窗口高度并建立视觉层级；动态列表不会挤压日志。
- 代价：需要 QWidget snapshot/人工视觉验证。
- 被否决方案：继续使用 QGroupBox 单列堆叠。

### DEC-008：应用图标使用可编辑 SVG 源与 Bundle `.icns`

- 上下文与需求：REQ-008 / AC-008.1、AC-008.2。
- 决策：在 `src/app/resources` 保存蓝色圆角 “G + Git 分支” SVG 源，使用能保留 alpha 的 macOS `sips` 直接渲染 RGBA 基图并生成 `GitCloneGui.icns`；CMake 将 `.icns` 标记为 `Resources` 并设置 `MACOSX_BUNDLE_ICON_FILE`。
- 理由：视觉与应用内品牌一致，源资产可维护，Finder/Dock 使用 macOS 原生图标格式。
- 代价：生成的 `.icns` 是二进制派生产物，需要在源图更新时重新生成；生成后必须检查 RGBA 与四角透明度，不能使用会合成白底的 thumbnail 路径。
- 被否决方案：仅在运行时设置 `QApplication::setWindowIcon`（不能可靠提供 Finder Bundle 图标）、只提交单尺寸 PNG。

### DEC-009：在安装阶段生成自包含 Bundle

- 上下文与需求：REQ-008 / AC-008.3、AC-008.4，NFR-007。
- 决策：配置时从当前 Qt CMake target/qmake 所在 bin 目录定位 `macdeployqt`；`cmake --install` 复制 `.app` 后执行该工具，收集 Qt Framework、plugins 并改写 install names。
- 理由：使用与实际链接版本一致的 Qt 官方部署工具，同时保持开发构建快速、轻量。
- 代价：安装 Bundle 明显变大；未签名部署会留下 ad-hoc/未签名限制。
- 被否决方案：手工复制 Framework/插件、硬编码 `/Users/...` 路径、对每次 build 执行部署。

### DEC-010：以独立异步 service 查询远程分支

- 上下文与需求：REQ-009、NFR-001、NFR-008。
- 决策：application 定义 `RemoteBranchService` 和 `RemoteBranchCatalog`；infrastructure 的 `GitRemoteBranchService` 为每个请求创建独立 `QProcess`，以结构化参数执行 `git ls-remote --symref URL HEAD refs/heads/*`，15 秒超时并设置 `GIT_TERMINAL_PROMPT=0`。
- 理由：父/子 URL 可并发查询且不干扰单进程克隆 runner，仍沿用用户的 Git URL 与 credential helper 配置。
- 代价：新增请求生命周期、缓存和输出解析逻辑；认证失败只能非阻塞提示。
- 被否决方案：复用 `GitProcessRunner`、presentation 直接使用 QProcess、同步执行。

### DEC-011：可编辑组合框提供默认/常用优先与包含式搜索

- 上下文与需求：REQ-009 / AC-009.2、AC-009.3。
- 决策：新增 `BranchSelector : QComboBox`，启用 editable 与 `QCompleter::PopupCompletion + Qt::MatchContains`；结果顺序为 default、常用 exact、常用 namespace、其余稳定名称顺序。折叠 chrome 不调用平台原生 QComboBox paint，而由组件用 QPainter 绘制完整圆角背景/边框、浅分隔线和可翻转 chevron；popup 列表继续使用 Qt 原生模型与 QSS。
- 理由：一个控件同时覆盖点击选择、任意手输和输入即筛选；Qt 5.15/6 均支持所需 API。
- 代价：排序代表默认/常用优先而非最近提交，失败状态主要通过 tooltip/控件属性表达；自绘 chrome 需用 snapshot 防止不同 DPI 下的视觉回归。
- 被否决方案：只读下拉、独立搜索框、伪造提交时间排序。

### DEC-012：页内结果优先并以 splitter 分配执行区

- 上下文与需求：REQ-010、REQ-003、REQ-006。
- 决策：移除完成/失败 `QMessageBox`；状态卡通过 `statusState=success|error|normal` 显示结果。preview/status 与日志放入纵向 `QSplitter`，日志最小 280px 且为默认主要区域。
- 理由：结果、错误和日志保留在同一视觉上下文，用户可按需拖动检查完整输出。
- 代价：MainWindow 需区分“配置校验状态”和“最近任务结果状态”。
- 被否决方案：仅美化系统 QMessageBox、固定压缩预览高度而不允许用户调整。

### DEC-013：最终 Completed/Failed 通过独立桌面通知适配器提示

- 上下文与需求：REQ-010 / AC-010.5，NFR-009。
- 决策：MainWindow 只在最终 `jobFinished(Completed|Failed)` 发出 `taskResultNotificationRequested(title, message, severity)`；Completed 使用完成标题和 Information，Failed 使用失败标题和 Critical，Cancelled 不发。presentation 的 `DesktopNotifier` 使用 `QSystemTrayIcon::showMessage` 转交操作系统，并在不支持 tray/messages 时静默返回。app 组合根连接两者。
- 理由：完成触发语义仍由已有 controller/MainWindow 结果流决定，系统能力封装在独立 UI 适配器；测试可验证请求次数而不真的打扰通知中心。
- 代价：Qt 的系统通知受操作系统权限控制；为投递消息会短暂显示 tray/status item，随后自动隐藏。
- 被否决方案：恢复阻塞式 QMessageBox、用 shell/AppleScript 注入通知、在 CloneController 中直接依赖 Widgets。

### DEC-014：GitHub Actions 原生双平台构建，签名能力按 Secrets 门控

- 上下文与需求：REQ-011、NFR-010。
- 决策：单一 `release.yml` 编排独立的 `macos-15` arm64 与 `windows-2022` x64 job；共同执行 Release configure/build/CTest/install，各平台再使用 Qt 官方部署工具生成 DMG/ZIP。普通 push/PR/手动运行上传 Actions artifact，只有 `v*` 标签在两个 job 成功后通过 `gh release` 创建长期 Release。Apple Developer ID/公证和 Windows Authenticode 由独立脚本消费 GitHub Secrets；Secrets 不完整时明确降级为 unsigned artifact。
- 理由：目标平台原生 runner 能给出真实编译/链接/部署证据；部署与签名脚本可本地复用和单独审查，YAML 只负责编排；无证书时不阻塞开源项目的可下载测试包。
- 代价：可信发布依赖外部证书、Apple Developer Program 与公证服务；Windows 首期提供便携 ZIP 而非 MSI。
- 被否决方案：在 Mac 上交叉编译 Windows、上传裸 `.exe`/build-tree `.app`、把证书提交到仓库、每次普通 push 自动创建 Release。

## 总体架构

```mermaid
flowchart LR
    Card["ChildRepositoryCard"] --> Window["MainWindow"]
    Selector["BranchSelector"] --> Card
    Selector --> Window
    Selector --> BranchPort["RemoteBranchService"]
    Window --> NotifyRequest["final result notification signal"]
    NotifyRequest --> Notifier["DesktopNotifier / QSystemTrayIcon"]
    Window --> Controller["CloneController"]
    Window --> StorePort["ConfigurationStore"]
    Controller --> Core["CloneRequest / ClonePlan list"]
    Controller --> RunnerPort["ProcessRunner"]
    GitRunner["GitProcessRunner"] --> RunnerPort
    BranchGit["GitRemoteBranchService"] --> BranchPort
    Settings["QSettingsConfigurationStore"] --> StorePort
    Root["main.cpp"] --> Window
    Root --> Controller
    Root --> GitRunner
    Root --> Settings
    Root --> BranchGit
    Root --> Notifier
```

### 组件与职责

| 组件 | 职责与边界 | 输入/输出 | 相关需求 |
|---|---|---|---|
| `CloneRequest/ClonePlan` | 父字段、0～N 子列表、校验、命令计划、重复路径检查 | 表单值 → 有序计划/错误 | REQ-001, REQ-002 |
| `CloneController` | 父+子队列、当前索引、取消/结果 | plan + runner events → status/result | REQ-002, REQ-003 |
| `ConfigurationStore` | 持久化契约 | optional request / save result | REQ-007 |
| `QSettingsConfigurationStore` | schemaVersion、数组序列化、sync/status | QSettings ↔ CloneRequest | REQ-007 |
| `ChildRepositoryCard` | 单个子仓库字段、序号、删除信号、配置读写 | child value ↔ signals | REQ-005, REQ-006 |
| `MainWindow` | 双栏页面、卡片集合、摘要、预览、保存定时器 | 用户事件 ↔ controller/store | REQ-001, REQ-005～REQ-007 |
| `GitProcessRunner` | 单进程 QProcess 适配 | command ↔ async signals | REQ-002, REQ-003 |
| `RemoteBranchService` | 分支查询 port、请求 ID 与结果目录 | URL → catalog/error | REQ-009 |
| `GitRemoteBranchService` | 独立 QProcess、refs 解析、排序、缓存、超时 | Git refs ↔ catalog | REQ-009 |
| `BranchSelector` | 可编辑下拉、URL debounce、包含式搜索、过期结果隔离、自绘圆角 chrome/chevron | URL/catalog ↔ branch text | REQ-009 |
| `DesktopNotifier` | 把最终成功/失败请求转为系统托盘消息并处理能力降级/自动隐藏 | title/message/severity → OS notification | REQ-010 |

## 模块与依赖边界

### ARCH-001：依赖向 core/application 契约收敛

- 决策：presentation → application/core；infrastructure → application/core；app 组合具体实现。
- 组合根：`src/app/main.cpp`。
- 禁止：presentation include QProcess/QSettings/具体 infrastructure；application include Widgets/具体适配器；core include Widgets/I/O。
- 验证：target link/include 扫描。

### ARCH-002：有序请求与路径不变量归 core

- 每个子请求分别校验 URL/分支/相对路径；错误带 1-based 卡片序号。
- 任一子目标必须严格位于父目录内；规范化目标不得重复。
- 0 个子项有效；计划顺序与输入顺序一致。
- 预览由同一 `ClonePlan` 生成。

### ARCH-003：单 runner 串行队列

- runner 同时最多一个进程；controller 只在 exit 0 后推进。
- currentChildIndex 仅在 CloningChild 有效；finish 时复位。
- 取消 timer 与旧实现一致。

### ARCH-004：表示层分为页面与单卡组件

- `ChildRepositoryCard` 只拥有单卡控件和序号，不访问 controller/store。
- MainWindow 拥有卡片集合、布局、预览、摘要、保存 debounce 和运行门控。
- 配置区内部滚动；右栏日志获得 stretch。

### ARCH-005：配置存储通过契约隔离

- `ConfigurationStore` 位于 application，仅以 `CloneRequest` 读写。
- QSettings key：`schemaVersion=1`、`parent/url|branch|directory`、`destinationRoot`、`children` array 的 url/branch/path。
- `load()` 仅在 schemaVersion 存在时返回值；因此首次启动和已保存 0 子项可区分。
- save 后 `sync()`，status 非 NoError 返回 false。

### ARCH-006：视觉样式集中管理

- QSS 由 presentation 内的 `AppStyle::applicationStyleSheet()` 单点提供；对象名/动态属性区分 card、primary、secondary、danger、muted。
- 不在业务槽函数散落颜色/字体设置。
- 默认/最小尺寸和 splitter stretch 是页面契约。

### ARCH-007：远程分支查询通过独立 port/adapter 隔离

- `RemoteBranchService` 位于 application，公开 request/cancel 与带请求 ID 的 success/failure 信号；不依赖 Widgets 或具体 QProcess。
- `GitRemoteBranchService` 位于 infrastructure，拥有并发查询进程、15 秒超时、会话缓存、refs 解析和排序；不得启动 clone/fetch 或修改仓库。
- `BranchSelector` 位于 presentation，只管理一个 URL/分支输入的 debounce、suggestion model 与过期结果；MainWindow/Card 不解析 Git 输出。
- 克隆 `ProcessRunner` 与分支 service 无调用或状态共享；app 仍是唯一组合根。

### ARCH-008：最终成功事件与系统通知能力分离

- MainWindow 只根据最终 `CloneController::Outcome::Completed|Failed` 各发一次相应通知请求，不创建 `QSystemTrayIcon`，Cancelled 不发请求。
- `DesktopNotifier` 位于 presentation，只依赖 QtWidgets，负责 capability check、匹配应用视觉的通知图标、`showMessage` 和延时隐藏；不访问 controller/store/request。
- `src/app/main.cpp` 是唯一连接通知请求与具体 notifier 的组合根；通知失败不得反向改变任务状态。

### ARCH-009：CI 编排、平台部署与凭据处理分离

- `.github/workflows/release.yml` 只声明触发条件、runner、最小权限、构建步骤和产物传递，不包含业务源码或证书内容。
- `src/app/CMakeLists.txt`/`cmake/Deploy*.cmake.in` 拥有 build-tree 到自包含安装树的规则；Windows 资源只归 app target，不进入 core/application/infrastructure/presentation。
- `scripts/release` 拥有临时证书导入、平台签名、公证、DMG/ZIP 校验；脚本只接受环境变量/参数，不把 Secret 输出到日志或写入仓库路径。
- release job 只消费已完成的两个 artifact 并写 GitHub contents；构建 job 保持 `contents: read`，PR/fork 无 Secrets 时走 unsigned 分支。

## 构建与交付结构

### BUILD-001：现有 target 分层保持不变

- `git_clone_core`：列表模型/校验，QtCore。
- `git_clone_application`：controller + store contract，core/QtCore。
- `git_clone_infrastructure`：Git runner + QSettings store，application/QtCore。
- `git_clone_presentation`：MainWindow + card，application/core/QtWidgets。
- `GitCloneGui`：唯一组合根。

### BUILD-002：Qt 版本选择不泄漏业务代码

- 继续使用 `${QT_PACKAGE}::Core/Widgets/Test` 与 AUTOMOC；不添加 QtQuick。

### BUILD-003：Preset 与平台开发产物契约

- 本地 Debug/Release、macOS `build/<preset>/bin/GitCloneGui.app` 与 Windows `build/<preset>/bin/GitCloneGui.exe` 输出规则保持；CI 使用独立 `build/ci-*` 目录，避免污染本地 preset。

### BUILD-004：测试所有权扩展

- `test_clone_core`：0～N 计划、重复/逃逸路径。
- `test_clone_controller`：多项队列、0 项、失败门控、取消。
- `test_configuration_store`：临时 INI 路径往返、0 项、特殊字符。
- `test_presentation`：卡片增删/重编号、配置恢复、摘要、默认尺寸，并可输出 snapshot。
- `test_git_clone_workflow`：真实父 + 至少 2 个子仓库。

### BUILD-005：macOS 资源与部署职责归 app target

- `src/app/CMakeLists.txt` 只声明 app 资源、Bundle 元数据、安装规则以及当前 Qt kit 部署工具定位；业务 target 不感知打包。
- `.icns` 以 `MACOSX_PACKAGE_LOCATION=Resources` 随 Debug/Release Bundle 生成，`Info.plist` 的 `CFBundleIconFile` 与文件名一致。
- `.icns` 的 1024/512/256/128/32/16 各尺寸来自保留 alpha 的 RGBA 基图；四角 alpha 必须为 0。
- `cmake --install build/release --prefix build/install` 先复制 Bundle，再由生成的 install script 调用 `macdeployqt -always-overwrite`。
- 部署工具缺失时 Apple 配置必须给出明确 fatal 诊断，禁止静默产出伪自包含安装树。
- 非 Apple 平台不执行此部署脚本，现有 `WIN32`/普通 executable 入口保持不变。

### BUILD-006：分支查询与交互测试所有权

- `git_clone_application` 新增 branch service 契约；仍只依赖 core/QtCore。
- `git_clone_infrastructure` 新增 Git branch adapter；继续只依赖 application/QtCore，不新增 target 边。
- `git_clone_presentation` 新增 `BranchSelector`；继续依赖 application/core/QtWidgets。
- `test_git_remote_branches` 使用本地临时 Git 仓库验证解析、默认/常用排序、并发与错误；`test_presentation` 验证可编辑、包含匹配、过期结果和 splitter/页内状态。

### BUILD-007：桌面通知保持在 presentation/app 边界

- `git_clone_presentation` 新增 `DesktopNotifier`，沿用既有 QtWidgets 依赖；MainWindow 新增携带 title、message 与 `NotificationSeverity` 的通知信号。
- `GitCloneGui` 组合根只实例化 notifier 并连接信号，不引入平台 shell、额外 framework 或新 target 边。
- `test_presentation` 验证 Completed/Failed 各恰好一次对应通知且 Cancelled 零次；系统权限/通知中心展示作为 macOS 人工补充，不在测试中实际弹通知。

### BUILD-008：GitHub 双平台部署、签名与发布

- Windows：Visual Studio 2022 x64/MSVC 编译显式使用 UTF-8；app target 包含 `.ico`/`.rc`；install tree 为 `build/ci-windows/install/bin`，install script 调用同 Qt kit 的 `windeployqt` 收集 Qt DLL、`platforms/qwindows.dll` 和 compiler runtime；可选脚本用 PFX + `signtool` 签名主 `.exe`，最终 ZIP 根目录为 `GitCloneGui/`。
- macOS：`macos-15` arm64 runner 安装 Qt 6.8 LTS；现有 install script 继续用同 Qt kit 的 `macdeployqt`，存在 `GIT_CLONE_GUI_MACOS_SIGNING_IDENTITY` 时启用 notarization signing/hardened runtime/timestamp；最终以 staging app + `/Applications` 链接生成 DMG。
- 公证：仅在 Developer ID 签名与 `APPLE_ID`、`APPLE_APP_PASSWORD`、`APPLE_TEAM_ID` 齐全时提交最终 DMG，等待 accepted 后 staple 并用 `spctl`/`stapler validate` 验证。
- GitHub：第三方 Actions 固定 commit SHA；两个 build job 分别上传单文件 artifact，tag-only release job 下载并校验恰有 DMG/ZIP 后通过 `gh release create/upload` 发布。

## 平台与交付矩阵

| 目标平台/架构 | 开发构建物 | 安装产物 | 发布包 | 运行时依赖 | 原生验证 |
|---|---|---|---|---|---|
| macOS / arm64 | `build/debug/bin/GitCloneGui.app`，带 `.icns`、仍可依赖开发 Qt | `build/install/GitCloneGui.app`，自包含 Qt Framework/plugins | 不适用 | 安装产物内置 Qt 5.15.2 framework/plugin；运行仍需系统 Git | Debug/Release、CTest、self-contained delivery、`otool`、launch、Finder 图标检查 |
| GitHub macOS / arm64 | `build/ci-macos/bin/GitCloneGui.app` | `build/ci-macos/install/GitCloneGui.app`，Qt 6.8 自包含、按 Secrets 可选 Developer ID 签名 | `GitCloneGui-macOS-arm64.dmg`；`v*` 附加到 Release | Bundle 内置 Qt Framework/Cocoa plugin；运行仍需系统 Git | run `31506923442`：configure/build/CTest 6/6/install/unsigned DMG/artifact 成功；codesign/notary 待 Secrets |
| GitHub Windows / x64 | `build/ci-windows/bin/GitCloneGui.exe` | `build/ci-windows/install/bin/`，Qt 6.8 DLL/plugins/MSVC runtime、按 Secrets 可选 Authenticode | `GitCloneGui-Windows-x64.zip`；`v*` 附加到 Release | ZIP 内置 Qt/MSVC 运行时；运行仍需 Git for Windows | run `31506923442`：configure/build/CTest 6/6/windeployqt/runtime/ZIP/artifact 成功；Authenticode 待 Secrets |

### macOS 应用束约束

- Bundle ID、版本、可执行文件和输出路径沿用现有契约。
- `Contents/Resources/GitCloneGui.icns` 必须存在，`CFBundleIconFile=GitCloneGui.icns`。
- 安装树必须包含 `Contents/Frameworks/QtCore.framework`、`QtGui.framework`、`QtWidgets.framework` 和 `Contents/PlugIns/platforms/libqcocoa.dylib`。
- QSettings 使用 macOS 用户域，由 organization/application 名决定；不写入 bundle。
- 开发构建物与安装/部署产物分离；GitHub job 基于安装树创建 DMG。没有 Secrets 时 DMG 为未签名测试包；Secrets 完整时必须通过 Developer ID、公证和 stapling 验证。

### Windows 便携包约束

- `GitCloneGui.exe` 使用 `WIN32` subsystem，并通过 `.rc` 嵌入与 macOS 一致的 Windows `.ico`。
- `cmake --install` 的 `bin/` 是部署根；必须含 `Qt6Core.dll`、`Qt6Gui.dll`、`Qt6Widgets.dll`、`platforms/qwindows.dll` 及 `windeployqt` 判定的 compiler runtime。
- ZIP 内保留单一 `GitCloneGui/` 根目录；不生成 MSI，不把 Qt 开发目录或证书临时文件打入 ZIP。
- 仅主程序由项目证书 Authenticode 签名；Qt/运行库保留上游签名。签名路径必须 timestamp 并用 `signtool verify /pa` 验证。

## 复杂度预算与演进规则

| 维度 | 当前基线 | 边界/触发条件 | 触发后动作 | 验证 |
|---|---|---|---|---|
| MainWindow | 旧版 303 行、同时构建单子表单 | 单文件超过约 420 行或出现单卡字段细节 | 布局拆到 MainWindowUi，单卡保留独立组件 | inspect_structure + 职责审查 |
| BranchSelector | 新组件 | 解析 Git 输出、管理多 URL 或访问 CloneController | 分别留在 infrastructure/MainWindow | include/API 审查 |
| Child card | 新组件 | 访问 controller/store 或负责列表 | 上移 MainWindow | include/API 审查 |
| Controller | 190 行 | 队列策略与 runner I/O 混合或出现并行 | 提取 queue policy | 状态机测试 |
| Store | 新适配器 | QSettings 泄漏到 presentation/application | 保持 port/adapter | include 扫描 |
| 顶层 CMake | 35 行 | 出现模块源码/样式/设置细节 | 下沉模块 CMake | 清单审查 |
| app CMake | 29 行 | 部署脚本逻辑超过约 80 行或混入其他平台细节 | 提取 `cmake/DeployMacOS.cmake.in` | 清单审查 + install 回归 |
| Release workflow | 新增 | 单个 YAML step 混合证书导入、签名、公证与打包细节 | 下沉 `scripts/release` 平台脚本 | actionlint/YAML 解析 + 原生 run |
| Release scripts | 新增 | 单脚本同时处理 macOS 与 Windows 或打印 Secret | 按平台/职责拆分并只经环境变量传值 | shell/PowerShell 静态审查 |

## 接口契约

| 接口 | 输入 | 输出/副作用 | 错误语义 | 兼容性 |
|---|---|---|---|---|
| `buildClonePlan(request)` | parent + ordered children | parentCommand + ordered children plans | 聚合编号错误，不写文件 | QtCore 5.15/6 |
| `CloneController::start` | 请求快照 | 顺序 runner.start | busy/invalid/git missing 拒绝 | QtCore 5.15/6 |
| `ConfigurationStore::load` | 无 | `optional<CloneRequest>` | 无配置/不可读返回 nullopt | C++17 |
| `ConfigurationStore::save` | CloneRequest | bool | sync 错误 false | C++17 |
| `ChildRepositoryCard` | child value/index | value、configurationChanged、removeRequested | 不自行校验全局路径 | QtWidgets 5.15/6 |
| `RemoteBranchService::requestBranches` | 仓库 URL | request ID；异步 catalog/error | 15 秒超时、取消/过期可忽略 | QtCore 5.15/6 |
| `BranchSelector` | URL + 可选初始分支 | branch text、下拉 suggestions | 查询失败仍可手输 | QtWidgets 5.15/6 |

## 数据模型与状态

```text
CloneRequest
  parentRepositoryUrl
  parentBranch
  parentDirectoryName
  destinationRoot
  children[]
    repositoryUrl
    branch
    relativePath

ClonePlan
  parentCommand
  parentTargetPath
  children[]
    command
    targetPath
```

```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> CloningParent: start(valid)
    CloningParent --> Idle: success and children=0
    CloningParent --> CloningChild: success and children>0
    CloningChild --> CloningChild: child i succeeds and more remain
    CloningChild --> Idle: last child succeeds
    CloningParent --> Idle: error/nonzero
    CloningChild --> Idle: error/nonzero
    CloningParent --> Cancelling: cancel
    CloningChild --> Cancelling: cancel
    Cancelling --> Idle: exited/killed
```

## 关键流程

```mermaid
sequenceDiagram
    participant W as MainWindow
    participant S as ConfigurationStore
    participant C as CloneController
    participant R as ProcessRunner
    W->>S: load on startup
    S-->>W: optional request
    W->>S: save after 300ms debounce
    W->>C: start(request snapshot)
    C->>R: parent command
    loop each child in card order
        R-->>C: exit 0
        C->>R: child i command
    end
    R-->>C: final exit 0
    C-->>W: completed(count, path)
```

```mermaid
sequenceDiagram
    participant U as URL Edit
    participant B as BranchSelector
    participant S as RemoteBranchService
    participant G as git ls-remote
    U->>B: textChanged(url)
    B->>B: debounce 450ms / cancel stale
    B->>S: requestBranches(url)
    S->>G: --symref URL HEAD refs/heads/*
    G-->>S: HEAD + heads
    S-->>B: catalog(requestId)
    B->>B: apply only current request; preserve typed text
```

## 算法与伪代码

```text
onFinished(success):
  if cancelling -> Cancelled
  if failure -> Failed(current stage)
  if parent:
    if children empty -> Completed(0)
    else currentChildIndex=0; startChild(0)
  else if currentChildIndex + 1 < childCount:
    ++currentChildIndex; startChild(index)
  else -> Completed(childCount)

save debounce:
  on any field/card change -> timer.start(300)
  timeout -> store.save(currentRequest)
  close -> if timer active, stop and save immediately
```

## 错误处理与恢复

| 失败点 | 检测 | 处理 | 用户可见结果 | 恢复 |
|---|---|---|---|---|
| 卡片输入/重复路径 | core 校验 | 不启动 | 最多 3 行摘要 + 剩余数 | 修改对应卡片 |
| 任一 Git 阶段 | runner events | 停止队列 | 父或子 i/N + 错误 | 保留文件 |
| 设置不可写 | save=false | 不阻塞 | 状态区提示 | 当前会话继续、后续重试 |
| 旧/无 schema | load nullopt | 首次默认 1 张空卡 | 无错误 | 用户填写 |
| 远程分支查询失败/超时 | QProcess exit/error/timer | 释放请求、发非阻塞 error | selector tooltip/error property | 保持手工输入、URL 变化可重试 |
| 目标目录为文件或非空目录 | core 校验 | 不启动 | “父项目目标目录必须为空” | 更改目录名或清空目录（用户自行处理） |
| Windows/macOS 编译或 CTest 失败 | Actions job exit code | 停止该平台 job，不上传伪成品 | GitHub Checks 失败及日志 | 修复后重新运行 |
| Qt 部署工具缺失/失败 | install script fatal | 停止打包 | GitHub step 明确失败 | 检查 Qt kit/action 版本 |
| 签名 Secrets 缺失或不完整 | 平台脚本条件检查 | 跳过真实签名/公证，标记 unsigned | Job Summary | 配齐 Secrets 后重跑 |
| 签名或公证验证失败 | `codesign`/`notarytool`/`signtool` 非零 | 停止发布，不上传冒充正式签名的包 | GitHub job 失败 | 更新证书/密码/协议后重跑 |

## 非功能设计

- 安全/隐私：不使用 shell；QSettings 只含表单；不保存日志/任务状态；UI/README 提醒 URL Token 风险。
- 响应性：Git 异步；设置保存 debounce；左栏 scroll；日志 10,000 block。
- 可观测性：状态显示子项 i/N；预览展示所有命令；错误摘要压缩。
- 视觉：#F5F7FB 背景、#FFFFFF 卡片、#2563EB 主色、#DC2626 危险色、12px 圆角、清晰焦点环；字体使用系统默认，命令/日志用等宽字体。
- 兼容性：不依赖 macOS 私有 API；QSettings 使用 NativeFormat，测试使用 IniFormat 临时文件。
- 远程查询：450ms debounce、15 秒超时、`GIT_TERMINAL_PROMPT=0`、每 URL 会话缓存；只传结构化参数，不记录 URL/refs 到日志。
- 结果/日志：任务结束后的状态卡优先保持到下一次配置编辑；纵向 splitter 默认分配日志不少于 280px，用户可拖动。
- 发布安全：工作流顶层只读权限，tag release job 单独 `contents: write`；Actions 固定 SHA；证书从 Secrets 写入 runner 临时目录/临时 keychain，job 生命周期结束即销毁。
- 交付兼容：GitHub 标准 runner 只承诺 macOS arm64 与 Windows x64；macOS Intel/universal、Windows ARM64、MSI/PKG 不在本轮契约。

## 正确性属性

### PROP-001：所有子目标均位于父目录内且唯一

- 来源：REQ-001 / AC-001.3。
- 属性：任意通过校验的计划中，每个 childTarget 严格位于 parentTarget 下，且 childTarget 集合无重复。
- 验证：表驱动 core 测试。

### PROP-002：失败不会越过队列边界

- 来源：REQ-002 / AC-002.2、AC-002.3。
- 属性：父或子 i 失败时，runner 历史不包含任何后续子项。
- 验证：fake runner 状态机测试。

### PROP-003：执行参数不经过 shell

- 来源：REQ-002 / AC-002.1、NFR-001。
- 属性：每个输入字段在其命令中保持一个参数元素。
- 验证：core 测试与静态检查。

### PROP-004：任务互斥并最终恢复 Idle

- 来源：REQ-002 / AC-002.5、REQ-003 / AC-003.4。
- 属性：非 Idle 拒绝 start；完成/失败/取消最终回 Idle。
- 验证：状态机序列测试。

### PROP-005：配置保存恢复往返保持顺序和值

- 来源：REQ-007 / AC-007.1、AC-007.2。
- 属性：对任意可序列化 CloneRequest，`save(x); load()` 保持所有字段、子项数量与顺序相等，包括 0 子项。
- 验证：临时 INI QSettings 往返测试。

### PROP-006：卡片列表与请求列表一一对应

- 来源：REQ-005 / AC-005.1、AC-005.2。
- 属性：任意增删序列后，请求 children 数量/顺序等于当前可见卡片，标题连续为 1..N。
- 验证：presentation 测试。

### PROP-007：安装 Bundle 的 Qt 依赖闭包位于应用内部

- 来源：REQ-008 / AC-008.3、AC-008.4，NFR-007。
- 属性：部署后的主 executable 与 Qt 插件不包含指向 Qt 开发目录的绝对动态依赖；所需 Qt Framework 与 Cocoa platform plugin 均位于 Bundle 内。
- 验证：`verify_delivery.py --require-self-contained`、递归 `otool -L`、bundle 结构与启动检查。

### PROP-008：应用图标圆角外保持透明

- 来源：REQ-008 / AC-008.5。
- 属性：从源 SVG 生成的 1024 PNG 以及从 Bundle `.icns` 反解出的最大尺寸图像均为 RGBA，四个角像素 alpha 等于 0；不得出现不透明白色画布。
- 验证：`sips -g hasAlpha` 与图像像素 alpha 断言、Dock/Finder 人工检查。

### PROP-009：分支建议始终属于当前 URL 请求

- 来源：REQ-009 / AC-009.1、AC-009.4、AC-009.5。
- 属性：对于任意交错完成的 URL 查询序列，BranchSelector 只应用其最后一个 URL 对应的当前 request ID；失败或过期结果不改变用户已输入分支文本。
- 验证：fake service 乱序完成测试与本地 Git 并发查询测试。

### PROP-010：目标目录校验与 Git 空目录能力一致

- 来源：REQ-010 / AC-010.1。
- 属性：父目标不存在或为空目录时计划有效；同一路径为文件或包含任意可见/隐藏条目的目录时计划无效且错误包含“必须为空”。
- 验证：临时目录表驱动 core 测试。

### PROP-011：任务结果不会被同一完成事件的重新校验覆盖

- 来源：REQ-010 / AC-010.2、AC-010.3。
- 属性：Completed/Failed 到达后状态卡分别保持 success/error 与任务消息，直到用户修改配置或启动新任务；日志内容不清空。
- 验证：presentation 信号驱动测试与 snapshot 人工检查。

### PROP-012：分支选择器不暴露平台原生直角边框

- 来源：REQ-006 / AC-006.3，REQ-009 / AC-009.6。
- 属性：任意正常、悬停、聚焦、禁用与 popup 展开状态下，BranchSelector 的折叠外框由同一 8px 圆角路径绘制；右侧只有浅分隔线和 chevron，不出现独立黑色矩形边框，popup 可见时 chevron 翻转。
- 验证：selector paint/state 测试与默认窗口 snapshot 人工检查。

### PROP-013：结果通知与最终成功/失败一一对应

- 来源：REQ-010 / AC-010.5，NFR-009。
- 属性：任意克隆状态序列中，每个最终 Completed 恰好产生一个 Information 完成通知请求，每个最终 Failed 恰好产生一个 Critical 失败通知请求；Cancelled、父阶段成功和中间子阶段成功产生零个通知。notifier capability 失败不改变结果或页内状态。
- 验证：presentation 的可控 runner 完成/失败/取消信号计数和标题/级别测试，以及 notifier capability 代码审查。

### PROP-014：每个平台发布包具有完整运行时闭包

- 来源：REQ-011 / AC-011.2、AC-011.3。
- 属性：成功上传的 Windows ZIP 解压后包含 exe、Qt Core/Gui/Widgets DLL、qwindows plugin 和 compiler runtime；macOS DMG 内 app 包含 Qt Frameworks、Cocoa plugin 与图标，二者均不引用 runner Qt 安装绝对路径。
- 验证：原生 runner 上的结构断言、`windeployqt`/`macdeployqt` 退出码、Windows 文件清单、macOS delivery/`otool` 检查。

### PROP-015：签名声明与实际验证状态一致

- 来源：REQ-011 / AC-011.4、AC-011.5，NFR-010。
- 属性：Secrets 不完整时不运行或声称真实签名；进入 signed path 后任何导入、timestamp、签名、公证、staple 或 verify 失败都会使 job 失败，因而 Release 不会发布未通过验证却标称 signed 的包。
- 验证：workflow 条件/summary 审查、无 Secret run、配置 Secret 后的 `codesign`/`spctl`/`notarytool`/`signtool` 日志。

## 测试策略

| 行为/属性 | 层级 | 场景 | 证据 |
|---|---|---|---|
| REQ-001 / PROP-001,003 | core | 0/1/N、重复/逃逸、元字符、预览顺序 | `test_clone_core` |
| REQ-002,003 / PROP-002,004 | application | 0/多项成功、中间失败、取消、互斥 | `test_clone_controller` |
| REQ-007 / PROP-005 | infrastructure | no config、0/N、特殊字符、覆盖 | `test_configuration_store` |
| REQ-005,006 / PROP-006 | presentation | add/remove/renumber、restore、尺寸、摘要、snapshot | `test_presentation` + 视觉检查 |
| REQ-002,004 | integration/delivery | 父+2 子真实 clone、preset、bundle、launch | CTest + delivery script |
| REQ-008 / PROP-007 | app/delivery | plist/icon、Framework/plugin、无开发 Qt 绝对依赖、安装幂等、launch | `plutil` + `find` + `otool` + self-contained delivery |
| REQ-008 / PROP-008 | app/resource | RGBA、四角透明、Bundle icns 反解 | alpha 像素断言 + Dock/Finder 检查 |
| REQ-009 / PROP-009 | infrastructure/presentation | 本地 refs、默认/常用排序、并发、错误、可编辑包含匹配 | `test_git_remote_branches` + `test_presentation` |
| REQ-010 / PROP-010,011,013 | core/presentation | 空/非空/文件、页内成功失败、splitter 默认日志高度、最终成功/失败通知次数与取消零通知 | core/UI tests + snapshot |
| REQ-011 / PROP-014,015 | CI/delivery | Windows/macOS 原生 build+CTest、自包含结构、unsigned 降级、签名/公证门控、tag Release | Actions jobs + artifact/Release 清单 + 平台签名工具 |

## 需求覆盖矩阵

| 行为 | 组件 | 边界 | 决策 | 属性 | 测试 |
|---|---|---|---|---|---|
| REQ-001 | CloneRequest/MainWindow | ARCH-002, ARCH-004 | DEC-005 | PROP-001,003,006 | core/UI |
| REQ-002 | CloneController/Runner | ARCH-002, ARCH-003 | DEC-001,002,005 | PROP-002～004 | controller/integration |
| REQ-003 | Controller/MainWindow | ARCH-003, ARCH-004 | DEC-002,004 | PROP-004 | controller/UI |
| REQ-004 | CMake/app/README | BUILD-001～004 | DEC-003 | 不适用 | build/delivery |
| REQ-005 | ChildRepositoryCard/MainWindow | ARCH-004, ARCH-006 | DEC-005,007 | PROP-006 | presentation |
| REQ-006 | MainWindow/QSS | ARCH-004, ARCH-006 | DEC-007 | PROP-006 | snapshot/人工 |
| REQ-007 | ConfigurationStore/QSettings | ARCH-005 | DEC-006 | PROP-005 | store/UI |
| REQ-008 | app CMake/resources/install script | BUILD-003, BUILD-005 | DEC-008,009 | PROP-007,008 | icon alpha/bundle/self-contained delivery/launch |
| REQ-009 | RemoteBranchService/GitRemoteBranchService/BranchSelector | ARCH-007, BUILD-006 | DEC-010,011 | PROP-009,012 | infrastructure/presentation/snapshot |
| REQ-010 | CloneRequest/MainWindow/MainWindowUi/DesktopNotifier | ARCH-002, ARCH-004, ARCH-006, ARCH-008 | DEC-012,013 | PROP-010,011,013 | core/presentation/snapshot/notification signal |
| REQ-011 | app CMake/Deploy scripts/release workflow | ARCH-009, BUILD-003, BUILD-005, BUILD-008 | DEC-014 | PROP-014,015 | Windows/macOS Actions delivery/signature/release |

## 风险与未决问题

- RISK-001：QSS 在 Qt 5/6 和不同平台细节会有差异；以 macOS Qt 5 原生截图为本轮证据。
- RISK-002：保存的 URL 可能含用户手工嵌入 Token；UI/README 明示风险，不尝试不可靠地解析/脱敏所有 URL 格式。
- RISK-003：Qt 6.8 已由 GitHub macOS arm64/Windows x64 run `31506923442` 原生验证；未来 Qt 6.8.x 更新仍以 Actions 回归为准。
- RISK-004：自包含 Bundle 体积会从 KB 级增加到数十 MB；这是 Framework/plugin 闭包的预期代价。
- RISK-005：未签名、未公证 artifact 会触发 Gatekeeper；只有配置 Developer ID/公证 Secrets 并通过在线验证的 tag 包才能声明可信分发。
- RISK-006：Quick Look 等 thumbnail 工具可能把透明 SVG 合成到白色画布；图标生成禁止使用该中间结果并以像素 alpha 断言防回归。
- RISK-007：通用 `ls-remote` 无提交时间，分支“活跃”只能以默认/常用工作流启发式表达；UI 文案使用“默认与常用优先”。
- RISK-008：私有远端可能依赖交互式认证；查询进程禁止终端提示并超时，用户仍可手工输入，实际 clone 沿用原凭据流程。
- RISK-009：Apple/Windows 证书获取、续期和费用属于外部状态；本实现提供安全消费和验证路径，不伪造证书。
- RISK-010：GitHub runner 与 Qt 下载源可能变化；固定 runner 大版本、Qt 6.8 LTS 范围和 Actions commit，首个线上 run 作为最终交付证据。
- 代码设计无阻塞问题；真实 signed/notarized 证据待用户配置 Secrets 后由线上 run 产生。
