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
#include "cSchedManager.hpp"
#include "cProcess.hpp"

using namespace std;
using namespace fpga;

// Operators
constexpr auto const opSleep = 1;

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

	for(int currScheduler = 0; currScheduler < vfid; currScheduler++)
	{
		    cService *cservice = cSchedManager::getScheduler(currScheduler);

		    auto prnt = [](cProcess *cprocess, std::vector<uint64_t> params)
		    {
			syslog(LOG_NOTICE, "This is task0 that runs for %lds, it starts now...", params[0]);
			sleep(params[0]);
			syslog(LOG_NOTICE, "Done, waking up");
		    };

		    auto prnt1 = [](cProcess *cprocess, std::vector<uint64_t> params)
		    {
			syslog(LOG_NOTICE, "This is task1 that runs for %lds, it starts now...", params[0]);
			sleep(params[0]);
			syslog(LOG_NOTICE, "Done, waking up");
		    };

		     auto prnt2 = [](cProcess *cprocess, std::vector<uint64_t> params)
		    {
			syslog(LOG_NOTICE, "This is task2 that runs for %lds, it starts now...", params[0]);
			sleep(params[0]);
			syslog(LOG_NOTICE, "Done, waking up");
		    };

		     auto prnt3 = [](cProcess *cprocess, std::vector<uint64_t> params)
		    {
			syslog(LOG_NOTICE, "This is task3 that runs for %lds, it starts now...", params[0]);
			sleep(params[0]);
			syslog(LOG_NOTICE, "Done, waking up");
		    };

		     auto prnt4 = [](cProcess *cprocess, std::vector<uint64_t> params)
		    {
			syslog(LOG_NOTICE, "This is task4 that runs for %lds, it starts now...", params[0]);
			sleep(params[0]);
			syslog(LOG_NOTICE, "Done, waking up");
		    };

		    int32_t sleepTask0 = 1;
		    int32_t sleepTask1 = 2;
		    int32_t sleepTask2 = 3;
		    int32_t sleepTask3 = 4;
		    int32_t sleepTask4 = 5;
		    cservice->addTask(sleepTask0, prnt);
		    cservice->addTask(sleepTask1, prnt);
		    cservice->addTask(sleepTask2, prnt);
		    cservice->addTask(sleepTask3, prnt);
		    cservice->addTask(sleepTask4, prnt);

	}
	cSchedManager::StartRunning();
}
