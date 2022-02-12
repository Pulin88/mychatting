#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"
// user表的数据操作类
class UserModel
{
public:
    // user表的数据增加方法
    bool insert(User &user);

    // 根据用户编号查询user
    User query(int id);

    // 更新用户状态信息 id state
    bool updateState(User user);

    // 重置用户状态信息
    void resetState();
};

#endif
