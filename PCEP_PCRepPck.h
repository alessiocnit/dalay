#ifndef PCEP_PCRepPck_H
#define PCEP_PCRepPck_H

#include "defs.h"
#include "CPck.h"
#include "PCEP_PCReqPck.h"

using namespace std;

class CLink;
class CDomain;
class PathComputationSession;

enum PCEP_RepType {
    Rep_PCE_PCC,
    Rep_HPCE_PCE,
    Rep_PCE_HPCE,
    Rep_PCE_PCE
};

class PCEP_PCRepPck:public CPck {
public:
    PCEP_RepType RepType;

    int RestorationFlag; //This is one if the request is for restoration
    int NoPath;
    CConnection* Conn;
    CNode* ConnSource;
    CNode* ConnDestination;
    
    //The path computation has been performed between FromNode->ToNode
    //FreeLambdas is the number of available lambdas on the computed path
    //LabelSet indicates the status of each lambdas on the computed path
    CNode* FromNode;
    CNode* ToNode;
    int FreeLambdas;
    vector<int> LabelSet;

    PathComputationSession* ComputationSession;
    
    //ERO objects
    vector<CNode*> EdgeNodePath;
    vector<CLink*> EdgeLinkPath;
    vector<CDomain*> DomainPath;

    vector<CNode*> NodePath;
    vector<CLink*> LinkPath;
    vector<int> LambdaPath;

    int SuggestedLabel_ON;
    int SuggestedLabel;

    PCEP_PCRepPck(CNode* src,CNode* dst,int dim,int ttl,double msgGenTime,CConnection* conn, vector<CNode*> node_path, vector<CLink*> link_path, vector<CNode*> node_edge_path, vector<CDomain*> domain_path,int SuggestedLabel, int NoPath, int rest_flag, PCEP_RepType type);
    PCEP_PCRepPck(CNode* src,CNode* dst,int dim,int ttl,double msgGenTime,CConnection* conn, int NoPath, int rest_flag, PCEP_RepType type);
    ~PCEP_PCRepPck();

    virtual void arrived(CNode* node);
};

#endif

