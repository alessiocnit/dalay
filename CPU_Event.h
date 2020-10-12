#ifndef CPU_EVENT_H
#define CPU_EVENT_H

#include "CEvent.h"

using namespace std;

class CNode;
class CPU_Buffer;

class CPU_Event: public CEvent {
  CNode* AtNode;
  CPU_Buffer* buffer;
  
public:
  CPU_Event(CNode* node, double time, EventType type);
  virtual ~CPU_Event();
  
  virtual void execute();
};

#endif // CPU_EVENT_H
