#include "UIJsonBridgeImporter.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/EditableText.h"
#include "Components/MultiLineEditableText.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/Widget.h"
#include "Containers/StringConv.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/SlateFontInfo.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "ScopedTransaction.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SoftObjectPath.h"
#include "Animation/WidgetAnimation.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "UIJsonBridgeImporter"

namespace UIJsonBridge
{
	static bool LoadUtf8FileToString(const FString& FilePath, FString& OutString)
	{
		TArray<uint8> FileBytes;
		if (!FFileHelper::LoadFileToArray(FileBytes, *FilePath))
		{
			return false;
		}

		int32 Offset = 0;
		if (FileBytes.Num() >= 3 && FileBytes[0] == 0xEF && FileBytes[1] == 0xBB && FileBytes[2] == 0xBF)
		{
			Offset = 3;
		}

		const int32 ByteCount = FileBytes.Num() - Offset;
		const ANSICHAR* Utf8Data = ByteCount > 0 ? reinterpret_cast<const ANSICHAR*>(FileBytes.GetData() + Offset) : "";
		FUTF8ToTCHAR Converter(Utf8Data, ByteCount);
		OutString = FString(Converter.Length(), Converter.Get());
		return true;
	}

	static TSharedPtr<FJsonObject> GetOptionalObjectField(const TSharedPtr<FJsonObject>& Json, const TCHAR* FieldName)
	{
		const TSharedPtr<FJsonObject>* ObjectJson = nullptr;
		if (Json.IsValid() && Json->TryGetObjectField(FieldName, ObjectJson) && ObjectJson)
		{
			return *ObjectJson;
		}
		return nullptr;
	}

