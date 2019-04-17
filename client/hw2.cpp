#include <iostream>
#include "UdpSocket.h"
#include "Timer.h"
using namespace std;

const int PORT = 12345;       // my UDP port
const int MAX = 20000;        // times of message transfer
const int MAX_WIN = 30;       // maximum window size
const bool verbose = false;   //use verbose mode for more information during run

// client packet sending functions
void ClientUnreliable(UdpSocket &sock, int max, int message[]);
int ClientStopWait(UdpSocket &sock, int max, int message[]);
int ClientSlidingWindow(UdpSocket &sock, int max, int message[],int windowSize);

// server packet receiving functions
void ServerUnreliable(UdpSocket &sock, int max, int message[]);
void ServerReliable(UdpSocket &sock, int max, int message[]);
void ServerEarlyRetrans(UdpSocket &sock, int max, int message[],int windowSize );

enum myPartType {CLIENT, SERVER} myPart;

int main( int argc, char *argv[] ) 
{
    int message[MSGSIZE/4]; 	  // prepare a 1460-byte message: 1460/4 = 365 ints;

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
        cerr << "usage: " << argv[0] << " [serverIpName]" << endl;
        return -1;
    }

    // Set up communication
    // Use different initial ports for client server to allow same box testing
    UdpSocket sock( PORT + myPart );  
    if (myPart == CLIENT)
    {
        if (! sock.setDestAddress(argv[1], PORT + SERVER)) 
        {
            cerr << "cannot find the destination IP name: " << argv[1] << endl;
            return -1;
        }
    }

    int testNumber;
    cerr << "Choose a testcase" << endl;
    cerr << "   1: unreliable test" << endl;
    cerr << "   2: stop-and-wait test" << endl;
    cerr << "   3: sliding windows" << endl;
    cerr << "--> ";
    cin >> testNumber;

    if (myPart == CLIENT) 
    {
        Timer timer;           
        int retransmits = 0;   

        switch(testNumber) 
        {
        case 1:
            timer.Start();
            ClientUnreliable(sock, MAX, message); 
            cout << "Elasped time = ";  
            cout << timer.End( ) << endl;
            break;
        case 2:
            timer.Start();   
            retransmits = ClientStopWait(sock, MAX, message); 
            cout << "Elasped time = "; 
            cout << timer.End( ) << endl;
            cout << "retransmits = " << retransmits << endl;
            break;
        case 3:
            for (int windowSize = 1; windowSize <= MAX_WIN; windowSize++ ) 
            {
	        timer.Start( );
	        retransmits = ClientSlidingWindow(sock, MAX, message, windowSize);
	        cout << "Window size = ";  
	        cout << windowSize << " ";
	        cout << "Elasped time = "; 
	        cout << timer.End( ) << endl;
	        cout << "retransmits = " << retransmits << endl;
            }
            break;
        default:
            cerr << "no such test case" << endl;
            break;
        }
    }
    if (myPart == SERVER) 
    {
        switch(testNumber) 
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
	            ServerEarlyRetrans( sock, MAX, message, windowSize );
                }
                break;
            default:
                cerr << "no such test case" << endl;
                break;
        }

        // The server should make sure that the last ack has been delivered to client.
        
        if (testNumber != 1)
        {
            if (verbose)
            {
                cerr << "server ending..." << endl;
            }
            for ( int i = 0; i < 10; i++ ) 
            {
                sleep( 1 );
                int ack = MAX - 1;
                sock.ackTo( (char *)&ack, sizeof( ack ) );
            }
        }
    }
    cout << "finished" << endl;
    return 0;
}

// Test 1 Client
void ClientUnreliable(UdpSocket &sock, int max, int message[]) 
{
    // transfer message[] max times; message contains sequences number i
    for ( int i = 0; i < max; i++ ) 
    {
        message[0] = i;                            
        sock.sendTo( ( char * )message, MSGSIZE ); 
        if (verbose)
        {
            cerr << "message = " << message[0] << endl;
        }
    }
    cout << max << " messages sent." << endl;
}

// Test1 Server
void ServerUnreliable(UdpSocket &sock, int max, int message[]) 
{
    // receive message[] max times and do not send ack
    for (int i = 0; i < max; i++) 
    {
        sock.recvFrom( ( char * ) message, MSGSIZE );
        if (verbose)
        {  
            cerr << message[0] << endl;
        }                    
    }
    cout << max << " messages received" << endl;
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
   //Implement this function
   return;
}

void ServerEarlyRetrans(UdpSocket &sock, int max, int message[],int windowSize )
{
   //Implement this function
   return;
}
