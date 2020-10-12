#include "CPathTable.h"
#include "Statistics.h"
#include "PCx_Node.h"
#include "CConnection.h"
#include "CDomain.h"
#include "CNet.h"

//Build the network CPathTable_HPCE using the function DepthFirstSearch_Network and the H matrix
CPathTable::CPathTable(CNet* Net) {
    vector<int> visited;
    vector<int> path;
    int last_table_size=0;
    int nodes=Net->Node.size();
    int logic_size=Net->Node.size()+Net->Domain.size();
    int MaxDistance;
    ifstream PathTest;

    this->Type=ABSTRACT_NETWORK;
    this->ParentNet=Net;
    this->ParentDomain=Net->HPCE_Node->ParentDomain;
    this->ParentNode=Net->HPCE_Node;
    this->FailureFlag=0;
    this->FileName="./paths/HPCE_"+this->ParentNet->NameS+".path";
    
    PathTest.open(this->FileName.c_str(),ios::in);
    if (!PathTest.is_open()) { //The .path file is not existing

        //Computes minimum distance between node pairs in number of hops
        this->LoadDistanceMatrix_HPCE();

	#ifdef STDOUT
	cout << endl << "Building a new CPathTable saved in file: " << this->FileName << endl;
	#endif
	
	#ifdef DEBUG
	for (int i=0; i<logic_size; i++) {
	  for (int j=0; j<logic_size; j++)
	    if (this->DistanceMatrix[i][j]==MAX) cout << "-1" << " ";
	    else cout << this->DistanceMatrix[i][j] << " ";
	  cout << endl;
	}
	#endif
	
        for (int s=0; s<nodes; s++) {
            for (int d=0; d<nodes; d++) {
                CNode* src=this->ParentNet->Node[s];
                CNode* dst=this->ParentNet->Node[d];
		MaxDistance=this->DistanceMatrix[nodes+src->ParentDomain->Id][nodes+dst->ParentDomain->Id];

		last_table_size=this->table.size();
                this->index.push_back(this->table.size());

                if ((src->ParentDomain!=dst->ParentDomain) && (!src->ParentDomain->OutEdgeNode(dst))) {
                    //Initializing visited vector
                    for (int i=0; i<logic_size; i++)
                        visited.push_back(0);

                    //Populates the ltable
                    this->DepthFirstSearch_HPCE(src->Id,dst->Id,src,dst,0,0,MaxDistance,visited,path,this->table);
                }
                
                //Fill the PathNumber field
		for (int i=last_table_size; i<this->table.size(); i++) {
		  this->table[i]->PathNumber=(this->table.size()-last_table_size);
		}
            }
        }
        this->save(logic_size);
    }
    else { //The .path file is existing --> load the paths
        this->load(logic_size);
    }

    #ifdef DEBUG
    cout << endl << " --- HPCE PathTable --- " << endl;
    this->print(logic_size);
    #endif
}

//Build the CNet network CPathTable using the function DepthFirstSearch_Network
CPathTable::CPathTable(CNet* Net, int add_hops, int FailureFlag) {
    vector<int> visited;
    vector<CNode*> path;
    int last_table_size=0;
    ifstream PathTest;
    int size=Net->Node.size();

    this->Type=REAL_NETWORK;
    this->ParentNet=Net;
    this->ParentDomain=Net->HPCE_Node->ParentDomain;
    this->ParentNode=Net->HPCE_Node;
    this->FailureFlag=FailureFlag;
    this->FileName="./paths/CNet_"+this->ParentNet->NameS+".path";

    PathTest.open(this->FileName.c_str(),ios::in);
    if (!PathTest.is_open()) { //The .path file is not existing
      
	#ifdef STDOUT
	cout << endl << "Building a new CPathTable saved in file: " << this->FileName << endl;
	#endif

        //Computes minimum distance between node pairs in number of hops
        this->LoadDistanceMatrix_Network();

        for (int s=0; s<size; s++) {
            for (int d=0; d<size; d++) {
                CNode* src = this->ParentNet->Node[s];
                CNode* dst = this->ParentNet->Node[d];

		last_table_size=this->table.size();
                this->index.push_back(this->table.size());

                if (s!=d) {
                    //Initializing visited vector
                    for (int i=0; i<this->ParentNet->Node.size(); i++)
                        visited.push_back(0);

                    //Populates the ltable
                    this->DepthFirstSearch_Network(src, dst, 0, 0, add_hops + this->DistanceMatrix[src->Id][dst->Id], visited, path, this->table);
		    
		    //Fill the PathNumber field
		    for (int i=last_table_size; i<this->table.size(); i++) {
		      this->table[i]->PathNumber=(this->table.size()-last_table_size);
		    }
                }
            }
        }
        this->save(size);
    }
    else { //The .path file is existing --> load the paths
        this->load(size);
    }

    #ifdef DEBUG
    cout << endl << " --- Network PathTable --- " << endl;
    this->print(size);
    #endif
}

//Build a domain CPathTable using the function DepthFirstSearch_Domain. This is the CPathTable used by the Child PCEs
CPathTable::CPathTable(CNet* Net, CDomain* LocalDomain, int add_hops, int FailureFlag) {
    vector<int> visited;
    vector<CNode*> path;
    ifstream PathTest;
    int node_size=Net->Node.size();

    this->Type=REAL_DOMAIN;
    this->ParentNet=Net;
    this->ParentDomain=LocalDomain;
    this->ParentNode=LocalDomain->PCE_Node;
    this->FailureFlag=FailureFlag;
    this->FileName="./paths/CDomain_"+this->ParentNet->NameS+"_"+this->ParentDomain->Name+".path";

    
    PathTest.open(this->FileName.c_str(),ios::in);
    if (!PathTest.is_open()) { //The .path file is not existing
      
	#ifdef STDOUT
	cout << endl << "Building a new CPathTable saved in file: " << this->FileName << endl;
	#endif

        //Computes minimum distance between node pairs in number of hops
        this->LoadDistanceMatrix_Domain();

        for (int s=0; s<this->ParentNet->Node.size(); s++) {
            for (int d=0; d<this->ParentNet->Node.size(); d++) {
                CNode* src = this->ParentNet->Node[s];
                CNode* dst = this->ParentNet->Node[d];

                this->index.push_back(this->table.size());

                if (src->ParentDomain == this->ParentDomain) {
                    if (this->DistanceMatrix[src->Id][dst->Id] < MAX) {
                        if (s!=d) {
                            //Initializing visited vector
                            for (int i=0; i<this->ParentNet->Node.size(); i++)
                                visited.push_back(0);

                            //Populates the ltable
                            this->DepthFirstSearch_Domain(src, dst, 0, 0, add_hops + this->DistanceMatrix[src->Id][dst->Id], visited, path, this->table);
                        }
                    }
                }
            }
        }
        this->save(node_size);
    }
    else { //The .path file is existing --> load the paths
        this->load(node_size);
    }

    #ifdef DEBUG
    cout << endl << " --- Domain PathTable: " << this->ParentDomain->Name << " --- " << endl;
    this->print(node_size);
    #endif
}

//Build a node CPathTable filtering a bigger CPathTable (i.e., the domain CPathTable)
CPathTable::CPathTable(CNet* Net,CNode* LocalNode,CPathTable* CentralTable) {
    CNode* s;

    this->Type=REAL_NODE;
    this->ParentNet=Net;
    this->ParentNode=LocalNode;
    this->ParentDomain=LocalNode->ParentDomain;
    this->FailureFlag=CentralTable->FailureFlag;

    for (int i=0; i<CentralTable->table.size();i++) {
        s=CentralTable->table[i]->Src;
	
        if (LocalNode==s) {
            Path* P=new Path;
            P->Src=CentralTable->table[i]->Src;
            P->Dst=CentralTable->table[i]->Dst;
            P->ParentDomain=P->Src->ParentDomain;
            P->nodes=CentralTable->table[i]->nodes;
            P->links=CentralTable->table[i]->links;
            P->Id=CentralTable->table[i]->Id;
	    
            this->table.push_back(P);
        }
    }

    //Completing the index
    int current_entry, last_entry=-1;
    for (int i=0; i<this->ParentNet->Node.size(); i++)
        for (int j=0; j<this->ParentNet->Node.size(); j++)
            this->index.push_back(0);
    for (int i=0; i<this->table.size(); i++) {
        current_entry=this->table[i]->Src->Id*this->ParentNet->Node.size()+this->table[i]->Dst->Id;
        if (current_entry!=last_entry) {
	  this->index[current_entry]=i;
	  last_entry=current_entry;
	}
    }

    #ifdef DEBUG
    cout << endl << " --- Node PathTable: " << this->ParentNode->Id << " --- " << endl;
    this->print(this->table.size());
    #endif
}

//Builds the PathTable including a Failure to be used in CDomain
CPathTable::CPathTable(CNet* Net, CLink* failed_1, CLink* failed_2, int add_hops, int FailureFlag) {
    vector<int> visited;
    vector<CNode*> path;
    int last_table_size=0;
    ifstream PathTest;
    int size=Net->Node.size();
    int failed_link_id;
    string failed_link;

    this->Type=FAIL_NETWORK;
    this->ParentNet=Net;
    this->ParentDomain=failed_1->FromNode->ParentDomain;
    this->ParentNode=this->ParentDomain->PCE_Node;
    this->FailureFlag=FailureFlag;
    
    if (failed_1->Id < failed_2->Id) failed_link_id=failed_1->Id;
    else failed_link_id=failed_2->Id;
    IntegerToString(failed_link_id,&failed_link);
    
    this->FileName="./paths/Failure_" + this->ParentNet->NameS + "_" + this->ParentDomain->Name + "_" + failed_link + ".path";
    
    PathTest.open(this->FileName.c_str(),ios::in);
    if (!PathTest.is_open()) { //The .path file is not existing
      
	#ifdef STDOUT
	cout << endl << "Building a new CPathTable saved in file: " << this->FileName << endl;
	#endif

        //Computes minimum distance between node pairs in number of hops
        this->LoadDistanceMatrix_Domain();
	
        for (int s=0; s<size; s++) {
            for (int d=0; d<size; d++) {
                CNode* src = this->ParentNet->Node[s];
                CNode* dst = this->ParentNet->Node[d];

		last_table_size=this->table.size();
                this->index.push_back(this->table.size());

                if (s!=d) {
                    //Initializing visited vector
                    for (int i=0; i<this->ParentNet->Node.size(); i++)
                        visited.push_back(0);

                    //Populates the ltable
                    this->DepthFirstSearch_Domain(src, dst, 0, 0, add_hops + this->DistanceMatrix[src->Id][dst->Id], visited, path, this->table);
		    
		    //Fill the PathNumber field
		    for (int i=last_table_size; i<this->table.size(); i++) {
		      this->table[i]->PathNumber=(this->table.size()-last_table_size);
		    }
                }
            }
        }
        this->save(size);
    }
    else { //The .path file is existing --> load the paths
        this->load(size);
    }

    #ifdef DEBUG
    cout << endl << " --- Failure PathTable --- " << endl;
    this->print(this->table.size());
    #endif
};

//Builds the PathTable including a Failure to be used at the CNet level
CPathTable::CPathTable(CNet* Net, CLink* failed_1, CLink* failed_2, int add_hops, int FailureFlag, int cnet_flag) {
    vector<int> visited;
    vector<CNode*> path;
    int last_table_size=0;
    ifstream PathTest;
    int size=Net->Node.size();
    int failed_link_id;
    string failed_link;
    
    this->Type=FAIL_NETWORK;
    this->ParentNet=Net;
    this->ParentDomain=Net->HPCE_Node->ParentDomain;;
    this->ParentNode=Net->HPCE_Node;;
    this->FailureFlag=FailureFlag;
    
    if (failed_1->Id < failed_2->Id) failed_link_id=failed_1->Id;
    else failed_link_id=failed_2->Id;
    IntegerToString(failed_link_id,&failed_link);
    
    this->FileName="./paths/Failure_" + this->ParentNet->NameS + "_CNet_" + failed_link + ".path";
    
    PathTest.open(this->FileName.c_str(),ios::in);
    if (!PathTest.is_open()) { //The .path file is not existing
      
	#ifdef STDOUT
	cout << endl << "Building a new CPathTable saved in file: " << this->FileName << endl;
	#endif

        //Computes minimum distance between node pairs in number of hops
        this->LoadDistanceMatrix_Network();
	
        for (int s=0; s<size; s++) {
            for (int d=0; d<size; d++) {
                CNode* src = this->ParentNet->Node[s];
                CNode* dst = this->ParentNet->Node[d];
		
		last_table_size=this->table.size();
                this->index.push_back(this->table.size());

                if (s!=d) { 
                    //Initializing visited vector
                    for (int i=0; i<this->ParentNet->Node.size(); i++)
                        visited.push_back(0);

                    //Populates the ltable
                    this->DepthFirstSearch_Network(src, dst, 0, 0, add_hops + this->DistanceMatrix[src->Id][dst->Id], visited, path, this->table);
		    
		    //Fill the PathNumber field
		    for (int i=last_table_size; i<this->table.size(); i++) {
		      this->table[i]->PathNumber=(this->table.size()-last_table_size);
		    }
                }
            }
        }
        this->save(size);
    }
    else { //The .path file is existing --> load the paths
        this->load(size);
    }

    #ifdef DEBUG
    cout << endl << " --- Failure PathTable --- " << endl;
    this->print(this->table.size());
    #endif
};


CPathTable::~CPathTable() {
  //cout << "Destroying CPathTable " << this->Type << " flag " << this->FailureFlag << " " << this->ParentNode->Id << " " << this->ParentDomain->Id << " " << this->ParentNet->NameS << endl; 
  
  for (int i=0;i<this->table.size();i++)
        delete this->table[i];

  this->table.clear();
  this->index.clear();
}

