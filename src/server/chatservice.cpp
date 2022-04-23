// 聊天服务器
#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <string>
#include <vector>

using namespace placeholders;
using namespace muduo;
using namespace std;

// 获取单例对象
ChatService *ChatService::instance()
{
    static ChatService chat;
    return &chat;
}

//注册消息和回调操作
ChatService::ChatService()
{
    // 用户基本业务 事件+回调函数
    _msgHandlerMap.insert({LOGIN_MSG, bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务 事件+回调函数
    _msgHandlerMap.insert({CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接redis服务器
    if(_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 获取消息对应的回调操作
MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        //找不到回调操作，返回空操作并记录日志
        return [=](const TcpConnectionPtr &conn,
                   json &js, Timestamp time)
        {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    else
    {
        //找到回调操作就返回
        return _msgHandlerMap[msgid];
    }
}

// 处理登录业务 id password state
void ChatService::login(const TcpConnectionPtr &conn,
                        json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            //用户已经登录 不许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            conn->send(response.dump());
        }
        else
        {
            // 用户登录成功 记录连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            // 用户登录成功 更改连接状态
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["id"] = user.getId();
            response["name"] = user.getName();
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;

            // 查询用户是否有离线信息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                _offlineMsgModel.remove(id);
            }


            // 查询用户的好友信息
            vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty())
            {
                vector<string> vec2;
                for(User &user:userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if(groupuserVec.empty() == false)
            {
                //groups:[{id,groupname,groupdesc,users:[{}...]}...] 
                //把groupV和userV中的每个单位的键值对信息保存成json序列化并dump保存在vector<string>中
                //否则写成vector<map<string,string>>对比vector<string>来说十分不好用
                vector<string> groupV;
                for(Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for(GroupUser &user : group.getUsers())
                    {
                        json ujs;
                        ujs["id"] = user.getId();
                        ujs["name"] = user.getName();
                        ujs["state"] = user.getState();
                        ujs["role"] = user.getRole();
                        userV.push_back(ujs.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;

            }

            // 将数据发送给客户端 
            conn->send(response.dump());
        }
    }
    else
    {
        // 用户不存在 or 账号密码错误
        json response;
        response["errmsg"] = "id or password is invalid!";
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr& conn,
                      json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
            _userConnMap.erase(it);
    }

    // 用户注销，相当于下线，在redis中取消订阅
    _redis.unsubscribe(userid);

    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

// 处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn,
                      json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    if (_userModel.insert(user))
    {
        // 注册成功
        json response;
        response["id"] = user.getId();
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        // 从_userConnMap表中删除用户连接
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    // 更新数据库中用户状态
    if(user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 服务器异常重置方法
void ChatService::reset()
{
    // 把online状态的用户设置为offline
    _userModel.resetState();
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn,
                          json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线 转发消息
            it->second->send(js.dump());
            return;
        }
    }

    // 查询toid是否在线, 在线则说明在toid在其他服务器登录
    User user = _userModel.query(toid);
    if(user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    //toid不在线 存储离线信息
    _offlineMsgModel.insert(toid, js.dump());
}

// 添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn,
                            json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    if(_friendModel.insert(userid, friendid))
    {
        json response;
        response["msgid"] = ADD_FRIEND_MSG_ACK;
        response["userid"] = userid;
        response["friendid"] = friendid;
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = ADD_FRIEND_MSG_ACK;
        response["errno"] = 5;
        response["errmsg"] = _friendModel.getErrmsg();
        conn->send(response.dump());
    }
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn,
             json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group(-1, name,desc);
    if(_groupModel.createGroup(group))
    {
        _groupModel.addGroup(userid, group.getId(), "creator");
        
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["groupname"] = group.getName();
        response["groupid"] = group.getId();
        conn->send(response.dump()); 
        
    }
    else
    {
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["errno"] = 3;
        response["errmsg"] = _groupModel.getErrmsg();
        conn->send(response.dump());
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn,
             json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    if(_groupModel.addGroup(userid, groupid, "normal"))
    {
        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["groupid"] = groupid;
        response["userid"] = userid;
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["errno"] = 4;
        response["errmsg"] = _groupModel.getErrmsg();
        conn->send(response.dump());
    }
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn,
             json &js, Timestamp time)
{
   int userid = js["id"].get<int>();
   int groupid = js["groupid"].get<int>();
   vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

   lock_guard<mutex> lock(_connMutex);
   for(int id : useridVec)
   {
       auto it = _userConnMap.find(id);
       if(it != _userConnMap.end())
       {
           it->second->send(js.dump());
       }
       else
       {
            // 查询toid是否在线, 在线则说明在toid在其他服务器登录
            User user = _userModel.query(id);
            if(user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
       }
   } 
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if(it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}
