# UIJsonBridge 使用说明

`UIJsonBridge` 是一个 UE Editor 插件，用来把 UMG Widget Blueprint 导出成 Codex 可读的 JSON，并把可导入的布局 JSON 再导回 Widget Blueprint。

它的核心目标不是替代 UE 编辑器，而是建立一条稳定的协作链路：

- UE 负责承载真实 Widget Blueprint 资产。
- JSON 负责暴露 WidgetTree、布局、样式、事件分析信息。
- Codex 负责阅读、修改和生成 layout JSON。
- 插件负责安全导入 WidgetTree，不自动重建 Blueprint Graph。

## 插件位置

插件目录：

```text
UE56_TTSHost/Plugins/UIJsonBridge
```

工程已启用插件：

```text
UE56_TTSHost/UE56_TTSHost.uproject
```

手动编译：

```bat
Build_UE56_TTSHost.bat
```

如果编译失败并提示 `UnrealEditor-UIJsonBridgeEditor.dll` 被占用，先关闭 UE 编辑器再重新编译。

## Codex AI 插件分发

本仓库同时提供一个 Codex 插件包，用于把“制作 UE 的 UI”的 AI 工作流随项目一起分发给团队成员。

插件目录：

```text
UE56_TTSHost/Plugins/UIJsonBridge/plugins/ui-json-bridge-ai
```

Marketplace 文件：

```text
UE56_TTSHost/Plugins/UIJsonBridge/.agents/plugins/marketplace.json
```

内置 Skill：

```text
ue-umg-json-ui-builder
```

同事安装该 Codex 插件后，可以直接用类似下面的说法触发 AI UI 制作流程：

```text
制作 UE 的 UI，根据这张参考图修改我的 UMG JSON，保持蓝图变量名不变。
```

这个 Codex 插件不替代 UE 插件 `UIJsonBridge`。UE 插件负责在 Unreal Editor 内导入/导出 UMG JSON；Codex 插件负责给 AI 提供稳定的 UI 制作规则、父子 UI 引用规则、CJK 字体处理、`SizeBox` 文本约束和导入前检查流程。

## 导出分级

右键 Widget Blueprint 后，插件提供三种导出入口。

### Export UI JSON - Layout Only

推荐给 Codex 改 UI 布局时使用。

文件名默认类似：

```text
WBP_Name_layout.json
```

内容包括：

- WidgetTree 层级
- 控件名称和类型
- Slot 信息
- Canvas Slot 布局
- 可编辑属性和样式
- 资源引用

不包括：

- bindings
- animations
- Blueprint graphs
- graph pins / links

`Layout Only` 是最稳定、最推荐回导入 UE 的格式。

### Export UI JSON - Interaction

推荐给 Codex 分析事件响应时使用。

文件名默认类似：

```text
WBP_Name_interaction.json
```

内容包括：

- WidgetTree 基本结构
- bindings
- animations
- 精简 Blueprint Graph 节点信息
- 常见事件 metadata，例如按钮 `OnClicked`

特点：

- `profile` 为 `interaction`
- `importable` 为 `false`
- 导入器会拒绝导入这种 JSON

原因：interaction 文件是分析用，不是回灌布局用。它通常缺少完整可还原属性，并且 Blueprint Graph 当前不会自动重建。

### Export UI JSON - Full

用于深度分析和备份参考。

文件名默认类似：

```text
WBP_Name_full.json
```

内容包括：

- Layout Only 的所有内容
- bindings
- animations
- Blueprint Graph nodes
- pins
- links
- node comments
- default values

注意：`Full` 可以导入其中的 WidgetTree，但目前仍不会重建 Blueprint Graph。日常改布局优先使用 `Layout Only`。

## 推荐工作流

### 只改布局

1. 在 UE 中右键 Widget Blueprint。
2. 选择 `Export UI JSON - Layout Only`。
3. 把 `_layout.json` 路径发给 Codex。
4. Codex 修改或生成新的 layout JSON。
5. 在 UE 中新建或复制一个 Widget Blueprint。
6. 右键目标 Widget Blueprint，选择 `Import UI JSON`。
7. 选择修改后的 layout JSON。
8. 打开 Widget Blueprint 检查画布和详情面板。

### 分析事件

1. 导出 `_layout.json`。
2. 再导出 `_interaction.json`。
3. 把两份都给 Codex。
4. 明确说明：`layout` 用来改界面，`interaction` 只用来分析事件响应。

推荐表达：

```text
请基于 layout JSON 调整 UI；interaction JSON 只用于理解按钮点击和蓝图调用关系，不要把 interaction JSON 当作可导入文件。
```

## 导入行为

导入会替换目标 Widget Blueprint 的整个 WidgetTree。

当前会导入：

- WidgetTree 层级
- 控件类
- 控件名称
- Display Label
- 可编辑控件属性
- Panel Slot 属性
- Canvas Slot 布局

当前不会导入：

- Blueprint Graph 节点
- 事件节点
- Pin 连线
- 函数图
- 动画时间轴

Graph 目前只导出、不导入。自动重建蓝图逻辑风险较高，容易破坏已有事件图。

## 导入安全策略

导入器已经做了几层防护：

- 导入前预检 JSON。
- 检查 Widget class 是否可解析。
- 检查控件名称是否重复。
- 拒绝 `interaction` 或 `importable=false` 的 JSON。
- 先在临时 WidgetTree 中完整构建。
- 临时构建成功后才替换目标 WidgetTree。
- 旧控件会先移到 transient，避免同名不同类导致 UE fatal。
- 导入后同步 `WidgetVariableNameToGuidMap`，避免 UMG 编译器 GUID ensure。
- 导入后自动编译 Widget Blueprint。

