#include <iostream>
#include <string>
#include <malloc.h>
#include <time.h> 
#include <sys/time.h>  
#include <chrono>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <iomanip>
#include <x86intrin.h>
#include <boost/program_options.hpp>
#include <sys/socket.h>
#include <sys/un.h>
#include <sstream>

#include "cLib.hpp"
#include "cSched.hpp"
#include "cProcess.hpp"

using namespace std;
using namespace fpga;

// Evaluation parameter
constexpr auto const defIterations = 1;
constexpr auto const defApplications = 4;

// Runtime
constexpr auto const defSize = 4 * 1024;
constexpr auto const defAdd = 10;
constexpr auto const defMul = 2;
constexpr auto const defType = 0;
constexpr auto const defPredicate = 100;

// Operators
constexpr auto const opIdAddMul = 1;
constexpr auto const opIdMinMax = 2;
constexpr auto const opIdRotate = 3;
constexpr auto const opIdSelect = 4;

int main(int argc, char *argv[]) 
{
    // Read arguments
    boost::program_options::options_description programDescription("Options:");
    programDescription.add_options()
        ("vfid,v", boost::program_options::value<uint32_t>(), "vFPGA ID")
    ;

    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    uint32_t vfid = 0;
    if(commandLineArgs.count("vfid") > 0) vfid = commandLineArgs["vfid"].as<uint32_t>();

    // TODO: Implement creating and using our scheduler
    cSched default_sched(0, true, true, DEFAULT);
    cSched time_sched(0, true, true, TIME_DEPENDENT);

    std::string location = "../build_reconfiguration_bench_hw/bitstreams/config_";

    // // Adding tasks into the scheduler
    default_sched.addBitstream(location + "0/" + "part_bstream_c0_" + std::to_string(vfid) + ".bin", opIdAddMul);
    default_sched.addBitstream(location + "1/" + "part_bstream_c1_" + std::to_string(vfid) + ".bin", opIdAddMul);
    default_sched.addBitstream(location + "2/" + "part_bstream_c2_" + std::to_string(vfid) + ".bin", opIdAddMul);
    default_sched.addBitstream(location + "3/" + "part_bstream_c3_" + std::to_string(vfid) + ".bin", opIdAddMul);

    uint32_t size = 4 * 1024;
    void *hMem = memalign(axiDataWidth, size);
    void *rMem = memalign(axiDataWidth, size);
    for(int i = 0; i < size / 8; i++) {
        ((uint64_t*) hMem)[i] = rand();
        ((uint64_t*) rMem)[i] = rand();
    }

    // Testing for a single process
    cProcess def_cproc(0, 0, &default_sched);
    cProcess time_cproc(0, 1, &time_sched);

    auto timeBegin = std::chrono::high_resolution_clock::now();

    // First the addition

    def_cproc.setCSR(10, 0);
    def_cproc.setCSR(2, 1);
    def_cproc.userMap(hMem, size);
    // def_cproc.userMap(rMem, size);
    printf("Starting first operation: add\n");
    def_cproc.invoke({CoyoteOper::TRANSFER, hMem, hMem, size, size});
    while(def_cproc.checkCompleted(CoyoteOper::TRANSFER) != 1) { 
        nanosleep((const struct timespec[]){{0, 100L}}, NULL); 
    } 
    printf("Operation returned, unmapping...\n");
    def_cproc.userUnmap(hMem);
    // def_cproc.userUnmap(rMem);

    auto timeEnd = std::chrono::high_resolution_clock::now();
    using dsec = std::chrono::duration<double>;
    double dur = std::chrono::duration_cast<dsec>(timeEnd-timeBegin).count();
    printf("Duration: %.6fs\n", dur);

    // Next the Rotation
    timeBegin = std::chrono::high_resolution_clock::now();
    def_cproc.userMap(hMem, size);
    // def_cproc.userMap(rMem, size);
    printf("Starting second operation: rotate\n");
    def_cproc.invoke({CoyoteOper::TRANSFER, hMem, hMem, size, size});
    printf("Operation returned, unmapping...\n");
    def_cproc.userUnmap(hMem);
    // def_cproc.userUnmap(rMem);

    timeEnd = std::chrono::high_resolution_clock::now();
    dur = std::chrono::duration_cast<dsec>(timeEnd-timeBegin).count();
    printf("Duration: %.6fs\n", dur);

    // ID min max
    timeBegin = std::chrono::high_resolution_clock::now();

    def_cproc.setCSR(0x1, 1);
    def_cproc.userMap(hMem, size);
    printf("Starting third operation: min max\n");
    def_cproc.invoke({CoyoteOper::READ, hMem, size});
    while(def_cproc.getCSR(1) != 0x1) { 
        nanosleep((const struct timespec[]){{0, 100L}}, NULL); 
    }
    printf("Operation returned, unmapping...\n");
    def_cproc.userUnmap(hMem);

    timeEnd = std::chrono::high_resolution_clock::now();
    dur = std::chrono::duration_cast<dsec>(timeEnd-timeBegin).count();
    printf("Duration: %.6fs\n", dur);

    // Last the select
    timeBegin = std::chrono::high_resolution_clock::now();

    def_cproc.setCSR(0, 2);
    def_cproc.setCSR(100, 3);
    def_cproc.setCSR(0x1, 0);
    def_cproc.userMap(hMem, size);
    printf("Starting last operation: select\n");
    def_cproc.invoke({CoyoteOper::READ, hMem, size});
    while(def_cproc.getCSR(1) != 0x1) {
        nanosleep((const struct timespec[]){{0, 100L}}, NULL); 
    }
    printf("Operation returned, unmapping...\n");
    def_cproc.userUnmap(hMem);
    
    timeEnd = std::chrono::high_resolution_clock::now();
    dur = std::chrono::duration_cast<dsec>(timeEnd-timeBegin).count();
    printf("Duration: %.6fs\n", dur);

    return (EXIT_SUCCESS);
}
