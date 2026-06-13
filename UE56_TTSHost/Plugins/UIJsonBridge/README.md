# UI JSON Bridge

Editor-only UE plugin for exporting UMG Widget Blueprints to JSON and importing safe layout JSON back into Widget Blueprints.

中文完整文档：

```text
使用说明.md
```

Bundled Codex AI plugin:

```text
plugins/ui-json-bridge-ai
.agents/plugins/marketplace.json
```

## Export Profiles

The content browser menu exposes three export modes:

- `Export UI JSON - Layout Only`
  - Import-friendly.
  - Exports WidgetTree, widget classes, names, slots, layout, styles, and editable properties.
  - Does not export bindings, animations, or graphs.

- `Export UI JSON - Interaction`
  - Analysis-only.
  - Exports compact interaction data: bindings, animations, graph node metadata, event names, and function calls.
  - Marked as `importable=false`; the importer rejects it.

- `Export UI JSON - Full`
  - Deep analysis / backup.
  - Exports layout plus graph nodes, pins, links, comments, defaults, bindings, and animations.
  - Import currently only rebuilds the WidgetTree, not Blueprint Graphs.

Default filenames:

```text
WBP_Name_layout.json
WBP_Name_interaction.json
WBP_Name_full.json
```

## Import

`Import UI JSON` replaces the target Widget Blueprint's whole WidgetTree.

Imported:

- Widget hierarchy
- Widget classes and names
- Display labels
- Editable widget properties
- Panel slot properties
- Canvas slot layout

Not imported:

- Blueprint Graph nodes
- Event nodes
- Pin links
- Function graphs
- Animation timelines

The importer rejects `interaction` JSON and files marked `importable=false`.

## Safety

The importer is intentionally defensive:

- Preflights widget classes and duplicate names.
- Builds the new tree in a temporary WidgetTree first.
- Replaces the target tree only after successful construction.
- Moves old widgets to transient before rebuilding, preventing same-name/different-class UE fatal errors.
- Synchronizes `WidgetVariableNameToGuidMap` after import.
- Compiles the Widget Blueprint after import.

## Encoding And CJK Text

Exports are written as UTF-8 with BOM.

Imports are decoded explicitly as UTF-8 and support both BOM and no-BOM files.

If Details panel text is correct but the UMG canvas displays garbled Chinese, the issue is usually font coverage rather than JSON encoding. The importer now applies a CJK fallback font for common text widgets when CJK characters are detected and no valid font is set:

```text
Engine/Content/Slate/Fonts/DroidSansFallback.ttf
```

Covered widgets:

- `TextBlock`
- `EditableText`
- `MultiLineEditableText`

`MultiLineEditableTextBox` needs separate style handling in the future because UE 5.6 does not expose a simple public font setter for it.

## Tests

Run all plugin tests:

```bat
"C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\project\UETTsProject\UE56_TTSHost\UE56_TTSHost.uproject" -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation RunTests UIJsonBridge; Quit" -TestExit="Automation Test Queue Empty" -log
```

Important tests:

```text
UIJsonBridge.Exporter.ProfileFields
UIJsonBridge.Layout.RoundTripModifiedLayoutIntoFreshBlueprint
UIJsonBridge.Importer.ImportsFromScratchLayoutJson
UIJsonBridge.Importer.ImportsProjectResponsiveSafeJson
UIJsonBridge.Importer.ReplacesSameNamedDifferentClassWidgets
UIJsonBridge.Importer.FailedImportPreservesExistingTree
```

Manual from-scratch layout test file:

```text
UE56_TTSHost/UI_export/WBP_Codex_FromScratch_layout.json
```
