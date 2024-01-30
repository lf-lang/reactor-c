rm -rf build
cd ../../../plugin-defaults/trace/ && ./build.sh && cd -
cmake -S . -B build
cmake --build build
