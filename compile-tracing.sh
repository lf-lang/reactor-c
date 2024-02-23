rm -rf build
mkdir build
cd build
cmake .. -UNUMBER_OF_WORKERS -DLF_TRACE=YES -DLOG_LEVEL=4
cmake --build .
