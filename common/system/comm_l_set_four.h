/* 
 * File:   comm_l_set_four.h
 * Author: agung
 *
 * Created on January 17, 2019, 12:41 PM
 */

#ifndef COMMLFOUR_H
#define COMMLFOUR_H

#include <set>
#include <unordered_set>
#include "fixed_types.h"
#include "lock.h"

class CommLFour {
public:
    IntPtr m_Line;
    size_t m_Ref; //Start from 1, 0 means dummy
    mutable thread_id_t m_First;
    mutable thread_id_t m_Second;
    mutable thread_id_t m_Third;
    mutable thread_id_t m_Fourth;
    
    mutable IntPtr m_First_addr;
    mutable IntPtr m_Second_addr;
    mutable IntPtr m_Third_addr;
    mutable IntPtr m_Fourth_addr;

    CommLFour(IntPtr line):
        m_Line(line), m_Ref(0), m_First(0), m_Second(0), m_Third(0), m_Fourth(0),
        m_First_addr(0), m_Second_addr(0), m_Third_addr(0), m_Fourth_addr(0) {
    }
    
    CommLFour(IntPtr line, size_t ref):
        m_Line(line), m_Ref(ref), m_First(0), m_Second(0), m_Third(0), m_Fourth(0),
        m_First_addr(0), m_Second_addr(0), m_Third_addr(0), m_Fourth_addr(0) {
    }

    bool operator< (const CommLFour &clObj) const {
        return (this->m_Line < clObj.m_Line);
    }

    bool operator ==(const CommLFour &clObj) const {
        return (m_Line == clObj.m_Line);
    }

    void update(thread_id_t tid, IntPtr addr) {
        m_Fourth = m_Third;
        m_Third = m_Second;
        m_Second = m_First;
        m_First= tid;
        
        m_Fourth_addr = m_Third_addr;
        m_Third_addr = m_Second_addr;
        m_Second_addr = m_First_addr;
        m_First_addr = addr;
    }
    thread_id_t getFirst() {
        return m_First;
    }
    thread_id_t getSecond() {
        return m_Second;
    }
    thread_id_t getThird() {
        return m_Third;
    }
    thread_id_t getFourth() {
        return m_Fourth;
    }
    IntPtr getFirst_addr() {
        return m_First_addr;
    }
    IntPtr getSecond_addr() {
        return m_Second_addr;
    }
    IntPtr getThird_addr() {
        return m_Third_addr;
    }
    IntPtr getFourth_addr() {
        return m_Fourth_addr;
    }
};


class CommLSetFour {
private:
    //Lock setLock;
    Lock m_set_lock;
    std::set<CommLFour> setCommLF;

public:
    CommLSetFour();
    ~CommLSetFour();
    CommLFour getLine(IntPtr line);
    CommLFour getLineMod(IntPtr line);
    void updateLine(IntPtr line, thread_id_t tid, IntPtr addr);
    bool exists(IntPtr line);
    Lock &getLock() { return m_set_lock; }
};

#endif /* COMMLFOUR_H */

