#include "BHTree.h"
#include "CNet.h"

BHTree::BHTree(CNet* net) {
    this->ParentNet=net;

    Head=NULL;
    LastLeaf=NULL;
    globtemp=new BHEvent();
};

BHTree::~BHTree() {
    while (Head!=LastLeaf) {
        delete LastLeaf;
    }
    delete Head;
    delete globtemp;
};

//To create a new event object in the first available leaf place
BHEvent* BHTree::NewLeaf(CEvent* value) {

    //cout << "\t...new event: type=" << value->eventType << ",time=" << value->eventTime << endl;
    int level=0;
    BHEvent *temp;
    temp=new BHEvent();
    temp->c = value;
    if (LastLeaf==NULL) {
        Head=temp;
        LastLeaf=Head;
        return Head;
    }
    if (LastLeaf!=Head) {
        if (LastLeaf->up->right==NULL) {
            LastLeaf->up->right=temp;
            temp->up=LastLeaf->up;
            LastLeaf=temp;
            return temp;
        }
        while ((LastLeaf->up!=NULL)&&(LastLeaf->up->right==LastLeaf)) {
            LastLeaf=LastLeaf->up;
            level++;
        }
        if (LastLeaf->up!=NULL) {
            LastLeaf=LastLeaf->up->right;
        } else {
            level++;
        }

        //else I'm at the top of the tree and I have only to start another
        //level at the rightmost side.

        while (level != 1) {
            LastLeaf=LastLeaf->left;
            level--;
        }
    } //endif(LastLeaf!=Head);
    LastLeaf->left=temp;
    temp->up=LastLeaf;
    LastLeaf=temp;
    return temp;
}

//It returns the Leaf that was the last leaf before the last leaf,
//erasing the last one. LastLeaf is going to change.
BHEvent* BHTree::DeleteLastLeaf() {
    int level=0;
    BHEvent *temp;
    temp=LastLeaf;
    if (LastLeaf==Head) {
        Head=NULL;
        LastLeaf=NULL;
        delete Head;
        return LastLeaf;
    }
    if (LastLeaf->up->right==LastLeaf) {
        LastLeaf=LastLeaf->up;
        delete temp;//@ probably it doesn't work
        LastLeaf->right=NULL;
        LastLeaf=LastLeaf->left;
        return LastLeaf;
    }
    while ((LastLeaf->up!=NULL)&&(LastLeaf->up->left==LastLeaf)) {
        LastLeaf=LastLeaf->up;
        level++;
    }
    if (LastLeaf->up!=NULL) {
        LastLeaf=LastLeaf->up->left;
    } else {
        level--;
    }

    while (level !=0) {
        LastLeaf=LastLeaf->right;
        level--;
    }
    delete temp;
    temp->up->left=NULL;
    //LastLeaf=LastLeaf->left;
    return LastLeaf;
}

BHEvent* BHTree::GainPositions(BHEvent *eve) {
    if (Head==LastLeaf) {
        return Head;
    }
    while ((eve->up!=NULL)&& (eve->getEventContent()->getHotField()<eve->up->getEventContent()->getHotField())) {
        GoUp(eve);
        // keep pointing the same value;
        eve=eve->up;
    }
    return eve;
}

BHEvent* BHTree::Add(CEvent* val) {
    if (val->eventTime < this->ParentNet->Now) {
        cout << val->eventTime << " must be greater than " << this->ParentNet->Now << ' ';
        ErrorMsg("BHTree event time not consistent"); //inserted event time must be greater than now
    }
    BHEvent *NewEventBefore, *NewEventAfter;
    NewEventBefore=NewLeaf(val);
    NewEventAfter=GainPositions(NewEventBefore);
    return NewEventAfter;
}

void BHTree::ReleaseEvent(BHEvent *eve) {
    while (eve->right!=NULL) {
        if (eve->left->getEventContent()->getHotField()<eve->right->getEventContent()->getHotField()) {
            GoUp(eve->left);
            eve=eve->left;
        } else {
            GoUp(eve->right);
            eve=eve->right;
        }
    }
    if (eve->left!=NULL) {
        GoUp(eve->left);
        /*Lasteaf is eve->left (for sure) but, instead of manually delete it with
        eve->left=NULL, we use DeleteLastLeaf() because also detect the next LastLeaf*/
        DeleteLastLeaf();
    } else {
        if (eve!=LastLeaf) {
            XChange(eve,LastLeaf);
            GainPositions(eve);
        }
        DeleteLastLeaf();
    }
}

BHEvent* BHTree::NextEvent() {
    return Head;
}

void BHTree::execute() {
    while (Head!=NULL) {
        this->ParentNet->Now=Head->getHotField();
        NextEvent()->execute();
        ReleaseEvent(Head);
    }
}

void BHTree::XChange(BHEvent *event1, BHEvent *event2) {
    globtemp->c=event2->c;
    event2->c=event1->c;
    event1->c=globtemp->c;
    globtemp->c=NULL;
}

void BHTree::GoUp(BHEvent *event) {
    XChange(event,event->up);
}

