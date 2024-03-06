### Features
Added x64 build targets  
ISO C++ 17 Standard  
Removed DirectInput dependency.  
Added US Standard keyboard key mapper  
Added //TODO tags  
Added [[maybe_unused]] (since C++17) for unused variables  

### TODOs:
Remove 'register' annotation as it is removed in C++17 standard.  
Refactor exception handling for LoadWAVFile helpers.  

### Update history
2024-03-06  
Cleanup, changed repo to public, sent link to Guy  

2024-02-26  
Replaced dinput with GetKeyboardState for keyboard input. DirectInput is deprecated since Windows 8.  
XInput is for handling input from Xbox-compatible game controllers  

2024-02-25  
Project settings cleanup  
Temporarily disabled DirectInput code  
Added x64 build targets  
All debug and release builds for Win32 and x64 are tested.  

2024-02-24  
Updated project to VS 2022, Platform toolset v143, Windows SDK 10.0  
Fixed Include path Debug|Release  
Fixed wav file path in vz.rc  
Added .gitignore  

2023-02-26  
Created git repo from VZEM_20230225_distro  
[FB Post : VZEM 20230225 release](https://www.facebook.com/groups/4609469943/posts/10151784235744944/)  

### References
[C++ 17 New Features and Trick](https://www.codeproject.com/Articles/5262072/Cplusplus-17-New-Features-and-Trick)  