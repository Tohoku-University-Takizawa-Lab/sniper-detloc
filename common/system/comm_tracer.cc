#include "comm_tracer.h"

#include "log.h"
#include "config.h"
#include "simulator.h"
#include "core_manager.h"
#include "config.hpp"
#include "memory_manager.h"
#include "thread_manager.h"
#include "thread.h"
//#include "standalone_pin3.0/libsift/fixed_types.h"

#include <fstream>
#include <math.h>
#include <unistd.h>

CommTracer::CommTracer()
: m_num_threads(0) {
    // Check configuration parameter for comm detection
    //m_num_threads = Sim()->getThreadManager()->getNumThreads();
    m_num_threads = Sim()->getConfig()->getApplicationCores();
    fname_comm_count = Sim()->getConfig()->formatOutputFileName("sim.comm_count");
    fname_comm_sz = Sim()->getConfig()->formatOutputFileName("sim.comm_size");
    fname_comm_events = Sim()->getConfig()->formatOutputFileName("sim.comm_events");
    fname_thread_lats = Sim()->getConfig()->formatOutputFileName("sim.thread_lats");
    fname_mem_access = Sim()->getConfig()->formatOutputFileName("sim.mem_access");

    m_trace_mem = Sim()->getCfg()->getBoolDefault("comm_tracer/mem_access_enable", false);
    m_trace_comm = Sim()->getCfg()->getBoolDefault("comm_tracer/enable", false);
    if (m_trace_comm) {
        m_block_size = Sim()->getCfg()->getInt("comm_tracer/comm_size");
        if (Sim()->getCfg()->hasKey("comm_tracer/max_block")) {
            m_max_block = Sim()->getCfg()->getInt("comm_tracer/max_block");
        }
        m_prodcons = Sim()->getCfg()->getBoolDefault("comm_tracer/producer_consumer_mode", false);
        printf("[DeTLoc] Communication tracing is enabled, block_size=%lu (%d bytes), prod/cons: %d\n",
                m_block_size, 1 << m_block_size, m_prodcons);
        
        // Enable timestamp traces
        m_trace_comm_events = Sim()->getCfg()->getBoolDefault("comm_tracer/trace_time", false);
        if (m_trace_comm_events) {
            if (Sim()->getCfg()->hasKey("comm_tracer/time_res")) {
                m_time_res = Sim()->getCfg()->getFloat("comm_tracer/time_res");
                if (m_time_res < 1) 
                    m_time_res = 1;
                printf("[DeTLoc] comm_tracer.time_res must be > 0, using default (1 ns).\n");
            }
            printf("Communication time will be traced (resolution = %.2f ns)\n", m_time_res);
            m_flushInterval = Sim()->getCfg()->getFloat("comm_tracer/flush_interval");
            printf("Comm. events will be printed every interval = %lu * %.2f ns)\n",
                    m_flushInterval, m_time_res);
            //if (m_flushInterval > 0)
            //    run_flush_thread();
        }
        m_trace_comm_four = Sim()->getCfg()->getBoolDefault("comm_tracer/four_win_mode", false);
        if (m_trace_comm_four)
            printf("[DeTLoc] Using four window size for the tracing\n");
        //init();
        // Pause tracing on startup, the simulator or ROI will resume it as necessary
        setPaused(true);
    }
    m_trace_t_lats = Sim()->getCfg()->getBoolDefault("comm_tracer/latency_trace_enable", false);
    if (m_trace_t_lats)
        printf("[DeTLoc] Threads' latency tracing is enabled\n");
}

void
CommTracer::init() {
    /*
    for (int i = 0; i < m_max_block; i++) {
        commmap[i].first = 0;
        commmap[i].second = 0;
    }
     */
    //m_commLine.initZeros(m_max_block);
}

void
CommTracer::setPaused(bool paused) {
    m_paused = paused;
    std::cout << "[DeTLoc] Update paused state: " << m_paused << std::endl;
}

CommTracer::~CommTracer() {
}

