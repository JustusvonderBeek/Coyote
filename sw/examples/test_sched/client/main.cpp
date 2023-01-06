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

#include "cService.hpp"
#include "cLib.hpp"

using namespace std;
using namespace fpga;

// Runtime
constexpr auto const defSleepSeconds = 1;
constexpr auto const defIterations = 1;

// Operators
constexpr auto const opSleep = 1;

int main(int argc, char *argv[]) 
{
    // Read arguments
    boost::program_options::options_description programDescription("Options:");
    programDescription.add_options()
        ("vfid,v", boost::program_options::value<uint32_t>(), "vFPGA ID")
        ("sleep,s", boost::program_options::value<uint32_t>(), "Time in seconds to sleep")
        ("iterations,i", boost::program_options::value<uint32_t>(), "Iterations per application")
    ;

    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    uint32_t vfid = 0;
    uint32_t sleep = defSleepSeconds;
    uint32_t iterations = defIterations;
    if(commandLineArgs.count("vfid") > 0) vfid = commandLineArgs["vfid"].as<uint32_t>();
    if(commandLineArgs.count("sleep") > 0) sleep = commandLineArgs["sleep"].as<uint32_t>();
    if(commandLineArgs.count("iterations") > 0) iterations = commandLineArgs["iterations"].as<uint32_t>();

    cLib clib(("/tmp/coyote-daemon-vfid-" + to_string(vfid)).c_str());

    for (size_t i = 0; i < iterations; i++)
    {
        clib.task({opSleep, {sleep}});
    }
    

    return (EXIT_SUCCESS);
}
