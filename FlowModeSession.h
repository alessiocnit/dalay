#ifndef FLOWMODESESSION_H
#define FLOWMODESESSION_H

#include "defs.h"
#include "InterDomainSession.h"

enum OFP_Status {OFP_OPEN, OFP_CLOSED, OFP_CONFIG_SENT, OFP_CONFIGURED, OFP_DEL_SENT, OFP_DELETED};

class CNet;
class CNode;
class CLink;
class CConnection;
class OFP_FlowModPck;
class InterDomainSession;

struct FlowModRequest {
  OFP_Status RequestStatus;

  //CConnection* conn;
  CNode* ConfiguredNode;
  //CLink* ConfiguredLink;
  
  //For flexible
  //int base_slot;
  //int width_slots;
};

class FlowModeSession {
public:
  static int active_flowmodesessions;
  
  FlowModeType Type;
  
  CNet* ParentNet;
  CNode* OpenFlowController;
  CNode* NodeToReply;
  vector<CConnection*> Conns;
  double SessionStartTime;
  int RestorationFlag;
  
  InterDomainSession* ParentInterDomain_Session;
  
  OFP_Status SessionStatus; //It is CONFIGURED when the status of all requests is CONFIGURED
  vector<FlowModRequest*> Requests;
  
  void start(vector<CConnection*>& conns);
  int close();
    
  void Create_Request(CNode* n);
  void ReceivedReply(CNode* n);
  int LastReply();
  
  FlowModeSession(CNet* net, CNode* controller, vector<CConnection*>& conns, FlowModeType type, CNode* reply_node);
  virtual ~FlowModeSession();
};

#endif // FLOWMODESESSION_H
