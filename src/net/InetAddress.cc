#include "InetAddress.h"

#include <strings.h>
#include <string.h>
#include <iostream>

InetAddress::InetAddress(uint port)
{
    std::string ip = "192.168.88.133";
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

InetAddress::InetAddress(std::string ip, uint port)
{
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const
{
    char buf[64] = {0};

    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));

    return buf;
}
std::string InetAddress::toIpPort() const
{
    char buf[64] = {0};

    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));

    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf + end, ":%u", port);
    return buf;
}
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

const sockaddr_in *InetAddress::getSockAddr() const
{
    return &addr_;
}
