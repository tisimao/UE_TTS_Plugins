# 轻量 Demo UI 与关卡设计

## 1. 目标

本文用于指导当前阶段在 UE 中搭建一个轻量可用的 `LocalTTS` 演示界面和测试关卡。

这个 UI 的定位是：

- 验证插件正式蓝图入口是否好用。
- 展示 `LocalTTS` 的核心价值：文本输入、语音生成、播放、长文本分段、暂停、继续、跳段、停止。
- 辅助测试长文本队列和状态回调。
- 给后续正式展示关卡提供蓝图接线参考。

这个 UI 不是最终产品界面，不需要复杂美术，也不要把业务逻辑写死在 Widget 里。Widget 只负责调用插件公开蓝图节点和显示状态。

## 2. 建议创建的资源

建议放在宿主工程内容目录：

```text
UE56_TTSHost/Content/LocalTTS/Demo
├─ Maps
│  └─ L_LocalTTS_Demo
├─ UI
│  └─ WBP_LocalTTS_DemoPanel
└─ Blueprints
   └─ BP_LocalTTS_DemoController
```

资源说明：

| 资源 | 类型 | 作用 |
| --- | --- | --- |
| `L_LocalTTS_Demo` | Level | 专门用于展示和测试插件能力的关卡。 |
| `WBP_LocalTTS_DemoPanel` | Widget Blueprint | 主 UI 面板，包含服务、单句、长文本和调试区域。 |
| `BP_LocalTTS_DemoController` | Actor Blueprint | 放在关卡里，负责创建并显示 Widget。 |

为什么需要 `BP_LocalTTS_DemoController`：

- 关卡里有一个明确入口，打开关卡后自动显示 UI。
- Widget 不需要自己处理关卡初始化。
- 后续如果要把 UI 换成 3D Widget 或接角色 Actor，可以从这个 Controller 扩展。

当前代码侧已经提供 `ALocalTTSDemoController`，建议 `BP_LocalTTS_DemoController` 直接继承它，而不是继承普通 `Actor`。

这样做的好处：

- C++ 层已经保存 `LongTextQueue`、最近 WAV、最近请求 ID、错误提示和事件日志，Widget 不用自己维护一套复杂变量。
- C++ 层已经绑定单句异步节点和长文本队列回调，Widget 只需要点击按钮调用函数，再监听 `Demo 状态已更新` 刷新文本。
- Demo 逻辑集中在示例层，插件正式蓝图节点仍然保持独立可用，不会和测试 UI 强绑定。

## 3. 关卡搭建

### 3.1 `L_LocalTTS_Demo`

关卡里建议放：

- 一个 `BP_LocalTTS_DemoController`
- 一个简单角色或占位 Actor，命名为 `TTS_Speaker`
- 一个普通摄像机和灯光

当前阶段不用做 MetaHuman。`TTS_Speaker` 只用来表达“声音来自这里”这个概念，后续可用于测试 `生成并在 Actor 位置播放 Local TTS`。

### 3.2 `BP_LocalTTS_DemoController`

推荐父类：

| 蓝图 | 父类 | 说明 |
| --- | --- | --- |
| `BP_LocalTTS_DemoController` | `ALocalTTSDemoController` | 继承后可直接调用 Demo 封装函数，并读取服务、单句、长文本、错误和日志状态。 |

变量：

| 变量名 | 类型 | 用途 |
| --- | --- | --- |
| `DemoPanelClass` | `Class Reference: WBP_LocalTTS_DemoPanel` | 指定要创建的 UI 类。 |
| `DemoPanel` | `WBP_LocalTTS_DemoPanel Object Reference` | 保存创建出来的 UI 引用。 |

BeginPlay 接线：

```text
Event BeginPlay
-> Create Widget
   Class = DemoPanelClass
   Owning Player = Get Player Controller
-> Set DemoPanel
-> 如果 Widget 有 DemoController 变量：
   Set DemoController = self
-> Add to Viewport
-> Set Input Mode Game and UI
-> Show Mouse Cursor = true
```

为什么这么连：

