---
title: "How to install and use GCC on Windows, and configure VS Code to use it"
source: "https://gist.github.com/Ronatos/db5e62c2b085797b2449a79ff6535298"
author:
  - "[[Ronatos]]"
published:
created: 2026-01-04
description: "How to install and use GCC on Windows, and configure VS Code to use it - gcc-launch.bat"
tags:
  - "clippings"
---

```bash
gcc hello_world.c -o hello.exe
hello.exe
```

# How to install and use GCC on Windows, and configure VSCode to use it

## How to install GCC

1. Get the MinGW installer from https://sourceforge.net/projects/mingw/files/?source=navbar, following the installation instructions.
    * I know it looks sketchy, but it's just the big green button.
    * Your installation path should be the default, ```C:\MinGW```.
    * Installation for only your user is recommended.
    * When prompted to select installation packages, only opt for ```mingw32-base``` and ```mingw32-binutils```.
1. After installation is complete, add ```C:\MinGW\bin``` to your windows environment variable PATH.
    1. Open the Control Panel and select ```System and Security```.
    1. Open the ```System``` menu.
    1. On the left, open ```Advanced system settings```.
    1. Open ```Environment Variables```.
    1. Under your user variables, **not** your system variables, click the ```Path``` variable, and select ```Edit```.
    1. Click ```New```, and type ```C:\MinGW\bin```. Then click ```Ok``` on every menu you've opened thus far with that option.
1. Open a fresh command line and type ```mingw-get``` to verify the installation was successful.
    * It should open the MinGW installer menu from before, but we're not using this.
1. In your command line, type ```mingw-get install gcc```. The GCC compiler and all required packages will be installed.

## How to use GCC

1. In the command line, navigate to the directory containing your source code, ```C:\myDirectoryPath\example.c```.
1. To compile a source code file, type ```gcc example.c -o example_program.exe```.
    * This will tell gcc to compile example.c and output the results to example_program.exe.
    * **Be careful** what you input to gcc. Incorrect gcc parameters can and will lead to frustration and code being overwritten.
    * The documentation for the gcc command can be found at https://gcc.gnu.org/onlinedocs/.
1. To run your compiled file, simply type ```example.exe```.

## How to configure VSCode to compile and run your programs

1. Go to the VSCode extensions tab and install the C/C++ extension by Microsoft, then click ```Reload```.

### The tasks.json

1. Press ```Ctrl + Shift + p``` and in the menu type "task". Configure a new task, and don't worry about which type of task (we're going to paste something else in there anyways). You will replace the code in this file with the following:
```
{
    // See https://go.microsoft.com/fwlink/?LinkId=733558
    // for the documentation about the tasks.json format
    "version": "2.0.0",
    "tasks": [
        {
            "label": "gcc launch",
            "type": "shell",
            "command": "${workspaceRoot}\\gcc-launch.bat",
            "problemMatcher": []
        }
    ]
}
```
1. Now move the ```gcc-launch.bat``` file from this gist to your ```${workspaceRoot}``` so the task can see it. Feel free to move this file as you see fit, but keep in mind that the task will look for this batch file at the path you tell it.
    * **This file contains a shell script that will try to compile and run a hello_world.c program in the ```${workspaceRoot}``` directory. Modify this script with the names of your files and directories before running your task.**
1. In order to run your task, all you need to do is press ```Ctrl + Shift + p``` and type "task" to run your task named "compile with gcc". Your program will run in the integrated terminal below.

### The c_cpp_properties.json

To make full use of the C/C++ extension you downloaded earlier, you will need to specify your includePath: where your C libraries exist. To do so, we'll need to create a file in the .vscode folder called c_cpp_properties.json.

1. Press ```Ctrl + Shift + p``` and type ```c/cpp```, choosing the option to edit configurations. This should create the file you need, filling in a large amount of information automatically.
1. In that file, find the object named "Win32". This is where you belong. The "includePath" should contain the following array:
```
[
    "${workspaceRoot}",
    "C:\\MinGW\\lib\\gcc\\mingw32\\6.3.0\\include",
    "C:\\MinGW\\lib\\gcc\\mingw32\\6.3.0\\include\\ssp",
    "C:\\MinGW\\lib\\gcc\\mingw32\\6.3.0\\include-fixed",
    "C:\\MinGW\\include"
]
```

The "browse" path should contain the following array:
```
[
    "C:\\MinGW\\lib\\gcc\\mingw32\\6.3.0\\include",
    "C:\\MinGW\\lib\\gcc\\mingw32\\6.3.0\\include-fixed",
    "C:\\MinGW\\include\\*"
]
```

You are now completely set up to compile programs using gcc either through the command line or through VSCode. Good luck!