void CPathTable::load(int dim_distance_matrix) {
    char a;
    int b,n,counter=0;
    ifstream f;
    CNode *s, *d, *ActualNode, *NextNode;
    CLink* ActualLink;
    int PathsSD, MaxPathsSD=101;

    #ifdef STDOUT
    cout << endl << "Loading from file: " << this->FileName << endl;
    #endif
    
    f.open(this->FileName.c_str(),ios::in);
    if (!f.is_open()) ErrorMsg("CPathTable .path file not found");

    //Loading DistanceMatrix
    for (int i=0;i<NMAX;i++)
      for (int j=0;j<NMAX;j++)
        this->DistanceMatrix[i][j]=MAX;
    
    f >> a;
    for (int i=0;i<dim_distance_matrix;i++)
      for (int j=0;j<dim_distance_matrix;j++)
        f >> this->DistanceMatrix[i][j];
    
    //Loading key
    f >> a;
    if (a=='K') {
        f >> b;
        while (b!=-1) {
            this->index.push_back(b);
            f >> b;
        }
    }

    //Loading paths
    f >> a;
    while (f) {
        while (a!='F') {
            if (a=='#') {
                PathsSD=0;
                f >> b;
                s=ParentNet->GetNodePtrFromNodeID(b);
                f >> b;
                d=ParentNet->GetNodePtrFromNodeID(b);
                f >> b;
                f >> b;
                n=b;
                f >> a;
            }

            if (a=='P') {
                Path* P=new Path;
                P->PathNumber=n;
                P->Src=s;
                P->Dst=d;
                P->ParentDomain=P->Src->ParentDomain;
                f >> b;
                while (b!=d->Id) {
                    ActualNode=this->ParentNet->GetNodePtrFromNodeID(b);
                    P->nodes.push_back(ActualNode);
                    f >> b;
                }
                ActualNode=this->ParentNet->GetNodePtrFromNodeID(b);
                P->nodes.push_back(ActualNode);
                P->Id=counter;
                counter++;

                table.push_back(P);
                PathsSD++;

		f >> a;
            }
        }
        f >> a;
    }

    //Loading links
    for (int i=0; i<this->table.size(); i++) {
        for (int j=0; j<this->table[i]->nodes.size()-1; j++) {
            ActualNode=this->table[i]->nodes[j];
            NextNode=this->table[i]->nodes[j+1];
            if (this->Type==ABSTRACT_NETWORK) ActualLink=ActualNode->GetInterDomainLinkToNode(NextNode);
            else ActualLink=ActualNode->GetLinkToNode(NextNode);
            this->table[i]->links.push_back(ActualLink);
        }
    }
}

void CPathTable::save(int dim_distance_matrix) {
    ofstream output_file;
    int s=table[0]->Src->Id;
    int d=table[0]->Dst->Id;

    output_file.open(this->FileName.c_str(),ios::out);
    
    //Saving DistanceMatrix
    output_file << "D" << endl;
    for (int i=0;i<dim_distance_matrix;i++) {
      for (int j=0;j<dim_distance_matrix;j++)
        output_file << this->DistanceMatrix[i][j] << " ";
    output_file << endl;
    }

    //Saving Index
    output_file << endl << "K ";
    for (int i=0; i<this->index.size(); i++)
        output_file << this->index[i] << " ";
    output_file << endl << "-1" << endl << endl;

     //Saving Table
    output_file << "# " << table[0]->Src->Id << " " << table[0]->Dst->Id << " 0 \t" << table[0]->PathNumber << endl;
    for (int i=0; i<this->table.size(); i++) {
        if ((s!=table[i]->Src->Id) || (d!=table[i]->Dst->Id)) {
            output_file << "F" << endl << endl;
            output_file << "# " << table[i]->Src->Id << " " << table[i]->Dst->Id << " 0 \t" << table[i]->PathNumber << endl;
        }

        output_file << "P ";
        for (int j=0; j<this->table[i]->nodes.size(); j++)
            output_file << this->table[i]->nodes[j]->Id << " ";
        output_file << endl;

        s=table[i]->Src->Id;
        d=table[i]->Dst->Id;
    }
    output_file << "F" << endl;
}

void CPathTable::print(int dim_distance_matrix) {
  /*
  cout << "DistanceMatrix: " << endl;
  for (int i=0;i<dim_distance_matrix;i++) {
    for (int j=0;j<dim_distance_matrix;j++) {
      if (this->DistanceMatrix[i][j]==MAX) cout << "M ";
      else cout << this->DistanceMatrix[i][j] << " ";
    }
    cout << endl;
  }
  */
  
  /*
  cout << "Index: " << flush;
  for (int i=0; i<this->index.size(); i++)
    cout << this->index[i] << " ";
  cout << endl;
  */
  
  cout << "Table size: " << this->table.size() << endl;
  for (int i=0; i<this->table.size(); i++) {
    cout << i << " ID: " << table[i]->Id << " (" << table[i]->Src->Id << "," << table[i]->Dst->Id << ")";
    cout << " nodes: ";
    for (int j=0; j<this->table[i]->nodes.size(); j++)
      cout << this->table[i]->nodes[j]->Id << " ";
    cout << " links: ";
    for (int j=0; j<this->table[i]->links.size(); j++) {
      if (this->table[i]->links[j]!=NULL) cout << this->table[i]->links[j]->Id << " ";
      else cout << "NULL ";
    }
    cout << endl;
  }
}

int CPathTable::ComputeCandidatePaths(CNode* s, CNode* d, vector<Path*>& ltable, CConnection* conn) {
    int min_hop_distance=MAX;
    int hop_distance;
    int table_entry;

    Path* CurrentPath;
    CNode* EdgeNode;

    //------------------------
    //Interdomain destinations
    //------------------------
    if (this->ParentDomain->PathTable->DistanceMatrix[s->Id][d->Id]==MAX) {
	#ifdef STDOUT_CPATHTABLE
        cout << "ComputeCandidatePaths conn id " << conn->Id << " INTER-DOMAIN computing path " << s->Id << "->" << d->Id << endl;
	#endif

        int index = 0;

        CDomain* NextDomain;
        CDomain* ActualDomain=s->ParentDomain;
        CDomain* DestinationDomain=d->ParentDomain;

        while ((DestinationDomain != ActualDomain->DomainRoutingTable_db[index].DestinationDomain) && (index < ActualDomain->DomainRoutingTable_db.size())) index++;
        if (index == ActualDomain->DomainRoutingTable_db.size())
            ErrorMsg("CNet::DomainLevel_RoutingScheme()... No way to the destination domain.");
        NextDomain = ActualDomain->DomainRoutingTable_db[index].NextDomain;
	//cout << "NextDomain: " << NextDomain->Name << endl;

        //Computes the minimum hop distance between s and the closest edge node
        for (index=0; index<ActualDomain->Out_EdgeNode.size(); index++) {
            EdgeNode = ActualDomain->Out_EdgeNode[index];
            if (EdgeNode->ParentDomain == NextDomain) {
                hop_distance=ActualDomain->PathTable->DistanceMatrix[s->Id][EdgeNode->Id];

                //cout << "EdgeNode: " << EdgeNode->Id << " distance " << hop_distance << endl;
                if (hop_distance < min_hop_distance)
                    min_hop_distance = hop_distance;
            }
        }
        //cout << " min_hop_distance " << min_hop_distance << endl;

        //Fill up the ltable with all the paths within min_hop_distance + net->RoutingHopThreshold
        for (index=0; index<ActualDomain->Out_EdgeNode.size(); index++) {
            EdgeNode=ActualDomain->Out_EdgeNode[index];
            table_entry=this->index[s->Id*this->ParentNet->Node.size()+EdgeNode->Id];
            CurrentPath=this->table[table_entry];

            while ((table_entry<this->table.size()) && ((CurrentPath->links.size()-1+100)<=(min_hop_distance+this->ParentNet->RoutingHopThreshold_PCC)) && (EdgeNode->ParentDomain==NextDomain) && (s==CurrentPath->Src) && (EdgeNode==CurrentPath->Dst)) {
                ltable.push_back(CurrentPath);

                table_entry++;
                CurrentPath=this->table[table_entry];
            }
        }

	#ifdef STDOUT_CPATHTABLE
        for (int i=0; i<ltable.size(); i++) {
            cout << "Candidate path for conn id " << conn->Id << " : ";
            for (int j=0; j<ltable[i]->nodes.size(); j++)
                cout << ltable[i]->nodes[j]->Id << " ";
            cout << endl;
        }
	#endif

        if (ltable.size() > 0) return 1;
        else ErrorMsg("ComputeCandidatePaths: No Inter-Domain candidate path");
    }

    //------------------------
    //Intradomain destinations
    //------------------------
    #ifdef STDOUT_CPATHTABLE
    cout << "ComputeCandidatePaths conn id " << conn->Id << " INTRA-DOMAIN computing path " << s->Id << "->" << d->Id;
    cout << " distance: " << this->ParentDomain->PathTable->DistanceMatrix[s->Id][d->Id];
    cout << " table_entry: " << s->Id*this->ParentNet->Node.size()+d->Id << " table_content " << this->index[s->Id*this->ParentNet->Node.size()+d->Id] << endl;
    #endif
    
    //Builds ltable starting from the node s table and just including paths from node s to node d
    //Compute the entry of the table where the proper paths are located
    table_entry=this->index[s->Id*this->ParentNet->Node.size()+d->Id];
    CurrentPath=this->table[table_entry];

    //Builds ltable starting from the CNet->PathTable_HPCE including node s as source and node d as destination
    while ((table_entry<this->table.size()) && (s==CurrentPath->Src) && (d==CurrentPath->Dst)) {
        ltable.push_back(CurrentPath);

        table_entry++;
        CurrentPath=this->table[table_entry];
    }

    #ifdef STDOUT_CPATHTABLE
    for (int i=0; i<ltable.size(); i++) {
        cout << "Candidate path for conn id " << conn->Id << " : ";
        for (int j=0; j<ltable[i]->nodes.size(); j++)
            cout << ltable[i]->nodes[j]->Id << " ";
        cout << endl;
    }
    #endif

    if (ltable.size() > 0) return 1;
    else ErrorMsg("ComputeCandidatePaths: No Intra-Domain candidate path");
}

//Returns 1 if there is at least one path between s and d, otherwise return 0.
//The utilized seed is initialized by CNet costructor
int CPathTable::NoInfo_RandomPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, CConnection* rconn) {
    vector<Path*> ltable; //local database of paths between nodes s and d
    int rand_id;

#ifdef STDOUT_CPATHTABLE
    cout << "NoInfo conn id " << rconn->Id << " computing path " << s->Id << "->" << d->Id << endl;
#endif

    //Computes the set of candidate paths and inserts them in the ltable
    this->ComputeCandidatePaths(s,d,ltable,rconn);

    //Randomly selects one of the paths in the ltable
    //this->ParentNet->TableSize->AddSample_Distribution(ltable.size());
    //this->ParentNet->TableSize->AddSample(ltable.size());

    if (ltable.size()) {
        rand_id=randint(0,(ltable.size()-1),&this->ParentNet->PathSelectionSeed);
        rconn->ActualPath.push_back(ltable[rand_id]);
        for (int j=0; j<(ltable[rand_id]->nodes.size()); j++)
            pathnode->push_back(ltable[rand_id]->nodes[j]);
        for (int j=0; j<(ltable[rand_id]->links.size()); j++)
            pathlink->push_back(ltable[rand_id]->links[j]);
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    return 1;
}

int CPathTable::AggInfo_LeastCongestedPath(CNode* s, CNode* d, std::vector< CNode* >* pathnode, std::vector< CLink* >* pathlink, CConnection* rconn) {
    vector<Path*> ltable, lltable;   //local database of paths between nodes s and d
    vector<int> ltable_weight;
    Path* ActualPath;
    int j, PathBottleneck, LinkBottleneck;
    int MaxWeight, rand_id, lambdas=this->ParentNet->W;

    #ifdef STDOUT_CPATHTABLE
    cout << "AggInfo_LeastCongestedPath conn id " << rconn->Id << " computing path " << s->Id << "->" << d->Id << endl;
    #endif

    //Computes the set of candidate paths and inserts them in the ltable
    this->ComputeCandidatePaths(s,d,ltable,rconn);

    //Initializes ltable_weight
    for (int i=0; i<ltable.size(); i++)
        ltable_weight.push_back(0);

    //Computes the ltable_weight based on the LinkState_dB of the node (source node or PCE node)
    if (ltable.size()) {
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {

            ActualPath=ltable[lt_id];
            PathBottleneck=lambdas;
            LinkBottleneck=0;

            //Check the first link availability in the local OutgoingLink_dB
            j=s->SearchLocalLink(ActualPath->links[0],s->OutgoingLink_dB);
            for (int k=0; k<lambdas; k++) {
                LinkBottleneck+=(1-s->OutgoingLink_dB[j]->LocalLambdaStatus[k]);
            }
            if (LinkBottleneck<PathBottleneck) PathBottleneck=LinkBottleneck;

	    //Check the other links availability in the OSPF LinkState_dB i=1
            for (int i=1; i<ActualPath->links.size(); i++) {
                LinkBottleneck=0;

                j=0;
                while (ActualPath->links[i]!=s->LinkState_dB[j]->Link) j++;
                for (int k=0; k<lambdas; k++) {
                    LinkBottleneck+=(1-s->LinkState_dB[j]->LambdaStatus[k]);
                }
                if (LinkBottleneck<PathBottleneck) PathBottleneck=LinkBottleneck;
                //cout << "CONN ID " << rconn->Id << " LinkBottleneck [" << i << "]: " << LinkBottleneck << endl;
            }
            ltable_weight[lt_id]=PathBottleneck;
        }
	#ifdef STDOUT_CPATHTABLE
        cout << "AggInfo_LeastCongestedPath conn id " << rconn->Id << " weights: " << flush;
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            cout << ltable_weight[lt_id] << " ";
        }
	#endif
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    //Computes the MaxWeight
    MaxWeight=ltable_weight[0];
    for (int lt_id=1; lt_id<ltable.size(); lt_id++)
        if (ltable_weight[lt_id]>MaxWeight) MaxWeight=ltable_weight[lt_id];

    #ifdef STDOUT_CPATHTABLE
    cout << " max: " << MaxWeight << endl;
    #endif

    if (MaxWeight==0) return 0;

    //Builds lltable starting from ltable
    for (int i=0; i<ltable.size(); i++)
        if (ltable_weight[i]==MaxWeight)
            lltable.push_back(ltable[i]);

    #ifdef STDOUT_CPATHTABLE
    cout << "AggInfo conn id " << rconn->Id << " LeastCongested lltable size: " << lltable.size() << flush;
    #endif

    //Statistic collection
    //this->ParentNet->TableSize->AddSample_Distribution(lltable.size());
    //this->ParentNet->TableSize->AddSample(lltable.size());

    if (lltable.size()) {
        rand_id=randint(0,(lltable.size()-1),&this->ParentNet->PathSelectionSeed);
	
	#ifdef STDOUT_CPATHTABLE
        cout << " random_id: " << rand_id << endl;
	#endif
	
        rconn->ActualPath.push_back(lltable[rand_id]);
        for (int j=0; j<(lltable[rand_id]->nodes.size()); j++)
            pathnode->push_back(lltable[rand_id]->nodes[j]);
        for (int j=0; j<(lltable[rand_id]->links.size()); j++)
            pathlink->push_back(lltable[rand_id]->links[j]);
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }
    return 1;
}

int CPathTable::Flexible_DetInfo_LeastCongestedPath(CNode* s, CNode* d, vector< CNode* >* pathnode, vector< CLink* >* pathlink, CConnection* rconn) {
    vector<Path*> ltable, lltable;   //local database of paths between nodes s and d
    vector<int> ltable_weight;
    Path* ActualPath;
    int LabelSetTest, MaxWeight, rand_id, lambdas=this->ParentNet->W, j;
    int VirtualLabelSet[lambdas];
    int required_slots=rconn->width_slots;

    for (int i=0; i<lambdas; i++) VirtualLabelSet[i]=1;

    #ifdef STDOUT_CPATHTABLE
    cout << "DetInfo_LeastCongestedPath conn id " << rconn->Id << " computing path " << s->Id << "->" << d->Id << " TIME: " << this->ParentNet->Now << endl;
    #endif

    //Computes the set of candidate paths and inserts them in the ltable
    this->ComputeCandidatePaths(s,d,ltable,rconn);

    //Initializes ltable_weight
    for (int i=0; i<ltable.size(); i++)
        ltable_weight.push_back(0);

    //Computes the ltable_weight based on the LinkState_dB of the node s
    if (ltable.size()) {
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            ActualPath=ltable[lt_id];
            for (int i=0; i<lambdas; i++) VirtualLabelSet[i]=1;

            //Check the first link availability in the local OutgoingLink_dB
            j=s->SearchLocalLink(ActualPath->links[0],s->OutgoingLink_dB);
            for (int k=0; k<lambdas; k++) {
                if ((VirtualLabelSet[k]==1) && (s->OutgoingLink_dB[j]->LocalLambdaStatus[k]==1)) {
                    VirtualLabelSet[k]=0;
                }
            }
            //Check the other links availability in the OSPF LinkState_dB i=1
            for (int i=1; i<ActualPath->links.size(); i++) {
                int j=0;
                while (ActualPath->links[i]!=s->LinkState_dB[j]->Link) j++;
                for (int k=0; k<lambdas; k++) {
                    if ((VirtualLabelSet[k]==1) && (s->LinkState_dB[j]->LambdaStatus[k]==1)) VirtualLabelSet[k]=0;
                }
            }
            
            //Compute the weights considering the slots required by the connection
            for (int i=0; i<=lambdas-required_slots; i++) {
	      LabelSetTest=0;
	      for (int k=0;k<required_slots;k++) {
		if(VirtualLabelSet[i+k]==1)
		  LabelSetTest++;
	      }
	      if (LabelSetTest==required_slots)
		ltable_weight[lt_id]++;
	    }
        }
        
	#ifdef STDOUT_CPATHTABLE
        cout << "DetInfo_LeastCongestedPath conn id " << rconn->Id << " weights: " << flush;
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            cout << ltable_weight[lt_id] << " ";
        }
	#endif
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    //Computes the MaxWeight
    MaxWeight=ltable_weight[0];
    for (int lt_id=1; lt_id<ltable.size(); lt_id++)
        if (ltable_weight[lt_id]>MaxWeight) MaxWeight=ltable_weight[lt_id];

    #ifdef STDOUT_CPATHTABLE
    cout << " max: " << MaxWeight << endl;
    #endif

    if (MaxWeight==0) return 0;

    //Builds lltable starting from ltable
    for (int i=0; i<ltable.size(); i++)
        if (ltable_weight[i]==MaxWeight)
            lltable.push_back(ltable[i]);

    #ifdef STDOUT_CPATHTABLE
    cout << "DetInfo_LeastCongestedPath conn id " << rconn->Id << " lltable size: " << lltable.size() << flush;
    #endif

    //Randomly selects one of the least congested paths in the lltable
    //Randomly selects one of the paths in the ltable
    //this->ParentNet->TableSize->AddSample_Distribution(lltable.size());
    //this->ParentNet->TableSize->AddSample(lltable.size());

    if (lltable.size()) {
        rand_id=randint(0,(lltable.size()-1),&this->ParentNet->PathSelectionSeed);
	
	#ifdef STDOUT_CPATHTABLE
        cout << " random_id: " << rand_id << endl;
	#endif
	
	rconn->ActualPath.clear();
	rconn->ActualPath.push_back(lltable[rand_id]);
	
        for (int j=0; j<(lltable[rand_id]->nodes.size()); j++)
            pathnode->push_back(lltable[rand_id]->nodes[j]);
        for (int j=0; j<(lltable[rand_id]->links.size()); j++)
            pathlink->push_back(lltable[rand_id]->links[j]);
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }
    return 1;
}

