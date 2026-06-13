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

下面这段不是伪代码，而是 UE 蓝图可复制的剪贴板文本。
前提是以下对象和变量已经存在：

- `BP_LocalTTS_DemoController`
- `WBP_LocalTTS_DemoPanel`
- `BP_LocalTTS_DemoController.DemoPanelClass`
- `BP_LocalTTS_DemoController.DemoPanel`
- `WBP_LocalTTS_DemoPanel.DemoController`

如果只想直接复制使用，也可以直接打开：

- [BP_LocalTTS_DemoController_BeginPlay_clipboard.txt](D:/project/UETTSProject/BP_LocalTTS_DemoController_BeginPlay_clipboard.txt)

```text
Begin Object Class=/Script/BlueprintGraph.K2Node_Event Name="K2Node_Event_0" ExportPath="/Script/BlueprintGraph.K2Node_Event'/Game/Blueprints/BP_LocalTTS_DemoController.BP_LocalTTS_DemoController:EventGraph.K2Node_Event_0'"
   EventReference=(MemberParent="/Script/CoreUObject.Class'/Script/Engine.Actor'",MemberName="ReceiveBeginPlay")
   bOverrideFunction=True
   NodePosX=160
   NodePosY=-80
   bCommentBubblePinned=True
   NodeGuid=82FDCA0043A4E6E01F75228C04F6D297
   CustomProperties Pin (PinId=F82217DB4FE740F32DB0229F47FE0698,PinName="OutputDelegate",Direction="EGPD_Output",PinType.PinCategory="delegate",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(MemberParent="/Script/CoreUObject.Class'/Script/Engine.Actor'",MemberName="ReceiveBeginPlay"),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=251282654C2EDA0BD55413B5CEAC1365,PinName="then",Direction="EGPD_Output",PinType.PinCategory="exec",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CreateWidget_0 36CB10394771B16E3AC774B31D06357D,),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
End Object
Begin Object Class=/Script/UMGEditor.K2Node_CreateWidget Name="K2Node_CreateWidget_0" ExportPath="/Script/UMGEditor.K2Node_CreateWidget'/Game/Blueprints/BP_LocalTTS_DemoController.BP_LocalTTS_DemoController:EventGraph.K2Node_CreateWidget_0'"
   NodePosX=496
   NodePosY=-16
   NodeGuid=D5F622854F09C29D6B4EB9971C041F37
   CustomProperties Pin (PinId=36CB10394771B16E3AC774B31D06357D,PinName="execute",PinType.PinCategory="exec",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_Event_0 251282654C2EDA0BD55413B5CEAC1365,),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=397318FD45C8EFAF9ADD0687BBAE7BF6,PinName="then",Direction="EGPD_Output",PinType.PinCategory="exec",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_VariableSet_0 246473D34DA010E33E735F9FB91E31FB,),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=84D9379046CB1D4D7E110FBC8816E284,PinName="Class",PinType.PinCategory="class",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/CoreUObject.Class'/Script/UMG.UserWidget'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_VariableGet_0 817065A1493437F63C3C5B9C5900F41A,),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=871455A04866282295B4B185704FDFFC,PinName="ReturnValue",Direction="EGPD_Output",PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/UMG.WidgetBlueprintGeneratedClass'/Game/UI/WBP_LocalTTS_DemoPanel.WBP_LocalTTS_DemoPanel_C'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_VariableSet_0 E503A871428DC85E38C009A022435603,K2Node_CallFunction_1 6829F89A4EAA2D259441C8B13D0AF7D6,K2Node_CallFunction_2 84198694461C7685A1B34FAF4A8B9756),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=72DE4314445F64364907C4844CB60586,PinName="OwningPlayer",PinToolTip="Player Controller Object Reference Owning Player\nThe player that 'owns' the widget.",PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/CoreUObject.Class'/Script/Engine.PlayerController'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CallFunction_0 E15D23B24F0D434FCDFB618AFA985408,),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
End Object
Begin Object Class=/Script/BlueprintGraph.K2Node_VariableGet Name="K2Node_VariableGet_0" ExportPath="/Script/BlueprintGraph.K2Node_VariableGet'/Game/Blueprints/BP_LocalTTS_DemoController.BP_LocalTTS_DemoController:EventGraph.K2Node_VariableGet_0'"
   VariableReference=(MemberName="DemoPanelClass",MemberGuid=60D2B23B4094D67A129964B7418148B7,bSelfContext=True)
   NodePosX=304
   NodePosY=16
   NodeGuid=EC607C7D4ECA4505A973CDBC62BDC5CC
   CustomProperties Pin (PinId=817065A1493437F63C3C5B9C5900F41A,PinName="DemoPanelClass",Direction="EGPD_Output",PinType.PinCategory="class",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/UMG.WidgetBlueprintGeneratedClass'/Game/UI/WBP_LocalTTS_DemoPanel.WBP_LocalTTS_DemoPanel_C'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CreateWidget_0 84D9379046CB1D4D7E110FBC8816E284,),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=80502BD54D46729229CE0D8BA25E95E6,PinName="self",PinFriendlyName=NSLOCTEXT("K2Node", "Target", "Target"),PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/Engine.BlueprintGeneratedClass'/Game/Blueprints/BP_LocalTTS_DemoController.BP_LocalTTS_DemoController_C'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,PersistentGuid=00000000000000000000000000000000,bHidden=True,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
End Object
Begin Object Class=/Script/BlueprintGraph.K2Node_CallFunction Name="K2Node_CallFunction_0" ExportPath="/Script/BlueprintGraph.K2Node_CallFunction'/Game/Blueprints/BP_LocalTTS_DemoController.BP_LocalTTS_DemoController:EventGraph.K2Node_CallFunction_0'"
   bDefaultsToPureFunc=True
   FunctionReference=(MemberParent="/Script/CoreUObject.Class'/Script/Engine.GameplayStatics'",MemberName="GetPlayerController")
   NodePosX=240
   NodePosY=64
   NodeGuid=CF12F0B7475DF8C4B784139DA5360AE4
   CustomProperties Pin (PinId=E70C7D2C442AB11C398BF68826CD9493,PinName="self",PinFriendlyName=NSLOCTEXT("K2Node", "Target", "Target"),PinToolTip="Target\nGameplay Statics Object Reference",PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/CoreUObject.Class'/Script/Engine.GameplayStatics'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,DefaultObject="/Script/Engine.Default__GameplayStatics",PersistentGuid=00000000000000000000000000000000,bHidden=True,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=339572BE4D72E9C7CFCC73ABA1058D7E,PinName="WorldContextObject",PinToolTip="World Context Object\nObject Reference",PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/CoreUObject.Class'/Script/CoreUObject.Object'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=True,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,PersistentGuid=00000000000000000000000000000000,bHidden=True,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=0DC94DA941F4025315B11AAA6200F939,PinName="PlayerIndex",PinToolTip="Player Index\nInteger\n\nIndex in the player controller list, starting first with local players and then available remote ones",PinType.PinCategory="int",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,DefaultValue="0",AutogeneratedDefaultValue="0",PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=E15D23B24F0D434FCDFB618AFA985408,PinName="ReturnValue",PinToolTip="Return Value\nPlayer Controller Object Reference",Direction="EGPD_Output",PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/CoreUObject.Class'/Script/Engine.PlayerController'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CreateWidget_0 72DE4314445F64364907C4844CB60586,K2Node_CallFunction_2 DAE57FF446796277E7AD7CB25310CE5C,K2Node_VariableSet_2 2E8BA00D4B03FB2A8D5821B603F95D0D),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
End Object
Begin Object Class=/Script/BlueprintGraph.K2Node_VariableSet Name="K2Node_VariableSet_0" ExportPath="/Script/BlueprintGraph.K2Node_VariableSet'/Game/Blueprints/BP_LocalTTS_DemoController.BP_LocalTTS_DemoController:EventGraph.K2Node_VariableSet_0'"
   VariableReference=(MemberName="DemoPanel",MemberGuid=462DDBB2463B16F14786BB911F897F56,bSelfContext=True)
   NodePosX=912
   NodeGuid=39BE54E54478DD741B6F21B5DB9715A0
   CustomProperties Pin (PinId=246473D34DA010E33E735F9FB91E31FB,PinName="execute",PinType.PinCategory="exec",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CreateWidget_0 397318FD45C8EFAF9ADD0687BBAE7BF6,),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=A9AC8AE44C07F7ABFCA9669F188859D0,PinName="then",Direction="EGPD_Output",PinType.PinCategory="exec",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_VariableSet_1 CA604EC548F0CA30ACE7638AF13913ED,),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=E503A871428DC85E38C009A022435603,PinName="DemoPanel",PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/UMG.WidgetBlueprintGeneratedClass'/Game/UI/WBP_LocalTTS_DemoPanel.WBP_LocalTTS_DemoPanel_C'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CreateWidget_0 871455A04866282295B4B185704FDFFC),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=317783864C439BB342830FB7D46829B4,PinName="Output_Get",PinToolTip="Retrieves the value of the variable, can use instead of a separate Get node",Direction="EGPD_Output",PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/UMG.WidgetBlueprintGeneratedClass'/Game/UI/WBP_LocalTTS_DemoPanel.WBP_LocalTTS_DemoPanel_C'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_VariableSet_1 C87D30FE43351F31FE688CB3F29E255B),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=E543F1994AE3A0679C94D3861941AC05,PinName="self",PinFriendlyName=NSLOCTEXT("K2Node", "Target", "Target"),PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/Engine.BlueprintGeneratedClass'/Game/Blueprints/BP_LocalTTS_DemoController.BP_LocalTTS_DemoController_C'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,PersistentGuid=00000000000000000000000000000000,bHidden=True,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
End Object
Begin Object Class=/Script/BlueprintGraph.K2Node_VariableSet Name="K2Node_VariableSet_1" ExportPath="/Script/BlueprintGraph.K2Node_VariableSet'/Game/Blueprints/BP_LocalTTS_DemoController.BP_LocalTTS_DemoController:EventGraph.K2Node_VariableSet_1'"
   VariableReference=(MemberParent="/Script/UMG.WidgetBlueprintGeneratedClass'/Game/UI/WBP_LocalTTS_DemoPanel.WBP_LocalTTS_DemoPanel_C'",MemberName="DemoController",MemberGuid=1F0B239846FC6CC860E9149F228A7AAB)
   SelfContextInfo=NotSelfContext
   NodePosX=1200
   NodeGuid=0DDB8CFB49C6DD7133C1C5912046310C
   CustomProperties Pin (PinId=CA604EC548F0CA30ACE7638AF13913ED,PinName="execute",PinType.PinCategory="exec",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_VariableSet_0 A9AC8AE44C07F7ABFCA9669F188859D0),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=D74836BA41D879E90417429C83488226,PinName="then",Direction="EGPD_Output",PinType.PinCategory="exec",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CallFunction_1 6DE5165F4A30A0A60CB8E69CF4A4D5E1),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=2154E53C4C41F95C1B146ABCE6E2ECDE,PinName="DemoController",PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/Engine.BlueprintGeneratedClass'/Game/Blueprints/BP_LocalTTS_DemoController.BP_LocalTTS_DemoController_C'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_Self_0 0DF635FE42BE45CBFC08C5A2DA51B8DB),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=C87D30FE43351F31FE688CB3F29E255B,PinName="self",PinFriendlyName=NSLOCTEXT("K2Node", "Target", "Target"),PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/UMG.WidgetBlueprintGeneratedClass'/Game/UI/WBP_LocalTTS_DemoPanel.WBP_LocalTTS_DemoPanel_C'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_VariableSet_0 317783864C439BB342830FB7D46829B4),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=5B00C54D4A7E4DC7A2CC5CBADBCFD0DB,PinName="Output_Get",PinToolTip="Retrieves the value of the variable, can use instead of a separate Get node",Direction="EGPD_Output",PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/Engine.BlueprintGeneratedClass'/Game/Blueprints/BP_LocalTTS_DemoController.BP_LocalTTS_DemoController_C'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
End Object
Begin Object Class=/Script/BlueprintGraph.K2Node_Self Name="K2Node_Self_0" ExportPath="/Script/BlueprintGraph.K2Node_Self'/Game/Blueprints/BP_LocalTTS_DemoController.BP_LocalTTS_DemoController:EventGraph.K2Node_Self_0'"
   NodePosX=992
   NodePosY=144
   NodeGuid=2A1D398D4D2D0CF529A643B09FA2E4AF
   CustomProperties Pin (PinId=0DF635FE42BE45CBFC08C5A2DA51B8DB,PinName="self",Direction="EGPD_Output",PinType.PinCategory="object",PinType.PinSubCategory="self",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_VariableSet_1 2154E53C4C41F95C1B146ABCE6E2ECDE),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
End Object
Begin Object Class=/Script/BlueprintGraph.K2Node_CallFunction Name="K2Node_CallFunction_1" ExportPath="/Script/BlueprintGraph.K2Node_CallFunction'/Game/Blueprints/BP_LocalTTS_DemoController.BP_LocalTTS_DemoController:EventGraph.K2Node_CallFunction_1'"
   FunctionReference=(MemberParent="/Script/CoreUObject.Class'/Script/UMG.UserWidget'",MemberName="AddToViewport")
   NodePosX=1472
   NodePosY=-16
   NodeGuid=2E3A5C8E4A0E84C784A19AAB8B1B1D7B
   CustomProperties Pin (PinId=6DE5165F4A30A0A60CB8E69CF4A4D5E1,PinName="execute",PinType.PinCategory="exec",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_VariableSet_1 D74836BA41D879E90417429C83488226),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=F230BD5E4707E7DB17E119986CB7A5CB,PinName="then",Direction="EGPD_Output",PinType.PinCategory="exec",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CallFunction_2 49B20D4E427815537A1B7D9D8B0F5BD0),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=6829F89A4EAA2D259441C8B13D0AF7D6,PinName="self",PinFriendlyName=NSLOCTEXT("K2Node", "Target", "Target"),PinToolTip="Target\nUser Widget Object Reference",PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/CoreUObject.Class'/Script/UMG.UserWidget'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CreateWidget_0 871455A04866282295B4B185704FDFFC),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=C464A16B4179A431F2E627A99663E3D5,PinName="ZOrder",PinToolTip="ZOrder\nInteger\n\nThe higher the number, the more on top this widget will be.",PinType.PinCategory="int",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,DefaultValue="0",AutogeneratedDefaultValue="0",PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
End Object
Begin Object Class=/Script/BlueprintGraph.K2Node_CallFunction Name="K2Node_CallFunction_2" ExportPath="/Script/BlueprintGraph.K2Node_CallFunction'/Game/Blueprints/BP_LocalTTS_DemoController.BP_LocalTTS_DemoController:EventGraph.K2Node_CallFunction_2'"
   FunctionReference=(MemberParent="/Script/CoreUObject.Class'/Script/UMG.WidgetBlueprintLibrary'",MemberName="SetInputMode_GameAndUIEx")
   NodePosX=1776
   NodePosY=-16
   NodeGuid=5B9D5A834EC7F6A3A0B16C8D7E4C76F2
   CustomProperties Pin (PinId=49B20D4E427815537A1B7D9D8B0F5BD0,PinName="execute",PinType.PinCategory="exec",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CallFunction_1 F230BD5E4707E7DB17E119986CB7A5CB),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=E34DE9D9434CF7E3D6E87EAA9328A61A,PinName="then",Direction="EGPD_Output",PinType.PinCategory="exec",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_VariableSet_2 92B3550F4877B79F56D1B0A733061511),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=CB5C576747D37C7DBF7EA9A6A228D0A8,PinName="self",PinFriendlyName=NSLOCTEXT("K2Node", "Target", "Target"),PinToolTip="Target\nWidget Blueprint Library Object Reference",PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/CoreUObject.Class'/Script/UMG.WidgetBlueprintLibrary'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,DefaultObject="/Script/UMG.Default__WidgetBlueprintLibrary",PersistentGuid=00000000000000000000000000000000,bHidden=True,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=DAE57FF446796277E7AD7CB25310CE5C,PinName="PlayerController",PinToolTip="Player Controller\nPlayer Controller Object Reference",PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/CoreUObject.Class'/Script/Engine.PlayerController'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CallFunction_0 E15D23B24F0D434FCDFB618AFA985408),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=84198694461C7685A1B34FAF4A8B9756,PinName="InWidgetToFocus",PinToolTip="In Widget To Focus\nWidget Object Reference",PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/CoreUObject.Class'/Script/UMG.Widget'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CreateWidget_0 871455A04866282295B4B185704FDFFC),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=8E3525E84FC3A8B7DCEB0A8A2B01BDBE,PinName="InMouseLockMode",PinToolTip="In Mouse Lock Mode\nEMouseLockMode",PinType.PinCategory="byte",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/CoreUObject.Enum'/Script/Engine.EMouseLockMode'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,DefaultValue="DoNotLock",AutogeneratedDefaultValue="DoNotLock",PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=9D1C8B0D48798A2B693E8588BFC62DF8,PinName="bHideCursorDuringCapture",PinToolTip="Hide Cursor During Capture\nBoolean",PinType.PinCategory="bool",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,DefaultValue="false",AutogeneratedDefaultValue="false",PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=08DDF61B4458E2A7B5A7C4A7A5260708,PinName="bFlushInput",PinToolTip="Flush Input\nBoolean",PinType.PinCategory="bool",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,DefaultValue="false",AutogeneratedDefaultValue="false",PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
End Object
Begin Object Class=/Script/BlueprintGraph.K2Node_VariableSet Name="K2Node_VariableSet_2" ExportPath="/Script/BlueprintGraph.K2Node_VariableSet'/Game/Blueprints/BP_LocalTTS_DemoController.BP_LocalTTS_DemoController:EventGraph.K2Node_VariableSet_2'"
   VariableReference=(MemberParent="/Script/CoreUObject.Class'/Script/Engine.PlayerController'",MemberName="bShowMouseCursor")
   SelfContextInfo=NotSelfContext
   NodePosX=2144
   NodePosY=112
   NodeGuid=57D341D24E39A1D8FE2F24BA0EFA50F8
   CustomProperties Pin (PinId=92B3550F4877B79F56D1B0A733061511,PinName="execute",PinType.PinCategory="exec",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CallFunction_2 E34DE9D9434CF7E3D6E87EAA9328A61A),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=7CA3FE49489EA2812F227C8BA0C14661,PinName="then",Direction="EGPD_Output",PinType.PinCategory="exec",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=793B36014F1D792A763B1BA5037A0E37,PinName="bShowMouseCursor",PinToolTip="Show Mouse Cursor\nBoolean",PinType.PinCategory="bool",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,DefaultValue="true",AutogeneratedDefaultValue="false",PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=2E8BA00D4B03FB2A8D5821B603F95D0D,PinName="self",PinFriendlyName=NSLOCTEXT("K2Node", "Target", "Target"),PinType.PinCategory="object",PinType.PinSubCategory="",PinType.PinSubCategoryObject="/Script/CoreUObject.Class'/Script/Engine.PlayerController'",PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CallFunction_0 E15D23B24F0D434FCDFB618AFA985408),PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
   CustomProperties Pin (PinId=0A910E4F4856C7D159C892A2EB22AA9C,PinName="Output_Get",PinToolTip="Retrieves the value of the variable, can use instead of a separate Get node",Direction="EGPD_Output",PinType.PinCategory="bool",PinType.PinSubCategory="",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,PersistentGuid=00000000000000000000000000000000,bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)
End Object
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

### 4.2 音色配置区

控件：

| 控件名 | 类型 | 推荐默认值 |
| --- | --- | --- |
| `Combo_VoiceMode` | ComboBoxString | `design` |
| `Input_Instruct` | EditableTextBox | `female, young adult, chinese accent, moderate pitch` |
| `Input_LanguageId` | EditableTextBox | `zh` |
| `Input_ReferenceAudioPath` | EditableTextBox | 空，仅 clone 模式使用 |
| `Input_ReferenceText` | MultiLineEditableTextBox | 空，仅 clone 模式使用 |
| `Spin_Speed` | SpinBox 或 Slider | `1.0` |
| `Spin_Duration` | SpinBox 或 Slider | `0.0` |
| `Btn_ApplyVoiceConfig` | Button | 应用音色配置 |
| `Txt_VoiceConfigState` | TextBlock | 当前音色配置状态 |

为什么测试 UI 需要这一块：

- `auto` 模式会更容易出现随机音色，不适合做稳定验收。
- `design` 模式配合固定 `Instruct` 可以把声音风格收窄到同一类角色。
- 长文本和单句应该共用同一套音色配置，避免同一段测试里声音跳变。

当前推荐先固定为：

```text
Mode = design
LanguageId = zh
Instruct = female, young adult, chinese accent, moderate pitch
Speed = 1.0
Duration = 0.0
```

注意：这里仍然不能保证每次采样完全相同，因为当前协议还没有 seed 字段。但它会比 `auto` 模式稳定得多。

### 4.3 单句语音区

控件：

| 控件名 | 类型 | 显示文字 |
| --- | --- | --- |
| `Input_SingleText` | MultiLineEditableTextBox | 单句或短文本输入 |
| `Btn_GenerateSingle` | Button | 仅生成 WAV |
| `Btn_PlayLastSingle` | Button | 播放最新 WAV |
| `Btn_PlayLastResponse` | Button | 播放最近响应 |
| `Btn_SpeakSingle` | Button | 生成并播放 |
| `Btn_StopSingle` | Button | 停止播放 |
| `Txt_SingleState` | TextBlock | 单句生成状态 |
| `Txt_PlaybackState` | TextBlock | 单句播放状态 |
| `Txt_SingleResult` | TextBlock | 单句结果 |

按钮定位：

- `Btn_GenerateSingle` 是新版主入口，用来验证“申请音频 / 生成 WAV”。
- `Btn_PlayLastSingle` 是新版主入口，用来验证“播放已有 WAV”，不再请求 TTS 服务。
- `Btn_PlayLastResponse` 用来验证 `FLocalTTSTTSResponse -> 播放` 链路，后续做历史列表时可复用。
- `Btn_SpeakSingle` 只作为兼容和快速冒烟入口保留，不建议作为复杂 UI 的唯一主流程。
- `Btn_StopSingle` 只停止 UE 播放层，不承诺取消服务端正在生成的请求。

### 4.4 长文本队列区

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

### 4.5 调试区

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
| `CurrentPlaybackState` | `ELocalTTSSpeakAsyncState` | `Idle` | 单句播放状态。播放最新 WAV、播放响应、播放语音事件时由 Widget 或后续 DemoController 包装函数更新。 |
| `CurrentQueueState` | `ELocalTTSLongTextQueueState` | `Idle` | 长文本队列状态。 |
| `CurrentSegmentIndex` | `Integer` | `-1` | 当前段序号。 |
| `TotalSegmentCount` | `Integer` | `0` | 总段数。 |
| `LastErrorText` | `String` | 空 | 最近错误。 |
| `LastWavPath` | `String` | 空 | 最近 wav。 |
| `LastRequestId` | `String` | 空 | 最近请求 ID。 |
| `LastPlaybackErrorText` | `String` | 空 | 最近播放错误。和生成错误分开显示，避免误判是服务端生成失败。 |

为什么必须保存 `LongTextQueue`：

- `创建 Local TTS 长文本队列` 返回的是一个 UObject。
- 如果只临时接线而不保存，蓝图里后续按钮可能找不到同一个队列对象。
- 保存变量后，暂停、继续、跳段、停止才会作用在同一个队列实例上。

如果使用 `ALocalTTSDemoController`：

- `LongTextQueue` 可以不放在 Widget 里，直接读取 `DemoController.LongTextQueue`。
- `CurrentSpeakState`、`CurrentQueueState`、`LastErrorText`、`LastWavPath`、`LastRequestId` 都可以直接绑定到 `DemoController` 对应字段。
- `CurrentPlaybackState` 当前建议先由 Widget 监听播放异步节点更新。后续如果 C++ 层给 `ALocalTTSDemoController` 增加播放包装函数，再统一迁移到 `DemoController`。
- Widget 的 `Event Construct` 只需要绑定 `Demo 状态已更新`，然后调用一个自定义 `RefreshFromDemoController` 函数刷新所有 TextBlock。

## 6. Widget 初始化接线

这部分内容实现位置是：

- 蓝图：`WBP_LocalTTS_DemoPanel`
- 图表：`Event Graph`

这里不是 `BP_LocalTTS_DemoController` 的图，也不是关卡蓝图。

`Event Construct` 的含义：

- 它是 `UserWidget` 自带的生命周期事件
- 触发时机是 Widget 被创建出来并开始构造时
- 在蓝图里它是一个事件节点，不是普通函数
- 常见用途就是做 UI 初始化、绑定按钮事件、绑定外部对象委托、首次刷新文本

因此第 6 节可以直接理解成：

- 打开 `WBP_LocalTTS_DemoPanel`
- 进入它的 `Event Graph`
- 放一个 `Event Construct` 节点
- 从这个节点开始接初始化逻辑

从这部分开始，当前文档里的内容优先是“接线说明”和“推荐结构”，不一定都是 UE 可直接粘贴的剪贴板文本。
原因是这里会依赖你当前 Widget 里已经实际存在的：

- TextBlock / Button 控件变量
- `DemoController` 变量
- 自定义函数 `RefreshFromDemoController`
- 你是否已经编译过蓝图

所以第 6 节先描述“应该放在哪里、逻辑应该怎么组织”，然后真正适合做成可粘贴文本的部分，再单独补充。

从这部分开始，推荐优先使用 `DemoController` 已经封装好的函数和状态字段。
原因是当前工程里 `ALocalTTSDemoController` 已经负责：

- 创建并持有 `LongTextQueue`
- 绑定单句异步节点回调
- 绑定长文本队列回调
- 维护服务、单句、长文本、错误、最近 WAV、最近请求 ID、事件日志

因此 Widget 更推荐做“显示层 + 按钮层”，而不是自己重新搭底层 `LocalTTS` 节点。

当前后续段落中的连线说明，统一按下面两条约定理解：

- 能直接从 `DemoController` 读取的状态，优先直接读 `DemoController` 字段
- 能直接通过 `DemoController` 调用的行为，优先调用 `DemoController` 函数

在 `WBP_LocalTTS_DemoPanel -> Event Graph` 中放置 `Event Construct`：

```text
Event Construct
-> Is Valid(DemoController)
-> Branch
   false:
     -> Set Txt_Error = "DemoController 未传入。请检查 BP_LocalTTS_DemoController.BeginPlay。"
   true:
     -> Bind Event to Demo 状态已更新
        Target = DemoController
        Event = CustomEvent CE_RefreshFromDemoController
     -> Call CE_RefreshFromDemoController
