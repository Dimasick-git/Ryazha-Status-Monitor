🔧 **Этот проект реализован специально для кастомной прошивки [Ryazhenka](https://github.com/Dimasick-git/Ryzhenka)** — полная интеграция и совместимость обеспечены!

# Ryazha-Status Monitor Overlay
Monitor Your hardware in real time!
This is an overlay homebrew dedicated to Nintendo Switch.
You need to have installed Tesla environment to use it.
Tool contains five menus to choose, each one is explained [here](/docs/modes.md).

> Overlay supports customizations through config file, more [here](/docs/config.md).

If it's not working in dock, you need to first start Status Monitor, then put Nintendo Switch to dock.

# What is currently supported:
- CPU Usage for each core (Cores `#0`-`#2` are used by apps/games, Core `#3` is used by OS, background processes and also Tesla overlays)
- GPU Load
- CPU, GPU & RAM target frequencies (also real frequencies + RAM Load if [sys-clk 2.0.0_rc4+](https://github.com/retronx-team/sys-clk/releases) is installed, use only official RetroNX release for reliable results)
- Used RAM categorized to: (not supported by FWs <5.0.0)
  - Total
  - Application
  - Applet
  - System
  - System Unsafe
- SoC, PCB & Skin temperatures (Skin temperature not supported by FWs <5.0.0)
- Fan Rotation Level
- PFPS, FPS, resolutions, game read speed (shows only if [my fork of SaltyNX](https://github.com/masagrator/SaltyNX/releases) is installed)
- Battery temperature, raw charge, age, average voltage, average current flow and average power flow
- Charger type, max voltage, and max current
- DSP usage (only for FW older than 17.0.0)
- NVDEC, NVENC and NVJPG clock rates
- Network type + Wi-fi password

# Requirements:
- RyazhaHand Overlay or Tesla Menu (version >=1.2.3)
How to setup everything: [HERE](https://gist.github.com/masagrator/65fcbd5ad09243399268d145aaab899b)

---

# Thanks to:
- RetroNX channel for helping with coding stuff
- SunTheCourier for sys-clk-Overlay from which I learned how to make my own Tesla homebrew
- Herbaciarz for providing screenshots from HDMI Grabber
- KazushiMe for writing code to read registers from max17050 chip
- CTCaer for Hekate from which I took max17050.h and calculation formulas for reading battery stats from max17050 chip
- ChanseyIsTheBest for testing Game Resolutions menu

# FAQ:
Q: This homebrew has any impact on games?
A: Negligible, you won't see any difference. Almost everything is done on Core `#3`, other cores usage is below 0.001%.

# Troubleshooting:
Q: When opening Full or Mini mode, overlay is showing that Core #3 usage is at 100% while everything else is showing 0, eventually leading to crash. Why this happens?
A: There are few possible explanations: 
1. You're using nifm services connection test patches (in short `nifm ctest patches`) that are included in various packs. Those patches allow to connect to network that has no internet connection. But they cause nifm to randomly rampage when connected to network. Find any folder in `atmosphere/exefs_patches` that has in folder name `nifm`, `nfim` and/or `ctest`, delete this folder and restart Switch (if you are using `sys-patch`, turn off `nifm` patching). If you must use it, only solution is to use this overlay only in airplane mode.
