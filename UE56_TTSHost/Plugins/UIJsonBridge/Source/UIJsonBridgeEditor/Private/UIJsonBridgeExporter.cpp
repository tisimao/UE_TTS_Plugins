#include "UIJsonBridgeExporter.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphUtilities.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/PropertyPortFlags.h"
#include "WidgetBlueprint.h"
#include "Animation/WidgetAnimation.h"
#include "WidgetBlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "UIJsonBridgeExporter"

namespace UIJsonBridge
{
	static FString GetExportProfileName(EUIJsonBridgeExportProfile Profile)
	{
		switch (Profile)
		{
		case EUIJsonBridgeExportProfile::LayoutOnly:
			return TEXT("layout");
		case EUIJsonBridgeExportProfile::Interaction:
			return TEXT("interaction");
		case EUIJsonBridgeExportProfile::Full:
		default:
			return TEXT("full");
		}
	}

	static bool ShouldExportWidgetProperties(EUIJsonBridgeExportProfile Profile)
	{
		return Profile != EUIJsonBridgeExportProfile::Interaction;
	}

	static bool ShouldExportBindings(EUIJsonBridgeExportProfile Profile)
	{
		return Profile == EUIJsonBridgeExportProfile::Interaction || Profile == EUIJsonBridgeExportProfile::Full;
	}

	static bool ShouldExportAnimations(EUIJsonBridgeExportProfile Profile)
	{
		return Profile == EUIJsonBridgeExportProfile::Interaction || Profile == EUIJsonBridgeExportProfile::Full;
	}

	static bool ShouldExportGraphs(EUIJsonBridgeExportProfile Profile)
	{
		return Profile == EUIJsonBridgeExportProfile::Interaction || Profile == EUIJsonBridgeExportProfile::Full;
	}

	static bool ShouldExportGraphPins(EUIJsonBridgeExportProfile Profile)
	{
		return Profile == EUIJsonBridgeExportProfile::Full;
	}

	static bool ShouldExportGraphClipboardText(EUIJsonBridgeExportProfile Profile)
	{
		return Profile == EUIJsonBridgeExportProfile::Full;
	}

