const fs = require("fs");
const path = require("path");

const mdjPath = path.join(process.cwd(), "Doc", "LocalTTS_Class_Diagram.mdj");

const descriptions = {
  FLocalTTSModule: "插件运行时模块入口",
  ULocalTTSBlueprintLibrary: "蓝图同步函数入口",
  ULocalTTSSpeakAsyncAction: "蓝图异步说话节点对象",
  ULocalTTSSubsystem: "插件总控与流程调度中心",
  FLocalTTSServiceProcess: "本地 TTS 服务进程管理器",
  FLocalTTSHttpClient: "本地 HTTP 请求封装器",
  FLocalTTSRequestValidator: "请求结构校验器",
  FLocalTTSWavLoader: "WAV 文件验证与加载器",
  ULocalTTSAudioPlayer: "UE 音频播放控制器",
  FLocalTTSSpeakRequest: "UE 侧 TTS 请求数据结构",
  FLocalTTSHealthResponse: "UE 侧健康检查响应结构",
  FLocalTTSTTSResponse: "UE 侧语音生成响应结构",
  AppConfig: "服务配置对象",
  TTSRequest: "服务侧 TTS 请求结构",
  HealthResponse: "服务健康检查返回结构",
  TTSResponse: "服务语音生成返回结构",
  OmniVoiceEngine: "OmniVoice 模型引擎封装类",
  WavCacheManager: "WAV 缓存与命名管理器",
  TTSService: "服务业务总控类",
};

const documentations = {
  FLocalTTSModule:
    "中文描述：插件运行时模块入口。职责：跟随插件加载和卸载，负责模块生命周期初始化，不承载具体业务流程。",
  ULocalTTSBlueprintLibrary:
    "中文描述：提供给蓝图使用的同步函数入口。职责：暴露 StartLocalTTS、IsLocalTTSReady、StopSpeaking，并将蓝图请求转交给 ULocalTTSSubsystem。",
  ULocalTTSSpeakAsyncAction:
    "中文描述：蓝图异步说话节点对象。职责：保存蓝图请求结构体，在 Activate 中发起请求，并广播 OnStarted、OnAudioReady、OnFinished、OnError。",
  ULocalTTSSubsystem:
    "中文描述：第一版插件总控类。职责：统一执行检查服务、校验请求、请求生成、加载 wav、播放和回调。",
  FLocalTTSServiceProcess:
    "中文描述：本地 TTS 服务进程管理器。职责：检查运行状态、启动 run_server.py、等待服务 Ready、停止服务。",
  FLocalTTSHttpClient:
    "中文描述：本地 HTTP 请求封装器。职责：发送 GET /health、POST /tts，并解析 JSON 响应。",
  FLocalTTSRequestValidator:
    "中文描述：请求结构校验器。职责：校验 mode、ReferenceAudioPath、Instruct、duration、speed。",
  FLocalTTSWavLoader:
    "中文描述：WAV 文件验证与加载器。职责：校验 wav_path 并把 wav 转成 UE 可播放音频对象。",
  ULocalTTSAudioPlayer:
    "中文描述：UE 音频播放控制器。职责：触发播放、停止播放、监听播放完成。",
  FLocalTTSSpeakRequest:
    "中文描述：UE 侧 TTS 请求数据结构。职责：封装文本、模式、语言、参考音频、参考文本、音色描述、时长和语速。",
  FLocalTTSHealthResponse:
    "中文描述：UE 侧健康检查响应结构。职责：保存服务可用状态、服务状态、模型名称和支持模式列表。",
  FLocalTTSTTSResponse:
    "中文描述：UE 侧语音生成响应结构。职责：保存请求结果、请求模式、wav 路径、采样率、耗时、错误码与错误信息。",
  AppConfig:
    "中文描述：服务配置对象。职责：保存监听地址、端口、模型目录、缓存目录和日志目录。",
  TTSRequest:
    "中文描述：服务侧接收到的 TTS 请求结构。职责：校验 text、mode、language_id、ref_audio、ref_text、instruct、duration、speed。",
  HealthResponse:
    "中文描述：服务健康检查返回结构。职责：返回服务名、当前状态、模型名和支持模式。",
  TTSResponse:
    "中文描述：服务语音生成返回结构。职责：返回请求编号、请求模式、wav 路径、采样率、耗时或错误信息。",
  OmniVoiceEngine:
    "中文描述：OmniVoice 模型引擎封装类。职责：加载 OmniVoice，根据 auto、clone、design 三种模式执行推理。",
  WavCacheManager:
    "中文描述：WAV 缓存与命名管理器。职责：创建 cache 目录、生成请求编号和 wav 文件路径。",
  TTSService:
    "中文描述：服务业务总控类。职责：响应健康检查、处理 TTS 请求、调用模型引擎和缓存管理器，并组织响应。",
};

function collectById(node, map) {
  if (!node || typeof node !== "object") return;
  if (node._id) map.set(node._id, node);
  for (const value of Object.values(node)) {
    if (Array.isArray(value)) {
      for (const item of value) collectById(item, map);
    } else if (value && typeof value === "object" && !("$ref" in value)) {
      collectById(value, map);
    }
  }
}

