#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h> 
#include <sys/time.h>  
#include <chrono>
#include <iomanip>
#include <fcntl.h>

#include "cSched.hpp"

using namespace std::chrono;

namespace fpga {

// ======-------------------------------------------------------------------------------
// cSched management
// ======-------------------------------------------------------------------------------

/**
 * @brief Construct a new cSched, bitstream handler
 * 
 * @param vfid - vFPGA id
 */
cSched::cSched(int32_t vfid, bool priority, bool reorder, schedType type) 
    : vfid(vfid), priority(priority), reorder(reorder), type(type),
      mlock(open_or_create, "vpga_mtx_mem_" + vfid),
      plock(open_or_create, "vpga_mtx_user_" + vfid),
      request_queue(taskCmprSched(priority, reorder, type)) 
{
    // unique_lock<mutex> lck_q(mtx_queue);

	syslog(LOG_NOTICE, "(DBG!) Acquiring cSched: %d", vfid);
	// Open
	std::string region = "/dev/fpga" + std::to_string(vfid);
	fd = open(region.c_str(), O_RDWR | O_SYNC); 
	if(fd == -1)
		throw std::runtime_error("cSched could not be obtained, vfid: " + to_string(vfid));

	syslog(LOG_NOTICE, "vFPGA device FD %d obtained in sched", vfid);

	// Cnfg
	uint64_t tmp[2];

	if(ioctl(fd, IOCTL_READ_CNFG, &tmp)) 
		throw std::runtime_error("ioctl_read_cnfg() failed, vfid: " + to_string(vfid));

	fcnfg.parseCnfg(tmp[0]);

    // Thread
    syslog(LOG_NOTICE, "cSched:  initial lock");

	// // pid_t pid = fork();
	// // if (pid == 0) {
	// // 	processRequests();
	// // }
    // scheduler_thread = thread(&cSched::processRequests, this);
    // syslog(LOG_NOTICE, "cSched:  thread started, vfid: %d", vfid);
	
    // cv_queue.wait(lck_q);
    // DBG2("cSched:  ctor finished, vfid: " << vfid);
}

/**
 * @brief Destructor cSched
 * 
 */
cSched::~cSched() 
{
	DBG3("cSched:  dtor called, vfid: " << vfid);
    run = false;

    DBG3("cSched:  joining");
    scheduler_thread.join();
	
    // Mapped
    for(auto& it: bstreams) {
		removeBitstream(it.first);
	}

	for(auto& it: mapped_pages) {
		freeMem(it.first);
	}

    named_mutex::remove("vfpga_mtx_mem_" + vfid);

	//close(fd);
}

// ======-------------------------------------------------------------------------------
// (Thread) Process requests
// ======-------------------------------------------------------------------------------
void cSched::startRequests() {
    unique_lock<mutex> lck_q(mtx_queue);
	
    scheduler_thread = thread(&cSched::processRequests, this);
    syslog(LOG_NOTICE, "cSched:  thread started, vfid: %d", vfid);
	
    cv_queue.wait(lck_q);
    syslog(LOG_NOTICE, "cSched:  ctor finished, vfid: %d", vfid);
}

void cSched::processRequests() 
{
    unique_lock<mutex> lck_queue(mtx_queue);
    unique_lock<mutex> lck_complete(mtx_cmpl);
    run = true;
    bool recIssued = false;
    int32_t curr_oid = -1;
	syslog(LOG_NOTICE, "Starting processing requests in the scheduler...");
    lck_queue.unlock();
	cv_queue.notify_one();
	// syslog(LOG_NOTICE, "After first lock");

	int lifesign = 0;
    while(run || !request_queue.empty()) {
        if (lifesign++ % 60000 == 0) {
			// syslog(LOG_NOTICE, "Waiting for lock lck_q");
			syslog(LOG_NOTICE, "Request Queue Empty: %d", request_queue.empty());
			syslog(LOG_NOTICE, "Request Queue Size: %lu", request_queue.size());
		}
		lck_queue.lock();
        if(!request_queue.empty()) {
            // Grab next reconfig request
            auto curr_req = std::move(const_cast<std::unique_ptr<cLoad>&>(request_queue.top()));
			request_queue.pop();
            lck_queue.unlock();

			syslog(LOG_NOTICE, "Checking reconfiguration..");

            // Obtain vFPGA
            // This fails if a file lock already exists (therefore it would be smart to change the lock type to a file_lock for testing)
			// Can be ignored in single vFPGA and scheduler testing
			plock.lock();
			// syslog(LOG_NOTICE, "After plock.lock()");

            // Check whether reconfiguration is needed
            if(isReconfigurable()) {
				// syslog(LOG_NOTICE, "Reconfiguration possible");
                if(reorder)
					// syslog(LOG_NOTICE, "Reordering possible");
                    if(curr_oid != curr_req->oid) {
						syslog(LOG_NOTICE, "Different opcode %d running (Req: %d)", curr_oid, curr_req->oid);
						if (bstreams.find(curr_req->oid) != bstreams.end()) {
							// syslog(LOG_NOTICE, "Starting reconfiguration");
							auto bstream = bstreams[curr_req->oid];
							reconfigure(bstream.first, bstream.second);
                        	recIssued = true;
							curr_oid = curr_req->oid;
						} else {
							syslog(LOG_NOTICE, "Requested bitstream %d not found!", curr_req->oid);
							plock.unlock();
							throw std::runtime_error("Requested bitstream " + to_string(curr_req->oid) + " not found!");
						}
                    } else {
                        recIssued = false;
                    }
            } else {
				syslog(LOG_NOTICE, "Reconfiguration not possible");
			}

            // Notify
            curr_cpid = curr_req->cpid;
            syslog(LOG_NOTICE, "Request from: %d", curr_cpid);
            lck_complete.unlock();
            cv_cmpl.notify_all();

            // Wait for completion or time out (5 seconds)
            // if(cv_cmpl.wait_for(lck_complete, cmplTimeout, []{return true;})) {
            //     syslog(LOG_NOTICE, "Completion received");

			// syslog(LOG_NOTICE, "Reconfig request: %s, vfid: %d, cpid: %d, oid: %d, prio: %u", (recIssued ? "oid loaded" : "oid present"), getVfid(), curr_req->cpid, curr_req->oid, curr_req->priority);
            // } else {
            //    syslog(LOG_NOTICE, "Timeout, flushing ...");
            // }

            plock.unlock();

        } else {
            lck_queue.unlock();
        }
        nanosleep(&PAUSE, NULL);
    }
}

// This function is not allowed to return until the fpga is reprogrammed (and the program can execute)
void cSched::pLock(int32_t cpid, int32_t oid, uint32_t priority) {
    unique_lock<std::mutex> lck_queue(mtx_queue);
    request_queue.emplace(std::unique_ptr<cLoad>(new cLoad{cpid, oid, priority}));
	// syslog(LOG_NOTICE, "Task enqueued: %lu", request_queue.size());
    lck_queue.unlock();
	// Waiting for reconfiguration to finish
    // auto now = std::chrono::system_clock::now();
	// syslog(LOG_NOTICE, "Waiting for task to finish");
	int ret = 1;
    unique_lock<std::mutex> lck_complete(mtx_cmpl);
	//  // Waiting for 
	// //  ret = cv_cmpl.wait_until(lck_complete, now + 10000ms, [=]{ return cpid == curr_cpid; });
	ret = cv_cmpl.wait(lck_complete, [=]{ return cpid == curr_cpid; });
	while (ret != 1 && curr_oid != oid) {
		ret = cv_cmpl.wait(lck_complete, [=]{ return cpid == curr_cpid; });
	}
	// if (ret == 0)
	// 	syslog(LOG_NOTICE, "Timeout in enqeue!");
	syslog(LOG_NOTICE, "Exiting task enqueued");
}

void cSched::pUnlock(int32_t cpid) {
    if(cpid == curr_cpid) {
		// plock.unlock();
        cv_cmpl.notify_one();
    }
}

// ======-------------------------------------------------------------------------------
// Memory management
// ======-------------------------------------------------------------------------------

/**
 * @brief Bitstream memory allocation
 * 
 * @param cs_alloc - allocatin config
 * @return void* - pointer to allocated mem
 */
void* cSched::getMem(const csAlloc& cs_alloc) 
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
				throw std::runtime_error("unauthorized memory allocation, vfid: " + to_string(vfid));
		}

		mapped_pages.emplace(mem, std::make_pair(cs_alloc, memNonAligned));
		DBG2("Mapped mem at: " << std::hex << reinterpret_cast<uint64_t>(mem) << std::dec);
	}

	return mem;
}