int CPathTable::PCE_ComputeCandidatePaths(CNode* s, CNode* d, vector<Path*>& ltable, CConnection* conn) {
    int min_hop_distance=MAX;
    int hop_distance;
    int table_entry;

    Path* CurrentPath;
    CNode* EdgeNode;

    //------------------------
    //Interdomain destinations
    //------------------------
    if (this->ParentDomain->PathTable->DistanceMatrix[s->Id][d->Id]==MAX) {
	#ifdef STDOUT_CPATHTABLE
        cout << "ComputeCandidatePaths conn id " << conn->Id << " INTER-DOMAIN computing path " << s->Id << "->" << d->Id << endl;
	#endif

        int index = 0;

        CDomain* NextDomain;
        CDomain* ActualDomain=s->ParentDomain;
        CDomain* DestinationDomain=d->ParentDomain;

	//Find out the next domain using static "BGP" table DomainRoutingTable_db
        while ((DestinationDomain != ActualDomain->DomainRoutingTable_db[index].DestinationDomain) && (index < ActualDomain->DomainRoutingTable_db.size())) index++;
        if (index == ActualDomain->DomainRoutingTable_db.size())
            ErrorMsg("CNet::DomainLevel_RoutingScheme()... No way to the destination domain.");
        NextDomain = ActualDomain->DomainRoutingTable_db[index].NextDomain;
	//cout << "NextDomain: " << NextDomain->Name << endl;

        //Computes the minimum hop distance between s and the closest edge node
        for (index=0; index<ActualDomain->Out_EdgeNode.size(); index++) {
            EdgeNode = ActualDomain->Out_EdgeNode[index];
            if (EdgeNode->ParentDomain == NextDomain) {
                hop_distance=ActualDomain->PathTable->DistanceMatrix[s->Id][EdgeNode->Id];

                //cout << "EdgeNode: " << EdgeNode->Id << " distance " << hop_distance << endl;
                if (hop_distance < min_hop_distance)
                    min_hop_distance = hop_distance;
            }
        }
        //cout << " min_hop_distance " << min_hop_distance << endl;

        //Fill up the ltable with all the paths within min_hop_distance + net->RoutingHopThreshold_PCE
        for (index=0; index<ActualDomain->Out_EdgeNode.size(); index++) {
            EdgeNode=ActualDomain->Out_EdgeNode[index];
            table_entry=this->index[s->Id*this->ParentNet->Node.size()+EdgeNode->Id];
            CurrentPath=this->table[table_entry];
	    
	    //cout << "looking for path toward edge_node: " << EdgeNode->Id << " at index: " << table_entry << " current_path_dst: " << CurrentPath->nodes[CurrentPath->nodes.size()-1]->Id << endl;
	    
            while ((table_entry<this->table.size()) && ((CurrentPath->links.size()-1+100)<=(min_hop_distance+this->ParentNet->RoutingHopThreshold_PCE)) && (EdgeNode->ParentDomain==NextDomain) && (s==CurrentPath->Src) && (EdgeNode==CurrentPath->Dst)) {
                ltable.push_back(CurrentPath);

                table_entry++;
                CurrentPath=this->table[table_entry];
            }
        }

	#ifdef STDOUT_CPATHTABLE
        for (int i=0; i<ltable.size(); i++) {
            cout << "Candidate path for conn id " << conn->Id << " : ";
            for (int j=0; j<ltable[i]->nodes.size(); j++)
                cout << ltable[i]->nodes[j]->Id << " ";
            cout << endl;
        }
	#endif

        if (ltable.size() > 0) return 1;
        else ErrorMsg("PCE_ComputeCandidatePaths: No Inter-Domain candidate path");
    }

    //------------------------
    //Intradomain destinations
    //------------------------
    #ifdef STDOUT_CPATHTABLE
    cout << "ComputeCandidatePaths conn id " << conn->Id << " INTRA-DOMAIN computing path " << s->Id << "->" << d->Id;
    cout << " distance: " << this->ParentDomain->PathTable->DistanceMatrix[s->Id][d->Id];
    cout << " key_entry: " << s->Id*this->ParentNet->Node.size()+d->Id << " table_entry: " << this->index[s->Id*this->ParentNet->Node.size()+d->Id] << endl;
    #endif
    
    //Builds ltable starting from the node s table and just including paths from node s to node d
    //Compute the entry of the table where the proper paths are located
    table_entry=this->index[s->Id*this->ParentNet->Node.size()+d->Id];
    CurrentPath=this->table[table_entry];

    //Builds ltable starting from the CNet->PathTable_HPCE including node s as source and node d as destination
    while ((table_entry<this->table.size()) && (s==CurrentPath->Src) && (d==CurrentPath->Dst)) {
        ltable.push_back(CurrentPath);

        table_entry++;
        CurrentPath=this->table[table_entry];
    }

    #ifdef STDOUT_CPATHTABLE
    for (int i=0; i<ltable.size(); i++) {
        cout << "Candidate path for conn id " << conn->Id << " : ";
        for (int j=0; j<ltable[i]->nodes.size(); j++)
            cout << ltable[i]->nodes[j]->Id << " ";
        cout << endl;
    }
    #endif

    if (ltable.size() > 0) return 1;
    else {
      cout << table_entry << " " << CurrentPath->Id << " " << CurrentPath->Src->Id << "->" << CurrentPath->Dst->Id << endl;
      cout << "NODE: " << this->ParentNode->Id << " conn id " << conn->Id << " " << s->Id << "->" << d->Id << endl;
      ErrorMsg("PCE_ComputeCandidatePaths: No Intra-Domain candidate path");
    }
}

