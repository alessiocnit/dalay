#include "ReleaseConnEvent.h"
#include "CNet.h"
#include "CDomain.h"
#include "PCx_Node.h"
#include "PathComputationSession.h"

ReleaseConnEvent::ReleaseConnEvent(CNode* node, CConnection* connection, double time,int RestorationFlag) {
    this->eventTime=time;
    this->eventType=RELEASE_CONN;
    this->Comment="RELEASE_CONN";

    this->InNode=node;
    this->Conn=connection;
    this->RestorationFlag=RestorationFlag;
}

ReleaseConnEvent::~ReleaseConnEvent() {
};

/*-----------------------------------------------------------------------------------------------------------
Lancia la procedura di rilascio della connessione gestita dal nodo sorgente. La deallocazione della memoria
(delete conn) puo' avvenire solo dopo la fine della segnalazione con il TEARDOWN ed e' percio' fittiziamente
rimandata al nodo destinazione del TEARDOWN.

Se siamo in modalità OPENFLOW si entra nel primo il rilascio della connessione è gestito dal controllore.
-----------------------------------------------------------------------------------------------------------*/
void ReleaseConnEvent::execute() {
  CNet* net=this->Conn->Net;
  
  #ifdef STDOUT
  cout << "ReleaseConnEvent -> conn id " << Conn->Id << " STATUS: " << Conn->Status << " r_flag " << this->RestorationFlag << " TIME: " << this->InNode->ParentNet->Now << endl;
  #endif
  
  if (net->DomainArchitecture==OPENFLOW_CONTROLLER) {
    PCx_Node* OpenFlowController=this->InNode->ParentDomain->PCE_Node;
    
    if (this->InNode->ParentDomain->PCE_Node==NULL) 
	ErrorMsg("net->DomainArchitecture==OPENFLOW_CONTROLLER: the pointer to the OpenFlow Controller of this domain is NULL");

    if (this->RestorationFlag==0) {
      switch (this->Conn->Status) {
	case ESTABLISHED:
	  OpenFlowController->ConnRelease_OpenFlow(this->Conn);
	  break;
	case DISRUPTED:
	case RESTORED:
	case ADMITTED_PATH_RESTORATION:
	  #ifdef STDOUT
	  cout << "Original ReleaseEvent for DISRUPTED/RESTORED/ADMITTED_PATH_RESTORATION connection... nothing to do conn id " << this->Conn->Id << endl;
	  #endif
	  break;
	case REFUSED_RESTORATION:
	  #ifdef STDOUT
	  cout << "Original ReleaseEvent for REFUSED_RESTORATION connection... delete conn id " << this->Conn->Id << endl;
	  #endif
	  delete this->Conn;
	  break;
	default:
	  ErrorMsg("Original ReleaseEvent connection status is not handled");
      }
    }
    
    if (this->RestorationFlag==1) {
      switch (this->Conn->Status) {
	case RESTORED:
	  OpenFlowController->ConnRelease_OpenFlow(this->Conn);
	  break;
	default:
	  ErrorMsg("Restoration ReleaseEvent connection status is not handled"); //Questo non dovrebbe accadere mai
      }
    }
    
    return;
  }
  
  switch (Conn->Status) {
    //Next two cases are the most common...
    
    case ESTABLISHED:
        if (this->RestorationFlag==0) {
            Conn->Status=RELEASING;
            Conn->SourceNode->ConnRelease(Conn);
            return;
        }
        else ErrorMsg("Releasing connection is ESTABLISHED and RestorationFlag=1... dalay_panic");
        break;
    case RESTORED:
        if (this->RestorationFlag==1) {
	    #ifdef STDOUT
            cout << "Releasing RESTORED connection conn id " << this->Conn->Id << endl;
	    #endif

            Conn->Status=RELEASING;
            Conn->SourceNode->ConnRelease(Conn);

            return;
        }
        else {
	    #ifdef STDOUT
            cout << "Original ReleaseEvent for RESTORED connection... nothing to do conn id " << this->Conn->Id << endl;
	    #endif
        }
        break;
        //Next four cases are not common but correctly handled
    case REQUESTED_PATH_RESTORATION:
    case ADMITTED_PATH_RESTORATION:
        if (this->RestorationFlag==0) {
	    #ifdef STDOUT
            cout << "Releasing connection in ADMITTED_PATH_RESTORATION status... nothing to do" << endl;
	    #endif
        }
        else ErrorMsg("Releasing connection in ADMITTED_PATH_RESTORATION status and RestorationFlag=1... daaly panic");
        break;
    case BLOCKED_PATH_RESTORATION:
        if (this->RestorationFlag==0) {
	    #ifdef STDOUT
            cout << "Releasing connection in BLOCKED_PATH_RESTORATION status... connection deleted" << endl;
	    #endif

            delete Conn;
        }
        else ErrorMsg("Releasing connection in BLOCKED_PATH_RESTORATION status and RestorationFlag=1... dalay_panic");
        break;
    case REFUSED_RESTORATION:
	#ifdef STDOUT
        cout << "Releasing connection in REFUSED_RESTORATION status... connection deleted" << endl;
	#endif

        delete Conn;
        break;
        //Next cases are not handled
    case REQUESTED:
    case ADMITTED_PATH:
    case BLOCKED_PATH:
    case RELEASING:
    case RELEASED:
    case REFUSED:
    case DISRUPTED:
        cout << "ReleaseConnEvent -> conn id " << Conn->Id << " STATUS: " << Conn->Status << " TIME: " << this->InNode->ParentNet->Now << endl;
        ErrorMsg("Releasing connection in not consistent status -> dalay_panic");
        break;
    default:
        ErrorMsg("ReleaseConnEvent connection in a not existing status -> dalay_panic");
    }
    return;
}
