#include "cSchedManager.cpp"

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
			schedMap.insert({vfid, cService::getInstance(vfid, priority, reorder, type)});
		}
		else
		{
			returnSched = foundScheduler->second
		}

		return returnSched;
	}
}