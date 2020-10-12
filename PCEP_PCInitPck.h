#ifndef PCEP_PCInitPck_H
#define PCEP_PCInitPck_H

#include "CPck.h"

using namespace std;

class CNode;
class CConnection;
class InterDomainSession;

struct Route;

class PCEP_PCInitPck:public CPck {
public:
    CConnection* Conn;
    CNode* ConnSource;
    CNode* ConnDestination;
    
    int ConnId;
    
    //The path should be established FromNode - ToNode
    CNode* FromNode;
    CNode* ToNode;
    
    InterDomainSession* SetupSession;

    PCEP_PCInitPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, CConnection* conn, InterDomainSession* session);
    ~PCEP_PCInitPck();

    virtual void arrived(CNode* node);
};

#endif