//Returns 1 if there is at least one path between s and d, otherwise return 0.
//The utilized seed is initialized by CNet costructor
int CPathTable::PCE_NoInfo_RandomPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, CConnection* rconn) {
    vector<Path*> ltable; //local database of paths between nodes s and d
    int rand_id;

    #ifdef STDOUT_CPATHTABLE
    cout << "PCE_NoInfo conn id " << rconn->Id << " computing path " << s->Id << "->" << d->Id << endl;
    #endif

    //Computes the set of candidate paths and inserts them in the ltable
    this->PCE_ComputeCandidatePaths(s,d,ltable,rconn);

    if (ltable.size()) {
        rand_id=randint(0,(ltable.size()-1),&this->ParentNet->PathSelectionSeed);
        rconn->ActualPath.push_back(ltable[rand_id]);
        for (int j=0; j<(ltable[rand_id]->nodes.size()); j++)
            pathnode->push_back(ltable[rand_id]->nodes[j]);
        for (int j=0; j<(ltable[rand_id]->links.size()); j++)
            pathlink->push_back(ltable[rand_id]->links[j]);
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    return 1;
}

int CPathTable::PCE_AggInfo_LeastCongestedPath(CNode* s, CNode* d, vector< CNode* >* pathnode, vector< CLink* >* pathlink, int* freelambdas, CConnection* rconn) {
    vector<Path*> ltable, lltable;   //local database of paths between nodes s and d
    vector<int> ltable_weight;
    Path* ActualPath;
    CNode* PCE_Node=this->ParentNode;
    int j, PathBottleneck, LinkBottleneck;
    int MaxWeight, rand_id, lambdas=this->ParentNet->W;

    #ifdef STDOUT_CPATHTABLE
    cout << "NODE: " << PCE_Node->Id << " PCE_AggInfo_LeastCongestedPath conn id " << rconn->Id << " computing path " << s->Id << "->" << d->Id << endl;
    #endif

    //Computes the set of candidate paths and inserts them in the ltable
    this->PCE_ComputeCandidatePaths(s,d,ltable,rconn);

    //Initializes ltable_weight
    for (int i=0; i<ltable.size(); i++)
        ltable_weight.push_back(0);

    //Computes the ltable_weight based on the LinkState_dB of the node (source node or PCE node)
    if (ltable.size()) {
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {

            ActualPath=ltable[lt_id];
            PathBottleneck=lambdas;
            LinkBottleneck=0;

	    //Check the other links availability in the OSPF LinkState_dB i=1
            for (int i=0; i<ActualPath->links.size(); i++) {
                LinkBottleneck=0;

                j=0;
                while (ActualPath->links[i]!=PCE_Node->LinkState_dB[j]->Link) j++;
                for (int k=0; k<lambdas; k++) {
                    LinkBottleneck+=(1-PCE_Node->LinkState_dB[j]->LambdaStatus[k]);
                }
                if (LinkBottleneck<PathBottleneck) PathBottleneck=LinkBottleneck;
                //cout << "CONN ID " << rconn->Id << " LinkBottleneck [" << i << "]: " << LinkBottleneck << endl;
            }
            ltable_weight[lt_id]=PathBottleneck;
        }
	#ifdef STDOUT_CPATHTABLE
        cout << "PCE_AggInfo_LeastCongestedPath conn id " << rconn->Id << " weights: " << flush;
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            cout << ltable_weight[lt_id] << " ";
        }
	#endif
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    //Computes the MaxWeight
    MaxWeight=ltable_weight[0];
    for (int lt_id=1; lt_id<ltable.size(); lt_id++)
        if (ltable_weight[lt_id]>MaxWeight) MaxWeight=ltable_weight[lt_id];

    #ifdef STDOUT_CPATHTABLE
    cout << " max: " << MaxWeight << endl;
    #endif

    if (MaxWeight==0) return 0;
    *freelambdas=MaxWeight;

    //Builds lltable starting from ltable
    for (int i=0; i<ltable.size(); i++)
        if (ltable_weight[i]==MaxWeight)
            lltable.push_back(ltable[i]);

    #ifdef STDOUT_CPATHTABLE
    cout << "PCE_AggInfo conn id " << rconn->Id << " LeastCongested lltable size: " << lltable.size() << flush;
    #endif

    if (lltable.size()) {
        rand_id=randint(0,(lltable.size()-1),&this->ParentNet->PathSelectionSeed);
	
	#ifdef STDOUT_CPATHTABLE
        cout << " random_id: " << rand_id << endl;
	#endif
	
        rconn->ActualPath.push_back(lltable[rand_id]);
        for (int j=0; j<(lltable[rand_id]->nodes.size()); j++)
            pathnode->push_back(lltable[rand_id]->nodes[j]);
        for (int j=0; j<(lltable[rand_id]->links.size()); j++)
            pathlink->push_back(lltable[rand_id]->links[j]);
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }
    return 1;
}

int CPathTable::PCE_DetInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, int* freelambdas, vector<int>* labelset, CConnection* rconn) {
    vector<Path*> ltable, lltable;   //local database of paths between nodes s and d
    vector<int> ltable_weight;
    Path* ActualPath;
    CNode* PCE_Node=this->ParentNode;
    int LabelSetTest, MaxWeight, rand_id, lambdas=this->ParentNet->W, j;
    int VirtualLabelSet[lambdas];
    
    int PathBottleneck=lambdas, LinkBottleneck=lambdas;

    for (int i=0; i<lambdas; i++)
      VirtualLabelSet[i]=1;

    #ifdef STDOUT_CPATHTABLE
    cout << "NODE: " << PCE_Node->Id << " PCE_DetInfo_LeastCongestedPath conn id " << rconn->Id << " computing path " << s->Id << "->" << d->Id << " TIME: " << this->ParentNet->Now << endl;
    #endif

    //Computes the set of candidate paths and inserts them in the ltable
    this->PCE_ComputeCandidatePaths(s,d,ltable,rconn);

    //Initializes ltable_weight
    for (int i=0; i<ltable.size(); i++) {
        ltable_weight.push_back(0);
    }

    //Computes the ltable_weight based on the LinkState_dB of the node PCE_Node (i.e., this->ParentNode)
    //In this fuction a bug has been fixed (15/11/2011). 
    if (ltable.size()) {
	//Computing weights
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            ActualPath=ltable[lt_id];
            for (int i=0; i<lambdas; i++) VirtualLabelSet[i]=1;

            //Check the other links availability in the OSPF LinkState_dB i=1
            for (int i=0; i<ActualPath->links.size(); i++) {
                int j=0;
                while (ActualPath->links[i]!=PCE_Node->LinkState_dB[j]->Link) j++;
                for (int k=0; k<lambdas; k++) {
                    if ((VirtualLabelSet[k]==1) && (PCE_Node->LinkState_dB[j]->LambdaStatus[k]==1))
		      VirtualLabelSet[k]=0;
                }
            }
            LabelSetTest=0;
            for (int k=0; k<lambdas; k++) LabelSetTest+=VirtualLabelSet[k];
            ltable_weight[lt_id]=LabelSetTest;
        }
        
	#ifdef STDOUT_CPATHTABLE
        cout << "PCE_DetInfo_LeastCongestedPath conn id " << rconn->Id << " weights: " << flush;
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            cout << ltable_weight[lt_id] << " ";
        }
	#endif
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    //Computes the MaxWeight and MaxWeight_freelambdas
    MaxWeight=ltable_weight[0];
    for (int lt_id=1; lt_id<ltable.size(); lt_id++) {
        if (ltable_weight[lt_id]>MaxWeight)
	  MaxWeight=ltable_weight[lt_id];
    }
        
    #ifdef STDOUT_CPATHTABLE
    cout << " max: " << MaxWeight << endl;
    #endif

    if (MaxWeight==0)
      return 0;

    //Builds lltable starting from ltable
    for (int i=0; i<ltable.size(); i++)
        if (ltable_weight[i]==MaxWeight)
            lltable.push_back(ltable[i]);

    #ifdef STDOUT_CPATHTABLE
    cout << "PCE_DetInfo_LeastCongestedPath conn id " << rconn->Id << " lltable size: " << lltable.size() << flush;
    #endif

    if (lltable.size()) {
        rand_id=randint(0,(lltable.size()-1),&this->ParentNet->PathSelectionSeed);
	ActualPath=lltable[rand_id];
        rconn->ActualPath.push_back(lltable[rand_id]);
	
	//Fill up pathnode, pathlink, and lambdastatus
        for (int j=0; j<(ActualPath->nodes.size()); j++)
	  pathnode->push_back(ActualPath->nodes[j]);
        for (int j=0; j<(ActualPath->links.size()); j++)
	  pathlink->push_back(ActualPath->links[j]);
	for (int i=0; i<lambdas; i++)
	  VirtualLabelSet[i]=1;
	
	//Propagate the VirtualLabelSet and computes freelambdas
        for (int i=0; i<(ActualPath->links.size()); i++) {
	  
	  int j=0;
	  while (ActualPath->links[i]!=PCE_Node->LinkState_dB[j]->Link) j++;
	  
	  for (int k=0; k<lambdas; k++) {
	    if ((VirtualLabelSet[k]==1) && (PCE_Node->LinkState_dB[j]->LambdaStatus[k]==1))
	      VirtualLabelSet[k]=0;
	  }
	  
	  LinkBottleneck=PCE_Node->LinkState_dB[j]->FreeLambdas;
	  if (LinkBottleneck<PathBottleneck) {
	    PathBottleneck=LinkBottleneck;
	  }
	  
	}
	
	//Set freelambdas and labelset
	*freelambdas=PathBottleneck;
	for (int i=0; i<lambdas; i++) {
	  labelset->push_back(VirtualLabelSet[i]);
	}
            
        #ifdef STDOUT_CPATHTABLE
        cout << " random_id: " << rand_id << endl;
	#endif
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }
    return 1;
}

int CPathTable::Flexible_PCE_DetInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, int* freelambdas, vector<int>* labelset, CConnection* rconn) {
    vector<Path*> ltable,lltable;   //local database of paths between nodes s and d
    vector<int> ltable_weight;
    Path* ActualPath;
    CNode* PCE_Node=this->ParentNode;
    int LabelSetTest, MaxWeight, rand_id, lambdas=this->ParentNet->W, j;
    int VirtualLabelSet[lambdas];
    int required_slots=rconn->width_slots;
    
    int PathBottleneck=lambdas, LinkBottleneck=lambdas;
    for (int i=0; i<lambdas; i++)
      VirtualLabelSet[i]=1;

    #ifdef STDOUT_CPATHTABLE
    cout << "NODE: " << PCE_Node->Id << " PCE_DetInfo_LeastCongestedPath conn id " << rconn->Id << " computing path " << s->Id << "->" << d->Id << " TIME: " << this->ParentNet->Now << endl;
    #endif

    //Computes the set of candidate paths and inserts them in the ltable
    this->PCE_ComputeCandidatePaths(s,d,ltable,rconn);

    //Initializes ltable_weight
    for (int i=0; i<ltable.size(); i++) {
        ltable_weight.push_back(0);
    }

    //Computes the ltable_weight based on the LinkState_dB of the node PCE_Node (i.e., this->ParentNode)
    //In this fuction a bug has been fixed (15/11/2011). 
    if (ltable.size()) {
	//Computing weights
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            ActualPath=ltable[lt_id];
            for (int i=0; i<lambdas; i++) VirtualLabelSet[i]=1;

            //Check the other links availability in the OSPF LinkState_dB i=1
            for (int i=0; i<ActualPath->links.size(); i++) {
                int j=0;
                while (ActualPath->links[i]!=PCE_Node->LinkState_dB[j]->Link) j++;
                for (int k=0; k<lambdas; k++) {
                    if ((VirtualLabelSet[k]==1) && (PCE_Node->LinkState_dB[j]->LambdaStatus[k]==1))
		      VirtualLabelSet[k]=0;
                }
            }
            
	    //Compute the weights considering the slots required by the connection
            for (int i=0; i<=lambdas-required_slots; i++) {
	      LabelSetTest=0;
	      for (int k=0;k<required_slots;k++) {
		if(VirtualLabelSet[i+k]==1)
		  LabelSetTest++;
	      }
	      if (LabelSetTest==required_slots)
		ltable_weight[lt_id]++;
	    }
        }
	#ifdef STDOUT_CPATHTABLE
        cout << "PCE_DetInfo_LeastCongestedPath conn id " << rconn->Id << " weights: " << flush;
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            cout << ltable_weight[lt_id] << " ";
        }
	#endif
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    //Computes the MaxWeight and MaxWeight_freelambdas
    MaxWeight=ltable_weight[0];
    for (int lt_id=1; lt_id<ltable.size(); lt_id++) {
        if (ltable_weight[lt_id]>MaxWeight)
	  MaxWeight=ltable_weight[lt_id];
    }

    #ifdef STDOUT_CPATHTABLE
    cout << " max: " << MaxWeight << endl;
    #endif

    if (MaxWeight==0)
      return 0;

    //Builds lltable starting from ltable
    for (int i=0; i<ltable.size(); i++)
        if (ltable_weight[i]==MaxWeight)
            lltable.push_back(ltable[i]);

    #ifdef STDOUT_CPATHTABLE
    cout << "PCE_DetInfo_LeastCongestedPath conn id " << rconn->Id << " lltable size: " << lltable.size() << flush;
    #endif

    if (lltable.size()) {
        rand_id=randint(0,(lltable.size()-1),&this->ParentNet->PathSelectionSeed);
	ActualPath=lltable[rand_id];
        rconn->ActualPath.push_back(lltable[rand_id]);
	
	//Fill up pathnode, pathlink, and lambdastatus
        for (int j=0; j<(ActualPath->nodes.size()); j++)
	  pathnode->push_back(ActualPath->nodes[j]);
        for (int j=0; j<(ActualPath->links.size()); j++)
	  pathlink->push_back(ActualPath->links[j]);
	for (int i=0; i<lambdas; i++)
	  VirtualLabelSet[i]=1;
	
	//Propagate the VirtualLabelSet and computes freelambdas
        for (int i=0; i<(ActualPath->links.size()); i++) {
	  
	  int j=0;
	  while (ActualPath->links[i]!=PCE_Node->LinkState_dB[j]->Link) j++;
	  
	  for (int k=0; k<lambdas; k++) {
	    if ((VirtualLabelSet[k]==1) && (PCE_Node->LinkState_dB[j]->LambdaStatus[k]==1))
	      VirtualLabelSet[k]=0;
	  }
	  
	  LinkBottleneck=PCE_Node->LinkState_dB[j]->FreeLambdas;
	  if (LinkBottleneck<PathBottleneck) {
	    PathBottleneck=LinkBottleneck;
	  }
	  
	}
	
	//Set freelambdas and labelset
	*freelambdas=PathBottleneck;
	for (int i=0; i<lambdas; i++) {
	  labelset->push_back(VirtualLabelSet[i]);
	}
            
        #ifdef STDOUT_CPATHTABLE
        cout << " random_id: " << rand_id << endl;
	#endif
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }
    return 1;
}

int CPathTable::HPCE_ComputeCandidatePaths(CNode* s, CNode* d, vector<Path*>& ltable, CConnection* conn) {
    int table_entry;
    Path* CurrentPath;

    #ifdef STDOUT_CPATHTABLE
    cout << "HPCE_ComputeCandidatePaths conn id " << conn->Id << " " << s->Id << "->" << d->Id << endl;
    #endif

    //Compute the entry of the table where the proper paths are located
    table_entry=this->index[s->Id*this->ParentNet->Node.size()+d->Id];
    CurrentPath=this->table[table_entry];

    //Builds ltable starting from the CNet->PathTable_HPCE including node s as source and node d as destination
    while ((table_entry<this->table.size()) && (s==CurrentPath->Src) && (d==CurrentPath->Dst)) {
        ltable.push_back(CurrentPath);

        table_entry++;
        CurrentPath=this->table[table_entry];
    }

    #ifdef STDOUT_CPATHTABLE
    cout << "HPCE_ComputeCandidatePaths conn id " << conn->Id << " ltable.size(): " << ltable.size() << endl;
    for (int i=0; i<ltable.size(); i++) {
      cout << "ID: " << ltable[i]->Id << " (" << ltable[i]->Src->Id << "," << ltable[i]->Dst->Id << ")";
      cout << " nodes: ";
      for (int j=0; j<ltable[i]->nodes.size(); j++)
	cout <<ltable[i]->nodes[j]->Id << " ";
      cout << " links: ";
      for (int j=0; j<ltable[i]->links.size(); j++) {
	if (ltable[i]->links[j]!=NULL) cout << ltable[i]->links[j]->Id << " ";
	else cout << "NULL ";
      }
    cout << endl;
    }
    #endif

    if (ltable.size() > 0) return 1;
    else ErrorMsg("No HPCE candidate path");
}

int CPathTable::CNet_ComputeCandidatePaths(CNode* s, CNode* d, vector<Path*>& ltable, CConnection* conn) {
    int table_entry;
    Path* CurrentPath;

    #ifdef STDOUT_CPATHTABLE
    cout << "CNet_ComputeCandidatePaths conn id " << conn->Id << " " << s->Id << "->" << d->Id << endl;
    #endif

    //Compute the entry of the table where the proper paths are located
    table_entry=this->index[s->Id*this->ParentNet->Node.size()+d->Id];
    CurrentPath=this->table[table_entry];

    //Builds ltable starting from the CNet->PathTable including node s as source and node d as destination
    while ((table_entry<this->table.size()) && (s==CurrentPath->Src) && (d==CurrentPath->Dst)) {
        ltable.push_back(CurrentPath);

        table_entry++;
        CurrentPath=this->table[table_entry];
    }

    #ifdef STDOUT_CPATHTABLE
    cout << "CNet_ComputeCandidatePaths conn id " << conn->Id << " ltable.size(): " << ltable.size() << endl;
    for (int i=0; i<ltable.size(); i++) {
      cout << "ID: " << ltable[i]->Id << " (" << ltable[i]->Src->Id << "," << ltable[i]->Dst->Id << ")";
      cout << " nodes: ";
      for (int j=0; j<ltable[i]->nodes.size(); j++)
	cout <<ltable[i]->nodes[j]->Id << " ";
      cout << " links: ";
      for (int j=0; j<ltable[i]->links.size(); j++) {
	if (ltable[i]->links[j]!=NULL) cout << ltable[i]->links[j]->Id << " ";
	else cout << "NULL ";
      }
    cout << endl;
    }
    #endif

    if (ltable.size() > 0) return 1;
    else ErrorMsg("No CNet candidate path");
}

