# OmniVoice 参数体验

## 1. 当前协议字段

UE 插件通过本地 `/tts` 接口调用 OmniVoice，当前请求结构是 `FLocalTTSSpeakRequest`。

字段说明：

| 字段 | JSON 字段 | 作用 |
| --- | --- | --- |
| `Text` | `text` | 要合成的文本，必填。 |
| `Mode` | `mode` | 生成模式，支持 `auto`、`design`、`clone`。 |
| `LanguageId` | `language_id` | 语言提示，目前 UE 侧推荐 `zh` 或 `en`，可留空。 |
| `ReferenceAudioPath` | `ref_audio` | clone 模式参考音频路径，当前要求本地 wav 文件。 |
| `ReferenceText` | `ref_text` | 参考音频对应文本。ASR 不可用时 clone 模式需要填写。 |
| `Instruct` | `instruct` | design 模式音色标签列表。不是自由描述文本。 |
| `Duration` | `duration` | 目标时长，`0` 表示不发送给服务端，让模型决定。 |
| `Speed` | `speed` | 语速倍率，推荐 `0.5` 到 `2.0`，当前 UE 校验允许大于 `0` 且小于等于 `3.0`。 |

UE 侧会把 `Mode`、`LanguageId` 自动 trim 并转小写，所以蓝图里填 `Auto`、` auto ` 也会发送成标准 `auto`。

## 2. 当前预设值总表

这些预设值来自 C++ 默认值、蓝图预设节点和测试 Actor，后续如果调参优先看这里。

| 位置 | 字段 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `FLocalTTSSpeakRequest` | `Mode` | `auto` | 默认使用 OmniVoice auto 模式。 |
| `FLocalTTSSpeakRequest` | `Duration` | `0` | 0 表示不发送 duration，让模型自己决定时长。 |
| `FLocalTTSSpeakRequest` | `Speed` | `1.0` | 正常语速。 |
| `Make Local TTS Auto Request` | `LanguageId` | `zh` | 当输入为空时自动使用中文语言提示。 |
| `Make Local TTS Auto Request` | `Speed` | `1.0` | 当输入小于等于 0 时自动使用正常语速。 |
| `Make Local TTS Design Request` | `Mode` | `design` | 预设为 design 模式。 |
| `Make Local TTS Design Request` | `Instruct` | `female, chinese accent` | 当 Instruct 为空时使用官方支持的英文标签组合。 |
| `Make Local TTS Clone Request` | `Mode` | `clone` | 预设为 clone 模式，仍需要填写参考音频。 |
| `ULocalTTSSettings` | `ServiceBaseUrl` | `http://127.0.0.1:50021` | 本地 FastAPI 服务地址。 |
| `ULocalTTSSettings` | `ServiceRelativeRoot` | `Services/tts_service` | 服务端代码相对仓库根目录的位置。 |
| `ULocalTTSSettings` | `PythonRelativePath` | `.venv/Scripts/python.exe` | 服务目录内 Python 虚拟环境路径。 |
| `ULocalTTSSettings` | `RunServerScriptName` | `run_server.py` | 服务启动脚本。 |
| `ULocalTTSSettings` | `MaxHealthPollCount` | `30` | 最多轮询 30 次服务 ready 状态。 |
| `ULocalTTSSettings` | `HealthPollIntervalSeconds` | `1.0` | 每 1 秒检查一次服务健康状态。 |
| `Speak Local TTS Async` | `bAutoPlay` | `true` | 生成 wav 后自动用 UE 2D 声音播放。 |
| `Speak Local TTS At Actor Async` | `bAutoPlay` | `true` | 生成 wav 后自动播放。 |
| `Speak Local TTS At Actor Async` | `bUseActorPlayback` | `true` | 使用指定 Actor 的位置进行 3D 播放。 |
| `Generate Local TTS Async` | `bAutoPlay` | `false` | 只生成 wav，不自动播放，适合数字人链路。 |
| `LocalTTS Example Actor` | `Text` | `你好，这是一次 UE 本地语音测试。` | 示例 Actor 默认朗读文本。 |
| `LocalTTS Example Actor` | `Mode` | `auto` | 示例 Actor 默认模式。 |
| `LocalTTS Example Actor` | `LanguageId` | `zh` | 示例 Actor 默认语言。 |
| `LocalTTS Example Actor` | `Speed` | `1.0` | 示例 Actor 默认语速。 |
| `LocalTTS Example Actor` | `bCheckHealthOnBeginPlay` | `true` | PIE 开始时自动检查健康状态。 |
| `LocalTTS Example Actor` | `bStartServiceOnBeginPlay` | `true` | PIE 开始时自动启动服务。 |
| `LocalTTS Example Actor` | `bSpeakOnBeginPlay` | `true` | PIE 开始后自动朗读。 |
| `LocalTTS Example Actor` | `AutoSpeakDelaySeconds` | `3.0` | 自动朗读前等待 3 秒。 |

