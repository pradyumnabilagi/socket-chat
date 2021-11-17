
#include<iostream>
#include<thread>
#include<string>
#include<WS2tcpip.h>


#pragma comment  (lib,"ws2_32.lib")

void func1(SOCKET sock)
{
    char buf[4096];
    while(true)
    {   
        ZeroMemory(buf,4096);
        int bytesReceieved=recv(sock,buf,4096,0);
        if(bytesReceieved>0)
        {
            std::cout<<"  server>"<<std::string(buf,0,bytesReceieved)<<std::endl;
        }
    }

}


void func2(SOCKET sock)
{
    std::string UserInput;
    while(true)
    {
        
        std::getline(std::cin,UserInput);
        std::cout<<"OK"<<UserInput<<std::endl;
        
        if(UserInput.size()>0)
        {
            int senResult=send(sock,UserInput.c_str(),UserInput.size()+1,0);
            if(senResult==SOCKET_ERROR)
            {
                std::cout<<"errroro\n";
                
            }
        }
        if(UserInput=="exit")
            break;
    }


}






void main()
{
    std::string ipAddress="127.0.0.1";
    int port= 8001;


    WSAData data;
    WORD ver = MAKEWORD(2,2);
    int wsResult = WSAStartup(ver,&data);
    if(wsResult != 0)
    {
        std::cout<<"can't start winsock err #"<<wsResult<<std::endl;
        return;
    }


    SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
    if(sock == INVALID_SOCKET)
    {
        std::cout<< "can't create socket ,Err #"<<WSAGetLastError()<<std::endl;
        WSACleanup();
        return;
    }


    sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET , ipAddress.c_str() , &addr.sin_addr);


    int connResult = connect(sock, (sockaddr*)&addr , sizeof(addr));
    if(connResult == SOCKET_ERROR)
    {
        std::cout<< "can't connect to server , Err #" << WSAGetLastError() <<std::endl;
        closesocket(sock);
        WSACleanup();
        return;
    }


    char buf[4096];
    std::string userInput;

    std::thread Reciver(func1,sock),Sender(func2,sock);

    Sender.join();
    Reciver.~thread();


    closesocket(sock);
    WSACleanup();

}




















