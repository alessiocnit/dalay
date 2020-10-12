#ifndef OFP_ERRORPCK_H
#define OFP_ERRORPCK_H

#include "CPck.h"

class CConnection;
class FlowModeSession;

enum OFP_ErrorType {
    FlowMod_FAIL
};

class OFP_ErrorPck:public CPck {
public:  
  static int active_pk;
  
  CConnection* Conn;
  int ConnId;
  FlowModeSession* Session;
  
  OFP_ErrorPck(CNode* src,CNode* dst,int dim, int ttl,double msgGenTime,CConnection* conn, FlowModeSession* Session);
  virtual ~OFP_ErrorPck();
};

#endif // OFP_ERRORPCK_H
