set -e

./incbuild.sh
export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T4 -DAUTOBUILD"
# For the first build we do it clean because it seems like build numbers were stale in the main.ino file
pio run -t clean
pio run # -v
./publish-firmware.sh R

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T5s -DAUTOBUILD"
pio run # -v
./publish-firmware.sh M

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T5_23 -DAUTOBUILD"
pio run # -v
./publish-firmware.sh L

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T5_16_RED -DAUTOBUILD"
pio run # -v
./publish-firmware.sh K

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T5_16_YELLOW -DAUTOBUILD"
pio run # -v
./publish-firmware.sh Y

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_O -DAUTOBUILD"
pio run # -v
./publish-firmware.sh O

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_CAMERA -DAUTOBUILD"
pio run # -v
./publish-firmware.sh C

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_CAMERA -DAUTOBUILD"
pio run # -v
./publish-firmware.sh C

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T_JOURNAL -DAUTOBUILD"
pio run # -v
./publish-firmware.sh T

