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
			syslog(LOG_NOTICE, "No scheduler for vfid %d found. Creating new one...", vfid);
			returnSched = cSLService::getInstance(vfid, priority, reorder, type, this);
			schedMap.insert({vfid, returnSched});
			vfidToOpcodeRunningMap.insert({vfid, std::make_pair(-1, nullptr)});
		}
		else
		{
			syslog(LOG_NOTICE, "Found scheduler for vfid %d.", vfid);
			returnSched = foundScheduler->second;
		}

		return returnSched;
	}

	void cSchedManager::insertThread(cSLThread *thread) {
		// Updating the map
		if (thread == nullptr)
			return;
		auto pair = std::make_pair(thread->cproc->csched->curr_oid, thread);
		s_Instance->vfidToOpcodeRunningMap.insert_or_assign(thread->cproc->vfid, pair);
		syslog(LOG_NOTICE, "Added/updated thread into map");
	}

	void cSchedManager::removeThread(cSLThread *thread) {
		if (thread == nullptr)
			return;
		int vfid = thread->cproc->vfid;
		s_Instance->vfidToOpcodeRunningMap.erase(vfid);
		syslog(LOG_NOTICE, "Removed thread on vFPGA %d", vfid);
	}

	void cSchedManager::scheduleTask(std::unique_ptr<bTask> ctask, cSLThread *thread) {
		if (thread == nullptr)
			return;
		// Updating the map
		s_Instance->insertThread(thread);

		auto iter = s_Instance->vfidToOpcodeRunningMap.begin();
		cSLThread *tmpThread = nullptr;
		while (iter != s_Instance->vfidToOpcodeRunningMap.end()) {
			std::pair<int, cSLThread *> pair = iter->second;
			if (pair.first == ctask->getOid()) {
				syslog(LOG_NOTICE, "Found vFPGA (%d) running the same OID (%d) as requested (%d)!", iter->first, pair.first, ctask->getOid());
				pair.second->emplaceTask(std::move(ctask));
				return;
			}
			if (pair.first == -1 && pair.second != nullptr && pair.second->cproc->csched->curr_running == 0 && tmpThread == nullptr) {
				tmpThread = iter->second.second;
			}
			++iter;
		}
		// If no vFPGA can be found schedule on a free one
		if (tmpThread != nullptr) {
			syslog(LOG_NOTICE, "No vFPGA found running OID %d. Scheduling on empty vFPGA %d", ctask->getOid(), tmpThread->cproc->vfid);
			tmpThread->emplaceTask(std::move(ctask));
			return;
		}

		syslog(LOG_NOTICE, "No vFPGA found running OID %d and all vFPGA busy. Scheduling on original vFPGA (%d)", ctask->getOid(), thread->cproc->vfid);
		thread->emplaceTask(std::move(ctask));
	}

	void cSchedManager::StartRunning()
{

	syslog(LOG_NOTICE, "Length of schedMap: %lu", s_Instance->schedMap.size());
	int counter = 0;
	for(auto itr = s_Instance->schedMap.begin(); itr != s_Instance->schedMap.end(); itr++)
	{
		syslog(LOG_NOTICE, "Starting service on vFPGA %d", counter++);
		itr->second->run();
	}
	syslog(LOG_NOTICE, "All services started");
}
	
}