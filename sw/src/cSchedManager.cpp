#include "cSLService.hpp"
#include "cSchedManager.hpp"

namespace fpga
{
	cSchedManager* cSchedManager:: s_Instance = nullptr;

	cSchedManager::cSchedManager()
	{
	}

	void cSchedManager::sig_handler(int signum) 
	{
		s_Instance->my_handler(signum);
	}

	void cSchedManager::my_handler(int signum) 
	{
	    if(signum == SIGTERM) {
		for(auto itr = schedMap.begin(); itr != schedMap.end(); itr++)
		{
			itr->second->my_handler(signum);
		}
	    } else {
		syslog(LOG_NOTICE, "Signal %d not handled", signum);
	    }
	}

	cSLService* cSchedManager::getSchedulerWithID(int32_t vfid, bool priority, bool reorder, schedType type)
	{
		cSLService* returnSched = nullptr;
		auto foundScheduler = schedMap.find(vfid);
		
		if(foundScheduler == schedMap.end())
		{
			returnSched = cSLService::getInstance(vfid, priority, reorder, type);
			schedMap.insert({vfid, returnSched});
		}
		else
		{
			returnSched = foundScheduler->second;
		}

		return returnSched;
	}

	void cSchedManager::StartRunning()
{

	for(auto itr = s_Instance->schedMap.begin(); itr != s_Instance->schedMap.end(); itr++)
	{
		itr->second->run();
	}
}
	
}