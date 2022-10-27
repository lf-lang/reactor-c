# Getting started guide for Zephyr development
Linux is the preferred platform. This guide was written using WSL in Windows which does work but has its quirks.
First part of guide shows you how to get a simple Zephyr program running on a qemu emulation of a CPU.
The second part shows how to run it on a nrf52dk_5832.

## Get the right versions of reactor-c and lingua-franca
Checkout `zephyr-support` branch of lingua-franca and run `~/bin/build-lfc`. You should find some simple Zephyr test programs in `test/C/src/zephyr`. 
Your reactor-c branch at `org.lflang/src/lib/c/reactor-c/` should be at the `zephyr` branch.

## Installing Zephyr
- Follow [this](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) great and simple guide to install Zpehyr, its dependencies and toolchains.
Use the option to install using a virtual environment. 
- Everytime you want to run Zephyr tools you will need to activate the venv. I do: `source ~/dev/zephyr/.venv/bin/activate.fish` (using the fish shell)
- Add the variable ZEPHYR_BASE to point to where you have cloned the zephyr repository. This can be done by adding
```
export ZEPHYR_BASE=/home/erling/dev/zephyr/zephyr
```

## Try an example application
- `cd $ZEPHYR_BASE`
- `west build -p always -b qemu_cortex_m3 samples/hello_world`
- `west build -t run`
Should generate output like
```
west build -t run
-- west build: running target run
[0/1] To exit from QEMU enter: 'CTRL+a, x'[QEMU] CPU: cortex-m3
qemu-system-arm: warning: nic stellaris_enet.0 has no peer
Timer with period zero, disabling
*** Booting Zephyr OS build v3.2.0-rc3-97-gf37db90541e7  ***
Hello World! qemu_cortex_m3
```

Also try to get the debugging working on the QEMU target by following [this](https://docs.zephyrproject.org/3.1.0/develop/application/index.html#application-debugging) guide.


## Build LF HelloWorld
```
cd ~/dev/lingua-franca
bin/lfc -c test/C/src/zephyr/HelloZephyr.lf
```

This should build the HelloWorld.lf for QEMU and start the emulation. You should see something like
```
...
[145/145] Linking C executable zephyr/zephyr.elf
Memory region         Used Size  Region Size  %age Used
           FLASH:       21052 B       256 KB      8.03%
            SRAM:        8384 B        64 KB     12.79%
        IDT_LIST:          0 GB         2 KB      0.00%
--- Executing on QEMU emulation
-- west build: running target run
[0/1] To exit from QEMU enter: 'CTRL+a, x'[QEMU] CPU: cortex-m3
*** Booting Zephyr OS build v3.2.0-rc3-97-gf37db90541e7  ***
Sys Clock has frequency of 12000000 Hz
---- Start execution at time 641916us
logical=0 physical=258500 diff = 258500 Hello World!
qemu-system-arm: warning: nic stellaris_enet.0 has no peer
Timer with period zero, disabling
logical=500000000 physical=500080667 diff = 80667 Hello World!
logical=1000000000 physical=1000057500 diff = 57500 Hello World!
logical=1500000000 physical=1500053250 diff = 53250 Hello World!
logical=2000000000 physical=2000052500 diff = 52500 Hello World!
logical=2500000000 physical=2500060084 diff = 60084 Hello World!
logical=3000000000 physical=3000055834 diff = 55834 Hello World!
logical=3500000000 physical=3500055084 diff = 55084 Hello World!
logical=4000000000 physical=4000054417 diff = 54417 Hello World!
logical=4500000000 physical=4500057250 diff = 57250 Hello World!
logical=5000000000 physical=5000063500 diff = 63500 Hello World!
logical=5500000000 physical=5500059584 diff = 59584 Hello World!
```

## How does it work
- Inspect the target properties of the zephyr LF programs. `build: scripts/zephyr_build.sh` make lfc invoke the script instead of buildint the cmake project directly.
- The build script takes 1-2 arguments, the board and an optional "flash" flag. And does the following:
1. It overwrites a bunch of the runtime files by copying from reactor-c into the src-gen folder. This is to avoid having to rebuild lfc for each change in reactor-c. This can be dropped once we have settled on an implementation of reactor-c.
2. Add find_pacakge(Zephyr ...) to the top of the generated CMakeLists.txt file.
3. Generate Zephyr config files: prj.config and Kconfig. Checkout Zephyr docs and example projects to understand these.
4. Build the project using west.
5. Flash program to NRF board or start executing on QEMU emulation.

- Also check out zephy.cmake which is included in the generated CMakeLists.txt. It uses some hacks to retrieve the sources and compile definitions from the generated CMakeLists and creates a new target based on how Zephyr example projects does it. This is where the final executable is defined.



## Running on NRF52
- Install SEGGER JLink Software and Documentation Pack download [here](https://www.segger.com/downloads/jlink/#J-LinkSoftwareAndDocumentationPack)
- Install NRF command line tools. Download [here](https://www.nordicsemi.com/Products/Development-tools/nRF-Command-Line-Tools/Download?lang=en#infotabs)
- Try running sample Zephyr applications on NRF52 using west build and flash.
- Run LF Blinky program: `bin/lfc -c test/C/src/zephyr/Blinky.lf` and verify that LED1 is indeed blinking
- To get debug access into the NRF you will have to follow [this](https://devzone.nordicsemi.com/guides/nrf-connect-sdk-guides/b/getting-started/posts/using-gdb-with-nordic-devices) guide from Nordic. You need to have a JLink GDB server running in your host machine listening to a specific port (2331 is default). Then you must let GDB connect to that port in order to step through the code
- There is a script in `scripts/zephyr_debug_nrf.sh` which automates this. (It assumes that the JLink GDB server is already running). I normally debug like this:

```
bin/lfc -c test/C/src/zephyr/Blinky.lf
bash test/C/src/scripts/zephyr_debug_nrf.sh test/C/src-gen/zephyr/Blinky
```

I then remove the 'flash' argument to the `build` target property in the LF file.
- To get access to printfs from the NRF52 you have to open a serial monitor. I use putty in windows, in Linux minicom works fine.


## Troubleshooting
- You always have to activate the virtual environment before bulding any zephyr application. Verify that its working by executing `west boards`
- In zephyr_build.sh we generate the prj.conf file where e.g. the heap size is defined. If the LF program tries to allocate more it will crash with a out of memory error.
