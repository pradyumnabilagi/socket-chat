#include<iostream>
#include<WS2tcpip.h>
#include<thread>

#pragma comment (lib, "ws2_32.lib")

void func(SOCKET Socket)
{
    //wait for a connection
    sockaddr_in client;
    int clientSize= sizeof(client);

    SOCKET  clientSocket = accept(Socket,(sockaddr*)&client,&clientSize);
    
    


    //accept messages and echo it back     
    char buf[4096];

    while(true)
    {
        ZeroMemory(buf,4096);

        int bytesReceived = recv(clientSocket, buf, 4096,0);
        if(bytesReceived==SOCKET_ERROR)
        {
            std::cout<<"f";
            break;
        }

        if(bytesReceived == 0)
        {
            std::cout<< "Client disconnected"<<std::endl;
            break;
        }

        send(clientSocket, buf,bytesReceived +1,0);

    }
    
    // close the sock
    closesocket(clientSocket);

}


void main()
{
    //Initialize winsock
    WSADATA wsData;
    WORD ver=MAKEWORD(2,2);

    int wsOk= WSAStartup(ver,&wsData);
    if(wsOk!=0)
    {
        std::cout<<"failed1\n";
        return;
    }    

    // create a socket
    SOCKET Socket= socket(AF_INET, SOCK_STREAM,0);
    if(Socket == INVALID_SOCKET)
    {
        std::cout<<"failed1\n";
        return;
    }


    // Bind the socket to an ip and port
    sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(8001);
    addr.sin_addr.S_un.S_addr= INADDR_ANY;

    bind(Socket,(sockaddr*)&addr,sizeof(addr));

    //tell winsock the socket is for listening
    listen(Socket,SOMAXCONN);


    std::thread workerProcess1(func,Socket),workerProcess2(func,Socket);

    workerProcess1.join();
    workerProcess2.join();

    //close listening socket
    closesocket(Socket);

    //shutdown winsock
    WSACleanup();
}