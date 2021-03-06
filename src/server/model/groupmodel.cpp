#include "group.hpp"
#include "groupuser.hpp"
#include "groupmodel.hpp"
#include "db.h"

// 创建群组
bool GroupModel::createGroup(Group &group)
{
     // 组装mysql语句
    char sql[1024];
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str());
            

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        { // 获取插入成功的数据主键id并设置到User中
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    errmsg = mysql.getErrmsg();
    return false;

}
// 加入群组
bool GroupModel::addGroup(int userid, int groupid, string role)
{
    // 组装mysql语句
    char sql[1024];
    sprintf(sql, "insert into groupuser(groupid,userid,grouprole) values(%d, %d, '%s')",
            groupid, userid, role.c_str());
            

    MySQL mysql;
    if (mysql.connect())
    {
        if(mysql.update(sql))
            return true;
    }

    errmsg = mysql.getErrmsg();
    return false;

}
// 登录时，显示user所加入的群
vector<Group> GroupModel::queryGroups(int userid)
{
    // 根据userid在groupuser查groupid，并在allgroup中查group信息
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join \
            groupuser b on a.id=b.groupid where b.userid=%d",
            userid);
    vector<Group> groupVec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    
    // 根据groupid在groupuser查其他userid，并在user中查user信息
    for(Group &group : groupVec)
    {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a inner join \
        groupuser b on a.id=b.userid where b.groupid=%d", group.getId());

        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}
// 群聊时，user向group的群成员发送消息
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
   char sql[1024] = {0};
   sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid,userid);
   
   vector<int> idVec;
   MySQL mysql;
   if(mysql.connect())
   {
       MYSQL_RES *res = mysql.query(sql);
       if(res != nullptr)
       {
           MYSQL_ROW row;
           while((row = mysql_fetch_row(res)) != nullptr)
           {
              idVec.push_back(atoi(row[0])); 
           }
           mysql_free_result(res);
       }
   }
   return idVec;
}