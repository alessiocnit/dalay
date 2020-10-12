#ifndef PATHCOMPUTATIONSESSION_H
#define PATHCOMPUTATIONSESSION_H

#include "defs.h"
#include "CPathTable.h"

class CNet;
class CNode;
class PCx_Node;
class CConnection;
class PCEP_PCReqPck;
class PCEP_PCRepPck;

enum Status {OPEN,CLOSED,UNINITIALIZED,STORED};

struct PathComputationRequest {
  Status RequestStatus;
  CNode* Src;
  CNode* Dst;
  
  double LastReplyTime;
  
  //Filled up after remote path computation
  vector<CNode*> nodes;
  vector<CLink*> links;
  
  int NoPath;
  int FreeLambdas;
  vector<int> LabelSet;
};

class PathComputationSession {
  public:
    CNet* ParentNet;
    PCx_Node* ParentNode_HPCE;
    
    int RestorationFlag;

    PCEP_PCReqPck* ReqPacket;
    CConnection* Conn;
    CNode* ConnSource;
    CNode* ConnDestination;
    
    vector<Path*> table;

    Status SessionStatus; //It is CLOSED when the status of all requests is CLOSED
    vector<PathComputationRequest*> Requests;
    
    PathComputationSession(PCEP_PCReqPck* pk, CConnection* conn, CNet* net);
    ~PathComputationSession();
    
    void start();
    int close(vector<CNode*>* pathnode,vector<CLink*>* pathlink, int* base_slot);
    
    void Create_Request(Path* path);
    int Find_Request(CNode* s, CNode* d);
    void ReceivedReply(PCEP_PCRepPck* rep_pk);
    int LastReply();
};

#endif // PATHCOMPUTATIONSESSION_H
