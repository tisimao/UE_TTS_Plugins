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
| `Instruct` | `instruct` | design 模式音色描述。 |
| `Duration` | `duration` | 目标时长，`0` 表示不发送给服务端，让模型决定。 |
| `Speed` | `speed` | 语速倍率，推荐 `0.5` 到 `2.0`，当前 UE 校验允许大于 `0` 且小于等于 `3.0`。 |

UE 侧会把 `Mode`、`LanguageId` 自动 trim 并转小写，所以蓝图里填 `Auto`、` auto ` 也会发送成标准 `auto`。

## 2. 推荐蓝图节点

创建请求时优先使用这些节点，不建议手填 `Mode` 字符串：

- `Make Local TTS Auto Request`
- `Make Local TTS Design Request`
- `Make Local TTS Clone Request`
- `Validate Local TTS Request`
- `Generate Local TTS Async`
- `Speak Local TTS Async`

推荐流程：

```text
Make Local TTS Auto/Design/Clone Request
-> Validate Local TTS Request
-> Speak Local TTS Async 或 Generate Local TTS Async
```

## 3. auto 模式

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

- 使用 `Make Local TTS Auto Request`。
- 明天普通播放测试优先用这个模式。

## 4. design 模式

用途：通过 `Instruct` 描述想要的声音风格。

推荐参数：

```text
Text = 欢迎来到我们的虚拟展厅，我会为你介绍今天的内容。
Mode = design
LanguageId = zh
Instruct = A warm, natural Chinese female voice.
Speed = 1.0
Duration = 0
```

蓝图建议：

- 使用 `Make Local TTS Design Request`。
- `Instruct` 必填，否则 UE 会在发送前拦截。
- 如果中文 instruct 效果不稳定，先用英文描述测试。

## 5. clone 模式

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

## 6. 当前 UE 侧校验

发送前会校验：

- `Text` 不能为空。
- `Mode` 只能是 `auto`、`design`、`clone`。
- `LanguageId` 只能是 `zh`、`en` 或留空。
- `Speed` 必须大于 `0` 且小于等于 `3`。
- `Duration` 不能小于 `0`，不能大于 `60`。
- `clone` 模式必须填写存在的 wav 参考音频。
- `design` 模式必须填写 `Instruct`。

## 7. 明天测试建议

建议顺序：

1. 先用 `auto` 跑通 `Speak` 和 `Gen`。
2. 再用 `design` 修改 `Instruct` 测声音风格变化。
3. 最后再测 `clone`，因为 clone 对参考音频和 ASR 状态更敏感。

如果出错，先看 `LastErrorMessage`。如果是参数问题，UE 侧现在应该能在发送前给出清晰原因。
