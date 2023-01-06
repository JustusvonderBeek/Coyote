#include "cService.hpp"

using namespace std;
namespace fpga
{
	class cSchedManager
	{
		private:
			static cSchedManager* s_Instance;

			cSchedManager();

			unordered_map<int, cService*> schedMap;

			cService* getSchedulerWithID(int32_t vfid, bool priority = true, bool reorder = true, schedType type = DEFAULT);

		public:
			static cService* getScheduler(int32_t vfid, bool priority = true, bool reorder = true, schedType type = DEFAULT)
			{
				if(s_Instance == nullptr)
				{
					s_Instance = new cSchedManager();
				}
				return s_Instance->getSchedulerWithID(vfid, priority, reorder, type);
			}
	};
}