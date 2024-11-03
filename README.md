Ben Wilson 1007289024
Gabriel Vainer 1007121204
Howard Yang 1006722478

### Ben's Contribution:

Find longest common prefix:

    struct sr_rt *sr_longest_prefix(struct sr_instance* sr, uint32_t ip)

    sr: Pointer to the router instance which contains the routing table.
    ip: The IP address for which we need to find the longest prefix match.

    This function iterates over the entries in sr's routing_table to identify
    the entry with the longest prefix match for ip, determined by comparing each 
    routing entry's destination and subnet mask with ip. The routing entry with 
    the longest match is returned.

    Returns NULL if no match is found in the routing table.


Send ICMP echos:

    int send_icmp_echo(struct sr_instance* sr, uint8_t *eth, uint8_t *ip, uint8_t * packet)

    sr: Pointer to the router instance that contains the routing information.
    eth: Pointer to the Ethernet header of the received packet
    ip: Pointer to the IP header the received packet
    packet: Pointer to the complete original packet.
    
    This function creates and sends an ICMP Echo Reply packet (type 8) 
    to the longest prefix maatch between the soruce IP address of ip 
    and the routing entires of sr's routing table.

    Sends the constructed packet with sr_send_packet.

    Returns 0 on successful sending of the ICMP packet, -1 on failure.


Send ICMP errors:

    int send_icmp(struct sr_instance* sr, uint8_t *eth, uint8_t *ip, uint8_t * packet, uint8_t icmp_type, uint8_t icmp_code)

    sr: Pointer to the router instance that contains the routing information.
    eth: Pointer to the Ethernet header of the received packet
    ip: Pointer to the IP header the received packet
    packet: Pointer to the complete original packet.
    icmp_type: The ICMP type.
    icmp_code: The ICMP code.

    This function creates and sends an ICMP packet fo type: icmp_type 
    and with error code: icmp_code: The ICMP code to the longest prefix maatch 
    between the soruce IP address of ip and the routing entires 
    of sr's routing table.

    Sends the constructed packet with sr_send_packet.
    
    Returns 0 on successful sending of the ICMP packet, -1 on failure.

Testing:
    I wrote test.py a simple python testing script that automatically runs most 
    of our test cases.

### Gabriel Contribution:

void handle_arp(struct sr_instance* sr, uint8_t* packet, unsigned int len, char* interface)

    sr: Pointer to the router instance handling the ARP packet.
    packet: Pointer to the incoming ARP packet data.
    len: Length of the ARP packet.
    interface: Name of the interface on which the ARP packet was received.

    This function processes incoming ARP packets, identifying if the packet is a request or reply. If it’s a request,
    it generates and sends an ARP reply back to the sender. If it’s a reply, it updates the ARP cache with the sender’s
    IP and MAC information and processes any packets waiting for this ARP response.

    No return value.

void sr_arpcache_sweepreqs(struct sr_instance *sr)

    sr: Pointer to the router instance that maintains the ARP cache.

    This function sweeps through all outstanding ARP requests, handling each one with `handle_arpreq`. It 
    determines if a request needs to be retransmitted or destroyed if it has exceeded the retry limit.

    No return value.

void handle_arpreq(struct sr_instance *sr, struct sr_arpreq *req)

    sr: Pointer to the router instance handling the ARP request.
    req: Pointer to the ARP request that needs to be handled.

    This function manages pending ARP requests by either retransmitting an ARP request if the wait time is exceeded or 
    sending an ICMP Host Unreachable message if the request retry limit is reached. It constructs Ethernet, IP, and ICMP 
    headers and sends the ICMP error response to any packets waiting for the unreachable ARP target.

    No return value.

### Howard's Contribution

Packet forwarding
    I worked on the packet forwarding which is integrated directly into the handle_packet function which builds on the ARP
    and ICMP functions that were created. The packet forwarding allows the client to reach either server through the router's interfaces
    and vice-versa.
    
    When receiving an IP packet that is not destined for the router, it first checks if for the longest prefix match
    with the destination IP and the routing table. If there is no match then I send an ICMP packet type 3/code 0 NET unreachable using
    Ben's send_icmp helper function.

    If the longest prefix match returns a match, we must then check the ARP cache for the MAC address of the destination IP. If the entry is missing from
    the ARP table then we queue the ARP request into the ARP cache's request queue using
        struct sr_arpreq *sr_arpcache_queuereq(struct sr_arpcache *cache, uint32_t ip, uint8_t *packet, unsigned int packet_len, char *iface)
    
    The arp sweep reqs function written by Gabe will eventually send out ARP request and wait for a response for a maximum of 5 times.

    I also send an ICMP packet type 11/code 0 back to the sender when the TTL field on the IP header reaches 0. This allows commands like
    traceroute to work.

    Before forwarding IP packets and responding to ICMP echoes, I check the checksum on the IP header to ensure no bits have been flipped while in transport-
    if the checksum check fails, then we just drop the packet   

    All this allows the client to traceroute, wget, and ping either server.

