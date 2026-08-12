# 需求文档：git-clone-gui

> 阶段：requirements
>
> 工作流：requirements-first
>
> 状态：已更新
>
> 最近更新：2026-08-12

## 事实与环境基线

| ID | 事实或未知项 | 状态 | 证据/来源 | 对需求的影响 |
|---|---|---|---|---|
| FACT-001 | 用户要用 GUI 代替重复输入父项目与嵌套子项目的带分支 `git clone` 命令。 | 用户明确 | 初始需求 | 核心行为仍由 Git CLI 完成。 |
| FACT-002 | 用户指定 Qt GUI、CMake，可使用 CMake Presets。 | 用户明确 | 初始需求 | 保持 Qt Widgets/CMake 技术栈。 |
| FACT-003 | 当前实现已支持父仓库与 0～N 个子仓库、异步输出、取消、安全参数执行、配置恢复和自包含 macOS 交付。 | 已验证 | `src/**`、既有 CTest/交付证据 | 本轮在保持克隆状态机与交付契约下扩展分支发现和表示体验。 |
| FACT-004 | 开发机为 macOS 15.7.5 arm64，CMake 3.27.1、Ninja 1.11.1、Apple Clang 17、Git 2.44.0。 | 已验证 | 版本命令 | 本轮继续原生验证 macOS arm64 `.app`。 |
| FACT-005 | CMake 可找到 `/Users/qingyizhu/Qt5.15.2`，Debug/Release 和 `.app` 已验证。 | 已验证 | CMake cache、既有任务证据 | 本机验证 Qt 5.15.2；Qt 6 仅保持源码兼容。 |
| FACT-006 | Qt 主版本未由用户指定。 | 未知但不阻塞 | 用户只指定 Qt | 继续使用 Qt 5.15/6 公共 API。 |
| FACT-007 | Windows、Linux、签名、公证和安装器未要求；自包含 macOS `.app` 已由后续反馈明确要求。 | 用户明确 | 用户需求与 2026-08-10 追加反馈 | 只扩展 macOS Bundle 部署，不扩展其他平台/发布渠道。 |
| FACT-008 | 用户明确认为当前纵向表单“太丑”，并要求子仓库卡片可任意增删、UI 美观、重启恢复配置。 | 用户明确 | 2026-08-10 反馈与截图 | 必须重做表示层，并改变核心模型与隐私约束。 |
| FACT-009 | 用户指出最终 `.app` 没有图标且体积异常小，希望优化最终应用。 | 用户明确 | 2026-08-10 追加反馈 | macOS Bundle 必须包含应用图标，并提供携带 Qt 运行时的自包含部署产物。 |
| FACT-010 | 当前 Debug Bundle 约 524 KB，`CFBundleIconFile` 为空，且无 `Frameworks`/`PlugIns`；Qt 5.15.2 kit 提供 `macdeployqt`。 | 已验证 | `du`、`plutil`、bundle 结构、`macdeployqt -h` | 小体积来自仅含业务 executable；可用官方工具完成本机自包含部署。 |
| FACT-011 | 用户反馈 Dock 图标外侧出现白色方底；旧图标中间 PNG 由 Quick Look thumbnail 生成，透明画布被合成为白色。 | 用户明确且已验证 | 2026-08-10 截图；旧生成命令与新旧 PNG alpha 检查 | 必须重新生成保留 alpha 的 `.icns`，并把透明角纳入验收。 |
| FACT-012 | 用户要求根据仓库 URL 获取可选分支，分支框仍可手输，并在输入时下拉显示近似名称。 | 用户明确 | 2026-08-11 追加反馈 | 需要异步远程分支查询和可编辑、可搜索的分支选择组件。 |
| FACT-013 | 用户认为克隆完成弹框不美观、完成后“目录已存在”提示含义不准确，且 Git 输出区域太小。 | 用户明确 | 2026-08-11 追加反馈 | 需要非模态完成反馈、空目录语义和可调整的大日志区域。 |
| FACT-014 | `git ls-remote --symref` 可返回远程默认分支与所有 heads，但通用远程协议不提供未获取对象的 `committerdate` 排序。 | 已验证 | Git 2.44.0 只读探测、Git 官方 `git-ls-remote` 文档（2026-08-11 查阅） | “活跃优先”采用远程默认分支、常用工作分支族、稳定名称顺序的可测口径，不宣称按最近提交时间排序。 |
| FACT-015 | 用户截图显示 macOS 原生 QComboBox 下拉子控件在自定义圆角框右侧形成黑色直角边框，与整体视觉不一致。 | 用户明确且已验证 | 2026-08-11 截图、`BranchSelector`/QSS 检查 | 分支选择器必须自绘统一外框和箭头，不能继续暴露原生子控件边框。 |
| FACT-016 | 用户希望所有 Git clone 阶段结束后收到系统消息，并明确要求失败也触发。 | 用户明确 | 2026-08-11 追加反馈 | 除页内结果外，最终 Completed 与 Failed 均需要各触发一次对应桌面通知。 |
| FACT-017 | 用户要求从 GitHub 直接下载 GitHub 托管 runner 编译的 macOS/Windows 成品，并需要说明和实现 macOS 签名/公证、Windows 签名流程。 | 用户明确 | 2026-08-11 追加反馈 | 目标平台扩大为 macOS arm64 与 Windows x64，新增 Actions artifact、tag Release、可选签名和公证。 |
| FACT-018 | 仓库为公开的 `qingyiz/git_clone_gui`，本地 `main` 与 `origin/main` 当前同位于 `6f002e2`，仓库尚无 `.github/workflows`。 | 已验证 | `git remote/status/log`、`gh repo view` | 可新增 GitHub Actions；现有未提交功能必须保留并随流水线一起提交后才会进入 GitHub 构建。 |
| FACT-019 | 当前 Mac 仅有 Apple Development 身份，没有可用于站外分发公证的 Developer ID Application 身份；Windows 代码签名证书也未提供。 | 已验证/未知 | `security find-identity`、用户尚未提供证书 | 流水线必须在无凭据时生成明确标识的未签名测试包，在 Secrets 完整时签名/公证，不能伪造签名成功。 |
| FACT-020 | PR #2 的 Actions run `31506923442` 已在 macOS arm64 与 Windows x64 上完成 Qt 6.8 Release 构建、6/6 CTest、部署、unsigned 打包和 artifact 上传。 | 已验证 | GitHub Actions job/step API、artifact API | Windows 源码编译与两平台 unsigned 交付路径已有原生证据；真实签名、公证与 tag Release 仍待凭据/标签。 |
| FACT-021 | Release `v0.1.0`/`v0.1.1` 的 DMG 校验和与 Bundle 依赖完整，但部署后的 `.app` 仅保留链接器 ad-hoc 签名；新增 Frameworks/PlugIns/Resources 后未重签，`codesign --verify --deep --strict` 报 `code has no resources but signature indicates they must be present`，Safari quarantine 下被 Gatekeeper 显示为“已损坏”。 | 已验证 | 用户截图；下载文件 SHA-256；DMG 挂载、`codesign`/`spctl` 原生复现 | 无 Developer ID 路径也必须重签整个 Bundle 并严格验证；可信无警告分发仍需要 Developer ID 与公证。 |
| FACT-022 | 修复后的 PR #5 run `31511847361` 与标签 `v0.1.2` run `31512200305` 均在 macOS arm64/Qt 6.8 完成 ad-hoc Bundle 重签、6/6 CTest、严格 `codesign`、DMG 打包和上传；标签 run 同时完成 Windows 与 Release job。 | 已验证 | Actions job/step 日志；Release asset API | 发布后的 DMG 不再包含失效 Bundle 签名；无 Developer ID 时 Gatekeeper 仍会要求用户明确覆盖。 |
| FACT-023 | `GitProcessRunner` 已通过 `QProcess::readyRead` 异步转发 merged channels，但 `CloneRequest` 生成的 `git clone` 命令未携带 `--progress`；Git 在标准错误不是终端时默认不持续输出进度。 | 已验证 | `src/infrastructure/GitProcessRunner.cpp`、`src/core/CloneRequest.cpp`；Git 2.44.0 `git clone -h`/行为探测 | 实时链路无需重构；父/子 clone 显式启用进度输出，并在最终日志增加任务总耗时。 |

