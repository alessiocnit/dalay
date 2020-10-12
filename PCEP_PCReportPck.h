#ifndef PCEP_PCReportPck_H
#define PCEP_PCReportPck_H

#include "CPck.h"

using namespace std;

class CNode;
class CConnection;
class InterDomainSession;

struct Route;

class PCEP_PCReportPck:public CPck {
public:
    CConnection* Conn;
    CNode* ConnSource;
    CNode* ConnDestination;
    
    int ConnId;
    int ReplyType;
    
    //The path should be established FromNode - ToNode
    CNode* FromNode;
    CNode* ToNode;
    
    InterDomainSession* SetupSession;

    PCEP_PCReportPck(CNode* src, CNode* dst, int dim, int ttl, double msgGenTime, CConnection* conn, InterDomainSession* session, int reply);
    ~PCEP_PCReportPck();

    virtual void arrived(CNode* node);
};

#endif





