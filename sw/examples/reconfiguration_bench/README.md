# Multiple different services running in parallel to test the scheduler
This benchmark evaluates the different available scheduler algorithms and their performance difference.

# Setup
The benchmark can be compiled and setup in the following way.

    mkdir build_reconfiguration_bench_hw && cd build_reconfiguration_bench_hw
    cmake ../hw -DFDEV_NAME=u50 -DEXAMPLE=reconfiguration_bench -DCOMP_CORES=120
    make -j shell && make -j compile

    cd .. && mkdir build_reconfiguration_bench_sw_service && cd build_reconfiguration_bench_sw_service
    cmake ../sw -DTARGET_DIR=examples/reconfiguration_bench/service
    make -j
    
    # Below should not be necessary anymore
    (# Copy the bitstreams to the current folder for execution
    cp ../build_reconfiguration_bench_hw/bitstreams/config_*/*.bin .)


    cd .. && mkdir build_reconfiguration_bench_sw_client && build_reconfiguration_bench_sw_client
    cmake ../sw -DTARGET_DIR=examples/reconfiguration_bench/client
    make -j

# Running the benchmark
The benchmark allows to specify a few different options. The commands are listed below.

TODO:

# How it works
The following section summarizes how the scheduling in this example works. It should make clear the improvements introduced in the scheduler. The original paper identifies missing **fairness**, **starvation** and **predictability** of the current implementation. We have therefore concentrated on ** todo ** in our scheduler improvement.

1. The service daemon loads all four possible application benchmarks bitstreams for the applications
2. Then, the services are configured for the issued tasks
3. Next, the service is started and listens for incoming client requests
4. Requests are read and scheduled to run on the allocated vFPGA resource(s) // TODO: Check if 1 or multiple vFPGAs are used
5. After completion the next task from the list is processed // TODO: Issue with tasks running indefinitely
6. At the end of client requests the service daemon is closed