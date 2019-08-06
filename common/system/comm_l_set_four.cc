#include "comm_l_set_four.h"

CommLSetFour::CommLSetFour()
: setLock()
, setCommLF() {
  
}

CommLSetFour::~CommLSetFour() {
}

bool CommLSetFour::exists(IntPtr line) {
    bool found = false;
    CommLFour cand_cl = CommLFour(line);
    //CommL *cand_cl = new CommL(line);
    setLock.acquire_read();
    //std::set<CommL>::iterator it = setCommL.find(cand_cl);
    auto it = setCommLF.find(cand_cl);
    if(it != setCommLF.end())
        found = true;
    else
        found = false;
    setLock.release();
    return found;
}

CommLFour CommLSetFour::getLine(IntPtr line) {
    CommLFour cand_cl = CommLFour(line);
    //CommL *cand_cl = new CommL(line);
    //setLock.acquire_read();
    auto it = setCommLF.find(cand_cl);
    //setLock.release();
    if (it != setCommLF.end()) {
        return (*it);
        //return it;
    }
    else {
        CommLFour new_cl = CommLFour(line);
        //CommL *new_cl = new CommL(line);
        setLock.acquire();
        auto result = setCommLF.insert(new_cl);
        setLock.release();
        return (*result.first);
    }
}

void CommLSetFour::updateLine(IntPtr line, thread_id_t tid, IntPtr addr) {
    CommLFour cand_cl = CommLFour(line);
    auto it = setCommLF.find(cand_cl);
    const_cast<CommLFour&>(*it).update(tid, addr);
}

CommLFour CommLSetFour::getLineMod(IntPtr line) {
    CommLFour l = getLine(line);
    //CommL ptr = const_cast<CommL&>(l);
    //return ptr;
    return const_cast<CommLFour&>(l);
}

