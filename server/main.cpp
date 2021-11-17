#include<iostream>
#include<string>
#include<mutex>
#include<thread>
#include<future>
#include<chrono>
#include<WS2tcpip.h>

#pragma comment (lib, "ws2_32.lib")


class Client {
public:
    SOCKET socket;
    sockaddr_in socketAddr;
    int size;
    std::string Host;
    std::string Service;
    Client()
    {

    }

    Client(SOCKET socket,sockaddr_in socketAddr,int size)
    {
        this->socket = socket;
        this->socketAddr = socketAddr;
        this->size = size;
        char host[NI_MAXHOST];
        char service[NI_MAXSERV];
        ZeroMemory(host, NI_MAXHOST);
        ZeroMemory(service, NI_MAXSERV);
        if (getnameinfo((sockaddr*)&socketAddr, sizeof(socketAddr), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
        {
            Host = std::string(host);
            Service = std::string(service);
        }
        else
        {
            Host = std::string(host);
            Service=std::string(inet_ntop(AF_INET, &socketAddr.sin_addr, host, NI_MAXHOST));
        }
    }




};

struct IP {
    std::string Host;
    std::string Service;
    IP()
    {
        Host = "";
        Service = "";
    }
    IP(std::string Host, std::string Service)
    {
        this->Host = Host;
        this->Service = Service;
    }
};


struct Message {
    std::string msg;
    std::vector<IP> receivers;
    Message(std::string m)
    {
        msg = m;
    }
};

std::mutex MUTEX;
std::vector<Client> allClients;
std::vector<Message> messages;


Client newConnection(SOCKET serverSocket)
{
    sockaddr_in client;
    int clientSize = sizeof(client);

    SOCKET  s;
    int numTries=0;
    while (numTries<10)
    {
        s = accept(serverSocket, (sockaddr*)&client, &clientSize);
        if (s != INVALID_SOCKET)
            break;
        numTries++;
    }
    if(s==INVALID_SOCKET)
    {
        return Client(s,client,-1);
    }
    
    
    Client ret(s, client, clientSize);
    
    MUTEX.lock();
    allClients.push_back(ret);
    MUTEX.unlock();
    
    return ret;
}

void Reciver(Client client)
{
    char buf[4096];
    int bytesReceived;
    int j = 0;
    while (true)
    {
        bytesReceived = recv(client.socket, buf, 4096, 0);
       
        MUTEX.lock();
        std::cout << std::string("r start") << client.Service<< std::endl;

        if (bytesReceived > 0)
        {
            messages.push_back(Message(std::string(buf)));
            for (int i = 0; i < allClients.size(); i++)
            {
                if (allClients[i].Host != client.Host || allClients[i].Service != client.Service)
                    messages[messages.size() - 1].receivers.push_back(IP(allClients[i].Host, allClients[i].Service));
            }
        }
        else
        {
            
            for (j = 0; j < allClients.size(); j++)
            {
                if (allClients[j].Host == client.Host && allClients[j].Service == client.Service)
                    break;
            }
            if (j < allClients.size())
            {
                messages.push_back(Message("delete me"));
                messages[messages.size() - 1].receivers.push_back(IP(client.Host, client.Service));
            }


            /*
            std::cout << "{\n";
            for (int i = 0; i < messages.size(); i++)
            {
                std::cout << messages[i].msg<<" [";
                for (int j = 0; j < messages[i].receivers.size(); j++)
                {
                    std::cout << messages[i].receivers[j].Service << " ";
                }
                std::cout << "]\n";
            }
            std::cout << "}\n";
            */
            std::cout << "r end" << std::endl;
            
            MUTEX.unlock();
            break;
        }
        /*
        std::cout << "{\n";
        for (int i = 0; i < messages.size(); i++)
        {
            std::cout << messages[i].msg<<" [";
            for (int j = 0; j < messages[i].receivers.size(); j++)
            {
                std::cout << messages[i].receivers[j].Service << " ";
            }
            std::cout << "]\n";
        }
        std::cout << "}\n";
        */
        std::cout << "r end" << std::endl;
        
        MUTEX.unlock();
    }
}

void Sender(Client client)
{
    int sendRes;
    int j;
    bool ERR = false;
    while (true)
    {
        MUTEX.lock();
        ERR = false;
        for (int i = 0; i < messages.size();)
        {
            if (messages[i].receivers.size() == 0)
                messages.erase(messages.begin() + i);
            else
            {
                for (j = 0; j < messages[i].receivers.size(); j++)
                {
                    if (messages[i].receivers[j].Host == client.Host && messages[i].receivers[j].Service == client.Service)
                        break;
                }
                if (j < messages[i].receivers.size())
                {
                    sendRes = send(client.socket, messages[i].msg.c_str(), messages[i].msg.length() + 1, 0);
                    if (sendRes == SOCKET_ERROR)
                        ERR = true;
                    messages[i].receivers.erase(messages[i].receivers.begin() + j);
                }
                i++;
            }
        }
        
        if (ERR)
        {
            for (int i = 0; i < allClients.size(); i++)
            {
                if (allClients[i].socket == client.socket)
                {
                    allClients.erase(allClients.begin() + i);
                    break;
                }
            }
            closesocket(client.socket);
            
            MUTEX.unlock();
            break;
        }
       
        MUTEX.unlock();
    }
}



int main()
{
    //Initialize winsock
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);

    int wsOk = WSAStartup(ver, &wsData);
    if (wsOk != 0)
    {
        std::cout << "failed1\n";
        return 0;
    }

    // create a socket
    SOCKET Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (Socket == INVALID_SOCKET)
    {
        std::cout << "failed1\n";
        return 0;
    }


    // Bind the socket to an ip and port
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8001);
    addr.sin_addr.S_un.S_addr = INADDR_ANY;

