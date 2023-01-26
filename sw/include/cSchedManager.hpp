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
#include <sys/shm.h>
#include <sys/ipc.h>
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

#include "cSLThread.hpp"
#include "cTask.hpp"
#include "cSLService.hpp"
#include "cSched.hpp"

#define SHM_FNAME "/schedulerMapping"

using namespace std;
namespace fpga
{
	class cSLService;
	class cSLThread;
	class cSchedManager
	{
		private:
			// Forks
			pid_t pid;
			static cSchedManager *s_Instance;
			string socket_thread_name;
			string socket_task_name;

			// Threads
    		bool run_threads = { false };
    		thread thread_threads;
    		bool run_tasks = { false };
    		thread thread_tasks;

			cSchedManager();
			std::unordered_map<int, cSLService*> schedMap;
			std::unordered_map<int, std::pair<int32_t *, cSLThread *>> vfidToOpcodeRunningMap;
			std::unordered_map<int, cSLThread *> notifyMap;
			cSLService* getSchedulerWithID(int32_t vfid, bool priority = true, bool 
			reorder = true, schedType type= DEFAULT);
			void socket_init();
			void process_thread_requests();
			void process_task_requests();

			void _insertThread(cSLThread *thread);
			
			void _removeThread(cSLThread *thread);
			
			void _scheduleTask(std::unique_ptr<bTask> ctask, cSLThread *thread);
			void _notifyOrigin(std::unique_ptr<bTask> ctask);

		public:
			int sockfd;
    		int sockfd_task;

			static void sig_handler(int signum);
			void my_handler(int signum);
			
			static cSLService* getScheduler(int32_t vfid, bool priority = true, bool reorder = true, schedType type= DEFAULT)
			{
				if(s_Instance == nullptr)
				{
					s_Instance = new cSchedManager();

					// Start both sockets
					// s_Instance->socket_init("/tmp/coyote-schedManager-thread", &s_Instance->sockfd);
					// s_Instance->socket_init("/tmp/coyote-schedManager-task", &s_Instance->sockfd_task);

    				// thread_threads = std::thread(&cSchedManager::process_thread_requests, this);
					// thread_tasks = std::thread(&cSchedManager::process_task_requests, this);

					// Signal handler
					signal(SIGTERM, cSchedManager::sig_handler);
					signal(SIGCHLD, SIG_IGN);
					signal(SIGHUP, SIG_IGN);
					syslog(LOG_NOTICE, "CREATED NEW SCHED MANAGER!");
				}
				return s_Instance->getSchedulerWithID(vfid, priority, reorder, type);
			}

			void insertThread(cSLThread *thread) {
				syslog(LOG_NOTICE, "Inserting thread");
				if (s_Instance == nullptr)
					return;
				return s_Instance->_insertThread(thread);
			}
			
			void removeThread(cSLThread *thread) {
				return s_Instance->_removeThread(thread);
			}
			
			void scheduleTask(std::unique_ptr<bTask> ctask, cSLThread *thread) {
				return s_Instance->_scheduleTask(std::move(ctask), thread);
			}

			void notifyOrigin(std::unique_ptr<bTask> ctask) {
				return s_Instance->_notifyOrigin(std::move(ctask));
			}

			static void StartRunning();
	};
}