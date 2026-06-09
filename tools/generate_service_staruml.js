const fs = require("fs");
const path = require("path");

let counter = 1;
const nextId = (prefix = "ID") => `${prefix}_${String(counter++).padStart(4, "0")}`;
const ref = (id) => ({ $ref: id });

const projectId = nextId("PROJECT");
const modelId = nextId("MODEL");
const diagramId = nextId("DIAGRAM");

const project = {
  _type: "Project",
  _id: projectId,
  name: "OmniVoice Service Skeleton Design",
  ownedElements: [],
};

const model = {
  _type: "UMLModel",
  _id: modelId,
  _parent: ref(projectId),
  name: "Model",
  ownedElements: [],
};
project.ownedElements.push(model);

const diagram = {
  _type: "UMLClassDiagram",
  _id: diagramId,
  _parent: ref(modelId),
  name: "OmniVoice Service Skeleton Diagram",
  visible: true,
  defaultDiagram: true,
  ownedViews: [],
};
model.ownedElements.push(diagram);

function classElement(name, documentation, attributes = [], operations = [], note = "") {
  const id = nextId("CLASS");
  const cls = {
    _type: "UMLClass",
    _id: id,
    _parent: ref(modelId),
    name,
    documentation,
    visibility: "public",
    attributes: [],
    operations: [],
  };

  if (note) {
    cls.attributes.push({
      _type: "UMLAttribute",
      _id: nextId("ATTR"),
      _parent: ref(id),
      name: "说明",
      visibility: "public",
      isStatic: false,
      isLeaf: false,
      type: note,
      isReadOnly: true,
      isOrdered: false,
      isUnique: false,
      defaultValue: "",
    });
  }

  for (const attr of attributes) {
    cls.attributes.push({
      _type: "UMLAttribute",
      _id: nextId("ATTR"),
      _parent: ref(id),
      name: attr.name,
      visibility: "public",
      isStatic: false,
      isLeaf: false,
      type: attr.type,
      isReadOnly: false,
      isOrdered: false,
      isUnique: false,
      defaultValue: "",
    });
  }

  for (const op of operations) {
    cls.operations.push({
      _type: "UMLOperation",
      _id: nextId("OP"),
      _parent: ref(id),
      name: op.name,
      visibility: "public",
      isStatic: false,
      isLeaf: false,
      concurrency: "sequential",
      parameters: [],
      returnType: "",
    });
  }

  model.ownedElements.push(cls);
  return cls;
}

