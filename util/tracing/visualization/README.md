# Trace sequence diagram visualizer

`fedsd` is a utility that reports the interactions (exchanged messages)
between federates and the RTI in a sequence-diagram-like format. 
It also reports the interactions between different encalves in an enclaved execution, where no RTI trace file is provided.


To use `fedsd`, you need to first obtain an execution trace. To do this, enable the tracing mechanism in your Lingua Franca program by setting the `tracing` target property to `true` and then compile and run the program.

This utility starts by transforming each `.lft` file into a `.csv` file, by
internally running `trace_to_csv`. It then aggregates the data from all `.csv`
files to do the matching and draw the sequence diagram.

# Installing
`fedsd` is installed together with the rest of the tracing tools. For instructions refer to `util/tracing/README.md`.


# Running

In case the federation is launched using the `bash` script under `bin`, an `.lft` trace
file will be generated for each of the federates, in addition to `rti.lft`. The latter
contains the RTI trace.

If, however, the federation is launched manually, then running the `RTI` command should be passed the `-t` flag in order to make sure that it, too, has tracing enabled:
```bash
RTI -n <number_of_federates> -t
```

It is most convenient to launch the RTI and all federates from the same working directory so that they will all write their trace file to that directory.

Once the federation stopped executing, running `fedsd` will operate on all the `.lft` files in the current directory:
```bash
fedsd
```
It is also possible to operate on specific files. In such a case, run `fedsd` with `-r` flag to provide the RTI trace file, and `-f` flag to provide the list of federate.

```bash
fedsd -r <rti.lft> -f <federate__f1.lft> <federate__f2.lft>
```

If the trace is too long, the target time interval can be specified. Running `fedsd` with `-s <start_time_value> <time_unit>` will show the messages with the tag later than or equal to the start time and with `-e <end_time_value> <time_unit>` will show the messages with the tag strictly earlier than the end_time.

```bash
fedsd -s <start_time_value> <time_unit> -e <end_time_value> <time_unit>
```

The output is an html file named `trace_svg.html` (in the current directory) that contains the sequence of interactions between the federates and the RTI.
