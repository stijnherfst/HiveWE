# HiveWE
A Warcraft III world editor. [Thread on Hive](https://www.hiveworkshop.com/threads/introducing-hivewe.303183/)

![HiveWE Screenshot](http://g2f.nl/0qx1hh2)



## Features

- Directly edit the pathing map  
![Edit the Pathing Map](http://g2f.nl/0bgv29i)
- Edit the terrain
![Edit the Terrain](http://g2f.nl/0nfvw4c)
- Edit JASS with syntax hightlighting, tabs, code completion and more.
![Jass Editor](https://i.imgur.com/CFA5DQU.png)
- Manage your imports with folders!
![Manage imports](http://g2f.nl/0j59f6v)
- Edit global tile pathing  
![Edit global tile pathing](http://g2f.nl/0ihyqgo)

## Download

See the [releases page](https://github.com/stijnherfst/HiveWE/releases) for binary downloads.

## Build Instructions

For if you want to build HiveWE yourself from source. If you run into any problems then don't be afraid to contact me.

1. Download [vcpkg](https://github.com/microsoft/vcpkg). Easiest if you place it in the root of the C drive: "C:/vcpkg"

2. Run bootstrap-vcpkg.bat (or bootstrap-vcpkg.sh for Linux)
3.  
	3.1 Add vcpkg.exe to the Windows environment variable (PATH)  
Or  
	3.2 Open CMD and navigate to the folder containing vcpkg.exe  
4. Run the following commands to install dependencies 

[//]: # (Hello)

	vcpkg install glm:x64-windows  
	vcpkg install soil2:x64-windows
	vcpkg install stormlib:x64-windows
	vcpkg install casclib:x64-windows
	vcpkg install libjpeg-turbo:x64-windows
	vcpkg install qt-advanced-docking-system:x64-windows
	vcpkg install qscintilla:x64-windows  




This might take a while depending on your computer (\~20 minutes)  

Now if you placed vcpkg in the root then you are ready to compile HiveWE.  
You can either:  
- Generate the project files for your IDE using CMake  
or  
- Open Visual Studio as an Administrator and using the open folder button to open the HiveWE folder. (Administrator needed for creating a symbolic link on Windows)  

[//]: # (Hello)

**Done**

If you didn't place vcpkg in C:/ then you will need to open CMakeLists.txt and CMakeSettings.json and change the paths containing C:/vcpkg to point to where your vcpkg is located.  
**Done**

If you run into any issues then feel free to contact me.

## Possible Contributions

Want to help with the development of HiveWE? Below is a list of features that you could implement. You can try one of these or just add something else you feel like HiveWE should have. Any contributions are welcome!

- Being able to change forces/teams
- Changing map sizes/camera bound
- Ramp editing with the terrain palette
- Or any other functionality you think would be cool

If you have any questions then don't be afraid to message me here, at HiveWorkshop (eejin) or on Discord eejin#4240
