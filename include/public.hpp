#ifndef PUBLIC_H
#define PUBLIC_H

//server与client的公共文件
enum EnMsgType
{
    LOGIN_MSG = 1, //登录消息
    LOGIN_MSG_ACK,      //登录响应  errno=1 errmsg=id or password is invalid!
    LOGINOUT_MSG,       // 注销消息
    REG_MSG,        //注册消息
    REG_MSG_ACK,     //注册响应 errno=2 errmsg=this account is using, input another!
    ONE_CHAT_MSG,    //聊天消息
    ADD_FRIEND_MSG,  //添加好友

    CREATE_GROUP_MSG,  // 创建群组
    CREATE_GROUP_MSG_ACK,  // 创建群组响应 errno=3 errmsg=
    ADD_GROUP_MSG,      // 加入群组
    ADD_GROUP_MSG_ACK,  // 加入群组响应 errno=4 errmsg=
    GROUP_CHAT_MSG      // 群聊
};

#endif