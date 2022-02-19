#include "redis.hpp"
#include <iostream>

Redis::Redis()
    : _publish_context(nullptr), _subscribe_context(nullptr)
{
}
Redis::~Redis()
{
    if(_publish_context != nullptr)
        redisFree(_publish_context);
    if(_publish_context != nullptr)
        redisFree(_subscribe_context);
}

// 连接redis服务器
bool Redis::connect()
{
    // publish发布信息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379);
    if(nullptr == _publish_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }
    // subscribe订阅消息的上下文连接
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if(nullptr == _subscribe_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 在单独的线程中监听事件，有消息即给业务层上报
    thread t([&](){observer_channel_message();});
    t.detach();

    cout << "connect redis-server success!" << endl;
    return true;
}

// 向channel发布消息
bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if(nullptr == reply)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向channel订阅消息，在shell中使用redis-cli命令进行订阅会导致阻塞，所以要把阻塞部分放到另外一个线程执行
bool Redis::subscribe(int channel)
{
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite循环发送缓冲区，直到发送完毕(done设置为1)
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 向channel取消订阅
bool Redis::unsubscribe(int channel)
{
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送发送缓冲区，直到缓冲区发送完毕
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 独立线程中接收订阅消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while(REDIS_OK == redisGetReply(this->_subscribe_context, (void **)&reply))
    {
        if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        freeReplyObject(reply);
    }

    cerr << ">>>>>>>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<<<" << endl;
}

// 设置对于订阅消息的回调操作
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}