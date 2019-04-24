#include <iostream>
#include "UdpSocket.h"
#include "Timer.h"
//using namespace std;

const int PORT = 40385;       // my UDP port
const int MAX = 50;        // times of message transfer
const int MAX_WIN = 6;       // maximum window size
const bool verbose = false;   //use verbose mode for more information during run

// client packet sending functions
void ClientUnreliable(UdpSocket &sock, int max, int message[]);

int ClientStopWait(UdpSocket &sock, int max, int message[]);

int ClientSlidingWindow(UdpSocket &sock, int max, int message[], int windowSize);

// server packet receiving functions
void ServerUnreliable(UdpSocket &sock, int max, int message[]);

void ServerReliable(UdpSocket &sock, int max, int message[]);

void ServerEarlyRetrans(UdpSocket &sock, int max, int message[], int windowSize);

enum myPartType
{
    CLIENT, SERVER
} myPart;

int main(int argc, char *argv[])
{
    int message[MSGSIZE / 4];      // prepare a 1460-byte message: 1460/4 = 365 ints;

    // Parse arguments
    if (argc == 1)
    {
        myPart = SERVER;
    }
    else if (argc == 2)
    {
        myPart = CLIENT;
    }
    else
    {
        std::cerr << "usage: " << argv[0] << " [serverIpName]" << std::endl;;
        return -1;
    }

    // Set up communication
    // Use different initial ports for client server to allow same box testing
    UdpSocket sock(PORT + myPart);
    if (myPart == CLIENT)
    {
        if (!sock.setDestAddress(argv[1], PORT + SERVER))
        {
            std::cerr << "cannot find the destination IP name: " << argv[1] << std::endl;;
            return -1;
        }
    }

    int testNumber;
    std::cerr << "Choose a testcase" << std::endl;;
    std::cerr << "   1: unreliable test" << std::endl;;
    std::cerr << "   2: stop-and-wait test" << std::endl;;
    std::cerr << "   3: sliding windows" << std::endl;;
    std::cerr << "--> ";
    std::cin >> testNumber;

    if (myPart == CLIENT)
    {
        Timer timer;
        int retransmits = 0;

        switch (testNumber)
        {
            case 1:
                timer.Start();
                ClientUnreliable(sock, MAX, message);
                std::cout << "Elapsed time = ";
                std::cout << timer.End() << std::endl;;
                break;
            case 2:
                timer.Start();
                retransmits = ClientStopWait(sock, MAX, message);
                std::cout << "Elapsed time = ";
                std::cout << timer.End() << std::endl;;
                std::cout << "retransmits = " << retransmits << std::endl;;
                break;
            case 3:
                for (int windowSize = 1; windowSize <= MAX_WIN; windowSize++)
                {
                    timer.Start();
                    retransmits = ClientSlidingWindow(sock, MAX, message, windowSize);
                    std::cout << "Window size = ";
                    std::cout << windowSize << " ";
                    std::cout << "Elapsed time = ";
                    std::cout << timer.End() << std::endl;;
                    std::cout << "retransmits = " << retransmits << std::endl;;
                }
                break;
            default:
                std::cerr << "no such test case" << std::endl;;
                break;
        }
    }
    if (myPart == SERVER)
    {
        switch (testNumber)
        {
            case 1:
                ServerUnreliable(sock, MAX, message);
                break;
            case 2:
                ServerReliable(sock, MAX, message);
                break;
            case 3:
                for (int windowSize = 1; windowSize <= MAX_WIN; windowSize++)
                {
                    ServerEarlyRetrans(sock, MAX, message, windowSize);
                }
                break;
            default:
                std::cerr << "no such test case" << std::endl;;
                break;
        }

        // The server should make sure that the last ack has been delivered to client.

        if (testNumber != 1)
        {
            if (verbose)
            {
                std::cerr << "server ending..." << std::endl;;
            }
            for (int i = 0; i < 10; i++)
            {
                sleep(1);
                int ack = MAX - 1;
                sock.ackTo((char *) &ack, sizeof(ack));
            }
        }
    }
    std::cout << "finished" << std::endl;;
    return 0;
}

// Test 1 Client
void ClientUnreliable(UdpSocket &sock, int max, int message[])
{
    // transfer message[] max times; message contains sequences number i
    for (int i = 0; i < max; i++)
    {
        message[0] = i;
        sock.sendTo((char *) message, MSGSIZE);
        if (verbose)
        {
            std::cerr << "message = " << message[0] << std::endl;;
        }
    }
    std::cout << max << " messages sent." << std::endl;;
}

