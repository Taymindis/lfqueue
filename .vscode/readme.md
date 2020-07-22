# Notes about VS Code C++ setup

AFAIK these are the notes missing from 
[VS Code MSVC setup page](https://code.visualstudio.com/docs/cpp/config-msvc).

- `tasks.json` and `launch.json` are **not** connected
    - when you do "CTRL+SHIFT+B" `tasks.json` is used. It defines how you want you executable to be built.
    - when you start debugger `launch.json` is used
    - this is Code "out of the box" behavior.
- `c_cpp_properties` has to be re-checked on each C++ tools update. Or SDK update.
    - this is especially true if you use Visual Studio on the same machine
    - if you want "pure" VS Code C++ experience use it on machine which has no Visual Studio installed
        - this means "[Build Tools for Visual Studio](https://visualstudio.microsoft.com/downloads/#other)"


**clang on Windows**

I have not had time yet to resolve this one.

## Caveat Emptor

If you use WIN10 as a C/C++ development machine use Visual Studio Community Eddition. 

If you like pain use VS Code.

It is as simple as that.

---
(c) 2020 by dbj@dbj.org CC BY SA 4.0