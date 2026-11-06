# Utvecklingsbygge: fristående app (arm64, Debug), testfallen, stilprovet och panelbilden.
set -e
cmake -B build-dev -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build-dev --target Rostverket_Standalone RVSnapshot RVRender
./build-dev/RVRender_artefacts/Debug/RVRender --test
./build-dev/RVSnapshot_artefacts/Debug/RVSnapshot --klicktest
./build-dev/RVSnapshot_artefacts/Debug/RVSnapshot --stilprov docs/stilprov.png 2
./build-dev/RVSnapshot_artefacts/Debug/RVSnapshot docs/panelen.png 2
open build-dev/Rostverket_artefacts/Debug/Standalone/Rostverket.app
