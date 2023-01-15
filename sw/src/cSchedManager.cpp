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
			returnSched = cSLService::getInstance(vfid, priority, reorder, type, this);
			schedMap.insert({vfid, returnSched});
			vfidToOpcodeRunningMap.insert({vfid, std::make_pair(-1, nullptr)});
		}
		else
		{
			returnSched = foundScheduler->second;
		}

		return returnSched;
	}

	void cSchedManager::scheduleTask(std::unique_ptr<bTask> ctask, cSLThread *thread) {
		
		// Updating the map
		auto pair = std::make_pair(thread->cproc->csched->curr_oid, thread);
		s_Instance->vfidToOpcodeRunningMap.insert_or_assign(thread->cproc->vfid, pair);

		auto iter = s_Instance->vfidToOpcodeRunningMap.begin();
		while (iter != s_Instance->vfidToOpcodeRunningMap.end()) {
			if (iter->second.first == ctask->getOid()) {
				syslog(LOG_NOTICE, "Found vFPGA (%d) running the same OID as requested!", iter->first);
				iter->second.second->emplaceTask(std::move(ctask));
				return;
			}
			iter++;
		}
		// If no vFPGA can be found schedule on the currently running one
		thread->emplaceTask(std::move(ctask));
	}

	void cSchedManager::StartRunning()
{

	for(auto itr = s_Instance->schedMap.begin(); itr != s_Instance->schedMap.end(); itr++)
	{
		itr->second->run();
	}
}
	
}