	static TSharedRef<FJsonObject> MakeVector2DObject(const FVector2D& Value)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetNumberField(TEXT("x"), Value.X);
		Json->SetNumberField(TEXT("y"), Value.Y);
		return Json;
	}

	static TSharedRef<FJsonObject> MakeMarginObject(const FMargin& Value)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetNumberField(TEXT("left"), Value.Left);
		Json->SetNumberField(TEXT("top"), Value.Top);
		Json->SetNumberField(TEXT("right"), Value.Right);
		Json->SetNumberField(TEXT("bottom"), Value.Bottom);
		return Json;
	}

	static TSharedRef<FJsonObject> MakeAnchorsObject(const FAnchors& Value)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetObjectField(TEXT("minimum"), MakeVector2DObject(Value.Minimum));
		Json->SetObjectField(TEXT("maximum"), MakeVector2DObject(Value.Maximum));
		return Json;
	}

	static FString GetClassPath(const UClass* Class)
	{
		return Class ? Class->GetPathName() : FString();
	}

	static FString GetObjectPath(const UObject* Object)
	{
		return Object ? Object->GetPathName() : FString();
	}

	static bool ShouldSkipProperty(const FProperty* Property)
	{
		if (!Property)
		{
			return true;
		}

		const bool bEditorVisible =
			Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible | CPF_AssetRegistrySearchable);
		const bool bUnsafeForExport =
			Property->HasAnyPropertyFlags(CPF_Transient | CPF_DuplicateTransient | CPF_NonPIEDuplicateTransient);

		return !bEditorVisible || bUnsafeForExport;
	}

	static FString ExportPropertyText(const FProperty* Property, const void* Container)
	{
		FString ValueText;
		const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Container);
		Property->ExportTextItem_Direct(ValueText, ValuePtr, nullptr, nullptr, PPF_None);
		return ValueText;
	}

	static TSharedRef<FJsonObject> ExportPropertyBag(const UObject* Object)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		if (!Object)
		{
			return Json;
		}

		TMap<FName, FString> ExportedProperties;
		FWidgetBlueprintEditorUtils::ExportPropertiesToText(const_cast<UObject*>(Object), ExportedProperties);
		for (const TPair<FName, FString>& Property : ExportedProperties)
		{
			Json->SetStringField(Property.Key.ToString(), Property.Value);
		}

		return Json;
	}

	static TSharedRef<FJsonObject> ExportSlot(const UPanelSlot* Slot, EUIJsonBridgeExportProfile Profile)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		if (!Slot)
		{
			return Json;
		}

		Json->SetStringField(TEXT("class"), Slot->GetClass()->GetName());
		Json->SetStringField(TEXT("classPath"), GetClassPath(Slot->GetClass()));
		if (ShouldExportWidgetProperties(Profile))
		{
			Json->SetObjectField(TEXT("properties"), ExportPropertyBag(Slot));
		}

		if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
		{
			const FAnchorData Layout = CanvasSlot->GetLayout();
			TSharedRef<FJsonObject> Canvas = MakeShared<FJsonObject>();
			Canvas->SetObjectField(TEXT("offsets"), MakeMarginObject(Layout.Offsets));
			Canvas->SetObjectField(TEXT("anchors"), MakeAnchorsObject(Layout.Anchors));
			Canvas->SetObjectField(TEXT("alignment"), MakeVector2DObject(Layout.Alignment));
			Canvas->SetBoolField(TEXT("autoSize"), CanvasSlot->GetAutoSize());
			Canvas->SetNumberField(TEXT("zOrder"), CanvasSlot->GetZOrder());
			Json->SetObjectField(TEXT("canvas"), Canvas);
		}

		return Json;
	}

	static TSharedRef<FJsonObject> ExportWidget(UWidget* Widget, EUIJsonBridgeExportProfile Profile)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		if (!Widget)
		{
			return Json;
		}

		Json->SetStringField(TEXT("name"), Widget->GetName());
		Json->SetStringField(TEXT("displayLabel"), Widget->GetLabelText().ToString());
		Json->SetStringField(TEXT("class"), Widget->GetClass()->GetName());
		Json->SetStringField(TEXT("classPath"), GetClassPath(Widget->GetClass()));
		Json->SetStringField(TEXT("objectPath"), GetObjectPath(Widget));
		Json->SetBoolField(TEXT("isVariable"), Widget->bIsVariable != 0);
		Json->SetStringField(TEXT("visibility"), StaticEnum<ESlateVisibility>()->GetNameStringByValue(static_cast<int64>(Widget->GetVisibility())));
		Json->SetObjectField(TEXT("slot"), ExportSlot(Widget->Slot, Profile));
		if (ShouldExportWidgetProperties(Profile))
		{
			Json->SetObjectField(TEXT("properties"), ExportPropertyBag(Widget));
		}

		TArray<TSharedPtr<FJsonValue>> Children;
		if (const UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
		{
			const int32 ChildCount = PanelWidget->GetChildrenCount();
			for (int32 Index = 0; Index < ChildCount; ++Index)
			{
				if (UWidget* Child = PanelWidget->GetChildAt(Index))
				{
					Children.Add(MakeShared<FJsonValueObject>(ExportWidget(Child, Profile)));
				}
			}
		}
		Json->SetArrayField(TEXT("children"), Children);

		return Json;
	}

	static TSharedRef<FJsonObject> ExportBindings(const UWidgetBlueprint* WidgetBlueprint)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();

		TArray<TSharedPtr<FJsonValue>> Bindings;
		for (const FDelegateEditorBinding& Binding : WidgetBlueprint->Bindings)
		{
			TSharedRef<FJsonObject> BindingJson = MakeShared<FJsonObject>();
			BindingJson->SetStringField(TEXT("objectName"), Binding.ObjectName);
			BindingJson->SetStringField(TEXT("propertyName"), Binding.PropertyName.ToString());
			BindingJson->SetStringField(TEXT("functionName"), Binding.FunctionName.ToString());
			BindingJson->SetStringField(TEXT("sourceProperty"), Binding.SourceProperty.ToString());
			BindingJson->SetStringField(TEXT("memberGuid"), Binding.MemberGuid.ToString(EGuidFormats::DigitsWithHyphens));
			BindingJson->SetStringField(TEXT("kind"), StaticEnum<EBindingKind>()->GetNameStringByValue(static_cast<int64>(Binding.Kind)));
			Bindings.Add(MakeShared<FJsonValueObject>(BindingJson));
		}

		Json->SetArrayField(TEXT("items"), Bindings);
		Json->SetNumberField(TEXT("count"), Bindings.Num());
		return Json;
	}

	static TArray<TSharedPtr<FJsonValue>> ExportAnimations(const UWidgetBlueprint* WidgetBlueprint)
	{
		TArray<TSharedPtr<FJsonValue>> Animations;
		for (const UWidgetAnimation* Animation : WidgetBlueprint->Animations)
		{
			if (!Animation)
			{
				continue;
			}

			TSharedRef<FJsonObject> AnimationJson = MakeShared<FJsonObject>();
			AnimationJson->SetStringField(TEXT("name"), Animation->GetName());
			AnimationJson->SetStringField(TEXT("objectPath"), Animation->GetPathName());
			Animations.Add(MakeShared<FJsonValueObject>(AnimationJson));
		}
		return Animations;
	}

	static TSharedRef<FJsonObject> ExportPinType(const FEdGraphPinType& PinType)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("category"), PinType.PinCategory.ToString());
		Json->SetStringField(TEXT("subCategory"), PinType.PinSubCategory.ToString());
		Json->SetStringField(TEXT("subCategoryObject"), PinType.PinSubCategoryObject.IsValid() ? PinType.PinSubCategoryObject->GetPathName() : FString());
		Json->SetStringField(TEXT("containerType"), StaticEnum<EPinContainerType>()->GetNameStringByValue(static_cast<int64>(PinType.ContainerType)));
		Json->SetBoolField(TEXT("isReference"), PinType.bIsReference != 0);
		Json->SetBoolField(TEXT("isConst"), PinType.bIsConst != 0);
		Json->SetBoolField(TEXT("isWeakPointer"), PinType.bIsWeakPointer != 0);
		Json->SetBoolField(TEXT("isUObjectWrapper"), PinType.bIsUObjectWrapper != 0);
		return Json;
	}

	static TSharedRef<FJsonObject> ExportFunctionParameter(const UEdGraphPin* Pin)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		if (!Pin)
		{
			return Json;
		}

		Json->SetStringField(TEXT("name"), Pin->PinName.ToString());
		Json->SetStringField(TEXT("displayName"), Pin->GetDisplayName().ToString());
		Json->SetObjectField(TEXT("type"), ExportPinType(Pin->PinType));
		Json->SetStringField(TEXT("defaultValue"), Pin->DefaultValue);
		Json->SetStringField(TEXT("defaultObject"), Pin->DefaultObject ? Pin->DefaultObject->GetPathName() : FString());
		Json->SetStringField(TEXT("defaultText"), Pin->DefaultTextValue.ToString());
		return Json;
	}

	static TSharedRef<FJsonObject> ExportPin(const UEdGraphPin* Pin)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		if (!Pin)
		{
			return Json;
		}

		Json->SetStringField(TEXT("id"), Pin->PinId.ToString(EGuidFormats::DigitsWithHyphens));
		Json->SetStringField(TEXT("name"), Pin->PinName.ToString());
		Json->SetStringField(TEXT("displayName"), Pin->GetDisplayName().ToString());
		Json->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
		Json->SetObjectField(TEXT("type"), ExportPinType(Pin->PinType));
		Json->SetStringField(TEXT("defaultValue"), Pin->DefaultValue);
		Json->SetStringField(TEXT("defaultAsString"), Pin->GetDefaultAsString());
		Json->SetStringField(TEXT("defaultObject"), Pin->DefaultObject ? Pin->DefaultObject->GetPathName() : FString());
		Json->SetStringField(TEXT("defaultText"), Pin->DefaultTextValue.ToString());
		Json->SetBoolField(TEXT("hidden"), Pin->bHidden != 0);
		Json->SetBoolField(TEXT("notConnectable"), Pin->bNotConnectable != 0);
		Json->SetBoolField(TEXT("orphaned"), Pin->bOrphanedPin != 0);

		TArray<TSharedPtr<FJsonValue>> Links;
		for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
		{
			if (!LinkedPin)
			{
				continue;
			}

			const UEdGraphNode* LinkedNode = LinkedPin->GetOwningNodeUnchecked();
			TSharedRef<FJsonObject> LinkJson = MakeShared<FJsonObject>();
			LinkJson->SetStringField(TEXT("nodeGuid"), LinkedNode ? LinkedNode->NodeGuid.ToString(EGuidFormats::DigitsWithHyphens) : FString());
			LinkJson->SetStringField(TEXT("nodeName"), LinkedNode ? LinkedNode->GetName() : FString());
			LinkJson->SetStringField(TEXT("pinId"), LinkedPin->PinId.ToString(EGuidFormats::DigitsWithHyphens));
			LinkJson->SetStringField(TEXT("pinName"), LinkedPin->PinName.ToString());
			Links.Add(MakeShared<FJsonValueObject>(LinkJson));
		}
		Json->SetArrayField(TEXT("links"), Links);

		return Json;
	}

	static TSharedRef<FJsonObject> ExportNodeMetadata(const UEdGraphNode* Node)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		if (!Node)
		{
			return Json;
		}

		if (const UK2Node_ComponentBoundEvent* ComponentEvent = Cast<UK2Node_ComponentBoundEvent>(Node))
		{
			Json->SetStringField(TEXT("eventKind"), TEXT("componentBoundEvent"));
			Json->SetStringField(TEXT("componentName"), ComponentEvent->GetComponentPropertyName().ToString());
			Json->SetStringField(TEXT("delegateName"), ComponentEvent->DelegatePropertyName.ToString());
			Json->SetStringField(TEXT("delegateOwnerClass"), ComponentEvent->DelegateOwnerClass ? ComponentEvent->DelegateOwnerClass->GetPathName() : FString());
			Json->SetStringField(TEXT("customFunctionName"), ComponentEvent->CustomFunctionName.ToString());
		}
		else if (const UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
		{
			Json->SetStringField(TEXT("eventKind"), TEXT("event"));
			Json->SetStringField(TEXT("functionName"), EventNode->GetFunctionName().ToString());
			Json->SetStringField(TEXT("customFunctionName"), EventNode->CustomFunctionName.ToString());
			Json->SetBoolField(TEXT("overrideFunction"), EventNode->bOverrideFunction != 0);
		}

		if (const UK2Node_CallFunction* CallFunction = Cast<UK2Node_CallFunction>(Node))
		{
			UFunction* Function = CallFunction->GetTargetFunction();
			Json->SetStringField(TEXT("callKind"), TEXT("function"));
			Json->SetStringField(TEXT("functionName"), CallFunction->GetFunctionName().ToString());
			Json->SetStringField(TEXT("functionPath"), Function ? Function->GetPathName() : FString());
			Json->SetBoolField(TEXT("latent"), CallFunction->IsLatentFunction());
			Json->SetBoolField(TEXT("pure"), CallFunction->IsNodePure());
		}

		return Json;
	}

	static TSharedRef<FJsonObject> ExportNode(const UEdGraphNode* Node, EUIJsonBridgeExportProfile Profile)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		if (!Node)
		{
			return Json;
		}

		Json->SetStringField(TEXT("name"), Node->GetName());
		Json->SetStringField(TEXT("guid"), Node->NodeGuid.ToString(EGuidFormats::DigitsWithHyphens));
		Json->SetStringField(TEXT("class"), Node->GetClass()->GetName());
		Json->SetStringField(TEXT("classPath"), Node->GetClass()->GetPathName());
		Json->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
		Json->SetStringField(TEXT("tooltip"), Node->GetTooltipText().ToString());
		Json->SetNumberField(TEXT("x"), Node->NodePosX);
		Json->SetNumberField(TEXT("y"), Node->NodePosY);
		Json->SetNumberField(TEXT("width"), Node->NodeWidth);
		Json->SetNumberField(TEXT("height"), Node->NodeHeight);
		Json->SetBoolField(TEXT("enabled"), Node->IsNodeEnabled());
		Json->SetBoolField(TEXT("commentBubbleVisible"), Node->bCommentBubbleVisible != 0);
		Json->SetStringField(TEXT("nodeComment"), Node->NodeComment);
		Json->SetObjectField(TEXT("metadata"), ExportNodeMetadata(Node));

		if (ShouldExportGraphPins(Profile))
		{
			TArray<TSharedPtr<FJsonValue>> Pins;
			for (const UEdGraphPin* Pin : Node->Pins)
			{
				Pins.Add(MakeShared<FJsonValueObject>(ExportPin(Pin)));
			}
			Json->SetArrayField(TEXT("pins"), Pins);
		}

		return Json;
	}

	static TSharedRef<FJsonObject> ExportGraph(const UEdGraph* Graph, const FString& GraphKind, EUIJsonBridgeExportProfile Profile)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		if (!Graph)
		{
			return Json;
		}

		Json->SetStringField(TEXT("name"), Graph->GetName());
		Json->SetStringField(TEXT("displayName"), Graph->GetFName().ToString());
		Json->SetStringField(TEXT("kind"), GraphKind);
		Json->SetStringField(TEXT("class"), Graph->GetClass()->GetName());
		Json->SetStringField(TEXT("objectPath"), Graph->GetPathName());

		TArray<TSharedPtr<FJsonValue>> Nodes;
		for (const UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node)
			{
				Nodes.Add(MakeShared<FJsonValueObject>(ExportNode(Node, Profile)));
			}
		}
		Json->SetArrayField(TEXT("nodes"), Nodes);
		Json->SetNumberField(TEXT("nodeCount"), Nodes.Num());

		if (ShouldExportGraphClipboardText(Profile) && GraphKind == TEXT("ubergraph"))
		{
			TSet<UObject*> NodesToExport;
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (Node)
				{
					NodesToExport.Add(Node);
				}
			}

			FString ClipboardText;
			FEdGraphUtilities::ExportNodesToText(NodesToExport, ClipboardText);
			Json->SetStringField(TEXT("clipboardText"), ClipboardText);
		}

		return Json;
	}

	static TSharedRef<FJsonObject> ExportGraphs(const UWidgetBlueprint* WidgetBlueprint, EUIJsonBridgeExportProfile Profile)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> Graphs;

		for (const UEdGraph* Graph : WidgetBlueprint->UbergraphPages)
		{
			if (Graph)
			{
				Graphs.Add(MakeShared<FJsonValueObject>(ExportGraph(Graph, TEXT("ubergraph"), Profile)));
			}
		}

		for (const UEdGraph* Graph : WidgetBlueprint->FunctionGraphs)
		{
			if (Graph)
			{
				Graphs.Add(MakeShared<FJsonValueObject>(ExportGraph(Graph, TEXT("function"), Profile)));
			}
		}

		for (const UEdGraph* Graph : WidgetBlueprint->MacroGraphs)
		{
			if (Graph)
			{
				Graphs.Add(MakeShared<FJsonValueObject>(ExportGraph(Graph, TEXT("macro"), Profile)));
			}
		}

		Json->SetArrayField(TEXT("items"), Graphs);
		Json->SetNumberField(TEXT("count"), Graphs.Num());
		return Json;
	}

	static TSharedRef<FJsonObject> ExportFunctionSignature(const UEdGraph* Graph)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		if (!Graph)
		{
			return Json;
		}

		Json->SetStringField(TEXT("name"), Graph->GetName());
		Json->SetStringField(TEXT("displayName"), Graph->GetFName().ToString());
		Json->SetStringField(TEXT("objectPath"), Graph->GetPathName());

		TArray<TSharedPtr<FJsonValue>> Inputs;
		TArray<TSharedPtr<FJsonValue>> Outputs;
		for (const UEdGraphNode* Node : Graph->Nodes)
		{
			if (const UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node))
			{
				for (const UEdGraphPin* Pin : EntryNode->Pins)
				{
					if (Pin && Pin->Direction == EGPD_Output && !Pin->bHidden && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
					{
						Inputs.Add(MakeShared<FJsonValueObject>(ExportFunctionParameter(Pin)));
					}
				}
			}
			else if (const UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(Node))
			{
				for (const UEdGraphPin* Pin : ResultNode->Pins)
				{
					if (Pin && Pin->Direction == EGPD_Input && !Pin->bHidden && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
					{
						Outputs.Add(MakeShared<FJsonValueObject>(ExportFunctionParameter(Pin)));
					}
				}
			}
		}

		Json->SetArrayField(TEXT("inputs"), Inputs);
		Json->SetArrayField(TEXT("outputs"), Outputs);
		return Json;
	}

	static TSharedRef<FJsonObject> ExportFunctionSignatures(const UWidgetBlueprint* WidgetBlueprint)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> Functions;
		for (const UEdGraph* Graph : WidgetBlueprint->FunctionGraphs)
		{
			if (Graph)
			{
				Functions.Add(MakeShared<FJsonValueObject>(ExportFunctionSignature(Graph)));
			}
		}

		Json->SetArrayField(TEXT("items"), Functions);
		Json->SetNumberField(TEXT("count"), Functions.Num());
		return Json;
	}

	static TSharedRef<FJsonObject> ExportBlueprintVariable(const FBPVariableDescription& Variable)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("name"), Variable.VarName.ToString());
		Json->SetStringField(TEXT("guid"), Variable.VarGuid.ToString(EGuidFormats::DigitsWithHyphens));
		Json->SetObjectField(TEXT("type"), ExportPinType(Variable.VarType));
		Json->SetStringField(TEXT("defaultValue"), Variable.DefaultValue);
		Json->SetStringField(TEXT("category"), Variable.Category.ToString());
		Json->SetStringField(TEXT("friendlyName"), Variable.FriendlyName);
		return Json;
	}

	static TSharedRef<FJsonObject> ExportBlueprintVariables(const UWidgetBlueprint* WidgetBlueprint)
	{
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> Variables;
		for (const FBPVariableDescription& Variable : WidgetBlueprint->NewVariables)
		{
			Variables.Add(MakeShared<FJsonValueObject>(ExportBlueprintVariable(Variable)));
		}

		Json->SetArrayField(TEXT("items"), Variables);
		Json->SetNumberField(TEXT("count"), Variables.Num());
		return Json;
	}
}