    bind(Socket, (sockaddr*)&addr, sizeof(addr));

    //tell winsock the socket is for listening
    listen(Socket, SOMAXCONN);
    int timeOut;
    std::cout<<"what sould be the time out(s) till the server shuts down when there are no connections: ";
    std::cin>>timeOut;
    std::cout<<"starting server\n";
    std::vector<std::future<void>> receivers, senders;
    std::future<Client>* newProcess;
    std::future_status status, status1, status2;
    Client s;
    int count = 0;
    while (true)
    {
        newProcess = new std::future<Client>();
        *(newProcess) = std::async(std::launch::async, newConnection, Socket);
       
        count=0;
        do {
            status = newProcess->wait_for(std::chrono::milliseconds(100));
            if (status == std::future_status::ready)
            {
                s = newProcess->get();
                break;
            }
            else
            {
                for (int i = 0; i < receivers.size();)
                {
                    status1 = receivers[i].wait_for(std::chrono::milliseconds(0));
                    status2 = senders[i].wait_for(std::chrono::milliseconds(0));
                    if (status1 == std::future_status::ready && status2 == std::future_status::ready)
                    {
                        std::cout << "one deleted\n";
                        receivers.erase(receivers.begin() + i);
                        senders.erase(senders.begin() + i);
                    }
                    else
                        i++;
                }
                
                if (receivers.size() == 0)
                    count++;
                if (count > 10*timeOut)
                    break;
            }

        } while (status != std::future_status::ready);
        std::cout<<"ok\n";
        
        
        if (count > 10*timeOut)
        {
            break;
        }
        delete newProcess;
        if(s.size!=-1)
        {
            std::cout << "new client " << s.Host << " port " << s.Service << std::endl;
            receivers.push_back(std::future<void>());
            senders.push_back(std::future<void>());
            receivers[receivers.size() - 1] = std::async(std::launch::async, Reciver, s);
            senders[senders.size() - 1] = std::async(std::launch::async, Sender, s);
        }
        


    }
    std::cout<<"ending server\n";
    //close listening socket
    closesocket(Socket);

    //shutdown winsock
    WSACleanup();
    
    return 0;
}