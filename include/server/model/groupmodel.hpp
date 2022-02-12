#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"

class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);
    // 加入群组
    void addGroup(int userid, int groupid, string role);
    // 登录时，显示user所加入的群
    vector<Group> queryGroups(int userid);
    // 群聊时，user向group的群成员发送消息
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif