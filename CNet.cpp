#include "CNet.h"
#include "Statistics.h"
#include "PCx_Node.h"
#include "CConnection.h"
#include "CDomain.h"
#include "CLink.h"

CNet::CNet() {
    Tree=new BHTree(this);

    Now=0.0;
    PckBlocked=0;

    //Initialisation of provisioning statistics
    ConnRequested=ConnRequestedREP=ConnAdmittedPath=ConnAdmittedPathREP=ConnRefused=ConnRefusedREP=ConnEstablished=ConnEstablishedREP=ConnReleased=0;
    ConnBlocked=ConnBlockedREP=ConnBlockedRouting=ConnBlockedRoutingREP=ConnBlockedForward=ConnBlockedForwardREP=ConnBlockedBackward=ConnBlockedBackwardREP=0;
    ConnBlocked_EmptyLabelSet=0;
    ConnBlocked_Collision=ConnBlocked_LinkDown=0;
    
    //Initialisation of restoration statistics
    ConnDisrupted=ConnDisruptedREP=ConnAdmittedPath_Restoration=ConnAdmittedPath_RestorationREP=ConnRestored=ConnRestoredREP=0;
    ConnRefused_Restoration=ConnRefused_RestorationREP=ConnRefusedRouting_Restoration=ConnRefusedRouting_RestorationREP=ConnRefusedMaxAtt_Restoration=ConnRefusedMaxAtt_RestorationREP=0;
    ConnBlocked_Restoration=ConnBlocked_RestorationREP=ConnBlockedForward_Restoration=ConnBlockedForward_RestorationREP=ConnBlockedBackward_Restoration=ConnBlockedBackward_RestorationREP=0;
    ConnBlocked_EmptyLabelSet_Restoration=0;
    ConnBlocked_Collision_Restoration=ConnBlocked_LinkDown_Restoration=0;
    
    //Initialisation of packets statistics
    RSVP_PckCounter=0;
    RSVP_PckCounterREP=0;
    OSPF_PckCounter=0;
    OSPF_PckCounterREP=0;
    PCEP_PckCounter=0;
    PCEP_PckCounterREP=0;
    OFP_PckCounter=0;
    OFP_PckCounterREP=0;
    TOT_PckCounter=0;
    TOT_PckCounterREP=0;
    TOT_ControllerPckCounter=0;
    TOT_ControllerPckCounterREP=0;
    
    TimeStartREP=0;
    TimeForRep=0;

    this->InterArrivalTime=new CStat(0,200,2000,"arrival.out");
    this->HoldingTime=new CStat(0,5000,50000,"holding.out");
    this->TableSize=new CStat(0,150,150,"table.out");
    this->LabelSetSize=new CStat(0,40,40,"labelset.out");

    this->TotSignTimeREP=0;
    this->TotSignTimeREP_1=0;
    this->TotSignTimeREP_2=0;
    this->TotSignTimeREP_3=0;
    this->TotSignTimeREP_4=0;
    this->TotSignTimeREP_5=0;
    
    this->TotRestTimeREP=0;
    this->MaxRestTimeREP=0;
    
    this->SimEnd=0;
    this->Experiment_ID=0;

    this->W=0;
    this->GeneratedLSA=0;
    this->ReceivedLSA=0;
    this->ForwardedLSA=0;
    this->DroppedLSA=0;
    this->ActiveLSA=0;
    this->TooFrequentLSA=0;
    this->OutOfDateLSA=0;
    this->DuplicateLSA=0;
    this->SelfOriginatedLSA=0;
    this->PostponedLSA=0;
    this->ImmediateLSA=0;

    //this->OSPF_MinLSInterval=5;
    this->OSPF_MinLSArrival=0;

    this->last_failure_generation_time=0.0;
    this->FailureCounter=0;

    this->HPCE_Node=NULL;
    
    this->distr_routing_time = 10e-3;
    this->distr_wa_time = 0.1e-3;
    this->pce_routing_time = 20e-3;
    this->pce_wa_time = 0.1e-3;
    this->oxc_crossconnection_time = 50e-3;
    
    ifstream conf_file;

    //Prime numbers of 9 digits are used for seeds
    InterarrivalSeed=160481219;
    SrcDstSeed=179424673;
    DurationSeed=179424691;
    WavelengthAssignmentSeed=198491317;
    PathSelectionSeed=198491329;
    LinkFailureSeed=217645177;
    OSPF_FirstGenerationSeed=179424691;
    RequiredSlotsSeed=198491317;

    string logfile="dalay.log";
    cout << "\t...opening output file:" << logfile << "...";
    this->LogFile.open(logfile.c_str(),ios::out);
    if (!(this->LogFile.is_open())) {
        ErrorMsg("It is not possible to open the log file");
    }
    cout << "[ok]" << endl;
}

