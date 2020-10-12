#ifndef CPATHTABLE_H
#define CPATHTABLE_H

#include "CNode.h"
#include "CNet.h"

enum PathTableType {REAL_NETWORK, ABSTRACT_NETWORK, REAL_DOMAIN, REAL_NODE, FAIL_NETWORK};

class CNet;
class CNode;
class CLink;
class CConnection;

struct Path {
    int PathNumber; //It stores the number of paths between Src and Dst
    int Id; //It identifies the path

    CDomain* ParentDomain;

    CNode* Src;
    CNode* Dst;

    vector<CNode*> nodes;
    vector<CLink*> links;
    
    vector<CNode*> SegmentRouting;
};

class CPathTable {
  
  public:
   
    PathTableType Type;
    CNet* ParentNet;
    CDomain* ParentDomain;
    CNode* ParentNode;
    int FailureFlag;
    string FileName;

    int DistanceMatrix[NMAX][NMAX]; //Store distances in number of hops
    
    //table contains the list of paths
    //index is used for quick find of a src-dst in the table
    //index[src->id*net->node.size()+dst->id] contains the id of the table entry containing the first path from src to dst
    vector<Path*> table;
    vector<int> index; 

    //Builders
    CPathTable(CNet* Net); //Builder for CNet->HPCE_PathTable
    CPathTable(CNet* Net, int add_hops, int FailureFlag); //Builder for CNet->PathTable
    CPathTable(CNet* Net, CDomain* LocalDomain, int add_hops, int FailureFlag); //Builder for CDomain->PathTable
    CPathTable(CNet* Net, CNode* LocalNode, CPathTable* CentralTable); //Builder for CNode->PathTable
    CPathTable(CNet* Net, CLink* failed_1, CLink* failed_2, int add_hops, int FailureFlag); //Builder for failures
    CPathTable(CNet* Net, CLink* failed_1, CLink* failed_2, int add_hops, int FailureFlag, int cnet_flag); //CNET builder for failures
    ~CPathTable();

    //Routing functions at simple nodes 
    int ComputeCandidatePaths(CNode* s, CNode* d, vector<Path*>& ltable, CConnection* conn);
    int NoInfo_RandomPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, CConnection* rconn);
    int AggInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, CConnection* rconn);
    int Flexible_DetInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, CConnection* rconn);
    int Flexible_PCE_WavelengthAssignmentFF(vector<CLink*> pathlink, int required_slots, int conn_id);
    
    //Routing functions at PCE without the utilizatrion of SNMP database
    int PCE_ComputeCandidatePaths(CNode* s, CNode* d, vector<Path*>& ltable, CConnection* conn);
    int PCE_NoInfo_RandomPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, CConnection* rconn);
    int PCE_AggInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, int* freelambdas, CConnection* rconn);
    int PCE_DetInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, int* freelambdas, vector<int>* lambdastatus, CConnection* rconn);
    int Flexible_PCE_DetInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, int* freelambdas, vector<int>* lambdastatus, CConnection* rconn);
    
    
    //Routing functions at PCE with the utilizatrion the SNMP database
    int PCE_SNMP_AggInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, CConnection* rconn);
    int Flexible_PCE_SNMP_DetInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, CConnection* rconn);
    int Flexible_PCE_SNMP_WavelengthAssignmentFF(vector<CLink*> pathlink, int required_slots, int conn_id, int rest_flag, CConnection* conn);

    //Routing functions at the HPCE
    int HPCE_ComputeCandidatePaths(CNode* s, CNode* d, vector<Path*>& ltable, CConnection* conn);
    int HPCE_NoInfo_EdgeSequence(CNode* s, CNode* d, vector<CNode*>* pathnode, vector <CLink*>* pathlink,CConnection* rconn);
    int HPCE_AggInfo_EdgeSequence(CNode* s, CNode* d, vector<CNode*>* pathnode, vector <CLink*>* pathlink,CConnection* rconn);
    int HPCE_DetInfo_EdgeSequence(CNode* s, CNode* d, vector<CNode*>* pathnode, vector <CLink*>* pathlink,CConnection* rconn);
    
    int CNet_ComputeCandidatePaths(CNode* s, CNode* d, vector<Path*>& ltable, CConnection* conn);
    int Flexible_HPCE_BGP_DetInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector <CLink*>* pathlink,CConnection* rconn);
    int Flexible_HPCE_WavelengthAssignmentFF(vector<CLink*> pathlink, int required_slots, int conn_id, int rest_flag, CConnection* conn);
    
    //Routing using actual resources on the links
    int Flexible_LINKS_DetInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector <CLink*>* pathlink,CConnection* rconn);
    int Flexible_LINKS_AggInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector <CLink*>* pathlink,CConnection* rconn);
    //Spectrum assignment using actual resources on the links
    int Flexible_LINKS_WavelengthAssignmentFF(vector<CLink*> pathlink, int required_slots, int conn_id, int rest_flag, CConnection* conn); 
    
     //Statistical utility functions
    int CollidingLinksOccurrence(vector<CLink*> PathLinks, vector<CLink*> CollidingLinks);
    void LinkInterArrival(double NetworkInterArrival);
    void Routes_PerLink(CLink* link, int *N, int *X);
    void LightPaths_PerLink(CLink* link, int *n, int *x);
    
    void SegmentRoutingComputation();
    void SegmentRouting_EqualPath(Path* path, vector<CNode*>, int depth);
    
  private:
    
    //Functions used by the class builders 
    void DepthFirstSearch_Network(CNode* s, CNode* d, int depth, int ElapsedDistance, int MaxDistance, vector<int> visited, vector<CNode*> path, vector<Path*>& ltable);
    void DepthFirstSearch_Domain(CNode* s, CNode* d, int depth, int ElapsedDistance, int MaxDistance, vector<int> visited, vector<CNode*> path, vector<Path*>& ltable);
    void DepthFirstSearch_HPCE(int s, int d, CNode* src_node, CNode* dst_node, int depth, int ElapsedDistance, int MaxDistance, vector<int> visited, vector<int> path, vector<Path*>& ltable);
 
    void LoadDistanceMatrix_Network();
    void LoadDistanceMatrix_Domain();
    void LoadDistanceMatrix_HPCE();

    void print(int dim_distance_matrix);
    void save(int dim_distance_matrix);
    void load(int dim_distance_matrix);
};

#endif
