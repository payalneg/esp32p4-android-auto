## Requirements
* **MinGW**

## Usage

### Get the source of simulator

Clone the PC project and the related sub modules:

```
git clone ssh://git@bitbucket.sw.nxp.com/auto_tools/lvgl-simulator.git --recurse-submodules
```

### Install MinGW
You can download MinGW from https://osdn.net/projects/mingw/releases/

### Set environment
1. Add the path of **mingw32-make** to the system's path. By default, it is `C:\MinGW\bin`.
2. Copy `bin\SDL2.dll` to `C:\MinGW\lib`.

### Compile the project(Note: don't use git bash)
1. Enter project direction:
   ```
   cd lvgl-simulator
   ```
2. Build the target:
   ```
   mingw32-make -j8
   ```
### Run simulator
```
mingw32-make run
```

### Add your code for ui's callbacks
1. User can edit callbacks in [event_cb.c](demo/event_cb.c).
   
## Work with Visual Studio Code

1. Download and install latest version of Visual Studio Code
2. Install extensions listed below:
   - C/C++
   - C/C++ Clang Command Adapter
   - Make
3. Build the target
   In Visual Studio Code, you can open a terminal via `Terminal-> New Terminal`. After Openning the terminal, type `make -j8` to build the target.
4. Run simulaor
   Type `make run` to run the simualor.