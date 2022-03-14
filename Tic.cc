#include <stdio.h>
#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

class Tic: public cSimpleModule {
private:
    simtime_t timeout;
    cMessage *event;  // pointer to the event object which we'll use for timing
    cMessage *tictocMsg;
    cMessage *deleteQ;
    cMessage *data;  // variable to remember the message until we send it back
    cMessage *cwz;
    cMessage *timeoutEvent;
    int count = 0;
    int acklost = 0;
    int lostpacketcount = 0;
    int N;
    int rm = 0;
    int counter = 2;
    int seq_count = 0;
    int number = 0;
    int win_size;
    int windowtest = 0;
    int i = 0;
    int j = 0;
    int SWC = 0;
    int rnr=0;
    cQueue TikQ;
public:
    Tic();
    virtual ~Tic();

protected:
    virtual void createMessage(cMessage *msg);
    // virtual void sendMessage();
    virtual void receiveMessage(cMessage *msg);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(Tic);

Tic::Tic() {
    // Set the pointer to nullptr, so that the destructor won't crash
    // even if initialize() doesn't get called because of a runtime
    // error or user cancellation during the startup process.
    event = tictocMsg = timeoutEvent = nullptr;
}

Tic::~Tic() {
    // Dispose of dynamically allocated the objects
    cancelAndDelete(event);
    cancelAndDelete(timeoutEvent);
    delete tictocMsg;
}

void Tic::initialize() {
    cQueue queue("TikQ");
    event = new cMessage("event");
    timeout = 5.0;
    timeoutEvent = new cMessage("timeoutEvent");
    tictocMsg = nullptr;
    if (strcmp("tic", getName()) == 0) {
        EV << "Scheduling first send to t=5.0s\n";
        tictocMsg = new cMessage("tictocMsg");
        EV << "start timer\n";
        scheduleAt(5.0, event);
        scheduleAt(simTime()+timeout, timeoutEvent);
    }

}

void Tic::handleMessage(cMessage *msg) {
    if (uniform(0, 1) < 0.1) {
        EV << "Losing messagein Tic\n";
        bubble("ack lost");

        acklost = 1;
        delete msg;

        tictocMsg = new cMessage("tictocMsg");
        createMessage(tictocMsg);
    }
    else
    {
      if(msg == timeoutEvent) {
                 if(counter) {
                        counter --;
                        EV<< "Timeout expired, re-sending message\n";
                        bubble("retransmission");
                         if (count == 0) { /* What if the initial message is dropped, resend the initial message again */
                             tictocMsg = new cMessage("tictocMsg");
                             createMessage(tictocMsg);
                        }  else {
                            count = count - win_size;
                            EV<<"seq at tic="<<count<<"\n";
                            tictocMsg = new cMessage("tictocMsg");
                            createMessage(tictocMsg);
                          }
                        scheduleAt(simTime()+timeout, timeoutEvent);
                    }  else {
                        EV << "No response from toc, Exiting the program\n";
                    }
              }
        else if (msg == event) {
            cancelEvent(event);
            EV << "Handshake done\n";
            scheduleAt(simTime() + 1.0, event);
        } else {

            receiveMessage(msg);
            EV<<"timer cancelled";
            cancelEvent(timeoutEvent);
            counter=2;
            createMessage(msg);
            scheduleAt(simTime()+timeout, timeoutEvent);
        }

    }

}

void Tic::createMessage(cMessage *msg) {
    if (acklost) {
        acklost = 1;
        count = rm;
        //EV << "seq_count " << count << "...\n";
    }
    for (i = 0; i < win_size; i++) {
        count++;
        tictocMsg = new cMessage("tictocMsg");
        tictocMsg->addPar("seq_count");
        // EV << "seq_count " << count << "...\n";
        tictocMsg->par("seq_count").setLongValue(count);

        send(tictocMsg, "out");

        if (count >= 255) {
            count = 0;
        }
    }
}

void Tic::receiveMessage(cMessage *msg) {

    if (windowtest == 0) {
        //EV << "Message arrived, starting to wait " << delay << " secs...\n";
        win_size = msg->par("win_size").longValue();
        EV << "window size " << win_size << "...\n";
        windowtest = 1;
        delete msg;
    }

    else if (!strcmp(msg->getName(), "lostMsg")) {
        TikQ.insert(msg);
        lostpacketcount = msg->par("seq_count").longValue();
        count = --lostpacketcount;
        tictocMsg = new cMessage("tictocMsg");
        createMessage(tictocMsg);

    }

    else if (!strcmp(msg->getName(), "RNR")) {
               deleteQ = new cMessage("deleteQ");
                    send(deleteQ, "out");
                    createMessage(tictocMsg);
            rnr = msg->par("seq_count").longValue();
            count = --rnr;
            tictocMsg = new cMessage("tictocMsg");
            createMessage(tictocMsg);

        }

    else {
        rm = msg->par("seq_count").longValue();
        EV << "received in tic " << rm << "...\n";

        if (rm >= 255) {
            count = 0;
        }
        delete msg;
    }
}