已覆盖的关键回归测试：

- 坏 JSON 导入失败时不破坏原 WidgetTree。
- 同名控件从 `HorizontalBox` 变为 `WrapBox` 不再触发 UE fatal。
- 真实 responsive safe JSON 可导入。
- LayoutOnly 导出后修改布局，再导入全新 Widget Blueprint。
- 从零生成的 layout JSON 可导入全新 Widget Blueprint。
- 中文 TextBlock 导入后文本正常，并带有效字体。

## 编码和中文显示

这部分是实际测试踩出来的重点，后续不要再忽略。

### JSON 文件编码

插件导出 JSON 使用：

```text
UTF-8 with BOM
```

导入器读取 JSON 时会强制按 UTF-8 解码，并兼容有 BOM / 无 BOM。

原因：Windows / UE 环境中，无 BOM UTF-8 有时会被误判成 ANSI，导致中文属性导入后变成乱码。

### 详情面板正常但画布乱码

如果出现：

- Details 面板里的中文 Text 是正常的；
- 但 UMG 画布预览里的中文显示乱码、方块或异常字符；

这通常不是 JSON 编码问题，而是字体问题。

UE 的默认 UMG 字体常用 Roboto，不一定包含中文字形。编辑器详情面板使用的是编辑器 UI 字体，所以详情面板能显示中文，不代表 UMG 画布字体也能显示中文。

### CJK 字体兜底

导入器现在包含 CJK 字体兜底逻辑：

- 如果控件属性中包含中文、日文或韩文字符；
- 且文本控件没有有效字体；
- 导入器会自动设置为 UE 自带字体：

```text
Engine/Content/Slate/Fonts/DroidSansFallback.ttf
```

当前兜底覆盖：

- `TextBlock`
- `EditableText`
- `MultiLineEditableText`

注意：`MultiLineEditableTextBox` 在 UE 5.6 中没有公开的简单字体 setter。若输入框 hint 仍有字体问题，后续需要通过完整 `WidgetStyle` 属性或项目字体资产解决。

## 从零导入测试文件

当前保留一个专门测试“从空白 Widget Blueprint 导入”的 layout 文件：

```text
UE56_TTSHost/UI_export/WBP_Codex_FromScratch_layout.json
```

用途：

- 验证插件基础导入链路。
- 验证 UTF-8 中文。
- 验证 TextBlock 字体兜底。
- 验证从无到有创建 UI 的流程。

手动测试步骤：

1. 关闭 UE。
2. 运行 `Build_UE56_TTSHost.bat`。
3. 打开 UE。
4. 新建一个空白 Widget Blueprint。
5. 右键这个 Widget Blueprint。
6. 选择 `Import UI JSON`。
7. 选择 `WBP_Codex_FromScratch_layout.json`。
8. 检查画布上中文是否正常显示。

## 自动化测试

可用命令：

```bat
"C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\project\UETTsProject\UE56_TTSHost\UE56_TTSHost.uproject" -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation RunTests UIJsonBridge; Quit" -TestExit="Automation Test Queue Empty" -log
```

重点测试项：

```text
UIJsonBridge.Exporter.ProfileFields
UIJsonBridge.Layout.RoundTripModifiedLayoutIntoFreshBlueprint
UIJsonBridge.Importer.ImportsFromScratchLayoutJson
UIJsonBridge.Importer.ImportsProjectResponsiveSafeJson
UIJsonBridge.Importer.ReplacesSameNamedDifferentClassWidgets
UIJsonBridge.Importer.FailedImportPreservesExistingTree
```

## 常见问题

### 导入时 UE 崩溃：Cannot replace existing object of a different class

历史原因：JSON 中同名控件类型发生变化，例如旧控件是 `HorizontalBox_182: HorizontalBox`，新 JSON 中变成 `HorizontalBox_182: WrapBox`，旧 WidgetTree 中同名对象还存在，UE 会 fatal。

当前处理：导入器会先把旧 WidgetTree 控件移出，再构建新树，已通过自动化测试覆盖。

### 导入后事件不工作

优先检查：

- 控件名称是否被改掉。
- 原 Graph 是否引用了旧控件变量。
- interaction JSON 中的 `componentName` 是否还能在新 WidgetTree 中找到。

建议：如果要保留事件图，复制原 Widget Blueprint 后导入到副本，并尽量保持控件名称稳定。

### 不小心导入 interaction JSON

导入器会拒绝，并提示应使用 `Layout Only` 或 `Full` JSON。

### JSON 很大

正常。`Full` 会包含 Graph pins / links / 默认值，体积会明显增大。

只改 UI 时请优先导出：

```text
Export UI JSON - Layout Only
```

### 中文仍然乱码

先区分两种情况：

- JSON 文件里已经是 `???`：生成文件时编码坏了，需要重新生成 UTF-8 文件。
- Details 面板中文正常但画布乱码：通常是字体问题，需要 CJK 字体兜底。

当前插件已经处理 TextBlock 等常见文本控件。如果某类控件仍有问题，需要为该控件补专门字体处理。

## 后续计划

建议后续继续做：

- 导入前 diff / preview。
- 自动备份目标 Widget Blueprint。
- 只导入指定子树。
- 检查 Graph 引用的控件名是否在新 WidgetTree 中存在。
- 完善 `MultiLineEditableTextBox` 字体样式导入。
- 为 LayoutOnly 生成更严格的 schema 校验。
