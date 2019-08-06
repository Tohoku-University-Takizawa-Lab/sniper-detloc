#include "comm_trace_line.h"
#include "simulator.h"

ThreadLine::ThreadLine()
//: lineLock()
: first(0)
, second(0) {
    //first = 0;
    //second = 0;
}

void ThreadLine::update(thread_id_t tid) {
    //lineLock.acquire();
    thread_id_t tmp = first;
    first = tid;
    second = tmp;
    //lineLock.release();
}

thread_id_t ThreadLine::getFirst() {
    return first;
}

thread_id_t ThreadLine::getSecond() {
    return second;
}

ThreadLines::ThreadLines()
: mapLock()
//: mapMtx()
, firstPrecreateDone(false)
, defNumPrecreate(1000000)
, lines()
, lineMap() {
    ThreadLine *dummy = new ThreadLine(); // zero address means not allocated
    //size_t pos = lines.size();
    lines.push_back(dummy);

    // Initiate threadLines using pre-defined maxthreads, it is ugly but do no harm
    perThreadLines = new std::unordered_map<UInt64, size_t>(MAXTHREADS);
}

ThreadLines::~ThreadLines() {
    delete[] perThreadLines;
}

void ThreadLines::insert(UInt64 line, thread_id_t tid) {
    size_t pos = lineMap[line];
    lines[pos]->update(tid);
}

/**
 * Get region is synchronized if creating new region,
 * because this method can be called from many threads simultaneously.
 * @param line
 * @return 
 */
ThreadLine *ThreadLines::getLine(UInt64 line) {
    //mapLock.acquire();
    size_t pos = lineMap[line];
    //printf("Post: %lu\n", pos);
    //mapLock.release_read();
    //if (pos > 0) {
    //    mapLock.release();
    //return lines[pos];
    //} else {
    if (pos == 0) {
        // New line will be inserted
        //printf("New pos created: %lu\n", pos);
        //ThreadLine *new_line = new ThreadLine();
        ThreadLine *new_line = new ThreadLine();
        //mapLock.acquire();
        //if (lineMap[line] == 0) {
        pos = lines.size();
        //lines.push_back(*new_line);
        lines.push_back(new_line);
        //printf("New pos created: %lu\n", pos);
        lineMap[line] = pos;
        //}
        //mapLock.release();
        //return lines[pos];
    }
    //mapLock.release();
    return lines[pos];
}

ThreadLine *ThreadLines::getThreadLine(thread_id_t tid, UInt64 line) {
    printf("Tid-%d accessing line-%lu\n", tid, line);
    //size_t pos = perThreadLines[tid][line];
    //printf("Pos: %lu\n", *pos);
    auto it = perThreadLines[tid].find(line);
    size_t pos;
    //if (pos == 0) {
    if (it == perThreadLines[tid].end()) {
        pos = emplaceLine(line);
        perThreadLines[tid][line] = pos;
        //auto it = perThreadLines[tid].find(line);
        //if (it != perThreadLines[tid].end())
        //    it->second = pos;
        //printf("Thread-%d create line=%lu, pos=%lu\n", tid, line, pos);
    }else {
        pos = perThreadLines[tid][line];
    }
    return lines[pos];
    //thread_id_t curr_thread = Sim()->getThreadManager()->getCurrentThread()->getId();
    //printf("Curr_t=%d, core_tid=%d\n", curr_thread, tid);
    /*
    setLock.acquire();
    int n = globalLines.count(line);
    setLock.release();
    if (n == 0) {
        size_t pos = emplaceLine(line);
        setLock.acquire();
        globalLines.insert(line);
        setLock.release();
        return lines[pos];
    } else {
        return lines[lineMap[line]];
    }
    */
    /*
    if (!firstPrecreateDone) {
        size_t pos = emplaceLine(line);
        return lines[pos];
    } else {
        return lines[lineMap[line]];
    }*/
}

size_t ThreadLines::emplaceLine(UInt64 line) {
    mapLock.acquire();
    //mapMtx.lock();
    //size_t globalPos = lineMap[line]; // Check if any threads have it
    auto it = lineMap.find(line);
    size_t globalPos;
    //if (globalPos == 0) {
    if (it == lineMap.end()) {
        // New line will be inserted
        ThreadLine *new_line = new ThreadLine();
        globalPos = lines.size();
        lines.push_back(new_line);
        //lineMap[line] = globalPos;
        lineMap.insert(std::make_pair(line, globalPos));
        //preCreateRegion(line, defNumPrecreate);
        //if (!firstPrecreateDone) {
        //    preCreateRegion(line, defNumPrecreate);
        //    firstPrecreateDone = true;
        //}
    } {
      globalPos = lineMap[line];
    }
    //tRegions[tid][line] = globalPos;
    mapLock.release();
    //mapMtx.unlock();
    return globalPos;
}

void ThreadLines::preCreateRegion(UInt64 start_line, int num) {
    size_t new_pos;
    for (int i = 1; i < num + 1; i++) {
        ThreadLine *new_line = new ThreadLine();
        new_pos = lines.size();
        lines.push_back(new_line);
        lineMap[start_line + i] = new_pos;
    }
    //printf("Finished preCreate %d lines starts from %lu\n", num, start_line);
}