### 技术与运行环境调查

| 对象 | 探测方法 | 结果 | 结论 |
|---|---|---|---|
| 当前结构 | `inspect_structure.py`、源码/CMake 阅读 | core/application/infrastructure/presentation/app 五层，MainWindow 303 行 | 新增独立卡片组件和存储契约，避免继续膨胀 MainWindow |
| 构建与框架 | 既有 CMake cache 与回归 | Qt 5.15.2 可构建，3 个测试通过 | 可直接实施并原生验证 |
| 参考界面 | 用户截图 | 大面积灰底、单列、控件窄、错误文本占据主视图、无动态列表 | 新 UI 采用双栏、白色卡片、紧凑错误摘要与滚动区域 |
| macOS Bundle | `du`、`plutil`、`find`、`otool` | 524 KB、无图标资源、Qt 依赖仍为 `@rpath` 且未随包部署 | 新增 `.icns` 资源和独立部署阶段 |
| Qt 部署工具 | `/Users/qingyizhu/Qt5.15.2/bin/macdeployqt -h` | 可复制 Qt Framework 与插件并改写依赖 | Release 安装树使用该 kit 对应工具部署 |
| 远程分支能力 | Git 2.44.0 `ls-remote --symref` 与官方文档 | 可在不克隆对象的情况下读取 HEAD 与 `refs/heads/*`；提交日期不可用 | 使用默认/常用分支优先排序，查询失败时保留手工输入 |
| GitHub/runner | `gh repo view`、GitHub-hosted runner 官方文档、run `31506923442` | `windows-2022` x64 与 `macos-15` arm64 均成功完成 Qt 6.8 构建、6/6 CTest 和 artifact 上传 | CI 已在目标操作系统原生验证 unsigned 路径，不能用本机交叉编译替代签名/公证证据 |
| 平台部署 | Qt 官方部署文档 | `windeployqt` 收集 Windows DLL/plugins/runtime；`macdeployqt` 收集 macOS frameworks/plugins 并支持 notarization signing options | 发布包必须在部署工具执行后再上传，不能只上传裸 `.exe` 或 build-tree `.app` |

## 问题与目标

### 问题陈述

既有多仓库、配置恢复、分支发现、跨平台打包与发布已完成。2026-08-12 的追加反馈表明，大型父仓库在完成前看不到持续 Git 传输日志，用户无法判断任务是否仍在推进，最终日志也缺少本次克隆的总耗时。

### 目标与成功指标