void
CommTracer::fini() {
    if (!m_paused)
        m_paused = true;
    if (m_trace_comm) {
        print_comm();
        if (m_trace_comm_events) {
            //m_flushStopped = true;
            print_comm_events();
        }
    }
    if (m_trace_t_lats)
        print_t_lats();
    if (m_trace_mem)
        print_mem();
}

void
CommTracer::inc_comm(thread_id_t a, thread_id_t b, UInt32 dsize,
        IntPtr a_addr, IntPtr b_addr, bool a_w_op, bool b_w_op) {
    // Also check if two threads precisely access the same address
    thread_id_t r_b = b - 1;
    if (a_addr == b_addr && a != r_b && b_w_op == true && a_w_op == false) {
        comm_matrix[a][r_b] += 1;
        comm_sz_matrix[a][r_b] += dsize;
        //if (b_w_op == true) {
        //std::cout << "** Pair(" << a << "," << b << "), a_addr=" << a_addr 
        //          << ", b_addr=" << b_addr << ", a_op=" << a_w_op << ", b_op=" << b_w_op << std::endl;
        //}
    }
}

void
CommTracer::inc_comm_prod(thread_id_t a, thread_id_t b, UInt32 dsize) {
    thread_id_t r_b = b - 1;
    if (a != r_b) {
        comm_matrix[a][r_b] += 1;
        comm_sz_matrix[a][r_b] += dsize;
    }
}


void
CommTracer::inc_comm_f(thread_id_t a, thread_id_t b, UInt32 dsize,
        IntPtr a_addr, IntPtr b_addr) {
    thread_id_t r_b = b - 1;
    if (a_addr == b_addr && a != r_b) {
        comm_matrix[a][r_b] += 1;
        comm_sz_matrix[a][r_b] += dsize;
    }
}

void
CommTracer::print_comm() {
    //static long n = 0;
    //int num_threads = Sim()->getThreadManager()->getNumThreads();
    //int num_threads = Sim()->getConfig()->getApplicationCores();
    std::ofstream f, f_sz;
    //char fname[255];
    //char fname_sz[255];

    //sprintf(fname, "%s.full.%d.comm.csv", img_name.c_str(), cs);
    //sprintf(fname_l, "%s.full.%d.comm_size.csv", img_name.c_str(), cs);

    //String fname = "sim.comm-count";
    //String fname_sz = "sim.comm-size";

    /*
    int real_tid[MAXTHREADS + 1];
    int i = 0, a, b;

    for (auto it : pidmap)
        real_tid[it.second] = i++;
     */
    //cout << fname << endl;
    //cout << fname_l << endl;
    std::cout << "[CommTracer] Saving communication matrix to: " << fname_comm_count 
                << ',' << fname_comm_sz << std::endl;

    //f.open(Sim()->getConfig()->formatOutputFileName(fname).c_str());
    //f_sz.open(Sim()->getConfig()->formatOutputFileName(fname_sz).c_str());
    f.open(fname_comm_count.c_str());
    f_sz.open(fname_comm_sz.c_str());

    UInt32 i = m_num_threads;
    while (i > 0) {
        i--; 
        for (UInt32 j = 0; j < m_num_threads; j++) {
            //b = real_tid[j];
            //f << comm_matrix[a][b] + comm_matrix[b][a];
            //fl << comm_size_matrix[a][b] + comm_size_matrix[b][a];
            f << comm_matrix[i][j] + comm_matrix[j][i];
            f_sz << comm_sz_matrix[i][j] + comm_sz_matrix[j][i];
            if (j != m_num_threads - 1) {
                f << ",";
                f_sz << ",";
            }
        }
        f << std::endl;
        f_sz << std::endl;
    }
    /*
    for (UInt32 i = m_num_threads - 1; i == 0; i--) {
        //a = real_tid[i];
        for (UInt32 j = 0; j < m_num_threads; j++) {
            //b = real_tid[j];
            //f << comm_matrix[a][b] + comm_matrix[b][a];
            //fl << comm_size_matrix[a][b] + comm_size_matrix[b][a];
            f << comm_matrix[i][j] + comm_matrix[j][i];
            f_sz << comm_sz_matrix[i][j] + comm_sz_matrix[j][i];
            if (j != m_num_threads - 1) {
                f << ",";
                f_sz << ",";
            }
        }
        f << std::endl;
        f_sz << std::endl;
    }
    */
    f << std::endl;
    f_sz << std::endl;
    f.close();
    f_sz.close();
    std::cout << "[CommTracer] Finished saving comm. matrix." << std::endl;
}

