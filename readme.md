### Readme

This repository is to updating the VZ300 emulator 'vzem', originally authored by Guy Thomason, 
to the most recent versions of Visual Studio, the Platform Toolset, and the Windows SDK. 
Additionally, it aims to rectify any compiler warnings.

Please refer to the readme.txt file for detailed information on the vzem emulator.  
Please check migration notes.md for migration history and details.  

### Features
Added x64 build targets  
ISO C++ 17 Standard  
Removed DirectInput dependency.  
Added US Standard keyboard key mapper  
Added //TODO tags  
Added [[maybe_unused]] (since C++17) for unused variables  

### vzem version
VZEM_20230225_distro  

### Development Evnironment
Windows 11 23H2  
VS 2022  
Platform toolset v143  
Windows SDK 10.0  

### Build targets
x64|Win32, Debug and Release  

### TODOs
Consider refactoring DirectDraw to Direct2D, as it is no longer recommended  
Create github releases  


