#ifndef CPU_BUFFER_H
#define CPU_BUFFER_H

#include "CBuffer.h"

using namespace std;

class CNode;

class CPU_Buffer:public CBuffer { 
  public:
  
  CNode* ParentNode;
  
  CPU_Buffer(CNode* node);
  virtual ~CPU_Buffer();
};

#endif // CPU_BUFFER_H

