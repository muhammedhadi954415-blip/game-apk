# Chrono Protocol: Eclipse — Android C++ game

A mobile-first, stylized 3D sci-fi action game built in C++ with raylib 5.5. It includes 15 story missions across five acts, touchscreen movement/aim/fire, dash, EMP, enemy waves, bosses, upgrade progression, auto-saves, original generated music, sound effects, and procedural 3D scenery.

## Important scope note

This is a playable indie-style game project, not a AAA production. The roughly two-hour campaign is paced through mission survival timers (about 2 hours total if played straight through), with combat and objectives continuing until each mission's requirements are met. Performance and actual playtime vary by device and play style.

## Build an Android APK with GitHub Actions

1. Download and extract the complete project ZIP so the folder structure stays intact.
2. Replace your repository's `main.cpp`, `AndroidManifest.xml`, `.gitignore`, and `.github/workflows/build.yml` with the matching files from this project.
3. Upload the complete `assets/` folder, preserving every file and the `audio/` and `textures/` subfolders. The game needs these assets; replacing only `main.cpp` is not enough.
4. Commit the changes to the `main` branch.
5. Open **Actions → Build Chrono Protocol Android APK** and wait for the green check.
6. Open the successful run, scroll to **Artifacts**, and download `ChronoProtocol-Android-APK`. Extract the ZIP if GitHub downloads it as a ZIP; install `ChronoProtocol.apk`.

The workflow builds an ARM64 APK and includes `libc++_shared.so`, which matters for launching a native C++ app. It creates a debug-signed APK for testing; it is not a Play Store release build. The workflow currently generates a fresh test key each run, so uninstall any earlier test copy before installing a new APK if Android reports a signature conflict.

## Controls

- **Left virtual stick:** move
- **Right virtual stick:** aim and fire (hold / drag)
- **DASH:** quick evade with brief invulnerability
- **EMP:** spend energy for a radial blast
- Desktop test controls: **WASD / arrows**, mouse to aim/fire, **Space / Left Shift** to dash, **E / Q** for EMP.

## Campaign

15 missions in five acts: Cold Boot, Sunken Reactor, Neon Ascent, Neural Archive, and Eclipse Protocol. Story progress and upgrades are stored in the Android app-private data directory via NativeActivity; desktop builds use `chrono_save.txt` in the working directory.

## If the app still fails to launch

First uninstall any earlier test build, then install the newly downloaded APK. Open the failed workflow run and inspect the **build log**; for a built-but-crashing APK, use Android logcat. A green Actions check confirms the APK build/package steps passed, not that the app has been runtime-tested on your exact phone.