```

为什么这里不建议 Widget 自己创建队列：

- `ALocalTTSDemoController.BeginPlay()` 已经会创建并持有 `LongTextQueue`。
- `ALocalTTSDemoController` 已经绑定好了长文本队列回调。
- Widget 只监听 `Demo 状态已更新`，就能统一刷新界面，蓝图更短、更稳。

然后在同一个 `Event Graph` 中新增一个 `Custom Event`，名称建议为 `CE_RefreshFromDemoController`：

```text
Custom Event CE_RefreshFromDemoController
-> Is Valid(DemoController)
-> Branch
   false:
     -> Return
   true:
     -> Set Txt_ServiceState = DemoController.ServiceStateText
     -> Set Txt_SingleState = DemoController.SingleStateText
     -> Set Txt_LongState = DemoController.QueueStateText
     -> Set Txt_CurrentSegment = DemoController.CurrentSegmentText
     -> DemoController.GetLongTextProgressText
     -> Set Txt_LongProgress
     -> Set Txt_LastWavPath = DemoController.LastWavPath
     -> Set Txt_LastRequestId = DemoController.LastRequestId
     -> Set Txt_Error = DemoController.LastErrorMessage
     -> Set Txt_EventLog = DemoController.EventLogText
```

注意：

- 如果 Widget 会被反复创建销毁，重新 `Bind Event to Demo 状态已更新` 时要避免重复绑定。
- 如果你已经用 `Assign` 或确保只 Construct 一次，通常不会有问题。

## 7. 服务区蓝图接线

### 7.1 启动服务按钮

`Btn_StartService.OnClicked`：

```text
Btn_StartService.OnClicked
-> Is Valid(DemoController)
-> Branch
   false:
     -> Set Txt_Error = "DemoController 无效。"
   true:
     -> DemoController.Demo 启动服务
        ErrorMessage = Local String
     -> Branch(ReturnValue)
        true:
          -> Set Txt_Error = ""
          -> Call CE_RefreshFromDemoController
          -> Set Txt_ServiceDetail = "服务启动请求已发送，等待健康检查。"
        false:
          -> Set Txt_Error = ErrorMessage
