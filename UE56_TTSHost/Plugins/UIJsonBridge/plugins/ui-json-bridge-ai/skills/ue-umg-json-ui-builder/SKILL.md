---
name: ue-umg-json-ui-builder
description: Build, refine, or repair Unreal Engine UMG Widget Blueprint JSON layouts exported through UIJsonBridge. Use when the user says they want to make, create, design, iterate, or produce "UE UI", "UE's UI", "UE的UI", "制作UE的UI", "制作 UE 的 UI", "做UE界面", or Unreal/UMG UI; also use for UE/UMG JSON UI files, parent UI with child/sub UI references, UserWidget composition, reference mockup images or HTML, CJK text rendering, Blueprint variable preservation, SizeBox text constraints, responsive HUD-style layouts, or import crash/garbled-text fixes.
---

# UE UMG JSON UI Builder

Use this workflow when creating or iterating a UE UMG interface by editing UIJsonBridge JSON and importing it into a Widget Blueprint.

## Core Rules

- Preserve existing widget `name`, `class`, and `isVariable` for all event-facing Blueprint variables unless the user explicitly allows breaking Blueprint references.
- Treat both top-level `isVariable` and property `bIsVariable` as part of the Blueprint contract. If an existing Blueprint-accessed widget is variable in the source export, keep both set to true in generated JSON.
- Do not rename typo variables that already exist in Blueprint graphs, such as `Text_Erro`; compatibility beats cleanup.
- Prefer hierarchical region layout over flat widgets: root stage, top bar, main columns, bottom panels, then repeated rows/items.
- Match the reference mockup by layout first, text containers second, typography third, and decorative effects last.
- Keep dynamic effects minimal and intentional. If the user only wants a status effect, do not add unrelated animated glows, pulses, or sweeping elements.
- Avoid creating endless output variants. Modify the user-specified JSON in place unless they ask for a new test file.

## Reference Intake

When given a mockup image, HTML, and an exported JSON:

1. Inspect the mockup visually for region proportions, hierarchy, text density, and overflow points.
2. Inspect the HTML/CSS only as a design source, not as exact UE sizing truth.
3. Inspect the original exported JSON for variable names, widget classes, and event-bound controls.
4. Build a target map of panels and controls before editing JSON.

CSS pixel font sizes usually render too large after UE import. Use CSS sizes as relative hierarchy only.

## Text And Typography

Do not solve all text problems by repeatedly changing font sizes. Use explicit text regions.

- Wrap risky text controls in a `SizeBox` frame when they can overflow: titles, subtitles, input text, queue rows, status values, debug logs, diagnostic values, paths, request IDs, and long CJK paragraphs.
- Keep the original named/variable text widget inside the frame so Blueprint references survive.
- Put the original CanvasPanel slot on the outer `SizeBox`; give the inner text widget a `SizeBoxSlot`.
- Set the outer `SizeBox` and inner text widget `Clipping` to `ClipToBounds` for bounded UI text.
- Use `AutoWrapText=True` and `WrapTextAt` for paragraph-like content.
- Use `AutoWrapText=False` and clipping for one-line HUD labels, values, and compact status fields.
- Use restrained UE font sizes for dense HUD UI. A useful starting scale:
  - hero title: 32-34
  - panel title: 18-20
  - metric value: 20-22
  - important state value: 18-20
  - paragraph/input: 13-15
  - buttons: 11-12
  - labels/logs/queue rows: 10-12

## CJK Text

For Chinese/Japanese/Korean UI text:

- Use persistent UE font assets that support CJK glyphs.
- Prefer project-local fallback font assets such as `/Game/UIJsonBridge/Fonts/UIJB_CJK_Mixed` and `/Game/UIJsonBridge/Fonts/UIJB_CJK_Mixed_Bold` when available.
- Do not allow JSON to fall back to `DroidSansFallback` or `FontObject=None` if the project has known-good CJK font assets.
- Keep JSON encoded as UTF-8, preferably preserving an existing BOM if the file already has one.
- If viewport text is garbled but the details panel text is correct, suspect font asset/glyph resolution before suspecting JSON text encoding.

## Layout Strategy

For a reference-driven HUD/tool UI:

