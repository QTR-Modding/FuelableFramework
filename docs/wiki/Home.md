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
```

`AutoRefuel` defaults to true. With false, an exhausted item is unequipped until the player refills it. With SkyPrompt loaded, highlight the item in the player's inventory and hold its action key. Each configured fuel offers an action; completing the hold consumes one complete cost and adds the configured duration without discarding stored time. Release the key before holding again for another refill. Added time survives saving and loading.

LoreBox descriptions require both LoreBox and Dynamic Translation Framework to be loaded. SkyPrompt actions require SkyPrompt to be loaded. Both `Enabled` options default to true independently for each INI; absent integrations do not prevent normal automatic operation. Manual mode requires SkyPrompt. There is no separate Dynamic Translation Framework switch.

LoreBox text is available for keyword-capable items, including wearable armor and jewelry, in the player's inventory. Vanilla torches are `TESObjectLIGH` records without a keyword component, so they support fuel costs and SkyPrompt actions but cannot use this LoreBox integration.

LoreBox groups each INI's refill options under one title and summary. The example displays a purple "Recharge" heading, the total stored charge once, and a gold row for each alternative cost and the time it adds. Rows with SkyPrompt actions are numbered in the same order as its stack indicator, so `1.` matches `1/2` and `2.` matches `2/2`. `Title`, `Summary`, and `Text` are customizable; an empty value hides that part. The default title is `Fuelable Framework`, the summary is `Remaining: {total_remaining}`, and each option uses `{requirements} (+{duration})`. Colors are six-digit RGB hexadecimal values. The displayed charge refreshes after a manual refill.

LoreBox strings and SkyPrompt `Text` accept `{item}`, `{requirements}`, `{remaining}`, and `{duration}`. Requirements include ingredient names and quantities. Times use compact in-game days, hours, and minutes, such as `3h 20m`. `{remaining}` is the stored time for that fuel option; LoreBox also accepts `{total_remaining}` for the sum of this INI's options for the selected item. In the shared title and summary, option-specific tokens use the first fuel option. SkyPrompt defaults to `Refuel`; an empty `Text` hides the action.

Fuelable Framework uses its own SkyPrompt theme to position its inventory actions. Edit it under SkyPrompt's Theme settings and use Save Theme to persist changes. Keybinds remain shared with SkyPrompt.

## SkyPrompt compatibility

SkyPrompt handshakes allow participating mods' prompts to appear together. Fuelable Framework's handshake key is `0x4655454C41424C45` (ASCII `FUELABLE`). A cooperating mod must request a reciprocal handshake with that key.

Set `CompatibleKeys` in `[SkyPrompt]` to a comma-separated list of the cooperating mods' hexadecimal handshake keys. It defaults to empty. The requests apply to the framework's SkyPrompt client; SkyPrompt handshakes are pairwise, not limited to one INI's actions.
