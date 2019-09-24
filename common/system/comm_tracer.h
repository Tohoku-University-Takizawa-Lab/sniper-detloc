#ifndef COMM_TRACER_H
#define COMM_TRACER_H

#include "fixed_types.h"
#include "lock.h"
#include "locked_hash.h"
#include "_thread.h"
#include "comm_trace_line.h"
#include "comm_l_set.h"
#include "comm_l_set_four.h"
#include "comm_l_prod.h"

#include <unordered_map>
#include <mutex>

#define COMM_MAXTHREADS 128
//
//class FlushThread : public Runnable {
//private:
//    _Thread *m_thread;
//    void run();
//
//public:
//    FlushThread();
//    ~FlushThread();
//    void spawn();
//
//};
struct TMemAccessCtr {
    thread_id_t tid;
    UInt32 nReads;
    UInt32 nWrites;
    UInt64 szReads;
    UInt64 szWrites;
};

struct TComm {
    UInt32 count;
    UInt64 size;
};

class CommTracer {

    /*
    class TIDlist {
    public:
        thread_id_t first;
        thread_id_t second;

        TIDlist() {
            first = 0;
            second = 0;
        }
        
        void insert(thread_id_t tid) {
            second = first;
            first = tid;
        }
    };
     */

    class TPayload {
    public:
        UInt32 count;
        UInt64 size;

        TPayload() {
            count = 0;
            size = 0;
        }

        TPayload(UInt32 c, UInt64 s) {
            count = c;
            size = s;
        }

        void incr_num() {
            count++;
        }

        void incr_size(UInt64 sz) {
            size += sz;
        }

        void update(UInt64 sz) {
            incr_num();
            incr_size(sz);
        }
    };

    class CommWindow {
    private:
        //Lock lock;
        mutable std::mutex _cwmtx;
        thread_id_t first;
        thread_id_t second;
    public:

        CommWindow() {
            first = 0;
            second = 0;
        }

        void insert(thread_id_t tid) {
            //lock.acquire();
            _cwmtx.lock();
            thread_id_t a = first;
            first = tid;
            second = a;
            _cwmtx.unlock();
            //lock.release();
        }

        thread_id_t getFirst() {
            return first;
        }

        thread_id_t getSecond() {
            return getSecond();
        }
    };

    class CommLine {
    private:
        mutable std::mutex _mtx;
        //mutable std::mutex _smtx;
        Lock cl_lock;
        //, ins_lock;
        //std::unordered_map<UInt64, thread_id_t> _firstMap;
        //std::unordered_map<UInt64, thread_id_t> _secondMap;
        //std::unordered_map<UInt64, CommWindow> lines;
        std::unordered_map<UInt64, UInt64> firsts;
        std::unordered_map<UInt64, UInt64> seconds;

    public:

        void insert(UInt64 line, thread_id_t tid) {
            /*
            _mtx.lock();
            thread_id_t a = _firstMap[line];
            _firstMap[line] = tid;
            _mtx.unlock();
            
            std::lock_guard<std::mutex> s(_smtx);
            _secondMap[line] = a;
             */
            //lines[line].insert(tid);
            cl_lock.acquire();
            thread_id_t tmp = firsts[line];
            firsts[line] = tid;
            seconds[line] = tmp;
            cl_lock.release();
        }

        thread_id_t first(UInt64 line) {
            // std::lock_guard<std::mutex> l(_mtx);
            //return _firstMap[line];
            /*
            thread_id_t f;
            if (lines.count(line) == 0) {
                //cl_lock.acquire();
                // Will write the first element
                _mtx.lock();
                f = lines[line].getFirst();
                _mtx.unlock();
                //cl_lock.release();
            }
            else {
                //cl_lock.acquire();
                //thread_id_t f = lines[line].getFirst();
                //cl_lock.release();
                f = lines[line].getFirst();
            }
            return f;
             */
            thread_id_t f;
            if (firsts.count(line) == 0) {
                cl_lock.acquire();
                f = firsts[line] = 0;
                //if (seconds.count(line) == 0)
                seconds[line] = 0;
                cl_lock.release();
                //return 0;
            } else {
                f = firsts[line];
            }
            return f;
        }

