# GitCloneGui：多仓库克隆工具

一个紧凑的 Qt Widgets 桌面工具：先克隆父项目，再按卡片顺序克隆任意数量的子仓库。每个仓库可以使用不同 URL、分支和父项目内路径。

程序实际调用本机 `git`，并通过结构化参数启动进程，不会把表单内容拼接成 shell 命令。任一步骤失败或取消后立即停止后续队列，也不会自动删除已下载文件。

## 主要功能

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

## 环境要求

- CMake 3.21+
- 支持 C++17 的编译器
- Qt 6，或 Qt 5.15（Core、Widgets、Test）
- Git CLI
- 推荐 Ninja；仓库 Preset 默认使用 Ninja

已验证环境：macOS arm64、CMake 3.27.1、Apple Clang 17、Qt 5.15.2、Ninja 1.11.1、Git 2.44.0。

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

## 使用方法

1. 填写父仓库 URL；稍候可从分支下拉框选择，也可以直接输入分支名。输入关键词会筛选包含该文本的远程分支。
2. 选择父项目目录的上一级保存位置。
3. 点击“添加子仓库”创建卡片，分别填写 URL、分支和父项目内相对路径。
4. 不需要子仓库时，可以删除全部卡片，只克隆父项目。
5. 右侧显示完整命令预览；配置有效后点击“开始克隆”。

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

配置由 Qt `QSettings` 写入当前用户的系统设置位置。macOS 上位于当前用户的 `~/Library/Preferences/` 范围，具体文件名由 Qt 和应用标识决定；不会写入源码目录或 `.app` 内部。

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

项目暂未制作 DMG/PKG，也未执行 Developer ID 签名、公证或自动更新。因此复制到其他 Mac 后仍可能遇到 Gatekeeper 提示；这与 Qt 依赖是否自包含是两件不同的事。

## 项目结构

```text
src/core             父项目与有序子仓库模型、路径校验、命令计划
src/application      克隆队列状态机、进程和配置存储契约
src/infrastructure   QProcess Git 适配器、QSettings 配置适配器
src/presentation     卡片组件、双栏窗口、集中式视觉样式
src/app              应用组合根
tests                core/application/infrastructure/presentation/真实 Git 测试
.codex/specs         需求、设计、任务与实施证据
```
