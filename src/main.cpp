#include "TcpServer.h"
#include "EventLoop.h"
#include "WorkerThreadPool.h"
#include <iostream>
#include <functional>
#include <string>
using namespace std;

using namespace placeholders;
/*
1、组合TcpServer对象
2、创建Eventloop事件循环对象得指针
3、明确TcpServer构造函数中所需要得参数，输出ChatServer得构造函数
4、在当前服务器类的构造函数当中，注册处理相关事件的回调函数
5、设置合适的服务端线程数量，muduo库会自己分配I/0线程和worker线程

*/

class ChatServer
{
private:
    TcpServer server_;
    EventLoop *loop_; // epoll
    WorkThreadPool threadPool_;
    // 专门处理用户得连接创建和断开
    void onConnection(const ConnectionPtr &conn)
    {
        cout << "新连接到达" << endl;
    }
    // 专门处理用户的读写事件
    void onMessage(const ConnectionPtr &conn,
                   Buffer *buffer,
                   Timestamp time)
    {
        string buf = buffer->retrieveAllAsString();
        threadPool_.addtask([this, conn, time, buf]()
                            { echoMessage(conn, buf, time); });
    }

    void echoMessage(const ConnectionPtr &conn,
                     const string &buf,
                     Timestamp time)
    {
        cout << "recv data : " << buf << " time : " << time.toString() << endl;
        conn->send(buf);
    }

public:
    ChatServer(EventLoop *loop,               // 事件循环
               const InetAddress &listenAddr, // IP+port
               const string &nameArg)         // 服务器名字
        : server_(loop, listenAddr, nameArg), loop_(loop), threadPool_(4)
    {

        // 给服务器注册用户连接得创建和断开回调
        server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
        // 给服务器注册用户读写事件回调
        server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
        // 设置服务器端的线程数量   自适应：1个监听线程，3个处理工作线程
        server_.setThreadNum(4);
    }

    // 开启事件循环
    void start()
    {
        server_.start();
    }
};

int main()
{

    EventLoop loop; // epoll;
    InetAddress addr("192.168.88.133", 54321);
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // listenfd epoll_ctrl=>epollfd

    // 以阻塞的方式等待新用户连接
    // 已连接用户的读写事件
    loop.loop(); // epoll_wait

    return 0;
}