- 子仓库以独立卡片展示，支持添加、删除任意数量，并按卡片顺序克隆。
- 允许没有子仓库的父项目单独克隆；存在的每张卡片必须填写完整。
- 默认窗口在 1100×760 左右即可同时看到主要配置、状态、预览和日志入口，不再依赖超高窗口。
- 使用一致的圆角卡片、间距、字号、强调色、悬停/禁用状态和紧凑错误摘要。
- 表单变化后持久化父仓库、所有子仓库顺序及目标根目录；重启恢复。
- 保持 shell 安全、失败停止、取消、日志和 macOS `.app` 交付行为。
- 为 macOS `.app` 提供与界面视觉一致的图标，并在 Finder/Dock 使用该图标。
- 生成携带所需 Qt Framework 与 platform plugin 的自包含安装树 `.app`，同时保留轻量开发构建物以缩短日常编译。
- 仓库 URL 稳定后异步获取远程分支，默认与常用工作分支优先，分支框支持选择、自由输入和包含式搜索。
- 允许克隆到不存在或已存在但为空的目标目录；非空目录给出明确提示。
- 使用页内完成/失败状态替代系统完成弹框，并让 Git 输出默认占据更大且可拖动调整的区域。
- GitHub 每次推送/PR 在 macOS arm64 与 Windows x64 原生 runner 上构建、测试并生成可下载 artifact；推送 `v*` 标签时自动创建 Release 并附加 DMG/ZIP。
- macOS 发布包为包含自包含 `.app` 的 DMG；配置 Developer ID 与公证 Secrets 后必须签名、启用 Hardened Runtime、提交 Apple 公证并 stapling。
- Windows 发布包为包含 `.exe`、Qt DLL、platform plugin 与 MSVC runtime 的便携 ZIP；配置 PFX Secret 后必须 Authenticode 签名主程序。
- 父项目和子仓库克隆在非终端 GUI 管道中仍应持续输出 Git 传输进度；任务最终成功、失败或取消时打印总耗时。

### 非目标

- 不保存 Git 密码、Token、SSH Key、环境变量或运行日志。
- 不支持拖拽排序；卡片顺序即添加顺序，删除后剩余卡片自动重编号。
- 不新增 pull/fetch/checkout、Git submodule 写入、远端账号管理或自动清理。
- 不为判断最近提交而浅克隆所有分支，不接入 GitHub/GitLab 等托管平台专用 API。
- 不制作 PKG/MSI/App Store/Microsoft Store 安装包，不实现应用内自动更新。
- 不在仓库或 artifact 中保存证书私钥、证书密码、Apple ID app-specific password 或其他发布凭据。

## 角色、术语与范围

### 角色

| 角色 | 目标 | 权限或限制 |
|---|---|---|
| 开发者 | 一次配置并反复获取一组父项目与若干嵌套仓库 | 本机 Git/凭据已配置；对目标根目录可写 |

### 术语

| 术语 | 精确定义 |
|---|---|
| 父项目 | 第一个克隆的仓库，提供所有子仓库的目标根。 |
| 子仓库卡片 | 一组 URL、分支、父项目内相对路径输入及删除操作。 |
| 子仓库列表 | 0～N 张有顺序的卡片；执行顺序与显示顺序一致。 |
| 保存配置 | 由 `QSettings` 持久化的表单字段和卡片顺序，不含日志/凭据。 |
| 克隆任务 | 父阶段加 0～N 个子阶段组成的单个互斥任务。 |
| Actions artifact | 每次成功 workflow run 可在 Actions 页面下载、具有保留期限的 CI 构建包，不等同于长期 Release。 |
| GitHub Release | `v*` 标签触发后创建的长期版本下载页，附件为 macOS DMG 与 Windows ZIP。 |
| 签名模式 | 对应平台 Secrets 完整时执行真实签名/公证；Secrets 缺失时生成未签名测试包并明确记录状态。 |

### 系统边界与依赖

- 范围内：动态卡片、紧凑现代界面、输入校验、远程分支发现/搜索、命令预览、顺序克隆、取消、实时日志、配置保存/恢复、GitHub Actions 跨平台构建与 Release 交付。
- 范围外：认证管理、仓库内容、凭据安全存储、按提交时间分析远程分支活跃度。
- 外部依赖：Git CLI、Qt Core/Widgets/Settings、文件系统；构建交付阶段使用对应 Qt kit 的 `macdeployqt`/`windeployqt`、GitHub Actions，以及可选的 Apple/Windows 签名服务。

## 用户旅程

1. 应用启动并恢复上次父仓库、子仓库卡片顺序和目标根目录；首次启动默认显示一张空子仓库卡片。
2. 用户填写父项目，并通过“添加子仓库”新增卡片或删除不需要的卡片。
3. 右侧预览随输入更新，错误以紧凑摘要呈现，不用整块红字占据主界面。
4. 用户点击开始；系统保存当前配置、冻结控件，先克隆父项目，再按卡片顺序逐个克隆子仓库。
5. 克隆过程中 Git 传输进度随输出到达实时追加；任一阶段失败/取消即停止，最终结果同时显示任务总耗时。
6. 用户退出再打开，表单和卡片恢复，日志保持为空。
7. 开发者构建 Release 并安装后，得到带图标、携带 Qt 运行时、可直接启动的 macOS `.app`。
8. 用户输入仓库 URL 后，从默认/常用优先的分支列表中选择，或输入关键词筛选后继续手工填写。
9. 克隆完成后，执行中心以内嵌成功状态展示结果，Git 输出保持可查看；再次执行前仅在目标目录非空时提示必须为空。
10. 维护者推送普通提交后从 Actions 下载测试包；推送 `v1.2.3` 等标签后从 GitHub Releases 下载 macOS DMG 或 Windows ZIP。配置签名 Secrets 后，标签发布包同时具有对应平台信任链。

## 功能需求

### REQ-001：配置父项目与子仓库列表

**用户故事：** 作为开发者，我希望在一个界面维护父项目和若干子仓库，从而覆盖不同项目组合。

- 优先级：Must
- 前置条件：应用已启动。
- 结果/副作用：更新界面模型并触发延迟保存，不启动 Git。

#### 验收标准

- AC-001.1：当用户填写父 URL、父分支、父目录名、目标根目录及任意数量子卡片时，系统应当按显示顺序保存全部输入。
- AC-001.2：当任一字段、卡片数量或顺序变化时，系统应当更新包含父命令和全部子命令的预览。
- AC-001.3：如果父字段非法、目标根不存在、任一现存子卡片字段不完整、子路径逃逸或多个子卡片目标路径相同，系统应当阻止启动并指出具体卡片序号。
- AC-001.4：当没有子卡片且父项目输入有效时，系统应当允许只克隆父项目。
- AC-001.5：目录选择器取消不得改变原目标根目录。