int CPathTable::HPCE_NoInfo_EdgeSequence(CNode* s, CNode* d, vector<CNode*>* pathnode, vector <CLink*>* pathlink,CConnection* rconn) {
    vector<Path*> ltable;
    int rand_id;
    PCx_Node* hpce;

    #ifdef STDOUT_CPATHTABLE
    cout << "HPCE NoInfo conn id " << rconn->Id << " " << rconn->SourceNode->Id << "->" << rconn->DestinationNode->Id << endl;
    #endif

    if (this->ParentNode->Type==PCC_PCE_HPCE)
        hpce=(PCx_Node*) this->ParentNode;
    else ErrorMsg("HPCE_NoInfo_EdgeSequence exetuted not at the HPCE");

    //Computes the set of candidate paths based on the Edge Topology and inserts them in the ltable
    this->HPCE_ComputeCandidatePaths(s,d,ltable,rconn);

    if (ltable.size()) {
        rand_id=randint(0,(ltable.size()-1),&this->ParentNet->PathSelectionSeed);
        rconn->ActualPath.push_back(ltable[rand_id]);

        for (int j=0; j<(ltable[rand_id]->nodes.size()); j++)
            pathnode->push_back(ltable[rand_id]->nodes[j]);
        for (int j=0; j<(ltable[rand_id]->links.size()); j++)
            pathlink->push_back(ltable[rand_id]->links[j]);
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

#ifdef STDOUT_CPATHTABLE
    cout << "HPCE_NoInfo_EdgeSequence conn id " << rconn->Id << " ltable.size(): " << ltable.size() << " rand_id: " << rand_id << " node path: ";
    for (int i=0; i<ltable[rand_id]->nodes.size();i++) {
        cout << ltable[rand_id]->nodes[i]->Id << " ";
    }
    cout << endl;
    cout << "HPCE_NoInfo_EdgeSequence conn id " << rconn->Id << " ltable.size(): " << ltable.size() << " rand_id: " << rand_id << " link path: ";
    for (int i=0; i<ltable[rand_id]->links.size();i++) {
        if (ltable[rand_id]->links[i]!=NULL) cout << ltable[rand_id]->links[i]->Id << " ";
        else cout << "NULL ";
    }
    cout << endl;
#endif

    return 1;
}

int CPathTable::HPCE_AggInfo_EdgeSequence(CNode* s, CNode* d, vector<CNode*>* pathnode, vector <CLink*>* pathlink,CConnection* rconn) {
    vector<Path*> ltable, lltable;
    vector<int> ltable_weight;
    Path* ActualPath;
    CLink* ActualLink;
    int LinkBottleneck,PathBottleneck,LinkStateIndex;
    int MaxWeight, rand_id, lambdas=this->ParentNet->W;
    int TableEntry;
    PCx_Node* hpce;

    #ifdef STDOUT_CPATHTABLE
    cout << "HPCE AggInfo conn id " << rconn->Id << " " << rconn->SourceNode->Id << "->" << rconn->DestinationNode->Id << endl;
    #endif

    if (this->ParentNode->Type==PCC_PCE_HPCE)
        hpce=(PCx_Node*) this->ParentNode;
    else ErrorMsg("HPCE_AggInfo_EdgeSequence exetuted not at the HPCE");

    //Computes the set of candidate paths based on the Edge Topology and inserts them in the ltable
    this->HPCE_ComputeCandidatePaths(s,d,ltable,rconn);

    //Initializes ltable_weight
    for (int i=0; i<ltable.size(); i++)
        ltable_weight.push_back(0);

    //Computes the ltable_weight based on the InterLinkState_dB of the HPCE
    if (ltable.size()) {
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {

            ActualPath=ltable[lt_id];
            PathBottleneck=lambdas;
            LinkBottleneck=0;

	    //Use the SNMP_HPCE_InterDomain_LinkState_dB for weight computation
            for (int j=0;j<ActualPath->links.size();j++) {
                if (ActualPath->links[j]!=NULL) { //Interdomain link
		  ActualLink=ActualPath->links[j];
		  
		  TableEntry=0;
		  while ((TableEntry<hpce->SNMP_HPCE_InterDomain_LinkState_dB.size()) && (hpce->SNMP_HPCE_InterDomain_LinkState_dB[TableEntry]->Link!=ActualLink))
		    TableEntry++;
		  
		  if (TableEntry==hpce->SNMP_HPCE_InterDomain_LinkState_dB.size())
		    ErrorMsg("HPCE_AggInfo_EdgeSequence inter-domain link state not found at the HPCE");
		  
		  LinkBottleneck=hpce->SNMP_HPCE_InterDomain_LinkState_dB[TableEntry]->FreeLambdas;

		  //Updating Path Bottleneck
		  if (LinkBottleneck<PathBottleneck)
		    PathBottleneck=LinkBottleneck;
                }
            }
            ltable_weight[lt_id]=PathBottleneck;

	    #ifdef STDOUT_CPATHTABLE
            cout << "Candidate path for conn id " << rconn->Id << " : ";
            for (int j=0;j<ActualPath->nodes.size();j++) {
                cout << ActualPath->nodes[j]->Id << " ";
            }
            cout << " weight: " << ltable_weight[lt_id] << endl;
	    #endif
        }
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    //Computes the MaxWeight
    MaxWeight=ltable_weight[0];
    for (int lt_id=1; lt_id<ltable.size(); lt_id++)
        if (ltable_weight[lt_id]>MaxWeight) MaxWeight=ltable_weight[lt_id];

    //Builds lltable starting from ltable
    for (int i=0; i<ltable.size(); i++)
        if (ltable_weight[i]==MaxWeight)
            lltable.push_back(ltable[i]);

    if (MaxWeight==0) return 0;

    //Randomly selects one of the least congested paths in the lltable
    if (lltable.size()) {
        rand_id=randint(0,(lltable.size()-1),&this->ParentNet->PathSelectionSeed);
        rconn->ActualPath.push_back(lltable[rand_id]);
        for (int j=0; j<(lltable[rand_id]->nodes.size()); j++)
            pathnode->push_back(lltable[rand_id]->nodes[j]);
        for (int j=0; j<(lltable[rand_id]->links.size()); j++)
            pathlink->push_back(lltable[rand_id]->links[j]);
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    #ifdef STDOUT_CPATHTABLE
    cout << "HPCE AggInfo conn id " << rconn->Id << " LeastCongested lltable size: " << lltable.size() << " random_id: " << rand_id << endl;
    #endif

    return 1;
}

int CPathTable::HPCE_DetInfo_EdgeSequence(CNode* s, CNode* d, vector<CNode*>* pathnode, vector <CLink*>* pathlink,CConnection* rconn) {
    vector<Path*> ltable, lltable;
    vector<int> ltable_weight;
    Path* ActualPath;
    CLink* ActualLink;
    int MaxWeight, LabelSetTest,rand_id, LinkStateFound, LinkStateIndex, lambdas=this->ParentNet->W;
    int VirtualLabelSet[lambdas];
    int TableEntry;
    PCx_Node* hpce;

    #ifdef STDOUT_CPATHTABLE
    cout << "HPCE DetInfo conn id " << rconn->Id << " " << rconn->SourceNode->Id << "->" << rconn->DestinationNode->Id << endl;
    #endif

    if (this->ParentNode->Type==PCC_PCE_HPCE)
        hpce=(PCx_Node*) this->ParentNode;
    else ErrorMsg("HPCE_DetInfo_EdgeSequence exetuted not at the HPCE");

    for (int i=0;i<lambdas;i++) VirtualLabelSet[i]=1;

    //Computes the set of candidate paths based on the Edge Topology and inserts them in the ltable
    this->HPCE_ComputeCandidatePaths(s,d,ltable,rconn);

    //Initializes ltable_weight
    for (int i=0; i<ltable.size(); i++)
        ltable_weight.push_back(0);

    //Computes the ltable_weight based on the InterLinkState_dB of the HPCE
    if (ltable.size()) {
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            ActualPath=ltable[lt_id];
            for (int i=0;i<lambdas;i++) VirtualLabelSet[i]=1;

	    //Use the SNMP_HPCE_InterDomain_LinkState_dB for weight computation
            for (int j=0;j<ActualPath->links.size();j++) {
                if (ActualPath->links[j]!=NULL) { //Interdomain link
		  ActualLink=ActualPath->links[j];
		  
		  TableEntry=0;
		  while ((TableEntry<hpce->SNMP_HPCE_InterDomain_LinkState_dB.size()) && (hpce->SNMP_HPCE_InterDomain_LinkState_dB[TableEntry]->Link!=ActualLink))
		    TableEntry++;
		  
		  if (TableEntry==hpce->SNMP_HPCE_InterDomain_LinkState_dB.size())
		    ErrorMsg("HPCE_DetInfo_EdgeSequence inter-domain link state not found at the HPCE");
		  
		  //Propoagation of VirtualLabelSet on inter-domain links
		  for (int k=0; k<lambdas; k++) {
		    if ((VirtualLabelSet[k]==1) && (hpce->SNMP_HPCE_InterDomain_LinkState_dB[TableEntry]->LambdaStatus[k]==1))
                            VirtualLabelSet[k]=0;
                    }
                }
            }

            LabelSetTest=0;
            for (int k=0;k<lambdas;k++) LabelSetTest+=VirtualLabelSet[k];
            ltable_weight[lt_id]=LabelSetTest;

	    #ifdef STDOUT_CPATHTABLE
            cout << "Candidate path for conn id " << rconn->Id << " : ";
            for (int j=0;j<ActualPath->nodes.size();j++) {
                cout << ActualPath->nodes[j]->Id << " ";
            }
            cout << " weight: " << ltable_weight[lt_id] << endl;
	    #endif
        }
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    //Computes the MaxWeight
    MaxWeight=ltable_weight[0];
    for (int lt_id=1; lt_id<ltable.size(); lt_id++)
        if (ltable_weight[lt_id]>MaxWeight) MaxWeight=ltable_weight[lt_id];

    //Builds lltable starting from ltable
    for (int i=0; i<ltable.size(); i++)
        if (ltable_weight[i]==MaxWeight)
            lltable.push_back(ltable[i]);

    if (MaxWeight==0) return 0;

    //Randomly selects one of the least congested paths in the lltable
    //Randomly selects one of the paths in the ltable
    //this->ParentNet->TableSize->AddSample_Distribution(lltable.size());
    //this->ParentNet->TableSize->AddSample(lltable.size());

    if (lltable.size()) {
        rand_id=randint(0,(lltable.size()-1),&this->ParentNet->PathSelectionSeed);
	
	#ifdef STDOUT_CPATHTABLE
        cout << " random_id: " << rand_id << endl;
	#endif
	
        rconn->ActualPath.push_back(lltable[rand_id]);
        for (int j=0; j<(lltable[rand_id]->nodes.size()); j++)
            pathnode->push_back(lltable[rand_id]->nodes[j]);
        for (int j=0; j<(lltable[rand_id]->links.size()); j++)
            pathlink->push_back(lltable[rand_id]->links[j]);
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }
    
    #ifdef STDOUT_CPATHTABLE
    cout << "HPCE AggInfo conn id " << rconn->Id << " LeastCongested lltable size: " << lltable.size() << " random_id: " << rand_id << endl;
    #endif

    return 1;
}

int CPathTable::Flexible_HPCE_BGP_DetInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector <CLink*>* pathlink,CConnection* rconn) {
    vector<Path*> ltable, lltable;
    vector<int> ltable_weight;
    Path* ActualPath;
    CLink* ActualLink;
    int MaxWeight, LabelSetTest,rand_id, LinkStateFound, LinkStateIndex, lambdas=this->ParentNet->W;
    int VirtualLabelSet[lambdas];
    int TableEntry;
    PCx_Node* hpce;
    int required_slots=rconn->width_slots;
    
    #ifdef STDOUT_CPATHTABLE
    cout << "HPCE BGP DetInfo conn id " << rconn->Id << " " << rconn->SourceNode->Id << "->" << rconn->DestinationNode->Id << endl;
    #endif

    //This function is called by the CPathTable of CNet
    if (this->ParentNode->Type==PCC_PCE_HPCE) {
        hpce=(PCx_Node*) this->ParentNode;
    }
    else ErrorMsg("Flexible_HPCE_BGP_DetInfo_LeastCongestedPath exetuted not at the HPCE");

    for (int i=0;i<lambdas;i++) VirtualLabelSet[i]=1;

    //Computes the set of candidate paths based on the Edge Topology and inserts them in the ltable
    this->CNet_ComputeCandidatePaths(s,d,ltable,rconn);

    //Initializes ltable_weight
    for (int i=0; i<ltable.size(); i++)
        ltable_weight.push_back(0);

    //Computes the ltable_weight based on the BGP_LinkState_dB of the HPCE
    if (ltable.size()) {
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            ActualPath=ltable[lt_id];
            for (int i=0;i<lambdas;i++) VirtualLabelSet[i]=1;

	    //Use the HPCE_LinkState_dB for weight computation
            for (int j=0;j<ActualPath->links.size();j++) {
		ActualLink=ActualPath->links[j];
		  
		TableEntry=0;
		while ((TableEntry<hpce->BGPLS_HPCE_LinkState_dB.size()) && (hpce->BGPLS_HPCE_LinkState_dB[TableEntry]->Link!=ActualLink))
		TableEntry++;
		  
		if (TableEntry==hpce->BGPLS_HPCE_LinkState_dB.size())
		  ErrorMsg("HPCE_DetInfo_EdgeSequence inter-domain link state not found at the HPCE");
		  
		//Propoagation of VirtualLabelSet on inter-domain links
		for (int k=0; k<lambdas; k++) {
		  //Here it is important to remember that LambdaStatus[k] can be also equal to 2 when it is over-reserved
		  if ((VirtualLabelSet[k]==1) && (hpce->BGPLS_HPCE_LinkState_dB[TableEntry]->LambdaStatus[k]>0))
		    VirtualLabelSet[k]=0;
		}
            }
	    
	    //Compute the weights considering the slots required by the connection
            for (int i=0; i<=lambdas-required_slots; i++) {
	      LabelSetTest=0;
	      for (int k=0;k<required_slots;k++) {
		if(VirtualLabelSet[i+k]==1)
		  LabelSetTest++;
	      }
	      if (LabelSetTest==required_slots)
		ltable_weight[lt_id]++;
	    }

	    #ifdef STDOUT_CPATHTABLE
            cout << "Candidate path for conn id " << rconn->Id << " : ";
            for (int j=0;j<ActualPath->nodes.size();j++) {
                cout << ActualPath->nodes[j]->Id << " ";
            }
            cout << " weight: " << ltable_weight[lt_id] << endl;
	    #endif
        }
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    //Computes the MaxWeight
    MaxWeight=ltable_weight[0];
    for (int lt_id=1; lt_id<ltable.size(); lt_id++)
        if (ltable_weight[lt_id]>MaxWeight) MaxWeight=ltable_weight[lt_id];

    //Builds lltable starting from ltable
    for (int i=0; i<ltable.size(); i++)
        if (ltable_weight[i]==MaxWeight)
            lltable.push_back(ltable[i]);

    if (MaxWeight==0) return 0;

    //Randomly selects one of the least congested paths in the lltable
    if (lltable.size()) {
        rand_id=randint(0,(lltable.size()-1),&this->ParentNet->PathSelectionSeed);
	
        rconn->ActualPath.push_back(lltable[rand_id]);
        for (int j=0; j<(lltable[rand_id]->nodes.size()); j++)
            pathnode->push_back(lltable[rand_id]->nodes[j]);
        for (int j=0; j<(lltable[rand_id]->links.size()); j++)
            pathlink->push_back(lltable[rand_id]->links[j]);
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }
    
    #ifdef STDOUT_CPATHTABLE
    cout << "HPCE BGP DetInfo conn id " << rconn->Id << " LeastCongested lltable size: " << lltable.size() << " random_id: " << rand_id << endl;
    #endif

    return 1;
}

int CPathTable::Flexible_PCE_SNMP_DetInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, CConnection* rconn) {
    vector<Path*> ltable, lltable;//local database of paths between nodes s and d
    vector<int> ltable_weight;

    int LabelSetTest, MaxWeight, rand_id, lambdas=this->ParentNet->W, j;
    int VirtualLabelSet[lambdas];
    int required_slots=rconn->width_slots;

    PCx_Node* pce=this->ParentDomain->PCE_Node;
    Path* ActualPath;
    
    //Builds ltable starting from central table including paths from node s to node d
    for (int i=0; i<table.size(); i++)
        if ((s->Id==table[i]->Src->Id) && (d->Id==table[i]->Dst->Id)) {
            ltable.push_back(table[i]);
            ltable_weight.push_back(0);
        }
        
    //Removes from ltable the paths blocked for EmptyLabelSet
    for (int i=0; i<rconn->PathsBlocked_EmptyLabelSet.size(); i++) {
        vector<Path*>::iterator it_path=ltable.begin();
        while (rconn->PathsBlocked_EmptyLabelSet[i]!=*it_path) it_path++;
        ltable.erase(it_path);
    }

    #ifdef STDOUT_CPATHTABLE
    cout << "PCE DetInfo conn id " << rconn->Id << " TIME: " << this->ParentNet->Now << " ActiveLSAs: " << this->ParentNet->ActiveLSA << endl;
    cout << "PCE DetInfo conn id " << rconn->Id << " " << rconn->SourceNode->Id << "->" << rconn->DestinationNode->Id << endl;
    cout << "PCE DetInfo conn id " << rconn->Id << " ltable size: " << ltable.size() << flush;
    #endif

    //Computes the ltable_weight based on the LinkState_dB of the PCE
    if (ltable.size()) {
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            ActualPath=ltable[lt_id];
            for (int i=0; i<lambdas; i++) VirtualLabelSet[i]=1;

            //Check the links availability in the SNMP LinkState_dB for computing the VirtualLabelSet
            for (int i=0; i<ActualPath->links.size(); i++) {
                int j=0;
                while (ActualPath->links[i]!=pce->SNMP_LinkState_dB[j]->Link) j++;
                for (int k=0; k<lambdas; k++) {
                    if ((VirtualLabelSet[k]==1) && (pce->SNMP_LinkState_dB[j]->LambdaStatus[k]>0)) 
		      VirtualLabelSet[k]=0;
                }
            }
            
            //Computed the weights considering the slots required by the connection
            for (int i=0; i<=lambdas-required_slots; i++) {
	      LabelSetTest=0;
	      for (int k=0;k<required_slots;k++) {
		if(VirtualLabelSet[i+k]==1)
		  LabelSetTest++;
	      }
	      if (LabelSetTest==required_slots)
		ltable_weight[lt_id]++;
	    }
	}
	#ifdef STDOUT_CPATHTABLE
	cout << " lambdas " << lambdas << " required_slots " << required_slots;
        cout << " weights " << flush;
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            cout << ltable_weight[lt_id] << " ";
        }
	#endif
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    //Computes the MaxWeight
    MaxWeight=ltable_weight[0];
    for (int lt_id=1; lt_id<ltable.size(); lt_id++)
        if (ltable_weight[lt_id]>MaxWeight) MaxWeight=ltable_weight[lt_id];
	
    #ifdef STDOUT_CPATHTABLE
    cout << " max " << MaxWeight << endl;
    #endif

    //Builds lltable starting from ltable (lltable contains only paths with the MaxWeight)
    for (int i=0; i<ltable.size(); i++)
        if (ltable_weight[i]==MaxWeight)
            lltable.push_back(ltable[i]);

    #ifdef STDOUT_CPATHTABLE
    cout << "DetInfo conn id " << rconn->Id << " lltable size: " << lltable.size() << flush;
    #endif

    if (MaxWeight==0) return 0;

    //Randomly selects one of the least congested paths in the lltable
    if (lltable.size()) {
        rand_id=randint(0,(lltable.size()-1),&this->ParentNet->PathSelectionSeed);
	
	#ifdef STDOUT_CPATHTABLE
        cout << " random_id: " << rand_id << endl;
	#endif
	
	rconn->ActualPath.clear();
	rconn->ActualPath.push_back(lltable[rand_id]);
	
        for (int j=0; j<(lltable[rand_id]->nodes.size()); j++)
            pathnode->push_back(lltable[rand_id]->nodes[j]);
        for (int j=0; j<(lltable[rand_id]->links.size()); j++)
            pathlink->push_back(lltable[rand_id]->links[j]);
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }
    return 1;
}

