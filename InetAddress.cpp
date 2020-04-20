//
// Created by oil_you on 2019/11/3.
//

#include "InetAddress.h"
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

//     struct sockaddr_in6 {
//         sa_family_t     sin6_family;   /* address family: AF_INET6 */
//         uint16_t        sin6_port;     /* port in network byte order */
//         uint32_t        sin6_flowinfo; /* IPv6 flow information */
//         struct in6_addr sin6_addr;     /* IPv6 address */
//         uint32_t        sin6_scope_id; /* IPv6 scope-id */
//     };

static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
{
    if (ipv6)
    {

        bzero(&addr6_, sizeof addr6_);
        addr6_.sin6_family = AF_INET6;
        in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
        addr6_.sin6_addr = ip;
        addr6_.sin6_port =  htobe16(port);
    }
    else
    {
        bzero(&addr_, sizeof addr_);
        addr_.sin_family = AF_INET;
        in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
        addr_.sin_addr.s_addr = htobe32(ip);
        addr_.sin_port =  htobe16(port);
    }
}

InetAddress::InetAddress(const char *ip, uint16_t port, bool ipv6)
{
    if (ipv6)
    {
        bzero(&addr6_, sizeof addr6_);
        addr6_.sin6_family=AF_INET6;
        addr6_.sin6_port=htobe16(port);
        if (::inet_pton(AF_INET6, ip, &addr6_.sin6_addr) <= 0)
        {
            perror("InetAddress::InetAddress(IP)");
        }
    }
    else
    {
        bzero(&addr_, sizeof addr_);

        addr_.sin_family=AF_INET;
        addr_.sin_port=htobe16(port);
        if (::inet_pton(AF_INET, ip, &addr_.sin_addr) <= 0)
        {
            perror("InetAddress::InetAddress(IP)");
        }
    }
}

string InetAddress::toIp() const
{
    char buf[64] = "";
    socketOps::toIp(buf, sizeof buf, getSockAddr());
    return buf;
}

string InetAddress::toIpPort() const
{
    char buf[64] = "";
    socketOps::toIpPort(buf, sizeof buf, getSockAddr());
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return socketOps::networkToHost16(addr_.sin_port);
}