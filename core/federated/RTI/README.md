This folder contains the source code for the Run-Time Infrastructure (RTI) that
is necessary for federated Lingua Franca programs. To compile and install, do:

```bash
mkdir build && cd build
cmake ../
make
sudo make install
```

**Note:** To enable DEBUG messages, use the following build commands instead:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=DEBUG ../
make
sudo make install
```

If you would like to go back to the non-DEBUG mode, you would have to remove all contents of the `build` folder.

**Note:** To enable simple HMAC-based authentication of federates,
add `-DAUTH=ON` option to the cmake command as shown below:

```bash
mkdir build && cd build
cmake -DAUTH=ON ../
make
sudo make install
```

If you would like to go back to non-AUTH mode, you would have to remove all contents of the `build` folder.

**Note:** To enable SST security of RTI and federates,
add `-DSST=ON` option to the cmake command as shown below:

```bash
cd ../security/RTI
mkdir build && cd build
cmake -DSST=ON ../
make
sudo make install
```

If you would like to go back to non-SST mode, you would have to remove all contents of the `build` folder.

To build a docker image for the RTI, do 
```bash
docker build -t rti:rti -f rti.Dockerfile ../../../core/
```