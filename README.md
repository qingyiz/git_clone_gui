# GitCloneGui：多仓库克隆工具

一个紧凑的 Qt Widgets 桌面 Git 工作台：既能先克隆父项目、再按卡片顺序克隆任意数量的子仓库，也能递归发现工作目录中的本地 Git 仓库并集中切换分支。

程序实际调用本机 `git`，并通过结构化参数启动进程，不会把表单内容拼接成 shell 命令。任一步骤失败或取消后立即停止后续队列，也不会自动删除已下载文件。

## 主要功能

- 左侧导航在“仓库克隆”和“仓库工作区”之间切换，关闭应用后仍会恢复上次页面；
- 子仓库使用独立卡片，可随时添加、删除并自动重编号；
- 支持 0 个子仓库，此时只克隆父项目；
- 按界面卡片顺序逐个执行，状态显示当前子仓库 `i/N`；
- 输入仓库 URL 后自动读取远程分支；默认与常用分支优先，可下拉选择、自由输入或按关键词搜索；
- 双栏界面：左侧配置，右侧命令预览、状态和实时 Git 输出；
- 完成与失败在执行中心页内显示，Git 输出区域默认更大并可拖动分隔条调整；
- 全部任务最终成功或任一阶段失败后发送对应系统通知；用户取消不误报；
- 自动保存父项目、目标目录、所有子仓库字段和顺序；
- macOS 应用包含蓝色 “G + Git 分支” 图标；
- 支持取消：先请求终止，3 秒未退出则强制结束；
- 严格校验路径逃逸、重复子仓库目标和已存在的父目录。
- 工作区页面递归发现普通仓库、嵌套仓库以及 `.git` 为文件的 worktree/submodule 形态；
- 工作目录变化后自动保存；重启应用会恢复输入，有效目录会在后台自动扫描一次；
- 选中仓库后显示当前分支、本地分支、全部远端跟踪分支，以及本地尚无同名分支的远端候选；
- 选中仓库后实时检查工作树；干净状态明确说明，有已暂存、未暂存、未跟踪或冲突项时以醒目警示卡提示谨慎切换；
- 本地分支通过 `git switch -- <branch>` 切换，远端候选通过 `git switch --track -- <remote>/<branch>` 创建跟踪分支；`--` 用于明确结束 Git 选项解析。

## 直接下载

