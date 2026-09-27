# Hold Overlay for Geode — source project

**Status: source implementation, not a compiled or game-tested mod.**
Target: Windows Geometry Dash 2.2081 and Geode SDK 5.0.0.
Do not put this source ZIP in your mods folder or rename it to .geode.

## What it does
White bars rise from a small gold-outlined key box. Holding jump makes a taller
bar; releasing leaves a gap. It includes fading, an optional second player lane,
a click count or rolling one-second CPS, and Geode settings for position, size,
scroll speed, opacity and scale. Defaults resemble the supplied screenshot.

This is a fresh C++ implementation of the visual idea, informed by the supplied
KeyOverlay config and screenshot. The uploaded ZIP contains a compiled C# / SFML
Windows application, not Geode source. None of its binaries or fonts are bundled.
Original inspiration: https://github.com/Blondazz/KeyOverlay

## Get the installable mod with GitHub Actions
1. Create a GitHub repository and add the CONTENTS of this project at its root.
   Include .github/workflows/build.yml (a dot-folder sometimes hidden by file explorers).
   mod.json and CMakeLists.txt must be at the repository root.
2. Open Actions, choose "Build Windows mod", then "Run workflow".
3. After a successful run, download the HoldOverlay-Windows artifact.
4. Extract the .geode file and copy it into Geometry Dash/geode/mods.
5. Restart GD. Open Geode > Hold Overlay > settings to adjust the overlay.

The workflow uses the official geode-sdk/build-geode-mod action with SDK v5.0.0.
https://github.com/geode-sdk/build-geode-mod
A successful build still needs the in-game checks below. If it fails, keep the
first compiler error from the Actions log for diagnosis. This workflow has not
been run from this environment.

## Local Windows build
Install the Geode SDK/CLI and its required Visual Studio C++ build tools, then
set GEODE_SDK to the SDK directory. From a matching Windows developer terminal:

    cmake -S . -B build -A x64
    cmake --build build --config Release

The SDK packages the compiled output as a .geode file.

## Input behavior and limits
- Tracks jump events received by GD, not arbitrary global keyboard keys.
- Mouse, Space, Up and other bindings appear when GD routes them to jump.
- Inputs are grouped by player. This is not separate mouse/Space/Up lanes.
- Only jump is shown; platformer left/right are not displayed.
- Inputs generated or modified by other mods can appear; bot/CBF compatibility
  is unverified. This is not a hardware timing or input-latency measurement tool.
- History uses the overlay scheduler clock; pausing can freeze its animation.
- Pausing closes active bars; attempt resets clear both counters and history.
- Overlay is limited to PlayLayer, not editor playtesting or menus.
- Version metadata is a build target, not a claim of compatibility with later GD.

## Validation done here
Compiled and ran tests/timeline.cpp with GCC C++20. Tested repeat suppression,
held duration, CPS expiration, history expiration, long holds, release on pause,
independent lanes, reset and bounded event storage.
The Geode-facing code was not compiled: no SDK, CMake or Windows compiler is
installed here, and GitHub source downloads were unavailable.

## In-game checks still needed
- Tap and hold jump; verify bar height grows with the hold duration.
- Try mouse, Space and Up, including overlapping inputs.
- Reset, die, pause while holding, resume, complete and exit a level.
- Enable P2 and verify two-player/dual input mapping.
- Change settings; check scaling and positions at your screen resolution.
- Check conflicts with installed input, replay, pause and HUD mods.

## Core tests
    g++ -std=c++20 -Wall -Wextra -pedantic tests/timeline.cpp -o timeline-test
    ./timeline-test