void
CommTracer::add_comm_event(thread_id_t a, thread_id_t b, UInt64 tsc, UInt32 dsize) {
    if (a != b - 1) {
        //std::string tsc = std::to_string(ms);
        //TPayload p = timeEventMap[a][b][tsc];
        //p.incr_num();
        //p.size = p.size + dsize;
        //p.count += 1;
        // Time res here, tsc is always positive
        tsc = (tsc + (m_time_res / 2)) / m_time_res;

        nEvents[a][b - 1][tsc] = nEvents[a][b - 1][tsc] + 1;
        szEvents[a][b - 1][tsc] = szEvents[a][b - 1][tsc] + dsize;
        //auto updatefn = [](TPayload &tp) { tp.update(dsize); };
        //int curr_count = pairEvents[a][b].find(tsc);
        //pairEvents[a][b].upsert(tsc, updatefn, new TPayload(1, dsize));
        //timeEventMap[a][b][tsc].size = timeEventMap[a][b][tsc].size + dsize;
        //timeEventMap[a][b][tsc].count = timeEventMap[a][b][tsc].count + 1;
        if (m_flushInterval > 0 && nEvents[a][b - 1].size() >= m_flushInterval) {
            flush_thread_events(a, b - 1);
        }
    }
}

void
CommTracer::add_comm_event_f(thread_id_t a, thread_id_t b, UInt64 tsc, UInt32 dsize,
        IntPtr a_addr, IntPtr b_addr) {
    thread_id_t r_b = b -1;
    if (a_addr == b_addr && a != r_b) {
        // Time res here, tsc is always positive
        //if (m_time_res > 1)
        tsc = (tsc + (m_time_res / 2)) / m_time_res;

        nEvents[a][r_b][tsc] = nEvents[a][r_b][tsc] + 1;
        szEvents[a][r_b][tsc] = szEvents[a][r_b][tsc] + dsize;
       
        if (m_flushInterval > 0 && nEvents[a][r_b].size() >= m_flushInterval) {
            flush_thread_events(a, r_b);
        }
    }
}

void
CommTracer::trace_comm(IntPtr addr, thread_id_t tid, UInt64 ns, UInt32 dsize, bool w_op) {
    if (m_trace_mem) {
        threadsMemAccesses[tid] += 1;
        threadsMemSizes[tid] += dsize;
    }

    if (m_trace_comm && !m_paused) {
        IntPtr line = addr >> m_block_size;
        //std::cout << "** thread-" << tid << ", addr=" << addr << ", line=" << line << std::endl;
        if (m_trace_comm_events == false && m_prodcons == true)
            trace_comm_spat_prod(line, tid, dsize, addr, w_op);
        else if (!m_trace_comm_events && !m_trace_comm_four)
            trace_comm_spat(line, tid, dsize, addr, w_op);
        else if (m_trace_comm_events && !m_trace_comm_four)
            trace_comm_spat_tempo(line, tid, ns, dsize, addr, w_op);
        else if (!m_trace_comm_events && m_trace_comm_four)
            trace_comm_spat_f(line, tid, dsize, addr);
        else
            trace_comm_spat_tempo_f(line, tid, ns, dsize, addr);
    }
}

