# 打包交付与部署 LocalTTS 版本

## 1. 目标

本文档说明如何把 `UE56_TTSHost` 打包成 Windows 可运行版本，并把本地 `LocalTTS` 服务、Python 依赖和 OmniVoice 模型一起随包交付。

目标体验：

```text
开发机打包
-> 压缩 Dist/UE56_TTSHost_Win64/Windows
-> 交给测试用户
-> 用户解压
-> 双击 UE56_TTSHost.exe
-> 在 UI 中启动 LocalTTS 服务并生成/播放语音
```

本文档面向“内测交付包”。当前方案会随包复制 `.venv`，适合项目内测试和固定机器测试。正式给普通用户发布前，建议把 Python 服务冻结成独立 exe，详见第 10 节。

## 2. 当前交付策略

UE 打包和 TTS 服务交付分两部分完成：

1. 使用 Unreal `RunUAT BuildCookRun` 打包 Win64 Shipping。
2. 把 `Services/tts_service` 复制到 UE 打包产物旁边，并把 OmniVoice Hugging Face 模型缓存复制进包内。

最终交付目录固定为：

```text
Dist/
  UE56_TTSHost_Win64/
    Windows/
      UE56_TTSHost.exe
      UE56_TTSHost/
        Binaries/
        Content/
        Plugins/
        Saved/
        ...
      Services/
        tts_service/
          run_server.py
          requirements.txt
          app/
          .venv/
          models/
            huggingface/
              models--k2-fsa--OmniVoice/
```

交付时压缩整个 `Windows` 文件夹，不要只压缩 exe。

## 3. 开发机前置条件

打包开发机需要满足：

- Windows 10/11 64 位。
- Unreal Engine 5.6，默认路径为 `C:\Program Files\Epic Games\UE_5.6`。
- Visual Studio 2022，安装 C++ 游戏开发组件。
- Python 服务已初始化。
- 本机已经成功启动过 LocalTTS 服务，并完成过一次 OmniVoice 模型加载。

如果 UE 安装路径不是默认路径，需要修改：

```text
Package_UE56_TTSHost_Windows.bat
```

里的：

```bat
set "UE_ROOT=C:\Program Files\Epic Games\UE_5.6"
```

## 4. 打包前检查

在项目根目录执行：

```bat
Setup_TTS_Service.bat
```

确认 Python 解释器存在：

```text
Services/tts_service/.venv/Scripts/python.exe
```

确认 UE 编辑器版本能编译：

```bat
Build_UE56_TTSHost.bat
```

确认本机已经下载 OmniVoice 模型缓存：

```text
%USERPROFILE%\.cache\huggingface\hub\models--k2-fsa--OmniVoice
```

当前主模型缓存约 3GB 以上。打包脚本会把它复制到包内：

```text
Windows/Services/tts_service/models/huggingface/models--k2-fsa--OmniVoice
```

如果使用 clone 模式并启用了 ASR，首次运行还可能下载 Whisper ASR 模型。正式离线包也需要把对应 Hugging Face 缓存目录复制到：

```text
Windows/Services/tts_service/models/huggingface
```

常见 ASR 缓存目录名称类似：

```text
models--openai--whisper-large-v3-turbo
```

## 5. 一键打包流程

在项目根目录运行：

```bat
Package_UE56_TTSHost_Windows.bat
```

脚本会执行：

1. 调用 UE 5.6 `RunUAT.bat`。
2. 执行 Win64 Shipping 的 build、cook、stage、pak、archive。
3. 输出到 `Dist/UE56_TTSHost_Win64/Windows`。
4. 复制 `Services/tts_service` 到 `Windows/Services/tts_service`。
5. 排除运行时临时目录：`cache`、`logs`、`__pycache__`、`.pytest_cache`。
6. 复制本机 OmniVoice 模型缓存到包内 `models/huggingface`。

脚本完成后看到：

```text
Package is ready:
D:\project\UETTsProject\Dist\UE56_TTSHost_Win64\Windows
```

即可压缩 `Windows` 文件夹。

## 6. UE 运行时如何找到服务

插件启动 LocalTTS 服务时，会查找多个候选路径，兼容编辑器和打包环境。

编辑器开发时常用路径：

```text
D:\project\UETTsProject\Services\tts_service
```

打包后推荐路径：

```text
Windows/Services/tts_service
```

只要该目录下存在：

```text
run_server.py
```

插件就会把它作为服务根目录。

当前配置仍在：

```text
UE56_TTSHost/Config/DefaultGame.ini
```

关键配置：

```ini
[/Script/LocalTTS.LocalTTSSettings]
ServiceBaseUrl="http://127.0.0.1:50021"
ServiceRelativeRoot=Services/tts_service
PythonRelativePath=.venv/Scripts/python.exe
RunServerScriptName=run_server.py
```

## 7. 模型、缓存和日志目录

包内模型目录：

```text
Windows/Services/tts_service/models/huggingface
```

UE 启动服务前，如果该目录存在，会自动设置：

```text
LOCAL_TTS_HF_CACHE_DIR=<包内 Services/tts_service/models/huggingface>
```

服务生成的 wav 和日志不会写到程序安装目录，而是写到 UE Saved 目录：

```text
Saved/LocalTTS/cache
Saved/LocalTTS/logs
```

这样做是为了避免用户把程序放在 `Program Files` 或其他无写权限目录后，服务无法创建缓存和日志。

