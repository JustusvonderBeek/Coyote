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

using namespace std;
namespace fpga
{
	class cService;
	class cSchedManager
	{
		private:
			// Forks
			pid_t pid;
			static cSchedManager* s_Instance;

			cSchedManager();
			std::unordered_map<int, cService*> schedMap;
			cService* getSchedulerWithID(int32_t vfid, bool priority = true, bool reorder = true, schedType type= DEFAULT);

		public:
			static void sig_handler(int signum);
			void my_handler(int signum);
			
			static cService* getScheduler(int32_t vfid, bool priority = true, bool reorder = true, schedType type= DEFAULT)
			{
				if(s_Instance == nullptr)
				{
					s_Instance = new cSchedManager();
					// Signal handler
					signal(SIGTERM, cSchedManager::sig_handler);
					signal(SIGCHLD, SIG_IGN);
					signal(SIGHUP, SIG_IGN);

				}
				return s_Instance->getSchedulerWithID(vfid, priority, reorder, type);
			}

			static void StartRunning();

		
	};
}