void
CommTracer::trace_comm_spat_tempo(IntPtr line, thread_id_t tid, UInt64 curr_ts,
        UInt32 dsize, IntPtr addr, bool w_op) {
    //if (num_threads < 2)
    //    return;

    //UInt64 line = addr >> m_block_size;
    //UInt64 curr_ts = lround(ns / m_time_res);
    //printf("-- %ul\n", curr_ts);

    //thread_id_t a = commmap[line].first;
    //thread_id_t b = commmap[line].second;

    //thread_id_t a = commmap_f[line];
    //thread_id_t b = commmap_s[line];
    //printf("(a,b)=(%d,%d)\n", a, b);
    //ThreadLine *threadLine = m_threadLines.getThreadLine(tid, line);
    CommL comm_l = m_commLSet.getLine(line);

    //thread_id_t a = m_commLine.first(line);
    //thread_id_t a = threadLine->getFirst();
    thread_id_t a = comm_l.getFirst();

    // We put the tid in line in the format tid+1, so dont forget to tid-1
    // when we store to events
    if ((tid + 1) == a)
        return;

    //thread_id_t b = m_commLine.second(line);
    //thread_id_t b = threadLine->getSecond();
    thread_id_t b = comm_l.getSecond();

    if (a == 0 && b == 0) {
        // sh = 0;
        // no one accessed line before, store accessing thread in pos 0
        //commmap[line].first = tid + 1;
        //commmap_f[line] = tid;
        //m_commLine.insert(line, tid + 1);
        //threadLine->update(tid+1);
        m_commLSet.updateLine(line, tid + 1, addr, w_op);
    } else if (a != 0 && b != 0) {
        // sh = 2;
        // two previous accesses
        inc_comm(tid, a, dsize, addr, comm_l.getFirst_addr(), w_op, comm_l.getFirst_w_op());
        inc_comm(tid, b, dsize, addr, comm_l.getSecond_addr(), w_op, comm_l.getSecond_w_op());

        add_comm_event(tid, a, curr_ts, dsize);
        add_comm_event(tid, b, curr_ts, dsize);
        //commmap[line].first = tid + 1;
        //commmap[line].second = a;
        //commmap_f[line] = tid;
        //commmap_s[line] = a;
        //m_commLine.insert(line, tid + 1);
        //threadLine->update(tid+1);
        m_commLSet.updateLine(line, tid + 1, addr, w_op);
    } else {
        // sh = 1;
        // one previous access => needs to be in pos 0
        inc_comm(tid, a, dsize, addr, comm_l.getFirst_addr(), w_op, comm_l.getFirst_w_op());

        add_comm_event(tid, a, curr_ts, dsize);
        //commmap[line].first = tid + 1;
        //commmap[line].second = a;
        //commmap_f[line] = tid;
        //commmap_s[line] = a;
        //m_commLine.insert(line, tid + 1);
        //threadLine->update(tid+1);
        m_commLSet.updateLine(line, tid + 1, addr, w_op);
    }
}

