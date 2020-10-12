#include "CDomain.h"

#include "PCx_Node.h"
#include "CNet.h"

CDomain::CDomain(int id, string name, PCx_Node* pce_node, vector<CNode*> domain_nodes, CNet* parent_net) {
    this->Id = id;
    this->Name = name;
    this->ParentNet = parent_net;

    if (pce_node == NULL) {
        this->PCE_Node = NULL;
        this->PCE_Node_Id = -1;
    }
    else {
        this->PCE_Node = pce_node;
        this->PCE_Node_Id = pce_node->Id;
    }

    //Filling up the Domain nodes
    for (int i=0;i<domain_nodes.size();i++)
        this->Node.push_back(domain_nodes[i]);

    //Filling up the Domain links
    for (int i=0;i<domain_nodes.size();i++)
        for (int j=0;j<this->Node[i]->OutLink.size();j++)
            this->Link.push_back(this->Node[i]->OutLink[j]);

    //Writing the ParentDomain field of the Domain nodes
    for (int i=0;i<this->Node.size();i++)
        this->Node[i]->ParentDomain = this;

    //Writing the ParentDomain field of the Domain links
    for (int i=0;i<this->Link.size();i++)
        this->Link[i]->ParentDomain = this;

    //Filling up the InterDomain OutLinks
    for (int i=0;i<this->Node.size();i++) {
        for (int j=0;j<this->Node[i]->OutLink.size();j++)
            if (this->Node[i]->OutLink[j]->ToNode->ParentDomain != this) {
                this->InterDomain_OutLink.push_back(this->Node[i]->OutLink[j]);
            }
    }
    
    //Filling up the InterDomain InLinks
    for (int i=0;i<this->Node.size();i++) {
        for (int j=0;j<this->Node[i]->InLink.size();j++)
            if (this->Node[i]->InLink[j]->FromNode->ParentDomain != this) {
                this->InterDomain_InLink.push_back(this->Node[i]->InLink[j]);
            }
    }

    //Fill up the list of Out_Edge nodes (the gateway nodes in each next domain)
    for (int j=0; j<this->InterDomain_OutLink.size(); j++) {
        if (j==0) {
            this->Out_EdgeNode.push_back(this->InterDomain_OutLink[j]->ToNode);
        }
        else {
            int k=0;
            while ((this->InterDomain_OutLink[j]->ToNode!=this->Out_EdgeNode[k]) && (k<this->Out_EdgeNode.size())) k++;

            if (k==this->Out_EdgeNode.size()) {
                this->Out_EdgeNode.push_back(this->InterDomain_OutLink[j]->ToNode);
            }
        }
    }

    //Fill up the list of In_Edge nodes (the gateway nodes in this domain)
    for (int j=0; j<this->InterDomain_OutLink.size(); j++) {
        if (j==0) {
            this->In_EdgeNode.push_back(this->InterDomain_OutLink[j]->FromNode);
        }
        else {
            int k=0;
            while ((this->InterDomain_OutLink[j]->FromNode!=this->In_EdgeNode[k]) && (k<this->In_EdgeNode.size())) k++;

            if (k==this->In_EdgeNode.size()) {
                this->In_EdgeNode.push_back(this->InterDomain_OutLink[j]->FromNode);
            }
        }
    }


    //TODO Remove this table
    //Filling up the EdgeNodesTable
    for (int j=0; j<this->InterDomain_OutLink.size(); j++) {
        EdgeNodesTable_entry* entry= new EdgeNodesTable_entry;

        entry->EdgeNode = this->InterDomain_OutLink[j]->ToNode;
        entry->NextDomain = this->InterDomain_OutLink[j]->ToNode->ParentDomain;

        this->EdgeNodesTable_db.push_back(entry);
    }
}

CDomain::~CDomain() {
}

void CDomain::RoutingTable_AddRecord(CDomain* dst_domain, CDomain* next_domain, int nhop) {
    DomainRoutingTable_entry new_entry;

    new_entry.DestinationDomain = dst_domain;
    new_entry.NHop = nhop;
    new_entry.NextDomain = next_domain;

    this->DomainRoutingTable_db.push_back(new_entry);
}

