# Release: VST3, AU och CLAP (universal), kopieras till ~/Library/Audio/Plug-Ins.
set -e
cmake -B build -G Xcode
cmake --build build --config Release --target Rostverket_All
cp -R build/Rostverket_artefacts/Release/VST3/Rostverket.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/Rostverket_artefacts/Release/AU/Rostverket.component ~/Library/Audio/Plug-Ins/Components/
cp -R build/Rostverket_artefacts/Release/CLAP/Rostverket.clap ~/Library/Audio/Plug-Ins/CLAP/