int CPathTable::PCE_SNMP_AggInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector<CLink*>* pathlink, CConnection* rconn) {
    vector<Path*> ltable, lltable;   //local database of paths between nodes s and d
    vector<int> ltable_weight;
    Path* ActualPath;
    int j, PathBottleneck, LinkBottleneck;
    int MaxWeight, rand_id, lambdas=this->ParentNet->W;

    PCx_Node* pce=this->ParentDomain->PCE_Node;

    //if (this->ParentNode->Type!=PCE) ErrorMsg("PCE_CollisionAvoidancePath called not in the PCE");

    //Builds ltable starting from central table including paths from node s to node d
    for (int i=0; i<table.size(); i++)
        if ((s->Id==table[i]->Src->Id) && (d->Id==table[i]->Dst->Id)) {
            ltable.push_back(table[i]);
            ltable_weight.push_back(0);
        }

    //Removes from ltable the paths blocked for EmptyLabelSet
    for (int i=0; i<rconn->PathsBlocked_EmptyLabelSet.size(); i++) {
        vector<Path*>::iterator it_path=ltable.begin();
        while (rconn->PathsBlocked_EmptyLabelSet[i]!=*it_path) it_path++;
        ltable.erase(it_path);
    }

    #ifdef STDOUT_CPATHTABLE
    cout << "PCE AggInfo conn id " << rconn->Id << " " << rconn->SourceNode->Id << "->" << rconn->DestinationNode->Id << endl;
    cout << "PCE AggInfo conn id " << rconn->Id << " ltable size: " << ltable.size() << flush;
    #endif

    //Computes the ltable_weight based on the LinkState_dB of the node s
    if (ltable.size()) {
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {

            ActualPath=ltable[lt_id];
            PathBottleneck=lambdas;
            LinkBottleneck=0;

            for (int i=0; i<ActualPath->links.size(); i++) {
                LinkBottleneck=0;

                j=0;
                while (ActualPath->links[i]!=pce->SNMP_LinkState_dB[j]->Link) j++;
                for (int k=0; k<lambdas; k++) {
                    LinkBottleneck+=(1-pce->SNMP_LinkState_dB[j]->LambdaStatus[k]);
                }
                if (LinkBottleneck<PathBottleneck) PathBottleneck=LinkBottleneck;
            }
            ltable_weight[lt_id]=PathBottleneck;
        }
	#ifdef STDOUT_CPATHTABLE
        cout << "AggInfo conn id " << rconn->Id << " weights: " << flush;
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            cout << ltable_weight[lt_id] << " ";
        }
	#endif
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    //Computes the MaxWeight
    MaxWeight=ltable_weight[0];
    for (int lt_id=1; lt_id<ltable.size(); lt_id++)
        if (ltable_weight[lt_id]>MaxWeight) MaxWeight=ltable_weight[lt_id];
    #ifdef STDOUT_CPATHTABLE
    cout << " max " << MaxWeight << endl;
     #endif

    //Builds lltable starting from ltable
    for (int i=0; i<ltable.size(); i++)
        if (ltable_weight[i]==MaxWeight)
            lltable.push_back(ltable[i]);
    #ifdef STDOUT_CPATHTABLE
    cout << "AggInfo conn id " << rconn->Id << " LeastCongested lltable size: " << lltable.size() << flush;
    #endif

    if (MaxWeight==0) return 0;

    //Randomly selects one of the least congested paths in the lltable
    if (lltable.size()) {
        rand_id=randint(0,(lltable.size()-1),&this->ParentNet->PathSelectionSeed);
	#ifdef STDOUT_CPATHTABLE
        cout << " random_id: " << rand_id << endl;
	#endif
        rconn->ActualPath.push_back(lltable[rand_id]);
        for (int j=0; j<(lltable[rand_id]->nodes.size()); j++)
            pathnode->push_back(lltable[rand_id]->nodes[j]);
        for (int j=0; j<(lltable[rand_id]->links.size()); j++)
            pathlink->push_back(lltable[rand_id]->links[j]);
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }
    return 1;
}

int CPathTable::Flexible_PCE_WavelengthAssignmentFF(vector<CLink*> pathlink, int required_slots, int conn_id) {
  int SelectedSlotBase;
  vector<int> CandidatesSlotBase;
  int slots;
  int spectrum_test;
  slots=this->ParentNet->W;
  int virtual_label_set[slots];
  int j;
  
  PCx_Node* pce=this->ParentDomain->PCE_Node;

  #ifdef STDOUT
  cout << "NODE: " << pce->Id << " (" << pce->Name << "): conn id " << conn_id << " Flexible PCE_WavelengthAssignmentFF" << endl;
  #endif
  
  //Estimate the LabelSet along the computed link using OSPF database at PCE
  for (int i=0; i<slots; i++) {
    virtual_label_set[i]=1;
  }
  
  for (int i=0; i<pathlink.size(); i++) {
    j=0;
    while (pathlink[i]!=pce->LinkState_dB[j]->Link) j++;
    for (int k=0; k<slots; k++) {
      if ((virtual_label_set[k]==1) && (pce->LinkState_dB[j]->LambdaStatus[k]==1))
	virtual_label_set[k]=0;
    }
  }
  
  //Fill the candidate slot base
  for (int i=0; i<=slots-required_slots; i++) {
    spectrum_test=0;
    
    //If virtual label set is 1 the lambda is available
    for (int j=0; j<required_slots; j++) 
      if (virtual_label_set[i+j]==1) spectrum_test++;
    
    if (spectrum_test==required_slots)
      CandidatesSlotBase.push_back(i);
  }
  
  if (CandidatesSlotBase.size()!=0) {
    #ifdef STDOUT
    cout << "NODE: " << pce->Id << " (" << pce->Name << "): conn id " << conn_id;
    cout << " Suggested base slot: " << CandidatesSlotBase[0];
    cout << " RequiredSlots: " << required_slots << endl;
    #endif
    
    //Use first-fit
    return CandidatesSlotBase[0];
  }
  else {
    return -1;
  }
}

int CPathTable::Flexible_HPCE_WavelengthAssignmentFF(vector<CLink*> pathlink, int required_slots, int conn_id, int rest_flag,CConnection* conn) {
  int SelectedSlotBase;
  vector<int> CandidatesSlotBase;
  int slots;
  int spectrum_test;
  slots=this->ParentNet->W;
  int virtual_label_set[slots];
  int j;
  int SmartLabel;
  
  PCx_Node* hpce=this->ParentNet->HPCE_Node;

  #ifdef STDOUT
  cout << "NODE: " << hpce->Id << " (" << hpce->Name << "): conn id " << conn_id << " Flexible HPCE_WavelengthAssignmentFF" << endl;
  #endif
  
  //Estimate the LabelSet along the computed link using OSPF database at PCE
  for (int i=0; i<slots; i++) {
    virtual_label_set[i]=1;
  }
  
  for (int i=0; i<pathlink.size(); i++) {
    j=0; 
    while (pathlink[i]!=hpce->BGPLS_HPCE_LinkState_dB[j]->Link) {j++;};
    
    for (int k=0; k<slots; k++) {
      if ((virtual_label_set[k]==1) && (hpce->BGPLS_HPCE_LinkState_dB[j]->LambdaStatus[k]>0))
	virtual_label_set[k]=0;
    }
  }
  
  //Fill the candidate slot base
  for (int i=0; i<=slots-required_slots; i++) {
    spectrum_test=0;
    
    //If virtual label set is 1 the lambda is available
    for (int j=0; j<required_slots; j++) 
      if (virtual_label_set[i+j]==1) spectrum_test++;
    
    if (spectrum_test==required_slots)
      CandidatesSlotBase.push_back(i);
  }

  if ((rest_flag==1) && (this->ParentNet->SmartRestorationMapping==1)) {
    SmartLabel = this->ParentNet->W - conn->OldLabel - conn->width_slots + 1;
	  
    for (int i=0; i<CandidatesSlotBase.size(); i++) {
      if (CandidatesSlotBase[i]==SmartLabel)
	return SmartLabel;
    }
  }
  
  if (CandidatesSlotBase.size()!=0) {
    #ifdef STDOUT
    cout << "NODE: " << hpce->Id << " (" << hpce->Name << "): conn id " << conn_id;
    cout << " Suggested base slot: " << CandidatesSlotBase[0];
    cout << " RequiredSlots: " << required_slots << endl;
    #endif
    
    //Use first-fit
    return CandidatesSlotBase[0];
  }
  else {
    return -1;
  }
}

