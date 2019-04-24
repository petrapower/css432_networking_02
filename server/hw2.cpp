#include <iostream>
#include "UdpSocket.h"
#include "Timer.h"
//using namespace std;

const int PORT = 40385;       // my UDP port
const int MAX = 20000;        // times of message transfer
const int MAX_WIN = 30;       // maximum window size
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
        std::cerr << "usage: " << argv[0] << " [serverIpName]" << std::endl;
        return -1;
    }

    // Set up communication
    // Use different initial ports for client server to allow same box testing
    UdpSocket sock(PORT + myPart);
    if (myPart == CLIENT)
    {
        if (!sock.setDestAddress(argv[1], PORT + SERVER))
        {
            std::cerr << "cannot find the destination IP name: " << argv[1] << std::endl;
            return -1;
        }
    }

    int testNumber;
    std::cerr << "Choose a testcase" << std::endl;
    std::cerr << "   1: unreliable test" << std::endl;
    std::cerr << "   2: stop-and-wait test" << std::endl;
    std::cerr << "   3: sliding windows" << std::endl;
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
                std::cout << timer.End() << std::endl;
                break;
            case 2:
                timer.Start();
                retransmits = ClientStopWait(sock, MAX, message);
                std::cout << "Elapsed time = ";
                std::cout << timer.End() << std::endl;
                std::cout << "retransmits = " << retransmits << std::endl;
                break;
            case 3:
                for (int windowSize = 1; windowSize <= MAX_WIN; windowSize++)
                {
                    timer.Start();
                    retransmits = ClientSlidingWindow(sock, MAX, message, windowSize);
                    std::cout << "Window size = ";
                    std::cout << windowSize << " ";
                    std::cout << "Elapsed time = ";
                    std::cout << timer.End() << std::endl;
                    std::cout << "retransmits = " << retransmits << std::endl;
                }
                break;
            default:
                std::cerr << "no such test case" << std::endl;
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
                std::cerr << "no such test case" << std::endl;
                break;
        }

        // The server should make sure that the last ack has been delivered to client.

        if (testNumber != 1)
        {
            if (verbose)
            {
                std::cerr << "server ending..." << std::endl;
            }
            for (int i = 0; i < 10; i++)
            {
                sleep(1);
                int ack = MAX - 1;
                sock.ackTo((char *) &ack, sizeof(ack));
            }
        }
    }
    std::cout << "finished" << std::endl;
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
            std::cerr << "message = " << message[0] << std::endl;
        }
    }
    std::cout << max << " messages sent." << std::endl;
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
            std::cerr << message[0] << std::endl;
        }
    }
//    std::cout << max << " messages received" << std::endl;
}

int ClientStopWait(UdpSocket &sock, int max, int message[])
{
    //Implement this function
    return -1;
}

int ClientSlidingWindow(UdpSocket &sock, int max, int message[], int windowSize)
{
    //Implement this function
    return -1;
}

void ServerReliable(UdpSocket &sock, int max, int message[])
{
    int ackToSend = -1;

    for (int sequence = 0; sequence < max; )
    {
        // get message from client
        sock.recvFrom((char *) message, MSGSIZE);

        // check if expected message seq number came
        if(message[0] == sequence)
        {
            int ack = message[0];
            sock.ackTo((char *) &ack, sizeof(ack));
            sequence++;
        }
        // else wait for the same sequence

        if (verbose)
        {
            std::cerr << message[0] << std::endl;;
        }
    }
    return;
}

void ServerEarlyRetrans(UdpSocket &sock, int max, int message[], int windowSize)
{
    bool msgsReceived[max];
    int seqExpected = 0;
    int cumulativeAck = -1;

    // init msg array
    for(int i = 0; i < max; i++)
    {
        msgsReceived[i] = false;
    }

    // until the expected number of messages is received
    for( seqExpected = 0; seqExpected < max; )
    {
        sock.recvFrom((char *) message, MSGSIZE);

        // did we receive the message in the expected order
        if(message[0] == seqExpected)
        {
            msgsReceived[seqExpected] = true;

            for(int i = seqExpected; i < max; i++)
            {
                if(msgsReceived[i])
                {
                    cumulativeAck = i;
                    seqExpected = i + 1;
                }
                else
                {
                    break;
                }
            }

        }
        else
        {
            // message came out of order
            // mark as received, but don't change expected seq or cumulative ack
            msgsReceived[message[0]] = true;
        }
        sock.ackTo((char *) &cumulativeAck, sizeof(cumulativeAck));
    }
    return;
}
