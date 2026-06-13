#if WITH_AUTOMATION_TESTS

#include "UIJsonBridgeImporter.h"
#include "UIJsonBridgeExporter.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Fonts/CompositeFont.h"
#include "K2Node_FunctionEntry.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "HAL/FileManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "WidgetBlueprint.h"

namespace UIJsonBridge::Tests
{
	static FString MakeTempJsonPath(const FString& Prefix)
	{
		const FString Dir = FPaths::ProjectIntermediateDir() / TEXT("UIJsonBridgeTests");
		IFileManager::Get().MakeDirectory(*Dir, true);
		return Dir / FString::Printf(TEXT("%s_%s.json"), *Prefix, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	static bool WriteJsonFile(const FString& Json, FString& OutPath)
	{
		OutPath = MakeTempJsonPath(TEXT("Import"));
		return FFileHelper::SaveStringToFile(Json, *OutPath, FFileHelper::EEncodingOptions::ForceUTF8);
	}

	static FString MakeMinimalJson(const TCHAR* ChildClass, const TCHAR* ChildClassPath)
	{
		return FString::Printf(TEXT(R"JSON(
{
  "rootWidget": {
    "name": "CanvasPanel_165",
    "class": "CanvasPanel",
    "classPath": "/Script/UMG.CanvasPanel",
    "properties": {},
    "children": [
      {
        "name": "HorizontalBox_182",
        "class": "%s",
        "classPath": "%s",
        "properties": {},
        "children": []
      }
    ]
  }
}
)JSON"), ChildClass, ChildClassPath);
	}

	static UWidgetBlueprint* CreateTransientWidgetBlueprint()
	{
		UPackage* Package = CreatePackage(*FString::Printf(TEXT("/Engine/Transient/UIJsonBridgeTest_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
		UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
			UUserWidget::StaticClass(),
			Package,
			TEXT("WBP_UIJsonBridgeTest"),
			BPTYPE_Normal,
			UWidgetBlueprint::StaticClass(),
			UWidgetBlueprintGeneratedClass::StaticClass(),
			NAME_None));

		return WidgetBlueprint;
	}

	static UWidget* FindWidgetByName(UWidgetBlueprint* WidgetBlueprint, const FName Name)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return nullptr;
		}

		TArray<UWidget*> Widgets;
		WidgetBlueprint->WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (Widget && Widget->GetFName() == Name)
			{
				return Widget;
			}
		}
		return nullptr;
	}

	static TSharedPtr<FJsonObject> LoadJsonObject(const FString& Path)
	{
		FString JsonText;
		if (!FFileHelper::LoadFileToString(JsonText, *Path))
		{
			return nullptr;
		}

		TSharedPtr<FJsonObject> JsonObject;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
		if (!FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			return nullptr;
		}
		return JsonObject;
	}

	static bool SaveJsonObject(const TSharedPtr<FJsonObject>& JsonObject, const FString& Path)
	{
		if (!JsonObject.IsValid())
		{
			return false;
		}

		FString JsonText;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonText);
		if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
		{
			return false;
		}

		return FFileHelper::SaveStringToFile(JsonText, *Path, FFileHelper::EEncodingOptions::ForceUTF8);
	}

