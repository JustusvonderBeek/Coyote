#include "cSchedManager.hpp"

namespace fpga
{

	cSchedManager* cSchedManager::s_Instance = nullptr;

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
		// 	munmap(vfidToOpcodeRunningMap, sizeof(vfidToOpcodeRunningMap));
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
		}
		else
		{
			syslog(LOG_NOTICE, "Found scheduler for vfid %d.", vfid);
			returnSched = foundScheduler->second;
		}

		return returnSched;
	}


	void cSchedManager::_insertThread(cSLThread *thread) {
		// Updating the map
		if (thread == nullptr)
			return;
		syslog(LOG_NOTICE, "Trying to add new Thread (Map: %lu)", schedMap.size());
		if (vfidToOpcodeRunningMap.find(thread->cproc->vfid) == vfidToOpcodeRunningMap.end()) {
			auto pair = std::make_pair(&thread->cproc->csched->curr_oid, thread);
			vfidToOpcodeRunningMap.insert_or_assign(thread->cproc->vfid, pair);
			syslog(LOG_NOTICE, "Added/updated thread into map (Size: %lu)",  vfidToOpcodeRunningMap.size());
		}
		notifyMap.insert_or_assign(thread->cproc->pid, thread);
		syslog(LOG_NOTICE, "Added thread in notify map (Size: %lu)", notifyMap.size());
	}

	void cSchedManager::_removeThread(cSLThread *thread) {
		if (thread == nullptr)
			return;
		int vfid = thread->cproc->vfid;
		vfidToOpcodeRunningMap.erase(vfid);
		syslog(LOG_NOTICE, "Removed thread on vFPGA %d", vfid);
	}

	void print_map(std::unordered_map<int, std::pair<int32_t *, cSLThread *>> map) {
		auto iter = map.begin();
		syslog(LOG_NOTICE, "-------------------------------------");
		syslog(LOG_NOTICE, "Size: %lu", map.size());
		while(iter != map.end()) {
			int vfid = iter->first;
			std::pair<int32_t *, cSLThread *> content = iter->second;
			syslog(LOG_NOTICE, "VFID: %d", vfid);
			if (content.first == nullptr) {
				syslog(LOG_NOTICE, "OID: 0, Running: 0");
			} else {
				syslog(LOG_NOTICE, "OID: %d (internal %d), Running: %d ", *content.first, content.second->cproc->csched->curr_oid, content.second->cproc->csched->curr_running);
			}
			++iter;
		}
		syslog(LOG_NOTICE, "-------------------------------------");
	}

	void cSchedManager::_scheduleTask(std::unique_ptr<bTask> ctask, cSLThread *thread) {
		if (thread == nullptr)
			return;
		// Updating the map
		// s_Instance->insertThread(thread);

		auto iter = vfidToOpcodeRunningMap.begin();
		cSLThread *tmpThread = nullptr;
		print_map(vfidToOpcodeRunningMap);
		while (iter != vfidToOpcodeRunningMap.end()) {
			std::pair<int32_t *, cSLThread *> pair = iter->second;
			if (pair.first == nullptr)
				continue;
			if (*pair.first == ctask->getOid()) {
				syslog(LOG_NOTICE, "Found vFPGA (%d) running the same OID (%d) as requested (%d)!", iter->first, *pair.first, ctask->getOid());
				pair.second->emplaceTask(std::move(ctask));
				return;
			}
			if (*pair.first == -1 && pair.second != nullptr && pair.second->cproc->csched->curr_running == 0 && tmpThread == nullptr) {
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

	void cSchedManager::_notifyOrigin(std::unique_ptr<bTask> ctask) {
		cSLThread *thread = notifyMap[ctask->getOrigin()];
		thread->emplaceFinished(ctask->getTid());
	}

	void cSchedManager::StartRunning()
{

	syslog(LOG_NOTICE, "Length of schedMap: %lu", s_Instance->schedMap.size());
	int counter = 0;
	thread *thread_map[s_Instance->schedMap.size()];
	for(auto itr = s_Instance->schedMap.begin(); itr != s_Instance->schedMap.end(); itr++)
	{
		syslog(LOG_NOTICE, "Starting service on vFPGA %d", counter);
		thread *t = itr->second->run();
		thread_map[counter++] = t;
	}
	// syslog(LOG_NOTICE, "All services started. Waiting for threads (%d) to finish", s_Instance->schedMap.size());
	for (size_t i = 0; i < s_Instance->schedMap.size(); i++)
	{
		thread_map[i]->join();
	}
	// syslog(LOG_NOTICE, "All threads finished");
}

}