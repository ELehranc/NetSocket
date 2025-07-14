#pragma once
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
// 封装socket地址类型
class InetAddress
{
public:
    explicit InetAddress(uint port);
    explicit InetAddress(std::string ip, uint port);
    explicit InetAddress(const sockaddr_in &addr) : addr_(addr) {}
    InetAddress() {}

    
    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const sockaddr_in *getSockAddr() const;
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

private:
    struct sockaddr_in addr_;
};