长期版本从 [GitHub Releases](https://github.com/qingyiz/git_clone_gui/releases) 下载：

当前版本：`0.1.4`。应用左侧底部、macOS Bundle 元数据与 GitHub 标签使用同一版本号。

- `GitCloneGui-macOS-arm64.dmg`：Apple Silicon Mac（M1/M2/M3/M4 等）；
- `GitCloneGui-Windows-x64.zip`：64 位 Windows 10/11 便携包，解压后运行 `GitCloneGui.exe`。

每次 `main` 推送、PR 或手动运行流水线也会生成同名 Actions artifact，保留 14 天，适合测试。Release 由 `v*` 标签触发，适合长期下载。无签名 Secrets 时流水线仍会生成测试包，但 macOS 会显示 Gatekeeper 警告，Windows 也不会显示受信任发布者；正式对外发布前应配置后文的签名凭据。

## 环境要求

- CMake 3.21+
- 支持 C++17 的编译器
- Qt 6，或 Qt 5.15（Core、Widgets、Test）
- Git CLI
- 推荐 Ninja；仓库 Preset 默认使用 Ninja

本机已验证环境：macOS arm64、CMake 3.27.1、Apple Clang 17、Qt 5.15.2、Ninja 1.11.1、Git 2.44.0。GitHub 流水线还会使用 macOS arm64/Qt 6.8 和 Windows x64/MSVC 2022/Qt 6.8；以仓库 Actions 页的实际成功运行作为线上平台验证证据。

## 构建并运行

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
open build/debug/bin/GitCloneGui.app
```

Release：

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure
open build/release/bin/GitCloneGui.app
```

Qt 不在 CMake 默认搜索路径时：

```bash
cmake --preset debug -DCMAKE_PREFIX_PATH="$HOME/Qt/6.8.0/macos"
```

Qt 5 示例：

```bash
cmake --preset debug -DCMAKE_PREFIX_PATH="$HOME/Qt5.15.2"
```

普通 CMake 命令同样可用：

```bash
cmake -S . -B build/manual -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH=/你的/Qt/路径
cmake --build build/manual
ctest --test-dir build/manual --output-on-failure
```

Windows 本机可使用 Visual Studio 2022 x64：

```powershell
cmake -S . -B build/windows -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.8.3\msvc2022_64"
cmake --build build/windows --config Release --parallel
ctest --test-dir build/windows -C Release --output-on-failure
cmake --install build/windows --config Release --prefix build/install-windows
```

安装步骤会自动调用同一 Qt kit 的 `windeployqt`。可运行目录是 `build/install-windows/bin`，其中应包含 `GitCloneGui.exe`、Qt DLL、`platforms/qwindows.dll` 和 MSVC runtime；目标电脑仍需安装 Git for Windows 并确保 `git.exe` 在 `PATH` 中。

## GitHub Actions 自动构建与发版

工作流位于 `.github/workflows/release.yml`，执行以下流程：

1. macOS 使用 `macos-15` arm64 runner，Qt 6.8 LTS、CMake/Ninja 完成 Release 编译和全部 CTest；`macdeployqt` 生成自包含 `.app`，无 Developer ID 时对整个 Bundle 执行 ad-hoc 重签并严格验证，随后打成 DMG。
2. Windows 使用 `windows-2022` x64 runner，Qt 6.8 LTS、Visual Studio 2022 完成 Release 编译和全部 CTest；`windeployqt` 收集 Qt/plugin，脚本补齐 MSVC runtime 后生成便携 ZIP。
3. 普通 `main` push、PR、手动运行只上传 Actions artifact。
4. `v*` 标签在两个平台都成功后创建或更新同名 GitHub Release，并附加 DMG 与 ZIP；若存在 `docs/releases/<tag>.md`，该文件会作为 Release 正文，否则回退到 GitHub 自动生成说明。

首次启用时，将本分支合并到 `main` 或直接从 Actions 页面选择 “Build and Release” → “Run workflow”。正式发布版本示例：

```bash
git switch main
git pull --ff-only
git tag -a v0.1.4 -m "GitCloneGui v0.1.4"
git push origin v0.1.4
```

标签推送后到仓库的 Actions 页面观察两个平台 job；全部成功后，Release 页面会自动出现附件。不要在构建失败时手工上传裸 `.exe` 或 build-tree `.app`，它们没有完整运行时。

没有签名 Secrets 时，workflow 仍会生成测试产物。macOS `.app` 使用 ad-hoc identity 保证部署后的 executable、Frameworks、PlugIns 和 Resources 属于同一个有效 Bundle；这不等于 Developer ID 信任或 Apple 公证。Windows 包在没有 PFX 时同样不具备 Authenticode 信任链，因此 Gatekeeper 或 SmartScreen 仍可能要求额外确认，不要把这类包描述为已签名正式版。

首次打开无 Developer ID 的 macOS 测试包时，如果系统提示无法验证开发者，请先尝试打开一次，然后进入“系统设置 → 隐私与安全性”，在安全性区域点击“仍要打开”，再次确认“打开”。如果系统直接提示应用“已损坏”，说明 Bundle 签名结构没有通过验证，应停止使用并下载修复后的版本，而不是清除 quarantine 属性绕过检查。操作含义和安全风险见 [Apple 官方说明](https://support.apple.com/102445)。

### macOS 签名与公证

站外分发不能使用普通的 `Apple Development` 证书。需要加入 Apple Developer Program，并在 Apple Developer 的 Certificates 页面创建 `Developer ID Application` 证书。当前开发机探测到的 `Apple Development` 身份只适合开发，无法通过 Developer ID 公证。

准备流程：

1. 在钥匙串中确认证书和对应私钥同时存在，然后导出为带密码的 `.p12`。
2. 在 Apple ID 账户页面为 `notarytool` 创建 app-specific password；不要使用 Apple ID 登录密码。
3. 在 GitHub 仓库进入 Settings → Secrets and variables → Actions，新增以下 Repository secrets：

| Secret | 内容 |
|---|---|
| `MACOS_CERTIFICATE` | P12 的单行 Base64，例如 `base64 < DeveloperID.p12 \| tr -d '\n'` |
| `MACOS_CERTIFICATE_PASSWORD` | 导出 P12 时设置的密码 |
| `APPLE_ID` | Apple Developer 账户邮箱 |
| `APPLE_APP_PASSWORD` | Apple ID app-specific password |
| `APPLE_TEAM_ID` | Apple Developer Team ID |

流水线把 P12 写入 runner 临时目录并导入临时 keychain，`macdeployqt` 使用 `Developer ID Application`、Hardened Runtime 和 secure timestamp 签署 `.app`；之后签署最终 DMG，用 `notarytool --wait` 提交公证，成功后 stapling，并通过 `codesign`、`stapler` 和 `spctl` 验证。任一签名或公证步骤失败都会阻止 Release job。

Apple 公证是在线外部服务，证书申请、开发者年费、协议状态和公证服务异常都不由本项目控制。只配置证书但未配齐三个公证 Secret 时会得到“已签名但未公证”的包；面向普通用户发布时建议五个 Secret 全部配置。

### Windows Authenticode 签名

需要从受信任的代码签名 CA 或企业证书体系取得 Authenticode 证书。流水线当前支持“可导出的 PFX”模式：

| Secret | 内容 |
|---|---|
| `WINDOWS_CERTIFICATE` | PFX 文件的单行 Base64，例如 PowerShell：`[Convert]::ToBase64String([IO.File]::ReadAllBytes('codesign.pfx'))` |
| `WINDOWS_CERTIFICATE_PASSWORD` | PFX 密码 |

配置后，流水线使用 Windows SDK `signtool` 对 `GitCloneGui.exe` 执行 SHA-256 Authenticode 签名和 RFC3161 时间戳，再执行 `signtool verify /pa /v`。PFX 仅写入 runner 临时目录并在完成后删除。

部分公开 CA 根据当前行业规则只把私钥保存在 USB Token 或云 HSM，不能导出 PFX；这种证书不能直接使用上述两个 Secret，需要改接该 CA 的云签名服务（例如 Azure Trusted Signing 或证书供应商的远程签名工具）。自签名 PFX 虽能让技术步骤通过，但不会让 Windows/SmartScreen 把发布者识别为公共受信任开发者。

## 使用方法

### 仓库克隆

1. 填写父仓库 URL；稍候可从分支下拉框选择，也可以直接输入分支名。输入关键词会筛选包含该文本的远程分支。
2. 选择父项目目录的上一级保存位置。
3. 点击“添加子仓库”创建卡片，分别填写 URL、分支和父项目内相对路径。
4. 不需要子仓库时，可以删除全部卡片，只克隆父项目。
5. 右侧显示完整命令预览；配置有效后点击“开始克隆”。

### 仓库工作区

1. 点击左侧“仓库工作区”，选择一个包含多个项目的工作目录；选择后会自动扫描，也可点击“扫描仓库”重新扫描。应用会记住关闭前所在页面和工作目录，下次启动时恢复页面，并对仍然有效的目录自动扫描一次。
2. 左侧仓库树按工作目录相对路径显示所有 Git 工作树。扫描不会进入 `.git` 元数据目录，也不会跟随目录符号链接；发现父仓库后仍会继续检查普通子目录，因此嵌套仓库也会显示。
3. 选中仓库后，右侧显示当前分支和实时工作树状态。工作区干净时显示绿色说明；存在改动时显示橙色警示，并列出已暂存、未暂存、未跟踪和冲突中的非零数量。
4. 分支标签显示本地分支、远端待跟踪分支和全部远端跟踪引用。这里读取的是本机现有 remote-tracking refs，不会自动执行 `fetch`。
5. 在“本地分支”或“远端待跟踪”标签中选择目标并点击“切换到所选分支”，也可以双击。远端候选会按短分支名过滤本地已存在项，例如本地已有 `main` 时不再列出 `origin/main`。

切换失败时会原样显示 Git 的诊断信息。应用不会自动执行 stash、reset、clean、pull 或 fetch；请先自行处理未提交改动、分支冲突或过期远端引用，再点击“刷新”。

队列最终成功时会发送“GitCloneGui · 克隆完成”系统通知；父项目或任一子仓库失败时会发送“GitCloneGui · 克隆失败”通知并带上阶段错误。用户主动取消不会发送通知。若系统不支持桌面通知或通知权限被关闭，页内结果与 Git 输出仍会正常保留，克隆结果不会受到影响。

分支清单通过本机 Git 的 `ls-remote --symref` 异步读取，不会下载仓库对象。远程默认分支最先显示，其次是 main/develop/release 等常用工作分支，再按名称排序。通用 Git 远程引用不包含提交时间，因此这里的“优先”不是按最近提交日期排序。查询失败或私有仓库认证未配置时，分支框仍可手工输入。

示例：

```text
目标根目录：/Users/me/code
父目录名：platform

/Users/me/code/platform/
├── services/
│   ├── account/      # 子仓库 1
│   └── billing/      # 子仓库 2
└── plugins/
    └── analytics/    # 子仓库 3
```

## 配置保存与隐私

表单变化后约 300ms 自动保存；退出前也会刷新待保存内容。再次启动会恢复：

- 父仓库 URL、分支、目录名；
- 目标根目录；
- 子仓库卡片数量、顺序、URL、分支和相对路径。
- 仓库工作区的工作目录路径。

配置由 Qt `QSettings` 写入当前用户的系统设置位置。macOS 上位于当前用户的 `~/Library/Preferences/` 范围，具体文件名由 Qt 和应用标识决定；不会写入源码目录或 `.app` 内部。

工作目录恢复后，如果路径仍存在、是目录且可读，应用会在事件循环启动后自动后台扫描一次；无效路径只保留在输入框供修正，不会循环重试。仓库清单、当前分支和工作树改动状态均不持久化，每次扫描、选择仓库或刷新时实时读取。

扫描器不会为了提速忽略 `build`、`node_modules` 等普通目录，因为这些目录中仍可能存在独立 Git 仓库。macOS/Linux 使用目录项类型减少重复元数据查询，Windows 保留 Qt 兼容实现；两种路径都不进入 `.git`、不跟随目录符号链接，并继续发现仓库内部更深层的嵌套仓库。

不会保存 Git 输出、运行状态、密码、Token、SSH Key 或环境变量。但仓库 URL 本身会作为普通表单字段保存，因此不要把 Token 或密码嵌入 URL；请使用 SSH agent 或 Git credential helper。

## 路径与执行规则

- 父项目目录名必须是单个目录名，不能是 `.`、`..` 或包含路径分隔符；
- 每个子仓库路径必须位于父项目内部，不能包含 `..`；
- 多张子卡片不能指向相同目标路径；
- 父项目目标目录可以不存在，也可以已存在但必须为空；如果它是文件或含有任何内容（包括隐藏项），应用会明确提示“父项目目标目录必须为空”；
- 父项目成功后才执行第一个子仓库；任一子仓库失败后不再执行后续卡片；
- URL、分支和路径始终作为独立参数传给 Git，不经过 shell 解释。

## 生成自包含 macOS 应用

```bash
cmake --preset release
cmake --build --preset release
cmake --install build/release --prefix build/install
open build/install/GitCloneGui.app
```

`build/release/bin/GitCloneGui.app` 是日常开发构建物：包含应用图标，但仍引用开发机 Qt，因此体积较小。执行安装后，当前 Qt kit 的 `macdeployqt` 会把 Qt Framework 和 Cocoa platform plugin 部署到 `build/install/GitCloneGui.app`；这个安装产物是可脱离 Qt 开发目录运行的最终 Bundle，体积会从 KB/少量 MB 增长到数十 MB，这是内置 Qt 运行时的正常结果。

可检查部署内容：

```bash
find build/install/GitCloneGui.app/Contents -maxdepth 2 -type d
otool -L build/install/GitCloneGui.app/Contents/MacOS/GitCloneGui
```

GitHub Actions 会在安装树基础上生成 DMG。没有 Secrets 时 `.app` 是经过完整 ad-hoc Bundle 重签和严格验证、但不受 Gatekeeper 信任的测试包；配齐 Developer ID 和公证 Secrets 后，流水线会生成并验证已签名、公证、stapled 的 DMG。项目仍不制作 PKG、不提交 Mac App Store，也不实现自动更新。

## 项目结构

```text
src/core             父项目与有序子仓库模型、路径校验、命令计划
src/application      克隆队列状态机、进程和配置存储契约
src/infrastructure   QProcess Git 适配器、QSettings 配置适配器
src/presentation     卡片组件、双栏窗口、集中式视觉样式
src/app              应用组合根
tests                core/application/infrastructure/presentation/真实 Git 测试
scripts/release      macOS/Windows 打包、签名与交付结构校验
.github/workflows    GitHub Actions 双平台构建与标签 Release
.codex/specs         需求、设计、任务与实施证据
```
