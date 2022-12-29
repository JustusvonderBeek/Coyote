# Multiple different services running in parallel to test the scheduler
This benchmark evaluates the different available scheduler algorithms and their performance difference.

# Setup
The benchmark can be compiled and setup in the following way.

    mkdir build_reconfiguration_bench_hw && cd build_reconfiguration_bench_hw
    cmake ../hw -DFDEV_NAME=u50 -DEXAMPLE=reconfiguration_bench
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