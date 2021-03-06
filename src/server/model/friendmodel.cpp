#include "friendmodel.hpp"
#include "db.h"

// 添加好友关系
bool FriendModel::insert(int userid, int friendid)
{
    // 组装mysql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend(userid,friendid) values(%d,%d)",
            userid, friendid);

    MySQL mysql;
    if (mysql.connect())
    {
        if(mysql.update(sql))
            return true;
    }

    errmsg = mysql.getErrmsg();
    return false;
}

// 返回用户好友列表
vector<User> FriendModel::query(int userid)
{
    // 组装mysql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid=a.id where b.userid=%d;", userid);

    MySQL mysql;
    vector<User> vec;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            // 把userid的好友信息User放入vec中返回
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != NULL)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
            return vec;   
        }
    }
    return vec;
    
}