#ifndef INTERDOMAINSESSION_H
#define INTERDOMAINSESSION_H

#include "defs.h"

enum INTERDOMAIN_PCEP_Status {
  INTERDOMAIN_PCEP_OPEN, 
  INTERDOMAIN_PCEP_CLOSED, 
  INTERDOMAIN_PCEP_CONFIG_SENT, 
  INTERDOMAIN_PCEP_CONFIGURED, 
  INTERDOMAIN_PCEP_DEL_SENT, 
  INTERDOMAIN_PCEP_DELETED
};

enum INTERDOMAIN_SESSION_Type {
  PARALLEL_SESSION,
  SEQUENTIAL_SESSION
};

class CNet;
class CNode;
class CLink;
class CConnection;
class PCx_Node;

struct ControllerRequest {
  INTERDOMAIN_PCEP_Status RequestStatus;

  CConnection* conn_segment;
  PCx_Node* ConfiguringController;
  //CLink* ConfiguredLink;
  
  //For flexible
  //int base_slot;
  //int width_slots;
  int ReplyType; // {NO_PATH=0, YES_PATH=0}
};

class InterDomainSession {
public:
  CNet* ParentNet;
  CNode* OpenFlowController;
  CConnection* Conn;
  double SessionStartTime;
  
  INTERDOMAIN_SESSION_Type SessionType;
  INTERDOMAIN_PCEP_Status SessionStatus; //It is CONFIGURED when the status of all requests is CONFIGURED
  vector<ControllerRequest*> Requests;
  vector<PCx_Node*> controllers_sequence;
  
  void start_parallel_session(CConnection* conn);
  void start_sequential_session(CConnection* conn);
  int close();
    
  void Create_Request(PCx_Node* n);
  void ReceivedReply(CNode* n, CConnection* conn, int reply);
  int LastReply();
  
  InterDomainSession(CNet* net, PCx_Node* controller, CConnection* conn);
  virtual ~InterDomainSession();
};

#endif // FLOWMODESESSION_H