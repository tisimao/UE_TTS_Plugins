# UETTsProject

`LocalTTS` 是一个 UE5.6 插件，用于启动本地 OmniVoice TTS 服务、检查服务健康状态、生成 WAV 文件，并可在 Unreal 内直接播放。

## 快速开始

1. 运行 `Setup_TTS_Service.bat`。
2. 使用 `Build_UE56_TTSHost.bat` 编译 Unreal 工程。
3. 打开 [UE56_TTSHost.uproject](D:/project/UETTsProject/UE56_TTSHost/UE56_TTSHost.uproject)。
4. 如有需要，启用 `LocalTTS` 插件并重启编辑器。
5. 在 Unreal 中优先使用下面列出的正式蓝图节点，而不是示例 Actor 按钮。

## 正式蓝图入口

项目正式接入推荐使用这些节点：

- `启动 Local TTS`
- `异步检查 Local TTS 健康`
- `生成并播放 Local TTS`
- `生成并在 Actor 位置播放 Local TTS`
- `仅生成 Local TTS`
- `停止 Local TTS 播放`
- `获取最近 Local TTS WAV 路径`

推荐流程：

1. 在启动阶段或第一次发声前调用 `启动 Local TTS`。
2. 调用 `异步检查 Local TTS 健康`，等待状态进入 `就绪`。
3. 使用 `创建 Local TTS 自动模式请求`、`创建 Local TTS 音色设计请求` 或 `创建 Local TTS 音色克隆请求` 构建请求。
4. 用 `生成并播放 Local TTS` 或 `仅生成 Local TTS` 发起请求。
5. 如果请求失败，读取 `获取最近 Local TTS 健康错误`、`获取最近 Local TTS 语音错误` 以及对应错误码。

## UI 状态流建议

正式 UI 推荐监听异步节点的这些回调：

1. `OnStarted`
   把按钮切到禁用状态，显示“思考中”或“生成中”。
2. `OnStateChanged`
   如果希望 UI 状态机更稳定，优先监听这个统一状态回调。
   它会按阶段给出：`请求开始`、`等待服务就绪`、`生成中`、`音频已就绪`、`播放中`、`流程结束`、`发生错误`。
2. `OnAudioReady`
   把 UI 切到“可播放”或“播放中”。
   对自动播放节点，这通常表示音频已经准备好并即将开始播放。
   对仅生成节点，这表示 wav 已经可供数字人、自定义播放器或后处理系统使用。
3. `OnFinished`
   自动播放结束后恢复按钮状态，关闭“播放中”提示。
4. `OnError`
   显示错误原因并恢复按钮状态。
5. `OnSpeechEventReady`
   如果要对接数字人、字幕、口型或时间线系统，在这里拿语音事件最方便。

补充建议：

- 重复点击前，先用 `Local TTS 是否忙碌` 做按钮节流。
- 长文本时，不要等到播放才改 UI，`OnStarted` 就应该先显示“思考中”。
- 如果要减少蓝图里自己维护布尔量，优先用 `OnStateChanged` 驱动 UI 状态机。
- `OnSucceeded` 表示服务端已经成功生成了结果，但真正适合切到“播放中”的时机通常是 `OnAudioReady`。

## 示例 Actor

`ALocalTTSTestActor` 现在仅用于 PIE 调试和冒烟验证。

- 适合验证服务启动、健康检查、语音生成和播放链路。
- 不应作为正式业务蓝图入口。
- 在编辑器中显示为 `LocalTTS 示例 Actor`。

## 常见问题

- 如果 LocalTTS 找不到 Python，请先运行 `Setup_TTS_Service.bat`，然后检查 `Project Settings > Plugins > LocalTTS`。
- 如果 `/health` 或 `/tts` 无法访问，请检查 `Service Base URL` 并确认服务进程是否成功启动。
- 如果 LocalTTS 长时间无法进入 `就绪`，请检查服务控制台输出，并确认模型是否已完成加载。
- 如果请求因为忙碌被拒绝，请等待 `OnFinished`，或在 UI 层使用 `Local TTS 是否忙碌` 防止重复点击。

## 更多文档

项目文档索引见 [Doc/README.md](D:/project/UETTsProject/Doc/README.md)。
