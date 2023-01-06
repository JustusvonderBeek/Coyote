#include "cSchedManager.hpp"

namespace fpga
{
	cSchedManager* cSchedManager::s_Instance = nullptr;

	cSchedManager::cSchedManager()
	{
	}

	cService* cSchedManager::getSchedulerWithID(int32_t vfid, bool priority, bool reorder, schedType type)
	{
		cService* returnSched = nullptr;

		auto foundScheduler = schedMap.find(vfid);
		
		if(foundScheduler == schedMap.end())
		{
			returnSched = cService::getInstance(vfid, priority, reorder, type);
			schedMap.insert({vfid, returnSched});
		}
		else
		{
			returnSched = foundScheduler->second;
		}

		return returnSched;
	}
}