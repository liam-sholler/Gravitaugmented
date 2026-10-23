# Roboaugmented

A small class project + SWE experiment targetting Robomo: a GMTK Game Jam game made in 4 days, using a custom C++ game engine.

## The Scope

There are a few concrete goals of this project:
1. Create cheats to trivialize / break the game
2. Enable the game's original ImGui dev overlay, and add a custom menu
3. Don't use the original source code or watch devlogs, as to not see any of the original code

## Overview

This project uses DLL injection via. <windows.h> aswell as Augment's framework to hook the raw addresses derived from Ghidra and mutate game state.