        thread_id_t second(UInt64 line) {
            // std::lock_guard<std::mutex> s(_smtx);
            // return _secondMap[line];
            //return lines[line].getSecond();
            return seconds[line];
        }

        void initZeros(int max) {
            /*
            for (int i = 0; i < max; i++) {
                _firstMap[i] = 0;
                _secondMap[i] = 0;
            }
             */
        }
    };

public:
    CommTracer();
    ~CommTracer();

    void inc_comm(thread_id_t a, thread_id_t b, UInt32 dsize, IntPtr a_addr, IntPtr b_addr,
            bool a_w_op, bool b_w_op);
    void add_comm_event(thread_id_t a, thread_id_t b, UInt64 tsc, UInt32 dsize);
    void trace_comm(IntPtr addr, thread_id_t tid, UInt64 ms, UInt32 dsize, bool w_op);
    void trace_comm_spat_tempo(IntPtr line, thread_id_t tid, UInt64 ms, UInt32 dsize, IntPtr addr, bool w_op);
    void trace_comm_spat(IntPtr line, thread_id_t tid, UInt32 dsize, IntPtr addr, bool w_op);
    void inc_comm_f(thread_id_t a, thread_id_t b, UInt32 dsize, IntPtr a_addr, IntPtr b_addr);
    void trace_comm_spat_f(IntPtr line, thread_id_t tid, UInt32 dsize, IntPtr addr);
    void trace_comm_spat_tempo_f(IntPtr line, thread_id_t tid, UInt64 ms, UInt32 dsize, IntPtr addr);
    void add_comm_event_f(thread_id_t a, thread_id_t b, UInt64 tsc, UInt32 dsize, IntPtr a_addr, IntPtr b_addr);
    void rec_t_latency(thread_id_t tid, UInt64 lat, UInt32 data_len, UInt64 ts);
    void rec_core_dram_lat(core_id_t core_id, UInt64 lat, UInt32 data_len, UInt64 ts);
    //void rec_t_qdelay(thread_id_t tid, UInt64 q_delay, UInt64 ts);
    //void rec_t_lat_delay(thread_id_t tid, UInt64 lat, UInt64 delay, UInt32 data_len, UInt64 ts);
    void print_comm();
    void print_comm_events();
    void print_mem();
    //void print_t_lats();
    void flush_thread_events(thread_id_t t1, thread_id_t t2);
    void flush_tempo();
    void run_flush_thread();
    void setPaused(bool paused);
    void fini();

    // Prod/cons methods
    void inc_comm_prod(thread_id_t a, thread_id_t b, UInt32 dsize);
    void trace_comm_spat_prod(IntPtr line, thread_id_t tid, UInt32 dsize, IntPtr addr, bool w_op);
    void incNumThreads(thread_id_t tid);
    void updateThreadMemWrites(thread_id_t tid, UInt32 dsize);
    void updateThreadMemReads(thread_id_t tid, UInt32 dsize);

    //void simThreadStartCallback();
    //void simThreadExitCallback();

private:
    bool m_trace_comm;
    bool m_trace_comm_four;
    bool m_trace_t_lats;
    UInt64 m_block_size;
    int m_max_block = 100 * 1000 * 1000;
    // Time interval (in ns) for communication granularity
    float m_time_res = 1;
    bool m_trace_comm_events;
    bool m_trace_mem;
    bool m_paused;
    CommLine m_commLine;

    // filenames
    String fname_comm_count;
    String fname_comm_sz;
    String fname_comm_events;
    String fname_thread_lats;
    String fname_mem_access;

    // Dedicated thread for flushing
    UInt64 m_flushInterval;
    //bool m_flushStopped = false;
    //_Thread *m_flushThread;

