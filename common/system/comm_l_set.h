/* 
 * File:   comm_l_set.h
 * Author: agung
 *
 * Created on July 2, 2018, 9:53 PM
 */

#ifndef COMML_H
#define COMML_H

#include <set>
#include <vector>
#include <unordered_set>
#include "fixed_types.h"
#include "lock.h"

struct CommLWin {
    thread_id_t first;
    thread_id_t second;
   
    CommLWin(): first(0), second(0) {
    }

    void update(thread_id_t new_tid) {
        thread_id_t tmp = first;
        first = new_tid;
        second = tmp;
    }
};

class CommL {
public:
    UInt64 m_Addr;
    size_t m_Ref; //Start from 1, 0 means dummy
    mutable thread_id_t m_First;
    mutable thread_id_t m_Second;

    CommL(UInt64 addr):
        m_Addr(addr), m_Ref(0), m_First(0), m_Second(0) {
    }
    
    CommL(UInt64 addr, size_t ref):
        m_Addr(addr), m_Ref(ref), m_First(0), m_Second(0) {
    }

    bool operator< (const CommL &clObj) const {
        return (this->m_Addr < clObj.m_Addr);
    }

    bool operator ==(const CommL &clObj) const {
        return (m_Addr == clObj.m_Addr);
    }

    void update(thread_id_t tid) {
        thread_id_t tmp = m_First;
        m_First = tid;
        m_Second = tmp;
    }
    thread_id_t getFirst() {
        return m_First;
    }
    thread_id_t getSecond() {
        return m_Second;
    }
};

/*
namespace std
{
  template<>
    struct hash<CommL>
    {
      size_t operator()(const CommL &obj) const {
        return hash<UInt64>()(obj.m_Addr);
      }
    };
}
*/

class CommLSet {
private:
    Lock setLock;
    Lock vecLock;
    std::set<CommL> setCommL;
    //std::unordered_set<CommL> setCommL;
    std::vector<CommLWin *> lineWindows;

public:
    CommLSet();
    ~CommLSet();
    CommL getLine(UInt64 addr);
    CommL getLineMod(UInt64 addr);
    void updateLine(UInt64 addr, thread_id_t tid);
    bool exists(UInt64 addr);
    CommLWin *getLineWindow(UInt64 addr);
};

#endif /* COMML_H */