	static FString GetPrimaryFontFilename(const FSlateFontInfo& FontInfo)
	{
		const FCompositeFont* CompositeFont = FontInfo.GetCompositeFont();
		if (!CompositeFont || CompositeFont->DefaultTypeface.Fonts.Num() == 0)
		{
			return FString();
		}

		return CompositeFont->DefaultTypeface.Fonts[0].Font.GetFontFilename();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIJsonBridgeExporterProfilesTest,
	"UIJsonBridge.Exporter.ProfileFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIJsonBridgeExporterProfilesTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* WidgetBlueprint = UIJsonBridge::Tests::CreateTransientWidgetBlueprint();
	TestNotNull(TEXT("Transient Widget Blueprint is created"), WidgetBlueprint);
	if (!WidgetBlueprint)
	{
		return false;
	}

	FString JsonPath;
	TestTrue(TEXT("Initial JSON file is written"), UIJsonBridge::Tests::WriteJsonFile(
		UIJsonBridge::Tests::MakeMinimalJson(TEXT("HorizontalBox"), TEXT("/Script/UMG.HorizontalBox")),
		JsonPath));

	FText Error;
	TestTrue(TEXT("Initial import succeeds"), FUIJsonBridgeImporter::ImportWidgetBlueprint(WidgetBlueprint, JsonPath, Error));

	FString LayoutPath = UIJsonBridge::Tests::MakeTempJsonPath(TEXT("LayoutExport"));
	FString InteractionPath = UIJsonBridge::Tests::MakeTempJsonPath(TEXT("InteractionExport"));
	FString FullPath = UIJsonBridge::Tests::MakeTempJsonPath(TEXT("FullExport"));

	TestTrue(TEXT("Layout export succeeds"), FUIJsonBridgeExporter::ExportWidgetBlueprint(WidgetBlueprint, LayoutPath, EUIJsonBridgeExportProfile::LayoutOnly, Error));
	TestTrue(TEXT("Interaction export succeeds"), FUIJsonBridgeExporter::ExportWidgetBlueprint(WidgetBlueprint, InteractionPath, EUIJsonBridgeExportProfile::Interaction, Error));
	TestTrue(TEXT("Full export succeeds"), FUIJsonBridgeExporter::ExportWidgetBlueprint(WidgetBlueprint, FullPath, EUIJsonBridgeExportProfile::Full, Error));

	const TSharedPtr<FJsonObject> LayoutJson = UIJsonBridge::Tests::LoadJsonObject(LayoutPath);
	const TSharedPtr<FJsonObject> InteractionJson = UIJsonBridge::Tests::LoadJsonObject(InteractionPath);
	const TSharedPtr<FJsonObject> FullJson = UIJsonBridge::Tests::LoadJsonObject(FullPath);
	TestTrue(TEXT("Layout JSON loaded"), LayoutJson.IsValid());
	TestTrue(TEXT("Interaction JSON loaded"), InteractionJson.IsValid());
	TestTrue(TEXT("Full JSON loaded"), FullJson.IsValid());

	if (LayoutJson.IsValid())
	{
		TestEqual(TEXT("Layout profile field"), LayoutJson->GetStringField(TEXT("profile")), FString(TEXT("layout")));
		TestTrue(TEXT("Layout profile is importable"), LayoutJson->GetBoolField(TEXT("importable")));
		TestFalse(TEXT("Layout profile does not export graphs"), LayoutJson->HasField(TEXT("graphs")));
		TestFalse(TEXT("Layout profile does not export bindings"), LayoutJson->HasField(TEXT("bindings")));
	}

	if (InteractionJson.IsValid())
	{
		TestEqual(TEXT("Interaction profile field"), InteractionJson->GetStringField(TEXT("profile")), FString(TEXT("interaction")));
		TestFalse(TEXT("Interaction profile is analysis-only"), InteractionJson->GetBoolField(TEXT("importable")));
		TestTrue(TEXT("Interaction profile exports graphs"), InteractionJson->HasField(TEXT("graphs")));
		TestFalse(TEXT("Interaction root widget omits heavy properties"), InteractionJson->GetObjectField(TEXT("rootWidget"))->HasField(TEXT("properties")));
	}

	if (FullJson.IsValid())
	{
		TestEqual(TEXT("Full profile field"), FullJson->GetStringField(TEXT("profile")), FString(TEXT("full")));
		TestTrue(TEXT("Full profile is importable"), FullJson->GetBoolField(TEXT("importable")));
		TestTrue(TEXT("Full profile exports graphs"), FullJson->HasField(TEXT("graphs")));
		TestTrue(TEXT("Full root widget includes properties"), FullJson->GetObjectField(TEXT("rootWidget"))->HasField(TEXT("properties")));
	}

	IFileManager::Get().Delete(*JsonPath);
	IFileManager::Get().Delete(*LayoutPath);
	IFileManager::Get().Delete(*InteractionPath);
	IFileManager::Get().Delete(*FullPath);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIJsonBridgeLayoutRoundTripIntoFreshBlueprintTest,
	"UIJsonBridge.Layout.RoundTripModifiedLayoutIntoFreshBlueprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIJsonBridgeLayoutRoundTripIntoFreshBlueprintTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* SourceBlueprint = UIJsonBridge::Tests::CreateTransientWidgetBlueprint();
	UWidgetBlueprint* TargetBlueprint = UIJsonBridge::Tests::CreateTransientWidgetBlueprint();
	TestNotNull(TEXT("Source Widget Blueprint is created"), SourceBlueprint);
	TestNotNull(TEXT("Target Widget Blueprint is created"), TargetBlueprint);
	if (!SourceBlueprint || !TargetBlueprint)
	{
		return false;
	}

	FString JsonPath;
	TestTrue(TEXT("Initial JSON file is written"), UIJsonBridge::Tests::WriteJsonFile(
		UIJsonBridge::Tests::MakeMinimalJson(TEXT("HorizontalBox"), TEXT("/Script/UMG.HorizontalBox")),
		JsonPath));

	FText Error;
	TestTrue(TEXT("Initial source import succeeds"), FUIJsonBridgeImporter::ImportWidgetBlueprint(SourceBlueprint, JsonPath, Error));

	FString LayoutExportPath = UIJsonBridge::Tests::MakeTempJsonPath(TEXT("LayoutRoundTripExport"));
	TestTrue(TEXT("Layout export succeeds"), FUIJsonBridgeExporter::ExportWidgetBlueprint(SourceBlueprint, LayoutExportPath, EUIJsonBridgeExportProfile::LayoutOnly, Error));

	TSharedPtr<FJsonObject> LayoutJson = UIJsonBridge::Tests::LoadJsonObject(LayoutExportPath);
	TestTrue(TEXT("Layout JSON reloads"), LayoutJson.IsValid());
	if (!LayoutJson.IsValid())
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* RootWidgetJson = nullptr;
	TestTrue(TEXT("Layout JSON has rootWidget"), LayoutJson->TryGetObjectField(TEXT("rootWidget"), RootWidgetJson) && RootWidgetJson && RootWidgetJson->IsValid());
	if (!RootWidgetJson || !RootWidgetJson->IsValid())
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* Children = nullptr;
	TestTrue(TEXT("Root widget has children"), (*RootWidgetJson)->TryGetArrayField(TEXT("children"), Children) && Children && Children->Num() > 0);
	if (!Children || Children->IsEmpty())
	{
		return false;
	}

	const TSharedPtr<FJsonObject> FirstChild = (*Children)[0]->AsObject();
	TestTrue(TEXT("First child is valid"), FirstChild.IsValid());
	if (!FirstChild.IsValid())
	{
		return false;
	}

	const TSharedPtr<FJsonObject> SlotJson = FirstChild->GetObjectField(TEXT("slot"));
	const TSharedPtr<FJsonObject> SlotPropertiesJson = SlotJson->GetObjectField(TEXT("properties"));
	SlotPropertiesJson->SetStringField(TEXT("LayoutData"), TEXT("(Offsets=(Left=32.000000,Top=48.000000,Right=32.000000,Bottom=64.000000),Anchors=(Minimum=(X=0.000000,Y=0.000000),Maximum=(X=1.000000,Y=0.000000)),Alignment=(X=0.000000,Y=0.000000))"));
	SlotPropertiesJson->SetStringField(TEXT("ZOrder"), TEXT("7"));

	FString ModifiedLayoutPath = UIJsonBridge::Tests::MakeTempJsonPath(TEXT("LayoutRoundTripModified"));
	TestTrue(TEXT("Modified layout JSON is written"), UIJsonBridge::Tests::SaveJsonObject(LayoutJson, ModifiedLayoutPath));

	Error = FText::GetEmpty();
	const bool bModifiedImportSucceeded = FUIJsonBridgeImporter::ImportWidgetBlueprint(TargetBlueprint, ModifiedLayoutPath, Error);
	if (!bModifiedImportSucceeded)
	{
		AddError(FString::Printf(TEXT("Modified layout import failed: %s"), *Error.ToString()));
	}
	TestTrue(TEXT("Modified layout imports into fresh Widget Blueprint"), bModifiedImportSucceeded);

	UWidget* ImportedWidget = UIJsonBridge::Tests::FindWidgetByName(TargetBlueprint, TEXT("HorizontalBox_182"));
	TestNotNull(TEXT("Modified layout child exists in fresh Widget Blueprint"), ImportedWidget);

	const UCanvasPanelSlot* CanvasSlot = ImportedWidget ? Cast<UCanvasPanelSlot>(ImportedWidget->Slot) : nullptr;
	TestNotNull(TEXT("Imported child has CanvasPanelSlot"), CanvasSlot);
	if (CanvasSlot)
	{
		TestEqual(TEXT("Modified left offset applied"), CanvasSlot->GetLayout().Offsets.Left, 32.0f);
		TestEqual(TEXT("Modified z-order applied"), CanvasSlot->GetZOrder(), 7);
	}

	IFileManager::Get().Delete(*JsonPath);
	IFileManager::Get().Delete(*LayoutExportPath);
	IFileManager::Get().Delete(*ModifiedLayoutPath);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIJsonBridgeImporterClassReplacementTest,
	"UIJsonBridge.Importer.ReplacesSameNamedDifferentClassWidgets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIJsonBridgeImporterClassReplacementTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* WidgetBlueprint = UIJsonBridge::Tests::CreateTransientWidgetBlueprint();
	TestNotNull(TEXT("Transient Widget Blueprint is created"), WidgetBlueprint);
	if (!WidgetBlueprint)
	{
		return false;
	}

	FString FirstJsonPath;
	TestTrue(TEXT("HorizontalBox JSON file is written"), UIJsonBridge::Tests::WriteJsonFile(
		UIJsonBridge::Tests::MakeMinimalJson(TEXT("HorizontalBox"), TEXT("/Script/UMG.HorizontalBox")),
		FirstJsonPath));

	FText Error;
	TestTrue(TEXT("Initial HorizontalBox import succeeds"), FUIJsonBridgeImporter::ImportWidgetBlueprint(WidgetBlueprint, FirstJsonPath, Error));
	TestTrue(TEXT("HorizontalBox_182 is a HorizontalBox"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("HorizontalBox_182"))->IsA<UHorizontalBox>());

	FString ReplacementJsonPath;
	TestTrue(TEXT("WrapBox JSON file is written"), UIJsonBridge::Tests::WriteJsonFile(
		UIJsonBridge::Tests::MakeMinimalJson(TEXT("WrapBox"), TEXT("/Script/UMG.WrapBox")),
		ReplacementJsonPath));

	Error = FText::GetEmpty();
	TestTrue(TEXT("Same-name class replacement import succeeds"), FUIJsonBridgeImporter::ImportWidgetBlueprint(WidgetBlueprint, ReplacementJsonPath, Error));

	UWidget* ReplacedWidget = UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("HorizontalBox_182"));
	TestNotNull(TEXT("HorizontalBox_182 still exists"), ReplacedWidget);
	TestTrue(TEXT("HorizontalBox_182 is now a WrapBox"), ReplacedWidget && ReplacedWidget->IsA<UWrapBox>());

