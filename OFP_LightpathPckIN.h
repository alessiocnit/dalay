#ifndef OFP_LIGHTPATHPCKIN_H
#define OFP_LIGHTPATHPCKIN_H

#include "CPck.h"

class CNode;
class CConnection;

class OFP_LightpathPckIN:public CPck {
  
public:
  static int active_pk;
  
  vector<CConnection*> Conns;
  vector<int> ConnIds;
  vector<double> ConnGenerationTimes;
  vector<double> ConnDurationTimes;
  
  int RestorationFlag;

  OFP_LightpathPckIN(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, vector<CConnection*>& conns);
  virtual ~OFP_LightpathPckIN();
  virtual void arrived(CNode* node);
};

#endif // OFP_LIGHTPATHPCKIN_H
