//网络模块代码
//在ood语言中使用面向接口或回调函数来实现模块解耦
//cpp面向接口也就是虚基类
//cpp回调函数也就是functional和bind
#include "chatserver.hpp"
#include <functional>
#include <string>
#include "json.hpp"
#include "chatservice.hpp"

using namespace std;
using namespace placeholders;
using json = nlohmann::json;


//初始化TcpServer
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
: _server(loop, listenAddr, nameArg),_loop(loop)
{
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
    _server.setMessageCallback(bind(&ChatServer::onMessage,this,_1,_2,_3));
    _server.setThreadNum(4);
}

//启动函数
void ChatServer::start()
{
    _server.start();
}

//Connetion回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if(!conn->connected())
    {
        conn->shutdown();
        ChatService::instance()->clientCloseException(conn);
    }
}

//Message回调函数
void ChatServer::onMessage(const TcpConnectionPtr & conn,
                         Buffer *buffer,
                         Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    json js = json::parse(buf);
    // 网络模块与业务模块解耦合
    // 通过js["msgid"]获得回调函数
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 调用回调函数，执行业务模块的处理
    msgHandler(conn, js, time);
}