### REQ-002：顺序执行父项目与全部子仓库

**用户故事：** 作为开发者，我希望一次点击顺序克隆父项目和所有子仓库，从而不用重复执行命令。

- 优先级：Must
- 前置条件：REQ-001 校验通过且当前无任务。
- 结果/副作用：创建父工作树及 0～N 个嵌套子工作树。

#### 验收标准

- AC-002.1：系统应当通过 `QProcess::start(program, arguments)` 结构化参数执行父命令。
- AC-002.2：仅当父阶段退出码为 0 时，系统应当从第 1 张卡片开始按顺序执行子命令。
- AC-002.3：仅当前一子仓库退出码为 0 时，系统才应启动下一子仓库；任一失败立即停止队列。
- AC-002.4：如果子仓库数为 0，父阶段成功后任务应直接完成。
- AC-002.5：运行期间配置、添加、删除和开始控件应禁用；取消保持可用。
- AC-002.6：完成信息应包含父项目路径及成功克隆的子仓库数量。

### REQ-003：显示过程、失败并支持取消

**用户故事：** 作为开发者，我希望看到当前阶段、Git 输出并能取消，从而诊断问题并及时停止。

- 优先级：Must
- 前置条件：任务可能空闲或处于任一克隆阶段。
- 结果/副作用：日志仅驻留当前会话；取消不清理文件。

#### 验收标准

- AC-003.1：Git 输出应按到达顺序追加，日志最多保留 10,000 个文本块。
- AC-003.2：状态区应显示“父项目”或“子仓库 i/N”的当前阶段。
- AC-003.3：启动错误、异常退出或非零退出码应停止队列，显示具体阶段与错误并保留文件。
- AC-003.4：取消先 terminate，3 秒未退出再 kill；结束后恢复所有配置控件。
- AC-003.5：关闭运行中窗口应先取消当前 Git 进程再退出。
- AC-003.6：父项目和每个子仓库的 `git clone` 命令应显式启用进度输出，使标准错误连接 GUI 管道而非终端时仍能持续产生 Git 传输进度；应用应通过既有异步信号链按到达顺序立即追加这些输出，不等待阶段结束。
- AC-003.7：从有效克隆任务开始计时；无论最终成功、失败或取消，最终会话日志都应包含一次格式稳定、精度到 0.1 秒的“总耗时：N.N 秒”，无效输入或找不到 Git 而未启动任务时不计时。

### REQ-004：提供可复现构建与使用入口

**用户故事：** 作为使用者，我希望继续用 CMake Preset 构建并直接启动 `.app`。

- 优先级：Must
- 前置条件：具备 Qt/CMake/C++/Git。
- 结果/副作用：生成开发产物。

#### 验收标准

- AC-004.1：Debug/Release preset 应成功构建并通过全部测试。
- AC-004.2：macOS arm64 Debug 产物应位于 `build/debug/bin/GitCloneGui.app` 且可启动。
- AC-004.3：README 应说明多子仓库、配置保存位置/内容与隐私边界。
- AC-004.4：Qt 6 或 Qt 5.15 的查找策略和缺失诊断应保持有效。

### REQ-005：管理子仓库卡片

**用户故事：** 作为开发者，我希望用卡片增删子仓库，从而直观看到每个仓库的独立配置。

- 优先级：Must
- 前置条件：任务空闲。
- 结果/副作用：改变子仓库列表并触发预览/保存。

#### 验收标准

- AC-005.1：点击“添加子仓库”应在列表末尾新增一张包含 URL、分支、相对路径和删除按钮的空卡片。
- AC-005.2：点击某卡片删除按钮应只删除该卡片，剩余卡片标题应连续重编号。
- AC-005.3：卡片列表超过可见高度时应在配置栏内部滚动，而不是无限增高主窗口。
- AC-005.4：首次启动无保存配置时应默认创建一张空卡片；用户删除全部卡片后重启仍应保持 0 张。

### REQ-006：提供紧凑且一致的现代界面

**用户故事：** 作为开发者，我希望界面清晰美观，从而快速配置而不被大面积空白和错误文本干扰。

- 优先级：Must
- 前置条件：应用启动。
- 结果/副作用：只影响表示和交互布局。

#### 验收标准

- AC-006.1：主窗口应采用左侧配置、右侧执行信息的双栏布局；窗口默认尺寸不超过 1180×800，最小尺寸不超过 960×680。
- AC-006.2：父项目、每个子仓库和目标位置应以白色圆角卡片呈现，使用统一 8/12/16/24 像素间距层级和蓝色主操作色。
- AC-006.3：输入框、主要/次要/危险按钮应具有明确的正常、悬停、聚焦和禁用状态。
- AC-006.4：空表单错误应以最多 3 行摘要和剩余数量呈现，不应把所有错误逐行铺满主窗口。
- AC-006.5：命令预览与 Git 输出应在右栏分区显示并使用等宽字体；日志区域应优先获得剩余空间。

### REQ-007：保存并恢复上次配置

**用户故事：** 作为重复使用者，我希望再次打开时恢复上次输入，从而不必重新填写项目组合。

- 优先级：Must
- 前置条件：应用有可写的用户配置目录。
- 结果/副作用：写入当前用户范围的应用设置。

#### 验收标准

- AC-007.1：任一配置变化后 300 毫秒内，系统应当保存父字段、目标根目录和全部子卡片字段/顺序。
- AC-007.2：应用启动时应恢复最后一次成功写入的配置；保存的 0 张子卡片必须与首次启动区分。
- AC-007.3：保存失败时系统应当在状态区给出非阻塞提示，不能影响当前会话继续使用。
- AC-007.4：系统不得持久化 Git 输出、运行状态、密码、Token、SSH Key 或环境变量；README 应提醒不要把 Token 嵌入 URL。
- AC-007.5：运行期间不因控件禁用丢失已保存配置；关闭窗口前应强制刷新待保存变化。

