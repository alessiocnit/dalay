#ifndef OFP_LIGHTPATHPCKOUT_H
#define OFP_LIGHTPATHPCKOUT_H

#include "CPck.h"

class CNode;
class CConnection;

class OFP_LightpathPckOUT:public CPck { 
  
public:
  static int active_pk;
  
  CConnection* Conn;
  int ConnId;
  double ConnGenerationTime;
  double ConnDurationTime;

  OFP_LightpathPckOUT(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, CConnection* conn);
  virtual ~OFP_LightpathPckOUT();
  virtual void arrived(CNode* node); 
};

#endif // OFP_LIGHTPATHPCKOUT_H
