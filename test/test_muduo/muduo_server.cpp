#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace muduo;
using namespace net;
using namespace placeholders;

class ChatServer
{
public:
    ChatServer(EventLoop* loop,
                const InetAddress& listenAddr,
                const string& nameArg)
                : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));

        _server.setMessageCallback(bind(&ChatServer::OnMessage, this, _1, _2, _3));

        _server.setThreadNum(4);
    }

    void start()
    {
        _server.start();
    }
private:
    TcpServer _server;
    EventLoop *_loop;

    void onConnection(const TcpConnectionPtr &conn) 
    {
        if(conn->connected())
        {
            cout << conn->peerAddress().toIpPort() << "->"
             << conn->localAddress().toIpPort()
             << "state:online" << endl;
        }
        else
        {
            cout << conn->peerAddress().toIpPort() << "->"
             << conn->localAddress().toIpPort()
             << "state:offline" << endl;
             conn->shutdown();  //等效于clost(fd);
             //_loop->quit();   //等效于关掉epoll循环
        }
    }

    void OnMessage(const TcpConnectionPtr &conn,
                   Buffer *buffer,
                   Timestamp time)
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data:" << buf 
        << "time:" << time.toFormattedString() << endl;
        conn->send(buf);
    }
};

int main()
{
    EventLoop loop;
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();
    return 0;
}