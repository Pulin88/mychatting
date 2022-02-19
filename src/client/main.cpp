#include <iostream>
#include <sstream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "public.hpp"
#include "group.hpp"
#include "user.hpp"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;
// 当前系统的登录用户
User g_currentUser;
// 登录用户的好友列表
vector<User> g_currentUserFriendList;
// 登录用户的群组列表
vector<Group> g_currentUserGroupList;

// 控制主菜单页面
bool isMainMenuRunning = false;
// 登录状态
atomic_bool g_isLoginSuccess(false);

// 读写线程间的通信
sem_t rwsem;

// 接收线程 
void readTaskHandler(int clientfd);
// 获取系统时间
string getCurrentTime();
// 主聊天页面
void mainMenu(int);
// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 聊天客户端，主线程cout显示，子线程接收
int main(int argc, char** argv)
{
    if(argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }   

    // 解析得到ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client的socket
    int clientfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // client需要连接的server的ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server进行连接
    if(-1 == ::connect(clientfd, (sockaddr*)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // 信号量初始化
    sem_init(&rwsem, 0, 0);

    // 连接服务器成功，启动接收子线程
    std::thread readTask(readTaskHandler, clientfd);
    readTask.detach();

    for(;;)
    {
        // 显示登录，注册，退出
        cout << "===========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "===========================" << endl;
        cout << "choice: ";
        /**
         * 尽量不要使用cin接收整数,浮点数等，因为若cin出错，之后则无法输入
         * 应该使用getline(cin, str)把数据输入到string中，再使用stringstream转换成整数,浮点数等
         */
        string input;
        if(!getline(cin, input))
        {
            cerr << "user canceled or unrecoverable stream error" << endl;
            continue;
        }
        stringstream ss(input);
        int choice;
        if(!(ss >> choice))
        {
            if(ss.eof() || ss.bad())
                cerr << "empty-input or unrecoverable error" << endl;
            else if(ss.fail())
                cerr << "error: invalid integer input." << endl;
            cout << "example : 1" << endl;
            continue;
        }


        switch(choice)
        {
            case 1: // login
            {
               int id = 0;
               char pwd[50] = {0};
               cout << "userid: ";
               string input;
               if(!getline(cin, input))
               {
                   cerr << "user canceled or unrecoverable error" << endl;
                   continue;
               }
               stringstream ss(input);
               if(!(ss >> id))
               {
                    if(ss.eof() || ss.bad())
                        cerr << "empty input or unrecoverable error" << endl;
                    if(ss.fail())
                        cerr << "error : invalid integer input" << endl;
                    cout << "example : 2" << endl;
                    break;
               }
               cout << "userpassword: ";
               cin.getline(pwd, 50);

               json js;
               js["msgid"] = LOGIN_MSG;
               js["id"] = id;
               js["password"] = pwd;
               string request = js.dump();

               g_isLoginSuccess = false;
               int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
               if(-1 == len)
               {
                   cerr << "send login msg error:" << request << endl;
               }

                // 等待子线程处理登录响应
               sem_wait(&rwsem);

               if(g_isLoginSuccess)
               {
                   // 进入主菜单
                   isMainMenuRunning = true;
                   mainMenu(clientfd);
               }
               else
               {
                   cerr << "userid or password error" << endl;
               }
            }
            break;

            case 2: // register
            {
                char name[50] = {0};
                char pwd[50] = {0};
                cout << "username: ";
                cin.getline(name, 50);
                cout << "userpassword: ";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if(len == -1)
                {
                    cerr << "send reg msg error: " << request << endl;
                }

                sem_wait(&rwsem); // 等待子线程处理信息
            }
            break;
            
            case 3: // quit
            {
                ::close(clientfd);
                sem_destroy(&rwsem);
                exit(0);
            }
            break;

            default:
            {
                cerr << "invalid input!" << endl;
                cerr << "input must be between 1 and 3" << endl;
            }
            break;
        } 
    }
    return 0;
}

void doRegResponse(json &responsejs);
void doLoginResponse(json &responsejs);


// 子线程 接收线程
void readTaskHandler(int clientfd)
{
    for(;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if(-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        //接收ChatServer数据，反序列化
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if(ONE_CHAT_MSG == msgtype)
        {
            char output[1024] = {0};
            snprintf(output, 1024, "%s[%d]%s said: %s",
            js["time"].get<string>().c_str(),
            js["id"].get<int>(),
            js["name"].get<string>().c_str(),
            js["msg"].get<string>().c_str());
            cout << output << endl;
            continue;
        }

        if(GROUP_CHAT_MSG == msgtype)
        {
            char output[1024] = {0};
            snprintf(output, 1024, "groupmsg[%d]:%s[%d]%s said: %s",
            js["groupid"].get<int>(),
            js["time"].get<string>().c_str(),
            js["id"].get<int>(),
            js["name"].get<string>().c_str(),
            js["msg"].get<string>().c_str());
            cout << output << endl;
            continue;
        }

        if(LOGIN_MSG_ACK == msgtype)
        {
            doLoginResponse(js);
            sem_post(&rwsem);
            continue;
        }

        if(REG_MSG_ACK == msgtype)
        {
            doRegResponse(js);
            sem_post(&rwsem);
            continue;
        }

        if(CREATE_GROUP_MSG_ACK == msgtype)
        {
            if(js.contains("errno"))
            {
                cout << "create group error, reasons are as follows" << endl;
                cout << js["errmsg"] << endl;
            }
            else
            {
                printf("groupname:%s\n groupid:%d\n please remember firmly\n", 
                    js["groupname"].get<string>().c_str(), js["groupid"].get<int>());
            }
            continue;
        }

        if(ADD_GROUP_MSG_ACK == msgtype)
        {
            if(js.contains("errno"))
            {
                cout << "add group error, reasons are as follows" << endl;
                cout << js["errmsg"] << endl;
            }
            else
            {
                printf("userid:%d join in groupid:%d successfully",
                    js["userid"].get<int>(), js["groupid"].get<int>());
            }
            continue;
        }
    }
}

void doRegResponse(json &responsejs)
{
    if(0 != responsejs["errno"].get<int>())
    {
        cerr << "name is already exist, register erro!" << endl;
    }
    else
    {
        cout << "name register success, userid is" << responsejs["id"]
        << ", do not forget it!" << endl;
    }
}

void doLoginResponse(json &responsejs)
{
    if(0 != responsejs["errno"].get<int>())
    {
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
    }
    else
    {
        // 当前用户id和name
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);

        // 当前用户的好友
        if(responsejs.contains("friends"))
        {
            g_currentUserFriendList.clear();

            vector<string> vec = responsejs["friends"];
            for(string &str : vec)
            {
                json js = json::parse(str);
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        // 当前用户的群组信息
        if(responsejs.contains("groups"))
        {
            g_currentUserGroupList.clear();
            vector<string> vec1 = responsejs["groups"];
            for(string &groupstr : vec1)
            {
                json grpjs = json::parse(groupstr);
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                vector<string> vec2 = grpjs["users"];
                for(string &userstr : vec2)
                {
                    json js = json::parse(userstr);
                    GroupUser user;
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }

                g_currentUserGroupList.push_back(group);
            }
        }

        showCurrentUserData();

        // 当前用户的离线信息
        if(responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for(string &str : vec)
            {
                json js = json::parse(str);
                if(ONE_CHAT_MSG == js["msgid"].get<int>())
                {
                    char output[1024] = {0};
                    snprintf(output, 1024, "%s[%d]%s said: %s",
                    js["time"].get<string>().c_str(),
                    js["id"].get<int>(),
                    js["name"].get<string>().c_str(),
                    js["msg"].get<string>().c_str());
                    cout << output << endl;
                }
                else
                {
                    char output[1024] = {0};
                    snprintf(output, 1024, "groupmsg[%d]:%s[%d]%s said: %s",
                    js["groupid"].get<int>(),
                    js["time"].get<string>().c_str(),
                    js["id"].get<int>(),
                    js["name"].get<string>().c_str(),
                    js["msg"].get<string>().c_str());
                    cout << output << endl;
                }
            }
        }

        g_isLoginSuccess = true;
    }
}

// 登录成功用户的基本信息
void showCurrentUserData()
{
    cout << "=======================login user=======================" << endl;
    cout << "current login user => id: " << g_currentUser.getId() 
         << " name: " << g_currentUser.getName() << endl;
    cout << "-----------------------friend list-----------------------" << endl;
    if(!g_currentUserFriendList.empty())
    {
        for(User &user : g_currentUserFriendList)
            cout << user.getId() << " "
                 << user.getName() << " "
                 << user.getState() << " " << endl;
    }
    cout << "-----------------------group list-----------------------" << endl;
    if(!g_currentUserGroupList.empty())
    {
        for(Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " "
                 << group.getName() << " "
                 << group.getDesc() << " " << endl;
            for(GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " "
                     << user.getName() << " "
                     << user.getState() << " "
                     << user.getRole() << " " << endl;
            }
        }
    }
    cout << "=======================login user=======================" << endl;
}

void helpCallback(int fd = 0, string str = "");
void chatCallback(int, string);
void addfriendCallback(int, string);
void creategroupCallback(int, string);
void addgroupCallback(int, string);
void groupchatCallback(int, string);
void loginoutCallback(int, string);

// 客户端命令列表
unordered_map<string,string> commandMap = {
    {"help","显示所有支持的命令,格式help"},
    {"chat","一对一聊天,格式chat:friendid:message"},
    {"addfriend","添加好友,格式addfriend:friendid"},
    {"creategroup","创建群主,格式creategroup:groupname:groupdesc"},
    {"addgroup","加入群组,格式addgroup:groupid"},
    {"groupchat","群聊,格式groupchat:groupid:message"},
    {"loginout","注销,格式loginout"}
};

//回调函数
unordered_map<string, function<void(int,string)>> commandHandlerMap = {
    {"help",helpCallback},
    {"chat",chatCallback},
    {"addfriend",addfriendCallback},
    {"creategroup",creategroupCallback},
    {"addgroup",addgroupCallback},
    {"groupchat",groupchatCallback},
    {"loginout",loginoutCallback}
};

//聊天主界面
void mainMenu(int clientfd)
{
    helpCallback();

    char buffer[1024] = {0};
    while(isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; //存储命令
        int idx = commandbuf.find(":"); //find从0开始算起
        if(idx == -1)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
        }
        else
        {   // 调用相应命令的回调函数，解耦合，substr(开始位置，截断的元素个数)
            it->second(clientfd, commandbuf.substr(idx+1));
        }
    }
}

void helpCallback(int,string)
{
    cout << "show command list" << endl;
    for(auto &p : commandMap)
        cout << p.first << " : " << p.second << endl;
    cout << endl;
}

void addfriendCallback(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["friendid"] = clientfd;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(-1 == len)
        cerr << "send addfriend msg error" << endl;
}

void chatCallback(int clientfd, string str)
{
    int idx = str.find(":");
    if(idx == -1)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }
    
    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx+1);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(-1  == len)
        cerr << "send chat msg error ->" << buffer << endl;
}

void creategroupCallback(int clientfd, string str)
{
     int idx = str.find(":");
     if(idx == -1)
     {
         cerr << "creategroup command invalid!" << endl;
         return;
     }

     string groupname = str.substr(0, idx);
     string groupdesc = str.substr(idx+1);
     
     json js;
     js["msgid"] = CREATE_GROUP_MSG;
     js["id"] = g_currentUser.getId();
     js["groupname"] = groupname;
     js["groupdesc"] = groupdesc;
     string buffer = js.dump();
     
     int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
     if(-1 == len)
     {
         cerr << "send creategroup msg error ->" << buffer << endl; 
     }

     // 通知用户群组id和群组name
     /* 接收代码写到子线程中去
     char msg[1024] = {0};
     len = recv(clientfd, msg, 1024, 0);
     if(len == -1)
     {
         cerr << "recv creategroup msg error" << endl;
     }
     json jsmsg = json::parse(msg);
     if(jsmsg.contains("errno"))
     {
         cout << jsmsg["errmsg"] << endl;
     }
     else
     {
        printf("groupname:%s\n groupid:%d\n please remember firmly\n", jsmsg["groupname"].get<string>().c_str(), jsmsg["groupid"].get<int>());
     }
     */
}

void addgroupCallback(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(-1 == len)
    {
        cerr << "send addgroup msg error" << endl;
    }
}

void groupchatCallback(int clientfd, string str)
{
    int idx = str.find(":");
    if(-1 == idx)
    {
        cerr << "grouphat command invalid!" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx+1);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] =  g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(-1 == len)
        cerr << "send groupchat msg error->" << buffer << endl;
}

void loginoutCallback(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(-1 == len)
        cerr << "send loginout msg error->" << buffer << endl;
    else 
        isMainMenuRunning = false;
}

string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[64] = {0};
    snprintf(date, 64, "%d-%02d-%02d %02d:%02d:%02d",
        (int)ptm->tm_year+1900, (int)ptm->tm_mon+1, (int)ptm->tm_mday,
        (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}