- `Create Widget` 和 `Add to Viewport` 是最轻量的 Demo 展示方式。
- 保存 `DemoPanel` 可以避免 Widget 被意外回收，也方便后续从关卡 Actor 传入测试目标。
- 把 `self` 传给 Widget 后，Widget 按钮可以直接调用 `Demo 单句生成并播放`、`Demo 长文本生成并播放` 等函数。
- `Set Input Mode Game and UI` 让按钮可点，同时不完全锁死游戏输入。

## 4. UI 布局

`WBP_LocalTTS_DemoPanel` 建议分四块：

### 4.1 服务区

控件：

| 控件名 | 类型 | 显示文字 |
| --- | --- | --- |
| `Btn_StartService` | Button | 启动服务 |
| `Btn_CheckHealth` | Button | 检查健康 |
| `Txt_ServiceState` | TextBlock | 服务状态 |
| `Txt_ServiceDetail` | TextBlock | 服务详情 |

### 4.2 单句语音区

控件：

| 控件名 | 类型 | 显示文字 |
| --- | --- | --- |
| `Input_SingleText` | MultiLineEditableTextBox | 单句或短文本输入 |
| `Btn_SpeakSingle` | Button | 生成并播放 |
| `Btn_GenerateSingle` | Button | 仅生成 WAV |
| `Btn_StopSingle` | Button | 停止播放 |
| `Txt_SingleState` | TextBlock | 单句状态 |
| `Txt_SingleResult` | TextBlock | 单句结果 |

### 4.3 长文本队列区

控件：

| 控件名 | 类型 | 显示文字 |
| --- | --- | --- |
| `Input_LongText` | MultiLineEditableTextBox | 长文本输入 |
| `Btn_StartLongSpeak` | Button | 长文本播放 |
| `Btn_StartLongGenerate` | Button | 长文本仅生成 |
| `Btn_PauseLong` | Button | 暂停 |
| `Btn_ResumeLong` | Button | 继续 |
| `Btn_SkipLong` | Button | 下一段 |
| `Btn_StopLong` | Button | 停止 |
| `Txt_LongState` | TextBlock | 队列状态 |
| `Txt_CurrentSegment` | TextBlock | 当前段文本 |
| `Txt_LongProgress` | TextBlock | 当前进度 |

### 4.4 调试区

控件：

| 控件名 | 类型 | 显示文字 |
| --- | --- | --- |
| `Txt_LastWavPath` | TextBlock | 最近 WAV 路径 |
| `Txt_LastRequestId` | TextBlock | 最近请求 ID |
| `Txt_Error` | TextBlock | 错误提示 |
| `Txt_EventLog` | MultiLineEditableTextBox 或 TextBlock | 最近事件日志 |

## 5. Widget 变量

`WBP_LocalTTS_DemoPanel` 建议新增变量：

| 变量名 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `DemoController` | `Local TTS Demo Controller Object Reference` | 空 | 推荐由 `BP_LocalTTS_DemoController` 创建 Widget 后传入。Widget 优先调用它，少接底层回调线。 |
| `LongTextQueue` | `Local TTS Long Text Queue Object Reference` | 空 | 必须保存。否则队列对象可能被 GC。 |
| `CurrentSpeakState` | `ELocalTTSSpeakAsyncState` | 空 | 单句异步请求状态。 |
| `CurrentQueueState` | `ELocalTTSLongTextQueueState` | `Idle` | 长文本队列状态。 |
| `CurrentSegmentIndex` | `Integer` | `-1` | 当前段序号。 |
| `TotalSegmentCount` | `Integer` | `0` | 总段数。 |
| `LastErrorText` | `String` | 空 | 最近错误。 |
| `LastWavPath` | `String` | 空 | 最近 wav。 |
| `LastRequestId` | `String` | 空 | 最近请求 ID。 |

为什么必须保存 `LongTextQueue`：

- `创建 Local TTS 长文本队列` 返回的是一个 UObject。
- 如果只临时接线而不保存，蓝图里后续按钮可能找不到同一个队列对象。
- 保存变量后，暂停、继续、跳段、停止才会作用在同一个队列实例上。

如果使用 `ALocalTTSDemoController`：

- `LongTextQueue` 可以不放在 Widget 里，直接读取 `DemoController.LongTextQueue`。
- `CurrentSpeakState`、`CurrentQueueState`、`LastErrorText`、`LastWavPath`、`LastRequestId` 都可以直接绑定到 `DemoController` 对应字段。
- Widget 的 `Event Construct` 只需要绑定 `Demo 状态已更新`，然后调用一个自定义 `RefreshFromDemoController` 函数刷新所有 TextBlock。

