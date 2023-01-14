# Testing Reconfiguration
This code snippet is meant to test the functionality of the reconfiguration as it is currently implemented in the repository. No dependencies or other code is required to execute the functionality of changing the currently running code on the vFPGA. 

## Setup
The program can be setup and compiled like all other examples in the repository.

    mkdir build_test_reconfig && cd build_test_reconfig
    cmake ../sw -DTARGET_DIR=examples/test_reconfig
    make -j

## Running
After compilation the program can be executed

    sudo ./main

## Problems
We noticed during our testing that the reconfiguration often failed. We did not investigate further to analyze where exactly the problem lies but would suspect the driver or hardware to be of problem.