// Test1 Server
void ServerUnreliable(UdpSocket &sock, int max, int message[])
{
    // receive message[] max times and do not send ack
    for (int i = 0; i < max; i++)
    {
        sock.recvFrom((char *) message, MSGSIZE);
        if (verbose)
        {
            std::cerr << message[0] << std::endl;;
        }
    }
    std::cout << max << " messages received" << std::endl;;
}

int ClientStopWait(UdpSocket &sock, int max, int message[])
{
    int msgRetransmitCount = 0;
    bool gotAck = false;
    int ack = -1;

    for (int i = 0; i < max; i++)
    {
        do
        {
            message[0] = i;
            sock.sendTo((char *) message, MSGSIZE);

            // check if server sent ack immediately
            if (sock.pollRecvFrom() == 0)
            {
                std::cout << "No ACK yet" << std::endl;
                Timer timer;
                timer.Start();
                while (timer.End() < 1500)
                {
                    if (sock.pollRecvFrom() > 0)
                    {
                        std::cout << "Got ACK!" << std::endl;
                        gotAck = true;
                        break;
                    }
                        std::cout << "Still no ACK" << std::endl;
                }
            }
            else
            {
                gotAck = true;
            }
            if(!gotAck)
            {
                std::cout << "Retransmitting " << i << std::endl;
                msgRetransmitCount++;
            }
        } while (!gotAck);

        sock.recvFrom((char *) &ack, sizeof(ack));
        std::cout << "ACK " << i << " received" << std::endl;

        if (verbose)
        {
            std::cerr << "message = " << message[0] << std::endl;;
        }
    }
    return msgRetransmitCount;
}

int ClientSlidingWindow(UdpSocket &sock, int max, int message[], int windowSize)
{
    // TODO: report
    // throughput #bits/seconds
    // measure in bits (Mb, Gb, whatever, but not bytes)
    // 20,000 * 1460 * 8 / time is takes

    // TODO: slides
    // today's lecture Slide 1 or 2 or something

    std::cout << std::endl;
    std::cout << "WINDOW SIZE " << windowSize << std::endl;
    std::cout << std::endl;

    int msgRetransmitCount = 0;
    int lastReceivedAck = -1;
    int expectedSeqAck = 0;

    for(int seqToSend = 0; seqToSend < max || expectedSeqAck < max;)
    {
        if( expectedSeqAck + windowSize > seqToSend && seqToSend < max)
        {
            std::cout << "- Inside window" << std::endl;
            message[0] = seqToSend;
            sock.sendTo((char *) message, MSGSIZE);

            if(sock.pollRecvFrom() > 0)
            {
                std::cout << "-- Ack received immediately" << std::endl;
                sock.recvFrom((char *) &lastReceivedAck, sizeof(lastReceivedAck));
                if(lastReceivedAck == expectedSeqAck)
                {
                    std::cout << "--- Ack received == Ack expected " << lastReceivedAck <<
                                                                                     std::endl;
                    expectedSeqAck++;
                }
            }
            seqToSend++;
            std::cout << "- Increasing seqToSend inside window to " << seqToSend <<
                                                                                std::endl;
        }
        else
        {
            // timeout
            std::cout << "- Inside timeout" << std::endl;
            bool gotAck = false;
            Timer timer;
            timer.Start();
            while(timer.End() < 1500)
            {
                if(sock.pollRecvFrom() > 0)
                {
                    sock.recvFrom((char *) &lastReceivedAck, sizeof(lastReceivedAck));
                    std::cout << "--- Ack received in timeout " << lastReceivedAck <<
                                                                                std::endl;
                    if(lastReceivedAck >= expectedSeqAck)
                    {
                        expectedSeqAck = lastReceivedAck + 1;
                        std::cout << "---- Ack >= expected => next expected is " <<
                                                                          expectedSeqAck << std::endl;
                        gotAck = true;
                        break;
                    }
                    else
                    {
                        std::cout << "--- Ack < expected => resending " <<
                                                                       expectedSeqAck <<
                                                                              std::endl;
                        message[0] = seqToSend; // seqToSend or expectedSeqAck
                        sock.sendTo((char *) message, MSGSIZE);
                        std::cout << "--- retransmitting in timeout" << std::endl;
                        msgRetransmitCount++;
                    }
                }
            }
            if(!gotAck)
            {
                // no ack received in timeout
                std::cout << "-- No ack received in timeout - retransmitting " <<
                                                                       expectedSeqAck <<
                          std::endl;
                message[0] = expectedSeqAck;
                sock.sendTo((char *) message, MSGSIZE);
                msgRetransmitCount++;
            }
        }
    }

    return msgRetransmitCount;


}

void ServerReliable(UdpSocket &sock, int max, int message[])
{
    //Implement this function
    return;
}

void ServerEarlyRetrans(UdpSocket &sock, int max, int message[], int windowSize)
{
    //Implement this function
    return;
}
