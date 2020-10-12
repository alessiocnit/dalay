#ifndef CDOMAIN_H
#define CDOMAIN_H

#include "defs.h"

using namespace std;

class CNet;
class CDomain;
class CNode;
class CLink;
class PCx_Node;
class CPathTable;

/* Domain routing table contains for each network domain:
--- 1. Destination domain;
--- 2. Next hop domain;
--- 3. Distance as number of domain hops; */
struct DomainRoutingTable_entry {
    CDomain*	DestinationDomain;
    CDomain*	NextDomain;
    int		NHop;
};

/* Domain routing table contains for each network domain:
--- 1. Next hop domain;
--- 2. Edge Nodes; */
struct EdgeNodesTable_entry {
    CDomain*	NextDomain;
    CNode*		EdgeNode;
};

class CDomain {
public:
    int Id;
    string Name;

    CNet* ParentNet;

    vector<CNode*> Node;
    vector<CLink*> Link; //Outgoing links are considered part of the domain, incoming links are *not* 

    vector<CNode*> In_EdgeNode; //Edge node inside the domain
    vector<CNode*> Out_EdgeNode;//Edge node outside the domain
   
    vector<CLink*> InterDomain_OutLink;
    vector<CLink*> InterDomain_InLink;

    //Domain routing table
    vector<DomainRoutingTable_entry> DomainRoutingTable_db;

    //Domain edge nodes table
    vector<EdgeNodesTable_entry*> EdgeNodesTable_db;

    int PCE_Node_Id;
    PCx_Node* PCE_Node;

    //Topology Matrix of the domain, links toward external nodes are seen as uni-directional
    int G[NMAX][NMAX];

    CPathTable* PathTable;

    CDomain(int id, string name, PCx_Node* pce_node, vector<CNode*> domain_nodes, CNet* parent_net);
    ~CDomain();

    void RoutingTable_AddRecord(CDomain* dst_domain, CDomain* next_domain, int nhop);
    int DijkstraDistance(CNode* s, int DistanceTree[]);
    int InEdgeNode(CNode* node);
    int OutEdgeNode(CNode* node);
    void print();
};

#endif







