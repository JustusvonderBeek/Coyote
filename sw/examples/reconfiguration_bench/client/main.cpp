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

using namespace std;
using namespace fpga;

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
        ("size,s", boost::program_options::value<uint32_t>(), "Data size")
        ("iterations,i", boost::program_options::value<uint32_t>(), "Iterations per application")
        ("applications,n", boost::program_options::value<uint32_t>(), "Number of apps to schedule per iteration")
    ;

    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    uint32_t size = defSize;
    if(commandLineArgs.count("size") > 0) size = commandLineArgs["size"].as<uint32_t>();

    // Some data ...
    void *hMem = memalign(axiDataWidth, size);
    for(int i = 0; i < size / 8; i++) {
        ((uint64_t*) hMem)[i] = rand();
    }

    // 
    // Open a UDS and sent a task request
    // This is the only place of interaction needed with Coyote daemon
    // 
    cLib clib("/tmp/coyote-daemon-vfid-0");

    auto timeBegin = std::chrono::high_resolution_clock::now();

    // First request is the addmul operator
    clib.task({opIdAddMul, {(uint64_t) hMem, (uint64_t) size, (uint64_t) defAdd, (uint64_t) defMul}});

    // Now we perform some rotation 
    clib.task({opIdRotate, {(uint64_t) hMem, (uint64_t) size}});

    // Some statistics on this data, first minimum and maximum
    clib.task({opIdMinMax, {(uint64_t) hMem, (uint64_t) size}});

    // Finally, perform the count + select operation
    clib.task({opIdSelect, {(uint64_t) hMem, (uint64_t) size, (uint64_t) defType, (uint64_t) defPredicate}});
    
    // TODO: Implement some more tasks to evaluate the scheduling performance
    clib.task({opIdSelect, {(uint64_t) hMem, (uint64_t) size, (uint64_t) defType, (uint64_t) defPredicate}});

    auto timeEnd = std::chrono::high_resolution_clock::now();
    using dsec = std::chrono::duration<double>;
    double dur = std::chrono::duration_cast<dsec>(timeEnd-timeBegin).count();
    printf("Duration: %.6fs\n", dur);

    return (EXIT_SUCCESS);
}
