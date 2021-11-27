# HiveWE
HiveWE is a Warcraft III World Editor (WE) that focusses on speed and ease of use. It improves massively on the vanilla WE, especially for large maps where the regular World Editor is often too slow and clunky.  [Thread on Hive](https://www.hiveworkshop.com/threads/introducing-hivewe.303183/)

Some of the benefits over the vanilla WE:
- Faster loading
- Faster rendering
- Faster editing
- Modern UI/UX
- Edit the pathing map directly
- Edit global tile pathing
- Import heightmaps
- Improved editing palettes

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

## Build Instructions

1. Clone HiveWE somewhere 
`git clone https://github.com/stijnherfst/HiveWE.git`
2. Clone [vcpkg](https://github.com/microsoft/vcpkg) somewhere central (eg. "C:/")
`git clone https://github.com/Microsoft/vcpkg.git`
3. Run vcpkg/bootstrap-vcpkg.bat
4. Add 2 environment variables to your system:
- `VCPKG_ROOT`: the location where vcpkg is installed (e.g. "C:\vcpkg")
- `VCPKG_DEFAULT_TRIPLET`: depending on your operating system (`x64-windows`, `x64-linux`, `x64-osx`)
5. Add the vcpkg location to your System Path variable (eg. "C:\vcpkg")
6. Close and reopen your CMD (so it reloads environment variables)
7. Install dependencies  
`vcpkg install qtbase[default-features] glm soil2 stormlib casclib libjpeg-turbo bullet3 qt-advanced-docking-system abseil[cxx17] fmt`  
Estimated to take about 30 minutes
8. Open Visual Studio as an Administrator and using the open folder button to open the HiveWE folder. (Administrator required for creating a symbolic link on Windows)  
**Done**

If you run into any issues then feel free to contact me.

## Possible Contributions

Want to help with the development of HiveWE? Below is a list of features that you could implement. You can try one of these or just add something else you feel like HiveWE should have. Any contributions are welcome!

- Being able to change forces/teams
- Changing map sizes/camera bound
- Ramp editing with the terrain palette
- Making HiveWE run faster
- An FDF frame editor
- Text colorizer
- Advanced terrain editing tools (e.g. flood fill, magic wand selection)
- Or any other functionality you think would be cool

If you have any questions then don't be afraid to message me here, at HiveWorkshop (eejin) or on Discord eejin#4240