#include "comm_l_prod.h"

void
CommLProdCons::update(thread_id_t tid, IntPtr addr) {
    m_Second = m_First;
    m_First = tid;

    m_Second_addr = m_First_addr;
    m_First_addr = addr;
}

CommLProdConsSet::CommLProdConsSet()
: setCommLPS() {
}

CommLProdConsSet::~CommLProdConsSet() {
}

bool CommLProdConsSet::exists(IntPtr line) {
    bool found = false;
    CommLProdCons cand = CommLProdCons(line);
    ScopedLock sl(getLock());
    auto it = setCommLPS.find(cand);
    if(it != setCommLPS.end())
        found = true;
    else
        found = false;
    //setLock.release();
    return found;
}

CommLProdCons CommLProdConsSet::getLine(IntPtr line) {
    CommLProdCons cand = CommLProdCons(line);
    //setLock.acquire_read();
    auto it = setCommLPS.find(cand);
    //setLock.release();
    if (it != setCommLPS.end()) {
        return (*it);
    }
    else {
        //CommLProdCons new_cl = CommLProdCons(line);
        //setLock.acquire();
        ScopedLock sl(getLock());
        auto result = setCommLPS.insert(cand);
        //setLock.release();
        return (*result.first);
    }
}

CommLProdCons CommLProdConsSet::getLineLazy(IntPtr line) {
    CommLProdCons cand = CommLProdCons(line);
    auto it = setCommLPS.find(cand);
    if (it != setCommLPS.end()) {
        return (*it);
    }
    else {
        cand.setEmpty();
        return cand;
    }
}

void CommLProdConsSet::updateCreateLine(IntPtr line, thread_id_t tid, IntPtr addr) {
    CommLProdCons cand = CommLProdCons(line);
    auto it = setCommLPS.find(cand);
    if (it != setCommLPS.end()) {
        const_cast<CommLProdCons&>(*it).update(tid, addr);
    }
    else {
        cand.update(tid, addr);
        ScopedLock sl(getLock());
        setCommLPS.insert(cand);
        //setLock.release();
    }
}

void CommLProdConsSet::updateCreateLineBatch(IntPtr start, UInt32 len,
        thread_id_t tid, IntPtr addr) {
    for (IntPtr line = start; line < start+len; ++line) {
        CommLProdCons cand = CommLProdCons(line);
        auto it = setCommLPS.find(cand);
        if (it == setCommLPS.end()) {
            // Not found, new insertion
            cand.update(tid, addr);
            ScopedLock sl(getLock());
            setCommLPS.insert(cand);
        }
        else if (tid != (*it).m_First) {
            const_cast<CommLProdCons&>(*it).update(tid, addr);
        }
    }
}

void CommLProdConsSet::updateLineLazy(IntPtr line, thread_id_t tid, IntPtr addr) {
    CommLProdCons cand = CommLProdCons(line);
    auto it = setCommLPS.find(cand);
    const_cast<CommLProdCons&>(*it).update(tid, addr);
}

CommLProdCons CommLProdConsSet::getLineMod(IntPtr line) {
    CommLProdCons l = getLine(line);
    //CommLProdCons ptr = const_cast<CommLProdCons&>(l);
    //return ptr;
    return const_cast<CommLProdCons&>(l);
}
