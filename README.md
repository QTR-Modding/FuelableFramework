# Fuelable Framework

## Mod author guide

See the [Fuelable Framework wiki](https://github.com/QTR-Modding/FuelableFramework/wiki) for configuration examples, recipe fuels, form groups, and optional LoreBox and SkyPrompt support.

The wiki source is [docs/wiki/Home.md](docs/wiki/Home.md). Documentation changes are reviewed in pull requests and published to the wiki after merging to `main`.

#### Build on Windows

1. Initialize the pinned CommonLibVR-MIT dependency with `git submodule update --init --recursive`.
2. Set **`VCPKG_ROOT`** to your [vcpkg](https://github.com/microsoft/vcpkg) folder.
3. In a Visual Studio x64 developer shell, run `cmake --preset relwithdebuginfo`, then `cmake --build build/relwithdebuginfo --target FuelableFramework`.

Optional deployment: set **`SKYRIM_FOLDER`** to a Skyrim installation or **`SKYRIM_MODS_FOLDER`** to an MO2 mods folder before configuring. Builds then copy their output there.
