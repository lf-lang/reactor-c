rm -rf build
mkdir build
cd build
cmake .. -DNUMBER_OF_WORKERS=4
cmake --build .
