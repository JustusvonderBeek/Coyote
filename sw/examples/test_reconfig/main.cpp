#include <dirent.h>
#include <iterator>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstdio>
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
#include <unordered_set> 
#include <sys/ioctl.h>
#include <mutex>
#include <condition_variable>
#include <boost/program_options.hpp>
#include <boost/functional/hash.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <queue>
#include <tuple>
#include <sys/mman.h>
#include <atomic>
#include <inttypes.h>
#include <fcntl.h>
#include <byteswap.h>
#include <fstream>


#include "cDefs.hpp"
// #include "cService.hpp"

using namespace std;
using namespace fpga;
using namespace boost::interprocess;

// Runtime
constexpr auto const defTargetRegion = 0;
constexpr auto const defFolder = "../build_reconfiguration_bench_hw/bitstreams/";
constexpr auto const defConfigFolder = "config_";

// Operators
constexpr auto const opIdAddMul = 1;
constexpr auto const opIdMinMax = 2;
constexpr auto const opIdRotate = 3;
constexpr auto const opIdSelect = 4;


int32_t fd;
named_mutex Mlock(open_or_create, "vpga_mtx_mem_" + 0);
named_mutex Plock(open_or_create, "vpga_mtx_user_" + 0);
mutex reconfigLock; // Debug locking the reconfiguration to a single thread
fCnfg fcnfg;
using mappedVal = std::pair<csAlloc, void*>; // n_pages, vaddr_non_aligned
using bStream = std::pair<void*, uint32_t>; // vaddr*, length
/* Bitstream memory */
std::unordered_map<void*, mappedVal> mapped_pages;
std::unordered_map<int32_t, bStream> bstreams;

// Util
uint8_t readByte(ifstream& fb) 
{
	char temp;
	fb.read(&temp, 1);
	return (uint8_t)temp;
}

/* Internal locks */
inline auto mLock() { Mlock.lock(); }
inline auto mUnlock() { Mlock.unlock(); }

void* getMem(const csAlloc& cs_alloc) 
{
	void *mem = nullptr;
	void *memNonAligned = nullptr;
	uint64_t tmp[2];
	uint32_t size;

	if(cs_alloc.n_pages > 0) {
		tmp[0] = static_cast<uint64_t>(cs_alloc.n_pages);

		switch (cs_alloc.alloc) {
			case CoyoteAlloc::RCNFG_2M : // m lock

                mLock();

				if(ioctl(fd, IOCTL_ALLOC_HOST_PR_MEM, &tmp)) {
                    mUnlock();
					throw std::runtime_error("ioctl_alloc_host_pr_mem mapping failed");
                }
				
				memNonAligned = mmap(NULL, (cs_alloc.n_pages + 1) * hugePageSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmapPr);
				if(memNonAligned == MAP_FAILED) {
                    mUnlock();
					throw std::runtime_error("get_pr_mem mmap failed");
                }

                mUnlock();

				mem =  (void*)( (((reinterpret_cast<uint64_t>(memNonAligned) + hugePageSize - 1) >> hugePageShift)) << hugePageShift);

				break;

			default:
				throw std::runtime_error("unauthorized memory allocation, vfid: ");
		}

		mapped_pages.emplace(mem, std::make_pair(cs_alloc, memNonAligned));
		// DBG2("Mapped mem at: " << std::hex << reinterpret_cast<uint64_t>(mem) << std::dec);
	}

	return mem;
}

