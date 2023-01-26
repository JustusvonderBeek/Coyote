#include "cSLService.hpp"

namespace fpga {

//cSLService* cSLService::cservice = nullptr;

// ======-------------------------------------------------------------------------------
// Ctor, dtor
// ======-------------------------------------------------------------------------------

/**
 * @brief Constructor
 * 
 * @param vfid
 */
cSLService::cSLService(int32_t vfid, bool priority, bool reorder, schedType type, cSchedManager *mgm) 
    : vfid(vfid), schedulingManager(mgm), cSched(vfid, priority, reorder, type)
{
    // ID
    service_id = ("coyote-daemon-vfid-" + std::to_string(vfid)).c_str();
    socket_name = ("/tmp/coyote-daemon-vfid-" + std::to_string(vfid)).c_str();
    // syslog(LOG_NOTICE, "cSLService newly created?");
}

// ======-------------------------------------------------------------------------------
// Sig handler
// ======-------------------------------------------------------------------------------

/**
 * @brief Signal handler 
 * 
 * @param signum : Kill signal
 */
void cSLService::sig_handler(int signum)
{   
    //cservice->my_handler(signum);
}

void cSLService::my_handler(int signum) 
{
    if(signum == SIGTERM) {
        syslog(LOG_NOTICE, "SIGTERM sent to %d\n", (int)pid);//cSLService::getPid());
        unlink(socket_name.c_str());

        run_req = false;
        run_rsp = false;
        thread_req.join();
        thread_rsp.join();

        kill(pid, SIGTERM);
        syslog(LOG_NOTICE, "Exiting");
        closelog();
        exit(EXIT_SUCCESS);
    } else {
        syslog(LOG_NOTICE, "Signal %d not handled", signum);
    }
}

// ======-------------------------------------------------------------------------------
// Init
// ======-------------------------------------------------------------------------------

/**
 * @brief Initialize the daemon service
 * 
 */
pid_t cSLService::daemon_init()
{
    // Fork
    syslog(LOG_NOTICE, "Forking...");
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    pid = fork();
    if(pid < 0 ) 
        exit(EXIT_FAILURE);
    if(pid > 0 ) {
        // exit(EXIT_SUCCESS);
        // We need this in order to be able to start the other services
        return pid;
        // while(true) {
        //     std::this_thread::sleep_for(std::chrono::seconds(20));
        // }
    }

    // syslog(LOG_NOTICE, "Before sid");
    // std::this_thread::sleep_for(std::chrono::seconds(1));

    // Sid
    if(setsid() < 0)
        exit(EXIT_FAILURE);

    // syslog(LOG_NOTICE, "Before sig handler");

    // Signal handler
    //signal(SIGTERM, cSLService::sig_handler);
    //signal(SIGCHLD, SIG_IGN);
    //signal(SIGHUP, SIG_IGN);

    // Fork
    pid = fork();
    if(pid < 0 ) 
        exit(EXIT_FAILURE);
    if(pid > 0 ) 
        exit(EXIT_SUCCESS);

    // Permissions
    umask(0);

    // Cd
    if((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    // Syslog
    openlog(service_id.c_str(), LOG_NOWAIT | LOG_PID, LOG_USER);
    syslog(LOG_NOTICE, "Successfully started %s", service_id.c_str());

    // Close fd
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return pid;
}

/**
 * @brief Initialize listening socket
 * 
 */
void cSLService::socket_init() 
{
    syslog(LOG_NOTICE, "Socket initialization");

    sockfd = -1;
    struct sockaddr_un server;
    socklen_t len;

    if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        syslog(LOG_ERR, "Error creating a server socket");
        exit(EXIT_FAILURE);
    }

    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, socket_name.c_str());
    unlink(server.sun_path);
    len = strlen(server.sun_path) + sizeof(server.sun_family);
    
    if(bind(sockfd, (struct sockaddr *)&server, len) == -1) {
        syslog(LOG_ERR, "Error bind()");
        exit(EXIT_FAILURE);
    }

    if(listen(sockfd, maxNumClients) == -1) {
        syslog(LOG_ERR, "Error listen()");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Accept connections
 * 
 */
void cSLService::accept_connection()
{
    syslog(LOG_NOTICE, "Starting accepting connections");
    while (1)
    {
        sockaddr_un client;
        socklen_t len = sizeof(client); 
        int connfd;
        char recv_buf[recvBuffSize];
        int n;

        if((connfd = accept(sockfd, (struct sockaddr *)&client, &len)) == -1) {
            syslog(LOG_NOTICE, "No new connections");
        } else {
            syslog(LOG_NOTICE, "Connection accepted, connfd: %d", connfd);

            pid_t rpid = 0;
            if(n = read(connfd, recv_buf, sizeof(pid_t)) == sizeof(pid_t)) {
                memcpy(&rpid, recv_buf, sizeof(pid_t));
                syslog(LOG_NOTICE, "Registered pid: %d", rpid);
            } else {
                syslog(LOG_ERR, "Registration failed, connfd: %d, received: %d", connfd, n);
            }

            mtx_cli.lock();
            
            if(clients.find(connfd) == clients.end()) {
                clients.insert({connfd, std::make_unique<cSLThread>(vfid, rpid, this, schedulingManager)});
                syslog(LOG_NOTICE, "Connection thread created");
            }

            mtx_cli.unlock();
        }

        nanosleep((const struct timespec[]){{0, sleepIntervalDaemon}}, NULL);
        
    }
    
}

// ======-------------------------------------------------------------------------------
// Tasks
// ======-------------------------------------------------------------------------------
void cSLService::addTask(int32_t oid, std::function<void(cProcess*, std::vector<uint64_t>)> task) {
    if(task_map.find(oid) == task_map.end()) {
        task_map.insert({oid, task});
    }
}

void cSLService::removeTask(int32_t oid) {
    if(bstreams.find(oid) != bstreams.end()) {
		bstreams.erase(oid);
    }
}

// ======-------------------------------------------------------------------------------
// Threads
// ======-------------------------------------------------------------------------------

void cSLService::process_requests() {
    char recv_buf[recvBuffSize];
    memset(recv_buf, 0, recvBuffSize);
    uint8_t ack_msg;
    int32_t msg_size;
    int n, taskId = 0;
    run_req = true;
    int counter = 0;

    syslog(LOG_NOTICE, "Starting thread");

    while(run_req) {
        for (auto & el : clients) {
            mtx_cli.lock();
            int connfd = el.first;

            if(read(connfd, recv_buf, sizeof(int32_t)) == sizeof(int32_t)) {
                int32_t opcode = int32_t(recv_buf[0]);
                // bool scheduled = false;
                // if (0x80000000 & opcode > 0) {
                //     scheduled = true;
                // }
                // opcode = opcode & 0x7FFFFFFF; // Remove first bit
                syslog(LOG_NOTICE, "Client: %d, opcode: %d", el.first, opcode);

                switch (opcode) {

                // Close connection
                case defOpClose:
                    syslog(LOG_NOTICE, "Received close connection request, connfd: %d", connfd);
                    close(connfd);
                    syslog(LOG_NOTICE, "Closed connection %d", connfd);
                    // schedulingManager->removeThread(el.second.get());
                    syslog(LOG_NOTICE, "Reconfigurations on %d: %d", vfid, reconfigurations);
                    syslog(LOG_NOTICE, "Reconfiguration time on %d: %.6fms", vfid, reconfiguration_time);
                    // Removes the thread (calling the destructor)
                    // clients.erase(el.first);
                    // syslog(LOG_NOTICE, "Destroyed client");
                    break;

                // Schedule the task
                default:
                    // Check bitstreams
                    /*if(isReconfigurable()) {
                        if(!checkBitstream(opcode))
                            syslog(LOG_ERR, "Opcode invalid, connfd: %d, received: %d", connfd, n);
                    }*/

                    // Check task map
                    if(task_map.find(opcode) == task_map.end()) {
                       syslog(LOG_ERR, "Opcode invalid, connfd: %d, received: %d", connfd, n);
                        break;
                    }

                    auto taskIter = task_map.find(opcode);
         
                    // Read the payload size
                    if(n = read(connfd, recv_buf, sizeof(int32_t)) == sizeof(int32_t)) {
                        memcpy(&msg_size, recv_buf, sizeof(int32_t));

                        // Read the payload
                        if(n = read(connfd, recv_buf, msg_size) == msg_size) {
                            std::vector<uint64_t> msg(msg_size / sizeof(uint64_t)); 
                            memcpy(msg.data(), recv_buf, msg_size);
                            
                            // if (scheduled == false)  {
                            //    // Checking with the scheduler
                            //    cLib clib("/tmp/coyote-schedManager");
                            //    clib.task({opcode, {msg}});
                            //    continue;
                            // }

                            syslog(LOG_NOTICE, "Received new request, connfd: %d, msg size: %d", el.first, msg_size);

                            // Schedule
                            el.second->scheduleTask(std::unique_ptr<bTask>(new cTask(taskId++, opcode, 1, el.second->cproc->pid, taskIter->second, msg)));
                            syslog(LOG_NOTICE, "Task scheduled, client %d, opcode %d", el.first, opcode);
                        } else {
                            syslog(LOG_ERR, "Request invalid, connfd: %d, received: %d", connfd, n);
                        }

                    } else {
                        syslog(LOG_ERR, "Payload size not read, connfd: %d, received: %d", connfd, n);
                    }
                    break;

                }
            }

            mtx_cli.unlock();
        }

        if (counter++ % 10000 == 0) {
            syslog(LOG_NOTICE, "Reconfigurations on %d: %d", vfid, reconfigurations);
            syslog(LOG_NOTICE, "Reconfiguration time on %d: %.6fms", vfid, reconfiguration_time);
        }

        nanosleep((const struct timespec[]){{0, sleepIntervalRequests}}, NULL);
    }
}

void cSLService::process_responses() {
    int n;
    int ack_msg;
    run_rsp = true;
    
    while(run_rsp) {

        for (auto & el : clients) {
            int32_t tid = el.second->getCompletedNext();
            if(tid != -1) {
                syslog(LOG_NOTICE, "Running here...");
                int connfd = el.first;

                if(write(connfd, &tid, sizeof(int32_t)) == sizeof(int32_t)) {
                    syslog(LOG_NOTICE, "Sent completion, connfd: %d, tid: %d", connfd, tid);
                } else {
                    syslog(LOG_ERR, "Completion could not be sent, connfd: %d", connfd);
                }
            }
        }

        nanosleep((const struct timespec[]){{0, sleepIntervalCompletion}}, NULL);
    }
}

/**
 * @brief Main run service
 * 
 */
thread *cSLService::run() {

    // printf("Bitstreams: %lu\n", bstreams.size());

    // Init daemon
    // pid_t pid = daemon_init();
    // if (pid > 0)
    //     return nullptr;

    startRequests();

    syslog(LOG_NOTICE, "Bitstreams: %lu", bstreams.size());

    // Init socket
    socket_init();
    
    // Init threads
    syslog(LOG_NOTICE, "Thread initialization");

    thread_req = std::thread(&cSLService::process_requests, this);
    thread_rsp = std::thread(&cSLService::process_responses, this);

    // Should block here...
    thread_accept = std::thread(&cSLService::accept_connection, this);
    // Main
    // while(1) {
    //     accept_connection();
    // }
    // thread_accept.join();
    return &thread_accept;
}

}