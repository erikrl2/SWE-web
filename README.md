## SWE-(web)

### How to build

#### Desktop

Just use `cmake` and `make` as usual. Platform is detected automatically.

#### Emscripten

Follow instructions [here](https://github.com/emscripten-core/emsdk) to install
and activate emsdk. Source `emsdk_env.sh` to set up the environment.

To build an run this project

``` sh
cd swe-web
emcmake cmake ..
emmake make SWE-App
emrun SWE-App.html
```