void addBitstream(std::string name, int32_t oid) 
{	
	if(bstreams.find(oid) == bstreams.end()) {
		// Stream
		ifstream f_bit(name, ios::ate | ios::binary);
		if(!f_bit) 
			throw std::runtime_error("Bitstream could not be opened '" + name + "'");

		// Size
		uint32_t len = f_bit.tellg();
		f_bit.seekg(0);
		uint32_t n_pages = (len + hugePageSize - 1) / hugePageSize;
		
		// Get mem
		void* vaddr = getMem({CoyoteAlloc::RCNFG_2M, n_pages});
		uint32_t* vaddr_32 = reinterpret_cast<uint32_t*>(vaddr);

		// Read in
		for(uint32_t i = 0; i < len/4; i++) {
			vaddr_32[i] = 0;
			vaddr_32[i] |= readByte(f_bit) << 24;
			vaddr_32[i] |= readByte(f_bit) << 16;
			vaddr_32[i] |= readByte(f_bit) << 8;
			vaddr_32[i] |= readByte(f_bit);
		}

		printf("Bitstream loaded, oid: %d\n", oid);
		f_bit.close();

		bstreams.insert({oid, std::make_pair(vaddr, len)});	
		return;
	}		

	throw std::runtime_error("bitstream with same operation ID already present");
}


void reconfigure(void *vaddr, uint32_t len) 
{
    // Thread
	printf("Reconfiguration called\n");
	if(fcnfg.en_pr) {
		uint64_t tmp[2];
		tmp[0] = reinterpret_cast<uint64_t>(vaddr);
		tmp[1] = static_cast<uint64_t>(len);
		printf("Reconfiguring...\n");
		unique_lock<mutex> lck_reconfig(reconfigLock);
		if(ioctl(fd, IOCTL_RECONFIG_LOAD, &tmp)) { // Blocking
			printf("Reconfiguration failed!\n");
			lck_reconfig.unlock();
			throw std::runtime_error("ioctl_reconfig_load failed");
		}
		lck_reconfig.unlock();
	}
	printf("Reconfiguration completed\n");
}

/**
 * @brief Main
 *  
 */
int main(int argc, char *argv[]) 
{   
    /* Args */
    boost::program_options::options_description programDescription("Options:");
    programDescription.add_options()
        ("vfid,i", boost::program_options::value<uint32_t>(), "Target vFPGA")
        ("folder,f", boost::program_options::value<string>(), "Bitstream folder")
    ;
    
    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    uint32_t vfid = defTargetRegion;
    string folder = defFolder;
    string config = defConfigFolder;
    if(commandLineArgs.count("vfid") > 0) vfid = commandLineArgs["vfid"].as<uint32_t>();
    if(commandLineArgs.count("folder") > 0) folder = commandLineArgs["folder"].as<string>();

    /* Create a daemon */
    // cService *cservice = cService::getInstance(vfid);
    std::string region = "/dev/fpga" + std::to_string(vfid);
	fd = open(region.c_str(), O_RDWR | O_SYNC); 
	if(fd == -1)
		throw std::runtime_error("cSched could not be obtained");

	printf("vFPGA device FD %d obtained\n", vfid);

	// Cnfg
	uint64_t tmp[2];

	if(ioctl(fd, IOCTL_READ_CNFG, &tmp)) 
		throw std::runtime_error("ioctl_read_cnfg() failed, vfid: " + to_string(vfid));

	fcnfg.parseCnfg(tmp[0]);

    std::string location = folder + config;

    // Load addmul bitstream
    addBitstream(location + "0/" + "part_bstream_c0_" + std::to_string(vfid) + ".bin", opIdAddMul);
    addBitstream(location + "1/" + "part_bstream_c1_" + std::to_string(vfid) + ".bin", opIdMinMax);
    addBitstream(location + "2/" + "part_bstream_c2_" + std::to_string(vfid) + ".bin", opIdRotate);
    addBitstream(location + "3/" + "part_bstream_c3_" + std::to_string(vfid) + ".bin", opIdSelect);

    auto addMulStream = bstreams[opIdAddMul];
    auto minMaxStream = bstreams[opIdMinMax];
    auto rotateStream = bstreams[opIdRotate];
    auto selectStream = bstreams[opIdSelect];

    reconfigure(addMulStream.first, addMulStream.second);
    reconfigure(rotateStream.first, rotateStream.second);
    reconfigure(minMaxStream.first, minMaxStream.second);
    reconfigure(selectStream.first, selectStream.second);
}