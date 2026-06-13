# UIJsonBridge AI Codex 插件使用说明

`ui-json-bridge-ai` 是随项目分发的 Codex 插件，用于给 Codex 提供“制作 UE 的 UI”的稳定工作流。

它不替代 UE 插件 `UIJsonBridge`：

- `UIJsonBridge` 负责在 Unreal Editor 中导入/导出 UMG JSON。
- `ui-json-bridge-ai` 负责让 AI 按固定规则制作、修改和检查 UE UMG JSON UI。

## 插件内容

```text
plugins/ui-json-bridge-ai
├─ .codex-plugin/plugin.json
└─ skills/
   └─ ue-umg-json-ui-builder/
```

内置 Skill：

```text
ue-umg-json-ui-builder
```

触发语示例：

```text
制作 UE 的 UI，根据参考图修改我的 UMG JSON，保持蓝图变量名不变。
```

也可以显式调用：

```text
使用 $ue-umg-json-ui-builder 帮我制作 UE 的 UI。
```

## 安装方式

仓库内提供 marketplace 文件：

```text
.agents/plugins/marketplace.json
```

团队成员拿到仓库后，可以在 Codex 插件界面中从该 marketplace 安装 `ui-json-bridge-ai`。

如果使用命令行安装非默认 marketplace，请指向仓库中的 `.agents/plugins/marketplace.json` 所在位置。安装后重启或刷新 Codex 插件列表。

## 与 UE 插件配合

推荐流程：

1. 在 UE 中启用并编译 `UE56_TTSHost/Plugins/UIJsonBridge`。
2. 右键目标 Widget Blueprint。
3. 导出 `Layout Only` JSON。
4. 如果需要理解事件响应，再导出 `Interaction` JSON。
5. 把 JSON、参考图或 HTML 交给 Codex。
6. 用“制作 UE 的 UI”触发本插件内置 skill。
7. Codex 修改可导入的 layout JSON。
8. 回到 UE 中导入修改后的 layout JSON。

## Skill 固化的规则

AI 制作 UI 时会遵守这些约束：

- 保持 Widget Blueprint 中已有变量名、控件类和 `isVariable` 不变。
- 原始导出中是变量的控件，必须同时保留 `isVariable=true` 和 `bIsVariable=True`。
- 不擅自修正已经被蓝图引用的拼写错误变量名，例如 `Text_Erro`。
- 优先做区域层级和尺寸约束，不只靠调字号解决布局。
- 对长文本、状态值、日志、路径、队列行等高风险文本使用 `SizeBox` 约束区域。
- 使用支持 CJK 的字体资产，避免中文在 UMG 画布中显示成方框或乱码。
- 父 UI 管布局，子 UI 管局部功能。
- 子 UI 默认保持独立引用，不拍平成父 UI，除非明确要求。
- 动态效果只加在明确指定的区域。
- 导入前检查 JSON 解析、重复名称、变量保留、字体引用和动态效果范围。

## 已知踩坑：变量存在但不是 Blueprint 变量

如果导入后蓝图编译提示某些控件变量不存在，而 JSON 中实际能搜到同名控件，重点检查：

```json
"isVariable": true,
"properties": {
  "bIsVariable": "True"
}
```

控件存在但 `isVariable=false` 或 `bIsVariable=False` 时，UE 不会把它作为 Widget Blueprint 变量暴露给蓝图图表，事件图里访问该控件就会编译报错。

AI 修改 UI JSON 时必须以最新可工作的导出 JSON 为变量基准，保证所有原本蓝图访问的控件仍然同时保留：

- 相同 `name`
- 相同 `class`
- `isVariable=true`
- `bIsVariable=True`

UE 插件导入器也会对“同名同类控件”继承旧 WidgetTree 的变量标记做兜底，但 JSON 本身仍应保持正确。

## 父 UI / 子 UI 输入建议

如果界面引用子 UI，最好同时提供：

- 父 UI JSON。
- 所有可能修改的子 UI JSON。
- 一张完整参考图或 HTML，说明子 UI 在父 UI 中的位置、尺寸和层级。
- 如果子 UI 内部也要修改，提供子 UI 自己的参考图或说明。
- 明确哪些子 UI 只引用不展开，哪些子 UI 可以修改内部结构。

推荐说法：

```text
制作 UE 的 UI。父 UI 是 WBP_MainPanel.json，子 UI 有 WBP_QueueRow.json、WBP_StatusCard.json。参考图是 Mockup.png。父 UI 里引用这些子 UI，不要展开 WBP_QueueRow 内部；WBP_StatusCard 可以按参考图修改。保持所有变量名不变。
```

## 相关文件

项目级提示词文档：

```text
Doc/20_UE_UMG_JSON_UI制作提示词.md
```

UE 插件说明：

```text
UE56_TTSHost/Plugins/UIJsonBridge/使用说明.md
```
