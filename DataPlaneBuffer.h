#ifndef DATAPLANEBUFFER_H
#define DATAPLANEBUFFER_H

#include "CBuffer.h"

using namespace std;

class CNode;

class DataPlaneBuffer:public CBuffer {

public:
  
  CNode* ParentNode;
  
  DataPlaneBuffer(CNode* node);
  virtual ~DataPlaneBuffer();
};

#endif // DATAPLANEBUFFER_H