## 3. 服务端环境变量预设值

这些值来自 `Services/tts_service/app/AppConfig.py`。一般不需要改；只有换端口、换设备、换缓存目录或排查模型加载时才会用到。

| 环境变量 | 默认值 | 中文说明 |
| --- | --- | --- |
| `LOCAL_TTS_HOST` | `127.0.0.1` | 服务监听地址。默认只允许本机 UE 访问，比较安全。 |
| `LOCAL_TTS_PORT` | `50021` | 服务端口。UE 插件的 `ServiceBaseUrl` 默认也指向这个端口，两边要保持一致。 |
| `LOCAL_TTS_MODEL_NAME` | `k2-fsa/OmniVoice` | Hugging Face 上的 OmniVoice 模型名。 |
| `LOCAL_TTS_DEVICE` | `cuda` | 推理设备。当前推荐 NVIDIA CUDA；如果机器没有 CUDA，可临时改成 `cpu`，但会明显变慢。 |
| `LOCAL_TTS_DTYPE` | `float16` | 推理精度。CUDA 下默认 `float16` 更省显存；CPU 测试时如果报精度相关问题，可尝试 `float32`。 |
| `LOCAL_TTS_CACHE_DIR` | `Services/tts_service/cache` | 生成 wav 的缓存目录。UE 收到的 `wav_path` 通常就在这里。 |
| `LOCAL_TTS_MAX_CACHE_WAVS` | `50` | 最多保留多少个最近生成的 wav。超过上限时服务会自动删除更旧文件。 |
| `LOCAL_TTS_LOG_DIR` | `Services/tts_service/logs` | 服务日志目录。HTTP 502 或模型推理失败时优先看这里。 |
| `LOCAL_TTS_HF_CACHE_DIR` | 空 | Hugging Face 模型缓存目录。为空时使用默认缓存位置；想把大模型放到指定磁盘时再设置。 |
| `LOCAL_TTS_REQUEST_TIMEOUT_SEC` | `300` | 单次 TTS 请求超时时间，单位秒。长文本或首次推理较慢时需要足够大。 |
| `LOCAL_TTS_EAGER_LOAD` | `true` | 服务启动时立即加载模型。默认打开，这样 UE 第一次请求时更稳定。 |
| `LOCAL_TTS_LOAD_ASR` | `true` | 是否尝试加载 ASR。clone 模式未填写 `ReferenceText` 时会用到；加载失败后服务会降级为无 ASR。 |
| `LOCAL_TTS_ASR_MODEL_NAME` | `openai/whisper-large-v3-turbo` | ASR 模型名。只有需要参考音频自动识别时才重要。 |

## 4. 推荐蓝图节点

创建请求时优先使用这些节点，不建议手填 `Mode` 字符串：

