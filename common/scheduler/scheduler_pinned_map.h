#ifndef __SCHEDULER_PINNED_MAP_H
#define __SCHEDULER_PINNED_MAP_H

#include "scheduler_pinned_base.h"
#include <string>
#include <map>

typedef std::map<thread_id_t, core_id_t> tcMap;

class SchedulerPinnedMap : public SchedulerPinnedBase
{
   public:
      SchedulerPinnedMap(ThreadManager *thread_manager);

      virtual void threadSetInitialAffinity(thread_id_t thread_id);
      bool loadMap(const String &);
	  void printMap();
	  ~SchedulerPinnedMap();

   private:
      core_id_t getNextCore(core_id_t core_first);
      core_id_t getFreeCore(core_id_t core_first);

      const int m_interleaving;
      std::vector<bool> m_core_mask;

      core_id_t m_next_core;
	  tcMap threadCoreMap;
};

#endif // __SCHEDULER_ROAMING_H
