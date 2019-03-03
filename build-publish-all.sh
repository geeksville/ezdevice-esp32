set -e

./incbuild.sh
export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T4 -DAUTOBUILD"
pio run # -v
./publish-firmware.sh R

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T5s -DAUTOBUILD"
pio run # -v
./publish-firmware.sh M

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_T5_23 -DAUTOBUILD"
pio run # -v
./publish-firmware.sh L

export PLATFORMIO_BUILD_FLAGS="-DBOARD_TTGO_O -DAUTOBUILD"
pio run # -v
./publish-firmware.sh O