```

实现位置：

- 蓝图：`WBP_LocalTTS_DemoPanel`
- 图表：`Event Graph`
- 入口节点：按钮事件 `Btn_StartService.OnClicked`

为什么这么连：

- `Demo 启动服务` 最终内部仍然只负责拉起服务进程，不代表模型已经 ready。
- 启动后仍需要用健康检查确认服务状态。
- 错误直接显示，方便测试安装路径、端口、Python 环境问题。
- 通过 `DemoController` 调用后，相关状态字段和日志也会统一更新。

### 7.2 应用音色配置

`Btn_ApplyVoiceConfig.OnClicked`：

```text
Btn_ApplyVoiceConfig.OnClicked
-> Is Valid(DemoController)
-> Branch
   false:
     -> Set Txt_Error = "DemoController 无效。"
   true:
     -> Combo_VoiceMode.GetSelectedOption
     -> Input_LanguageId.GetText -> ToString
     -> Input_Instruct.GetText -> ToString
     -> Input_ReferenceAudioPath.GetText -> ToString
     -> Input_ReferenceText.GetText -> ToString
     -> Spin_Speed.GetValue
     -> Spin_Duration.GetValue
     -> DemoController.Demo 应用音色配置
        Mode = Combo_VoiceMode
        LanguageId = Input_LanguageId
        Instruct = Input_Instruct
        ReferenceAudioPath = Input_ReferenceAudioPath
        ReferenceText = Input_ReferenceText
        Speed = Spin_Speed
        Duration = Spin_Duration
        ErrorMessage = Local String
     -> Branch(ReturnValue)
        true:
          -> Set Txt_Error = ""
          -> Set Txt_VoiceConfigState = "音色配置已应用"
          -> Call CE_RefreshFromDemoController
        false:
          -> Set Txt_Error = ErrorMessage