CNet::~CNet() {
    /*this->Node.clear();
    this->Link.clear();*/

    if (this->ActiveConns.size())
        ErrorMsg("There were ActiveConns at the end of the simulation");
}

void CNet::genEvent(CEvent* eve) {
    Tree->Add(eve);
}

CDomain* CNet::GetDomainPtrFromDomainID(int ID) {
    int i=0;
    while (Domain[i]->Id!=ID) {
        i++;
    }
    return Domain[i];
}

// note: the ID is the CNode.id and not the sequence number of the vector's element.
CNode* CNet::GetNodePtrFromNodeID(int ID) {
  for (int i=0; i<this->Node.size(); i++) {
    if (Node[i]->Id==ID)
      return Node[i];
  }
  
  cout << "Node not found - link id " << ID << endl;
  ErrorMsg("GetLinkPtrFromLinkID: link not found");
  return NULL;
}

CLink* CNet::GetLinkPtrFromLinkID(int ID) {
  for (int i=0; i<this->Link.size(); i++) {
    //cout << Link[i]->Id << endl;
    if (Link[i]->Id==ID)
      return Link[i];
  }
  
  cout << "Link not found - link id " << ID << endl;
  ErrorMsg("GetLinkPtrFromLinkID: link not found");
  return NULL;
}

//If link is not present it returns NULL
CLink* CNet::GetLinkPtrFromNodesID(int a, int b) {
  if (a==b)
    return NULL;
  
  for (int i=0; i<this->Link.size(); i++)
    if ((Link[i]->FromNode->Id==a) && (Link[i]->ToNode->Id==b))
      return Link[i];
    
  ErrorMsg("GetLinkPtrFromNodes: link not found");
  return NULL;
}

CLink* CNet::GetLinkPtrFromNodes(CNode* s, CNode* d) {
  for (int i=0; i<Link.size(); i++)
    if ((this->Link[i]->FromNode==s) && (this->Link[i]->ToNode==d))
      return Link[i];
    
  ErrorMsg("GetLinkPtrFromNodes: link not found");
  return NULL;
}

CNode* CNet::CreateNode(int id, string name) {
    PCx_Node* node=new PCx_Node(id,name,this);
    this->Node.push_back(node);

    if (id==this->HPCE_NodeId) {
        this->HPCE_Node=node;
    }

    return node;
}

void CNet::execute() {
    cout << "Starting simulation..." << endl;
    Tree->execute();
    cout << "Simulation ended...[ok]" << endl;
}

//Provide shortest path routing by Dijkstra
//If a path is available between x and y, returns the weight of the path, otherwise return 0
int CNet::dijkstra(int G[NMAX][NMAX], int nodi, int x, int y, vector<int>* path) {
    int somma=0, j=0, temp, m;

    vector<int> u(nodi);	//Vettore dei predecessori
    vector<int> d(nodi);	//Segna peso minimo con cui si raggiunge quel nodo
    vector<int> v(nodi);	//Segna nodi visitati.....1 visitato 0 non visitato

    for (int i=0; i<nodi; i++) {
        d[i]=MAX;
        v[i]=0;
        u[i]=MAX;
    }

    v[x]=1;
    d[x]=0;

    //Sistemo il vector d per i nodi direttamente raggiungibili da x
    for (int i=0;i<nodi;i++)
        if (G[x][i]) {
            d[i]=G[x][i];
            u[i]=x;
        }

    for (int i=0;i<nodi;i++)
        somma=somma+d[i];

    //Se questa condizione è verificata il nodo x è isolato
    if (somma==((nodi-1)*MAX)) return 0;

    for (;;) {
        m=MAX;
        for (int i=0;i<nodi;i++) //sceglie il prossimo nodo da visitare
            if (!v[i] && d[i]<m) { //cerca l'indice di d con minimo d[i]
                m=d[i];
                j=i;
            }
        v[j]=1;

        if (j==y) break;
        if (m==MAX) return 0;

        for (int i=0; i<nodi; i++)
            if (G[j][i] && d[i] > d[j]+G[j][i]) {
                d[i]=d[j]+G[j][i];
                u[i]=j;
            }
    }

    //Interfaccia per restituire in uscita il vettore path nell'ordine giusto
    int i=0;
    vector<int> path_rev;
    temp=y;
    path_rev.push_back(temp);

    do {
        temp=u[temp];
        path_rev.push_back(temp);
        i++;
    }
    while (temp!=x);

    for (int k=0; k<path_rev.size(); k++) {
        path->push_back(path_rev[i]);
        i--;
    }
    return d[y];
}