/* Computes the distance in number of hops from s to each node in the network and write it in the DistanceTree */
int CDomain::DijkstraDistance(CNode* s, int DistanceTree[]) {
    int m, j=0, visited=0;
    int nodi=this->ParentNet->Node.size();

    vector<int> u;//Vettore dei predecessori
    vector<int> d;//Segna peso minimo con cui si raggiunge quel nodo
    vector<int> v;//Segna nodi visitati.....1 visitato 0 non visitato

    CLink* l;
    CNode* n;

    for (int i=0; i<nodi; i++) {
        u.push_back(MAX);
        d.push_back(MAX);
        v.push_back(0);
    }
    v[s->Id]=1;
    d[s->Id]=0;
    visited=1;

    //Sistemo il vector d per i nodi direttamente raggiungibili da s
    for (int i=0; i<s->OutLink.size(); i++) {
      if (s->OutLink[i]->Status==UP) {
        l=s->OutLink[i];
        n=s->OutLink[i]->ToNode;

        d[n->Id]=l->DistanceMetric;
        u[n->Id]=s->Id;
      }
    }

    do {
        m=MAX;
        for (int i=0; i<nodi; i++) //sceglie il prossimo nodo da visitare
            if (this->ParentNet->Node[i]->ParentDomain == this) { //visita solo nodi appartenenti al dominio
                if (!v[i] && d[i]<m) { //cercando l'indice di d con minimo d[i]
                    m=d[i];
                    j=i;
                }
            }
        v[j]=1;
        visited++;

        for (int i=0; i<this->ParentNet->Node[j]->OutLink.size(); i++) {
	  if (this->ParentNet->Node[j]->OutLink[i]->Status==UP) {
            l=this->ParentNet->Node[j]->OutLink[i];
            n=this->ParentNet->Node[j]->OutLink[i]->ToNode;

            if (d[n->Id] > d[j]+l->DistanceMetric) {
                d[n->Id]=d[j]+l->DistanceMetric;
                u[n->Id]=j;
            }
	  }
        }
    } while (visited < this->Node.size());

    #ifdef DEBUG
    cout << "Domain DijkstraDistances:";
    for (int k=0; k<nodi; k++)
        cout << " " << d[k];
    cout << endl;
    #endif

    for (int k=0; k<nodi; k++)
        DistanceTree[k]=d[k];

    return 1;
}

void CDomain::print() {
    cout << "-------------------------" << endl;
    cout << "Domain Id: " << this->Id << " name: " << this->Name << " pce: ";

    if (this->PCE_Node_Id == -1)
        cout << "NO_PCE";
    else
        cout << this->PCE_Node->Id;

    cout << " nodes: ";
    for (int i=0; i<this->Node.size();i++)
        cout << this->Node[i]->Id << " ";
    cout << endl;
    cout << "-------------------------" << endl;

    cout << "InterDomain Routing Table" << endl;
    for (int i=0;i<this->DomainRoutingTable_db.size(); i++) {
        cout << " To: " << this->DomainRoutingTable_db[i].DestinationDomain->Id;
        cout << " through: " << this->DomainRoutingTable_db[i].NextDomain->Id;
        cout << " cost: " << this->DomainRoutingTable_db[i].NHop << endl;
    }

    cout << "InterDomain OutLinks" << endl;
    for (int i=0;i<this->InterDomain_OutLink.size(); i++) {
        cout << " To node: " << this->InterDomain_OutLink[i]->ToNode->Name;
        cout << " of Domain: " << this->InterDomain_OutLink[i]->ToNode->ParentDomain->Name << endl;
    }

}

int CDomain::InEdgeNode(CNode* node) {
  for (int i=0; i<this->In_EdgeNode.size(); i++)
    if (this->In_EdgeNode[i]==node)
      return 1;
  return 0;
}

int CDomain::OutEdgeNode(CNode* node) {
  for (int i=0; i<this->Out_EdgeNode.size(); i++)
    if (this->Out_EdgeNode[i]==node)
      return 1;
  return 0;
}