- `MakeAutoSpeakRequest` / `创建 Auto 语音请求`
- `MakeDesignSpeakRequest` / `创建 Design 语音请求`
- `MakeCloneSpeakRequest` / `创建 Clone 语音请求`
- `ValidateSpeakRequest` / `校验 Local TTS 请求`
- `GenerateSpeechAsync` / `只生成 Local TTS`
- `SpeakTextAsync` / `生成并播放 Local TTS`
- `SpeakTextAtActorAsync` / `在 Actor 位置播放 Local TTS`

推荐流程：

```text
创建 Auto/Design/Clone 语音请求
-> 校验 Local TTS 请求
-> 生成并播放 Local TTS 或 只生成 Local TTS
```

## 5. auto 模式

用途：最常用的默认模式，适合先验证文字转语音链路。

推荐参数：

```text
Text = 你好，这是一次 OmniVoice 本地语音测试。
Mode = auto
LanguageId = zh
Speed = 1.0
Duration = 0
```

蓝图建议：

- 使用 `MakeAutoSpeakRequest` / `创建 Auto 语音请求`。
- 明天普通播放测试优先用这个模式。

## 6. Duration 目标时长

`Duration` 对应接口里的 `duration` 字段，含义是“希望模型生成出来的语音接近多少秒”。它不是等待超时，不是播放时长，也不是服务端返回的 `duration_ms`。

当前实现规则：

| UE 填写值 | 是否发送给服务端 | 预期效果 |
| --- | --- | --- |
| `0` | 不发送 `duration` | 默认推荐。让 OmniVoice 根据文本长度、语速和模型习惯自己决定语音长度。 |
| `0.1` 到 `60` | 发送 `duration` | 作为目标时长传给 OmniVoice，模型会尝试让生成音频接近这个秒数。 |
| 小于 `0` | UE 侧拦截 | 参数无效，会报 `目标时长 Duration 不能小于 0`。 |
| 大于 `60` | UE 侧拦截 | 当前插件限制最大 60 秒，避免长文本或误填导致请求过慢。 |

实际效果理解：

- `Duration = 0` 最自然。大多数普通 TTS、旁白、数字人台词都建议先用 0。
- `Duration` 大于自然朗读时长时，模型可能会放慢语速、拉长停顿，或让声音听起来更拖。
- `Duration` 小于自然朗读时长时，模型可能会加快语速、压缩停顿，过短时可能影响清晰度。
- `Duration` 是目标值，不保证生成音频精确等于这个秒数。OmniVoice 会尽量贴近，但最终仍取决于文本长度和模型能力。
- 服务端返回的 `duration_ms` 目前表示“本次生成耗时毫秒”，不是 wav 的真实播放时长。真实音频长度后续如果需要更精确，应从 wav 文件头或 UE `SoundWave.Duration` 读取。

和 `Speed` 的关系：

- `Speed` 控制语速倍率，`Duration` 控制目标总时长，两者都填时可能互相拉扯。
- 想要自然朗读：优先只调 `Speed`，保持 `Duration = 0`。
- 想要对齐动画、镜头、数字人口型时长：可以设置 `Duration`，但建议先保持 `Speed = 1.0`。
- 如果 `Duration` 和文本长度明显不匹配，比如很长文本要求 1 秒，实际效果通常不会理想。

推荐测试值：

| 场景 | 推荐值 | 说明 |
| --- | --- | --- |
| 普通中文短句 | `0` | 让模型自然生成，最稳定。 |
| 想稍微慢一点 | `0` + `Speed = 0.8` | 比直接写死 Duration 更自然。 |
| 想稍微快一点 | `0` + `Speed = 1.2` | 适合 UI 提示或较短反馈。 |
| 要对齐 3 秒动画 | `3.0` + `Speed = 1.0` | 先用短句测试，确认声音是否自然。 |
| 要对齐 5 秒数字人台词 | `5.0` + `Speed = 1.0` | 文本长度要和 5 秒朗读量匹配。 |

