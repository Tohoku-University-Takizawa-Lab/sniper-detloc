/* 
 * File:   comm_l_set.h
 * Author: agung
 *
 * Created on July 2, 2018, 9:53 PM
 */

#ifndef COMMLPROD_H
#define COMMLPROD_H

#include <set>
#include <vector>
#include <unordered_set>
#include "fixed_types.h"
#include "lock.h"

class CommLProdCons {
public:
    IntPtr m_Line;
    //Tid starts from 1, 0 means dummy
    mutable thread_id_t m_First;
    mutable thread_id_t m_Second;

    mutable IntPtr m_First_addr;
    mutable IntPtr m_Second_addr;

    CommLProdCons(IntPtr line):
        m_Line(line), m_First(0), m_Second(0), m_First_addr(0),
        m_Second_addr(0) {
    }
    
    bool operator< (const CommLProdCons &clObj) const {
        return (this->m_Line < clObj.m_Line);
    }

    bool operator ==(const CommLProdCons &clObj) const {
        return (m_Line == clObj.m_Line);
    }

    void update(thread_id_t tid, IntPtr addr);

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
    void setEmpty() {
        m_Line = 0;
    }
    bool isEmpty() {
        return (m_Line == 0);
    }
};

class CommLProdConsSet {
private:
    Lock m_set_lock;
    std::set<CommLProdCons> setCommLPS;
    //std::unordered_set<CommL> setCommL;
    //std::vector<CommLWin *> lineWindows;

public:
    CommLProdConsSet();
    ~CommLProdConsSet();
    CommLProdCons getLine(IntPtr line);
    CommLProdCons getLineMod(IntPtr line);
    CommLProdCons getLineLazy(IntPtr line);
    void updateLine(IntPtr line, thread_id_t tid, IntPtr addr);
    void updateLineLazy(IntPtr line, thread_id_t tid, IntPtr addr);
    void updateCreateLine(IntPtr line, thread_id_t tid, IntPtr addr);
    void updateCreateLineBatch(IntPtr start, UInt32 len, thread_id_t tid, IntPtr addr);
    bool exists(IntPtr line);
    Lock &getLock() { return m_set_lock; }
};

#endif /* COMML_H */

