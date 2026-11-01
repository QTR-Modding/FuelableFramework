# Fuelable Framework

Create `Data/SKSE/Plugins/FuelableFramework/MyMod.ini` with:

```ini
[Light Sources]
src1 = 0x1D4EC~Skyrim.esm
[Fuel Sources]
src1 = 0xF11C0~Skyrim.esm
[Durations]
src1 = 8
```

This makes the vanilla torch consume one Dwarven Oil for eight in-game hours of use. The matching `src1` keys connect the item, its fuel, and its duration. Add another matching key to configure another item or another fuel for the same item. The framework also supports equipped items such as wearable lanterns and jewelry.

Values can use `0xLocalFormID~PluginName.esp`, a full runtime FormID, or an EditorID. EditorIDs for inventory items require powerofthree's Tweaks. Plugin-local IDs remain usable when the load order changes.

## Configuration files

The original `Data/SKSE/Plugins/FuelableFramework.ini` continues to work. Additional INIs are loaded from the `FuelableFramework` folder and its subfolders, in sorted path order after the original file. Each file's keys and options apply to its own entries. A later definition of the same item/fuel pair replaces the earlier definition; different fuels remain alternatives for that item.

The legacy `[Other Stuff]` notification settings and `[QuickLight Support]` settings remain in the original INI.

## Form groups

A form group is a named list of items or recipes. Create `Data/SKSE/Plugins/FuelableFramework/formGroups/MyWearables.txt` with one form identifier per line:

```text
0x12E49~Skyrim.esm
0x12E4D~Skyrim.esm
```

These are the iron cuirass and iron helmet. Set `src1 = MyWearables` in `[Light Sources]` to apply that entry to both items. The filename without `.txt` is the group name, using QTR Utils' standard loader. Groups contain identifiers, not other group names; duplicate forms collapse and empty groups produce no entries.

Groups also work in `[Fuel Sources]`. Each member is an alternative fuel or recipe, so a source group and fuel group produce every item/fuel combination. Group members are considered in FormID order. A COBJ defines ingredients consumed together; a fuel group defines alternatives. Group names take precedence over individual identifiers, so choose distinct names.

## Recipe costs

A COBJ is a crafting-recipe record. Put its FormID or EditorID in `[Fuel Sources]` to use its ingredients and quantities as the resource cost. For example, a recipe requiring two ingots and one leather strip consumes that complete set and grants the configured duration. Missing ingredients consume nothing. The recipe's conditions, workbench, output item, and output quantity are ignored.

## Manual actions and descriptions

Add these sections to the INI that defines the items:

```ini
[Refueling]
AutoRefuel = false

[LoreBox]
Enabled = true
Title = Recharge
Summary = Stored charge: {total_remaining}
Text = {requirements} (+{duration})
TitleColor = A86CCD
TextColor = E8EAED
OptionColor = FFCC80

[SkyPrompt]
Enabled = true
Text = Recharge
; Leave empty unless another mod requests a reciprocal SkyPrompt handshake.
CompatibleKeys =
```

`AutoRefuel` defaults to true. With false, an exhausted item is unequipped until the player refills it. With SkyPrompt loaded, highlight the item in the player's inventory and hold its action key. Each configured fuel offers an action; completing the hold consumes one complete cost and adds the configured duration without discarding stored time. Release the key before holding again for another refill. Added time survives saving and loading.

LoreBox descriptions require both LoreBox and Dynamic Translation Framework to be loaded. SkyPrompt actions require SkyPrompt to be loaded. Both `Enabled` options default to true independently for each INI; absent integrations do not prevent normal automatic operation. Manual mode requires SkyPrompt. There is no separate Dynamic Translation Framework switch.

LoreBox text is available for keyword-capable items, including wearable armor and jewelry, in the player's inventory. Vanilla torches are `TESObjectLIGH` records without a keyword component, so they support fuel costs and SkyPrompt actions but cannot use this LoreBox integration.

### LoreBox text

For a wearable lantern configured to consume one Dwarven Oil for eight in-game hours, these lines set its description:

```ini
[LoreBox]
Title = Recharge
Summary = Stored charge: {total_remaining}
Text = {requirements} (+{duration})
```

With 2 hours and 30 minutes stored and its SkyPrompt action enabled, the description reads:

```text
Recharge
Stored charge: 2h 30m
1. Dwarven Oil (1) (+8h)
```

`Title` is the heading. `Summary` appears once below it and describes the stored time. `Text` creates a row for each alternative fuel configured by this INI. Here, the row says that one Dwarven Oil adds eight hours; it does not say that eight hours remain. The displayed charge refreshes after a manual refill.

The words in braces are **placeholders**: Fuelable Framework replaces them with values for the selected item. The surrounding words, spaces, plus sign, and parentheses appear as written. For example, changing the summary to `Summary = Fuel left: {total_remaining}` produces `Fuel left: 2h 30m` in the same situation. Change the title to `Lantern Fuel` or `Recharge` to suit your mod. An empty `Title`, `Summary`, or `Text` hides that part.

The text comes from your INI and the item's current charge, so no separate file under `Interface/Translations` is needed. Keep the supplied `SKSE/Plugins/DynamicTranslationFramework/FuelableFramework.json`: it registers the description with DTF, which requests the current text from Fuelable Framework's DLL for LoreBox.

Rows are numbered in the same order as SkyPrompt's stack indicator, so `1.` matches `1/2` and `2.` matches `2/2`. Disabling SkyPrompt in the INI or leaving its action text empty makes that INI's rows unnumbered. The earlier color settings give the title a purple color and the fuel rows a gold color; colors are six-digit RGB hexadecimal values.

When omitted, the title defaults to `Fuelable Framework`, the summary to `Remaining: {total_remaining}`, and the fuel rows to `{requirements} (+{duration})`.

### Text placeholders

Use these placeholders in LoreBox's `Title`, `Summary`, and `Text`, or in SkyPrompt's `Text`, except where noted:

| Placeholder | Replaced with | Example |
| --- | --- | --- |
| `{item}` | The selected item's name. | `Wearable Lantern` |
| `{requirements}` | The items and quantities consumed by one refill. A recipe lists its ingredients with commas. | `Dwarven Oil (1)` or `Leather Strips (2), Iron Ingot (3)` |
| `{duration}` | The in-game time added by one refill with this fuel. | `8h` |
| `{remaining}` | The time currently stored for this individual fuel option. | `2h 30m` |
| `{total_remaining}` | The sum of the selected item's stored time across this INI's fuel options. LoreBox only. | `2h 30m` |

With one fuel option, `{remaining}` and `{total_remaining}` are equal. With two options storing one hour each, `{remaining}` is `1h` for each row and `{total_remaining}` is `2h`. In the shared LoreBox title and summary, option-specific placeholders use the first fuel option; use `{total_remaining}` for the combined stored time.

Times use compact in-game days, hours, and minutes, such as `1d 3h 20m`. SkyPrompt's `Text = Recharge` displays the literal label `Recharge`; `Text = Refuel {item}` includes the selected item's name. SkyPrompt defaults to `Refuel`, and an empty `Text` hides its action.

Fuelable Framework uses its own SkyPrompt theme to position its inventory actions. Edit it under SkyPrompt's Theme settings and use Save Theme to persist changes. Keybinds remain shared with SkyPrompt.

## SkyPrompt compatibility

SkyPrompt handshakes allow participating mods' prompts to appear together. Fuelable Framework's handshake key is `0x4655454C41424C45` (ASCII `FUELABLE`). A cooperating mod must request a reciprocal handshake with that key.

Leave `CompatibleKeys` empty for normal use. To show Fuelable Framework's prompts alongside another mod's prompts, enter that mod's hexadecimal handshake key here. Use the key supplied by its author; separate multiple keys with commas. The other mod must also request compatibility with Fuelable Framework's key above. Adding a key on only one side does not establish compatibility.

The requests apply to the framework's SkyPrompt client; SkyPrompt handshakes are pairwise, not limited to one INI's actions.
