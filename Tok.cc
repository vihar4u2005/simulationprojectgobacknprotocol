#include <stdio.h>
#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

class Tok: public cSimpleModule {
private:
    cMessage *event;  // pointer to the event object which we'll use for timing
    cMessage *tictocMsg;
    cMessage *lostMsg;
    cMessage *data;
    cMessage *RNR;//
    // char* mq;
    int count1 = 0;
    int ack_count = 0;
    int seq_count = 0;
    long window = 0;
    int RWC = 0;
    int j = 0;
    int n = 0;
    int N;
    int packetloss = 0;
    int PacketCheck = 0;
    int RTR = 0;
    cQueue queue;
    int queueMaxLen=0;
    int bufferfull=0;
public:
    Tok();
    virtual ~Tok();

protected:
    virtual void createMessage(cMessage *msg);
    //virtual void sendMessage(cMessage *msg);
    virtual void receiveMessage(cMessage *msg);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

};

Define_Module(Tok);

Tok::Tok() {
    // Set the pointer to nullptr, so that the destructor won't crash
    // even if initialize() doesn't get called because of a runtime
    // error or user cancellation during the startup process.
    event = tictocMsg = nullptr;

}

Tok::~Tok() {
    // Dispose of dynamically allocated the objects
    cancelAndDelete(event);
    delete tictocMsg;
}

void Tok::initialize() {
    N = par("N");
    queueMaxLen = par("win_size");
    event = new cMessage("event");
    tictocMsg = nullptr;
    queue.clear();    //("TokQ");

    if (strcmp("tok", getName()) == 0) {
        EV << "Scheduling first send to t=5.0s\n";
        tictocMsg = new cMessage("tictocMsg");
        scheduleAt(5.0, event);
    }
}

void Tok::handleMessage(cMessage *msg) {

    if (uniform(0, 1) < 0.1) {
        EV << "Losing message in Tok\n";
        bubble("message lost");

        packetloss = 1;
        delete msg;

        tictocMsg = new cMessage("tictocMsg");
        createMessage(tictocMsg);
    } else {

        if (msg == event)
        {
            EV << "Wait period is over, sending back message\n";
            tictocMsg = new cMessage("tictocMsg");
            window = par("win_size");
            tictocMsg->addPar("win_size");
            tictocMsg->par("win_size").setLongValue(window);
            cancelEvent(event);
            send(tictocMsg, "out");
            tictocMsg = nullptr;
            scheduleAt(simTime() + 1.0, event);
        } else {
            // EV << "Message arrived, starting to wait " << delay << " secs...\n";
            cancelEvent(event);
            receiveMessage(msg);
            if (j) {
                createMessage(msg);
            } else {

                packetloss = 1;
                delete msg;

                tictocMsg = new cMessage("tictocMsg");
                createMessage(tictocMsg);
            }
            scheduleAt(simTime() + 1.0, event);
        }

    }
}

void Tok::createMessage(cMessage *msg) {
    if (packetloss) {
        //counter = 0;
        packetloss = 0;
        lostMsg = new cMessage("lostMsg");
        lostMsg->addPar("seq_count");
        lostMsg->par("seq_count").setLongValue(j);
        // TokQ.insert(msg);
        send(lostMsg, "out");
    }

    else {
        RWC = msg->par("seq_count").longValue();

        if (RWC != RTR) {
            PacketCheck++;
        } else {
            PacketCheck = RTR;
            PacketCheck++;
        }

        if (RWC != PacketCheck) {
            packetloss = 0;
            RTR = PacketCheck;
            lostMsg = new cMessage("lostMsg");
            lostMsg->addPar("seq_count");
            lostMsg->par("seq_count").setLongValue(j);
            // TokQ.insert(msg);
            send(lostMsg, "out");

        }

        if (count1 != N) {

            //delete msg;
            count1++;
        }

        if (count1 == N) {
            tictocMsg = new cMessage("tictocMsg");
            tictocMsg->addPar("seq_count");
            tictocMsg->par("seq_count").setLongValue(j);
            // TokQ.insert(msg);
            n = j + 1;
            EV << "Tok is receive ready " << n << "\n";
            send(tictocMsg, "out");

            count1 = 0;
        }

    }
}

void Tok::receiveMessage(cMessage *msg)
{
   /*if (!strcmp(msg->getName(), "RNR")) {

       delete queue.pop();
   }
   else  {*/
    j = msg->par("seq_count").longValue();
    EV << "received packet in Tok :" << j << "\n";
    if (queue.getLength() > queueMaxLen)
    {
          EV << "Buffer overflow, discarding " << queue.front()->getName() << endl;
         // bufferfull=1;
          delete queue.pop();
           /* if (bufferfull) {

                      bufferfull = 0;
                     RNR = new cMessage("RNR");
                     RNR->addPar("seq_count");
                     RNR->par("seq_count").setLongValue(j);
                     send(RNR, "out");
                 }
      }
    else
      {*/
      queue.insert(msg);
      }


}
