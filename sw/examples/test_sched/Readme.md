# Scheduling Test Application
This application is meant to analyze the behavior of the scheduler. It is split into a service accepting requests and a client creating concrete tasks and submitting these to the service. 

## Setup
The test can be build like all others in the following way.

    mkdir build_test_sched_service && cd build_test_sched_service
    cmake ../sw -DTARGET_DIR=examples/test_sched/service
    make -j
    cd ..

    mkdir build_test_sched_client && cd build_test_sched_client
    cmake ../sw -DTARGET_DIR=examples/test_sched/client
    make -j

# Running
Once the application has been build, it can be executed. First, the service has to be started running in the background as a daemon. Then the client can query requests to the service. Both applications allow for a few configuration options.

    # Running on vFPGA 0
    cd build_test_sched_service
    sudo ./main -v 0

    # Requesting user tasks to run on vFPGA 0 (sleeping for 1s and requesting only 1 task)
    cd build_test_sched_client
    sudo ./main -v 0 -s 1 -i 1

