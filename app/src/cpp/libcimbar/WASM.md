## Demo encoder

[cimbar.org](https://cimbar.org)

## Encoder bundle

The self-contained v2 encoder bundle is available as [`cimbar_js.html`](https://github.com/turtleSST/libcimbar/releases/download/v0.6.8-fountain-v2/cimbar_js.html). It can be downloaded and opened in a modern browser without installation.

## Build

To build, use the `package-wasm.sh` script in a docker container:

```
docker run --mount type=bind,source="$(pwd)",target="/usr/src/app" -it emscripten/emsdk:3.1.39
```
Then, inside the container:
```
bash /usr/src/app/package-wasm.sh
```

## Alternative build for the adventurous

Alternatively, if you have a local emscripten setup, you can try to run the package-wasm.sh commands piecemeal:

To build opencv.js:
```
cd /path/to/opencv
mkdir opencv-build-wasm
cd opencv-build-wasm
python3 ../platforms/js/build_js.py build_wasm --build_wasm --emscripten_dir=/path/to/emscripten
```

With opencv.js built:
```
mkdir build-wasm
cd build-wasm
source /path/to/emscripten/emsdk/emsdk_env.sh
emcmake cmake .. -DUSE_WASM=1 -DOPENCV_DIR=/path/to/opencv
make -j5 install
```

(do `-DUSE_WASM=2` to use asm.js instead of wasm)

## What about a WASM cimbar decoder?

Some day!