### test.py output (ran from client on mininet)

Ran with client python test.py

-----------------------------------------------------------------------------
Test Cases

UDP, invalid checksum ICMP, invalid checksum IP, Ping router,
Ping servers, Traceroute router, Traceroute server, wget servers,
Destination Net Unreachable.


Instruction to run:

    mininet> client python test.py

Output:
Send UDP to router eth1 (192.168.2.1)

('Sent UDP message:', 'Hello, UDP!')
('Received ICMP response from:', ('10.0.1.1', 0))
('ICMP Type:', 3)
('ICMP Code:', 3)
 
Send UDP to router eth2 (172.64.3.1)

('Sent UDP message:', 'Hello, UDP!')
('Received ICMP response from:', ('10.0.1.1', 0))
('ICMP Type:', 3)
('ICMP Code:', 3)
 
Send UDP to router eth3 (10.0.1.1)

('Sent UDP message:', 'Hello, UDP!')
('Received ICMP response from:', ('10.0.1.1', 0))
('ICMP Type:', 3)
('ICMP Code:', 3)
 
Send UDP to server1 (192.168.2.2)

('Sent UDP message:', 'Hello, UDP!')
('Received ICMP response from:', ('192.168.2.2', 0))
('ICMP Type:', 3)
('ICMP Code:', 3)
 
Send UDP to server2 (172.64.3.10)

('Sent UDP message:', 'Hello, UDP!')
('Received ICMP response from:', ('172.64.3.10', 0))
('ICMP Type:', 3)
('ICMP Code:', 3)
 
Send invalid checksum ICMP to router eth1 (192.168.2.1)

Request timed out.
 
Send invalid checksum ICMP to router eth2 (172.64.3.1)

Request timed out.
 
Send invalid checksum ICMP to  router eth3 (10.0.1.1)

Request timed out.
 
Send invalid checksum IP to router eth1 (192.168.2.1)

 
Send invalid checksum IP to router eth2 (172.64.3.1)

 
Send invalid checksum IP to  router eth3 (10.0.1.1)

 
Ping router eth1 (192.168.2.1)

PING 192.168.2.1 (192.168.2.1) 56(84) bytes of data.
64 bytes from 10.0.1.1: icmp_seq=1 ttl=100 time=48.1 ms
64 bytes from 10.0.1.1: icmp_seq=2 ttl=100 time=41.2 ms
64 bytes from 10.0.1.1: icmp_seq=3 ttl=100 time=21.2 ms

--- 192.168.2.1 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2003ms
rtt min/avg/max/mdev = 21.193/36.822/48.120/11.411 ms
 
Ping router eth2 (172.64.3.1)

PING 172.64.3.1 (172.64.3.1) 56(84) bytes of data.
64 bytes from 10.0.1.1: icmp_seq=1 ttl=100 time=42.8 ms
64 bytes from 10.0.1.1: icmp_seq=2 ttl=100 time=39.3 ms
64 bytes from 10.0.1.1: icmp_seq=3 ttl=100 time=23.7 ms

--- 172.64.3.1 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2004ms
rtt min/avg/max/mdev = 23.655/35.269/42.823/8.335 ms
 
Ping router eth3 (10.0.1.1)

PING 10.0.1.1 (10.0.1.1) 56(84) bytes of data.
64 bytes from 10.0.1.1: icmp_seq=1 ttl=100 time=2.28 ms
64 bytes from 10.0.1.1: icmp_seq=2 ttl=100 time=25.9 ms
64 bytes from 10.0.1.1: icmp_seq=3 ttl=100 time=53.2 ms

--- 10.0.1.1 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2004ms
rtt min/avg/max/mdev = 2.277/27.118/53.200/20.807 ms
 
Traceroute router eth1 (192.168.2.1)