```

建议在 `Event Construct` 里也调用一次 `Demo 应用音色配置`，把 UI 默认值写入 `DemoController`。这样已有蓝图资产即使还保存着旧的 `auto` 模板，也会在 UI 构造时切到固定音色。

`Combo_VoiceMode` 推荐选项：

```text
design
auto
clone
```

测试阶段建议优先使用 `design`，不要默认使用 `auto`。

### 7.3 检查健康按钮

`Btn_CheckHealth.OnClicked`：

```text
Btn_CheckHealth.OnClicked
-> Is Valid(DemoController)
-> Branch
   false:
     -> Set Txt_Error = "DemoController 无效。"
   true:
     -> DemoController.Demo 检查健康
        ErrorMessage = Local String
     -> Branch(ReturnValue)
        true:
          -> Set Txt_Error = ""
          -> Call CE_RefreshFromDemoController
        false:
          -> Set Txt_Error = ErrorMessage
          -> Call CE_RefreshFromDemoController
```

实现位置：

- 蓝图：`WBP_LocalTTS_DemoPanel`
- 图表：`Event Graph`
- 入口节点：按钮事件 `Btn_CheckHealth.OnClicked`

为什么使用异步健康检查：

- `/health` 是 HTTP 请求，不应该阻塞 UI。
- `DemoController` 内部会把结果写回 `LastHealthResponse`、`ServiceStateText` 和日志。
- 成功不一定等于可播放，要看返回的 `Status` 是否为 `ready`。
- 失败可以区分服务未启动、端口不通、模型未加载等情况。

## 8. 单句语音区蓝图接线

### 8.0 推荐调用模型

现在插件已经把“申请音频”和“播放音频”拆开，Demo UI 也应该按两层组织：

```text
生成层：Text -> FLocalTTSTTSResponse / FLocalTTSSpeechEvent / WAV Path
播放层：WAV Path / FLocalTTSTTSResponse / FLocalTTSSpeechEvent -> UE AudioComponent
```

单句区推荐保留三条测试路径：

| 路径 | 按钮 | 调用入口 | 目的 |
| --- | --- | --- | --- |
| 新版主路径 A | `Btn_GenerateSingle` -> `Btn_PlayLastSingle` | `Demo 单句仅生成 WAV` -> `播放最新 Local TTS WAV` | 验证生成和播放完全分离。 |
| 新版主路径 B | `Btn_GenerateSingle` -> `Btn_PlayLastResponse` | `Demo 单句仅生成 WAV` -> `播放 Local TTS 响应` | 验证 Response 可被保存、传递、重播。 |
| 兼容路径 | `Btn_SpeakSingle` | `Demo 单句生成并播放` | 快速冒烟，确认旧式一键流程没有退化。 |

状态也拆成两组：

| 状态 | 推荐显示控件 | 来源 |
| --- | --- | --- |
| 生成状态 | `Txt_SingleState` | `DemoController.SingleStateText` |
| 播放状态 | `Txt_PlaybackState` | 播放异步节点 `OnStateChanged`，或后续 DemoController 播放包装字段 |
| 最近结果 | `Txt_SingleResult` / `Txt_LastWavPath` / `Txt_LastRequestId` | `DemoController.LastTTSResponse`、`LastWavPath`、`LastRequestId` |
| 错误 | `Txt_Error` | 生成错误和播放错误都显示，但文案要带阶段前缀 |

### 8.1 生成并播放兼容入口

`Btn_SpeakSingle.OnClicked` 保留为兼容入口和快速冒烟入口：

```text
Btn_SpeakSingle.OnClicked
-> Is Valid(DemoController)
-> Branch
   false:
     -> Set Txt_Error = "DemoController 无效。"
   true:
     -> Input_SingleText.GetText
     -> ToString
     -> DemoController.Demo 单句生成并播放
        Text = 上一步返回值
        ErrorMessage = Local String
     -> Branch(ReturnValue)
        true:
          -> Set Txt_Error = ""
          -> Set Txt_SingleState = "生成并播放请求已提交"
          -> Set Txt_PlaybackState = "等待音频就绪"
          -> Call CE_RefreshFromDemoController
        false:
          -> Set Txt_Error = "生成并播放失败：" + ErrorMessage
