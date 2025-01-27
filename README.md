# SWE-Web

SWE-Web is an interactive cross-platform 3D application for simulating artificial and realistic tsunamis.
By supporting [NetCDF](https://en.wikipedia.org/wiki/NetCDF) data input, the application can load the [bathymetry](https://en.wikipedia.org/wiki/Bathymetry) of real oceans and the displacement caused by earthquakes.

A live version of the Web-App is available at [swe-web.de](https://swe-web.de/), running the binary files in the `Public` directory of `main`.

## Libraries Used
- [glfw](https://github.com/glfw/glfw)
- [bgfx](https://github.com/bkaradzic/bgfx)
- [imgui](https://github.com/ocornut/imgui)
- [netcdf-cxx4](https://github.com/Unidata/netcdf-cxx4)

The dependencies are fetched and built automatically when generating/compiling the project (except for NetCDF).

## Rendering APIs
Various rendering APIs are used depending on the platform, thanks to bgfx:
- **D3D11** on Windows
- **Metal** on MacOS
- **OpenGL** on Linux (Vulkan caused some issues)
- **OpenGLES** on Emscripten (WebGL)

## Features
- **Scenarios:**
  - 4 built-in Tsunami scenarios: Tohoku (2011) and Tohoku (Zoom); Chile (2014); Artificial Tsunami
  - 1 custom scenario that takes NetCDF input files for bathymetry and displacement. Examples can be downloaded [here](https://tumde-my.sharepoint.com/:f:/g/personal/erik_lauterwald_tum_de/Eod1ZmKOPutLs8_TxyevuFMB6wDQbcHuwaQ64LJddqgR0A?e=gHidv3)
- **Views:**
  - Different view types for water height, momentums, bathymetry, and surface level (h, hu, hv, b, h+b)
- **Boundaries:**
  - Two boundary types: Outflow and Wall
- **Simulation Controls:**
  - Reapply the initial displacement to create new waves at runtime
  - Time scaling to slow down the simulation
  - Specify an end simulation time at which the simulation stops
- **Visualization:**
  - Set the data range for a customizable 3-color gradient or use automatic scaling
  - Scale the z-value of wet and dry cells for better visualization
  - Intuitive 3D camera mouse controls
  - Toggle seamlessly between orthographic and perspective camera
  - Choose between wireframe and solid rendering
  - Drag-and-drop NetCDF files to import or auto-load depending on if the selection window is open

## How to Build

First, create a `Build` directory and navigate into it.

### Generate Build Files

#### Native Desktop-App (Linux and Mac)
```
cmake ..
```
Note: `netcdf-cxx4` (on Linux `netcdf_c++4`) must be installed by a package manager.

##### Or using NetCDF from [vcpkg](https://github.com/microsoft/vcpkg) (Windows)
```
vcpkg install netcdf-cxx4:x64-windows-static
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static
```
This works for all platforms. Other triplets: `x64-linux`, `x64-osx`, `arm64-osx`, `x64-mingw-dynamic`, etc.

#### Web-App
Without NetCDF, excluding the NetCDF-Scenario:
```
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -DENABLE_NETCDF=OFF
```
Or, when `emsdk_env.sh` is sourced:
```
emcmake cmake .. -DENABLE_NETCDF=OFF
```

##### With NetCDF (requires manual patching because Emscripten is not supported officially)
Apply these [patches](https://gist.github.com/erikrl2/1d3b0ef856538fd09d6fd5c80f74c269) to the vcpkg ports of hdf5 and netcdf-c and then do:
```
vcpkg install netcdf-cxx4:wasm32-emscripten
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=/path/to/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -DVCPKG_TARGET_TRIPLET=wasm32-emscripten
```

#### Notes
You can always disable NetCDF linkage with `-DENABLE_NETCDF=OFF`

### Compile
```
cmake --build . --target SWE-App
```
Or open project files on Windows using Visual Studio.

### Start

#### Desktop-App
```
./SWE-App
```

#### Web-App
If `emsdk_env.sh` is sourced:
```
emrun SWE-App.html
```
Or manually host a local server using `python3 -m http.server` or `npx http-server` to run `SWE-App.html` in the browser.

## Additional Notes
- Emscripten cross-compiling is testet with emsdk version 3.1.74. Earlier versions might not work.
- When switching target platforms, you might need to clean the compiled bgfx shaders by calling `make -f Scripts/shader.mk clean`.