### REQ-008：提供有图标的自包含 macOS 应用

**用户故事：** 作为使用者，我希望最终 `.app` 有正式图标并自带 Qt 运行环境，从而能把它作为真正的桌面应用直接打开。

- 优先级：Must
- 前置条件：在已验证的 macOS arm64 + Qt 5.15.2 kit 上完成 Release 构建与安装。
- 结果/副作用：安装树 Bundle 体积增加，并包含 Qt Framework、插件和图标资源。

#### 验收标准

- AC-008.1：macOS Bundle 的 `Info.plist` 应设置非空 `CFBundleIconFile`，且 `Contents/Resources` 中存在对应 `.icns` 文件。
- AC-008.2：项目应保留可编辑的图标源资产，图标应与应用内蓝色 “G” 视觉语言一致，并能在构建中稳定复用。
- AC-008.3：Release 安装产物 `build/install/GitCloneGui.app` 应包含所需 Qt Framework 与 `platforms/libqcocoa.dylib`，且 Qt 动态依赖只指向 Bundle 内的 `@rpath`，不依赖 `/Users/qingyizhu/Qt5.15.2` 等开发机绝对路径。
- AC-008.4：自包含安装产物应通过 delivery self-contained 检查并可由 `open -n` 启动；开发构建物与安装/部署产物的区别应写入 README。
- AC-008.5：图标圆角外的四个角必须为透明 alpha，不得包含白色或其他不透明方形画布；Dock/Finder 显示不得出现额外白底。

### REQ-009：发现并搜索远程分支

**用户故事：** 作为开发者，我希望输入仓库 URL 后直接选择或搜索远程分支，从而不用记忆和重复输入完整分支名。

- 优先级：Must
- 前置条件：应用空闲，仓库 URL 非空且本机 Git 可用；远端认证沿用用户现有 Git 配置。
- 结果/副作用：只读取远程 refs，不克隆仓库对象、不修改磁盘工作树。

#### 验收标准

- AC-009.1：当父仓库或任一子仓库 URL 停止变化约 450 毫秒时，系统应当异步执行结构化参数的 `git ls-remote --symref <url> HEAD refs/heads/*`，界面不得阻塞。
- AC-009.2：查询成功时，下拉列表应包含去重后的远程 heads；远程默认分支排第一，随后依次为 `main/master/trunk`、`develop/development/dev`、`release/hotfix/feature/bugfix/fix` 分支族，其余名称按不区分大小写的稳定顺序排列。
- AC-009.3：分支控件应保持可编辑；用户输入任意文本时不得强制改写，并应在弹出建议中以不区分大小写的“包含”匹配显示近似分支名称。
- AC-009.4：如果查询失败、超时、URL 已变化或远端需要未配置的认证，系统应当忽略过期结果并保留手工输入能力；错误以非阻塞提示呈现，不阻止其他字段编辑和克隆校验。
- AC-009.5：多个父/子 URL 查询可独立完成；克隆任务启动后所有分支输入与 URL 输入一并禁用，查询进程不得占用克隆用的 `ProcessRunner`。
- AC-009.6：分支选择器折叠状态应与普通输入框保持相同的 8px 圆角、浅色 1px 边框和聚焦色；右侧只显示无独立黑框的细线箭头与浅色分隔线，展开时箭头方向应翻转，禁用/悬停/聚焦状态均不得退回平台原生直角边框。

### REQ-010：优化目录、完成反馈和 Git 输出体验

**用户故事：** 作为开发者，我希望克隆结果与日志在主界面中清晰可见，并获得准确的目录提示，从而能继续检查输出或修正配置。

- 优先级：Must
- 前置条件：应用已启动；克隆任务可能完成、失败或尚未开始。
- 结果/副作用：只调整校验和表示，不自动删除、覆盖或打开目录。

#### 验收标准

- AC-010.1：如果父项目目标路径不存在或已存在且为空，系统应当允许启动；如果路径是文件或目录内已有任何条目，系统应当阻止启动并明确提示“父项目目标目录必须为空”。
- AC-010.2：克隆完成后，系统应当在执行中心显示带成功视觉状态、结果摘要和父项目路径的页内反馈，不弹出系统信息框；完成反馈不得立即被目标目录非空校验覆盖。
- AC-010.3：克隆失败时，系统应当在同一状态区显示失败视觉状态与具体阶段，Git 输出和已下载文件继续保留。
- AC-010.4：Git 输出卡片应当在默认 1160×780 窗口中获得不少于 280 像素的可视高度，并与命令/状态区域通过纵向分隔条调整；日志仍保持无换行和 10,000 文本块上限。
- AC-010.5：当控制器产生最终 `Completed` 时，系统应当发送一次标题为“GitCloneGui · 克隆完成”的桌面通知，正文包含父项目路径和成功子仓库数量；当任一父/子阶段产生最终 `Failed` 时，应当发送一次标题为“GitCloneGui · 克隆失败”的桌面通知，正文包含具体阶段与错误。中间阶段成功和用户主动取消不得发送通知。若系统托盘消息不可用或用户关闭通知权限，页内结果仍应保留且任务 outcome 不受影响。

### REQ-011：通过 GitHub 自动构建并发布 macOS/Windows 成品

**用户故事：** 作为使用者，我希望直接从 GitHub 下载已包含运行时依赖的 macOS 或 Windows 软件，从而不必安装 Qt/CMake 自行编译。