/**
 * @brief Bitstream memory deallocation
 * 
 * @param vaddr - mapped al
 */
void cSched::freeMem(void* vaddr) 
{
	uint64_t tmp[2];
	uint32_t size;

	tmp[0] = reinterpret_cast<uint64_t>(vaddr);

	if(mapped_pages.find(vaddr) != mapped_pages.end()) {
		auto mapped = mapped_pages[vaddr];
		
		switch (mapped.first.alloc) {
		
		case CoyoteAlloc::RCNFG_2M :

			mLock();

			if(munmap(mapped.second, (mapped.first.n_pages + 1) * hugePageSize) != 0) {
				mUnlock();
				throw std::runtime_error("free_pr_mem munmap failed");
			}
				
			
			if(ioctl(fd, IOCTL_FREE_HOST_PR_MEM, &vaddr)) {
				mUnlock();
				throw std::runtime_error("ioctl_free_host_pr_mem failed");
			}
				
			mUnlock();

			break;

		default:
            throw std::runtime_error("unauthorized memory deallocation, vfid: " + to_string(vfid)); 
		}

		mapped_pages.erase(vaddr);
	}
}

// ======-------------------------------------------------------------------------------
// Reconfiguration
// ======-------------------------------------------------------------------------------

/**
 * @brief Reconfiguration IO
 * 
 * @param vaddr - bitstream pointer
 * @param len - bitstream length
 */
