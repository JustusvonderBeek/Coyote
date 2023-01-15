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
#include <random>

#include "cLib.hpp"

using namespace std;
using namespace fpga;

// Evaluation parameter
constexpr auto const defIterations = 1;
constexpr auto const defApplications = 4;
constexpr auto const defRandom = 42;

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
        ("size,s", boost::program_options::value<uint32_t>(), "Data size")
        ("iterations,i", boost::program_options::value<uint32_t>(), "Iterations per application")
        ("applications,n", boost::program_options::value<uint32_t>(), "Number of apps to schedule per iteration")
        ("random,r", boost::program_options::value<int32_t>(), "Random seed")
    ;

    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    uint32_t size = defSize;
    uint32_t iterations = defIterations;
    uint32_t applications = defApplications;
    uint32_t vfid = 0;
    int32_t random = defRandom;
    if(commandLineArgs.count("size") > 0) size = commandLineArgs["size"].as<uint32_t>();
    if(commandLineArgs.count("iterations") > 0) iterations = commandLineArgs["iterations"].as<uint32_t>();
    if(commandLineArgs.count("applications") > 0) applications = commandLineArgs["applications"].as<uint32_t>();
    if(commandLineArgs.count("vfid") > 0) vfid = commandLineArgs["vfid"].as<uint32_t>();
    if(commandLineArgs.count("random") > 0) random = commandLineArgs["random"].as<int32_t>();
    if (applications > 4)
        applications = 4;
        
    // Some data ...
    void *hMem = memalign(axiDataWidth, size);
    for(int i = 0; i < size / 8; i++) {
        ((uint64_t*) hMem)[i] = rand();
    }

    std::mt19937 rng(random);

    printf("Executing '%d' iterations\n", iterations);
    printf("Adding '%d' applications per iteration\n", applications);

    // 
    // Open a UDS and sent a task request
    // This is the only place of interaction needed with Coyote daemon
    // 
    cLib clib(("/tmp/coyote-daemon-vfid-" + to_string(vfid)).c_str());

    auto timeBegin = std::chrono::high_resolution_clock::now();
    std::uniform_int_distribution<int32_t> possibleOpcodes(opIdAddMul,opIdSelect);

    for (uint32_t i = 0; i < iterations; i++)
    {
        for (uint32_t j = 0; j < applications; j++)
        {
            int opcode = possibleOpcodes(rng);
            switch (opcode)
            {
            case opIdAddMul:
                clib.task({opIdAddMul, {(uint64_t) hMem, (uint64_t) size, (uint64_t) defAdd, (uint64_t) defMul}});
                break;
            case opIdRotate:
                clib.task({opIdRotate, {(uint64_t) hMem, (uint64_t) size}});
                break;
            case opIdMinMax:
                clib.task({opIdMinMax, {(uint64_t) hMem, (uint64_t) size}});
                break;
            case opIdSelect:
                clib.task({opIdSelect, {(uint64_t) hMem, (uint64_t) size, (uint64_t) defType, (uint64_t) defPredicate}});
                break;
            default:
                throw std::runtime_error("Failed to generate a valid opcode! Benchmark run incorrect!");
            }
        }
    }
    
    auto timeEnd = std::chrono::high_resolution_clock::now();
    using dsec = std::chrono::duration<double>;
    double dur = std::chrono::duration_cast<dsec>(timeEnd-timeBegin).count();
    // With all fluctuations we are not more precise than 4 digits anyway
    printf("Duration: %.5fs\n", dur);
    printf("Iteration(s): %d\n", iterations);
    printf("Application(s): %d\n", applications);

    return (EXIT_SUCCESS);
}