/* Computes the distance in number of hops from s to each node in the network and write it in the DistanceTree */
int CNet::DijkstraDistance(CNode* s, int DistanceTree[]) {
    int m, j=0, visited=0, nodi=this->Node.size();

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

    //Sistemo il vector d per i nodi direttamente raggiungibili da x
    for (int i=0; i<s->OutLink.size(); i++) {
      if (s->OutLink[i]->Status==UP) { // Non considera link guasti
        l=s->OutLink[i];
        n=s->OutLink[i]->ToNode;

        d[n->Id]=l->DistanceMetric;
        u[n->Id]=s->Id;
      }
    }

    do {
        m=MAX;
        for (int i=0;i<nodi;i++) //sceglie il prossimo nodo da visitare
            if (!v[i] && d[i]<m) { //cercando l'indice di d con minimo d[i]
                m=d[i];
                j=i;
            }
        v[j]=1;
        visited++;

        for (int i=0; i<this->Node[j]->OutLink.size(); i++) {
	  if (this->Node[j]->OutLink[i]->Status==UP) { // Non considera link guasti
            l=this->Node[j]->OutLink[i];
            n=this->Node[j]->OutLink[i]->ToNode;

            if (d[n->Id] > d[j]+l->DistanceMetric) {
                d[n->Id]=d[j]+l->DistanceMetric;
                u[n->Id]=j;
            }
	  }
        }
    } while (visited < nodi);

#ifdef DEBUG
    cout << "DijkstraDistances:";
    for (int k=0; k<nodi; k++)
        cout << " " << d[k];
    cout << endl;
#endif

    for (int k=0; k<nodi; k++)
        DistanceTree[k]=d[k];

    return 1;
}

/* Computes the distance in number of hops from s to each node in the network and write it in the DistanceTree */
//It can be used with F or H matrices
int CNet::DijkstraDistance_HPCE(int s, int DistanceTree[]) {
    int m, j=0, visited=0;
    int size=this->Node.size()+this->Domain.size();

    vector<int> u;//Vettore dei predecessori
    vector<int> d;//Segna peso minimo con cui si raggiunge quel nodo
    vector<int> v;//Segna nodi visitati.....1 visitato 0 non visitato
    
    for (int i=0; i<size; i++) {
        u.push_back(MAX);
        d.push_back(MAX);
        v.push_back(0);
    }
    v[s]=1;
    d[s]=0;
    visited=1;

    //Sistemo il vector d per i nodi direttamente raggiungibili da x
    for (int i=0; i<size; i++) {
        if (H[s][i]) {
            d[i]=H[s][i];
            u[i]=s;
        }
    }

    do {
        m=MAX;
        for (int i=0;i<size;i++) //sceglie il prossimo nodo da visitare
            if (!v[i] && d[i]<m) { //cercando l'indice di d con minimo d[i]
                m=d[i];
                j=i;
            }
        v[j]=1;
        visited++;

        for (int i=0; i<size; i++) {
            if (H[j][i] && d[i] > d[j]+H[j][i]) {
                d[i]=d[j]+H[j][i];
                u[i]=j;
            }
        }
    } while (visited < size);
    
    #ifdef DEBUG
    cout << "DijkstraDistances_HPCE:";
    for (int k=0; k<size; k++)
        cout << " " << d[k];
    cout << endl;
    #endif

    for (int k=0; k<size; k++)
        DistanceTree[k]=d[k];

    return 1;
}