- 优先级：Must
- 前置条件：代码与 workflow 已推送到 GitHub；GitHub Actions 可访问 Qt 安装源。
- 结果/副作用：Actions 保存短期 artifact；`v*` 标签额外创建长期 GitHub Release。

#### 验收标准

- AC-011.1：对 `main` push、PR、手动触发和 `v*` tag，workflow 应分别在 `macos-15` arm64 与 `windows-2022` x64 原生 runner 上用 Qt 6.8 LTS 进行 Release 配置、编译和全部 CTest；macOS 使用 CMake/Ninja，Windows 使用 CMake/Visual Studio 2022 x64。
- AC-011.2：Windows job 应运行 `cmake --install` 与 `windeployqt`，生成 `GitCloneGui-Windows-x64.zip`；解压后的根目录必须包含 `GitCloneGui.exe`、Qt DLL、`platforms/qwindows.dll` 和 MSVC runtime，不依赖 runner 的 Qt 安装目录。
- AC-011.3：macOS job 应运行 `cmake --install` 与 `macdeployqt`，生成 `GitCloneGui-macOS-arm64.dmg`；DMG 内必须包含带 `.icns`、Qt Frameworks 和 Cocoa plugin 的 `GitCloneGui.app`。没有 Developer ID 时，`macdeployqt` 仍应使用 ad-hoc identity `-` 对整个部署 Bundle 一致重签，随后 `codesign --verify --deep --strict` 必须通过，不能上传签名结构已失效而被 Gatekeeper 判为“已损坏”的包。
- AC-011.4：当 macOS Developer ID 与 Apple 公证 Secrets 完整时，workflow 应使用 `Developer ID Application` 身份和 Hardened Runtime 签名，使用 `notarytool` 公证最终 DMG、staple ticket，并验证签名/公证；凭据缺失时不得声称具有 Developer ID 信任链，只能生成通过 Bundle 签名结构验证的 ad-hoc 测试 artifact，并明确提示首次打开仍需在“隐私与安全性”中选择“仍要打开”。
- AC-011.5：当 Windows PFX 与密码 Secrets 完整时，workflow 应使用 `signtool` 和 RFC3161 timestamp 对 `GitCloneGui.exe` 执行 Authenticode 签名并验证；凭据缺失时生成未签名测试 artifact，不在日志打印私钥或密码。
- AC-011.6：普通成功运行应上传两个具名 artifact；仅当 ref 为 `refs/tags/v*` 时，release job 才能以最小 `contents: write` 权限创建/更新对应 GitHub Release 并附加 DMG/ZIP。
- AC-011.7：README 应说明 Actions artifact 与 Release 的区别、标签发版命令、所有 Secrets 的生成/配置方法、未签名包的系统提示以及证书采购前提。

## 非功能需求

| ID | 类别 | 可测约束 | 测量方式 |
|---|---|---|---|
| NFR-001 | 安全 | 所有 Git 阶段使用结构化 QProcess 参数，不使用 shell/system | 静态检查与元字符测试 |
| NFR-002 | 可靠性 | 单实例内最多一个任务；父失败或任一子失败不启动后续项 | fake runner 状态机与真实 Git 集成测试 |
| NFR-003 | 响应性 | Git 与配置保存不阻塞 UI；Git clone 在非终端管道中显式启用进度并按 `readyRead` 到达实时追加；卡片增删即时反馈 | 命令计划测试、异步代码审查与 controller/UI 测试 |
| NFR-004 | 兼容性 | 使用 Qt 5.15/6 共有 Core/Widgets/Settings API | Qt 5 构建 + Qt 6 API 审查 |
| NFR-005 | 隐私 | 只持久化表单字段；不保存日志或独立凭据；明示 URL 中 Token 风险 | store 单元测试与 README 审查 |
| NFR-006 | 可维护性 | MainWindow 不拥有单卡字段构建细节；卡片和设置适配器分别独立 | 结构检查和 include/link 审查 |
| NFR-007 | 可部署性 | Release 安装树包含图标、Qt Framework 与 Cocoa platform plugin，无 Qt 开发机绝对依赖，且完整 Bundle 通过严格代码签名结构验证 | bundle 结构、`otool`、`codesign --verify --deep --strict`、delivery self-contained、启动检查 |
| NFR-008 | 远程查询 | 每个分支查询 15 秒超时，设置 `GIT_TERMINAL_PROMPT=0`，结果按请求 ID 隔离并进行会话缓存 | 本地远程测试、超时与过期结果测试 |
| NFR-009 | 通知可靠性 | 系统通知是最终 Completed/Failed 后的附加副作用；不支持/无权限时静默降级，不能阻塞 UI、改变 outcome 或重复发送 | 完成/失败/取消信号测试与 Qt tray capability 审查 |
| NFR-010 | 发布安全 | Actions 默认只读；仅 release job 写 contents；第三方 Actions 固定 commit SHA；证书只从 Secrets 注入临时文件/钥匙串并在 job 结束销毁 | workflow 静态审查、GitHub run 日志、签名脚本审查 |
| NFR-011 | 可观测性 | 每个已启动任务的最终日志在 Completed/Failed/Cancelled 三种结果下均且仅包含一次单调计时得到的总耗时，显示精度 0.1 秒 | fake runner 三种结果测试与日志计数断言 |

## 边界、错误与状态转换

