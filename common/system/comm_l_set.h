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
#include <unordered_map>

/*
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
*/

class CommL {
public:
    IntPtr m_Line;
    size_t m_Ref; //Start from 1, 0 means dummy
    mutable thread_id_t m_First;
    mutable thread_id_t m_Second;

    mutable IntPtr m_First_addr;
    mutable IntPtr m_Second_addr;

    mutable bool m_First_w_op;
    mutable bool m_Second_w_op;

    CommL(IntPtr line):
        m_Line(line), m_Ref(0), m_First(0), m_Second(0), m_First_addr(0), m_Second_addr(0),
        m_First_w_op(0), m_Second_w_op(0) {
    }
    
    CommL(IntPtr line, size_t ref):
        m_Line(line), m_Ref(ref), m_First(0), m_Second(0), m_First_addr(0), m_Second_addr(0),
        m_First_w_op(0), m_Second_w_op(0) {
    }

    CommL():m_Line(0), m_Ref(0), m_First(0), m_Second(0), m_First_addr(0), m_Second_addr(0),
        m_First_w_op(0), m_Second_w_op(0) {
    }

    bool operator< (const CommL &clObj) const {
        return (this->m_Line < clObj.m_Line);
    }

    bool operator ==(const CommL &clObj) const {
        return (m_Line == clObj.m_Line);
    }

    void update(thread_id_t tid, IntPtr addr, bool w_op);
    /*{
        m_Second = m_First;
        m_First = tid;

        m_Second_addr = m_First_addr;
        m_First_addr = addr;

        m_Second_w_op = m_First_w_op;
        m_First_w_op = w_op;
    }
    */
    thread_id_t getFirst() {
        return m_First;
    }
    thread_id_t getSecond() {
        return m_Second;
    }
    IntPtr getFirst_addr() {
        return m_First_addr;
    }
    IntPtr getSecond_addr() {
        return m_Second_addr;
    }
    bool getFirst_w_op() {
        return m_First_w_op;
    }
    bool getSecond_w_op() {
        return m_Second_w_op;
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
    //Lock setLock;
    Lock m_set_lock;
    //Lock vecLock;
    std::set<CommL> setCommL;
    //std::unordered_set<CommL> setCommL;
    //std::vector<CommLWin *> lineWindows;

public:
    CommLSet();
    ~CommLSet();
    CommL getLine(IntPtr line);
    CommL getLineMod(IntPtr line);
    void updateLine(IntPtr line, thread_id_t tid, IntPtr addr, bool w_op);
    bool exists(IntPtr line);
    Lock &getLock() { return m_set_lock; }
    //CommLWin *getLineWindow(UInt64 addr);
};

#define LOCK_POOL_SIZE 1024

class CommLMap {
private:
    Lock m_set_lock;
    std::unordered_map<IntPtr, CommL> mapCommL;
    std::vector<Lock> locks;

public:
    CommLMap();
    ~CommLMap();
    CommL getLine(IntPtr line);
    CommL getLineMod(IntPtr line);
    void updateLine(IntPtr line, thread_id_t tid, IntPtr addr, bool w_op);
    bool exists(IntPtr line);
    Lock &getLock() { return m_set_lock; }
    Lock &getLock(IntPtr line) { return locks[line % LOCK_POOL_SIZE]; }
};

#endif /* COMML_H */