```

实现位置：

- 蓝图：`WBP_LocalTTS_DemoPanel`
- 图表：`Event Graph`
- 入口节点：按钮事件 `Btn_SpeakSingle.OnClicked`

状态刷新：

```text
Demo 状态已更新
-> Call CE_RefreshFromDemoController
-> DemoController.SingleState
-> 如果 State == AudioReady 或 Playing:
   -> Set Txt_PlaybackState = "自动播放中"
-> 如果 State == Finished:
   -> Set Txt_PlaybackState = "自动播放完成"
-> 如果 State == Error:
   -> Set Txt_PlaybackState = "自动播放错误"
-> Call UpdateSingleButtons
```

为什么优先监听 `状态变化`：

- 一个 `Demo 状态已更新` 回调就能驱动 UI 的“思考中 / 等待服务 / 生成中 / 播放中 / 完成 / 错误”。
- 不需要在 Widget 里重复绑定底层单句异步节点。
- 按钮是否禁用可以继续用插件提供的 `语音异步状态是否应禁用按钮`，但输入状态来自 `DemoController.SingleState`。
- 复杂 UI 不应只依赖这个按钮，因为它无法验证“生成后稍后播放、重播旧响应、播放历史项”等新能力。

为什么不只看 `生成成功`：

- `DemoController` 已经把底层“开始 / 成功 / 音频已就绪 / 完成 / 错误 / 状态变化”汇总到了统一状态字段里。
- UI 不必再单独绑定底层 `生成并播放 Local TTS` 的每个 delegate。

### 8.2 仅生成 WAV（新版主入口）

`Btn_GenerateSingle.OnClicked` 是新版单句主入口：

```text
Btn_GenerateSingle.OnClicked
-> Is Valid(DemoController)
-> Branch
   false:
     -> Set Txt_Error = "DemoController 无效。"
   true:
     -> Input_SingleText.GetText
     -> ToString
     -> DemoController.Demo 单句仅生成 WAV
        Text = 上一步返回值
        ErrorMessage = Local String
     -> Branch(ReturnValue)
        true:
          -> Set Txt_Error = ""
          -> Set Txt_SingleState = "生成请求已提交"
          -> Call CE_RefreshFromDemoController
        false:
          -> Set Txt_Error = "生成失败：" + ErrorMessage
          -> Call CE_RefreshFromDemoController