void
CommTracer::trace_comm_spat(IntPtr line, thread_id_t tid, UInt32 dsize, IntPtr addr,
        bool w_op) {
    //if (num_threads < 2)
    //    return;

    //thread_id_t a = m_commLine.first(line);
    // Get the element, it should pass-by-reference according to STL doc
    //ThreadLine *threadLine = m_threadLines.getLine(line);
    //thread_id_t exec_tid = Sim()->getThreadManager()->getCurrentThread()->getId();
    //printf("Curr tid=%d, sim_tid=%d\n", tid, sim_tid);

    //ThreadLine *threadLine = m_threadLines.getThreadLine(exec_tid, line);
    CommL comm_l = m_commLSet.getLine(line);
    //CommL comm_l = m_commLSet.getLineMod(line);
    //CommLWin *comm_win = m_commLSet.getLineWindow(line);

    //printf("ThreadLine::getLine: %lu\n", line);

    //thread_id_t a = threadLine->getFirst();
    thread_id_t a = comm_l.getFirst();
    //thread_id_t a = comm_win->first;

    if ((tid + 1) == a)
        return;

    //thread_id_t b = m_commLine.second(line);
    //thread_id_t b = threadLine->getSecond();
    thread_id_t b = comm_l.getSecond();
    //thread_id_t b = comm_win->second;

    //if (a == 0 && b == 0) {
        // sh = 0;
        // no one accessed line before, store accessing thread in pos 0
        //commmap[line].first = tid + 1;
        //commmap_f[line] = tid;
        //m_commLine.insert(line, tid + 1);
        //threadLine->update(tid + 1);
        //comm_l.update(tid + 1);
        //comm_win->update(tid + 1);
        //if (w_op == true)
        //m_commLSet.updateLine(line, tid + 1, addr, w_op);
    //}
    if (a != 0 && b != 0) {
    //else if (a != 0 && b != 0) {
        // sh = 2;
        // two previous accesses
        inc_comm(tid, a, dsize, addr, comm_l.getFirst_addr(), w_op, comm_l.getFirst_w_op());
        inc_comm(tid, b, dsize, addr, comm_l.getSecond_addr(), w_op, comm_l.getSecond_w_op());
        //m_commLine.insert(line, tid + 1);
        //threadLine->update(tid + 1);
        //comm_l.update(tid + 1);
        //comm_win->update(tid + 1);
        //if (w_op == true)
        //    m_commLSet.updateLine(line, tid + 1, addr, w_op);
    }
    else if (a != 0) {
    //else {
        // sh = 1;
        // one previous access => needs to be in pos 0
        inc_comm(tid, a, dsize, addr, comm_l.getFirst_addr(), w_op, comm_l.getFirst_w_op());
        //m_commLine.insert(line, tid + 1);
        //threadLine->update(tid + 1);
        //comm_l.update(tid + 1);
        //comm_win->update(tid + 1);
        //if (w_op == true)
        //    m_commLSet.updateLine(line, tid + 1, addr, w_op);
    }
    
    if (w_op == true)
        m_commLSet.updateLine(line, tid + 1, addr, w_op);
}

void CommTracer::trace_comm_spat_prod(IntPtr line, thread_id_t tid, UInt32 dsize,
        IntPtr addr, bool w_op) {
    
    if (w_op == true) {
        UInt32 n_line = 1 + (dsize >> m_block_size);
        //printf("[L-%ld] WRITE: %d, n_line: %d\n", line, dsize, n_line);
        m_commLPS.updateCreateLineBatch(line, n_line, tid+1, addr);
    }
    else {
        CommLProdCons clps = m_commLPS.getLineLazy(line);
        if (clps.isEmpty() == false) {
            if (clps.getSecond_addr() != 0 && clps.getSecond_addr() <= addr) {
                inc_comm_prod(tid, clps.getSecond(), dsize);
                //printf("[L-%ld] READ: %d, tid: %d, b: %d\n", line, dsize, tid, clps.getSecond());
            }
            else if (clps.getFirst_addr() <= addr) {
                inc_comm_prod(tid, clps.getFirst(), dsize);
                //printf("[L-%ld] READ: %d, tid: %d, a: %d\n", line, dsize, tid, clps.getFirst());
            }
        }
    }
}

void
CommTracer::trace_comm_spat_f(IntPtr line, thread_id_t tid, UInt32 dsize,
        IntPtr addr) {

    CommLFour comm_l = m_commLSetFour.getLine(line);
    thread_id_t a = comm_l.getFirst();
    thread_id_t b = comm_l.getSecond();

    if (a == 0 && b == 0) {
        // no one accessed line before, store accessing thread in pos 0
        m_commLSetFour.updateLine(line, tid + 1, addr);
    } else {
        thread_id_t c = comm_l.getThird();
        thread_id_t d = comm_l.getFourth();

        if (a != 0 && b != 0 && c != 0 && d != 0) {
            inc_comm_f(tid, a, dsize, addr, comm_l.getFirst_addr());
            inc_comm_f(tid, b, dsize, addr, comm_l.getSecond_addr());
            inc_comm_f(tid, c, dsize, addr, comm_l.getThird_addr());
            inc_comm_f(tid, d, dsize, addr, comm_l.getFourth_addr());
            m_commLSetFour.updateLine(line, tid + 1, addr);
        } else if (a != 0 && b != 0 && c != 0) {
            inc_comm_f(tid, a, dsize, addr, comm_l.getFirst_addr());
            inc_comm_f(tid, b, dsize, addr, comm_l.getSecond_addr());
            inc_comm_f(tid, c, dsize, addr, comm_l.getThird_addr());
            m_commLSetFour.updateLine(line, tid + 1, addr);
        } else if (a != 0 && b != 0) {
            inc_comm_f(tid, a, dsize, addr, comm_l.getFirst_addr());
            inc_comm_f(tid, b, dsize, addr, comm_l.getSecond_addr());
            m_commLSetFour.updateLine(line, tid + 1, addr);
        } else {
            inc_comm_f(tid, a, dsize, addr, comm_l.getFirst_addr());
            m_commLSetFour.updateLine(line, tid + 1, addr);
        }
    }
}

