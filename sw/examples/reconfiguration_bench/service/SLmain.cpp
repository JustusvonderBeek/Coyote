#include <dirent.h>
#include <iterator>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include <vector>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iomanip>
#include <chrono>
#include <thread>
#include <limits>
#include <assert.h>
#include <stdio.h>
#include <sys/un.h>
#include <errno.h>
#include <wait.h>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <boost/program_options.hpp>
#include <random>

#include "cSLService.hpp"
#include "cSchedManager.hpp"

using namespace std;
using namespace fpga;

// Runtime
constexpr auto const defTargetRegion = 1;
constexpr auto const defFolder = "../build_reconfiguration_bench_hw/bitstreams/";
constexpr auto const defConfigFolder = "config_";
constexpr auto const defSeed = 42;

// Operators
constexpr auto const opIdAddMul = 1;
constexpr auto const opIdMinMax = 2;
constexpr auto const opIdRotate = 3;
constexpr auto const opIdSelect = 4;

// In seconds
constexpr auto const addMulDuration = 0.5;
constexpr auto const minMaxDuration = 1.3;
constexpr auto const rotateDuration = 2.1;
constexpr auto const selectDuration = 1.8;

/**
 * @brief Main
 *  
 */
int main(int argc, char *argv[]) 
{   
    /* Args */
    boost::program_options::options_description programDescription("Options:");
    programDescription.add_options()
        ("vfid,i", boost::program_options::value<uint32_t>(), "Target vFPGA")
        ("folder,f", boost::program_options::value<string>(), "Bitstream folder")
        ("seed,s", boost::program_options::value<string>(), "Seed for random number generator")
    ;
    
    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    uint32_t vfid = defTargetRegion;
    string folder = defFolder;
    string config = defConfigFolder;
    int32_t seed = defSeed;
    if(commandLineArgs.count("vfid") > 0) vfid = commandLineArgs["vfid"].as<uint32_t>();
    if(commandLineArgs.count("folder") > 0) folder = commandLineArgs["folder"].as<string>();
    if(commandLineArgs.count("seed") > 0) seed = commandLineArgs["seed"].as<int32_t>();

	for(int currScheduler = 0; currScheduler < vfid; currScheduler++)
	    {
	    /* Create a daemon */
	    cSLService* cservice = cSchedManager::getScheduler(currScheduler);
	    std::mt19937 rng(seed);

	    /**
	     * @brief Load all operators
	     * 
	     */

	    std::string location = folder + config;

	    // Load addmul bitstream
	    cservice->addBitstream(location + "0/" + "part_bstream_c0_" + std::to_string(currScheduler) + ".bin", opIdAddMul);
	    cservice->addBitstream(location + "1/" + "part_bstream_c1_" + std::to_string(currScheduler) + ".bin", opIdMinMax);
	    cservice->addBitstream(location + "2/" + "part_bstream_c2_" + std::to_string(currScheduler) + ".bin", opIdRotate);
	    cservice->addBitstream(location + "3/" + "part_bstream_c3_" + std::to_string(currScheduler) + ".bin", opIdSelect);

	    // Load addmul operator
	    cservice->addTask(opIdAddMul, [](cProcess *cproc, std::vector<uint64_t> params) { // addr, len, add, mul
		// printf("Added first bitstream\n");
		// Prep
		cproc->setCSR(params[2], 0); // Addition
		cproc->setCSR(params[3], 1); // Multiplication

		// User map
		// cproc->userMap((void*)params[0], (uint32_t)params[1]);

		// Invoke
		// cproc->invoke({CoyoteOper::TRANSFER, (void*)params[0], (void*)params[0], (uint32_t) params[1], (uint32_t) params[1]});
		
		std::uniform_int_distribution<int> offset(1,100);
		int dur = offset(cproc->rng);
		std::this_thread::sleep_for(std::chrono::milliseconds((int)(addMulDuration * 1000)) + std::chrono::milliseconds(dur));
		// Check the result

		// User unmap
		// cproc->userUnmap((void*)params[0]);

		syslog(LOG_NOTICE, "Addmul finished!");
	    });
	    
	    // Load minmax bitstream
	    // cservice->addBitstream(location + "1/" + "part_bstream_c1_" + std::to_string(vfid) + ".bin", opIdMinMax);

	    // Load minmax operator
	    cservice->addTask(opIdMinMax, [] (cProcess *cproc, std::vector<uint64_t> params) { // addr, len
		// Prep
		cproc->setCSR(0x1, 1); // Start kernel

		// User map
		// cproc->userMap((void*)params[0], (uint32_t)params[1]);

		// Invoke
		// cproc->invoke({CoyoteOper::READ, (void*)params[0], (uint32_t) params[1]});
		std::uniform_int_distribution<int> offset(1,500);
		int dur = offset(cproc->rng);
		std::this_thread::sleep_for(std::chrono::milliseconds((int)(minMaxDuration * 1000)) + std::chrono::milliseconds(dur));

		// Check results
		// while(cproc->getCSR(1) != 0x1) {
		//     nanosleep((const struct timespec[]){{0,200L}}, NULL);
		// }
		// if((cproc->getCSR(2) != 10) || (cproc->getCSR(2) != 20))
		//     syslog(LOG_NOTICE, "MinMax failed!");

		// User unmap
		// cproc->userUnmap((void*)params[0]);

		syslog(LOG_NOTICE, "MinMax finished!");
	    });

	    // Load rotate bitstream
	    // cservice->addBitstream(location + "2/" + "part_bstream_c2_" + std::to_string(vfid) + ".bin", opIdRotate);


	    // Load rotete operator
	    cservice->addTask(opIdRotate, [] (cProcess *cproc, std::vector<uint64_t> params) { // addr, len
		// User map
		// cproc->userMap((void*)params[0], (uint32_t)params[1]);

		// Invoke
		// cproc->invoke({CoyoteOper::TRANSFER, (void*)params[0], (void*)params[0], (uint32_t) params[1], (uint32_t) params[1]});
		std::uniform_int_distribution<int> offset(1,1000);
		int dur = offset(cproc->rng);
		std::this_thread::sleep_for(std::chrono::milliseconds((int)(rotateDuration * 1000)) + std::chrono::milliseconds(dur));

		// User unmap
		// cproc->userUnmap((void*)params[0]);

		syslog(LOG_NOTICE, "Rotate finished");
	    });

	    // Load select bitstream
	    // cservice->addBitstream(location + "3/" + "part_bstream_c3_" + std::to_string(vfid) + ".bin", opIdSelect);

	    // Load select
	    cservice->addTask(opIdSelect, [] (cProcess *cproc, std::vector<uint64_t> params) { // addr, len, type, cond
		// Prep
		cproc->setCSR(params[2], 2); // Type of comparison
		cproc->setCSR(params[3], 3); // Predicate
		cproc->setCSR(0x1, 0); // Start kernel
		
		// User map
		// cproc->userMap((void*)params[0], (uint32_t)params[1]);

		// Invoke
		// cproc->invoke({CoyoteOper::READ, (void*)params[0], (uint32_t) params[1]});
		// while(cproc->getCSR(1) != 0x1) { nanosleep((const struct timespec[]){{0, 100L}}, NULL); }
		std::uniform_int_distribution<int> offset(1,800);
		int dur = offset(cproc->rng);
		std::this_thread::sleep_for(std::chrono::milliseconds((int)(selectDuration * 1000)) + std::chrono::milliseconds(dur));

		// User unmap
		// cproc->userUnmap((void*)params[0]);

		syslog(LOG_NOTICE, "Select finished");
	    });

	    /* Run a daemon */
	    //cservice->run();
	}
	cSchedManager::StartRunning();
}

