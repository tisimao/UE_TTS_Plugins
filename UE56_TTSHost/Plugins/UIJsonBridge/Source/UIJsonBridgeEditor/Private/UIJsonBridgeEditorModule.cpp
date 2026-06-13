#include "UIJsonBridgeEditorModule.h"

#include "AssetRegistry/AssetData.h"
#include "ContentBrowserMenuContexts.h"
#include "DesktopPlatformModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDesktopPlatform.h"
#include "Misc/Paths.h"
#include "ToolMenus.h"
#include "UIJsonBridgeExporter.h"
#include "UIJsonBridgeImporter.h"
#include "WidgetBlueprint.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "UIJsonBridgeEditor"

namespace UIJsonBridge
{
	static void ShowNotification(const FText& Message, SNotificationItem::ECompletionState State)
	{
		FNotificationInfo Info(Message);
		Info.bFireAndForget = true;
		Info.ExpireDuration = State == SNotificationItem::CS_Success ? 3.0f : 6.0f;

		if (TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info))
		{
			Notification->SetCompletionState(State);
		}
	}

	static bool PickOutputFile(const UWidgetBlueprint* WidgetBlueprint, EUIJsonBridgeExportProfile Profile, FString& OutFilePath)
	{
		OutFilePath = FUIJsonBridgeExporter::MakeDefaultExportFilePath(WidgetBlueprint, Profile);

		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (!DesktopPlatform)
		{
			return true;
		}

		TArray<FString> SaveFilenames;
		const bool bPicked = DesktopPlatform->SaveFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			*FString::Printf(TEXT("Export Widget Blueprint JSON (%s)"), *FUIJsonBridgeExporter::GetExportProfileDisplayName(Profile).ToString()),
			FPaths::GetPath(OutFilePath),
			FPaths::GetCleanFilename(OutFilePath),
			TEXT("JSON files (*.json)|*.json"),
			EFileDialogFlags::None,
			SaveFilenames);

		if (!bPicked || SaveFilenames.IsEmpty())
		{
			return false;
		}

		OutFilePath = SaveFilenames[0];
		if (FPaths::GetExtension(OutFilePath).IsEmpty())
		{
			OutFilePath += TEXT(".json");
		}
		return true;
	}

	static bool PickInputFile(FString& OutFilePath)
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (!DesktopPlatform)
		{
			return false;
		}

		TArray<FString> OpenFilenames;
		const bool bPicked = DesktopPlatform->OpenFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Import Widget Blueprint JSON"),
			FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("UIJsonBridge")),
			TEXT(""),
			TEXT("JSON files (*.json)|*.json"),
			EFileDialogFlags::None,
			OpenFilenames);

		if (!bPicked || OpenFilenames.IsEmpty())
		{
			return false;
		}

		OutFilePath = OpenFilenames[0];
		return true;
	}

	static void ExportSelectedWidgetBlueprints(TArray<FAssetData> SelectedAssets, EUIJsonBridgeExportProfile Profile)
	{
		int32 SuccessCount = 0;
		FString LastOutputPath;
		TArray<FText> Errors;

		for (const FAssetData& AssetData : SelectedAssets)
		{
			UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(AssetData.GetAsset());
			if (!WidgetBlueprint)
			{
				continue;
			}

			FString OutputPath;
			if (SelectedAssets.Num() == 1)
			{
				if (!PickOutputFile(WidgetBlueprint, Profile, OutputPath))
				{
					return;
				}
			}
			else
			{
				OutputPath = FUIJsonBridgeExporter::MakeDefaultExportFilePath(WidgetBlueprint, Profile);
			}

			FText Error;
			if (FUIJsonBridgeExporter::ExportWidgetBlueprint(WidgetBlueprint, OutputPath, Profile, Error))
			{
				++SuccessCount;
				LastOutputPath = OutputPath;
			}
			else
			{
				Errors.Add(Error);
			}
		}

		if (SuccessCount > 0)
		{
			if (!LastOutputPath.IsEmpty())
			{
				FPlatformApplicationMisc::ClipboardCopy(*LastOutputPath);
			}

			ShowNotification(
				FText::Format(
					LOCTEXT("ExportSuccess", "Exported {0} {1} Widget Blueprint JSON file(s). Last path copied to clipboard."),
					FText::AsNumber(SuccessCount),
					FUIJsonBridgeExporter::GetExportProfileDisplayName(Profile)),
				SNotificationItem::CS_Success);
		}

		if (!Errors.IsEmpty())
		{
			ShowNotification(Errors[0], SNotificationItem::CS_Fail);
		}
	}

	static void ImportIntoWidgetBlueprint(FAssetData AssetData)
	{
		UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(AssetData.GetAsset());
		if (!WidgetBlueprint)
		{
			ShowNotification(LOCTEXT("ImportNoWidgetBlueprint", "Selected asset is not a Widget Blueprint."), SNotificationItem::CS_Fail);
			return;
		}

		FString InputPath;
		if (!PickInputFile(InputPath))
		{
			return;
		}

		FText Error;
		if (FUIJsonBridgeImporter::ImportWidgetBlueprint(WidgetBlueprint, InputPath, Error))
		{
			ShowNotification(
				FText::Format(
					LOCTEXT("ImportSuccess", "Imported UI JSON into {0}."),
					FText::FromString(WidgetBlueprint->GetName())),
				SNotificationItem::CS_Success);
		}
		else
		{
			ShowNotification(Error, SNotificationItem::CS_Fail);
		}
	}
}

void FUIJsonBridgeEditorModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FUIJsonBridgeEditorModule::RegisterMenus));
}

void FUIJsonBridgeEditorModule::ShutdownModule()
{
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}
}

void FUIJsonBridgeEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu");
	if (!Menu)
	{
		return;
	}

	FToolMenuSection& Section = Menu->FindOrAddSection("CommonAssetActions");
	Section.AddDynamicEntry("UIJsonBridge_ExportWidgetBlueprint", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
	{
		const UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>();
		if (!Context)
		{
			return;
		}

		TArray<FAssetData> WidgetBlueprintAssets;
		for (const FAssetData& AssetData : Context->SelectedAssets)
		{
			if (AssetData.IsInstanceOf(UWidgetBlueprint::StaticClass()))
			{
				WidgetBlueprintAssets.Add(AssetData);
			}
		}

		if (WidgetBlueprintAssets.IsEmpty())
		{
			return;
		}

		InSection.AddMenuEntry(
			"UIJsonBridge_ExportWidgetBlueprintLayoutJson",
			LOCTEXT("ExportWidgetBlueprintLayoutJson", "Export UI JSON - Layout Only"),
			LOCTEXT("ExportWidgetBlueprintLayoutJsonTooltip", "Export only the import-friendly UMG hierarchy, slots, layout, styles, and resource references."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic(&UIJsonBridge::ExportSelectedWidgetBlueprints, WidgetBlueprintAssets, EUIJsonBridgeExportProfile::LayoutOnly)));

		InSection.AddMenuEntry(
			"UIJsonBridge_ExportWidgetBlueprintInteractionJson",
			LOCTEXT("ExportWidgetBlueprintInteractionJson", "Export UI JSON - Interaction"),
			LOCTEXT("ExportWidgetBlueprintInteractionJsonTooltip", "Export UMG hierarchy plus bindings, animations, and compact Blueprint graph metadata for Codex event analysis."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic(&UIJsonBridge::ExportSelectedWidgetBlueprints, WidgetBlueprintAssets, EUIJsonBridgeExportProfile::Interaction)));

		InSection.AddMenuEntry(
			"UIJsonBridge_ExportWidgetBlueprintFullJson",
			LOCTEXT("ExportWidgetBlueprintFullJson", "Export UI JSON - Full"),
			LOCTEXT("ExportWidgetBlueprintFullJsonTooltip", "Export layout, properties, bindings, animations, Blueprint graph nodes, pins, and links."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic(&UIJsonBridge::ExportSelectedWidgetBlueprints, WidgetBlueprintAssets, EUIJsonBridgeExportProfile::Full)));

		if (WidgetBlueprintAssets.Num() == 1)
		{
			InSection.AddMenuEntry(
				"UIJsonBridge_ImportWidgetBlueprintJson",
				LOCTEXT("ImportWidgetBlueprintJson", "Import UI JSON"),
				LOCTEXT("ImportWidgetBlueprintJsonTooltip", "Replace this Widget Blueprint's widget tree with a UI JSON layout."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&UIJsonBridge::ImportIntoWidgetBlueprint, WidgetBlueprintAssets[0])));
		}
	}));
}

IMPLEMENT_MODULE(FUIJsonBridgeEditorModule, UIJsonBridgeEditor)

#undef LOCTEXT_NAMESPACE