const defs = [
  {
    name: "AppConfig",
    note: "服务配置对象",
    doc: "中文描述：服务配置对象。职责：保存监听地址、端口、模型名称、缓存目录、日志目录、模型预热配置、ASR 配置和运行设备配置。",
    attrs: [
      ["host", "str"],
      ["port", "int"],
      ["model_name", "str"],
      ["device", "str"],
      ["dtype", "str"],
      ["cache_dir", "str"],
      ["log_dir", "str"],
      ["hf_cache_dir", "str"],
      ["request_timeout_sec", "float"],
      ["eager_load", "bool"],
      ["load_asr", "bool"],
      ["asr_model_name", "str"],
    ],
    ops: [],
  },
  {
    name: "TTSRequest",
    note: "语音生成请求结构",
    doc: "中文描述：语音生成请求结构。职责：表达 /tts 请求体，承载 auto、clone、design 三种模式参数，并保留参考音频、参考文本和说话人指令字段。",
    attrs: [
      ["text", "str"],
      ["mode", "str"],
      ["language_id", "str"],
      ["ref_audio", "str"],
      ["ref_text", "str"],
      ["instruct", "str"],
      ["duration", "float"],
      ["speed", "float"],
    ],
    ops: [],
  },
  {
    name: "HealthResponse",
    note: "健康检查响应结构",
    doc: "中文描述：健康检查响应结构。职责：表达 /health 响应，返回服务状态、模型名称和支持模式。",
    attrs: [
      ["ok", "bool"],
      ["service", "str"],
      ["status", "str"],
      ["model", "str"],
      ["supported_modes", "list[str]"],
    ],
    ops: [],
  },
  {
    name: "TTSResponse",
    note: "语音生成响应结构",
    doc: "中文描述：语音生成响应结构。职责：表达 /tts 响应，返回结果路径、采样率、耗时和错误信息。",
    attrs: [
      ["ok", "bool"],
      ["request_id", "str"],
      ["mode", "str"],
      ["wav_path", "str"],
      ["sample_rate", "int"],
      ["duration_ms", "int"],
      ["error_code", "str"],
      ["error_message", "str"],
    ],
    ops: [],
  },
  {
    name: "ServiceError",
    note: "服务业务异常对象",
    doc: "中文描述：服务业务异常对象。职责：统一承载错误码、错误信息和 HTTP 状态，为插件侧提供稳定失败语义。",
    attrs: [
      ["error_code", "str"],
      ["error_message", "str"],
      ["http_status", "int"],
    ],
    ops: [],
  },
  {
    name: "RequestValidator",
    note: "请求校验器",
    doc: "中文描述：请求校验器。职责：校验公共字段、模式字段、各模式必填项、速度时长和参考音频路径。",
    attrs: [],
    ops: [
      "validate_tts_request",
      "validate_mode",
      "validate_clone_fields",
      "validate_design_fields",
    ],
  },
  {
    name: "RequestIdGenerator",
    note: "请求编号生成器",
    doc: "中文描述：请求编号生成器。职责：生成唯一请求编号，用于缓存命名和日志追踪。",
    attrs: [],
    ops: ["next_id"],
  },
  {
    name: "WavCacheManager",
    note: "WAV 缓存管理器",
    doc: "中文描述：WAV 缓存管理器。职责：创建 cache 目录、生成路径、保存音频和检查输出文件。",
    attrs: [],
    ops: ["ensure_dirs", "build_wav_path", "save_audio", "exists"],
  },
  {
    name: "OmniVoiceEngine",
    note: "OmniVoice 模型引擎封装类",
    doc: "中文描述：OmniVoice 模型引擎封装类。职责：加载模型、检查模型状态、执行 generate、暴露 ASR 可用性，并在需要时转写参考音频。",
    attrs: [],
    ops: ["load_model", "is_ready", "generate", "can_transcribe_ref_audio", "transcribe_ref_audio"],
  },
  {
    name: "TTSService",
    note: "服务业务总控类",
    doc: "中文描述：服务业务总控类。职责：校验请求、生成请求编号、在 clone 模式下补齐参考文本、调用模型、保存音频并组织最终响应。",
    attrs: [],
    ops: ["health", "synthesize"],
  },
  {
    name: "ServiceLogger",
    note: "服务日志管理器",
    doc: "中文描述：服务日志管理器。职责：初始化日志、记录请求开始结束和异常信息。",
    attrs: [],
    ops: ["get_logger", "log_request_start", "log_request_finish", "log_exception"],
  },
  {
    name: "TTSController",
    note: "HTTP 控制器层",
    doc: "中文描述：HTTP 控制器层。职责：接收请求、调用服务层、返回统一响应。",
    attrs: [],
    ops: ["get_health", "post_tts"],
  },
  {
    name: "FastAPIAppFactory",
    note: "FastAPI 应用工厂",
    doc: "中文描述：FastAPI 应用工厂。职责：创建应用、注册健康检查和 TTS 路由，并在生命周期中完成目录初始化与可选预热。",
    attrs: [],
    ops: ["create_app", "register_routes"],
  },
  {
    name: "RunServer",
    note: "服务启动入口",
    doc: "中文描述：服务启动入口。职责：读取配置、构建运行依赖并启动 HTTP 服务。",
    attrs: [],
    ops: ["main", "build_runtime"],
  },
];

const classes = new Map();
for (const def of defs) {
  classes.set(
    def.name,
    classElement(
      def.name,
      def.doc,
      def.attrs.map(([name, type]) => ({ name, type })),
      def.ops.map((name) => ({ name })),
      def.note,
    ),
  );
}

const positions = {
  AppConfig: [40, 40],
  TTSRequest: [380, 40],
  HealthResponse: [760, 40],
  TTSResponse: [1120, 40],
  ServiceError: [1480, 40],
  RequestValidator: [40, 520],
  RequestIdGenerator: [380, 520],
  WavCacheManager: [760, 520],
  OmniVoiceEngine: [1120, 520],
  TTSService: [1480, 520],
  ServiceLogger: [40, 900],
  TTSController: [500, 900],
  FastAPIAppFactory: [960, 900],
  RunServer: [1420, 900],
};

function labelView(parentId, text, left, top, visible = true, width = null, font = "Arial;13;0", align = 0) {
  return {
    _type: "LabelView",
    _id: nextId("LABEL"),
    _parent: ref(parentId),
    visible,
    enabled: true,
    lineColor: "#000000",
    fillColor: "#ffffff",
    fontColor: "#000000",
    font,
    parentStyle: true,
    showShadow: true,
    containerChangeable: false,
    containerExtending: true,
    left,
    top,
    width: width ?? Math.max(60, text.length * 8),
    height: 13,
    autoResize: false,
    underline: false,
    text,
    horizontalAlignment: align,
    verticalAlignment: 5,
  };
}

