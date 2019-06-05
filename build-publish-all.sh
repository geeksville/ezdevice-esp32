set -e

./incbuild.sh
export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T4 -DAUTOBUILD"
# For the first build we do it clean because it seems like build numbers were stale in the main.ino file
pio run -t clean
pio run # -v
./publish-firmware.sh JR

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T5s -DAUTOBUILD"
pio run # -v
./publish-firmware.sh JM

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T5_23 -DAUTOBUILD"
pio run # -v
./publish-firmware.sh JL

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T5_16_RED -DAUTOBUILD"
pio run # -v
./publish-firmware.sh JK

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T5_16_YELLOW -DAUTOBUILD"
pio run # -v
./publish-firmware.sh JY

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_O -DAUTOBUILD"
pio run # -v
./publish-firmware.sh JO

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_GROW -DAUTOBUILD"
pio run # -v
./publish-firmware.sh JG

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_CAMERA -DAUTOBUILD"
pio run # -v
./publish-firmware.sh JC

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T_JOURNAL -DAUTOBUILD"
pio run # -v
./publish-firmware.sh JT

export PLATFORMIO_BUILD_FLAGS="-DBOARD_M5BASIC -DAUTOBUILD"
pio run # -v
./publish-firmware.sh MB

export PLATFORMIO_BUILD_FLAGS="-DBOARD_M5STICK -DAUTOBUILD"
pio run # -v
./publish-firmware.sh MS

