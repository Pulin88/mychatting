#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include "json.hpp"
#include "usermodel.hpp"
#include <mutex>
#include "offlinemessage.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;

using json = nlohmann::json;
// 处理消息的回调方法
using MsgHandler = function<void(const TcpConnectionPtr &conn,
                                 json &js, Timestamp time)>;

// 聊天服务器业务类
class ChatService
{
public:
    // 获取单例对象
    static ChatService *instance();
    // 获取消息对应的回调操作
    MsgHandler getHandler(int msgid);
    // 处理登录业务
    void login(const TcpConnectionPtr &conn,
    json &js, Timestamp time);
    // 处理注销业务
    void loginout(const TcpConnectionPtr &conn,
    json &js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn,
    json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn,
    json &js, Timestamp time);
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn,
    json &js, Timestamp time);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn,
    json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn,
    json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn,
    json &js, Timestamp time);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr&);
    // 服务器异常重置方法
    void reset();

private:
    ChatService();

    // 消息id和其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 数据操作类对象
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

    // 互斥锁保证_userConnMap的线程安全
    mutex _connMutex;
};

#endif