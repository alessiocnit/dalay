#ifndef OFP_FLOWMODPCK_H
#define OFP_FLOWMODPCK_H

#include "CPck.h"

class CLink;
class CConnection;
class FlowModeSession;

enum OFP_FlowModType {
    FlowMod_ADD,
    FlowMod_DELETE,
    FlowMod_ACK,
    FlowMod_CLOSE,
    FlowMod_NACK //This is not part of the standard
};


struct flow_entry {
  CConnection* Conn;
  int ConnId;
  
  CLink* LinkToReserve;
  
  //For flexible
  int base_slot;
  int width_slots;
  
  vector<CNode*> ConnNodePath;
  //vector<CLink*> ConnLinkPath;
};

class OFP_FlowModPck:public CPck {
public:
  static int active_pk;
  
  OFP_FlowModType OFP_PckType;
  
  vector<flow_entry*> FlowEntries;
  
  FlowModeSession* Session;
  
  OFP_FlowModPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, vector<CConnection*>& conns, FlowModeSession* session,OFP_FlowModType type);
  OFP_FlowModPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, vector<CConnection*>& conns, OFP_FlowModType type);
  OFP_FlowModPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, FlowModeSession* session, OFP_FlowModType type);
  
  virtual ~OFP_FlowModPck();
  virtual void arrived(CNode* node);
};

#endif // OFP_FLOWMODPCK_H
