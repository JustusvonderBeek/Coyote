#pragma once

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
#include <any>

#include "cSched.hpp"
#include "cSLThread.hpp"
#include "cSchedManager.hpp"

using namespace std;

namespace fpga {

class cSched;
class cSchedManager;

/**
 * @brief Coyote service
 * 
 * Coyote daemon, provides background scheduling service.
 * 
 */
class cSLService : public cSched {
private: 
    // Singleton
    //static cSLService *cservice;
    bool m_bIsRunning = false;
    cSchedManager *schedulingManager;

    // Forks
    pid_t pid;

    // ID
    int32_t vfid = { -1 };
    string service_id;

    // Threads
    bool run_req = { false };
    thread thread_req;
    bool run_rsp = { false };
    thread thread_rsp;

    // Conn
    string socket_name;
    int sockfd;
    int curr_id = { 0 };

    // Clients
    mutex mtx_cli;
    unordered_map<int, std::unique_ptr<cSLThread>> clients;

    // Task map
    unordered_map<int, std::function<void(cProcess*, std::vector<uint64_t>)>> task_map;

    cSLService(int32_t vfid, bool priority = true, bool reorder = true, schedType type = DEFAULT, cSchedManager mgm);

    void daemon_init();
    void socket_init();
    void accept_connection();

    static void sig_handler(int signum);
    
    void process_requests();
    void process_responses();

public:
    bool getRunningStatus(){return m_bIsRunning;}
    void acceptConnection();

    /**
     * @brief Creates a service for a single vFPGA
     * 
     * @param vfid - vVFPGA id
     * @param f_req - Process requests
     * @param f_rsp - Process responses
     */

    static cSLService* getInstance(int32_t vfid, bool priority = true, bool reorder = true, schedType type = DEFAULT, cSchedManager mgm) {
        return new cSLService(vfid, priority, reorder, type, mgm);
    }

    /**
     * @brief Run service
     * 
     * @param f_req - process requests lambda
     * @param f_rsp - process responses lambda
     */
    void run();
    void my_handler(int signum);
    /**
     * @brief Add an arbitrary user task
     * 
     */
    void addTask(int32_t oid, std::function<void(cProcess*, std::vector<uint64_t>)> task);
    void removeTask(int32_t oid);

};


}       