	IFileManager::Get().Delete(*FirstJsonPath);
	IFileManager::Get().Delete(*ReplacementJsonPath);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIJsonBridgeFullInteractionRoundTripTest,
	"UIJsonBridge.Full.InteractionMetadataRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIJsonBridgeFullInteractionRoundTripTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* SourceBlueprint = UIJsonBridge::Tests::CreateTransientWidgetBlueprint();
	UWidgetBlueprint* TargetBlueprint = UIJsonBridge::Tests::CreateTransientWidgetBlueprint();
	TestNotNull(TEXT("Source Widget Blueprint is created"), SourceBlueprint);
	TestNotNull(TEXT("Target Widget Blueprint is created"), TargetBlueprint);
	if (!SourceBlueprint || !TargetBlueprint)
	{
		return false;
	}

	FString JsonPath;
	TestTrue(TEXT("Initial JSON file is written"), UIJsonBridge::Tests::WriteJsonFile(
		UIJsonBridge::Tests::MakeMinimalJson(TEXT("TextBlock"), TEXT("/Script/UMG.TextBlock")),
		JsonPath));

	FText Error;
	TestTrue(TEXT("Initial source import succeeds"), FUIJsonBridgeImporter::ImportWidgetBlueprint(SourceBlueprint, JsonPath, Error));