traceroute to 192.168.2.1 (192.168.2.1), 30 hops max, 60 byte packets
 1  10.0.1.1  9.162 ms  9.279 ms  9.672 ms
 
Traceroute router  eth2 (172.64.3.1)

traceroute to 172.64.3.1 (172.64.3.1), 30 hops max, 60 byte packets
 1  10.0.1.1  41.199 ms  41.195 ms  41.194 ms
 
Traceroute router eth3 (10.0.1.1)

traceroute to 10.0.1.1 (10.0.1.1), 30 hops max, 60 byte packets
 1  10.0.1.1  30.596 ms  30.838 ms  31.374 ms
 
Ping server1 (192.168.2.2)

PING 192.168.2.2 (192.168.2.2) 56(84) bytes of data.
64 bytes from 192.168.2.2: icmp_seq=1 ttl=63 time=6.85 ms
64 bytes from 192.168.2.2: icmp_seq=2 ttl=63 time=79.6 ms
64 bytes from 192.168.2.2: icmp_seq=3 ttl=63 time=17.7 ms

--- 192.168.2.2 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2004ms
rtt min/avg/max/mdev = 6.849/34.705/79.606/32.054 ms
 
Ping server2 (172.64.3.10)

PING 172.64.3.10 (172.64.3.10) 56(84) bytes of data.
64 bytes from 172.64.3.10: icmp_seq=1 ttl=63 time=3.04 ms
64 bytes from 172.64.3.10: icmp_seq=2 ttl=63 time=72.2 ms
64 bytes from 172.64.3.10: icmp_seq=3 ttl=63 time=17.6 ms

--- 172.64.3.10 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2004ms
rtt min/avg/max/mdev = 3.044/30.962/72.222/29.775 ms
 
Traceroute  server1 (192.168.2.2)

traceroute to 192.168.2.2 (192.168.2.2), 30 hops max, 60 byte packets
 1  10.0.1.1  8.999 ms  9.022 ms  9.324 ms
 2  192.168.2.2  18.924 ms  19.222 ms  19.488 ms
 
Traceroute  server2 (172.64.3.10)

traceroute to 172.64.3.10 (172.64.3.10), 30 hops max, 60 byte packets
 1  10.0.1.1  44.004 ms  43.941 ms  43.916 ms
 2  172.64.3.10  75.122 ms  76.116 ms  76.977 ms
 
wget server1 (192.168.2.2)

--2024-11-03 12:59:27--  http://192.168.2.2/
Connecting to 192.168.2.2:80... connected.
HTTP request sent, awaiting response... 200 OK
Length: 161 [text/html]
Saving to: ‘index.html.11’

index.html.11       100%[===================>]     161  --.-KB/s    in 0s      

2024-11-03 12:59:27 (106 MB/s) - ‘index.html.11’ saved [161/161]

 
wget server2 (172.64.3.10)

--2024-11-03 12:59:27--  http://172.64.3.10/
Connecting to 172.64.3.10:80... connected.
HTTP request sent, awaiting response... 200 OK
Length: 161 [text/html]
Saving to: ‘index.html.12’

index.html.12       100%[===================>]     161  --.-KB/s    in 0s      

2024-11-03 12:59:27 (97.6 MB/s) - ‘index.html.12’ saved [161/161]

Destination Net Unreachable

PING 8.8.8.8 (8.8.8.8) 56(84) bytes of data.
From 10.0.1.1 icmp_seq=1 Destination Net Unreachable
From 10.0.1.1 icmp_seq=2 Destination Net Unreachable
From 10.0.1.1 icmp_seq=3 Destination Net Unreachable

--- 8.8.8.8 ping statistics ---
3 packets transmitted, 0 received, +3 errors, 100% packet loss, time 2004ms


### Host manual down test:

Destination Host Unreachable

Instructions to run:

    mininet> link server1 sw0 down
    mininet> client ping -c 3 server1

Output:

PING 192.168.2.2 (192.168.2.2) 56(84) bytes of data.
From 192.168.2.2 icmp_seq=3 Destination Host Unreachable
From 192.168.2.2 icmp_seq=2 Destination Host Unreachable
From 192.168.2.2 icmp_seq=1 Destination Host Unreachable

--- 192.168.2.2 ping statistics ---
3 packets transmitted, 0 received, +3 errors, 100% packet loss, time 2043ms
pipe 3