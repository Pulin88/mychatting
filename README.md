# mychatting
based on c++11, offer instant messages for multiple clients
server is divided by three parts, net-communiction, bussiness process, mysql-api.
net-communication is based on muduo, choosing bussiness process function by analysing client requirement.

### Require
1. Linux kernel version >= 3.10.0
2. GCC >= 4.7
3. [muduo](https://github.com/chenshuo/muduo)
4. Boost >= 1.69.0
5. redis >= 2.8 and hiredis port:6379
6. mysql port:3306
7. nginx
8. CMake

### Configure
1. `vim ./src/server/db/db.cpp` modify your mysql configuration information
2. add new configuration infomation as follow in your `nginx.conf`
```
stream
{
    upstream MyServer
    {
        server 127.0.0.1:6000 weight=1 max_fails=3 fail_timeout=30s;
        server 127.0.0.1:6002 weight=1 max_fails=3 fail_timeout=30s;
    }

    server
    {
        proxy_connect_timeout 1s;
        listen 8000;
        proxy_pass MyServer;
        tcp_nodelay on;
    }
}
```

### how to use it?
1. you should assure that the service of nginx:master(80),redis-server(6379),mysqld(3306) are starting.
2. ` cmake ./build/` in workspaceFolder
3. `cd ./bin`
4. `./ChatServer 127.0.0.1 6000` or `./ChatServer 127.0.0.1 6002`
5. `./ChatClient 127.0.0.1 8000`
