#pragma once

#include <iostream> 
#include <algorithm>
#include <vector>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <limits>
#include <unordered_map>
#include <syslog.h>

#include "cDefs.hpp"
#include "cProcess.hpp"
#include "cTask.hpp"
#include "cSchedManager.hpp"
#include "cSched.hpp"

using namespace std;

namespace fpga {

class cSched;
class cSchedManager;

/**
 * @brief Coyote thread
 * 
 * This is a thread abstraction. Each cThread object is attached to one of the available cProcess objects.
 * Multiple cThread objects can run within the same cProcess object.
 * Just like normal threads, these cThreads can share the state.
 * 
 */
class cSLThread {
private:
    /* Trhead */
    thread c_thread;
    bool run = { false };

    /* cProcess */
    bool cproc_own = { false }; 

    /* cSched */
    cSched *csched = { nullptr };

    /* Task queue */
    mutex mtx_task;
    condition_variable cv_task;
    queue<std::unique_ptr<bTask>> task_queue;
    cSchedManager *schedulingManager;

    /* Completion queue */
    mutex mtx_cmpl;
    queue<int32_t> cmpl_queue;
    std::atomic<int32_t> cnt_cmpl = { 0 };

    void startThread();
    void processRequests();

public:

    std::shared_ptr<cProcess> cproc;

    /**
	 * @brief Ctor, Dtor
	 * 
	 */
    cSLThread(int32_t vfid, pid_t pid, cSched *csched = nullptr, cSchedManager *mgm = nullptr); // create cProcess as well
    cSLThread(std::shared_ptr<cProcess> cproc); // provide existing cProc
    cSLThread(cSLThread &cthread); // copy constructor
    ~cSLThread();

    /**
     * @brief Getters, setters
     *
     */
    inline auto getCprocess() { return cproc; }
    inline auto getCompletedCnt() { return cnt_cmpl.load(); }
    inline auto getSize() { return task_queue.size(); }

    /**
     * @brief Completion
     * 
     */
    void emplaceFinished(int32_t tid);
    int32_t getCompletedNext();

    /**
     * @brief Schedule a task
     * 
     * @param ctask - lambda to be scheduled
     */
    void scheduleTask(std::unique_ptr<bTask> ctask);
    void emplaceTask(std::unique_ptr<bTask> ctask);
    

};

}