## 6. Widget 初始化接线

在 `Event Construct` 中：

```text
Event Construct
-> 创建 Local TTS 长文本队列
-> Set LongTextQueue
-> Bind Event to 队列状态变化
-> Bind Event to 段落开始
-> Bind Event to 段落已生成
-> Bind Event to 段落完成
-> Bind Event to 队列完成
-> Bind Event to 队列停止
-> Bind Event to 队列错误
-> 获取 Local TTS 服务状态文本
-> Set Txt_ServiceState
```

为什么在 Construct 创建队列：

- Demo UI 打开后，长文本按钮可以立即使用。
- 所有队列事件只绑定一次，不会每次点击按钮都重复绑定。
- 长文本队列对象生命周期跟随 Widget，测试时更直观。

注意：

- 如果 Widget 会被反复创建销毁，队列也会随 Widget 重建。
- 如果后续要跨关卡保留队列，应把队列放到 GameInstance 或 Subsystem 层管理。

## 7. 服务区蓝图接线

### 7.1 启动服务按钮

`Btn_StartService.OnClicked`：

```text
OnClicked
-> 启动 Local TTS
   WorldContextObject = self
   ErrorMessage = Local String
-> Branch(ReturnValue)
   true:
     -> 获取 Local TTS 服务状态文本
     -> Set Txt_ServiceState
     -> Set Txt_ServiceDetail = "服务启动请求已发送，等待健康检查。"
   false:
     -> Set Txt_Error = ErrorMessage
```

为什么这么连：

- `启动 Local TTS` 只负责拉起服务进程，不代表模型已经 ready。
- 启动后仍需要用健康检查确认服务状态。
- 错误直接显示，方便测试安装路径、端口、Python 环境问题。

### 7.2 检查健康按钮

`Btn_CheckHealth.OnClicked`：

```text
OnClicked
-> 异步检查 Local TTS 健康
   WorldContextObject = self
-> 检查成功
   -> Break FLocalTTSHealthResponse
   -> Set Txt_ServiceState = Status
   -> Set Txt_ServiceDetail = Model + SupportedModes
   -> Set Txt_Error = ""
-> 检查失败
   -> Set Txt_Error = ErrorMessage
   -> 获取 Local TTS 服务状态文本
   -> Set Txt_ServiceState
```

为什么使用异步健康检查：

- `/health` 是 HTTP 请求，不应该阻塞 UI。
- 成功不一定等于可播放，要看返回的 `Status` 是否为 ready。
- 失败可以区分服务未启动、端口不通、模型未加载等情况。

## 8. 单句语音区蓝图接线

### 8.1 生成并播放

`Btn_SpeakSingle.OnClicked`：

```text
OnClicked
-> Local TTS 是否忙碌
-> Branch
   true:
     -> Set Txt_Error = "LocalTTS 正在处理上一条语音，请稍候。"
   false:
     -> 创建 Local TTS 自动模式请求
        Text = Input_SingleText.GetText -> ToString
        LanguageId = "zh"
        Speed = 1.0
     -> 生成并播放 Local TTS
        WorldContextObject = self
        SpeakRequest = 上一步返回值
```

绑定 `生成并播放 Local TTS` 的回调：

```text
状态变化
-> Set CurrentSpeakState
-> 获取语音异步状态文本
-> Set Txt_SingleState
-> 语音异步状态是否应禁用按钮
-> Set Btn_SpeakSingle IsEnabled = NOT ReturnValue
-> Set Btn_GenerateSingle IsEnabled = NOT ReturnValue

音频已就绪
-> Break FLocalTTSTTSResponse
-> Set LastWavPath = WavPath
-> Set LastRequestId = RequestId
-> Set Txt_LastWavPath
-> Set Txt_LastRequestId

播放结束
-> Set Txt_SingleState = "播放完成"
-> Set Btn_SpeakSingle IsEnabled = true
-> Set Btn_GenerateSingle IsEnabled = true

发生错误
-> Set Txt_Error = ErrorMessage
-> Set Txt_SingleState = "语音流程失败"
-> Set Btn_SpeakSingle IsEnabled = true
-> Set Btn_GenerateSingle IsEnabled = true
```

