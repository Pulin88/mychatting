#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    //初始化TcpServer
    ChatServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg);
    //启动函数
    void start();

private:
    //Connetion回调函数
    void onConnection(const TcpConnectionPtr &);

    //Message回调函数
    void onMessage(const TcpConnectionPtr &, Buffer *, Timestamp);

    EventLoop *_loop;  //事件循环
    TcpServer _server; //服务器类对象
};

#endif