bool FUIJsonBridgeExporter::ExportWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint, const FString& OutputFilePath, FText& OutError)
{
	return ExportWidgetBlueprint(WidgetBlueprint, OutputFilePath, EUIJsonBridgeExportProfile::Full, OutError);
}

bool FUIJsonBridgeExporter::ExportWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint, const FString& OutputFilePath, EUIJsonBridgeExportProfile Profile, FText& OutError)
{
	if (!WidgetBlueprint)
	{
		OutError = LOCTEXT("NoWidgetBlueprint", "No Widget Blueprint was provided.");
		return false;
	}

	if (!WidgetBlueprint->WidgetTree)
	{
		OutError = FText::Format(
			LOCTEXT("NoWidgetTree", "{0} does not contain a WidgetTree."),
			FText::FromString(WidgetBlueprint->GetPathName()));
		return false;
	}

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("schema"), TEXT("ui-json-bridge.umg-widget-blueprint"));
	Root->SetNumberField(TEXT("schemaVersion"), 1);
	Root->SetStringField(TEXT("profile"), UIJsonBridge::GetExportProfileName(Profile));
	Root->SetBoolField(TEXT("importable"), Profile == EUIJsonBridgeExportProfile::LayoutOnly || Profile == EUIJsonBridgeExportProfile::Full);
	Root->SetStringField(TEXT("engineHint"), TEXT("UE 5.6"));
	Root->SetStringField(TEXT("assetName"), WidgetBlueprint->GetName());
	Root->SetStringField(TEXT("assetPath"), WidgetBlueprint->GetPathName());
	Root->SetStringField(TEXT("generatedClass"), WidgetBlueprint->GeneratedClass ? WidgetBlueprint->GeneratedClass->GetPathName() : FString());
	Root->SetStringField(TEXT("parentClass"), WidgetBlueprint->ParentClass ? WidgetBlueprint->ParentClass->GetPathName() : FString());

	if (UIJsonBridge::ShouldExportBindings(Profile))
	{
		Root->SetObjectField(TEXT("bindings"), UIJsonBridge::ExportBindings(WidgetBlueprint));
	}
	if (Profile == EUIJsonBridgeExportProfile::Full)
	{
		Root->SetObjectField(TEXT("blueprintVariables"), UIJsonBridge::ExportBlueprintVariables(WidgetBlueprint));
		Root->SetObjectField(TEXT("functionSignatures"), UIJsonBridge::ExportFunctionSignatures(WidgetBlueprint));
	}
	if (UIJsonBridge::ShouldExportAnimations(Profile))
	{
		Root->SetArrayField(TEXT("animations"), UIJsonBridge::ExportAnimations(WidgetBlueprint));
	}
	if (UIJsonBridge::ShouldExportGraphs(Profile))
	{
		Root->SetObjectField(TEXT("graphs"), UIJsonBridge::ExportGraphs(WidgetBlueprint, Profile));
	}

	if (UWidget* RootWidget = WidgetBlueprint->WidgetTree->RootWidget)
	{
		Root->SetObjectField(TEXT("rootWidget"), UIJsonBridge::ExportWidget(RootWidget, Profile));
	}
	else
	{
		Root->SetObjectField(TEXT("rootWidget"), MakeShared<FJsonObject>());
	}

	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	if (!FJsonSerializer::Serialize(Root, Writer))
	{
		OutError = LOCTEXT("SerializeFailed", "Failed to serialize Widget Blueprint JSON.");
		return false;
	}

	const FString OutputDirectory = FPaths::GetPath(OutputFilePath);
	IFileManager::Get().MakeDirectory(*OutputDirectory, true);

	if (!FFileHelper::SaveStringToFile(Output, *OutputFilePath, FFileHelper::EEncodingOptions::ForceUTF8))
	{
		OutError = FText::Format(
			LOCTEXT("SaveFailed", "Failed to save JSON to {0}."),
			FText::FromString(OutputFilePath));
		return false;
	}

	return true;
}

FString FUIJsonBridgeExporter::MakeDefaultExportFilePath(const UWidgetBlueprint* WidgetBlueprint, EUIJsonBridgeExportProfile Profile)
{
	const FString BaseDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("UIJsonBridge"));
	const FString AssetName = WidgetBlueprint ? WidgetBlueprint->GetName() : TEXT("WidgetBlueprint");
	return BaseDir / FString::Printf(TEXT("%s_%s.json"), *AssetName, *UIJsonBridge::GetExportProfileName(Profile));
}

FText FUIJsonBridgeExporter::GetExportProfileDisplayName(EUIJsonBridgeExportProfile Profile)
{
	switch (Profile)
	{
	case EUIJsonBridgeExportProfile::LayoutOnly:
		return LOCTEXT("ExportProfileLayoutOnly", "Layout Only");
	case EUIJsonBridgeExportProfile::Interaction:
		return LOCTEXT("ExportProfileInteraction", "Interaction");
	case EUIJsonBridgeExportProfile::Full:
	default:
		return LOCTEXT("ExportProfileFull", "Full");
	}
}

#undef LOCTEXT_NAMESPACE