## 8. 测试用户部署步骤

给用户的最短说明：

1. 解压交付压缩包。
2. 进入解压后的 `Windows` 目录。
3. 双击 `UE56_TTSHost.exe`。
4. 在 UI 中点击启动服务。
5. 等待健康状态变为 ready。
6. 输入文本，点击生成并播放。

不要让用户移动或删除：

```text
Windows/Services
```

否则 UE 可以启动，但 LocalTTS 服务无法找到。

## 9. 用户机器要求

当前内测包建议运行环境：

- Windows 10/11 64 位。
- NVIDIA 显卡。
- 已安装兼容当前 PyTorch CUDA wheel 的 NVIDIA 驱动。
- 建议至少 15GB 可用磁盘空间。
- 首次启动模型加载会比较慢，需要等待服务健康状态变为 ready。

如果目标机器没有 NVIDIA/CUDA，可以尝试把服务环境变量 `LOCAL_TTS_DEVICE` 调为 `cpu`，但速度可能明显下降，不建议作为主要体验。

## 10. 正式发布前建议

当前脚本复制 `.venv`，这是最快的内测方案，但不是最稳的商业交付方案。

原因是：

```text
Services/tts_service/.venv/pyvenv.cfg
```

里可能记录开发机 Python 路径，例如：

```text
home = C:\Users\...\Python310
```

在其他机器上可能仍能运行，也可能因为 Python、DLL、CUDA、依赖路径差异启动失败。

正式发布建议改成以下方案之一：

1. 用 PyInstaller 或 Nuitka 把 `run_server.py` 冻结成 `LocalTTSService.exe`。
2. 使用 Python embeddable runtime，并把依赖固定在包内 runtime 目录。
3. 单独做一个 LocalTTS Runtime 安装器，负责 Python、依赖、模型、CUDA 兼容检查。

正式方案还需要把 UE 插件的 `PythonRelativePath` 或启动逻辑改为优先启动：

```text
Services/tts_service/LocalTTSService.exe
```

而不是：

```text
Services/tts_service/.venv/Scripts/python.exe run_server.py
```

## 11. 许可与体积检查

正式对外分发前必须确认：

- OmniVoice 模型许可允许随产品再分发。
- `omnivoice` Python 包许可允许随产品再分发。
- PyTorch、torchaudio、FastAPI、uvicorn、soundfile 等依赖许可允许随包分发。
- 如果随包包含 CUDA 相关动态库，需要确认 NVIDIA/CUDA 组件的分发许可。
- 模型和依赖体积较大，需要评估下载、安装和更新方式。

## 12. 验收清单

打包开发机验收：

- [ ] `Build_UE56_TTSHost.bat` 编译成功。
- [ ] 编辑器内 LocalTTS UI 可以启动服务。
- [ ] 编辑器内单句生成并播放成功。
- [ ] 编辑器内长文本队列基本播放成功。
- [ ] `Package_UE56_TTSHost_Windows.bat` 打包成功。
- [ ] `Windows/Services/tts_service/run_server.py` 存在。
- [ ] `Windows/Services/tts_service/.venv/Scripts/python.exe` 存在。
- [ ] `Windows/Services/tts_service/models/huggingface/models--k2-fsa--OmniVoice` 存在。

交付包本机验收：

- [ ] 从 `Dist/UE56_TTSHost_Win64/Windows` 双击 `UE56_TTSHost.exe`。
- [ ] UI 显示服务启动反馈。
- [ ] 健康检查最终显示 `ready`。
- [ ] 单句生成并播放成功。
- [ ] 音频播放完成后再次点击生成并播放不会误报 busy。
- [ ] 长文本播放、下一段预生成、暂停、继续、停止符合预期。
- [ ] `Saved/LocalTTS/logs` 生成服务日志。
- [ ] `Saved/LocalTTS/cache` 生成 wav 缓存。

新机器验收：

- [ ] 在未安装项目源码的新机器上解压运行。
- [ ] 不依赖 Unreal Editor。
- [ ] 不依赖用户手动安装 Python。
- [ ] 不联网也能加载包内 OmniVoice 主模型。
- [ ] 服务启动失败时 UI 能显示明确错误。

## 13. 常见问题

### 启动后提示找不到 Python

检查包内是否存在：

```text
Windows/Services/tts_service/.venv/Scripts/python.exe
```

如果不存在，说明服务目录没有正确复制进包。

### 启动后重新下载模型

检查包内是否存在：

```text
Windows/Services/tts_service/models/huggingface/models--k2-fsa--OmniVoice
```

如果不存在，说明打包开发机上没有模型缓存，或打包脚本没有复制成功。

### 服务健康检查超时但稍后 ready

通常是首次加载模型耗时较长。可以等待一段时间后再次点击健康检查。如果经常发生，需要调大：

```ini
MaxHealthPollCount=30
HealthPollIntervalSeconds=1.0
```

### 用户机器没有声音或生成失败

先看日志：

```text
Saved/LocalTTS/logs
```

再确认：

- NVIDIA 驱动是否正常。
- PyTorch CUDA 是否能加载。
- 磁盘空间是否足够。
- 包内模型目录是否完整。

### 放到 Program Files 后异常

当前 wav 缓存和日志已经改到 `Saved/LocalTTS`，理论上不会写入程序目录。如果仍有写权限问题，优先检查 Python 依赖或模型库是否试图写入包内目录。
