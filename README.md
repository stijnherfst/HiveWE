# HiveWE
HiveWE is a Warcraft III World Editor (WE) that focuses on speed and ease of use.
It improves massively on the vanilla WE, especially for large maps where the regular World Editor is often too slow and clunky.
The aim is to be a lightweight tool that only does what it needs to and relegates other tasks to high-quality apps.

[Thread on the Hiveworkshop](https://www.hiveworkshop.com/threads/introducing-hivewe.303183/)

Some of the benefits over the vanilla WE:
- Way faster loading times (32s -> 4s on a sample map)
- Renders your whole map at 120 fps
- Modern UI/UX
- Edit the pathing map directly
- Edit global tile pathing
- Import heightmaps
- Improved editing palettes
  - Directly edit water height
  - Brushes of size 1000+
  - Doodad variation control
- Unified global search (double shift)

## Features

- Edit the terrain
![HiveWE Screenshot](/Screenshots/HiveWE.png)
- Advanced Object Editor
![HiveWE Screenshot](/Screenshots/ObjectEditor.png)
- Directly edit the pathing map
![Edit the Pathing Map](/Screenshots/PathingEditing.png)
- Edit global tile pathing
![Edit global tile pathing](/Screenshots/GlobalPathingEditing.png)

## Download

See the [releases page](https://github.com/stijnherfst/HiveWE/releases) for binary downloads.

## Other Community Tools

Trigger editing: [WC3 Typescript](https://cipherxof.github.io/w3ts/)
Model editing: [3DS Max Plugin](https://github.com/TaylorMouse/warcraft_III_reforged_tools)
or [Retera Model Studio](https://github.com/Retera/ReterasModelStudio)

## Build Instructions

HiveWE uses CMake and [vcpkg](https://github.com/microsoft/vcpkg) for dependency management.

The `VCPKG_ROOT` environment variable must point to your vcpkg installation.

### Windows

0. Requires Visual Studio 17.14 or higher (C++20 modules)
1. Clone HiveWE somewhere:
   ```shell
   git clone https://github.com/stijnherfst/HiveWE.git
   ```
2. Clone [vcpkg](https://github.com/microsoft/vcpkg) somewhere central (e.g. `C:\vcpkg`):
   ```shell
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   ```
3. Bootstrap vcpkg:
   ```shell
   C:\vcpkg\bootstrap-vcpkg.bat
   ```
4. Add a user environment variable:
   ```text
   VCPKG_ROOT=C:\vcpkg
   ```
5. Build.

#### Visual Studio or CLion

1. Open Visual Studio as an **Administrator** and use the Open Folder option to open the HiveWE folder. Administrator privileges are required for creating the symbolic link on Windows.
2. Dependencies will be compiled automatically. The initial build may take 15-20 minutes, mostly due to Qt.

#### CMake

```shell
cmake --preset Release
cmake --build --preset Release
ctest --preset Release
```

### Linux

HiveWE can be built natively on Linux using Clang, CMake, Ninja and vcpkg.

Clang 19 is the currently tested compiler.

#### Debian / Ubuntu dependencies

```shell
sudo apt update
sudo apt install -y \
    build-essential \
    clang-19 \
    cmake \
    ninja-build \
    pkg-config \
    git \
    curl \
    zip \
    unzip \
    tar \
    libxcb1-dev \
    libxcb-cursor-dev \
    libxcb-dri3-dev \
    libxcb-glx0-dev \
    libxcb-icccm4-dev \
    libxcb-image0-dev \
    libxcb-keysyms1-dev \
    libxcb-present-dev \
    libxcb-randr0-dev \
    libxcb-render0-dev \
    libxcb-render-util0-dev \
    libxcb-shape0-dev \
    libxcb-shm0-dev \
    libxcb-sync-dev \
    libxcb-util-dev \
    libxcb-xfixes0-dev \
    libxcb-xinerama0-dev \
    libxcb-xinput-dev \
    libxcb-xkb-dev \
    libx11-xcb-dev \
    libglu1-mesa-dev \
    libxrender-dev \
    libxi-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libegl1-mesa-dev \
    libwayland-dev \
    wayland-protocols \
    libfontconfig1-dev \
    libfreetype-dev \
    libharfbuzz-dev
```

#### vcpkg

Clone and bootstrap vcpkg:

```shell
git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
export VCPKG_ROOT="$HOME/vcpkg"
```

#### Configure and build

```shell
git clone https://github.com/stijnherfst/HiveWE.git
cd HiveWE
cmake --preset linux-release -DCMAKE_CXX_COMPILER=clang++-19
cmake --build --preset linux-release --parallel
```

Run HiveWE:

```shell
./build/linux-release/HiveWE
```

The Linux build supports XCB/XWayland and native Wayland.

The Linux Qt overlay uses the system Fontconfig, FreeType and HarfBuzz libraries so Qt can use the host font configuration.

The `linux-release` preset currently builds HiveWE with tests disabled.

#### Warcraft III and JassHelper

Wine is not required to build or run HiveWE itself.

Wine is required for functionality that still depends on Windows executables:

- JASS compilation through JassHelper
- Launching Warcraft III through Play Test

The `wine` and `winepath` commands must be available in `PATH` for these features to work.

If you run into any issues, then feel free to contact me at HiveWorkshop (eejin) or on Discord (eejin)

## Potential Contributions

Want to help with the development of HiveWE? Below is a list of features that you could implement. You can try one of these or just add something else you feel like HiveWE should have. Any contributions are welcome!

- Being able to change forces/teams
- Making HiveWE run faster
- An FDF frame editor
- Text colorizer
- Advanced terrain editing tools (e.g. flood fill, magic wand selection)
- Or any other functionality you think would be cool

If you have any questions, then don't be afraid to message me here, on the HiveWorkshop (eejin) or on Discord (eejin)
