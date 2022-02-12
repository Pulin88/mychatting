#include "json.hpp"
using json = nlohmann::json;


#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;

string func1()
{
   json js;
   js["msg_type"] = 2;
   js["from"] = "hu";
   js["to"] = "pulin";
   js["msg"] = "hello world";

   return js.dump(); 
}

string func2()
{
    json js;
    js["id"] = {1, 2, 3, 4};
    js["name"] = "pulin";
    // js["msg"]["zhang san"] = "hello world";
    // js["msg"]["liu shuo"]
    js["msg"] = {{"zhang san", "hello world"}, {"liu shuo", "hello world"}};
    return js.dump();
}

string func3()
{
    json js;

    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    js["list"] = vec;

    map<int, string> m;
    m.insert({1, "hu"});
    m.insert({2, "chen"});
    m.insert({3, "liu"});
    js["path"] = m;

    string sendBuf = js.dump();
    return sendBuf;
}

int main()
{
    // string recvBuf = func1();
    // json res = json::parse(recvBuf);
    // cout << res["msg_type"] << endl;
    // cout << res["from"] << endl;
    // cout << res["to"] << endl;
    // cout << res["msg"] << endl;
    
    // string recvBuf = func2();
    // json res = json::parse(recvBuf);
    // cout << res["id"][2] << endl;
    // cout << res["name"] << endl;
    
    // auto msgjs = res["msg"];
    // cout << msgjs["zhang san"] << endl;
    // cout << msgjs["liu shuo"] << endl;

    string recvBuf = func3();
    json res = json::parse(recvBuf);

    vector<int> vec = res["list"];
    for(int &a : vec)
        cout << a << " ";
    cout << endl;

    map<int, string> mp = res["path"];
    for(auto &m : mp)
        cout << m.first << " " << m.second << endl;
    cout << endl;


    return 0;

}





