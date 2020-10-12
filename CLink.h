#ifndef CLINK_H
#define CLINK_H

#include "defs.h"

using namespace std;

enum LinkType {INTER_DOMAIN,INTRA_DOMAIN};

class CNet;
class CNode;
class CDomain;
class LinkBuffer;

class CLink {
public:
    int Id;
    LinkStatus Status;
    LinkType Type;

    CDomain* ParentDomain;
    CNet* ParentNet;
    CNode* FromNode;
    CNode* ToNode;

    double length; //in km
    double PropagationDelay;  //in seconds
    double Trx_Bitrate;
    int DistanceMetric;

    LinkBuffer* ControlPlane_Buffer;

    double LinkInterArrival;

    int LSA_NextScheduled_FromNode;
    int LSA_LastSequenceNumber_FromNode;
    double LSA_NextScheduledTime_FromNode;
    double LSA_LastGenerationTime_FromNode;
    
    int LSA_NextScheduled_ToNode;
    int LSA_LastSequenceNumber_ToNode;
    double LSA_NextScheduledTime_ToNode;
    double LSA_LastGenerationTime_ToNode;

    vector<int> LambdaStatus; //Free lambda equal to 0, Busy lambda equal to 1
    int FreeLambdas;

    CLink(int id, double len, CNode* fromnode, CNode* tonode, double bitrate, int metric);
    ~CLink();

    void CreateBuffer(int MaxByte);
    void print();
};

#endif