为什么优先监听 `状态变化`：

- 一个回调就能驱动 UI 的“思考中 / 等待服务 / 生成中 / 播放中 / 完成 / 错误”。
- 不需要在 Widget 里维护很多布尔变量。
- 按钮是否禁用可以用插件提供的 `语音异步状态是否应禁用按钮` 统一判断。

为什么不只看 `生成成功`：

- `生成成功` 表示服务端已经返回 wav，不等于音频已经开始播放。
- UI 要显示“播放中”，更适合看 `状态变化` 或 `音频已就绪`。

### 8.2 仅生成 WAV

`Btn_GenerateSingle.OnClicked`：

```text
OnClicked
-> Local TTS 是否忙碌
-> Branch
   true:
     -> Set Txt_Error = "LocalTTS 正在处理上一条语音，请稍候。"
   false:
     -> 创建 Local TTS 自动模式请求
     -> 仅生成 Local TTS
```

回调接法：

```text
状态变化
-> Set Txt_SingleState

语音事件已就绪
-> Break FLocalTTSSpeechEvent
-> Set Txt_SingleResult = RequestId + Text + WavPath + DurationSeconds
-> Set Txt_LastWavPath = WavPath
-> Set Txt_LastRequestId = RequestId

音频已就绪
-> Set Txt_SingleState = "WAV 已生成，可交给数字人或自定义播放。"

发生错误
-> Set Txt_Error
```

为什么仅生成也监听 `语音事件已就绪`：

- 数字人和字幕更需要 `FLocalTTSSpeechEvent`，不只是 `FLocalTTSTTSResponse`。
- 以后接口型和表情时，仍然从这个事件继续扩展。

### 8.3 停止播放

`Btn_StopSingle.OnClicked`：

```text
OnClicked
-> 停止 Local TTS 播放
-> Set Txt_SingleState = "已请求停止播放"
```

为什么停止只影响播放：

- 单次 TTS 的服务端生成请求目前不能立即取消。
- 如果音频已经在 UE 播放，停止会释放当前 AudioComponent。

## 9. 长文本队列区蓝图接线

### 9.1 开始长文本播放

`Btn_StartLongSpeak.OnClicked`：

```text
OnClicked
-> Branch(LongTextQueue Is Valid)
   false:
     -> 创建 Local TTS 长文本队列
     -> Set LongTextQueue
     -> 绑定队列事件
   true:
     -> continue
-> 创建 Local TTS 自动模式请求
   Text = ""
   LanguageId = "zh"
   Speed = 1.0
-> Make FLocalTTSLongTextRequest
   原始长文本 = Input_LongText.GetText -> ToString
   模板请求 = 上一步请求
   每段最大字符数 = 80
   每段最小合并字符数 = 12
   是否按换行拆分 = true
   是否自动套用推荐时长 = false
-> LongTextQueue.开始长文本生成并播放
   WorldContextObject = self
   LongTextRequest = 上一步结构
   ErrorMessage = Local String
-> Branch(ReturnValue)
   true:
     -> Set Txt_Error = ""
     -> Set Txt_LongState = "长文本队列已启动"
   false:
     -> Set Txt_Error = ErrorMessage
```

为什么模板请求里的 `Text` 可以为空：

- 长文本队列会把每一段 `FLocalTTSTextSegment.Text` 写入模板请求的 `Text` 字段。
- 模板请求主要提供 Mode、LanguageId、Speed、Duration、Instruct 等通用参数。

为什么 `每段最大字符数` 建议先用 80：

- 太长会导致单段推理慢、字幕和暂停控制不细。
- 太短会生成很多 wav，听感碎。
- 80 适合当前阶段先做人工验证，后续再按实际声音效果调整。

### 9.2 开始长文本仅生成

`Btn_StartLongGenerate.OnClicked` 和播放版基本相同，只是最后调用：

```text
LongTextQueue.开始长文本仅生成
```

为什么需要仅生成：

- 用来验证长文本分段和每段语音事件输出。
- 不自动播放，适合数字人、字幕、批量生成、后处理测试。