	static TMap<FName, FString> ReadPropertyMap(const TSharedPtr<FJsonObject>& Json)
	{
		TMap<FName, FString> Properties;
		if (!Json.IsValid())
		{
			return Properties;
		}

		for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : Json->Values)
		{
			if (Field.Key.StartsWith(TEXT("UIJsonBridge")))
			{
				continue;
			}

			FString ValueText;
			if (Field.Value.IsValid() && Field.Value->TryGetString(ValueText))
			{
				Properties.Add(FName(*Field.Key), ValueText);
			}
		}
		return Properties;
	}

	static UClass* ResolveWidgetClass(const TSharedPtr<FJsonObject>& WidgetJson)
	{
		if (!WidgetJson.IsValid())
		{
			return nullptr;
		}

		FString ClassPath;
		if (WidgetJson->TryGetStringField(TEXT("classPath"), ClassPath) && !ClassPath.IsEmpty())
		{
			if (UClass* LoadedClass = Cast<UClass>(FSoftObjectPath(ClassPath).TryLoad()))
			{
				return LoadedClass;
			}
			if (UClass* FoundClass = FindObject<UClass>(nullptr, *ClassPath))
			{
				return FoundClass;
			}
		}

		FString ClassName;
		if (WidgetJson->TryGetStringField(TEXT("class"), ClassName) && !ClassName.IsEmpty())
		{
			for (TObjectIterator<UClass> It; It; ++It)
			{
				UClass* Class = *It;
				if (Class->GetName() == ClassName && Class->IsChildOf(UWidget::StaticClass()))
				{
					return Class;
				}
			}
		}

		return nullptr;
	}

	static bool ValidateWidgetClass(const UClass* WidgetClass, FText& OutError)
	{
		if (!WidgetClass)
		{
			OutError = LOCTEXT("MissingWidgetClass", "The JSON contains a widget with a class that could not be found.");
			return false;
		}

		if (!WidgetClass->IsChildOf(UWidget::StaticClass()) || WidgetClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
		{
			OutError = FText::Format(
				LOCTEXT("InvalidWidgetClass", "{0} is not a usable UMG widget class."),
				FText::FromString(WidgetClass->GetPathName()));
			return false;
		}

		return true;
	}

	static void ApplyProperties(UObject* Object, const TSharedPtr<FJsonObject>& PropertiesJson)
	{
		const TMap<FName, FString> Properties = ReadPropertyMap(PropertiesJson);
		FWidgetBlueprintEditorUtils::ImportPropertiesFromText(Object, Properties);
	}

	static bool ContainsCjkText(const FString& Text)
	{
		for (const TCHAR Character : Text)
		{
			const uint32 Codepoint = static_cast<uint32>(Character);
			if ((Codepoint >= 0x2E80 && Codepoint <= 0x9FFF) ||
				(Codepoint >= 0xF900 && Codepoint <= 0xFAFF) ||
				(Codepoint >= 0x3040 && Codepoint <= 0x30FF) ||
				(Codepoint >= 0xAC00 && Codepoint <= 0xD7AF))
			{
				return true;
			}
		}
		return false;
	}

	static bool JsonContainsCjkText(const TSharedPtr<FJsonObject>& PropertiesJson)
	{
		if (!PropertiesJson.IsValid())
		{
			return false;
		}

		for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : PropertiesJson->Values)
		{
			FString ValueText;
			if (Field.Value.IsValid() && Field.Value->TryGetString(ValueText) && ContainsCjkText(ValueText))
			{
				return true;
			}
		}
		return false;
	}

	static FString GetPreferredCjkFontFile(const bool bBold)
	{
		const FString BoldYaHeiPath = TEXT("C:/Windows/Fonts/msyhbd.ttc");
		const FString RegularYaHeiPath = TEXT("C:/Windows/Fonts/msyh.ttc");
		if (bBold && FPaths::FileExists(BoldYaHeiPath))
		{
			return BoldYaHeiPath;
		}
		if (FPaths::FileExists(RegularYaHeiPath))
		{
			return RegularYaHeiPath;
		}

		const FString SimHeiPath = TEXT("C:/Windows/Fonts/simhei.ttf");
		if (FPaths::FileExists(SimHeiPath))
		{
			return SimHeiPath;
		}

		return FPaths::EngineContentDir() / TEXT("Slate/Fonts/DroidSansFallback.ttf");
	}

	static UFont* GetOrCreateCjkFontAsset(const bool bBold)
	{
		const TCHAR* FontAssetPath = bBold
			? TEXT("/Game/UIJsonBridge/Fonts/UIJB_CJK_Mixed_Bold.UIJB_CJK_Mixed_Bold")
			: TEXT("/Game/UIJsonBridge/Fonts/UIJB_CJK_Mixed.UIJB_CJK_Mixed");
		if (UFont* ExistingFont = LoadObject<UFont>(nullptr, FontAssetPath))
		{
			if (ExistingFont->FontCacheType == EFontCacheType::Runtime &&
				ExistingFont->CompositeFont.DefaultTypeface.Fonts.Num() > 0)
			{
				return ExistingFont;
			}
		}

		const FString FontFile = GetPreferredCjkFontFile(bBold);
		TArray<uint8> FontBytes;
		if (!FPaths::FileExists(FontFile) || !FFileHelper::LoadFileToArray(FontBytes, *FontFile))
		{
			return nullptr;
		}

		const FString FontPackageName = bBold
			? TEXT("/Game/UIJsonBridge/Fonts/UIJB_CJK_Mixed_Bold")
			: TEXT("/Game/UIJsonBridge/Fonts/UIJB_CJK_Mixed");
		const FString FontFacePackageName = bBold
			? TEXT("/Game/UIJsonBridge/Fonts/UIJB_CJK_Mixed_Bold_Face")
			: TEXT("/Game/UIJsonBridge/Fonts/UIJB_CJK_Mixed_Face");
		const FName FontObjectName = bBold ? TEXT("UIJB_CJK_Mixed_Bold") : TEXT("UIJB_CJK_Mixed");
		const FName FontFaceObjectName = bBold ? TEXT("UIJB_CJK_Mixed_Bold_Face") : TEXT("UIJB_CJK_Mixed_Face");

		UPackage* FontFacePackage = CreatePackage(*FontFacePackageName);
		UFontFace* FontFace = FindObject<UFontFace>(FontFacePackage, *FontFaceObjectName.ToString());
		if (!FontFace)
		{
			FontFace = NewObject<UFontFace>(FontFacePackage, FontFaceObjectName, RF_Public | RF_Standalone | RF_Transactional);
			FAssetRegistryModule::AssetCreated(FontFace);
		}
		FontFace->SourceFilename = FontFile;
		FontFace->Hinting = EFontHinting::Default;
		FontFace->LoadingPolicy = EFontLoadingPolicy::Inline;
		FontFace->FontFaceData->SetData(MoveTemp(FontBytes));
#if WITH_EDITORONLY_DATA
		FontFace->CacheSubFaces();
#endif
		FontFacePackage->MarkPackageDirty();

		UPackage* FontPackage = CreatePackage(*FontPackageName);
		UFont* Font = FindObject<UFont>(FontPackage, *FontObjectName.ToString());
		if (!Font)
		{
			Font = NewObject<UFont>(FontPackage, FontObjectName, RF_Public | RF_Standalone | RF_Transactional);
			FAssetRegistryModule::AssetCreated(Font);
		}

		Font->FontCacheType = EFontCacheType::Runtime;
		Font->LegacyFontSize = 24;
		Font->LegacyFontName = NAME_None;
		Font->CompositeFont.DefaultTypeface.Fonts.Reset();
		FTypefaceEntry& DefaultTypefaceEntry = Font->CompositeFont.DefaultTypeface.Fonts.AddDefaulted_GetRef();
		DefaultTypefaceEntry.Name = NAME_None;
		DefaultTypefaceEntry.Font = FFontData(FontFace);
		FontPackage->MarkPackageDirty();

		const FString FontFaceFilename = FPackageName::LongPackageNameToFilename(FontFacePackageName, FPackageName::GetAssetPackageExtension());
		const FString FontFilename = FPackageName::LongPackageNameToFilename(FontPackageName, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(FontFacePackage, FontFace, *FontFaceFilename, SaveArgs);
		UPackage::SavePackage(FontPackage, Font, *FontFilename, SaveArgs);

		return Font;
	}

	static FSlateFontInfo MakeCjkFallbackFont(const FSlateFontInfo& CurrentFont)
	{
		const int32 FontSize = CurrentFont.Size > 0 ? CurrentFont.Size : 24;
		const bool bWantsBold = CurrentFont.TypefaceFontName.ToString().Contains(TEXT("Bold"), ESearchCase::IgnoreCase);
		if (UFont* FontAsset = GetOrCreateCjkFontAsset(bWantsBold))
		{
			return FSlateFontInfo(FontAsset, FontSize, NAME_None, CurrentFont.OutlineSettings);
		}

		return FSlateFontInfo(MakeShared<FStandaloneCompositeFont>(NAME_None, GetPreferredCjkFontFile(bWantsBold), EFontHinting::Default, EFontLoadingPolicy::LazyLoad), FontSize, NAME_None, CurrentFont.OutlineSettings);
	}

	static void ApplyCjkFallbackFontIfNeeded(UWidget* Widget, const TSharedPtr<FJsonObject>& PropertiesJson)
	{
		if (!JsonContainsCjkText(PropertiesJson))
		{
			return;
		}

		if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			TextBlock->SetFont(MakeCjkFallbackFont(TextBlock->GetFont()));
		}
		else if (UEditableText* EditableText = Cast<UEditableText>(Widget))
		{
			EditableText->SetFont(MakeCjkFallbackFont(EditableText->GetFont()));
		}
		else if (UMultiLineEditableText* MultiLineEditableText = Cast<UMultiLineEditableText>(Widget))
		{
			MultiLineEditableText->SetFont(MakeCjkFallbackFont(MultiLineEditableText->GetFont()));
		}
		else if (UEditableTextBox* EditableTextBox = Cast<UEditableTextBox>(Widget))
		{
			EditableTextBox->WidgetStyle.SetFont(MakeCjkFallbackFont(EditableTextBox->WidgetStyle.TextStyle.Font));
		}
		else if (UMultiLineEditableTextBox* MultiLineEditableTextBox = Cast<UMultiLineEditableTextBox>(Widget))
		{
			MultiLineEditableTextBox->WidgetStyle.SetFont(MakeCjkFallbackFont(MultiLineEditableTextBox->WidgetStyle.TextStyle.Font));
		}
	}

	static FSlateBrush MakeRoundedColorBrush(const FLinearColor& FillColor, const FLinearColor& OutlineColor, float OutlineWidth = 1.0f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
		Brush.OutlineSettings.Width = OutlineWidth;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.CornerRadii = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		Brush.SetImageSize(FVector2D(32.0f, 32.0f));
		return Brush;
	}

	static void ApplyButtonStyleRoleIfNeeded(UWidget* Widget, const TSharedPtr<FJsonObject>& PropertiesJson)
	{
		UButton* Button = Cast<UButton>(Widget);
		if (!Button || !PropertiesJson.IsValid())
		{
			return;
		}

		FString StyleRole;
		if (!PropertiesJson->TryGetStringField(TEXT("UIJsonBridgeStyleRole"), StyleRole) || StyleRole.IsEmpty())
		{
			return;
		}

		FLinearColor NormalFill(0.06f, 0.38f, 0.47f, 0.80f);
		FLinearColor HoverFill(0.10f, 0.45f, 0.55f, 0.92f);
		FLinearColor PressedFill(0.03f, 0.34f, 0.43f, 0.96f);
		FLinearColor Outline(0.12f, 0.82f, 1.0f, 0.55f);
		FLinearColor Foreground(0.78f, 1.0f, 0.95f, 1.0f);

		if (StyleRole.Equals(TEXT("Alt"), ESearchCase::IgnoreCase))
		{
			NormalFill = FLinearColor(0.18f, 0.11f, 0.37f, 0.80f);
			HoverFill = FLinearColor(0.22f, 0.18f, 0.45f, 0.92f);
			PressedFill = FLinearColor(0.15f, 0.07f, 0.33f, 0.96f);
			Outline = FLinearColor(0.56f, 0.42f, 1.0f, 0.55f);
			Foreground = FLinearColor(0.92f, 0.95f, 1.0f, 1.0f);
		}
		else if (StyleRole.Equals(TEXT("Stop"), ESearchCase::IgnoreCase))
		{
			NormalFill = FLinearColor(0.34f, 0.10f, 0.15f, 0.78f);
			HoverFill = FLinearColor(0.38f, 0.17f, 0.23f, 0.92f);
			PressedFill = FLinearColor(0.31f, 0.06f, 0.11f, 0.96f);
			Outline = FLinearColor(1.0f, 0.36f, 0.46f, 0.55f);
			Foreground = FLinearColor(1.0f, 0.92f, 0.94f, 1.0f);
		}

		FButtonStyle Style = Button->GetStyle();
		Style.SetNormal(MakeRoundedColorBrush(NormalFill, Outline));
		Style.SetHovered(MakeRoundedColorBrush(HoverFill, Outline));
		Style.SetPressed(MakeRoundedColorBrush(PressedFill, Outline));
		Style.SetDisabled(MakeRoundedColorBrush(FLinearColor(0.05f, 0.05f, 0.06f, 0.35f), FLinearColor(0.4f, 0.4f, 0.4f, 0.25f)));
		Style.SetNormalForeground(FSlateColor(Foreground));
		Style.SetHoveredForeground(FSlateColor(Foreground));
		Style.SetPressedForeground(FSlateColor(Foreground));
		Style.SetDisabledForeground(FSlateColor(FLinearColor(0.45f, 0.45f, 0.45f, 1.0f)));
		Style.SetNormalPadding(FMargin(10.0f, 4.0f));
		Style.SetPressedPadding(FMargin(10.0f, 5.0f, 10.0f, 3.0f));
		Button->SetStyle(Style);
	}

	static UWidget* BuildWidgetTree(UWidgetTree* WidgetTree, const TSharedPtr<FJsonObject>& WidgetJson, FText& OutError)
	{
		if (!WidgetTree || !WidgetJson.IsValid())
		{
			OutError = LOCTEXT("InvalidWidgetJson", "The JSON does not contain a valid rootWidget object.");
			return nullptr;
		}

		UClass* WidgetClass = ResolveWidgetClass(WidgetJson);
		if (!ValidateWidgetClass(WidgetClass, OutError))
		{
			return nullptr;
		}

		FString WidgetName;
		WidgetJson->TryGetStringField(TEXT("name"), WidgetName);
		const FName NewWidgetName = WidgetName.IsEmpty() ? NAME_None : FName(*WidgetName);

		UWidget* Widget = WidgetTree->ConstructWidget<UWidget>(WidgetClass, NewWidgetName);
		if (!Widget)
		{
			OutError = FText::Format(
				LOCTEXT("CreateWidgetFailed", "Failed to create widget {0}."),
				FText::FromString(WidgetClass->GetPathName()));
			return nullptr;
		}

		Widget->SetFlags(RF_Transactional);
		const TSharedPtr<FJsonObject> WidgetPropertiesJson = GetOptionalObjectField(WidgetJson, TEXT("properties"));
		ApplyProperties(Widget, WidgetPropertiesJson);
		ApplyCjkFallbackFontIfNeeded(Widget, WidgetPropertiesJson);
		ApplyButtonStyleRoleIfNeeded(Widget, WidgetPropertiesJson);

		FString DisplayLabel;
		if (WidgetJson->TryGetStringField(TEXT("displayLabel"), DisplayLabel) && !DisplayLabel.IsEmpty())
		{
			Widget->SetDisplayLabel(DisplayLabel);
		}

		if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			const TArray<TSharedPtr<FJsonValue>>* Children = nullptr;
			if (WidgetJson->TryGetArrayField(TEXT("children"), Children) && Children)
			{
				for (const TSharedPtr<FJsonValue>& ChildValue : *Children)
				{
					const TSharedPtr<FJsonObject> ChildJson = ChildValue.IsValid() ? ChildValue->AsObject() : nullptr;
					if (!ChildJson.IsValid())
					{
						continue;
					}

					UWidget* ChildWidget = BuildWidgetTree(WidgetTree, ChildJson, OutError);
					if (!ChildWidget)
					{
						return nullptr;
					}

					UPanelSlot* Slot = Panel->AddChild(ChildWidget);
					if (!Slot)
					{
						OutError = FText::Format(
							LOCTEXT("AddChildFailed", "Failed to add {0} to {1}."),
							FText::FromString(ChildWidget->GetName()),
							FText::FromString(Widget->GetName()));
						return nullptr;
					}

					Slot->SetFlags(RF_Transactional);
					const TSharedPtr<FJsonObject>* SlotJson = nullptr;
					if (ChildJson->TryGetObjectField(TEXT("slot"), SlotJson) && SlotJson && SlotJson->IsValid())
					{
						ApplyProperties(Slot, GetOptionalObjectField(*SlotJson, TEXT("properties")));
					}
				}
			}
		}

		return Widget;
	}

	static bool ValidateWidgetTreeJson(const TSharedPtr<FJsonObject>& WidgetJson, TSet<FName>& SeenNames, FText& OutError)
	{
		if (!WidgetJson.IsValid())
		{
			OutError = LOCTEXT("InvalidWidgetJsonPreflight", "The JSON contains an invalid widget object.");
			return false;
		}

		UClass* WidgetClass = ResolveWidgetClass(WidgetJson);
		if (!ValidateWidgetClass(WidgetClass, OutError))
		{
			return false;
		}

		FString WidgetName;
		WidgetJson->TryGetStringField(TEXT("name"), WidgetName);
		if (!WidgetName.IsEmpty())
		{
			const FName Name(*WidgetName);
			if (SeenNames.Contains(Name))
			{
				OutError = FText::Format(
					LOCTEXT("DuplicateWidgetName", "The JSON contains duplicate widget name {0}."),
					FText::FromString(WidgetName));
				return false;
			}
			SeenNames.Add(Name);
		}

		const TArray<TSharedPtr<FJsonValue>>* Children = nullptr;
		if (WidgetJson->TryGetArrayField(TEXT("children"), Children) && Children)
		{
			if (Children->Num() > 0 && !WidgetClass->IsChildOf(UPanelWidget::StaticClass()))
			{
				OutError = FText::Format(
					LOCTEXT("NonPanelHasChildren", "Widget {0} has children, but {1} cannot contain child widgets."),
					FText::FromString(WidgetName.IsEmpty() ? WidgetClass->GetName() : WidgetName),
					FText::FromString(WidgetClass->GetName()));
				return false;
			}

			for (const TSharedPtr<FJsonValue>& ChildValue : *Children)
			{
				const TSharedPtr<FJsonObject> ChildJson = ChildValue.IsValid() ? ChildValue->AsObject() : nullptr;
				if (!ValidateWidgetTreeJson(ChildJson, SeenNames, OutError))
				{
					return false;
				}
			}
		}

		return true;
	}

	static bool RenameWidgetTreeObjects(UWidgetTree* SourceTree, UObject* NewOuter, FText& OutError)
	{
		if (!SourceTree || !NewOuter)
		{
			return true;
		}

		TArray<UWidget*> Widgets;
		SourceTree->GetAllWidgets(Widgets);

		for (UWidget* Widget : Widgets)
		{
			if (!Widget)
			{
				continue;
			}

			if (!Widget->Rename(nullptr, NewOuter, REN_DontCreateRedirectors | REN_ForceNoResetLoaders))
			{
				OutError = FText::Format(
					LOCTEXT("RenameWidgetFailed", "Failed to move widget {0} into the target WidgetTree."),
					FText::FromString(Widget->GetName()));
				return false;
			}
		}

		return true;
	}

	static void MoveExistingWidgetsOutOfTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return;
		}

		TArray<UWidget*> ExistingWidgets;
		WidgetBlueprint->WidgetTree->GetAllWidgets(ExistingWidgets);

		WidgetBlueprint->WidgetTree->RootWidget = nullptr;
		WidgetBlueprint->WidgetTree->NamedSlotBindings.Empty();

		const FString ImportId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
		for (UWidget* Widget : ExistingWidgets)
		{
			if (!Widget)
			{
				continue;
			}

			Widget->RemoveFromParent();
			const FName StaleName = MakeUniqueObjectName(
				GetTransientPackage(),
				Widget->GetClass(),
				FName(*FString::Printf(TEXT("UIJsonBridge_Stale_%s_%s"), *ImportId, *Widget->GetName())));
			Widget->Rename(*StaleName.ToString(), GetTransientPackage(), REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
		}
	}

	static TMap<FName, UClass*> CaptureExistingVariableWidgets(UWidgetBlueprint* WidgetBlueprint)
	{
		TMap<FName, UClass*> VariableWidgets;
		if (!WidgetBlueprint)
		{
			return VariableWidgets;
		}

		WidgetBlueprint->ForEachSourceWidget([&VariableWidgets](UWidget* Widget)
		{
			if (Widget && Widget->bIsVariable)
			{
				VariableWidgets.Add(Widget->GetFName(), Widget->GetClass());
			}
		});

		return VariableWidgets;
	}

	static void RestoreExistingVariableFlags(UWidgetTree* WidgetTree, const TMap<FName, UClass*>& ExistingVariableWidgets)
	{
		if (!WidgetTree || ExistingVariableWidgets.IsEmpty())
		{
			return;
		}

		TArray<UWidget*> Widgets;
		WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (!Widget)
			{
				continue;
			}

			UClass* const* ExistingClass = ExistingVariableWidgets.Find(Widget->GetFName());
			if (ExistingClass && *ExistingClass == Widget->GetClass())
			{
				Widget->bIsVariable = true;
			}
		}
	}

	static void SynchronizeWidgetVariableGuids(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint)
		{
			return;
		}

		TSet<FName> CurrentWidgetNames;
		WidgetBlueprint->ForEachSourceWidget([&CurrentWidgetNames](UWidget* Widget)
		{
			if (Widget)
			{
				CurrentWidgetNames.Add(Widget->GetFName());
			}
		});

		for (UWidgetAnimation* Animation : WidgetBlueprint->Animations)
		{
			if (Animation)
			{
				CurrentWidgetNames.Add(Animation->GetFName());
			}
		}

		for (auto It = WidgetBlueprint->WidgetVariableNameToGuidMap.CreateIterator(); It; ++It)
		{
			if (!CurrentWidgetNames.Contains(It.Key()))
			{
				It.RemoveCurrent();
			}
		}

		for (const FName& WidgetName : CurrentWidgetNames)
		{
			if (!WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(WidgetName))
			{
				WidgetBlueprint->WidgetVariableNameToGuidMap.Add(WidgetName, FGuid::NewGuid());
			}
		}
	}
}

