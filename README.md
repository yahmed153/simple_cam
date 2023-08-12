# simple_cam
Simple Cam App

## Build App
meson setup build
ninja -j 1 -C build # 1 parallel job if on rasp pi 4 or it will OOM

## Run App with libcamera debug logs
LIBCAMERA_LOG_LEVELS=0 ./build/src/simple_cam