int CPathTable::Flexible_PCE_SNMP_WavelengthAssignmentFF(vector<CLink*> pathlink, int required_slots, int conn_id, int rest_flag, CConnection* conn) {
  int SelectedSlotBase;
  vector<int> CandidatesSlotBase;
  int slots;
  int spectrum_test;
  PCx_Node* pce=this->ParentDomain->PCE_Node;
  slots=this->ParentNet->W;
  int virtual_label_set[slots];
  int j;
  int SmartLabel;

  #ifdef STDOUT_CPATHTABLE
  cout << "PCE NODE: " << pce->Id << " (" << pce->Name << "): conn id " << conn_id << " segment Flexible PCE_SNMP_WavelengthAssignmentFF: ";
  #endif
  
  //Estimate the LabelSet along the computed link using SNMP database at PCE
  for (int i=0; i<slots; i++) {
    virtual_label_set[i]=1;
  }
  
  for (int i=0; i<pathlink.size(); i++) {
    j=0;
    while (pathlink[i]!=pce->LinkState_dB[j]->Link) j++;
    for (int k=0; k<slots; k++) {
      if ((virtual_label_set[k]==1) && (pce->SNMP_LinkState_dB[j]->LambdaStatus[k]>0))
	virtual_label_set[k]=0;
    }
  }
  
  //Fill the candidate slot base
  for (int i=0; i<=slots-required_slots; i++) {
    spectrum_test=0;
    
    //If virtual label set is 1 the lambda is available
    for (int j=0; j<required_slots; j++) 
      if (virtual_label_set[i+j]==1) spectrum_test++;
    
    if (spectrum_test==required_slots)
      CandidatesSlotBase.push_back(i);
  }
  
  if ((rest_flag==1) && (this->ParentNet->SmartRestorationMapping==1)) {
    SmartLabel = this->ParentNet->W - conn->OldLabel - conn->width_slots + 1;
	  
    for (int i=0; i<CandidatesSlotBase.size(); i++) {
      if (CandidatesSlotBase[i]==SmartLabel)
	return SmartLabel;
    }
  }
  
  if (CandidatesSlotBase.size()!=0) {
    #ifdef STDOUT_CPATHTABLE
    cout << "PCE NODE: " << pce->Id << " (" << pce->Name << "): conn id " << conn_id;
    cout << " Suggested base slot: " << CandidatesSlotBase[0];
    cout << " RequiredSlots: " << required_slots << endl;
    #endif
    
    //Use first-fit
    return CandidatesSlotBase[0];
  }
  else {
    return -1;
  }
}

void CPathTable::DepthFirstSearch_Network(CNode* s, CNode* d, int depth, int ElapsedDistance, int MaxDistance, vector<int> visited, vector<CNode*> path, vector<Path*>& ltable) {
    CNode *AdjNode, *ActualNode, *NextNode;
    CLink *ActualLink;
    static int iterations=0;

    iterations++;
    visited[s->Id]=1;//da inizializzare a 0 prima di entrare nella funzione
    path.push_back(s);

    //Condizione di iterazione
    if (s!=d) {
        for (int i=0; i<s->OutLink.size(); i++) {
            AdjNode=s->OutLink[i]->ToNode;
            ActualLink=s->GetLinkToNode(AdjNode);

            //This condition also avoid the utilization of LINK_DOWN
            if ((!visited[AdjNode->Id]) && (ActualLink->Status==UP)) {
                if (ElapsedDistance + this->DistanceMatrix[AdjNode->Id][d->Id] < MaxDistance) {
                    DepthFirstSearch_Network(AdjNode,d,depth+1,ElapsedDistance+ActualLink->DistanceMetric,MaxDistance,visited,path,ltable);
                }
            }
        }
    }

    //Condizione di arresto iterazione
    if (s==d) {
        if (ElapsedDistance<=MaxDistance) {
            Path* P=new Path;

            P->Id = ltable.size();

            P->Src = path[0];
            P->Dst = path[path.size()-1];
            P->ParentDomain=P->Src->ParentDomain;

            P->nodes=path;

            for (int i=0; i<path.size()-1; i++) {
                ActualNode=path[i];
                NextNode=path[i+1];
                ActualLink=ActualNode->GetLinkToNode(NextNode);
                P->links.push_back(ActualLink);
            }
            ltable.push_back(P);
        }
    }
    if (depth==0) {
        //cout << "Number of iterations: " << iterations << endl;
        iterations=0;
    }
    return;
}

void CPathTable::DepthFirstSearch_Domain(CNode* s, CNode* d, int depth, int ElapsedDistance, int MaxDistance, vector<int> visited, vector<CNode*> path, vector<Path*>& ltable) {
    CNode *AdjNode, *ActualNode, *NextNode;
    CLink *ActualLink;
    static int iterations=0;

    iterations++;
    visited[s->Id]=1;//da inizializzare a 0 prima di entrare nella funzione
    path.push_back(s);

    //Condizione di iterazione...se sono uscito dal dominio non posso rientrare
    if ((s!=d) && (s->ParentDomain == this->ParentDomain)) {
        for (int i=0; i<s->OutLink.size(); i++) {
            AdjNode=s->OutLink[i]->ToNode;
            ActualLink=s->GetLinkToNode(AdjNode);

            //This condition also avoid the utilization of LINK_DOWN
            if ((!visited[AdjNode->Id]) && (ActualLink->Status==UP)) {
                if (ElapsedDistance + this->DistanceMatrix[AdjNode->Id][d->Id] < MaxDistance) {
                    DepthFirstSearch_Domain(AdjNode,d,depth+1,ElapsedDistance+ActualLink->DistanceMetric,MaxDistance,visited,path,ltable);
                }
            }
        }
    }

    //Condizione di arresto iterazione
    if (s==d) {
        if (ElapsedDistance<=MaxDistance) {
            Path* P=new Path;

            P->Id = ltable.size();

            P->Src = path[0];
            P->Dst = path[path.size()-1];
            P->ParentDomain=P->Src->ParentDomain;

            P->nodes=path;

            for (int i=0; i<path.size()-1; i++) {
                ActualNode=path[i];
                NextNode=path[i+1];
                ActualLink=ActualNode->GetLinkToNode(NextNode);
                P->links.push_back(ActualLink);
            }
            ltable.push_back(P);
        }
    }
    if (depth==0) {
        //cout << "Number of iterations: " << iterations << endl;
        iterations=0;
    }
    return;
}

void CPathTable::DepthFirstSearch_HPCE(int s, int d, CNode* src_node, CNode* dst_node, int depth, int ElapsedDistance, int MaxDistance, vector<int> visited, vector<int> path, vector<Path*>& ltable) {
    CNode *AdjNode, *ActualNode, *NextNode;
    CLink *ActualLink;
    vector<CNode*> Logic_NodePath;
    vector<CNode*> NodePath;
    int s_id, d_id;
    CNet *net=this->ParentNet;
    int size=net->Node.size()+net->Domain.size();
    
    static int iterations=0;
    
    if (depth==0) {
      s_id=net->Node.size()+src_node->ParentDomain->Id;
      d_id=net->Node.size()+dst_node->ParentDomain->Id;
    }
    else {
      s_id=s;
      d_id=d;
    }

    #ifdef DEBUG
    cout << "DepthFirstSearch_HPCE " << src_node->Id << " " << dst_node->Id << " " <<  s << "->" << d << " --- " << s_id << "->" << d_id << " depth " << depth << endl; 
    #endif
    
    iterations++;
    visited[s_id]=1;//da inizializzare a 0 prima di entrare nella funzione
    path.push_back(s_id);

    //Condizione di iterazione
    if (s_id!=d_id) {
        for (int i=0; i<size; i++) {
            if (net->H[s_id][i]) {
                if (!visited[i]) {
                  if (ElapsedDistance + this->DistanceMatrix[i][d_id] < MaxDistance ) {
		    DepthFirstSearch_HPCE(i,d_id,src_node,dst_node,depth+1,ElapsedDistance+net->H[s_id][i],MaxDistance,visited,path,ltable);
                  }
                }
            }
        }
    }

    //Condizione di arresto iterazione
    if (s_id==d_id) {
        if (ElapsedDistance<=MaxDistance) {
            //Built the list of CNodes*
            for (int i=0; i<path.size(); i++) {
	      if (i==0) 
		Logic_NodePath.push_back(src_node);
	      else if (i==path.size()-1)
		Logic_NodePath.push_back(dst_node);
	      else 
                Logic_NodePath.push_back(this->ParentNet->Node[path[i]]);
            }
            //Remove duplicated nodes
            NodePath.push_back(Logic_NodePath[0]);
            for (int i=1; i<Logic_NodePath.size(); i++) {
	      if (Logic_NodePath[i]!=Logic_NodePath[i-1])
		NodePath.push_back(Logic_NodePath[i]);
	    }

            Path* P=new Path;

            P->Id = ltable.size();

            P->Src=NodePath[0];
            P->Dst=NodePath[NodePath.size()-1];
            P->ParentDomain=P->Src->ParentDomain;

            P->nodes=NodePath;

            for (int i=0; i<NodePath.size()-1; i++) {
                ActualNode=NodePath[i];
                NextNode=NodePath[i+1];
                ActualLink=ActualNode->GetInterDomainLinkToNode(NextNode);

                P->links.push_back(ActualLink);
            }

            ltable.push_back(P);
        }
    }
    if (depth==0) {
        //cout << "Number of iterations: " << iterations << endl;
        iterations=0;
    }
    return;
}

/* Fill up the distance matrix in terms of hops looking at the CDomain topology */
void CPathTable::LoadDistanceMatrix_Domain() {
    int nodes=this->ParentNet->Node.size();

    for (int i=0; i<NMAX; i++)
        for (int j=0; j<NMAX; j++) {
            this->DistanceMatrix[i][j]=MAX;
            this->DistanceMatrix[i][i] = 0;
        }

    for (int i=0; i<nodes; i++)
        if (this->ParentNet->Node[i]->ParentDomain == this->ParentDomain)
            this->ParentDomain->DijkstraDistance(this->ParentNet->Node[i],this->DistanceMatrix[i]);

    #ifdef DEBUG
    cout << "DistanceMatrix of Domain: " << this->ParentDomain->Name << endl;
    for (int i=0; i<nodes; i++) {
        for (int j=0; j<nodes; j++)
            cout << this->DistanceMatrix[i][j] << " ";
        cout << endl;
    }
    #endif
}

/* Fill up the distance matrix in terms of hops looking at the CNet topology */
void CPathTable::LoadDistanceMatrix_Network() {
    int nodes=this->ParentNet->Node.size();

    for (int i=0; i<NMAX; i++)
        for (int j=0; j<NMAX; j++)
            this->DistanceMatrix[i][j]=MAX;

    for (int i=0; i<nodes; i++)
        this->ParentNet->DijkstraDistance(this->ParentNet->Node[i],this->DistanceMatrix[i]);
}

/* Fill up the distance matrix in terms of hops looking at the CNet topology */
void CPathTable::LoadDistanceMatrix_HPCE() {
    int nodes=this->ParentNet->Node.size();
    int domains=this->ParentNet->Domain.size();

    for (int i=0; i<NMAX; i++)
        for (int j=0; j<NMAX; j++)
            this->DistanceMatrix[i][j]=MAX;

    for (int i=0; i<nodes+domains; i++)
        this->ParentNet->DijkstraDistance_HPCE(i,this->DistanceMatrix[i]);
}

int CPathTable::CollidingLinksOccurrence(vector<CLink*> PathLinks, vector<CLink*> CollidingLinks) {
    int CollidingLinksOccurrence=0;

#ifdef STDOUT_CPATHTABLE
    cout << "PCE routing PathLinks: ";
    for (int i=0; i<PathLinks.size(); i++) cout << PathLinks[i]->Id << " ";
    cout << "CollidingLinks: ";
    for (int i=0; i<CollidingLinks.size(); i++) cout << CollidingLinks[i]->Id << " ";
#endif

    for (int i=0; i<PathLinks.size(); i++) {
        for (int j=0; j<CollidingLinks.size(); j++)
            if (CollidingLinks[j]==PathLinks[i]) CollidingLinksOccurrence++;
    }

#ifdef STDOUT_CPATHTABLE
    cout << "Occurences: " << CollidingLinksOccurrence << endl;
#endif

    return CollidingLinksOccurrence;
}

void CPathTable::LinkInterArrival(double NetworkInterArrival) {
    int nodes=this->ParentNet->Node.size();
    double SrcDstInterArrival=NetworkInterArrival*nodes*(nodes-1);
    double percentile;

    percentile=((double) SrcDstInterArrival)/100;

    for (int i=0; i<this->table.size();i++) { 
        for (int j=0; j<this->table[i]->links.size(); j++) {
	  if (this->table[i]->links[j]!=NULL)
            this->table[i]->links[j]->LinkInterArrival+=((double) 1)/(this->table[i]->PathNumber);
        }
    }
    for (int i=0; i<this->ParentNet->Link.size();i++) {
        this->ParentNet->Link[i]->LinkInterArrival=SrcDstInterArrival/(this->ParentNet->Link[i]->LinkInterArrival);
    }
    
    //Writing the logfile
    this->ParentNet->LogFile << "------------------------------------------------------------" << endl;
    this->ParentNet->LogFile << "------------------------------------------------------------" << endl;
    this->ParentNet->LogFile << "Source-Destination pair Interarrival time [s]: " << SrcDstInterArrival << endl;
    this->ParentNet->LogFile << "Interarrival time at link [s]:" << endl;
    for (int i=0; i<this->ParentNet->Link.size();i++) {
        this->ParentNet->LogFile << i << ". " << this->ParentNet->Link[i]->FromNode->Name << "->" << this->ParentNet->Link[i]->ToNode->Name << ": ";
	this->ParentNet->LogFile << this->ParentNet->Link[i]->LinkInterArrival << endl;
        //this->ParentNet->LogFile << percentile * ((int) (this->ParentNet->Link[i]->LinkInterArrival/percentile)) << " 1" << endl;
    }
    this->ParentNet->LogFile << "------------------------------------------------------------" << endl;
    this->ParentNet->LogFile << "------------------------------------------------------------" << endl;
}

void CPathTable::Routes_PerLink(CLink* link, int *N, int *X) {
    int j;

    *N=0;

    for (int i=0; i<this->table.size(); i++) {
        j=0;
        while ((link!=this->table[i]->links[j]) && (j<this->table[i]->links.size())) j++;
        if (j<this->table[i]->links.size()) {
            *N=*N+1;
        }
    }
}

