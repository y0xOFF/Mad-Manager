## Mad Manager
Simple, Lightweight and Open Source Mod Manager for Mad Max on PC

## Instructions

* Your mods are stored in **Mods**. This folder is automatically created in the same directory of Mad Mananger after opening it's .exe.

* The structure for a mod must be **Mod_Main_folder/dropzone**. Inside dropzone put the mod file(s) and folder(s). Put the mod main folder inside **Mods**. The final path must be **Mods/Mod_Main_folder/dropzone**.

* To install a mod, use the **Install Mods button** or drag mod's folder or compressed file to Mad Manager window.

* To enable a mod, check it's box. Mad Manager will automatically copy it's dropzone content to the game's dropzone. Disabling a mod adds a .disabled extension to the mod's file(s). To delete a mod, right click it in Mad Manager and Delete.

## Notes
* Not backwards compatible with previous mods installed with the old version. Make sure to disable your mods and delete their manifest files. Also delete the old version of Mad Manager.
* Requires Windows 10+. C++ and Dear ImGui are embedded into Mad Manager. No external dependencies required.

## Changelogs

* Project rebuilt from Python to C++ and [Dear ImGui](https://github.com/ocornut/imgui)
* New config.ini for storing MADMAX, MODS and MADMANAGER directories
* New manifest.ini for storing mods file paths
* Optimized and organized code and better perfomance


Join [Mad Max Speedrunning & Modding Discord Community](https://discord.com/invite/7rhHfycZdK)
