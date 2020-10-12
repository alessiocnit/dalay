#include "CTopology.h"
#include "PCx_Node.h"
#include "CDomain.h"

#include "parser.h"
#include "PathComputationSession.h"

CTopology::CTopology(CNet* net) {
    this->GlobalNet = net;

    this->FileName = this->GlobalNet->NameS+".topology";
    this->GlobalNet->LogFile << "(CTopology Builder) Topology file name: " << this->FileName << endl;

    return;
};


/*Dato che CTopology crea e riempie la PathStateTable la distrugge anche...
pensare se sia il caso di spostare creazione e distruzione della LinkState nella CNode*/
CTopology::~CTopology() {
    for (int j=0 ; j<GlobalNet->Node.size(); j++) {
        CNode* ActualNode=GlobalNet->Node[j];

        for (int i=0; i<GlobalNet->Link.size(); i++) {
            delete ActualNode->LinkState_dB[i];
        }

        if (ActualNode->Id==ActualNode->ParentDomain->PCE_Node_Id) {
            PCx_Node* Domain_PCE=static_cast <PCx_Node*> (ActualNode);

            for (int i=0; i<GlobalNet->Link.size(); i++) {
                delete Domain_PCE->SNMP_LinkState_dB[i];
            }
        }
    }
};

void CTopology::load() {
    list <domain_block> ldomain;	// list of domains
    list <node_block> lnode;		// list of nodes
    list <link_block> llink;		// list of links
    list <demand_block> ldemand;	// list of demands
    FILE *input_file;			// pointer to the actual position of the file
    CLink* ActualLink;
    CNode* ActualNode;
    CDomain* ActualDomain;
    
    int size, src_edge, dst_edge, domain_id;
    CNode* src_node;
    CNode* dst_node;

    cout << "\t...opening input file:" << FileName << "...";
    GlobalNet->LogFile << "Opening input file:" << FileName << "...";
    input_file=fopen(FileName.c_str(),"r");
    if (input_file==NULL) {
        cout << " CTopology::load() - cannot open file " << FileName << endl;
        ErrorMsg("CTopology::load() - cannot open file");
    }
    cout << "[ok]" << endl;
    GlobalNet->LogFile << "[ok]" << endl;

    int ndomain=0;		// # of domains
    int nnode=0;		// # of nodes
    int nlink=0;		// # of links
    int ndemand=0;		// # of links demands
    int nclasse=0;		// # of classes

    if (FileName=="") {
        cout << "something went wrong" << endl;
        return;
    }

    read_input_file(&ldomain,&lnode,&llink,&ldemand,input_file,&ndomain,&nnode,&nlink,&ndemand,&nclasse);

    //Loads Network nodes in CNet GlobalNet -----------------------------
    cout << "\t...loading network nodes: ";
    GlobalNet->LogFile << "Creating nodes: ";
    for (int i=0;i<nnode;i++) {
        GlobalNet->CreateNode(lnode.front().ID,lnode.front().name);
        lnode.pop_front();
        GlobalNet->LogFile << i << " ";
        cout << i << " ";
    }
    GlobalNet->LogFile << "done" << endl;
    cout << "[ok]" << endl;
    //-------------------------------------------------------------------

    //Loads Network Outgoing links in CNodes (OutLink)-------------------
    cout << "\t...loading network outgoing links in network nodes: ";
    GlobalNet->LogFile << "Creating links: ";
    for (int i=0;i<nlink;i++) {
        int from_n=0;
        int to_n=0;
        while ((llink.front().from!=GlobalNet->Node[from_n]->Name) && (from_n<=nnode)) from_n++;
        while ((llink.front().to!=GlobalNet->Node[to_n]->Name) && (to_n<=nnode)) to_n++;
        if ((to_n==nnode)||(from_n==nnode)) {
            cout << "Error in the Topology file:" << llink.front().from << " or " << llink.front().to << " don't exist" << endl;
        }
        int Loaded_Id=llink.front().ID;
        double Loaded_Length=llink.front().distance;
        int Loaded_MaxByte=llink.front().maxbyte;
        double Loaded_Bitrate=llink.front().bitrate;
	int Loaded_Metric=llink.front().delay;

        GlobalNet->Node[from_n]->AddNewLink(Loaded_Id,GlobalNet->Node[to_n],Loaded_Length,Loaded_MaxByte,Loaded_Bitrate,Loaded_Metric);
        llink.pop_front();
        GlobalNet->LogFile << i << " ";
        cout << i << " ";
    }
    GlobalNet->LogFile << "done" << endl;
    cout << "[ok]" << endl;
    //-------------------------------------------------------------------
    //Loads Network Incoming links in CNodes (InLink)--------------------
    cout << "\t...loading network incoming links in network nodes...";
    for (int i=0; i<GlobalNet->Node.size(); i++) {
        GlobalNet->Node[i]->Fill_InLink();
    }
    cout << "[ok]" << endl;
    //-------------------------------------------------------------------
    //Loads Network links in CNet GlobalNet -----------------------------
    cout << "\t...loading network links in the network...";
    for (int i=0; i<GlobalNet->Node.size(); i++)
        for (int j=0; j< GlobalNet->Node[i]->OutLink.size();j++)
            GlobalNet->Link.push_back(GlobalNet->Node[i]->OutLink[j]);
    cout << "[ok]" << endl;
    //-------------------------------------------------------------------
    //Loads local links database in networks nodes ----------------------
    //-------------------------------------------------------------------
    cout << "\t...loading local links database in each network nodes...";
    for (int i=0; i<GlobalNet->Node.size(); i++) {
        GlobalNet->Node[i]->CreateLinkDatabase();
    }
    cout << "[ok]" << endl;
    //-------------------------------------------------------------------
    //Loads Network domains in CNet GlobalNet ---------------------------
    //-------------------------------------------------------------------
    cout << "\t...loading network domains: ";
    GlobalNet->LogFile << "Creating domains: ";
    for (int i=0;i<ndomain;i++) {
        GlobalNet->CreateDomain(ldomain.front().ID,ldomain.front().name,ldomain.front().PCE_ID,ldomain.front().NodeIDs_List);
        ldomain.pop_front();
        GlobalNet->LogFile << i << " ";
        cout << i << " ";
    }
    GlobalNet->LogFile << "done" << endl;
    cout << "[ok]" << endl;
    //-------------------------------------------------------------------
    //Checking if each node belongs to one domain -----------------------
    //-------------------------------------------------------------------
    cout << "\t...checking if every node belongs to one domain: ";
    for (int i=0; i<GlobalNet->Node.size(); i++) {
        cout << i << " ";
        if (GlobalNet->Node[i]->ParentDomain == NULL) {
            cout << endl << "Node: " << GlobalNet->Node[i]->Id << endl;
            ErrorMsg("There is a node not belonging to any domain");
        }
    }
    cout << "[ok]" << endl;
    //-------------------------------------------------------------------
    //Checking for domains including a single node (not allowed) --------
    //-------------------------------------------------------------------
    cout << "\t...checking for domains including a single node (not allowed): ";
    for (int i=0; i<GlobalNet->Domain.size(); i++) {
        cout << i << " ";
        if (GlobalNet->Domain[i]->Node.size() == 1)
            ErrorMsg("There is a domain including a single node");
    }
    cout << "[ok]" << endl;
    //-------------------------------------------------------------------
    //Checking if every domain node is connected to at least an other node of the same domain
    //-------------------------------------------------------------------
    cout << "\t...checking for domain node connectivity: ";
    //Filling up the InterDomain OutLinks
    for (int i=0;i<GlobalNet->Node.size();i++) {
        cout << i << " ";
        for (int j=0;j<GlobalNet->Node[i]->OutLink.size();j++) {
            if (GlobalNet->Node[i]->OutLink[j]->ToNode->ParentDomain == GlobalNet->Node[i]->ParentDomain)
                break;
            if (j == (GlobalNet->Node[i]->OutLink.size() - 1))
                ErrorMsg("There is a node not connected to other nodes of its domain");
        }
    }
    cout << "[ok]" << endl;
    //-------------------------------------------------------------------
    //Setting the Type field of the HPCE node
    //-------------------------------------------------------------------
    if ((GlobalNet->HPCE_NodeId>=GlobalNet->Node.size()) || (GlobalNet->HPCE_Node->Type!=PCC_PCE))
        ErrorMsg("The HPCE node is not colocated with a PCE node");
    else GlobalNet->HPCE_Node->Type=PCC_PCE_HPCE;
    //-------------------------------------------------------------------
    //Loading the CNet->EdgeNode list and the CNet->InterDomain_Link list 
    //-------------------------------------------------------------------
    for (int i=0; i<GlobalNet->Domain.size(); i++) {
        CDomain* dom=GlobalNet->Domain[i];
        for (int j=0;j<dom->In_EdgeNode.size();j++) {
            GlobalNet->EdgeNode.push_back(dom->In_EdgeNode[j]);
            dom->In_EdgeNode[j]->Location=EDGE_DOMAIN;
        }
        for (int j=0;j<dom->InterDomain_OutLink.size();j++) {
            GlobalNet->InterDomain_Link.push_back(dom->InterDomain_OutLink[j]);
            dom->InterDomain_OutLink[j]->Type=INTER_DOMAIN;
        }
    }
    //TODO to be removed ??? Studiare bene le implicazioni di questo !!!!!!!!!!
    for (int j=0;j<GlobalNet->InterDomain_Link.size();j++) {
        GlobalNet->InterDomain_Link[j]->DistanceMetric=100;
    }
    //-------------------------------------------------------------------
    //Inizializing the Link State Database (LinkState struct) in each network node
    //-------------------------------------------------------------------
    for (int j=0 ; j<GlobalNet->Node.size(); j++) {
	ActualNode=GlobalNet->Node[j];
        ActualDomain=ActualNode->ParentDomain;

	//Insert a link_state for all the link intra-domain and for outgoing inter-domain links
        for (int i=0; i<ActualDomain->Link.size(); i++) {
	    ActualLink=ActualDomain->Link[i];
	    
            LinkState* llsa=new LinkState;

            llsa->Link=ActualLink;
            llsa->AdvertisingNode=ActualNode;
            llsa->SequenceNumber=1;
            llsa->FreeLambdas=GlobalNet->W;
            for ( int k=0; k<GlobalNet->W; k++)
                llsa->LambdaStatus.push_back(0);

            ActualNode->LinkState_dB.push_back(llsa);

            //Fill the SNMP database in PCE nodes
            if ((ActualNode->Type==PCC_PCE) || (ActualNode->Type==PCC_PCE_HPCE)) {
            	PCx_Node* PCE_Node=static_cast <PCx_Node*> (ActualNode);
            	LinkState* pce_llsa=new LinkState;
		
            	*pce_llsa=*llsa;

            	PCE_Node->SNMP_LinkState_dB.push_back(pce_llsa);
            }
        }

	//Insert a link_state for all the incoming inter-domain links
        for (int i=0; i<ActualDomain->InterDomain_InLink.size(); i++) {
	  ActualLink=ActualDomain->InterDomain_InLink[i];
	    
	  LinkState* llsa=new LinkState;

          llsa->Link=ActualLink;
          llsa->AdvertisingNode=ActualNode;
          llsa->SequenceNumber=1;
          llsa->FreeLambdas=GlobalNet->W;
          for ( int k=0; k<GlobalNet->W; k++)
	    llsa->LambdaStatus.push_back(0);

          ActualNode->LinkState_dB.push_back(llsa);

          //Fill the SNMP database in PCE nodes
          if ((ActualNode->Type==PCC_PCE) || (ActualNode->Type==PCC_PCE_HPCE)) {
          	PCx_Node* PCE_Node=static_cast <PCx_Node*> (ActualNode);
          	LinkState* pce_llsa=new LinkState;

          	*pce_llsa=*llsa;

          	PCE_Node->SNMP_LinkState_dB.push_back(pce_llsa);
          }
        }
    }
    
    
    PCx_Node* HPCE_Node=this->GlobalNet->HPCE_Node;
    
    //Fill the SNMP_InterDomain link database in the HPCE node
    for (int j=0; j<this->GlobalNet->Domain.size(); j++) {
      ActualDomain=this->GlobalNet->Domain[j];
      
      for (int i=0; i<ActualDomain->InterDomain_InLink.size(); i++) {
	ActualLink=ActualDomain->InterDomain_InLink[i];
	
	LinkState* hpce_llsa = new LinkState;
	
	hpce_llsa->Link=ActualLink;
        hpce_llsa->AdvertisingNode=HPCE_Node;
        hpce_llsa->SequenceNumber=1;
        hpce_llsa->FreeLambdas=GlobalNet->W;
        for (int k=0; k<GlobalNet->W; k++)
	  hpce_llsa->LambdaStatus.push_back(0);

	HPCE_Node->SNMP_HPCE_InterDomain_LinkState_dB.push_back(hpce_llsa);
      }
    }
    
    //Fill the BGP-LS link database in the HPCE node
    for (int j=0; j<this->GlobalNet->Link.size(); j++) {
      ActualLink=this->GlobalNet->Link[j];
	
      LinkState* hpce_llsa = new LinkState;
	
      hpce_llsa->Link=ActualLink;
      hpce_llsa->AdvertisingNode=HPCE_Node;
      hpce_llsa->SequenceNumber=1;
      hpce_llsa->FreeLambdas=GlobalNet->W;
      for (int k=0; k<GlobalNet->W; k++)
	hpce_llsa->LambdaStatus.push_back(0);

      HPCE_Node->BGPLS_HPCE_LinkState_dB.push_back(hpce_llsa);
    }
    
    //Fill the PCEP_StoredRequests in the HPCE node
    for (int i=0; i<this->GlobalNet->Domain.size(); i++) {
      CDomain* ActualDomain=this->GlobalNet->Domain[i];
      for (int j=0; j<ActualDomain->In_EdgeNode.size(); j++) {
	for (int k=0; k<ActualDomain->In_EdgeNode.size(); k++) {
	  CNode* ActualSrc=ActualDomain->In_EdgeNode[j];
	  CNode* ActualDst=ActualDomain->In_EdgeNode[k];
	  if (ActualSrc!=ActualDst) {
	    PathComputationRequest* hpce_pcep_stored_req = new PathComputationRequest;
	    
	    hpce_pcep_stored_req->Src=ActualSrc;
	    hpce_pcep_stored_req->Dst=ActualDst;
	    hpce_pcep_stored_req->RequestStatus = UNINITIALIZED;
	    hpce_pcep_stored_req->LastReplyTime = 0.0; //Initialized to a negative value
	    
	    HPCE_Node->PCEP_StoredRequests.push_back(hpce_pcep_stored_req);
	  }
	}
      }
    }
    //-------------------------------------------------------------------
    //Write in logfile Network domains nodes and links ------------------
    //-------------------------------------------------------------------
    CNode* tn;
    CLink* tl;
    GlobalNet->LogFile << "-----------------------------------\n";
    GlobalNet->LogFile << "--- Domains summary ---\n";
    GlobalNet->LogFile << "-----------------------------------\n";
    for (int i=0; i<GlobalNet->Domain.size(); i++) {
        GlobalNet->LogFile << "Domain Id: " << GlobalNet->Domain[i]->Id << "\t" << GlobalNet->Domain[i]->Name << " PCE: ";

        if (GlobalNet->Domain[i]->PCE_Node_Id == -1) {
            GlobalNet->LogFile << "NO_PCE";
        }
        else  {
            GlobalNet->LogFile << GlobalNet->Domain[i]->PCE_Node->Name;
        }

        GlobalNet->LogFile << "\n- nodes: ";
        for (int j=0; j<GlobalNet->Domain[i]->Node.size();j++)
            GlobalNet->LogFile << GlobalNet->Domain[i]->Node[j]->Id << " ";
        GlobalNet->LogFile << endl;

        for (int j=0; j<GlobalNet->Domain[i]->InterDomain_OutLink.size(); j++) {
            GlobalNet->LogFile << "- to domain Id: ";
            GlobalNet->LogFile << GlobalNet->Domain[i]->InterDomain_OutLink[j]->ToNode->ParentDomain->Id;
            GlobalNet->LogFile << " (" << GlobalNet->Domain[i]->InterDomain_OutLink[j]->FromNode->Name;
            GlobalNet->LogFile << " - " << GlobalNet->Domain[i]->InterDomain_OutLink[j]->ToNode->Name << ")" << endl;
        }
    }

    GlobalNet->LogFile << "-----------------------------------\n";
    GlobalNet->LogFile << "--- Nodes and Links summary ---\n";
    GlobalNet->LogFile << "-----------------------------------\n";
    for (int i=0; i<GlobalNet->Node.size(); i++) {
        tn=GlobalNet->Node[i];
        GlobalNet->LogFile << "Node Id: " << tn->Id << "\t" << tn->Name << "@" << tn->ParentNet->Name << endl;
        for (int j=0; j<tn->OutLink.size() ;j++) {
            tl=tn->OutLink[j];
            GlobalNet->LogFile << "- to node Id: " << tl->ToNode->Id << " link Id: " << tl->Id << "\t" << tl->FromNode->Name << "<->" << tl->ToNode->Name << "\t" << tl->length << endl;
        }
    }
    GlobalNet->LogFile << "-----------------------------------\n";

    //-------------------------------------------------------------------
    //Initializing matrices G,D,F (node topology matrix) and D (domain topology matrix) in GlobalNet
    //-------------------------------------------------------------------
    if (NMAX<=GlobalNet->Node.size()+GlobalNet->Domain.size())
      ErrorMsg("NMAX must be greater than Nodes+Domains (due to matrix H)");
    
    for (int i=0;i<NMAX;i++)
        for (int j=0;j<NMAX;j++) {
            GlobalNet->G[i][j]=0;
            GlobalNet->D[i][j]=0;
            GlobalNet->F[i][j]=0;
	    GlobalNet->H[i][j]=0;
        }
    //-------------------------------------------------------------------
    //Load G matrix from CNode and Clink
    //-------------------------------------------------------------------
    for (int i=0;i<GlobalNet->Node.size();i++)
        for (int j=0;j<GlobalNet->Node[i]->OutLink.size();j++) {
	  int src_id=GlobalNet->Node[i]->Id;
	  int dst_id=GlobalNet->Node[i]->OutLink[j]->ToNode->Id;
          //GlobalNet->G[src_id][dst_id]=GlobalNet->Node[i]->OutLink[j]->length;
	  GlobalNet->G[src_id][dst_id]=1;
	}
    //-------------------------------------------------------------------
    //Load D matrix from CNode and Clink
    //-------------------------------------------------------------------
    for (int k=0;k<GlobalNet->Domain.size();k++) {
        for (int i=0; i<GlobalNet->Domain[k]->Node.size(); i++)
            for (int j=0;j<GlobalNet->Domain[k]->Node[i]->OutLink.size();j++)
                GlobalNet->D[GlobalNet->Domain[k]->Id][GlobalNet->Domain[k]->Node[i]->OutLink[j]->ToNode->ParentDomain->Id]=1;
    }
    for (int i=0;i<NMAX;i++)
        GlobalNet->D[i][i]=0;
    //-------------------------------------------------------------------
    //Load G topology matrix in every Domain
    //-------------------------------------------------------------------
    for (int k=0;k<GlobalNet->Domain.size();k++) {
        for (int i=0;i<NMAX;i++)
            for (int j=0;j<NMAX;j++)
                GlobalNet->Domain[k]->G[i][j]=0;

        //To load G matrix from CNode and Clink
        for (int i=0;i<GlobalNet->Domain[k]->Node.size();i++)
            for (int j=0;j<GlobalNet->Domain[k]->Node[i]->OutLink.size();j++)  {
		int src_id=GlobalNet->Domain[k]->Node[i]->Id;
		int dst_id=GlobalNet->Domain[k]->Node[i]->OutLink[j]->ToNode->Id;
                GlobalNet->Domain[k]->G[src_id][dst_id]=GlobalNet->Domain[k]->Node[i]->OutLink[j]->length;
            }
    }
    //-------------------------------------------------------------------
    //---- Load F topology matrix in CNet (currently not used)
    //-------------------------------------------------------------------
    //It is based on the G matrix but includes:
    //1. Interdomain links between edge nodes with cost 100 (the same links are present in G with cost 1)
    //2. A full mesh of logical links with cost 1 connecting the edge nodes of each domain (not present in G)
    //3. A pair of logical links with cost 1 connecting each *non* edge node to each edge node of its domain (not present in G)
    size=this->GlobalNet->Node.size();

    //Non edge nodes are connected to each edge nodes with a bidirectional link of cost 1
    for (int i=0; i<size; i++) { 
        for (int j=0; j<size; j++) {
            src_node=this->GlobalNet->Node[i];
            dst_node=this->GlobalNet->Node[j];

            if ((src_node!=dst_node) && (src_node->ParentDomain==dst_node->ParentDomain) && (src_node->Location==IN_DOMAIN) && (dst_node->Location==EDGE_DOMAIN)) {
                GlobalNet->F[i][j]=1;
                GlobalNet->F[j][i]=1;
            }
        }
    }

    //The cost of interdomains links is 100
    for (int i=0; i<GlobalNet->InterDomain_Link.size();i++) { 
        CLink* int_link = GlobalNet->InterDomain_Link[i];

        src_edge = int_link->FromNode->Id;
        dst_edge = int_link->ToNode->Id;

        GlobalNet->F[src_edge][dst_edge]=100;
    }

    //Each edge node is connected to other edge nodes of the same domain with a link of cost 1
    for (int i=0; i<GlobalNet->EdgeNode.size(); i++) { //including full mesh
        for (int j=0; j<GlobalNet->EdgeNode.size(); j++) {
            src_node=this->GlobalNet->EdgeNode[i];
            dst_node=this->GlobalNet->EdgeNode[j];

            if ((src_node!=dst_node) && (src_node->ParentDomain == dst_node->ParentDomain)) {
                GlobalNet->F[src_node->Id][dst_node->Id]=1;
            }
        }
    }
    //-------------------------------------------------------------------
    //---- Load H topology matrix in CNet [nodes+domains][nodes+domains]
    //-------------------------------------------------------------------
    //It is based on the G matrix but includes:
    //1. Interdomain links between edge nodes with cost 100 (the same links are present in G with cost 1)
    //2. A full mesh of logical links with cost 1 connecting the edge nodes of each domain (not present in G)
    //3. A virtual node in each domain connected to each edge nodes of its domain
    size=GlobalNet->Node.size()+GlobalNet->Domain.size();

    //The cost of interdomains links is 100
    for (int i=0; i<GlobalNet->InterDomain_Link.size();i++) { 
        CLink* int_link = GlobalNet->InterDomain_Link[i];

        src_edge = int_link->FromNode->Id;
        dst_edge = int_link->ToNode->Id;

        GlobalNet->H[src_edge][dst_edge]=100;
    }

    //Each edge node is connected to the other edge nodes of the same domain with a link of cost 1
    for (int i=0; i<GlobalNet->EdgeNode.size(); i++) { 
        for (int j=0; j<GlobalNet->EdgeNode.size(); j++) {
            src_node=this->GlobalNet->EdgeNode[i];
            dst_node=this->GlobalNet->EdgeNode[j];

            if ((src_node!=dst_node) && (src_node->ParentDomain == dst_node->ParentDomain)) {
                GlobalNet->H[src_node->Id][dst_node->Id]=1;
            }
        }
    }
    
    //Virtual nodes are connected to edge nodes in its domain with cost 1
    for (int i=0; i<GlobalNet->EdgeNode.size(); i++) { 
        for (int j=GlobalNet->Node.size(); j<size; j++) {
	  src_node=this->GlobalNet->EdgeNode[i];
	  domain_id=j-GlobalNet->Node.size();
	  
	  if (src_node->ParentDomain->Id == domain_id) {
	    GlobalNet->H[src_node->Id][j]=1;
	    GlobalNet->H[j][src_node->Id]=1;
	  }
	}
    }
    //-------------------------------------------------------------------
    //Write the loaded D matrix in the log file
    //-------------------------------------------------------------------
    GlobalNet->LogFile << "---------------------------" << endl;
    GlobalNet->LogFile << "----- Loaded D Matrix -----" << endl;
    for (int i=0;i<GlobalNet->Domain.size();i++) {
        for (int j=0;j<GlobalNet->Domain.size();j++) {
            if (GlobalNet->D[i][j]) GlobalNet->LogFile << GlobalNet->D[i][j] << "\t";
            else GlobalNet->LogFile << "-1\t";
        }
        GlobalNet->LogFile << endl;
    }
    GlobalNet->LogFile << "---------------------------" << endl;
    //-------------------------------------------------------------------
    //Write the loaded G matrix in the log file
    //-------------------------------------------------------------------
    GlobalNet->LogFile << "---------------------------" << endl;
    GlobalNet->LogFile << "----- Loaded G Matrix -----" << endl;
    for (int i=0;i<GlobalNet->Node.size();i++) {
        for (int j=0;j<GlobalNet->Node.size();j++) {
            if (GlobalNet->G[i][j]) GlobalNet->LogFile << GlobalNet->G[i][j] << "\t";
            else GlobalNet->LogFile << "-1\t";
        }
        GlobalNet->LogFile << endl;
    }
    GlobalNet->LogFile << "---------------------------" << endl;
    //-------------------------------------------------------------------
    //Write the loaded H matrix in the log file
    //-------------------------------------------------------------------
    GlobalNet->LogFile << "---------------------------" << endl;
    GlobalNet->LogFile << "----- Loaded H Matrix -----" << endl;
    for (int i=0;i<GlobalNet->Node.size()+GlobalNet->Domain.size();i++) {
        for (int j=0;j<GlobalNet->Node.size()+GlobalNet->Domain.size();j++) {
            if (GlobalNet->H[i][j]) GlobalNet->LogFile << GlobalNet->H[i][j] << "\t";
            else GlobalNet->LogFile << "-1\t";
        }
        GlobalNet->LogFile << endl;
    }
    GlobalNet->LogFile << "---------------------------" << endl;
    //-------------------------------------------------------------------
    //Write the loaded G matrix in the log file for each domain
    //-------------------------------------------------------------------
    for (int k=0;k<GlobalNet->Domain.size();k++) {
        GlobalNet->LogFile << "-------------------------------------" << endl;
        GlobalNet->LogFile << "----- Loaded G Matrix Domain " << k << " ----- " << endl;
        for (int i=0;i<GlobalNet->Node.size();i++) {
            for (int j=0;j<GlobalNet->Node.size();j++) {
                if (GlobalNet->Domain[k]->G[i][j]) GlobalNet->LogFile << GlobalNet->Domain[k]->G[i][j] << "\t";
                else GlobalNet->LogFile << "-1\t";
            }
            GlobalNet->LogFile << endl;
        }
        GlobalNet->LogFile << "-------------------------------------" << endl;
    }
    //-------------------------------------------------------------------

    GlobalNet->LogFile << "Closing file:" << FileName << endl;
    cout << "\t...closing file: " << FileName << "...";
    fclose(input_file);
    cout << "[ok]" << endl;
};