- Define stable panel bounds first, usually with CanvasPanelSlot `LayoutData` or equivalent parent layout constraints.
- Use separate background/border widgets for panel styling, then place text and controls inside predictable bounds.
- Use `SizeBox` for fixed text regions rather than relying on desired size.
- Keep buttons in stable-size slots so labels cannot resize the layout.
- For dense operational tools, avoid marketing-style hero sections, oversized cards, and decorative-only spacing.
- If importing into an existing Widget Blueprint, keep existing variables and replace only the visual tree needed for the requested UI.

## Parent And Child UI Composition

When the UI uses child widgets/sub UIs, treat the parent as layout orchestration and each child UI as a local feature module.

Ask for or look for these inputs:

- Parent UI JSON export.
- Child UI JSON exports for every referenced child widget that might be modified.
- A full-page reference image or HTML showing child UI positions in the parent.
- Child-specific reference images or instructions if child internals should change.
- A clear list of child UIs that should be referenced only and not expanded or edited.

Use these rules:

- Keep child UIs as independent `WBP_*` assets when they are reused, have their own logic, or expose their own events.
- Do not flatten child UI internals into the parent JSON unless the user explicitly requests a fully expanded one-file layout.
- In the parent JSON, preserve the child widget variable name and class/path so Blueprint graph references survive.
- Let the parent control position, size, anchoring, and visibility of the child widget.
- Let the child UI control its internal controls, text frames, bindings, events, and local layout.
- If a child UI must be edited, modify its own exported JSON separately and preserve its internal variable names.
- Import child UIs before importing the parent UI so referenced classes/assets already exist.
- When only a mockup image is provided, use it for visual placement but warn that child Blueprint variables and bindings cannot be verified without the child JSON.
- When only child JSON is provided, preserve behavior safely but ask for or infer visual placement from the parent/reference.

Useful user request pattern:

```text
Make a UE UI. Parent: WBP_MainPanel.json. Child UIs: WBP_QueueRow.json and WBP_StatusCard.json. Reference: Mockup.png. In the parent, reference the child UIs and do not expand WBP_QueueRow internals. WBP_StatusCard may be modified. Preserve all variable names.
```

## Known Failure: Existing Widget But Missing Blueprint Variable

If UE import succeeds but Blueprint compile reports missing widget variables, do not assume the widget was deleted. First check whether the widget exists in JSON but lost its variable flags.

Required variable shape:

```json
"isVariable": true,
"properties": {
  "bIsVariable": "True"
}
```

Failure pattern:

- The widget `name` still exists in JSON.
- The widget `class` is unchanged.
- Blueprint graph compile fails because the widget is no longer exposed as a Widget Blueprint variable.
- The broken JSON has `isVariable=false`, missing `isVariable`, or `bIsVariable=False`.

Prevention:

- Always compare the generated JSON against the latest known-good exported JSON before finishing.
- Build a variable table from the source export: `name`, `class`, `isVariable`, `properties.bIsVariable`.
- For every source-variable widget, require the generated JSON to preserve the same `name`, same `class`, `isVariable=true`, and `bIsVariable=True`.
- Treat this check as mandatory after wrapping text widgets in `SizeBox`, replacing visual structure, or regenerating a layout from a mockup.
- If a variable was intentionally removed, state that explicitly in the final answer.

## Import Safety

Before finishing:

1. Parse every edited JSON file.
2. Check for duplicate widget names.
3. Compare against the original export and confirm all original variable widgets still exist with the same class.
4. Compare variable flags against the source export: source-variable widgets must remain `isVariable=true` and `bIsVariable=True` unless intentionally removed.
5. Check that required compatibility variables exist, including known typos.
6. Check for bad font references: `DroidSansFallback`, `FontObject=None`.
7. Check that dynamic/effect widgets are limited to the intended region.
8. For parent/child UI work, confirm each referenced child UI exists or has an exported JSON to import first.
9. Run the UIJsonBridge importer automation test when available.

Typical UE command:

```powershell
& "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\project\UETTsProject\UE56_TTSHost\UE56_TTSHost.uproject" -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation RunTests UIJsonBridge.Importer.ImportsLocalTts; Quit" -TestExit="Automation Test Queue Empty" -log
```

Adjust paths for the current project.

## Editing Notes

- Use structured JSON parsing/rewrite for bulk JSON changes.
- Do not use PowerShell string literals for large Chinese JSON rewrites when Node or another UTF-8-safe parser is available.
- Preserve file encoding style.
- Keep generated frame names deterministic, such as `<OriginalName>_TextFrame`.
- If a wrapper is added, verify the original named widget remains findable in the tree.
