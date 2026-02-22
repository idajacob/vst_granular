# Building GranularSynth on Windows

## Requirements

1. **Visual Studio 2022** (Community edition is free)
   - Download: https://visualstudio.microsoft.com/
   - During install, select: **Desktop development with C++**

2. **CMake 3.22+**
   - Download: https://cmake.org/download/
   - Choose the Windows installer (.msi) and check "Add CMake to PATH"

3. **Git**
   - Download: https://git-scm.com/download/win

---

## Steps

### 1. Clone the repository
Open a terminal (CMD, PowerShell or Git Bash):
```bat
git clone <this-repo-url>
cd vst_granular
```

### 2. Configure (downloads JUCE automatically)
```bat
cmake -B build -DCMAKE_BUILD_TYPE=Release
```
> This will download JUCE (~300 MB) the first time. Grab a coffee.

### 3. Build
```bat
cmake --build build --config Release
```

### 4. Find your plugin
```
build\GranularSynth_artefacts\Release\VST3\GranularSynth.vst3
```

### 5. Install
Copy the `GranularSynth.vst3` **folder** to:
```
C:\Program Files\Common Files\VST3\
```
Then rescan plugins in your DAW (Ableton, FL Studio, Reaper, etc.).

---

## Troubleshooting

**"cmake is not recognized"** — restart terminal after installing CMake, or add it to PATH manually.

**"LINK : fatal error"** — make sure Visual Studio C++ tools are installed (not just the IDE).

**DAW doesn't find the plugin** — some DAWs need a manual rescan: look for "Rescan VST plugins" in preferences.