bool FUIJsonBridgeImporter::ImportWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint, const FString& InputFilePath, FText& OutError)
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

	FString InputJson;
	if (!UIJsonBridge::LoadUtf8FileToString(InputFilePath, InputJson))
	{
		OutError = FText::Format(
			LOCTEXT("ReadFailed", "Failed to read JSON file {0}."),
			FText::FromString(InputFilePath));
		return false;
	}

	TSharedPtr<FJsonObject> RootJson;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InputJson);
	if (!FJsonSerializer::Deserialize(Reader, RootJson) || !RootJson.IsValid())
	{
		OutError = LOCTEXT("ParseFailed", "Failed to parse UI JSON.");
		return false;
	}

	FString Profile;
	if (RootJson->TryGetStringField(TEXT("profile"), Profile) && Profile == TEXT("interaction"))
	{
		OutError = LOCTEXT("InteractionProfileNotImportable", "Interaction profile JSON is for analysis only. Please import a Layout Only or Full UI JSON file.");
		return false;
	}

	bool bImportable = true;
	if (RootJson->TryGetBoolField(TEXT("importable"), bImportable) && !bImportable)
	{
		OutError = LOCTEXT("JsonMarkedNotImportable", "This UI JSON file is marked as not importable. Please export a Layout Only or Full UI JSON file.");
		return false;
	}

	const TSharedPtr<FJsonObject>* RootWidgetJson = nullptr;
	if (!RootJson->TryGetObjectField(TEXT("rootWidget"), RootWidgetJson) || !RootWidgetJson || !RootWidgetJson->IsValid())
	{
		OutError = LOCTEXT("MissingRootWidget", "The JSON does not contain rootWidget.");
		return false;
	}

	TSet<FName> SeenWidgetNames;
	if (!UIJsonBridge::ValidateWidgetTreeJson(*RootWidgetJson, SeenWidgetNames, OutError))
	{
		return false;
	}

	UWidgetTree* ImportTree = NewObject<UWidgetTree>(GetTransientPackage(), NAME_None, RF_Transient);
	if (!ImportTree)
	{
		OutError = LOCTEXT("CreateImportTreeFailed", "Failed to create a temporary WidgetTree for import.");
		return false;
	}

	const TMap<FName, UClass*> ExistingVariableWidgets = UIJsonBridge::CaptureExistingVariableWidgets(WidgetBlueprint);

	UWidget* NewRootWidget = UIJsonBridge::BuildWidgetTree(ImportTree, *RootWidgetJson, OutError);
	if (!NewRootWidget)
	{
		return false;
	}
	ImportTree->RootWidget = NewRootWidget;

	const FScopedTransaction Transaction(LOCTEXT("ImportUIJsonTransaction", "Import UI JSON"));
	WidgetBlueprint->Modify();
	WidgetBlueprint->WidgetTree->SetFlags(RF_Transactional);
	WidgetBlueprint->WidgetTree->Modify();
	UIJsonBridge::MoveExistingWidgetsOutOfTree(WidgetBlueprint);

	if (!UIJsonBridge::RenameWidgetTreeObjects(ImportTree, WidgetBlueprint->WidgetTree, OutError))
	{
		return false;
	}

	WidgetBlueprint->WidgetTree->RootWidget = NewRootWidget;

	UIJsonBridge::RestoreExistingVariableFlags(WidgetBlueprint->WidgetTree, ExistingVariableWidgets);
	UIJsonBridge::SynchronizeWidgetVariableGuids(WidgetBlueprint);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	return true;
}

#undef LOCTEXT_NAMESPACE
