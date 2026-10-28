redhat 
======

Metaserver for Rage of Mages 2 - https://rom2.ru

## How to build in VS Code (Windows, 32-bit)

1. Install VS Code
2. Install Microsoft Visual Studio Build Tools 2022 https://aka.ms/vs/17/release/vs_BuildTools.exe
3. During installation, select:
 - MSVC v143 - VS 2022 C++ x64/x86 build tools
 - Windows 11 SDK (10.0.22621.0)
 - C++ CMake tools for Windows
4. In VS Code, install extensions: "C/C++" and "CMake Tools"
5. Open project folder in VS Code
6. Go to CMake menu on the left -> Project Status -> Configure:
 - 1st row: select "Visual Studio Build Tools 2022 Release - x86"
 - 2nd row: select "Release" instead of "Debug"
7. Ctrl+Shift+P -> Tasks: Run Build Task
8. Binary will appear at: build/Release/redhat.exe