void cSched::reconfigure(void *vaddr, uint32_t len) 
{
	syslog(LOG_NOTICE, "Reconfiguration called");
	if(fcnfg.en_pr) {
		uint64_t tmp[2];
		tmp[0] = reinterpret_cast<uint64_t>(vaddr);
		tmp[1] = static_cast<uint64_t>(len);
		syslog(LOG_NOTICE, "Reconfiguring...");
		unique_lock<mutex> lck_reconfig(reconfigLock);
		if(ioctl(fd, IOCTL_RECONFIG_LOAD, &tmp)) { // Blocking
			syslog(LOG_NOTICE, "Reconfiguration failed!");
			lck_reconfig.unlock();
			throw std::runtime_error("ioctl_reconfig_load failed");
		}
		lck_reconfig.unlock();
	}
	syslog(LOG_NOTICE, "Reconfiguration completed");
}

// Util
uint8_t cSched::readByte(ifstream& fb) 
{
	char temp;
	fb.read(&temp, 1);
	return (uint8_t)temp;
}

/**
 * @brief Add a bitstream to the map
 * 
 * @param name - path
 * @param oid - operator ID
 */
void cSched::addBitstream(std::string name, int32_t oid) 
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

		syslog(LOG_NOTICE, "Bitstream loaded, oid: %d",oid);
		f_bit.close();

		bstreams.insert({oid, std::make_pair(vaddr, len)});	
		return;
	}		

	throw std::runtime_error("bitstream with same operation ID already present");
}

/**
 * @brief Remove a bitstream from the map
 * 
 * @param: oid - Operator ID
 */
void cSched::removeBitstream(int32_t oid) 
{
	if(bstreams.find(oid) != bstreams.end()) {
		auto bstream = bstreams[oid];
		freeMem(bstream.first);
		bstreams.erase(oid);
	}
}

/**
 * @brief Check if bitstream is present
 * 
 * @param oid - Operator ID
 */
bool cSched::checkBitstream(int32_t oid) 
{
	if(bstreams.find(oid) != bstreams.end()) {
		return true;
	}
	return false;
}

}
