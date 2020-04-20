//
// Created by oil_you on 2019/11/3.
//

#ifndef NETSERVER_INETADDRESS_H
#define NETSERVER_INETADDRESS_H

#include <netinet/in.h>
#include <boost/core/noncopyable.hpp>
#include <string>
#include "Socket.h"

using namespace std;

class InetAddress{
public:
    explicit  InetAddress(uint16_t port=0,bool loopbackOnly=false,bool ipV6=false);
    InetAddress(const char *ip, uint16_t port, bool ipV6=false);

    explicit  InetAddress(const struct sockaddr_in& addr):addr_(addr){}
    explicit  InetAddress(const struct sockaddr_in6& addr):addr6_(addr){}

    sa_family_t family() const { return addr_.sin_family; }

    string toIp() const;
    string toIpPort() const;
    uint16_t toPort() const;

    const sockaddr* getSockAddr() const { return (struct sockaddr*) &addr6_; }//addr6_??


private:
    union
    {
    struct sockaddr_in addr_;
    struct  sockaddr_in6 addr6_;
    };

};


#endif //NETSERVER_INETADDRESS_H
