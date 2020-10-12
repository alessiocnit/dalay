#ifndef CROSSCONNECTIONEVENT_H
#define CROSSCONNECTIONEVENT_H

#include "CEvent.h"

using namespace std;

class CNode;
class DataPlaneBuffer;

class CrossConnectionEvent: public CEvent {
  CNode* AtNode;
  DataPlaneBuffer* buffer;
  
public:
  CrossConnectionEvent(CNode* node, double time);
  virtual ~CrossConnectionEvent();
  
  virtual void execute();
};

#endif // CROSSCONNECTIONEVENT_H
