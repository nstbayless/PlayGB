PDX="./CrankBoy.pdx"
ZIP="$PDX.zip"

set -e
set -x

if [ -d "$PDX" ]; then
    rm -r "$PDX"
fi
if [ -f "$ZIP" ]; then
    rm "$ZIP"
fi
make device
7z a -tzip "$ZIP" "$PDX"
