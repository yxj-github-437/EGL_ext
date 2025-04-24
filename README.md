# EGL_ext

A EGL warpper for android support Wayland display and X11 display

## Introductions

On Android, EGL supports PLATFORM-ANDROID, but in termux, we
may need to support EGL for Wayland or X11.
This project is different from Mesa, it implements the wayland-android
protocol and gralloc_adapter to enable shared hardware memory.
In the future, consider learning gl4es to implement GL interface

## Features

- [x] EGL
  - [x] EGL_PLATFORM_SURFACELESS_MESA
  - [x] EGL_PLATFORM_GBM_KHR
  - [x] EGL_PLATFORM_WAYLAND_EXT
  - [x] EGL_WL_bind_wayland_display
  - [ ] EGL_PLATFORM_X11_EXT
  - [ ] EGL_PLATFORM_XCB_EXT
  - [ ] hybris_EXT
- [x] GLES
  - [x] GLESv1_CM
  - [x] GLESv2
- [ ] GL
- [ ] GLX

## Getting Started

1. Build this project
2. Get libEGL.so.1 libGLESv1_CM.so.1 libGLESv2.so.2
3. Test this project with weston in termux

To build, run:

```sh
cmake -B build -S /this-project-path -DANDROID_PLATFORM=android-21 -DANDROID_ABI=arm64-v8a -DANDROID_STL=c++_shared -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake -GNinja
```
To test, run:
```sh
for i in libEGL.so.1 libGLESv1_CM.so.1 libGLESv2.so.2; do
    ln -sf /path/$i $PREFIX/lib/$i
done
weston -B vnc --renderer=gl
####  run you wayland program
```