    //ThreadLines m_threadLines;
    CommLSet m_commLSet;
    // Comm line of four window size
    CommLSetFour m_commLSetFour;
    // Comm line of prod/cons
    bool m_prodcons;
    CommLProdConsSet m_commLPS;

    thread_id_t m_num_threads;
    thread_id_t m_num_threads_res;
    // Skip every n access to speed up tracing
    int m_num_skips;
    /*
    struct TIDlist {
        thread_id_t first;
        thread_id_t second;
    };
    // mapping of cache line to a list of TIDs that previously accessed it
    std::unordered_map<UInt64, TIDlist> commmap;
     */
    // communication matrix
    //UInt64 comm_matrix[COMM_MAXTHREADS + 1][COMM_MAXTHREADS + 1];
    //UInt64 comm_sz_matrix[COMM_MAXTHREADS + 1][COMM_MAXTHREADS + 1];

    // Thread and time based payloads
    // Time in UINT64 and kilo cycle precision
    //std::unordered_map<UInt64, TPayload> timeEventMap[COMM_MAXTHREADS + 1][COMM_MAXTHREADS + 1];
    //std::unordered_map<UInt64, UInt32> nEvents[COMM_MAXTHREADS + 1][COMM_MAXTHREADS + 1];
    //std::unordered_map<UInt64, UInt64> szEvents[COMM_MAXTHREADS + 1][COMM_MAXTHREADS + 1];

    // Temporary map for accumulating communications of a pair (t1, t2), t1 < t2
    //std::unordered_map<UInt64, TPayload> accums[COMM_MAXTHREADS + 1][COMM_MAXTHREADS + 1];

    // Record the maximum and total latencies for each thread
    // Use basic array to ensure thread-safety in same thread access
    UInt64 threadsLatMax[COMM_MAXTHREADS + 1];
    UInt32 threadsLatMaxDataLen[COMM_MAXTHREADS + 1];
    UInt64 threadsLatMaxTime[COMM_MAXTHREADS + 1];
    UInt64 threadsLatSum[COMM_MAXTHREADS + 1];
    UInt64 threadsLatCount[COMM_MAXTHREADS + 1];
    //UInt64 threadsDelayTot[COMM_MAXTHREADS + 1];
    //UInt64 threadsDelayCount[COMM_MAXTHREADS + 1];
    //UInt64 threadsDelayMax[COMM_MAXTHREADS + 1];
    //UInt64 threadsDelayMaxTime[COMM_MAXTHREADS + 1];

    //std::unordered_map<thread_id_t, UInt64> threadsLatMax;
    //std::unordered_map<thread_id_t, UInt64> threadsLatMaxTime;
    //std::unordered_map<thread_id_t, UInt64> threadsLatSum;
    //Lock threadLocks[COMM_MAXTHREADS+1];
    bool isFlushing[COMM_MAXTHREADS+1];
    //UInt64 threadsMemAccesses[COMM_MAXTHREADS+1];
   // UInt64 threadsMemSizes[COMM_MAXTHREADS+1];

    // Spatial communications
    std::vector<TMemAccessCtr> threadMemAccesses;
    std::vector<std::vector<UInt64>> v_comm_matrix;
    std::vector<std::vector<UInt64>> v_comm_sz_matrix;
    // Temporal communications
    //std::unordered_map<UInt64, UInt32> nEvents[COMM_MAXTHREADS + 1][COMM_MAXTHREADS + 1];
    std::vector<std::vector<std::unordered_map<UInt64, UInt32>>> v_nEvents;
    std::vector<std::vector<std::unordered_map<UInt64, UInt32>>> v_szEvents;
    
    // Skip counters
    std::vector<int> threadSkipCounters;
    
    //std::unordered_map<UInt64, UInt32> nEvents[COMM_MAXTHREADS + 1][COMM_MAXTHREADS + 1];

    void init();
    void update_t_latency(thread_id_t tid, UInt64 lat, UInt32 data_len, UInt64 ts);
    //void run();
};

#endif // COMM_TRACER
