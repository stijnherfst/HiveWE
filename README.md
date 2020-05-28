# HiveWE
A Warcraft III world editor. HiveWE focusses on speed and ease of use, especially for large maps where the regular World Editor is often too slow and clunky.  [Thread on Hive](https://www.hiveworkshop.com/threads/introducing-hivewe.303183/)

![HiveWE Screenshot](http://g2f.nl/0qx1hh2)



## Features

- Directly edit the pathing map  
![Edit the Pathing Map](http://g2f.nl/0bgv29i)
- Edit the terrain
![Edit the Terrain](http://g2f.nl/0nfvw4c)
- Edit JASS with syntax hightlighting, tabs, code completion and more.
![Jass Editor](http://g2f.nl/0jb8j8t)
- Edit global tile pathing  
![Edit global tile pathing](http://g2f.nl/0202154)

## Download

See the [releases page](https://github.com/stijnherfst/HiveWE/releases) for binary downloads.

## Build Instructions

1. Clone [vcpkg](https://github.com/microsoft/vcpkg) somewhere central (eg. "C:/")
`git clone https://github.com/Microsoft/vcpkg.git`
2. Run bootstrap-vcpkg.bat
3. Add 2 environment variables to your system:
- `VCPKG_ROOT`: the location where vcpkg is installed (e.g. "C:\vcpkg")
- `VCPKG_DEFAULT_TRIPLET`: depending on your operating system (Windows, Linux, MacOS): (`x64-windows`, `x64-linux`, `x64-osx`)
4. You need to close and open the CMD window after step 3!!
5. Add the vcpkg location to your System Path variable (eg. "C:\vcpkg")
6. Install dependencies  
`vcpkg install qt5-base glm soil2 stormlib casclib libjpeg-turbo qscintilla bullet3 qt-advanced-docking-system abseil[cxx17]`  
Estimated to take about 30 minutes
7. Open Visual Studio as an Administrator and using the open folder button to open the HiveWE folder. (Administrator needed for creating a symbolic link on Windows)  
**Done**

If you run into any issues then feel free to contact me.

## Possible Contributions

Want to help with the development of HiveWE? Below is a list of features that you could implement. You can try one of these or just add something else you feel like HiveWE should have. Any contributions are welcome!

- Being able to change forces/teams
- Changing map sizes/camera bound
- Ramp editing with the terrain palette
- Making HiveWE run faster
- A FDF frame editor
- Or any other functionality you think would be cool

If you have any questions then don't be afraid to message me here, at HiveWorkshop (eejin) or on Discord eejin#4240