void
CommTracer::trace_comm_spat_tempo_f(IntPtr line, thread_id_t tid, UInt64 curr_ts,
        UInt32 dsize, IntPtr addr) {

    CommLFour comm_l = m_commLSetFour.getLine(line);
    thread_id_t a = comm_l.getFirst();
    thread_id_t b = comm_l.getSecond();

    if (a == 0 && b == 0) {
        // no one accessed line before, store accessing thread in pos 0
        m_commLSetFour.updateLine(line, tid + 1, addr);
    } else {
        thread_id_t c = comm_l.getThird();
        thread_id_t d = comm_l.getFourth();

        if (a != 0 && b != 0 && c != 0 && d != 0) {
            inc_comm_f(tid, a, dsize, addr, comm_l.getFirst_addr());
            inc_comm_f(tid, b, dsize, addr, comm_l.getSecond_addr());
            inc_comm_f(tid, c, dsize, addr, comm_l.getThird_addr());
            inc_comm_f(tid, d, dsize, addr, comm_l.getFourth_addr());
            
            add_comm_event_f(tid, a, curr_ts, dsize, addr, comm_l.getFirst_addr());
            add_comm_event_f(tid, b, curr_ts, dsize, addr, comm_l.getSecond_addr());
            add_comm_event_f(tid, c, curr_ts, dsize, addr, comm_l.getThird_addr());
            add_comm_event_f(tid, d, curr_ts, dsize, addr, comm_l.getFourth_addr());
            m_commLSetFour.updateLine(line, tid + 1, addr);
        } else if (a != 0 && b != 0 && c != 0) {
            inc_comm_f(tid, a, dsize, addr, comm_l.getFirst_addr());
            inc_comm_f(tid, b, dsize, addr, comm_l.getSecond_addr());
            inc_comm_f(tid, c, dsize, addr, comm_l.getThird_addr());
            
            add_comm_event_f(tid, a, curr_ts, dsize, addr, comm_l.getFirst_addr());
            add_comm_event_f(tid, b, curr_ts, dsize, addr, comm_l.getSecond_addr());
            add_comm_event_f(tid, c, curr_ts, dsize, addr, comm_l.getThird_addr());
            m_commLSetFour.updateLine(line, tid + 1, addr);
        } else if (a != 0 && b != 0) {
            inc_comm_f(tid, a, dsize, addr, comm_l.getFirst_addr());
            inc_comm_f(tid, b, dsize, addr, comm_l.getSecond_addr());
            
            add_comm_event_f(tid, a, curr_ts, dsize, addr, comm_l.getFirst_addr());
            add_comm_event_f(tid, b, curr_ts, dsize, addr, comm_l.getSecond_addr());
            m_commLSetFour.updateLine(line, tid + 1, addr);
        } else {
            inc_comm_f(tid, a, dsize, addr, comm_l.getFirst_addr());
            
            add_comm_event_f(tid, a, curr_ts, dsize, addr, comm_l.getFirst_addr());
            m_commLSetFour.updateLine(line, tid + 1, addr);
        }
    }    
}