建议你明天测试时这样试：

1. 先用同一句话测试 `Duration = 0`，听自然基准。
2. 再测试 `Duration = 2.0`、`3.0`、`5.0`，对比是否出现拖长或压缩。
3. 如果只是想改变快慢，优先回到 `Duration = 0`，改 `Speed`。
4. 如果将来要做数字人口型同步，再把 `Duration` 当作“对齐动画时长”的辅助参数使用。

## 7. design 模式

用途：通过 `Instruct` 标签组合描述想要的声音风格。

注意：OmniVoice 的 `Instruct` 不是自由描述文本，只能使用模型支持的标签。英文标签用半角逗号加空格分隔，中文标签用全角逗号分隔，并且不要混用中英文。

当前测试结论：`design` 模式配置链路已经通过 UE 实际播放测试，可以按照合法标签生成并播放。后续重点不是接口是否可用，而是根据不同项目、角色和场景选择合适的音色标签组合。

推荐参数：

```text
Text = 欢迎来到我们的虚拟展厅，我会为你介绍今天的内容。
Mode = design
LanguageId = zh
Instruct = female, chinese accent
Speed = 1.0
Duration = 0
```

也可以使用中文标签：

```text
Instruct = 女，青年，中音调
```

支持的英文标签：

```text
american accent, australian accent, british accent, canadian accent, child, chinese accent, elderly, female, high pitch, indian accent, japanese accent, korean accent, low pitch, male, middle-aged, moderate pitch, portuguese accent, russian accent, teenager, very high pitch, very low pitch, whisper, young adult
```

支持的中文标签：

```text
东北话，中年，中音调，云南话，低音调，儿童，四川话，女，宁夏话，少年，极低音调，极高音调，桂林话，河南话，济南话，甘肃话，男，石家庄话，老年，耳语，贵州话，陕西话，青岛话，青年，高音调
```

蓝图建议：

- 使用 `MakeDesignSpeakRequest` / `创建 Design 语音请求`。
- `Instruct` 必填，否则 UE 会在发送前拦截。
- 推荐先用 `female, chinese accent` 或 `女，青年，中音调` 测试。
- 不要填写 `A warm, natural Chinese female voice.` 这类自然语言描述，OmniVoice 会报 unsupported instruct item。

## 8. clone 模式

用途：使用参考音频克隆或贴近参考声音。

推荐参数：

```text
Text = 这是一段克隆音色测试文本。
Mode = clone
LanguageId = zh
ReferenceAudioPath = D:\path\to\reference.wav
ReferenceText = 参考音频里实际说的话
Speed = 1.0
Duration = 0
```

注意：

- `ReferenceAudioPath` 必须存在。
- 当前 UE 侧要求参考音频是 `wav`。
- 如果服务端 ASR 不可用，`ReferenceText` 必须填写。
- 参考音频建议先用短句，音质干净、背景噪声少。

## 9. 当前 UE 侧校验

发送前会校验：

- `Text` 不能为空。
- `Mode` 只能是 `auto`、`design`、`clone`。
- `LanguageId` 只能是 `zh`、`en` 或留空。
- `Speed` 必须大于 `0` 且小于等于 `3`。
- `Duration` 不能小于 `0`，不能大于 `60`。
- `clone` 模式必须填写存在的 wav 参考音频。
- `design` 模式必须填写 `Instruct`。
- `design` 模式的 `Instruct` 必须由 OmniVoice 支持的标签组成。

## 10. 测试建议

建议顺序：

1. 先用 `auto` 跑通 `Speak` 和 `Gen`。
2. 再用 `design` 修改 `Instruct` 测声音风格变化。
3. 最后再测 `clone`，因为 clone 对参考音频和 ASR 状态更敏感。

如果出错，先看 `LastErrorMessage`。如果是参数问题，UE 侧现在应该能在发送前给出清晰原因。