function createClassView(cls, x, y, width = 300) {
  const classViewId = nextId("CLASSVIEW");
  const nameCompId = nextId("NAMECOMP");
  const attrCompId = nextId("ATTRCOMP");
  const opCompId = nextId("OPCOMP");

  const stereotype = labelView(nameCompId, "", x + 5, y + 5, false, 0, "Arial;13;0", 2);
  const name = labelView(nameCompId, cls.name, x + 10, y + 7, true, width - 20, "Arial;13;1", 2);
  const namespace = labelView(nameCompId, "(from Model)", x + 5, y + 5, false, 80, "Arial;13;0", 2);
  const property = labelView(nameCompId, "", x + 5, y + 5, false, 0, "Arial;13;0", 1);

  const nameComp = {
    _type: "UMLNameCompartmentView",
    _id: nameCompId,
    _parent: ref(classViewId),
    model: ref(cls._id),
    subViews: [stereotype, name, namespace, property],
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
    left: x,
    top: y,
    width,
    height: 25,
    autoResize: false,
    stereotypeLabel: ref(stereotype._id),
    nameLabel: ref(name._id),
    namespaceLabel: ref(namespace._id),
    propertyLabel: ref(property._id),
  };

  let attrTop = y + 30;
  const attrViews = [];
  for (const attr of cls.attributes) {
    attrViews.push({
      _type: "UMLAttributeView",
      _id: nextId("ATTRVIEW"),
      _parent: ref(attrCompId),
      model: ref(attr._id),
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
      left: x + 5,
      top: attrTop,
      width: width - 10,
      height: 13,
      autoResize: false,
      underline: false,
      text: `+${attr.name}: ${attr.type}`,
      horizontalAlignment: 0,
      verticalAlignment: 5,
    });
    attrTop += 15;
  }
  const attrHeight = Math.max(25, attrViews.length * 15 + 10);
  const attrComp = {
    _type: "UMLAttributeCompartmentView",
    _id: attrCompId,
    _parent: ref(classViewId),
    model: ref(cls._id),
    subViews: attrViews,
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
    left: x,
    top: y + 25,
    width,
    height: attrHeight,
    autoResize: false,
  };

  let opTop = y + 25 + attrHeight + 5;
  const opViews = [];
  for (const op of cls.operations) {
    opViews.push({
      _type: "UMLOperationView",
      _id: nextId("OPVIEW"),
      _parent: ref(opCompId),
      model: ref(op._id),
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
      left: x + 5,
      top: opTop,
      width: width - 10,
      height: 13,
      autoResize: false,
      underline: false,
      text: `+${op.name}()`,
      horizontalAlignment: 0,
      verticalAlignment: 5,
    });
    opTop += 15;
  }
  const opHeight = Math.max(25, opViews.length * 15 + 10);
  const opComp = {
    _type: "UMLOperationCompartmentView",
    _id: opCompId,
    _parent: ref(classViewId),
    model: ref(cls._id),
    subViews: opViews,
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
    left: x,
    top: y + 25 + attrHeight,
    width,
    height: opHeight,
    autoResize: false,
  };

  const totalHeight = 25 + attrHeight + opHeight;
  return {
    _type: "UMLClassView",
    _id: classViewId,
    _parent: ref(diagramId),
    model: ref(cls._id),
    subViews: [nameComp, attrComp, opComp],
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
    left: x,
    top: y,
    width,
    height: totalHeight,
    autoResize: false,
    selectable: true,
    movable: true,
    sizable: true,
    nameCompartment: ref(nameCompId),
    attributeCompartment: ref(attrCompId),
    operationCompartment: ref(opCompId),
  };
}

for (const [name, cls] of classes) {
  const [x, y] = positions[name];
  diagram.ownedViews.push(createClassView(cls, x, y));
}

function escapeNonAscii(str) {
  return str.replace(/[\u0080-\uFFFF]/g, (ch) => `\\u${ch.charCodeAt(0).toString(16).padStart(4, "0")}`);
}

const outPath = path.join(process.cwd(), "Doc", "OmniVoice_Service_Skeleton_Diagram.mdj");
const json = JSON.stringify(project, null, 2);
fs.writeFileSync(outPath, escapeNonAscii(json), "ascii");
console.log(outPath);
