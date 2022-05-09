#!/usr/bin/env python
import socket 

group="224.1.1.1"
port=6000

ttl=2

sock= socket.socket(socket.AF_INET, 
    socket.SOCK_DGRAM,
    socket.IPPROTO_UDP)

sock.setsockopt(socket.IPPROTO_IP,
    socket.IP_MULTICAST_TTL,ttl)

sock.sendto(b"Hello", (group, port))