void
CommTracer::print_comm_events() {
    std::ofstream f_accu;
    //String fname_accu = "sim.comm-events";
    //int num_threads = Sim()->getConfig()->getApplicationCores();
    //int i, j, l, r;
    UInt32 i, j, l, r;

    std::cout << "Print communication events: " << fname_comm_events << std::endl;
    //std::unordered_map<UInt64, TPayload> accums[COMM_MAXTHREADS + 1][COMM_MAXTHREADS + 1];

    i = m_num_threads;
    while (i > 0) {
        i--;
    //for (i = m_num_threads - 1; i == 0; i--) {
        for (j = 0; j < m_num_threads; j++) {

            //for (auto it : timeEventMap[i][j]) {
            while (isFlushing[i])
                sleep(1000);

            for (auto it : nEvents[i][j]) {
                // Accumulate the pairs with just a different order
                // Pair of (T1,T2) is considered same with (T2,T1)
                if (i < j) {
                    l = i;
                    r = j;
                } else {
                    l = j;
                    r = i;
                }
                /*
                accums[l][r][it.first].size = accums[l][r][it.first].size + 
                        it.second.size;
                accums[l][r][it.first].count = accums[l][r][it.first].count + 
                        it.second.count;
                 */
                accums[l][r][it.first].count = accums[l][r][it.first].count +
                        it.second;
                accums[l][r][it.first].size = accums[l][r][it.first].size +
                        szEvents[i][j][it.first];
            }
        }
    }

    //f_accu.open(Sim()->getConfig()->formatOutputFileName(fname_accu).c_str());
    f_accu.open(fname_comm_events.c_str());
    if (f_accu.is_open()) {
        i = m_num_threads;
        while (i > 0) {
            i--;
        //for (i = m_num_threads - 1; i == 0; i--) {
            for (j = 0; j < m_num_threads; j++) {
                for (auto it : accums[i][j]) {
                    //if (it.first >= 0 && it.second.count > 0) {
                    f_accu << i << ',' << j << ',' << it.first << ','
                            << it.second.count << "," << it.second.size << '\n';
                    //}
                }
            }
        }
        f_accu << std::endl;
        f_accu.close();
    } else {
        std::cerr << "Failed opening file: " << strerror(errno) << std::endl;
    }
}

void
CommTracer::rec_t_latency(thread_id_t tid, UInt64 lat, UInt32 data_len, UInt64 ts) {
    if (m_trace_t_lats) {
       update_t_latency(tid, lat, data_len, ts);
    }
}

void
CommTracer::update_t_latency(thread_id_t tid, UInt64 lat, UInt32 data_len, UInt64 ts) {
    //UInt64 curr_max = threadsLatMax[tid];
    //UInt64 tp = lround(lat/data_len);
    if (lat > threadsLatMax[tid]) {
        threadsLatMax[tid] = lat;
        threadsLatMaxTime[tid] = ts;
        threadsLatMaxDataLen[tid] = data_len;
    }
    threadsLatSum[tid] += lat;
    threadsLatCount[tid] += 1;
}

void
CommTracer::rec_core_dram_lat(core_id_t core_id, UInt64 lat, UInt32 data_len, UInt64 ts) {
    if (m_trace_t_lats) {
        Core *c = Sim()->getCoreManager()->getCoreFromID(core_id);
        thread_id_t tid = c->getThread()->getId();
        update_t_latency(tid, lat, data_len, ts);
    }
}

void
CommTracer::rec_t_qdelay(thread_id_t tid, UInt64 q_delay, UInt64 ts) {
    if (m_trace_t_lats) {
        if (q_delay > threadsDelayMax[tid]) {
            threadsDelayMax[tid] = q_delay;
            threadsDelayMaxTime[tid] = ts;
        }
        threadsDelayCount[tid] += 1;
        threadsDelayTot[tid] += q_delay;
    }
}

void
CommTracer::rec_t_lat_delay(thread_id_t tid, UInt64 lat, UInt64 q_delay, UInt32 data_len, UInt64 ts) {
    if (m_trace_t_lats) {
        if (lat > threadsLatMax[tid]) {
            threadsLatMax[tid] = lat;
            threadsLatMaxTime[tid] = ts;
            threadsLatMaxDataLen[tid] = data_len;
        }
        threadsLatSum[tid] += lat;
        threadsLatCount[tid] += 1;

        if (q_delay > threadsDelayMax[tid]) {
            threadsDelayMax[tid] = q_delay;
            threadsDelayMaxTime[tid] = ts;
        }
        threadsDelayCount[tid] += 1;
        threadsDelayTot[tid] += q_delay;
    }
}