	UEdGraph* FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(
		SourceBlueprint,
		TEXT("DemoSignature"),
		UEdGraph::StaticClass(),
		UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddFunctionGraph<UFunction>(SourceBlueprint, FunctionGraph, true, nullptr);

	TArray<UK2Node_FunctionEntry*> EntryNodes;
	FunctionGraph->GetNodesOfClass(EntryNodes);
	TestTrue(TEXT("Function entry exists"), EntryNodes.Num() > 0);
	if (EntryNodes.Num() > 0)
	{
		FEdGraphPinType StringPinType;
		StringPinType.PinCategory = UEdGraphSchema_K2::PC_String;
		EntryNodes[0]->CreateUserDefinedPin(TEXT("InputText"), StringPinType, EGPD_Output, false);
	}

	FString FullPath = UIJsonBridge::Tests::MakeTempJsonPath(TEXT("FullInteractionExport"));
	TestTrue(TEXT("Full export succeeds"), FUIJsonBridgeExporter::ExportWidgetBlueprint(SourceBlueprint, FullPath, EUIJsonBridgeExportProfile::Full, Error));

	const TSharedPtr<FJsonObject> FullJson = UIJsonBridge::Tests::LoadJsonObject(FullPath);
	TestTrue(TEXT("Full JSON loaded"), FullJson.IsValid());
	if (FullJson.IsValid())
	{
		TestTrue(TEXT("Full JSON includes function signatures"), FullJson->HasField(TEXT("functionSignatures")));
		TestTrue(TEXT("Full JSON includes blueprint variables"), FullJson->HasField(TEXT("blueprintVariables")));
		const TSharedPtr<FJsonObject> GraphsJson = FullJson->GetObjectField(TEXT("graphs"));
		TestTrue(TEXT("Full JSON includes graph metadata"), GraphsJson.IsValid());
		if (GraphsJson.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* GraphItems = nullptr;
			TestTrue(TEXT("Graph item array exists"), GraphsJson->TryGetArrayField(TEXT("items"), GraphItems) && GraphItems != nullptr);
			bool bHasUbergraphClipboardText = false;
			if (GraphItems)
			{
				for (const TSharedPtr<FJsonValue>& GraphValue : *GraphItems)
				{
					const TSharedPtr<FJsonObject> GraphJson = GraphValue.IsValid() ? GraphValue->AsObject() : nullptr;
					FString Kind;
					if (GraphJson.IsValid() && GraphJson->TryGetStringField(TEXT("kind"), Kind) && Kind == TEXT("ubergraph"))
					{
						bHasUbergraphClipboardText = GraphJson->HasField(TEXT("clipboardText"));
						break;
					}
				}
			}
			TestTrue(TEXT("Ubergraph exports clipboard text field"), bHasUbergraphClipboardText);
		}
	}

	Error = FText::GetEmpty();
	TestTrue(TEXT("Full import succeeds"), FUIJsonBridgeImporter::ImportWidgetBlueprint(TargetBlueprint, FullPath, Error));
	TestNotNull(TEXT("Imported text widget exists"), UIJsonBridge::Tests::FindWidgetByName(TargetBlueprint, TEXT("HorizontalBox_182")));

	UEdGraph* ImportedFunction = nullptr;
	for (UEdGraph* Graph : TargetBlueprint->FunctionGraphs)
	{
		if (Graph && Graph->GetFName() == TEXT("DemoSignature"))
		{
			ImportedFunction = Graph;
			break;
		}
	}
	TestNotNull(TEXT("Function signature graph is imported"), ImportedFunction);
	if (ImportedFunction)
	{
		TArray<UK2Node_FunctionEntry*> ImportedEntries;
		ImportedFunction->GetNodesOfClass(ImportedEntries);
		TestTrue(TEXT("Imported function has entry node"), ImportedEntries.Num() > 0);
		if (ImportedEntries.Num() > 0)
		{
			TestNotNull(TEXT("Imported function keeps input pin"), ImportedEntries[0]->FindPin(TEXT("InputText")));
		}
	}

	IFileManager::Get().Delete(*JsonPath);
	IFileManager::Get().Delete(*FullPath);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIJsonBridgeImporterBlocksMismatchedEngineVersionTest,
	"UIJsonBridge.Importer.BlocksMismatchedEngineVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIJsonBridgeImporterBlocksMismatchedEngineVersionTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* WidgetBlueprint = UIJsonBridge::Tests::CreateTransientWidgetBlueprint();
	TestNotNull(TEXT("Transient Widget Blueprint is created"), WidgetBlueprint);
	if (!WidgetBlueprint)
	{
		return false;
	}

	FString ValidJsonPath;
	TestTrue(TEXT("Valid JSON file is written"), UIJsonBridge::Tests::WriteJsonFile(
		UIJsonBridge::Tests::MakeMinimalJson(TEXT("HorizontalBox"), TEXT("/Script/UMG.HorizontalBox")),
		ValidJsonPath));

	FText Error;
	TestTrue(TEXT("Initial import succeeds"), FUIJsonBridgeImporter::ImportWidgetBlueprint(WidgetBlueprint, ValidJsonPath, Error));
	TestNotNull(TEXT("Initial widget exists"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("HorizontalBox_182")));

	const FString MismatchedJson = TEXT(R"JSON(
{
  "schema": "ui-json-bridge.umg-widget-blueprint",
  "schemaVersion": 1,
  "profile": "full",
  "importable": true,
  "engineHint": "UE 5.5",
  "rootWidget": {
    "name": "CanvasPanel_165",
    "class": "CanvasPanel",
    "classPath": "/Script/UMG.CanvasPanel",
    "properties": {},
    "children": [
      {
        "name": "Should_Not_Import",
        "class": "TextBlock",
        "classPath": "/Script/UMG.TextBlock",
        "properties": {},
        "children": []
      }
    ]
  }
}
)JSON");

	FString MismatchedJsonPath;
	TestTrue(TEXT("Mismatched JSON file is written"), UIJsonBridge::Tests::WriteJsonFile(MismatchedJson, MismatchedJsonPath));

	Error = FText::GetEmpty();
	TestFalse(TEXT("Mismatched engine import is blocked"), FUIJsonBridgeImporter::ImportWidgetBlueprint(WidgetBlueprint, MismatchedJsonPath, Error));
	TestTrue(TEXT("Version mismatch error is reported"), Error.ToString().Contains(TEXT("current editor")));
	TestNotNull(TEXT("Previous widget remains after blocked import"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("HorizontalBox_182")));
	TestNull(TEXT("Mismatched JSON widget was not imported"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("Should_Not_Import")));

	IFileManager::Get().Delete(*ValidJsonPath);
	IFileManager::Get().Delete(*MismatchedJsonPath);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIJsonBridgeImporterRestoresExistingVariableFlagsTest,
	"UIJsonBridge.Importer.RestoresExistingVariableFlags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIJsonBridgeImporterRestoresExistingVariableFlagsTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* WidgetBlueprint = UIJsonBridge::Tests::CreateTransientWidgetBlueprint();
	TestNotNull(TEXT("Transient Widget Blueprint is created"), WidgetBlueprint);
	if (!WidgetBlueprint)
	{
		return false;
	}

	const FString InitialJson = TEXT(R"JSON(
{
  "rootWidget": {
    "name": "CanvasPanel_165",
    "class": "CanvasPanel",
    "classPath": "/Script/UMG.CanvasPanel",
    "properties": {
      "bIsVariable": "False"
    },
    "children": [
      {
        "name": "Txt_Status",
        "class": "TextBlock",
        "classPath": "/Script/UMG.TextBlock",
        "isVariable": true,
        "properties": {
          "Text": "NSLOCTEXT(\"[UIJsonBridgeTest]\", \"StatusA\", \"Ready\")",
          "bIsVariable": "True"
        },
        "children": []
      }
    ]
  }
}
)JSON");

	FString InitialJsonPath;
	TestTrue(TEXT("Initial variable JSON file is written"), UIJsonBridge::Tests::WriteJsonFile(InitialJson, InitialJsonPath));

	FText Error;
	TestTrue(TEXT("Initial variable import succeeds"), FUIJsonBridgeImporter::ImportWidgetBlueprint(WidgetBlueprint, InitialJsonPath, Error));
	UWidget* InitialStatus = UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("Txt_Status"));
	TestNotNull(TEXT("Initial status widget exists"), InitialStatus);
	TestTrue(TEXT("Initial status widget is variable"), InitialStatus && InitialStatus->bIsVariable);