```

实现位置：

- 蓝图：`WBP_LocalTTS_DemoPanel`
- 图表：`Event Graph`
- 入口节点：按钮事件 `Btn_GenerateSingle.OnClicked`

结果显示：

```text
Demo 状态已更新
-> Call CE_RefreshFromDemoController
-> Set Txt_SingleState = DemoController.SingleStateText
-> Set Txt_SingleResult = DemoController.LastRequestId + " / " + DemoController.LastWavPath
-> Set Txt_LastRequestId = DemoController.LastRequestId
-> Set Txt_LastWavPath = DemoController.LastWavPath
```

为什么仅生成也监听 `语音事件已就绪`：

- 数字人和字幕更需要 `FLocalTTSSpeechEvent`，不只是 `FLocalTTSTTSResponse`。
- 当前这些数据已经会被 `DemoController` 汇总到 `LastSpeechEvent`、`LastWavPath`、`LastRequestId`。
- UI 在 `AudioReady` 或 `Finished` 后就可以启用播放按钮，播放不再依赖重新生成。

### 8.3 播放最新 WAV

`Btn_PlayLastSingle.OnClicked` 用来播放最近一次生成成功的 WAV，不请求 TTS 服务：

```text
Btn_PlayLastSingle.OnClicked
-> Branch(LastWavPath 是否为空)
   true:
     -> Set Txt_Error = "播放失败：还没有可播放的 WAV，请先点击 仅生成 WAV。"
   false:
     -> 播放最新 Local TTS WAV
        WorldContextObject = self
     -> OnStateChanged:
        -> Set CurrentPlaybackState = State
        -> Set Txt_PlaybackState = "播放：" + DetailMessage
        -> Call UpdateSingleButtons
     -> OnAudioReady:
        -> Set Txt_Error = ""
        -> Set Txt_PlaybackState = "播放中"
        -> Set Txt_LastWavPath = Response.WavPath
     -> OnFinished:
        -> Set CurrentPlaybackState = Finished
        -> Set Txt_PlaybackState = "播放完成"
        -> Call UpdateSingleButtons
     -> OnError:
        -> Set CurrentPlaybackState = Error
        -> Set LastPlaybackErrorText = ErrorMessage
        -> Set Txt_Error = "播放失败：" + ErrorMessage
        -> Set Txt_PlaybackState = "播放错误"
        -> Call UpdateSingleButtons
```

实现位置：

- 蓝图：`WBP_LocalTTS_DemoPanel`
- 图表：`Event Graph`
- 入口节点：按钮事件 `Btn_PlayLastSingle.OnClicked`
- 异步节点：`播放最新 Local TTS WAV`

注意：

- `播放最新 Local TTS WAV` 消费的是 `ULocalTTSSubsystem` 内保存的最近生成结果。
- 它不调用 `/tts`，所以不会让 TTS 生成请求进入 busy。
- 如果服务端 cache 中的 WAV 已被清理，播放会失败。这种失败属于播放层错误，不是生成层错误。

### 8.4 播放最近响应

`Btn_PlayLastResponse.OnClicked` 用来验证 `FLocalTTSTTSResponse` 可以作为独立播放输入。当前 UI 如果不做历史列表，可以先播放 `DemoController.LastTTSResponse`：

```text
Btn_PlayLastResponse.OnClicked
-> Is Valid(DemoController)
-> Branch
   false:
     -> Set Txt_Error = "DemoController 无效。"
   true:
     -> Branch(DemoController.LastTTSResponse.WavPath 是否为空)
        true:
          -> Set Txt_Error = "播放失败：最近响应没有 WAV 路径。"
        false:
          -> 播放 Local TTS 响应
             WorldContextObject = self
             Response = DemoController.LastTTSResponse
          -> OnStateChanged:
             -> Set CurrentPlaybackState = State
             -> Set Txt_PlaybackState = "播放响应：" + DetailMessage
             -> Call UpdateSingleButtons
          -> OnAudioReady:
             -> Set Txt_Error = ""
             -> Set Txt_PlaybackState = "响应播放中"
             -> Set Txt_LastWavPath = Response.WavPath
          -> OnFinished:
             -> Set CurrentPlaybackState = Finished
             -> Set Txt_PlaybackState = "响应播放完成"
             -> Call UpdateSingleButtons
          -> OnError:
             -> Set CurrentPlaybackState = Error
             -> Set Txt_Error = "播放响应失败：" + ErrorMessage
             -> Set Txt_PlaybackState = "播放错误"
             -> Call UpdateSingleButtons
