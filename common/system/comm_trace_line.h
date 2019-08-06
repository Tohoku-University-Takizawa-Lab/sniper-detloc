/* 
 * File:   thread_line.h
 * Author: agung
 *
 * Created on July 2, 2018, 9:53 PM
 */

#ifndef COMM_TRACE_LINE_H
#define COMM_TRACE_LINE_H

#define MAXTHREADS 1024

#include <vector>
#include <unordered_map>
#include <set>
#include "lock.h"
#include "fixed_types.h"

class ThreadLine {
private:
        //Lock lineLock;
        thread_id_t first;
        thread_id_t second;
        
public:
    ThreadLine();
    void update(thread_id_t tid);
    thread_id_t getFirst();
    thread_id_t getSecond();
    
//        ThreadLine() {
//            first = 0;
//            second = 0;
//        }
//        
//        void update(thread_id_t tid) {
//            lineLock.acquire();
//            thread_id_t tmp = first;
//            first = tid;
//            second = tmp;
//            lineLock.release();
//        }
//        
//        thread_id_t getFirst() {
//            return first;
//        }
//        
//        thread_id_t getSecond() {
//            return second;
//        }
};

class ThreadLines {
private:    
    Lock mapLock;
    bool firstPrecreateDone;
    size_t defNumPrecreate;
    
    std::vector<ThreadLine*> lines;
    std::unordered_map<UInt64, size_t> lineMap;
    std::unordered_map<UInt64, size_t> *perThreadLines;
    //std::unordered_map<UInt64, size_t> perThreadLines;
    std::set<UInt64> globalLines;
  

public:
    ThreadLines();
    ~ThreadLines();
    void insert(UInt64 line, thread_id_t tid);
    ThreadLine *getLine(UInt64 line);
    ThreadLine *getThreadLine(thread_id_t tid, UInt64 line);
    size_t emplaceLine(UInt64 line);
    void preCreateRegion(UInt64 start_line, int num);
    
//    ThreadLines() {
//        ThreadLine dummy;   // zero address means not allocated
//        //size_t pos = lines.size();
//        lines.push_back(dummy);
//    }
//
//    ~ThreadLines() {
//    }
//
//    void insert(UInt64 line, thread_id_t tid) {
//        size_t pos = lineMap[line];
//        lines[pos].update(tid);
//    }
//    
//    /**
//     * Get region is synchronized if creating new region,
//     * because this method can be called from many threads simultaneously.
//     * @param line
//     * @return 
//     */
//    ThreadLine getLine(UInt64 line) {
//        mapLock.acquire_read(); 
//        size_t pos = lineMap[line];
//        mapLock.release_read();
//        if (pos > 0) {
//            return lines[lineMap[line]];
//        }
//        else {
//            // New line will be inserted
//            ThreadLine new_line;
//            mapLock.acquire();
//            if (lineMap[line] == 0) {
//                pos = lines.size();
//                lines.push_back(std::move(new_line));
//                lineMap[line] = pos;
//            }
//            mapLock.release();
//            return new_line;
//        }
//    }
};


#endif /* THREAD_LINE_H */

