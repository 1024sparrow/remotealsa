# remotealsa
Test application to send/receive audio to Linux

Builder server 

`
/home/gladish/work/rdkc/xcam2/master/sdk/toolchain/linaro-armv7ahf-2015.11-gcc5.2/bin/arm-linux-gnueabihf-g++
  -std=c++0x
  -I/home/gladish/work/rdkc/xcam2/master/sdk/fsroot/src/amba/prebuild/third-party/armv7-a-hf/alsa-lib/include
  -L/home/gladish/work/rdkc/xcam2/master/sdk/fsroot/src/amba/prebuild/third-party/armv7-a-hf/alsa-lib/usr/lib
  -lasound
  server.cpp
  -o xaudio
  `