//Utilizes dijkstra algorithm to update the domains database
//To load domain dB utilizing dijkstra algorithm
void CNet::InterDomain_RoutingScheme() {
    int peso;

    this->LogFile << "\nLoading inter-domain routes using the D matrix at the inter-domain level..." << endl;
    cout << "\t...performing dijkstra algorithm at the inter-domain level...";

    for (int i=0;i<Domain.size();i++) {
        for (int j=0;j<Domain.size();j++)
            if (i!=j) {
                vector<int> path;
                peso=this->dijkstra(D,Domain.size(),Domain[i]->Id,Domain[j]->Id,&path);
                if (peso==0) {
		  cout << "Inter-domain path: " << i << "->" << j << endl;
		  ErrorMsg("CNet->InterDomain_RoutingScheme... an inter-domain route does not exist");
		}
                else {
                    this->Domain[i]->RoutingTable_AddRecord(Domain[j],GetDomainPtrFromDomainID(path[1]),peso);

                    this->LogFile << "From: " << Domain[i]->Name << " to: " << Domain[j]->Name << "\tPath: ";
                    for (int k=0;k<path.size();k++) this->LogFile << path[k] << ' ';
                    this->LogFile << "\tPeso: " << peso << endl;
                }
            }
    }
    cout << "[ok]" << endl;
    this->LogFile << "Inter-domain routes loaded." << endl << endl;
}

//Utilizes dijkstra algorithm to update the nodes database
//To load Nodes dB utilizing dijkstra algorithm
void CNet::NetworkLevel_RoutingScheme() {
    int peso;

    this->LogFile << "\nLoading routes using the G matrix at the network level..." << endl;
    cout << "\t...performing dijkstra algorithm at the network level...";
    for (int i=0;i<Node.size();i++) {
        for (int j=0;j<Node.size();j++)
            if (i!=j) {
                vector<int> path;
                peso=this->dijkstra(G,Node.size(),Node[i]->Id,Node[j]->Id,&path);
                if (peso==0) ErrorMsg("CNet->NetworkLevel_RoutingScheme... a route does not exist");
                else {
                    Node[i]->RoutingTable_AddRecord(Node[j],peso,Node[i]->GetLinkToNode(Node[path[1]]));
                    this->LogFile << Node[i]->Id << "->" << Node[j]->Id << "\tpath: ";
                    for (int k=0;k<path.size();k++) this->LogFile << path[k] << ' ';
                    this->LogFile << "\tPeso: " << peso << endl;
                }
            }
    }
    cout << "[ok]" << endl;
    this->LogFile << "Routes loaded at the network level." << endl << endl;
}

//Utilizes dijkstra algorithm to update the nodes database using the D matrix of the specific Domain
//To load Nodes dB utilizing dijkstra algorithm
void CNet::DomainLevel_RoutingScheme() {
    int peso;
    int min_peso;
    vector<int> path;
    vector<int> min_path;

    this->LogFile << "\nLoading routes using the G matrices at the domain level..." << endl;
    cout << "\t...performing dijkstra algorithm at the domain level...";
    for (int k=0;k<this->Domain.size();k++) {
        CDomain* ActualDomain = this->Domain[k];
        for (int i=0;i<ActualDomain->Node.size();i++) {
            for (int j=0;j<this->Node.size();j++) {
	      
                if (ActualDomain->Node[i]!=this->Node[j]) {
                    path.clear();
                    peso=this->dijkstra(ActualDomain->G,this->Node.size(),ActualDomain->Node[i]->Id,this->Node[j]->Id,&path);

                    if (peso!=0) { // the destination node was inside the domain or at the edge of an adjacent domain
                        ActualDomain->Node[i]->RoutingTable_AddRecord(this->Node[j],peso,ActualDomain->Node[i]->GetLinkToNode(this->Node[path[1]]));
                        ActualDomain->Node[i]->Destination_DomainNode_Table_AddRecord(this->Node[j],this->Node[j]);

                        this->LogFile << ActualDomain->Node[i]->Id << "->" << this->Node[j]->Id << "\tpath: ";
                        for (int l=0;l<path.size();l++)
                            this->LogFile << path[l] << ' ';
                        this->LogFile << "\tPeso: " << peso << endl;
                    }
                    else {
                        CDomain* DestinationDomain = this->Node[j]->ParentDomain;
                        CDomain* NextDomain;
                        min_peso = MAX;

                        int index = 0;
                        while ((DestinationDomain != ActualDomain->DomainRoutingTable_db[index].DestinationDomain) && (index < ActualDomain->DomainRoutingTable_db.size()))
                            index++;
                        if (index == ActualDomain->DomainRoutingTable_db.size())
                            ErrorMsg("CNet::DomainLevel_RoutingScheme()... No way to the destination domain.");
                        NextDomain = ActualDomain->DomainRoutingTable_db[index].NextDomain;

                        for (index=0; index<ActualDomain->InterDomain_OutLink.size(); index++) {
                            CNode* EdgeNode = ActualDomain->InterDomain_OutLink[index]->ToNode;
                            if (EdgeNode->ParentDomain == NextDomain) {
                                path.clear();
                                peso=this->dijkstra(ActualDomain->G,this->Node.size(),ActualDomain->Node[i]->Id,EdgeNode->Id,&path);

                                if (peso == 0) ErrorMsg("CNet::DomainLevel_RoutingScheme()... No way to the edge node... dalay_panic");
                                if (peso < min_peso) {
                                    min_peso = peso;

                                    min_path.clear();
                                    min_path = path;
                                }
                            }
                        }
                        
                        ActualDomain->Node[i]->RoutingTable_AddRecord(this->Node[j],min_peso,ActualDomain->Node[i]->GetLinkToNode(this->Node[min_path[1]]));
                        ActualDomain->Node[i]->Destination_DomainNode_Table_AddRecord(this->Node[j],this->Node[min_path[min_path.size()-1]]);

                        this->LogFile << ActualDomain->Node[i]->Id << "->" << this->Node[j]->Name << " (InterDomain path): ";
                        for (int l=0;l<min_path.size();l++)
                            this->LogFile << min_path[l] << ' ';
                        this->LogFile << "\tPeso: " << min_peso << endl;
                    }
                }
                else
                    ActualDomain->Node[i]->Destination_DomainNode_Table_AddRecord(this->Node[j],this->Node[j]);
            }
        }
    }
    cout << "[ok]" << endl;
    this->LogFile << "Routes loaded at the domain level." << endl << endl;
}