| 场景 | 预期行为 | 关联 ID |
|---|---|---|
| 0 个子仓库 | 只克隆父项目并成功结束 | REQ-001, REQ-002 |
| 20 个子仓库 | 卡片内部滚动，按顺序执行，UI 不同步阻塞 | REQ-002, REQ-005, NFR-003 |
| 中间子仓库失败 | 停止后续队列，保留父及已完成/部分目录 | REQ-002, REQ-003 |
| 大型父仓库长时间传输 | 通过 `--progress` 让 Git 在 GUI 管道中持续产出进度，异步追加而不等待父阶段结束 | REQ-003, NFR-003 |
| 两张卡片使用相同目标路径 | 校验失败并指出卡片序号 | REQ-001 |
| 删除全部卡片后重启 | 仍为 0 张，不自动补回默认卡片 | REQ-005, REQ-007 |
| 设置文件不可写 | 显示非阻塞提示，当前输入仍可使用 | REQ-007 |
| URL 内含 Token | 作为普通字段会被保存；应用提醒不要这样输入 | REQ-007, NFR-005 |
| 本机 Qt 未提供 `macdeployqt` | 配置阶段明确报错，开发构建仍可诊断，不生成伪自包含产物 | REQ-008, NFR-007 |
| 重复安装同一前缀 | 覆盖/更新部署资源且保持可启动，不残留开发机路径 | REQ-008, NFR-007 |
| SVG 转换器合成背景色 | 生成后检查 RGBA 与四角 alpha；不满足则禁止作为 `.icns` 输入 | REQ-008, NFR-007 |
| URL 连续快速变化 | 取消旧查询并忽略晚到结果，只展示最后一个 URL 的分支 | REQ-009, NFR-008 |
| 私有仓库认证失败/超时 | 保留可编辑分支文本并给出非阻塞提示，不弹出凭据输入 | REQ-009, NFR-008 |
| 目标目录已存在但为空 | 允许 Git clone 使用该目录 | REQ-010 |
| 克隆完成后目标目录变为非空 | 页内完成反馈保持可见；用户再次编辑时恢复实时校验 | REQ-010 |
| 父项目或任一子仓库失败 | 保留页内结果与日志，并发送一次“克隆失败”系统通知 | REQ-003, REQ-010, NFR-009 |
| 用户主动取消 | 保留页内结果与日志，不发送系统通知 | REQ-003, REQ-010, NFR-009 |
| 成功、失败或取消结束 | 最终会话日志各追加且仅追加一次总耗时；未实际启动的校验失败不打印耗时 | REQ-003, NFR-011 |
| 系统通知不可用或权限被拒绝 | 静默保留页内最终状态，不影响 Completed/Failed 结果 | REQ-010, NFR-009 |
| 签名 Secrets 缺失 | CI 继续构建测试包；macOS Bundle 以 ad-hoc identity 完整重签并严格验证，step summary 明确无 Developer ID 信任链；不执行公证/Authenticode | REQ-011, NFR-007, NFR-010 |
| 只有部分 macOS Secrets | 不进入签名/公证路径，避免生成看似正式但无法验证的包 | REQ-011, NFR-010 |
| 推送普通分支/PR | 只上传 Actions artifact，不创建 GitHub Release | REQ-011 |
| 推送 `v*` 标签 | 两个平台都成功后创建/更新同名 Release 并附加两个平台包 | REQ-011 |

## 约束、假设与风险

### 已确认约束

- 保持 Qt/CMake/Git CLI 与 macOS arm64 原生验证（FACT-002、FACT-004、FACT-005）。
- UI、多子仓库和配置恢复均为 required（FACT-008）。
- macOS 图标与自包含 Release 安装产物为 required（FACT-009、FACT-010）。
- 远程分支发现、页内结果反馈、空目录语义和大日志区域为 required（FACT-012、FACT-013）。
- GitHub 托管的 macOS arm64/Windows x64 构建、部署包和标签 Release 为 required；真实签名/公证执行依赖用户配置对应平台 Secrets（FACT-017～FACT-019）。

### 待验证假设

- ASM-001：用户接受按卡片添加顺序执行，不需要拖拽排序。
- ASM-002：用户接受 0 个子仓库时只克隆父项目。
- ASM-003：配置使用系统标准 `QSettings` 用户路径，无需用户选择配置文件。

### 风险

- RISK-001：大量卡片会增加预览长度；使用独立滚动区域控制。
- RISK-002：仓库 URL 可能人为嵌入 Token；无法可靠识别所有格式，因此文档/UI 提醒并不保存独立凭据。
- RISK-003：失败/取消仍保留文件，避免自动删除风险。
- RISK-004：自包含部署显著增加体积；这是包含 Qt 运行时的预期结果，开发构建物继续保持轻量。
- RISK-005：未签名/未公证的 `.app` 在其他 Mac 上会触发 Gatekeeper；流水线允许测试 artifact 降级，但 Release 是否达到可信分发取决于 Developer ID 与公证 Secrets。
- RISK-006：通用 Git ref 广告不含提交时间；以默认/常用分支优先满足快速选择，真实“最近提交”排序留待未来明确接入托管 API 或受控抓取后实现。
- RISK-007：大量远程分支会扩大下拉模型；单次仅保存名称并用会话缓存，输入过滤由 Qt 模型完成。
- RISK-008：Apple Developer ID 与 Windows Authenticode 证书都由外部机构签发且可能收费/过期；代码只能验证和消费凭据，不能代替用户取得证书。
- RISK-009：GitHub runner/Qt 下载源和 Action 版本会变化；固定 runner 主版本、Qt LTS 范围和 Action commit，并以真实 Actions run 作为最终平台证据。

## 需求分析记录