```

如果后续 UI 做历史列表，推荐流程：

```text
获取 Local TTS 语音历史
-> For Each Response 创建一行 WBP_LocalTTS_HistoryRow
-> Row 显示 RequestId / WavPath / DurationMs
-> Row.Btn_Play.OnClicked
-> 播放 Local TTS 响应
   Response = 当前行保存的 Response
```

为什么要测试 `播放响应`，而不只测试 `播放最新 WAV`：

- `播放最新 WAV` 只能验证最近一次结果。
- `播放 Local TTS 响应` 可以验证历史列表、缓存列表、数字人任务列表里的任意一条声音。
- 后续如果要做“选中某句重播”，应该走 Response 或 SpeechEvent，而不是总读 Last。

### 8.5 停止播放

`Btn_StopSingle.OnClicked`：

```text
Btn_StopSingle.OnClicked
-> Is Valid(DemoController)
-> Branch
   false:
     -> Set Txt_Error = "DemoController 无效。"
   true:
     -> DemoController.Demo 停止单句播放
     -> Set CurrentPlaybackState = Finished
     -> Set Txt_PlaybackState = "已请求停止播放"
     -> Call CE_RefreshFromDemoController
     -> Call UpdateSingleButtons
```

实现位置：

- 蓝图：`WBP_LocalTTS_DemoPanel`
- 图表：`Event Graph`
- 入口节点：按钮事件 `Btn_StopSingle.OnClicked`

为什么停止只影响播放：

- 单次 TTS 的服务端生成请求目前不能立即取消。
- 如果音频已经在 UE 播放，停止会释放当前 AudioComponent。
- 如果正在生成但还没开始播放，停止不会让服务端 `/tts` 立即中断，只能影响后续播放层。

### 8.6 单句按钮启用规则

建议新增一个 Widget 自定义函数 `UpdateSingleButtons`，由 `CE_RefreshFromDemoController` 和播放异步节点回调共同调用。

输入状态：

```text
GenerateState = DemoController.SingleState
PlaybackState = CurrentPlaybackState
HasLastWav = DemoController.LastWavPath 非空
```

推荐规则：

| 条件 | 生成 WAV | 播放最新 | 播放响应 | 生成并播放 | 停止播放 |
| --- | --- | --- | --- | --- | --- |
| 空闲且没有 WAV | 可用 | 禁用 | 禁用 | 可用 | 禁用 |
| 生成中 | 禁用 | 如果已有 WAV 可用 | 如果已有 Response 可用 | 禁用 | 播放中才可用 |
| 生成完成且有 WAV | 可用 | 可用 | 可用 | 可用 | 禁用 |
| 播放中 | 可用，除非插件 busy | 禁用 | 禁用 | 禁用 | 可用 |
| 播放错误 | 可用 | 有 WAV 则可用 | 有 Response 则可用 | 可用 | 禁用 |
| 生成错误 | 可用 | 有旧 WAV 则可用 | 有旧 Response 则可用 | 可用 | 禁用 |

蓝图实现可以简化为：

```text
UpdateSingleButtons
-> GenerateBusy = 语音异步状态是否应禁用按钮(DemoController.SingleState)
-> PlaybackBusy = CurrentPlaybackState == Playing 或 CurrentPlaybackState == Started 或 CurrentPlaybackState == AudioReady
-> HasLastWav = DemoController.LastWavPath != ""
-> Set Btn_GenerateSingle IsEnabled = NOT GenerateBusy
-> Set Btn_SpeakSingle IsEnabled = NOT GenerateBusy AND NOT PlaybackBusy
-> Set Btn_PlayLastSingle IsEnabled = HasLastWav AND NOT PlaybackBusy
-> Set Btn_PlayLastResponse IsEnabled = HasLastWav AND NOT PlaybackBusy
-> Set Btn_StopSingle IsEnabled = PlaybackBusy
```

注意：

- 是否允许“播放中继续生成下一条”取决于测试目标。插件层播放和生成已经拆开，理论上播放中可以提交下一次生成；如果人工验收想降低变量，可以临时在 UI 层禁用生成按钮。
- 服务端同一时间仍只建议一个 TTS 生成请求。若插件返回 busy，UI 显示生成层错误即可。
- 播放按钮不应因为服务端生成 busy 而禁用，因为播放已有 WAV 不需要服务端。

### 8.7 单句区最小验收步骤

更新 UI 后，建议按下面顺序人工测试：

```text
1. 点击 启动服务。
2. 点击 检查健康，确认服务 ready。
3. 点击 仅生成 WAV。
4. 观察 Txt_SingleState 进入生成中，随后进入 AudioReady / Finished。
5. 确认 Txt_LastRequestId 和 Txt_LastWavPath 有值。
6. 点击 播放最新 WAV，确认不会再次请求 /tts，只触发播放状态。
7. 点击 停止播放，确认播放停止，生成结果仍保留。
8. 点击 播放最近响应，确认同一个 WAV 可以重播。
9. 点击 生成并播放，确认兼容路径仍可用。
10. 播放中再次点击 仅生成 WAV，按当前 UI 策略验证允许或禁用是否符合预期。
```

通过标准：

- `仅生成 WAV` 成功后，即使不播放，也能看到 RequestId 和 WavPath。
- `播放最新 WAV` 不应改变 RequestId，不应触发新的生成请求。
- `播放最近响应` 可以复用最近一次生成结果。
- `停止播放` 不清空最近生成结果。
- 生成错误和播放错误在 UI 文案上能区分。

## 9. 长文本队列区蓝图接线

### 9.1 开始长文本播放

`Btn_StartLongSpeak.OnClicked`：

```text
OnClicked
-> Branch(DemoController Is Valid)
   false:
     -> Set Txt_Error = "DemoController 无效。"
   true:
     -> Input_LongText.GetText
     -> ToString
     -> DemoController.Demo 长文本生成并播放
        Text = 上一步返回值
        ErrorMessage = Local String
     -> Branch(ReturnValue)
        true:
          -> Set Txt_Error = ""
          -> Call RefreshFromDemoController
          -> Set Txt_LongState = "长文本队列已启动"
        false:
          -> Set Txt_Error = ErrorMessage