	const FString MissingVariableFlagJson = TEXT(R"JSON(
{
  "rootWidget": {
    "name": "CanvasPanel_165",
    "class": "CanvasPanel",
    "classPath": "/Script/UMG.CanvasPanel",
    "properties": {
      "bIsVariable": "False"
    },
    "children": [
      {
        "name": "Txt_Status",
        "class": "TextBlock",
        "classPath": "/Script/UMG.TextBlock",
        "isVariable": false,
        "properties": {
          "Text": "NSLOCTEXT(\"[UIJsonBridgeTest]\", \"StatusB\", \"Updated\")",
          "bIsVariable": "False"
        },
        "children": []
      }
    ]
  }
}
)JSON");

	FString MissingVariableFlagJsonPath;
	TestTrue(TEXT("Missing-variable-flag JSON file is written"), UIJsonBridge::Tests::WriteJsonFile(MissingVariableFlagJson, MissingVariableFlagJsonPath));

	Error = FText::GetEmpty();
	TestTrue(TEXT("Reimport with missing variable flag succeeds"), FUIJsonBridgeImporter::ImportWidgetBlueprint(WidgetBlueprint, MissingVariableFlagJsonPath, Error));

	UWidget* ReimportedStatus = UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("Txt_Status"));
	TestNotNull(TEXT("Reimported status widget exists"), ReimportedStatus);
	TestTrue(TEXT("Existing same-name same-class widget variable flag is restored"), ReimportedStatus && ReimportedStatus->bIsVariable);
	TestTrue(TEXT("Variable GUID map keeps restored widget"), WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(TEXT("Txt_Status")));

	IFileManager::Get().Delete(*InitialJsonPath);
	IFileManager::Get().Delete(*MissingVariableFlagJsonPath);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIJsonBridgeImporterFailurePreservesTreeTest,
	"UIJsonBridge.Importer.FailedImportPreservesExistingTree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIJsonBridgeImporterFailurePreservesTreeTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* WidgetBlueprint = UIJsonBridge::Tests::CreateTransientWidgetBlueprint();
	TestNotNull(TEXT("Transient Widget Blueprint is created"), WidgetBlueprint);
	if (!WidgetBlueprint)
	{
		return false;
	}

	FString ValidJsonPath;
	TestTrue(TEXT("Valid JSON file is written"), UIJsonBridge::Tests::WriteJsonFile(
		UIJsonBridge::Tests::MakeMinimalJson(TEXT("HorizontalBox"), TEXT("/Script/UMG.HorizontalBox")),
		ValidJsonPath));

	FText Error;
	TestTrue(TEXT("Initial import succeeds"), FUIJsonBridgeImporter::ImportWidgetBlueprint(WidgetBlueprint, ValidJsonPath, Error));
	TestTrue(TEXT("Imported widget exists before failure"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("HorizontalBox_182")) != nullptr);

	const FString InvalidJson = TEXT(R"JSON(
{
  "rootWidget": {
    "name": "CanvasPanel_165",
    "class": "CanvasPanel",
    "classPath": "/Script/UMG.CanvasPanel",
    "properties": {},
    "children": [
      {
        "name": "HorizontalBox_182",
        "class": "NoSuchWidgetClass",
        "classPath": "/Script/UMG.NoSuchWidgetClass",
        "properties": {},
        "children": []
      }
    ]
  }
}
)JSON");

	FString InvalidJsonPath;
	TestTrue(TEXT("Invalid JSON file is written"), UIJsonBridge::Tests::WriteJsonFile(InvalidJson, InvalidJsonPath));

	Error = FText::GetEmpty();
	TestFalse(TEXT("Invalid import fails gracefully"), FUIJsonBridgeImporter::ImportWidgetBlueprint(WidgetBlueprint, InvalidJsonPath, Error));
	TestTrue(TEXT("Previous widget still exists after failed import"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("HorizontalBox_182")) != nullptr);

	IFileManager::Get().Delete(*ValidJsonPath);
	IFileManager::Get().Delete(*InvalidJsonPath);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIJsonBridgeImporterProjectSafeJsonTest,
	"UIJsonBridge.Importer.ImportsProjectResponsiveSafeJson",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIJsonBridgeImporterProjectSafeJsonTest::RunTest(const FString& Parameters)
{
	const FString SafeJsonPath = FPaths::ProjectDir() / TEXT("UI_export/WBP_LocalTTS_DemoPanel_responsive_safe.json");
	if (!FPaths::FileExists(SafeJsonPath))
	{
		AddWarning(FString::Printf(TEXT("Skipping project safe JSON import test because file does not exist: %s"), *SafeJsonPath));
		return true;
	}

	UWidgetBlueprint* WidgetBlueprint = UIJsonBridge::Tests::CreateTransientWidgetBlueprint();
	TestNotNull(TEXT("Transient Widget Blueprint is created"), WidgetBlueprint);
	if (!WidgetBlueprint)
	{
		return false;
	}

	FText Error;
	TestTrue(TEXT("Project responsive safe JSON imports"), FUIJsonBridgeImporter::ImportWidgetBlueprint(WidgetBlueprint, SafeJsonPath, Error));
	UWidget* RootWidget = WidgetBlueprint->WidgetTree ? WidgetBlueprint->WidgetTree->RootWidget.Get() : nullptr;
	TestNotNull(TEXT("Imported root widget exists"), RootWidget);

	TArray<UWidget*> Widgets;
	if (WidgetBlueprint->WidgetTree)
	{
		WidgetBlueprint->WidgetTree->GetAllWidgets(Widgets);
	}

	TestEqual(TEXT("Imported widget count"), Widgets.Num(), 83);
	TestTrue(TEXT("HorizontalBox_182 remains present"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("HorizontalBox_182")) != nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIJsonBridgeImporterMockupDynamicJsonTest,
	"UIJsonBridge.Importer.ImportsLocalTtsMockupDynamicJson",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIJsonBridgeImporterMockupDynamicJsonTest::RunTest(const FString& Parameters)
{
	const FString LayoutJsonPath = FPaths::ProjectDir() / TEXT("UI_export/WBP_LocalTTS_DemoPanel_layout_mockup_dynamic.json");
	if (!FPaths::FileExists(LayoutJsonPath))
	{
		AddWarning(FString::Printf(TEXT("Skipping LocalTTS mockup dynamic JSON import test because file does not exist: %s"), *LayoutJsonPath));
		return true;
	}

	UWidgetBlueprint* WidgetBlueprint = UIJsonBridge::Tests::CreateTransientWidgetBlueprint();
	TestNotNull(TEXT("Transient Widget Blueprint is created"), WidgetBlueprint);
	if (!WidgetBlueprint)
	{
		return false;
	}

	FText Error;
	TestTrue(TEXT("LocalTTS mockup dynamic JSON imports"), FUIJsonBridgeImporter::ImportWidgetBlueprint(WidgetBlueprint, LayoutJsonPath, Error));
	TestNotNull(TEXT("Input_SingleText remains present"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("Input_SingleText")));
	TestNotNull(TEXT("Btn_StartService remains present"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("Btn_StartService")));
	TestNotNull(TEXT("Text_Erro compatibility variable exists"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("Text_Erro")));
	TestNotNull(TEXT("Top_Subtitle exists"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("Top_Subtitle")));

	const UTextBlock* TitleText = Cast<UTextBlock>(UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("Top_Subtitle")));
	TestNotNull(TEXT("Top_Subtitle is a TextBlock"), TitleText);
	if (TitleText)
	{
		const FString FontFilename = UIJsonBridge::Tests::GetPrimaryFontFilename(TitleText->GetFont());
		const UObject* FontObject = TitleText->GetFont().FontObject;
		TestNotNull(TEXT("CJK mixed text uses persistent UFont asset"), FontObject);
		TestTrue(TEXT("CJK mixed text uses UIJsonBridge font asset"),
			FontObject && FontObject->GetPathName().Contains(TEXT("/Game/UIJsonBridge/Fonts/UIJB_CJK_Mixed")));
		TestFalse(TEXT("CJK mixed text does not use DroidSansFallback"), FontFilename.Contains(TEXT("DroidSansFallback")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIJsonBridgeImporterLocalTtsFromScratchMockupJsonTest,
	"UIJsonBridge.Importer.ImportsLocalTtsFromScratchMockupJson",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIJsonBridgeImporterLocalTtsFromScratchMockupJsonTest::RunTest(const FString& Parameters)
{
	const FString LayoutJsonPath = FPaths::ProjectDir() / TEXT("UI_export/WBP_LocalTTS_DemoPanel_from_scratch_mockup_dynamic.json");
	if (!FPaths::FileExists(LayoutJsonPath))
	{
		AddWarning(FString::Printf(TEXT("Skipping LocalTTS from-scratch mockup JSON import test because file does not exist: %s"), *LayoutJsonPath));
		return true;
	}

	UWidgetBlueprint* WidgetBlueprint = UIJsonBridge::Tests::CreateTransientWidgetBlueprint();
	TestNotNull(TEXT("Transient Widget Blueprint is created"), WidgetBlueprint);
	if (!WidgetBlueprint)
	{
		return false;
	}

	FText Error;
	TestTrue(TEXT("LocalTTS from-scratch mockup JSON imports"), FUIJsonBridgeImporter::ImportWidgetBlueprint(WidgetBlueprint, LayoutJsonPath, Error));
	TestNotNull(TEXT("CanvasPanel_FromScratchRoot exists"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("CanvasPanel_FromScratchRoot")));
	TestNotNull(TEXT("Text_Erro compatibility variable exists"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("Text_Erro")));
	TestNotNull(TEXT("Top_Subtitle exists"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("Top_Subtitle")));
	TestNotNull(TEXT("TextBlock_1 exists"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("TextBlock_1")));

	const UTextBlock* TitleText = Cast<UTextBlock>(UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("Top_Subtitle")));
	TestNotNull(TEXT("Top_Subtitle is a TextBlock"), TitleText);
	if (TitleText)
	{
		const FString FontFilename = UIJsonBridge::Tests::GetPrimaryFontFilename(TitleText->GetFont());
		const UObject* FontObject = TitleText->GetFont().FontObject;
		TestNotNull(TEXT("CJK mixed text uses persistent UFont asset"), FontObject);
		TestTrue(TEXT("CJK mixed text uses UIJsonBridge font asset"),
			FontObject && FontObject->GetPathName().Contains(TEXT("/Game/UIJsonBridge/Fonts/UIJB_CJK_Mixed")));
		TestFalse(TEXT("CJK mixed text does not use DroidSansFallback"), FontFilename.Contains(TEXT("DroidSansFallback")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIJsonBridgeImporterFromScratchLayoutJsonTest,
	"UIJsonBridge.Importer.ImportsFromScratchLayoutJson",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIJsonBridgeImporterFromScratchLayoutJsonTest::RunTest(const FString& Parameters)
{
	const FString LayoutJsonPath = FPaths::ProjectDir() / TEXT("UI_export/WBP_Codex_FromScratch_layout.json");
	if (!FPaths::FileExists(LayoutJsonPath))
	{
		AddWarning(FString::Printf(TEXT("Skipping from-scratch layout import test because file does not exist: %s"), *LayoutJsonPath));
		return true;
	}

	UWidgetBlueprint* WidgetBlueprint = UIJsonBridge::Tests::CreateTransientWidgetBlueprint();
	TestNotNull(TEXT("Transient Widget Blueprint is created"), WidgetBlueprint);
	if (!WidgetBlueprint)
	{
		return false;
	}

	FText Error;
	TestTrue(TEXT("From-scratch layout JSON imports"), FUIJsonBridgeImporter::ImportWidgetBlueprint(WidgetBlueprint, LayoutJsonPath, Error));
	TestNotNull(TEXT("RootCanvas exists"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("RootCanvas")));
	TestNotNull(TEXT("Btn_Primary exists"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("Btn_Primary")));
	TestNotNull(TEXT("InputBox exists"), UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("InputBox")));

	const UTextBlock* TitleText = Cast<UTextBlock>(UIJsonBridge::Tests::FindWidgetByName(WidgetBlueprint, TEXT("TitleText")));
	TestNotNull(TEXT("TitleText exists"), TitleText);
	if (TitleText)
	{
		TestEqual(TEXT("Chinese text imports without mojibake"), TitleText->GetText().ToString(), FString(TEXT("Codex 布局导入演示")));
		TestTrue(TEXT("Chinese TextBlock receives valid fallback font"), TitleText->GetFont().HasValidFont());
	}
	return true;
}

#endif
