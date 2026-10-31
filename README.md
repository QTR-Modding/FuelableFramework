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

## Recipe costs

A COBJ is a crafting-recipe record. Put its FormID or EditorID in `[Fuel Sources]` to use its ingredients and quantities as the resource cost. For example, a recipe requiring two ingots and one leather strip consumes that complete set and grants the configured duration. Missing ingredients consume nothing. The recipe's conditions, workbench, output item, and output quantity are ignored.

## Manual actions and descriptions

Add these sections to the INI that defines the items:

```ini
[Refueling]
AutoRefuel = false

[LoreBox]
Enabled = true
Text = Charge remaining: {remaining}. Recharge requires {requirements}.

[SkyPrompt]
Enabled = true
Text = Recharge ({requirements})
```

`AutoRefuel` defaults to true. With false, an exhausted item is unequipped until the player refills it. With SkyPrompt loaded, highlight the item in the player's inventory and accept its action. Each configured fuel offers an action; accepting it consumes one complete cost and adds the configured duration without discarding stored time. Added time survives saving and loading.

LoreBox descriptions require both LoreBox and Dynamic Translation Framework to be loaded. SkyPrompt actions require SkyPrompt to be loaded. Both `Enabled` options default to true independently for each INI; absent integrations do not prevent normal automatic operation. Manual mode requires SkyPrompt. There is no separate Dynamic Translation Framework switch.

LoreBox text is available for keyword-capable items, including wearable armor and jewelry, in the player's inventory. Vanilla torches are `TESObjectLIGH` records without a keyword component, so they support fuel costs and SkyPrompt actions but cannot use this LoreBox integration.

Both strings accept `{item}`, `{requirements}`, `{remaining}`, and `{duration}`. The time values include in-game hours; requirements include ingredient names and quantities. Omitted options use the defaults `Remaining: {remaining}. Requires: {requirements}.` and `Refuel ({requirements})`. An empty string hides that description or action.

## SkyPrompt compatibility

SkyPrompt handshakes allow participating mods' prompts to appear together. Fuelable Framework's handshake key is `0x4655454C41424C45` (ASCII `FUELABLE`). A cooperating mod must request a reciprocal handshake with that key.

Set `CompatibleKeys` in `[SkyPrompt]` to a comma-separated list of the cooperating mods' hexadecimal handshake keys. It defaults to empty. The requests apply to the framework's SkyPrompt client; SkyPrompt handshakes are pairwise, not limited to one INI's actions.

#### Build on Windows

1. Initialize the pinned CommonLibVR-MIT dependency with `git submodule update --init --recursive`.
2. Set **`VCPKG_ROOT`** to your [vcpkg](https://github.com/microsoft/vcpkg) folder.
3. In a Visual Studio x64 developer shell, run `cmake --preset relwithdebuginfo`, then `cmake --build build/relwithdebuginfo --target FuelableFramework`.

Optional deployment: set **`SKYRIM_FOLDER`** to a Skyrim installation or **`SKYRIM_MODS_FOLDER`** to an MO2 mods folder before configuring. Builds then copy their output there.
