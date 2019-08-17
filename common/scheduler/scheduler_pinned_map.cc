#include "scheduler_pinned_map.h"
#include "config.hpp"

#include "split_string.h"
#include <fstream>
#include <vector>
//#include "thread.h"

SchedulerPinnedMap::SchedulerPinnedMap(ThreadManager *thread_manager)
   : SchedulerPinnedBase(thread_manager, SubsecondTime::NS(Sim()->getCfg()->getInt("scheduler/pinned/quantum")))
   , m_interleaving(Sim()->getCfg()->getInt("scheduler/pinned/interleaving"))
   , m_next_core(0)
   , threadCoreMap()
{
   m_core_mask.resize(Sim()->getConfig()->getApplicationCores());

   for (core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); core_id++)
   {
       m_core_mask[core_id] = Sim()->getCfg()->getBoolArray("scheduler/pinned/core_mask", core_id);
   }

   String map_fname = Sim()->getCfg()->getString("scheduler/pinned/map_file");
   printf("[DeTLoc] Loading mapping file: %s\n", map_fname.c_str());
   loadMap(map_fname);
}

core_id_t SchedulerPinnedMap::getNextCore(core_id_t core_id)
{
   while(true)
   {
      core_id += m_interleaving;
      if (core_id >= (core_id_t)Sim()->getConfig()->getApplicationCores())
      {
         core_id %= Sim()->getConfig()->getApplicationCores();
         core_id += 1;
         core_id %= m_interleaving;
      }
      if (m_core_mask[core_id])
         return core_id;
   }
}

core_id_t SchedulerPinnedMap::getFreeCore(core_id_t core_first)
{
   core_id_t core_next = core_first;

   do
   {
      if (m_core_thread_running[core_next] == INVALID_THREAD_ID && m_core_mask[core_next])
         return core_next;

      core_next = getNextCore(core_next);
   }
   while(core_next != core_first);

   return core_first;
}

void SchedulerPinnedMap::threadSetInitialAffinity(thread_id_t thread_id)
{
	core_id_t core_id = INVALID_CORE_ID;
 	if (threadCoreMap.count(thread_id) == 1) {
          core_id = threadCoreMap[thread_id];
	}
    else {
   		core_id = getFreeCore(m_next_core);
   		m_next_core = getNextCore(core_id);
	}
   	//core_id_t core_id = getFreeCore(m_next_core);
   	//m_next_core = getNextCore(core_id);

   m_thread_info[thread_id].setAffinitySingle(core_id);
   //LOG_PRINT("Scheduler: thread %d now scheduled to core %d", thread_id, core_id);
   printf("[DeTLoc] map thread-%d to core-%d\n", thread_id, core_id);
}

bool SchedulerPinnedMap::loadMap(const String &filename) // Read process-to-core mapping
{
    ifstream ifile;
    ifile.open(filename.c_str());
    if(!ifile) {
        //could not read the file.
		printf("[DeTLoc] Error reading the mapping file, back to Default!\n");
        return false; 
	}
    string line;
    string key;
    vector<string> v_str;
    while(ifile>>line)
    {
        splitString(v_str,line,':');
        for(vector<string>::iterator iter = v_str.begin();;++iter)        //First vector element is the key.
        {
            if(iter == v_str.begin())
            {
                threadCoreMap[std::stoi(*iter)] = -1;
                key= *iter;
                continue;
            }

            thread_id_t key_i = std::stoi(key);
            core_id_t val_i = std::stoi(*iter);
            //cout << key_i << "=>" << val_i << "\n";
            threadCoreMap[key_i] = val_i;
            v_str.clear();
            break;
        }
    }
    if (ifile.is_open())
        ifile.close();
    return true;
}

void SchedulerPinnedMap::printMap()
{
    //for(tcMap::const_iterator iter= threadCoreMap.begin(); iter!=threadCoreMap.end(); ++iter)
    for(std::map<thread_id_t, core_id_t>::const_iterator iter= threadCoreMap.begin(); iter!=threadCoreMap.end(); ++iter)
    {
        cout <<iter->first<<":"<<iter->second;
        cout<<"\n";
    }
}

SchedulerPinnedMap::~SchedulerPinnedMap() {
    //delete &threadCoreMap;
}