int CNet::EqualLink(vector<int> path1, vector<int> path2) {
    int NumberEqualLink=0;

    for (int i=1; i<path1.size(); i++) {
        for (int j=1; j<path2.size(); j++) if ((path1[i-1]==path2[j-1]) && (path1[i]==path2[j])) NumberEqualLink++;
    }

    return NumberEqualLink;
}


/*-------------------------------------------------------------------------------------------------
Parametri
	numero minimo e massimo di ripetizioni da effettuare
	intervallo di confidenza e livello di confidenza desiderati in percentuale del valore medio
	vettore dei dati: un elemento per ogni ripetizione
Scrive
	in mu il valor medio
	in sigma2 la varianza
	in CI il raggio dell'intervallo di confidenza calcolato
Restituisce
	-1  se non siamo ancora a MinRep ripetizioni non calcola niente
	 1  se le ripetizioni sono superiori a MinRep e l'intervallo calcolato soddisfa livello di confidenza richiesto
	-2  se le ripetizioni sono tra MinRep e MaxRep ma l'intervallo calcolato non soddisfa livello di confidenza richiesto
	 0  se ha superato MaxRep senza soddisfare ci cl
----------------------------------------------------------------------------------------------------*/
int CNet::ConfidenceInterval(int MinRep, int MaxRep, int ci, int cl, vector<double> data, double *mu, double *sigma2, double *CI) {

    double zeta;
    double Relative_CI;

    #ifdef STDOUT
    //cout << "Punti vettore: " << data.size() << " ";
    #endif

    if (data.size()<MinRep) return -1;

    switch (cl) {
    case 80:
        zeta=1.28;
        break;
    case 85:
        zeta=1.44;
        break;
    case 90:
        zeta=1.645;
        break;
    case 95:
        zeta=1.96;
        break;
    case 99:
        zeta=2.575;
        break;
    default:
        ErrorMsg("Confidence level not supported !");
    }

    #ifdef STDOUT
    //cout << " Required confidence level: " << ci << " -> ";
    #endif
    
    /********************** Computation *******************************/
    
    #ifdef STDOUT
    cout << "DATA: ";
    for (int i=0; i<data.size(); i++) {
      cout << data[i] << " ";
    }
    cout << endl;
    #endif
    
    *mu=0;
    for (int i=0; i<data.size(); i++) 
        *mu+=data[i];
    *mu=*mu/data.size();
    
    *sigma2=0;
    for (int i=0; i<data.size(); i++)
        *sigma2+=pow(data[i]-*mu,2);
    *sigma2=*sigma2/(data.size()-1);

    *CI=zeta*sqrt(*sigma2/(data.size()));
    if (*mu!=0) Relative_CI=(2*(*CI))/(*mu);
    else Relative_CI=0;

    #ifdef STDOUT
    //cout << "Calcolo confidenza: " << *mu << '\t' << *CI << '\t' << Relative_CI << endl;
    #endif
    /******************************************************************/

    if (Relative_CI*100 < ci) return 1;
    
    if (data.size() < MaxRep) return -2;
    else return 0; //Equivalent to --- if (data.size()>= MaxRep) return 0;
}

