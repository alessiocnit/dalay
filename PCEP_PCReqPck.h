#ifndef PCEP_PCReqPck_H
#define PCEP_PCReqPck_H

#include "CPck.h"

using namespace std;

class CNode;
class CConnection;
class PathComputationSession;

struct Route;

enum PCEP_ReqType {
    Req_PCC_PCE,
    Req_PCE_HPCE,
    Req_HPCE_PCE,
    Req_PCE_PCE
};

class PCEP_PCReqPck:public CPck {
public:
    PCEP_ReqType ReqType;

    int RestorationFlag; //This is one if the request is for restoration
    CConnection* Conn;
    CNode* ConnSource;
    CNode* ConnDestination;
    int Conn_width_slots;
    
    //The path computation will be performed between FromNode->ToNode
    CNode* FromNode;
    CNode* ToNode;
    
    PathComputationSession* ComputationSession;

    Route* RestorationPath; //This field is initialized as NULL and mangaed at the PCE only in case of ILP
    Route* ProvisioningPath;

    PCEP_PCReqPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, CConnection* conn, int rest_flag, PCEP_ReqType type);
    ~PCEP_PCReqPck();

    virtual void arrived(CNode* node);
};

#endif