void
CommTracer::print_t_lats() {
    std::ofstream f;
    //String fname = "sim.thread-lats";
    //int num_threads = Sim()->getConfig()->getApplicationCores();

    std::cout << "Print thread's mem access latencies: " << fname_thread_lats << std::endl;

    //f.open(Sim()->getConfig()->formatOutputFileName(fname).c_str());
    f.open(fname_thread_lats.c_str());
    if (f.is_open()) {
        //printf("Print latencies of %d threads\n", num_threads);
        f << "thread,max_lat,max_lat_t,max_lat_data_len,total_lat,n_access,avg_lat,"
                << "t_delay_max,t_delay_max_t,t_delay_tot,t_delay_n\n";
        for (UInt32 i = 0; i < m_num_threads; i++) {
            if (threadsLatMax[i] > 0) {
                f << i << ',' << threadsLatMax[i] << ',' << threadsLatMaxTime[i]
                        << ',' << threadsLatMaxDataLen[i]
                        << ',' << threadsLatSum[i] << ',' << threadsLatCount[i]
                        << ',' << round(threadsLatSum[i] / threadsLatCount[i])
                        << ',' << threadsDelayMax[i] << ',' << threadsDelayMaxTime[i]
                        << ',' << threadsDelayTot[i] << ',' << threadsDelayCount[i]
                        << '\n';
            }
        }
        f << std::endl;
        f.close();
    } else {
        std::cerr << "Failed opening file: " << strerror(errno) << std::endl;
    }
}

void CommTracer::print_mem() {
    //int i;
    std::ofstream f;
    //String fname = "sim.mem-accesses";
    //int num_threads = Sim()->getConfig()->getApplicationCores();

    std::cout << "Print thread's mem access: " << fname_mem_access << std::endl;

    //f.open(Sim()->getConfig()->formatOutputFileName(fname).c_str());
    f.open(fname_mem_access.c_str());
    if (f.is_open()) {

        f << "tid,n_access,sz_access\n";
        for (UInt32 i = 0; i < m_num_threads; i++) {
            f << i << ',' << threadsMemAccesses[i] << ',' << threadsMemSizes[i]
                    << std::endl;
        }
        f.close();
    } else {
        std::cerr << "Failed opening file: " << strerror(errno) << std::endl;
    }
}

void CommTracer::flush_thread_events(thread_id_t t1, thread_id_t t2) {
    std::ofstream f;
    //thread_id_t l, r;
    //std::stringstream ss;

    //    if (t1 < t2) {
    //        l = t1;
    //        r = t2;
    //    } else {
    //        l = t2;
    //        r = t1;
    //    }
    //ss << "sim.comm-events-" << t1 << "_" << t2;
    String fname = fname_comm_events + "-" + itostr(t1) + "_" + itostr(t2);

    //f.open(Sim()->getConfig()->formatOutputFileName(fname).c_str(),
    //        std::ios::app);
    f.open(fname_comm_events.c_str(), std::ios::app);
    //std::cout << "Flush events[ " << t1 << "][" << t2 << ']' << std::endl;
    //f.open(ss.str(), std::ios::app);
    if (f.is_open()) {
        for (auto it : nEvents[t1][t2]) {
            f << t1 << ',' << t2 << ',' << it.first << ','
                    << it.second << ","
                    << szEvents[t1][t2][it.first] << '\n';
        }
        f.close();
        // Clear the flushed events
        //threadLocks[t1].acquire();
        isFlushing[t1] = true;
        nEvents[t1][t2].clear();
        szEvents[t1][t2].clear();
        isFlushing[t1] = false;
        //threadLocks[t2].release();
    } else {
        std::cerr << "Failed opening file: " << strerror(errno) << std::endl;
    }
}

