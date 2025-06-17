\file README.md
\ingroup RTI
# RTI Installation Instructions

This folder contains the source code for the Run-Time Infrastructure (RTI) that
is necessary for federated Lingua Franca programs.

By default, as of version 1.0, when you run `lfc` on a federated Lingua Franca program, a copy of
the RTI code will be included in the generated code, compiled, and invoked by the
launch scripts.  This mechanism ensures that you always get the version of the
RTI that matches your version of lfc.

Here, we provide instructions to manually compile and install the RTI.
For example, in the directory `core/federated/RTI` of the `reactor-c` repository, do:

```bash
mkdir build && cd build
cmake ../
make
sudo make install
```

To run the unit tests
```bash
make test
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

To build a docker image for the RTI, do 
```bash
docker build -t lflang/rti:latest -f rti.Dockerfile ../../../
```

To push it to DockerHub, run:
```bash
docker push lflang/rti:latest
```

You may need to login first:
```bash
docker login -u [username]
```

To authenticate, request a PAT on [DockerHub](https://hub.docker.com/settings/security).