void CPathTable::LightPaths_PerLink(CLink* link, int *n, int *x) {
    int j;

    *n=0;

    for (int i=0; i<this->ParentNet->ActiveConns.size(); i++) {
        j=0;
        while ((link!=this->ParentNet->ActiveConns[i]->LinkPath[j]) && (j<this->ParentNet->ActiveConns[i]->LinkPath.size())) j++;
        if (j<this->ParentNet->ActiveConns[i]->LinkPath.size()) {
            *n=*n+1;
        }
    }
}


void CPathTable::SegmentRoutingComputation() {
  Path* ActualPath;
  
  //this->table.size()
  for (int i=0; i<this->table.size(); i++) {
    ActualPath=this->table[i];
    
    this->SegmentRouting_EqualPath(ActualPath, ActualPath->nodes, 0);
    //this->SegmentRouting_ForkPath(ActualPath);
    
  }
  
  //Statistic collection and post processing
  
  for (int i=0; i<this->table.size(); i++) {
      ActualPath=this->table[i];
      
      
      cout << "---------------------------------------------------" << endl;
      cout << "Target path: ";
      for (int j=0; j<ActualPath->nodes.size(); j++) {
	cout << ActualPath->nodes[j]->Id << " ";
      }
      cout << endl;
	
      cout << "Segment routing stack: ";
       for (int j=0; j<ActualPath->SegmentRouting.size(); j++) {
	cout << ActualPath->SegmentRouting[j]->Id << " ";
      }
      cout << endl;
      cout << "---------------------------------------------------" << endl;
  }
  
  return;
}


void CPathTable::SegmentRouting_EqualPath(Path* path, vector<CNode*> target_path, int depth) {
  CNet* net=this->ParentNet;
  vector<CNode*> target_segment;
  vector<CNode*> shortest_segment;
  vector<CNode*> next_target_path;
  vector<int> shortest_int;
  CNode* src = target_path[0];
  CNode* dst;
  int i;
  int debug_src_id=26;
  int debug_dst_id=24;
  
  
  if ((path->Src->Id==debug_src_id) && (path->Dst->Id==debug_dst_id)) {
    
    /*for (int j=0; j<net->Node.size(); j++){
      for (int k=0; k<net->Node.size(); k++)
	cout << net->G[j][k] << " ";
      cout << endl;
    }
    cout << endl;*/
    
    cout << "--- Depth: " << depth << " Target path: ";
      for (int j=0; j<target_path.size(); j++)
	cout << target_path[j]->Id << " ";
      cout << endl;
  }
    
  //Scansione del percorso e confronto con gli shortest
  for (i=1; i<target_path.size(); i++) {
    target_segment.clear();
    shortest_segment.clear();
    shortest_int.clear();
    
    for (int j=0; j<=i; j++) {
      target_segment.push_back(target_path[j]);
    }
    
    dst=target_segment[target_segment.size()-1];
    net->dijkstra(net->G,net->Node.size(),src->Id,dst->Id,&shortest_int);
    
    for (int j=0; j<shortest_int.size(); j++) {
      shortest_segment.push_back(net->GetNodePtrFromNodeID(shortest_int[j]));
    }
    
    if ((path->Src->Id==debug_src_id) && (path->Dst->Id==debug_dst_id)) {
    cout << i << ". Target segment: ";
    for (int j=0; j<target_segment.size(); j++)
      cout << target_segment[j]->Id << " ";
    cout << endl;
    
    cout << i << ". Shortest segment: ";
    for (int j=0; j<shortest_segment.size(); j++)
      cout << shortest_segment[j]->Id << " ";
    cout << endl;
    }
    
    if (!net->EqualPath(target_segment,shortest_segment)) {
      break;
    }
  }
  
  //Condizione di iterazione
  if (i < target_path.size()) {
    
    //Inserisce il nodo nello stack
    path->SegmentRouting.push_back(target_path[i-1]);
    
    for (int j=i-1; j<target_path.size(); j++) {
      next_target_path.push_back(target_path[j]);
    }
    
    this->SegmentRouting_EqualPath(path, next_target_path, depth+1);
  }
  
  //Condizione di arresto iterazione
  if (i == target_path.size()) {
    path->SegmentRouting.push_back(target_path[target_path.size()-1]);
  }
  
  return;
}

int CPathTable::Flexible_LINKS_AggInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector <CLink*>* pathlink,CConnection* rconn) {
    vector<Path*> ltable, lltable;   //local database of paths between nodes s and d
    vector<int> ltable_weight;
    Path* ActualPath;
    int LabelSetTest, MaxWeight, rand_id, lambdas=this->ParentNet->W;
    int required_slots=rconn->width_slots;
    int LinkBottleneck, PathBottleneck;

    #ifdef STDOUT_CPATHTABLE
    cout << "Flexible_LINKS_AggInfo_LeastCongestedPath conn id " << rconn->Id << " computing path " << s->Id << "->" << d->Id << " TIME: " << this->ParentNet->Now << endl;
    #endif

    //Computes the set of candidate paths and inserts them in the ltable
    this->CNet_ComputeCandidatePaths(s,d,ltable,rconn);

    //Initializes ltable_weight
    for (int i=0; i<ltable.size(); i++)
        ltable_weight.push_back(0);

    //Computes the ltable_weight based on the LinkState_dB of the node (source node or PCE node)
    if (ltable.size()) {
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            ActualPath=ltable[lt_id];
            PathBottleneck=lambdas;
            LinkBottleneck=0;

	    //Check the links availability in the physical links
            for (int i=0; i<ActualPath->links.size(); i++) {

	      LinkBottleneck=0;
	      for (int k=0; k<lambdas; k++) {
		LinkBottleneck+=(1-ActualPath->links[i]->LambdaStatus[k]);
	      }
              if (LinkBottleneck<PathBottleneck) PathBottleneck=LinkBottleneck;
              //cout << "conn id " << rconn->Id  << " PATH: " << lt_id << " LinkBottleneck [" << i << "]: " << LinkBottleneck << endl;
            }
            ltable_weight[lt_id]=PathBottleneck;
	    //cout << "--- conn id " << rconn->Id  << " PATH: " << lt_id << " PathBottleneck " << PathBottleneck << endl;
        }
        
        #ifdef STDOUT_CPATHTABLE
        cout << "AggInfo_LeastCongestedPath conn id " << rconn->Id << " weights: " << flush;
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            cout << ltable_weight[lt_id] << " ";
        }
        #endif
        
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    //Computes the MaxWeight
    MaxWeight=ltable_weight[0];
    for (int lt_id=1; lt_id<ltable.size(); lt_id++)
        if (ltable_weight[lt_id]>MaxWeight) MaxWeight=ltable_weight[lt_id];

    #ifdef STDOUT_CPATHTABLE
    cout << " max: " << MaxWeight << endl;
    #endif

    if (MaxWeight==0) return 0;

    //Builds lltable starting from ltable
    for (int i=0; i<ltable.size(); i++)
        if (ltable_weight[i]==MaxWeight)
            lltable.push_back(ltable[i]);

    #ifdef STDOUT_CPATHTABLE
    cout << "Flexible_LINKS_AggInfo_LeastCongestedPath conn id " << rconn->Id << " lltable size: " << lltable.size() << flush;
    #endif

    //Randomly selects one of the paths in the ltable
    if (lltable.size()) {
        rand_id=randint(0,(lltable.size()-1),&this->ParentNet->PathSelectionSeed);
	
	#ifdef STDOUT_CPATHTABLE
    cout << " random_id: " << rand_id << endl;
	#endif
	
	rconn->ActualPath.clear();
	rconn->ActualPath.push_back(lltable[rand_id]);
	
    for (int j=0; j<(lltable[rand_id]->nodes.size()); j++)
        pathnode->push_back(lltable[rand_id]->nodes[j]);
    for (int j=0; j<(lltable[rand_id]->links.size()); j++)
        pathlink->push_back(lltable[rand_id]->links[j]);
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }
    return 1;
}
    

int CPathTable::Flexible_LINKS_DetInfo_LeastCongestedPath(CNode* s, CNode* d, vector<CNode*>* pathnode, vector <CLink*>* pathlink,CConnection* rconn) {
    vector<Path*> ltable, lltable;   //local database of paths between nodes s and d
    vector<int> ltable_weight;
    Path* ActualPath;
    int LabelSetTest, MaxWeight, rand_id, lambdas=this->ParentNet->W, j;
    int VirtualLabelSet[lambdas];
    int required_slots=rconn->width_slots;

    for (int i=0; i<lambdas; i++) VirtualLabelSet[i]=1;

    #ifdef STDOUT_CPATHTABLE
    cout << "Flexible_LINKS_DetInfo_LeastCongestedPath conn id " << rconn->Id << " computing path " << s->Id << "->" << d->Id << " TIME: " << this->ParentNet->Now << endl;
    #endif

    //Computes the set of candidate paths and inserts them in the ltable
    this->CNet_ComputeCandidatePaths(s,d,ltable,rconn);

    //Initializes ltable_weight
    for (int i=0; i<ltable.size(); i++)
        ltable_weight.push_back(0);

    //Computes the ltable_weight based on the LinkState_dB of the node s
    if (ltable.size()) {
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            ActualPath=ltable[lt_id];
	    
            for (int i=0; i<lambdas; i++)
	      VirtualLabelSet[i]=1;

            //Check the links availability in the physical links
            for (int i=0; i<ActualPath->links.size(); i++) {
                for (int k=0; k<lambdas; k++) {
                    if ((VirtualLabelSet[k]==1) && (ActualPath->links[i]->LambdaStatus[k]==1)) VirtualLabelSet[k]=0;
                }
            }
            
            //Compute the weights considering the slots required by the connection
            for (int i=0; i<=lambdas-required_slots; i++) {
	      LabelSetTest=0;
	      for (int k=0;k<required_slots;k++) {
		if(VirtualLabelSet[i+k]==1)
		  LabelSetTest++;
	      }
	      if (LabelSetTest==required_slots)
		ltable_weight[lt_id]++;
	    }
        }
        
	#ifdef STDOUT_CPATHTABLE
        cout << "Flexible_LINKS_DetInfo_LeastCongestedPath conn id " << rconn->Id << " weights: " << flush;
        for (int lt_id=0; lt_id<ltable.size(); lt_id++) {
            cout << ltable_weight[lt_id] << " ";
        }
	#endif
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }

    //Computes the MaxWeight
    MaxWeight=ltable_weight[0];
    for (int lt_id=1; lt_id<ltable.size(); lt_id++)
        if (ltable_weight[lt_id]>MaxWeight) MaxWeight=ltable_weight[lt_id];

    #ifdef STDOUT_CPATHTABLE
    cout << " max: " << MaxWeight << endl;
    #endif

    if (MaxWeight==0) return 0;

    //Builds lltable starting from ltable
    for (int i=0; i<ltable.size(); i++)
        if (ltable_weight[i]==MaxWeight)
            lltable.push_back(ltable[i]);

    #ifdef STDOUT_CPATHTABLE
    cout << "Flexible_LINKS_DetInfo_LeastCongestedPath conn id " << rconn->Id << " lltable size: " << lltable.size() << flush;
    #endif

    //Randomly selects one of the least congested paths in the lltable
    //Randomly selects one of the paths in the ltable
    //this->ParentNet->TableSize->AddSample_Distribution(lltable.size());
    //this->ParentNet->TableSize->AddSample(lltable.size());

    if (lltable.size()) {
        rand_id=randint(0,(lltable.size()-1),&this->ParentNet->PathSelectionSeed);
	
	#ifdef STDOUT_CPATHTABLE
        cout << " random_id: " << rand_id << endl;
	#endif
	
	rconn->ActualPath.clear();
	rconn->ActualPath.push_back(lltable[rand_id]);
	
        for (int j=0; j<(lltable[rand_id]->nodes.size()); j++)
            pathnode->push_back(lltable[rand_id]->nodes[j]);
        for (int j=0; j<(lltable[rand_id]->links.size()); j++)
            pathlink->push_back(lltable[rand_id]->links[j]);
    }
    else {
        if (rconn->ProvisioningAttempt==1) ErrorMsg("No paths between a source destination pair");
        else return 0;
    }
    return 1;
}

//Estimate the LabelSet along the computed path (list of links), considers actual resources on the LINKS
int CPathTable::Flexible_LINKS_WavelengthAssignmentFF(vector<CLink*> pathlink, int required_slots, int conn_id, int rest_flag, CConnection* conn) {
  int SelectedSlotBase;
  vector<int> CandidatesSlotBase;
  int slots;
  int spectrum_test;
  slots=this->ParentNet->W;
  int virtual_label_set[slots];
  int j;
  int SmartLabel;
  
  #ifdef STDOUT
  cout << "NODE: " << this->ParentNode->Id << " (" << this->ParentNode->Name << "): conn id " << conn_id << " Flexible_LINKS_WavelengthAssignmentFF" << endl;
  #endif
  
  //Initialization
  for (int i=0; i<slots; i++) {
    virtual_label_set[i]=1;
  }
  
  //Propagation of label set on the list of links
  for (int i=0; i<pathlink.size(); i++) {
    j=0; 
    while (pathlink[i]!=this->ParentNet->Link[j]) {j++;};
    
    for (int k=0; k<slots; k++) {
      if ((virtual_label_set[k]==1) && (this->ParentNet->Link[j]->LambdaStatus[k]>0))
        virtual_label_set[k]=0;
    }
  }
  
  //Fill the candidate slot base
  for (int i=0; i<=slots-required_slots; i++) {
    spectrum_test=0;
    
    //If virtual label set is 1 the lambda is available
    for (int j=0; j<required_slots; j++) 
      if (virtual_label_set[i+j]==1) spectrum_test++;
    
    if (spectrum_test==required_slots)
      CandidatesSlotBase.push_back(i);
  }

  if ((rest_flag==1) && (this->ParentNet->SmartRestorationMapping==1)) {
    SmartLabel = this->ParentNet->W - conn->OldLabel - conn->width_slots + 1;
	  
    for (int i=0; i<CandidatesSlotBase.size(); i++) {
      if (CandidatesSlotBase[i]==SmartLabel)
	return SmartLabel;
    }
  }
  
  if (CandidatesSlotBase.size()!=0) {
    #ifdef STDOUT
    cout << "NODE: " << this->ParentNode->Id << " (" <<this->ParentNode->Name << "): ALIEN conn id " << conn_id;
    cout << " Suggested base slot: " << CandidatesSlotBase[0];
    cout << " RequiredSlots: " << required_slots << endl;
    #endif
    
    //Use first-fit
    return CandidatesSlotBase[0];
  }
  else {
    return -1;
  }
}
















