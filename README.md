# SWE-Web

## About

SWE-Web is an interactive cross-platform 3D application for simulating artificial and realistic tsunamis.
By supporting [NetCDF](https://en.wikipedia.org/wiki/NetCDF) data input, we can load the [bathymetry](https://en.wikipedia.org/wiki/Bathymetry) of real oceans and the displacement caused by earthquakes and simulate upon that.

A version of the Web-App is live at [swe-web.de](https://swe-web.de/) running the binary files in the `Public` directory of `main`.

Used libraries are [glfw](https://github.com/glfw/glfw), [bgfx](https://github.com/bkaradzic/bgfx), [imgui](https://github.com/ocornut/imgui) and [netcdf-cxx4](https://github.com/Unidata/netcdf-cxx4).
The dependencies are fetched/built automatically when generating/compiling the project (except for netcdf).

Various rendering APIs are used depending on the platform (thanks to bgfx):
- *D3D11* on Windows
- *Metal* on MacOS
- *OpenGL* on Linux (as Vulkan caused some issues)
- *OpenGLES* on Emscripten (WebGL)

## Features

- Selecting and playing scenarios with specified grid sizes
- Different view types for water height, momentums, bathymetry and surface level (h, hu, hv, b, h+b)
- Two boundary types *Outflow* and *Wall*
- Reapply the initial displacement to create new waves at runtime
- Time scaling to slow down the simulation
- Specify an *end simulation time* at which the simulation stops
- Set the data range used for a customizable 3-color gradient or use automatic scaling
- Scale the z-value of the grid for better visualization
- Intuitive 3D camera mouse controls (left: rotate, middle: pan, right: zoom)
- Toggle seamlessly between orthographic and perspective camera
- Choose between wireframe and solid rendering
- Shortcuts for everything

## How to Build

### Generate Build Files

#### Native Desktop-App using normal/non-vcpkg NetCDF Installation (only supported by Linux and Mac)
```sh
cmake .. -DCMAKE_BUILD_TYPE=Release
```
`netcdf-cxx4` (on Linux `netcdf_c++4`) must be installed by package manager.

#### Native Desktop-App using [vcpkg](https://github.com/microsoft/vcpkg) NetCDF Installation (e.g. Windows with Triplet x64-windows-static)
```sh
vcpkg install netcdf-cxx4:x64-windows-static
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static
```
Other triplets are: `x64-linux`, `x64-osx`, `arm64-osx`, `x64-mingw-dynamic`, etc.

#### Web-App without NetCDF
```sh
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/path/to/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
```
Or, when `emsdk_env.sh` is sourced:
```
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
```

#### Web-App with NetCDF
First apply these [patches](https://gist.github.com/erikrl2/1d3b0ef856538fd09d6fd5c80f74c269) to the hdf5 and netcdf-c vcpkg ports.
```sh
vcpkg install netcdf-cxx4:wasm32-emscripten
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=/path/to/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -DVCPKG_TARGET_TRIPLET=wasm32-emscripten
```

#### Notes
- You can disable netcdf linkage with `-DENABLE_NETCDF=OFF`

### Compile
```sh
make SWE-App
```
Or open project files on Windows using Visual Studio.

### Start

#### Desktop-App
```sh
./SWE-App
```

#### Web-App
If `emsdk_env.sh` is sourced:
```sh
emrun SWE-App
```
Or else manually host a local server using `python3 -m http.server` or `npx http-server` and open SWE-App.html in the browser.

## Notes

- The emsdk version used for cross compiling needs to be v3.1.74 or later
- When switching target platform in the same build directory, you might need to clean the compiled bgfx shaders by calling `make -f Scripts/shader.mk clean`
