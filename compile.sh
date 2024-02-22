rm -rf build
mkdir build
cd build
cmake .. -UNUMBER_OF_WORKERS
cmake --build .