### 9.3 暂停

`Btn_PauseLong.OnClicked`：

```text
OnClicked
-> Branch(LongTextQueue Is Valid)
   false:
     -> Set Txt_Error = "长文本队列不存在。"
   true:
     -> LongTextQueue.暂停长文本队列
        ErrorMessage = Local String
     -> Branch(ReturnValue)
        true:
          -> Set Txt_LongState = "已暂停"
        false:
          -> Set Txt_Error = ErrorMessage
```

为什么暂停要走队列节点：

- 播放中暂停会暂停当前音频。
- 生成中暂停不能杀掉服务端推理，会在当前段完成后停在段落边界。
- 队列知道当前处于哪一段，单纯调用 `暂停 Local TTS 播放` 不知道长文本上下文。

### 9.4 继续

`Btn_ResumeLong.OnClicked`：

```text
OnClicked
-> LongTextQueue.继续长文本队列
-> Branch(ReturnValue)
   true:
     -> Set Txt_LongState = "继续处理"
   false:
     -> Set Txt_Error = ErrorMessage
```

为什么继续也走队列节点：

- 如果是播放中暂停，继续当前音频。
- 如果是生成后暂停，继续播放当前已生成段。
- 如果是段落边界暂停，继续进入下一段。

### 9.5 跳到下一段

`Btn_SkipLong.OnClicked`：

```text
OnClicked
-> LongTextQueue.跳到下一段
-> Branch(ReturnValue)
   true:
     -> Set Txt_LongState = "已请求跳到下一段"
   false:
     -> Set Txt_Error = ErrorMessage
```

为什么跳段是队列能力：

- 队列会清理当前段状态并推进段序号。
- 播放中跳段会停止当前播放。
- 生成中跳段属于软跳过，当前请求返回后丢弃该段结果。

### 9.6 停止

`Btn_StopLong.OnClicked`：

```text
OnClicked
-> LongTextQueue.停止长文本队列
-> Set Txt_LongState = "已停止"
```

为什么停止后不立刻销毁队列：

- Demo 里保留队列对象，方便查看最后状态和已完成段事件。
- 再次开始时可以复用同一个队列对象。

## 10. 长文本队列事件接线

### 10.1 队列状态变化

事件参数：

- `State`
- `DetailMessage`

接线：

```text
队列状态变化
-> Set CurrentQueueState = State
-> 获取长文本队列状态文本
-> Set Txt_LongState = DetailMessage + " / " + 状态文本
-> 长文本队列状态是否可开始
-> Set Btn_StartLongSpeak IsEnabled
-> Set Btn_StartLongGenerate IsEnabled
-> 长文本队列状态是否可暂停
-> Set Btn_PauseLong IsEnabled
-> 长文本队列状态是否可继续
-> Set Btn_ResumeLong IsEnabled
-> 长文本队列状态是否可跳到下一段
-> Set Btn_SkipLong IsEnabled
-> 长文本队列状态是否可停止
-> Set Btn_StopLong IsEnabled
```

这些 helper 节点内部对应下面的按钮状态：

| State | 开始 | 暂停 | 继续 | 下一段 | 停止 |
| --- | --- | --- | --- | --- | --- |
| Idle | 可用 | 禁用 | 禁用 | 禁用 | 禁用 |
| Segmenting | 禁用 | 可用 | 禁用 | 禁用 | 可用 |
| Generating | 禁用 | 可用 | 禁用 | 可用 | 可用 |
| Playing | 禁用 | 可用 | 禁用 | 可用 | 可用 |
| Paused | 禁用 | 禁用 | 可用 | 可用 | 可用 |
| Stopped | 可用 | 禁用 | 禁用 | 禁用 | 禁用 |
| Finished | 可用 | 禁用 | 禁用 | 禁用 | 禁用 |
| Error | 可用 | 禁用 | 禁用 | 禁用 | 禁用 |

为什么由状态统一控制按钮：

- 避免 UI 出现“暂停和继续同时可点”的混乱状态。
- 方便后续换成正式 UI 时复用同一套状态表。
- 现在插件已经提供状态 helper 节点，Widget 不需要手写枚举分支，后续规则调整也只需要改插件侧。

### 10.2 段落开始

事件参数：

- `Segment`

