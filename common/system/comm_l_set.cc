#include "comm_l_set.h"

void
CommL::update(thread_id_t tid, IntPtr addr, bool w_op) {
    m_Second = m_First;
    m_First = tid;

    m_Second_addr = m_First_addr;
    m_First_addr = addr;

    //m_Second_w_op = m_First_w_op;
    //m_First_w_op = w_op;

    // Set latest write
    //if (m_First_addr == m_Second_addr && w_op == true) {
    if (m_First_addr == m_Second_addr) {
       m_Second_w_op = false; 
    }
    else {
        m_Second_w_op = m_First_w_op;
    }
    m_First_w_op = w_op;
}

CommLSet::CommLSet()
: setCommL() {
//: setLock()
//, vecLock()
//, setCommL() {
//, lineWindows() {
    //lineWindows.reserve(1000);
    //CommLWin * new_win = new CommLWin();
    //lineWindows.push_back(new_win);
}

CommLSet::~CommLSet() {
}

bool CommLSet::exists(IntPtr line) {
    bool found = false;
    CommL cand_cl = CommL(line);
    //CommL *cand_cl = new CommL(line);
    //setLock.acquire_read();
    ScopedLock sl(getLock());
    //std::set<CommL>::iterator it = setCommL.find(cand_cl);
    auto it = setCommL.find(cand_cl);
    if(it != setCommL.end())
        found = true;
    else
        found = false;
    //setLock.release();
    return found;
}

CommL CommLSet::getLine(IntPtr line) {
    CommL cand_cl = CommL(line);
    //CommL *cand_cl = new CommL(line);
    //setLock.acquire_read();
    auto it = setCommL.find(cand_cl);
    //setLock.release();
    if (it != setCommL.end()) {
        return (*it);
        //return it;
    }
    else {
        CommL new_cl = CommL(line);
        //CommL *new_cl = new CommL(line);
        //setLock.acquire();
        ScopedLock sl(getLock());
        auto result = setCommL.insert(new_cl);
        //setLock.release();
        return (*result.first);
    }
}

void CommLSet::updateLine(IntPtr line, thread_id_t tid, IntPtr addr, bool w_op) {
    CommL cand_cl = CommL(line);
    auto it = setCommL.find(cand_cl);
    const_cast<CommL&>(*it).update(tid, addr, w_op);
}

CommL CommLSet::getLineMod(IntPtr line) {
    CommL l = getLine(line);
    //CommL ptr = const_cast<CommL&>(l);
    //return ptr;
    return const_cast<CommL&>(l);
}

/*
CommLWin *CommLSet::getLineWindow(UInt64 line) {
    CommL cand_cl = CommL(line);
    //setLock.acquire_read();
    auto it = setCommL.find(cand_cl);
    //setLock.release();
    if (it != setCommL.end()) {
        //return (*it);
        vecLock.acquire();
        //setLock.acquire();
        //CommLWin * win = lineWindows[(*it).m_Ref];
        CommLWin *win = lineWindows[(*it).m_Ref];
        vecLock.release();
        //setLock.acquire();
        //return lineWindows[(*it).m_Ref];
        return win;
        //return it;
    }
    else {
        //CommL new_cl = CommL(line);
        //CommL *new_cl = new CommL(line);
        //setLock.acquire();
        vecLock.acquire();
        CommLWin * new_win = new CommLWin();
        //CommLWin new_win;
        size_t new_pos = lineWindows.size();
        //lineWindows.push_back(std::move(new_win));
        lineWindows.push_back(new_win);
        //auto it = lineWindows.emplace(lineWindows.end());
        vecLock.release();
        //auto it = lineWindows.emplace(lineWindows.end());
        //auto result = setCommL.insert(CommL(line, new_pos));
        CommL new_cl = CommL(line, new_pos);
        setLock.acquire();
        setCommL.insert(new_cl);
        setLock.release();
        //vecLock.release();
        //return (*result.first);
        return new_win;
        //return (*it);
    }
}
*/

/*
CommL CommLSet::getLine(UInt64 line) {
    CommL cand_cl = CommL(line);
    //setLock.acquire_read();
    std::set<CommL>::iterator it = setCommL.find(cand_cl);
    //setLock.release();
    if (it != setCommL.end()) {
        return (*it);
        //return it;
    }
    else {
        CommL new_cl = CommL(line);
        setLock.acquire();
        auto result = setCommL.insert(new_cl);
        setLock.release();
        return (*result.first);
    }
}
*/