```

为什么模板请求里的 `Text` 可以为空：

- `DemoController` 内部会基于 `LongTextRequestTemplate` 生成实际请求。
- UI 不必自己 `Make FLocalTTSLongTextRequest`，除非你想在 Widget 上暴露所有高级参数。

为什么 `每段最大字符数` 建议先用 80：

- 太长会导致单段推理慢、字幕和暂停控制不细。
- 太短会生成很多 wav，听感碎。
- 80 适合当前阶段先做人工验证，后续再按实际声音效果调整。

### 9.2 开始长文本仅生成

`Btn_StartLongGenerate.OnClicked` 和播放版基本相同，只是最后调用：

```text
DemoController.Demo 长文本仅生成
```

为什么需要仅生成：

- 用来验证长文本分段和每段语音事件输出。
- 不自动播放，适合数字人、字幕、批量生成、后处理测试。

### 9.3 暂停

`Btn_PauseLong.OnClicked`：

```text
OnClicked
-> Branch(DemoController Is Valid)
   false:
     -> Set Txt_Error = "DemoController 无效。"
   true:
     -> DemoController.Demo 暂停长文本
        ErrorMessage = Local String
     -> Branch(ReturnValue)
        true:
          -> Call RefreshFromDemoController
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
-> Branch(DemoController Is Valid)
   false:
     -> Set Txt_Error = "DemoController 无效。"
   true:
     -> DemoController.Demo 继续长文本
        ErrorMessage = Local String
     -> Branch(ReturnValue)
        true:
          -> Call RefreshFromDemoController
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
-> Branch(DemoController Is Valid)
   false:
     -> Set Txt_Error = "DemoController 无效。"
   true:
     -> DemoController.Demo 跳到下一段
        ErrorMessage = Local String
     -> Branch(ReturnValue)
        true:
          -> Call RefreshFromDemoController
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
-> Branch(DemoController Is Valid)
   false:
     -> Set Txt_Error = "DemoController 无效。"
   true:
     -> DemoController.Demo 停止长文本
     -> Call RefreshFromDemoController
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
Demo 状态已更新
-> Call RefreshFromDemoController
-> DemoController.QueueState
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
Demo 状态已更新
-> Call RefreshFromDemoController
-> Set Txt_CurrentSegment = DemoController.CurrentSegmentText
-> Set Txt_LongProgress = DemoController.GetLongTextProgressText()
```

为什么在这里更新当前段文本：

- 段落开始代表队列已经决定处理哪一段。
- UI 可以提前显示字幕文本，即使音频还在生成。

### 10.3 段落已生成

事件参数：

- `SegmentSpeechEvent`

接线：

```text
Demo 状态已更新
-> Call RefreshFromDemoController
-> Set Txt_LastRequestId = DemoController.LastRequestId
-> Set Txt_LastWavPath = DemoController.LastWavPath
-> Set Txt_EventLog = DemoController.EventLogText
```

为什么在这里更新 request_id 和 wav：

- 这是服务端已返回该段音频结果的时刻。
- 字幕、数字人、调试日志都应该优先从这个事件拿数据。

### 10.4 段落完成

事件参数：

- `SegmentSpeechEvent`

接线：

```text
Demo 状态已更新
-> Call RefreshFromDemoController
-> Set Txt_LongProgress = DemoController.GetLongTextProgressText()
-> Set Txt_EventLog = DemoController.EventLogText
```

为什么 `段落已生成` 和 `段落完成` 要分开：

- 自动播放模式下，已生成只代表 wav 准备好了，完成代表这段播放结束。
- 仅生成模式下，生成完成和段落完成会更接近，但仍建议保留两个 UI 信号。

### 10.5 队列完成

接线：

```text
Demo 状态已更新
-> Call RefreshFromDemoController
-> Set Btn_StartLongSpeak IsEnabled = true
-> Set Btn_StartLongGenerate IsEnabled = true
```

### 10.6 队列停止

接线：

```text
Demo 状态已更新
-> Call RefreshFromDemoController
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
Demo 状态已更新
-> Call RefreshFromDemoController
-> Set Txt_Error = DemoController.LastErrorMessage
-> Set Txt_CurrentSegment = DemoController.CurrentSegmentText
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

- 当前 `ALocalTTSDemoController` 已经封装了单句生成和长文本队列，但还没有单独封装 `PlayLastSingle` / `PlayResponse` 这类播放包装函数。下一版 UI 可以先直接在 Widget 里调用 `ULocalTTSPlayWavAsyncAction` 系列节点。
- 单句区现在建议拆成生成状态和播放状态。`DemoController.SingleStateText` 只能完整描述生成并播放兼容流程，不能单独表达“播放最新 WAV”的全部状态。
- `播放最新 Local TTS WAV` 依赖运行期最近一次成功生成结果。重启 PIE 后历史会清空。
- 已生成 WAV 当前主要位于服务端 cache 目录。cache 被清理后，历史 Response 里即使还有路径，也可能播放失败。
- 长文本队列目前是 UObject 队列对象，不是专门的 `BlueprintAsyncActionBase` 节点，所以 UI 必须保存 `LongTextQueue` 变量。
- 生成中暂停不会立即暂停服务端推理，只会在当前段生成完成后停住。
- 生成中跳段和停止属于软控制，当前请求返回后会被队列忽略。
- 当前没有真实口型和表情驱动，`FLocalTTSSpeechEvent` 里的口型帧和表情帧仍是预留字段。
- Demo UI 不应该替代正式业务 UI，它只是插件能力验收和展示入口。

## 13. 后续可升级方向

当前 Demo 稳定后，再考虑：

- 在 `ALocalTTSDemoController` 增加 `PlayLastSingle`、`PlayLastResponse`、`PlayLastSpeechEvent` 包装函数，把播放状态也汇总进 `Demo 状态已更新`，让 Widget 不再直接绑定播放异步节点。
- 新增 `PlaybackStateText`、`LastPlaybackErrorMessage`，和 `SingleStateText`、`LastErrorMessage` 分开。
- 新增 `WBP_LocalTTS_SegmentRow`，用列表显示所有分段。
- 新增 `WBP_LocalTTS_HistoryRow`，用 `获取 Local TTS 语音历史` 显示最近 20 条 Response，并支持点击重播。
- 增加 3D Actor 播放测试，把声音绑定到 `TTS_Speaker`。
- 增加“保存到语音库”能力，把重要 WAV 从 cache 复制到长期目录，例如 `Saved/LocalTTS/Library`。
- 新增长文本专用异步蓝图节点，减少 UI 手动保存队列对象的步骤。
- 在关卡里加入字幕条，直接显示 `段落开始` 的文本。
- 后续数字人接入时，让 `段落已生成` 驱动口型分析入口。
