# NetSocket
本项目是在Linux环境下使用开发的高并发多Reactor多线程模型的网络通信框架，利用C++11新特性开发，去除了对第三方库的依赖。实现了I/O线程池与工作线程池分离，具体耗时业务卸载工作线程进一步提高并发数。实现网络和业务模块解耦，开发人员能够加专注于对业务的开发。


# 编译方式
cd build
rm -rf *
cmake ..
make

# 简单示例
在src目录下有一个main.cpp 里面实现了一个简单的ChatServer，通过仿照该类进行配置，类似main函数的调用就可以正常启动服务器。
可以自行扩展相关业务，参照main.cpp中定义自己的业务类。