| ID | 类型 | 涉及需求 | 发现 | 决议/接受风险 |
|---|---|---|---|---|
| ANA-001 | 变更 | REQ-001, REQ-002 | 单子仓库模型不能满足任意数量 | 改为 0～N 列表并顺序执行 |
| ANA-002 | 缺口 | REQ-005 | “任意数量”是否包含 0 未说明 | 采用更通用的 0～N，首次默认 1 张但允许删除全部 |
| ANA-003 | 冲突 | REQ-007, NFR-005 | 旧需求禁止保存 URL，新需求要求保存配置 | 更新隐私边界：保存表单，禁止日志/独立凭据并提示 Token 风险 |
| ANA-004 | 可测试性 | REQ-006 | “美观”主观 | 转化为双栏、尺寸、卡片、间距、状态、错误摘要和等宽日志等可检查标准 |
| ANA-005 | 架构 | REQ-005, NFR-006 | MainWindow 已 303 行，直接堆动态卡片会跨预算 | 独立 `ChildRepositoryCard` 和配置存储契约 |
| ANA-006 | 交付缺口 | REQ-008 | “体积小”并非业务代码缺失，而是 Qt Framework/插件未部署 | 保留开发 Bundle，新增 self-contained 安装产物，避免每次增量构建都复制 Qt |
| ANA-007 | 资源缺口 | REQ-008 | `CFBundleIconFile` 为空且 Resources 不存在 | 增加 SVG 源资产、生成 `.icns` 并作为 MACOSX_PACKAGE_LOCATION Resources 输入 |
| ANA-008 | 图标回归 | REQ-008 | 只检查尺寸/文件存在无法发现白色方底 | 使用保留透明通道的 SVG 转换路径，并显式验证四角 alpha 为 0 |
| ANA-009 | 歧义 | REQ-009 | “活跃分支”若解释为最近提交，需要获取对象或平台 API，和通用 URL/轻量查询冲突 | 定义为远程默认分支与常用工作分支族优先，并明确不宣称提交时间排序 |
| ANA-010 | 状态冲突 | REQ-010 | 克隆成功后重新校验会立即把成功状态替换成“目录已存在” | 操作结果状态优先保持，下一次用户编辑再恢复配置校验 |
| ANA-011 | 行为修正 | REQ-001, REQ-010 | Git 本身允许克隆到已存在空目录，当前代码却拒绝任何已存在路径 | 允许空目录，仅拒绝文件或非空目录，并用测试保护 |
| ANA-012 | 视觉回归 | REQ-006, REQ-009 | 全局 QSS 未完全覆盖 macOS 原生 QComboBox 子控件，导致右侧黑色直角边框 | `BranchSelector` 自绘 chrome/chevron，QSS 仅负责 popup 与编辑器，增加 snapshot 检查 |
| ANA-013 | 触发边界 | REQ-010 | 通知必须排除父阶段/中间子项完成，但用户追加要求最终失败也提示 | MainWindow 仅对最终 `jobFinished(Completed|Failed)` 各发一次对应通知请求，Cancelled 不发，由独立 desktop notifier 适配系统能力 |
| ANA-014 | 平台范围变化 | REQ-011 | 原 Spec 明确排除 Windows/签名/Release，现需求与其冲突 | 重新打开 requirements，新增双平台原生 runner、部署包、可选签名与标签 Release 契约，不改变业务架构 |
| ANA-015 | 凭据边界 | REQ-011 | 当前只有 Apple Development 证书，不能用于站外可信分发/公证；Windows 证书未知 | workflow 无 Secrets 时产出测试包，有完整 Secrets 时严格签名验证；文档明确证书申请步骤 |
| ANA-016 | macOS 交付回归 | REQ-011 | `macdeployqt` 添加资源与嵌套代码后保留了失效的链接器 ad-hoc 签名；只检查文件结构和启动、不执行严格 `codesign`，未发现 Safari 下载后的 Gatekeeper “已损坏” | 无 Developer ID 时调用 `macdeployqt -codesign=-` 重签全部嵌套代码；打包脚本无条件执行严格 Bundle 验证，并用带 quarantine 的副本验证系统行为 |
| ANA-017 | 实时输出根因 | REQ-003 | `QProcess::readyRead` 已实时读取，但 Git 检测到 stderr 非终端后默认抑制 clone 进度，造成大型父仓库完成前近似无日志 | 在 core 生成的父/子 `git clone` 参数中显式加入 `--progress`；controller 使用单调计时器统计整个任务并在统一 finish 路径打印耗时 |

## 需求追踪

| 需求 | 验收标准 | 成功证据 |
|---|---|---|
| REQ-001 | AC-001.1～AC-001.5 | core 列表校验测试、UI 测试 |
| REQ-002 | AC-002.1～AC-002.6 | controller 队列测试、真实 Git 多仓库测试 |
| REQ-003 | AC-003.1～AC-003.7 | core 命令参数测试、controller 实时转发/三种结果耗时回归、GUI 冒烟 |
| REQ-004 | AC-004.1～AC-004.4 | 双 preset、CTest、bundle 验证、README |
| REQ-005 | AC-005.1～AC-005.4 | presentation 测试与视觉检查 |
| REQ-006 | AC-006.1～AC-006.5 | 对象/尺寸测试、截图人工检查 |
| REQ-007 | AC-007.1～AC-007.5 | QSettings 临时路径往返测试、UI 恢复测试 |
| REQ-008 | AC-008.1～AC-008.5 | plist/资源/alpha 检查、self-contained delivery、`otool`、Finder/Dock 启动检查 |
| REQ-009 | AC-009.1～AC-009.6 | branch service、本地远程集成、presentation 搜索测试与 selector snapshot |
| REQ-010 | AC-010.1～AC-010.5 | core 目录测试、presentation 状态/布局/通知信号测试与 snapshot |
| REQ-011 | AC-011.1～AC-011.7 | workflow/schema 审查、macOS/Windows 原生 Actions run、artifact 结构、ad-hoc Bundle 严格签名验证、Developer ID/公证门控、tag Release |

## 未决问题

- ad-hoc Bundle 重签已在本机 quarantine 等价场景和 macOS 原生 runner 中验证。真实可信、无 Gatekeeper 覆盖步骤的发布仍需用户取得 Developer ID Application、Apple 公证凭据及可选 Windows Authenticode PFX，并配置 GitHub Secrets；在此之前 macOS Release 仅属于无 Developer ID 信任链的测试版。