/*Files converter from G[][] topology matrix to file .topo Valcarenghi style*/
void CNet::FileTopologyBuilder(int G[NMAX][NMAX], string TopoPath) {

    fstream topo;
    char FileName[200];

    strcpy(FileName,this->Name);
    strcat(FileName,".topo");

    topo.open(TopoPath.c_str(),ios::out);

    topo << "% File: " << FileName << endl;
    topo << "% Network topology under simulation: " << this->Name << endl;
    topo << "% Creator: automatic generation " << FileName << "logy -> G[][] -> " << FileName << endl;
    topo << "% Timestamp: " << __TIME__ << " " << __DATE__ << endl;
    topo << "% Record Structure:\n%\tNode ID Number\n%\tNode ID String\n%\tNode Type\n%\tx y coordinates\n%\tNumber of outgoing links\n%\tList of:\n%\tadjacent node, distance to adjacent node\n" << endl;

    topo << "% Number of nodes" << endl << this->Node.size() << "\n\n";

    for (int i=0; i<this->Node.size(); i++) {
        topo << "% Record " << i << endl;
        topo << i << endl;
        topo << "x\nr\n0.0 0.0\n";
        int OutgoingLinks=0;
        for (int j=0; j<this->Node.size() ; j++)
            if (G[i][j]>0) OutgoingLinks++;
        topo << OutgoingLinks << endl;
        for (int j=0; j<this->Node.size() ; j++)
            if (G[i][j]>0) topo << j << ' ' << G[i][j] << endl;
        topo << endl;
    }
    return;
}

CDomain* CNet::CreateDomain(int id, string name, int pce_id, int* node_list) {
    vector<CNode*> domain_nodes;
    CNode* node;
    CNode* pce_node;
    PCx_Node* pce_domain_node;
    CDomain* domain;

    int i;

    i=0;
    while (node_list[i] != -1) {
        node = this->GetNodePtrFromNodeID(node_list[i]);
        //Checking if the node is is not included in other domains
        for (int j=0; j<this->Domain.size(); j++) {
            for (int k=0; k<this->Domain[j]->Node.size();k++) {
                if (node == this->Domain[j]->Node[k])
                    ErrorMsg("A node belongs to two domains... check the .topology file");
            }
        }

        domain_nodes.push_back(node);
        i++;
    }

    if (pce_id == -1)
        pce_domain_node = NULL;
    else {
        pce_node = this->GetNodePtrFromNodeID(pce_id);
        pce_domain_node = static_cast <PCx_Node*> (pce_node);
        pce_domain_node->Type=PCC_PCE;
        pce_domain_node->pce_wa_enabled=1;

        i=0;
        while ((pce_node != domain_nodes[i]) && (i < domain_nodes.size())) i++;
        if (i == domain_nodes.size())
            ErrorMsg("The PCE node does not belong to the domain... check the .topology file");
    }

    //If the domain does not contain nodes it is not inserted in the CNet
    if (domain_nodes.size()>=1) {
        domain=new CDomain(id,name,pce_domain_node,domain_nodes,this);
        this->Domain.push_back(domain);
    }

    return domain;
}


//Used to print the input data
void CNet::print_inputs() {
  cout << " inputs:"; 
  cout << " " << this->MeanInterarrivalTime; 
  cout << " " << this->MeanDurationTime;
  cout << " " << this->pce_routing_time;
  cout << " " << this->oxc_crossconnection_time;
}

//Compares two vectors of node pointers. Return 1 if they are equal.
int CNet::EqualPath(vector<CNode*> path1, vector<CNode*> path2) {
  if ((path1[0] != path2[0]) || (path1[path1.size()-1] != path2[path2.size()-1]))
    ErrorMsg("EqualPath, path1 and path2 must have the same source and destination");

  if (path1.size() != path2.size())
    return 0;
  
  for (int i=0; i<path1.size(); i++) {
    if (path1[i] != path2[i])
      return 0;
  }
  
  return 1;
}

//Returns the first node in path which is not included in the shortest, if path are equal it returns NULL
CNode* CNet::ForkPath(vector<CNode*> path, vector<CNode*> shortest) {
  if ((path[0] != shortest[0]) || (path[path.size()-1] != shortest[shortest.size()-1]))
    ErrorMsg("ForkPath, path1 and shortest must have the same source and destination");
  
  for (int i=0; i<path.size(); i++) {
    if (path[i] != shortest[i])
      return path[i];
  }
  
  return NULL;
}