接线：

```text
段落开始
-> Break FLocalTTSTextSegment
-> Set CurrentSegmentIndex = SegmentIndex
-> Set Txt_CurrentSegment = Text
-> 获取已拆分文本段
-> Length
-> Set TotalSegmentCount
-> Set Txt_LongProgress = 当前段 / 总段数
```

为什么在这里更新当前段文本：

- 段落开始代表队列已经决定处理哪一段。
- UI 可以提前显示字幕文本，即使音频还在生成。

### 10.3 段落已生成

事件参数：

- `SegmentSpeechEvent`

接线：

```text
段落已生成
-> Break FLocalTTSSegmentSpeechEvent
-> Break SpeechEvent
-> Set LastRequestId = RequestId
-> Set LastWavPath = WavPath
-> Set Txt_LastRequestId
-> Set Txt_LastWavPath
-> Append Txt_EventLog = "段落已生成：" + RequestId
```

为什么在这里更新 request_id 和 wav：

- 这是服务端已返回该段音频结果的时刻。
- 字幕、数字人、调试日志都应该优先从这个事件拿数据。

### 10.4 段落完成

事件参数：

- `SegmentSpeechEvent`

接线：

```text
段落完成
-> Break FLocalTTSSegmentSpeechEvent
-> Break Segment
-> Set Txt_LongProgress = "第 X 段完成"
-> Append Txt_EventLog
```

为什么 `段落已生成` 和 `段落完成` 要分开：

- 自动播放模式下，已生成只代表 wav 准备好了，完成代表这段播放结束。
- 仅生成模式下，生成完成和段落完成会更接近，但仍建议保留两个 UI 信号。

### 10.5 队列完成

接线：

```text
队列完成
-> Set Txt_LongState = "长文本全部完成"
-> Set Btn_StartLongSpeak IsEnabled = true
-> Set Btn_StartLongGenerate IsEnabled = true
```

### 10.6 队列停止

接线：

```text
队列停止
-> Set Txt_LongState = "长文本队列已停止"
-> Set Btn_StartLongSpeak IsEnabled = true
-> Set Btn_StartLongGenerate IsEnabled = true
```

### 10.7 队列错误

事件参数：

- `SegmentIndex`
- `SegmentText`
- `ErrorMessage`

接线：

```text
队列错误
-> Set Txt_Error = "第 " + SegmentIndex + " 段失败：" + ErrorMessage
-> Set Txt_CurrentSegment = SegmentText
-> Set Txt_LongState = "队列错误"
```

为什么错误里要显示段文本：

- 长文本出错时，只显示“失败”无法定位是哪一句。
- 显示段序号和段文本可以直接回到输入内容里修改。

## 11. 推荐 Demo 测试文本

```text
第一段用于测试普通中文句号分段。第二段会继续生成新的语音，并观察 request_id 是否变化。
这里故意换一行，用来测试换行是否会被优先作为分段边界。
第三段内容稍微长一点，用来观察暂停、继续、跳到下一段和停止是否稳定。我们希望 UI 能显示当前段文本、当前段序号、生成的 WAV 路径，以及队列最终完成状态。
```

## 12. 当前限制

- 长文本队列目前是 UObject 队列对象，不是专门的 `BlueprintAsyncActionBase` 节点，所以 UI 必须保存 `LongTextQueue` 变量。
- 生成中暂停不会立即暂停服务端推理，只会在当前段生成完成后停住。
- 生成中跳段和停止属于软控制，当前请求返回后会被队列忽略。
- 当前没有真实口型和表情驱动，`FLocalTTSSpeechEvent` 里的口型帧和表情帧仍是预留字段。
- Demo UI 不应该替代正式业务 UI，它只是插件能力验收和展示入口。

## 13. 后续可升级方向

当前 Demo 稳定后，再考虑：

- 新增 `WBP_LocalTTS_SegmentRow`，用列表显示所有分段。
- 增加 3D Actor 播放测试，把声音绑定到 `TTS_Speaker`。
- 新增长文本专用异步蓝图节点，减少 UI 手动保存队列对象的步骤。
- 在关卡里加入字幕条，直接显示 `段落开始` 的文本。
- 后续数字人接入时，让 `段落已生成` 驱动口型分析入口。
