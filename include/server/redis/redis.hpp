#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

    // 连接redis服务器 
    bool connect();
    // 向channel发布消息
    bool publish(int channel, string message);
    // 向channel订阅消息，在shell中使用redis-cli命令进行订阅会导致阻塞，所以要把阻塞部分放到另外一个线程执行
    bool subscribe(int channel);
    // 向channel取消订阅
    bool unsubscribe(int channel);
    // 独立线程中接收订阅消息
    void observer_channel_message();
    // 设置对于订阅消息的回调操作
    void init_notify_handler(function<void(int,string)> fn);
private:
    redisContext *_publish_context;
    redisContext *_subscribe_context;
    function<void(int,string)> _notify_message_handler;
};

#endif