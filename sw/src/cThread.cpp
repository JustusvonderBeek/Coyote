#include "cThread.hpp"

#include <syslog.h>

namespace fpga {

// ======-------------------------------------------------------------------------------
// Ctor, dtor
// ======-------------------------------------------------------------------------------

void cThread::startThread()
{
    // Thread
    unique_lock<mutex> lck(mtx_task);
    syslog(LOG_NOTICE, "cThread:  initial lock");

    c_thread = thread(&cThread::processRequests, this);
    syslog(LOG_NOTICE, "cThread:  thread started");

    cv_task.wait(lck);
    syslog(LOG_NOTICE, "cThread:  ctor finished");
}

cThread::cThread(int32_t vfid, pid_t pid, cSched *csched)   :
      task_queue(taskCmprTask(true, true))  
{ 
    // cProcess
    cproc = std::make_shared<cProcess>(vfid, pid, csched);

    // Thread
    startThread();
}

cThread::cThread(std::shared_ptr<cProcess> cproc) :
      task_queue(taskCmprTask(true, true))  
{ 
    // cProcess
    this->cproc = cproc;

    // Thread
    startThread();
}


//cThread::cThread(cThread &cthread):
//	task_queue(cthread.task_queue)  
//{
//    // cProcess
//    this->cproc = cthread.getCprocess();
//
    // Thread
//    startThread();
//}

cThread::~cThread() 
{
    // cProcess
    if(cproc_own) {
        cproc->~cProcess();
    }

    // Thread
    DBG3("cThread:  dtor called");
    run = false;

    DBG3("cThread:  joining");
    c_thread.join();
}


// ======-------------------------------------------------------------------------------
// Main thread
// ======-------------------------------------------------------------------------------

void cThread::processRequests() {
    unique_lock<mutex> lck(mtx_task);
    run = true;
    lck.unlock();
    cv_task.notify_one();

    while(run || !task_queue.empty()) {
        lck.lock();
        if(!task_queue.empty()) {
            if(task_queue.top() != nullptr) {
                // Remove next task from the queue
                auto curr_task = std::move(const_cast<std::unique_ptr<bTask>&>(task_queue.top()));
                task_queue.pop();
                lck.unlock();

                syslog(LOG_NOTICE, "Process task: vfid: %d , tid: %d, oid: %d, prio: %u", cproc->getVfid(), curr_task->getTid(),curr_task->getOid(), curr_task->getPriority());

                // Run the task     
                // First reprogram the vFPGA if necessary
                cproc->pLock(curr_task->getOid(), curr_task->getPriority());           
                // Then execute the user functionality
                curr_task->run(cproc.get());
                // Freeing the vFPGA
                cproc->pUnlock();

                syslog(LOG_NOTICE, "Task %d processed", curr_task->getTid());

                // Completion
                cnt_cmpl++;
                mtx_cmpl.lock();
                cmpl_queue.push(curr_task->getTid());
                mtx_cmpl.unlock();
                 
            } else {
                task_queue.pop();
                lck.unlock();
            }
        } else {
            lck.unlock();
        }

        nanosleep(&PAUSE, NULL);
    }
}

// ======-------------------------------------------------------------------------------
// Schedule
// ======-------------------------------------------------------------------------------

int32_t cThread::getCompletedNext() {
    if(!cmpl_queue.empty()) {
        lock_guard<mutex> lck(mtx_cmpl);
        int32_t tid = cmpl_queue.front();
        cmpl_queue.pop();
        return tid;
    } 
    return -1;
}

void cThread::scheduleTask(std::unique_ptr<bTask> ctask) {
    lock_guard<mutex> lck2(mtx_task);
    task_queue.emplace(std::move(ctask));
	//std::unique_ptr<cLoad>(new cLoad{cpid, oid, priority})
}

}