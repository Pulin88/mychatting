#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

class GroupUser : public User
{
private:
    string role;

public:
    string getRole() { return this->role; }
    void setRole(string role) { this->role = role; }
};

#endif