function bumpClassViewLayout(classView, extraHeight) {
  classView.height += extraHeight;
  const nameComp = classView.subViews.find((v) => v._type === "UMLNameCompartmentView");
  const attrComp = classView.subViews.find((v) => v._type === "UMLAttributeCompartmentView");
  const opComp = classView.subViews.find((v) => v._type === "UMLOperationCompartmentView");

  if (nameComp) nameComp.height += extraHeight;
  if (attrComp) attrComp.top += extraHeight;
  if (opComp) opComp.top += extraHeight;

  if (attrComp) {
    for (const sub of attrComp.subViews || []) sub.top += extraHeight;
  }
  if (opComp) {
    for (const sub of opComp.subViews || []) sub.top += extraHeight;
  }
}

function ensureDescriptionAttribute(cls, descriptionText) {
  cls.attributes ||= [];
  let attr = cls.attributes.find((item) => item.name === "说明");
  if (!attr) {
    attr = {
      _type: "UMLAttribute",
      _id: `DESC_ATTR_${cls._id}`,
      _parent: { $ref: cls._id },
      name: "说明",
      visibility: "public",
      isStatic: false,
      isLeaf: false,
      type: descriptionText,
      isReadOnly: true,
      isOrdered: false,
      isUnique: false,
      defaultValue: "",
    };
    cls.attributes.unshift(attr);
  } else {
    attr.type = descriptionText;
  }
  return attr;
}

const project = JSON.parse(fs.readFileSync(mdjPath, "utf8"));
const byId = new Map();
collectById(project, byId);

const model = project.ownedElements?.[0];
const diagram = model?.ownedElements?.find((el) => el._type === "UMLClassDiagram");

for (const element of model.ownedElements) {
  if (element._type === "UMLClass" && documentations[element.name]) {
    element.documentation = documentations[element.name];
    ensureDescriptionAttribute(element, descriptions[element.name]);
  }
}

collectById(project, byId);

for (const view of diagram.ownedViews) {
  if (view._type !== "UMLClassView") continue;
  const modelRef = view.model?.$ref;
  const cls = byId.get(modelRef);
  if (!cls || !descriptions[cls.name]) continue;

  const nameComp = view.subViews.find((v) => v._type === "UMLNameCompartmentView");
  if (!nameComp) continue;

  const propertyRef = nameComp.propertyLabel?.$ref;
  const propertyLabel = byId.get(propertyRef);
  if (!propertyLabel) continue;

  propertyLabel.visible = true;
  propertyLabel.text = descriptions[cls.name];
  propertyLabel.left = view.left + 10;
  propertyLabel.top = view.top + 24;
  propertyLabel.width = Math.max(160, view.width - 20);
  propertyLabel.height = 13;
  propertyLabel.horizontalAlignment = 2;

  if (nameComp.height < 42) {
    bumpClassViewLayout(view, 17);
  }

  const attrComp = view.subViews.find((v) => v._type === "UMLAttributeCompartmentView");
  if (!attrComp) continue;

  const descAttr = cls.attributes.find((item) => item.name === "说明");
  if (!descAttr) continue;

  let descView = (attrComp.subViews || []).find((item) => item.model?.$ref === descAttr._id);
  if (!descView) {
    descView = {
      _type: "UMLAttributeView",
      _id: `DESC_ATTR_VIEW_${view._id}`,
      _parent: { $ref: attrComp._id },
      model: { $ref: descAttr._id },
      visible: true,
      enabled: true,
      lineColor: "#000000",
      fillColor: "#ffffff",
      fontColor: "#000000",
      font: "Arial;13;0",
      parentStyle: true,
      showShadow: true,
      containerChangeable: false,
      containerExtending: true,
      left: view.left + 5,
      top: attrComp.top + 5,
      width: view.width - 10,
      height: 13,
      autoResize: false,
      underline: false,
      text: `+说明: ${descriptions[cls.name]}`,
      horizontalAlignment: 0,
      verticalAlignment: 5,
    };
    attrComp.subViews ||= [];
    attrComp.subViews.unshift(descView);
  } else {
    descView.text = `+说明: ${descriptions[cls.name]}`;
  }

  const lineHeight = 15;
  for (let i = 0; i < attrComp.subViews.length; i += 1) {
    const sub = attrComp.subViews[i];
    sub.top = attrComp.top + 5 + i * lineHeight;
    sub.left = view.left + 5;
    sub.width = view.width - 10;
  }
  const desiredAttrHeight = Math.max(25, attrComp.subViews.length * lineHeight + 10);
  const delta = desiredAttrHeight - attrComp.height;
  if (delta !== 0) {
    attrComp.height = desiredAttrHeight;
    view.height += delta;
    const opComp = view.subViews.find((v) => v._type === "UMLOperationCompartmentView");
    if (opComp) {
      opComp.top += delta;
      for (const sub of opComp.subViews || []) {
        sub.top += delta;
      }
    }
  }
}

fs.writeFileSync(mdjPath, JSON.stringify(project, null, 2), "utf8");
console.log(`Updated ${mdjPath}`);
