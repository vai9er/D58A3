Ben Wilson 1007289024
Gabriel Vainer 1007121204
Howard Yang {student num}

Ben's Contribution:

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

Gabriel Contribution:

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

-----------------------------------------------------------------------------
Testing (If we use my test.py)

Test Cases:

Send UDP to router eth1 (192.168.2.1)

('Sent UDP message:', 'Hello, UDP!')
('Received ICMP response from:', ('192.168.2.1', 0))
('ICMP Type:', 3)
('ICMP Code:', 3)
 
Send UDP to router eth2 (172.64.3.1)

('Sent UDP message:', 'Hello, UDP!')
('Received ICMP response from:', ('172.64.3.1', 0))
('ICMP Type:', 3)
('ICMP Code:', 3)
 
Send UDP to  router eth3 (10.0.1.1)

('Sent UDP message:', 'Hello, UDP!')
('Received ICMP response from:', ('10.0.1.1', 0))
('ICMP Type:', 3)
('ICMP Code:', 3)
 
Send ARP request

ARP sent
Listening for ARP reply...
Received ARP reply:
('Sender MAC:', '76:0c:ee:07:d8:96')
('Sender IP:', '10.0.1.1')
 
Send invalid checksum IMCP to router eth1 (192.168.2.1)

Request timed out.
 
Send invalid checksum IMCP to router eth2 (172.64.3.1)

Request timed out.
 
Send invalid checksum IMCP to  router eth3 (10.0.1.1)

Request timed out.
 
Ping router eth1 (192.168.2.1)

PING 192.168.2.1 (192.168.2.1) 56(84) bytes of data.
64 bytes from 192.168.2.1: icmp_seq=1 ttl=64 time=20.2 ms
64 bytes from 192.168.2.1: icmp_seq=2 ttl=64 time=31.3 ms
64 bytes from 192.168.2.1: icmp_seq=3 ttl=64 time=30.5 ms

--- 192.168.2.1 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2006ms
rtt min/avg/max/mdev = 20.211/27.360/31.326/5.065 ms
 
Ping router eth2 (172.64.3.1)

PING 172.64.3.1 (172.64.3.1) 56(84) bytes of data.
64 bytes from 172.64.3.1: icmp_seq=1 ttl=64 time=8.32 ms
64 bytes from 172.64.3.1: icmp_seq=2 ttl=64 time=50.7 ms
64 bytes from 172.64.3.1: icmp_seq=3 ttl=64 time=13.8 ms

--- 172.64.3.1 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2005ms
rtt min/avg/max/mdev = 8.318/24.268/50.655/18.793 ms
 
Ping router eth3 (10.0.1.1)

PING 10.0.1.1 (10.0.1.1) 56(84) bytes of data.
64 bytes from 10.0.1.1: icmp_seq=1 ttl=64 time=7.75 ms
64 bytes from 10.0.1.1: icmp_seq=2 ttl=64 time=52.9 ms
64 bytes from 10.0.1.1: icmp_seq=3 ttl=64 time=24.8 ms

--- 10.0.1.1 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2005ms
rtt min/avg/max/mdev = 7.748/28.491/52.887/18.608 ms
 
Traceroute router eth1 (192.168.2.1)

traceroute to 192.168.2.1 (192.168.2.1), 30 hops max, 60 byte packets
 1  192.168.2.1  53.060 ms  52.563 ms  52.276 ms
 
Traceroute router  eth2 (172.64.3.1)

traceroute to 172.64.3.1 (172.64.3.1), 30 hops max, 60 byte packets
 1  172.64.3.1  35.912 ms  35.772 ms  35.701 ms
 
Traceroute router eth3 (10.0.1.1)

traceroute to 10.0.1.1 (10.0.1.1), 30 hops max, 60 byte packets
 1  10.0.1.1  76.762 ms  76.630 ms  76.544 ms
 
Ping server1 (192.168.2.2)

PING 192.168.2.2 (192.168.2.2) 56(84) bytes of data.
64 bytes from 192.168.2.2: icmp_seq=2 ttl=63 time=235 ms
64 bytes from 192.168.2.2: icmp_seq=1 ttl=63 time=1248 ms
64 bytes from 192.168.2.2: icmp_seq=3 ttl=63 time=92.6 ms

--- 192.168.2.2 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2005ms
rtt min/avg/max/mdev = 92.575/525.064/1247.746/514.304 ms, pipe 2
 
Ping server2 (172.64.3.10)

PING 172.64.3.10 (172.64.3.10) 56(84) bytes of data.
64 bytes from 172.64.3.10: icmp_seq=1 ttl=63 time=215 ms
64 bytes from 172.64.3.10: icmp_seq=2 ttl=63 time=21.7 ms
64 bytes from 172.64.3.10: icmp_seq=3 ttl=63 time=29.4 ms

--- 172.64.3.10 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2008ms
rtt min/avg/max/mdev = 21.713/88.725/215.053/89.382 ms
 
Traceroute  server1 (192.168.2.2)

traceroute to 192.168.2.2 (192.168.2.2), 30 hops max, 60 byte packets
 1  10.0.1.1  39.819 ms  39.456 ms  39.265 ms
 2  192.168.2.2  67.935 ms  67.104 ms  109.016 ms
 
Traceroute  server2 (172.64.3.10)

traceroute to 172.64.3.10 (172.64.3.10), 30 hops max, 60 byte packets
 1  10.0.1.1  35.692 ms  38.322 ms  81.617 ms
 2  172.64.3.10  114.734 ms  117.225 ms  124.507 ms
 
wget server1 (192.168.2.2)

--2024-10-30 17:06:11--  http://192.168.2.2/
Connecting to 192.168.2.2:80... connected.
HTTP request sent, awaiting response... 200 OK
Length: 161 [text/html]
Saving to: ‘index.html.3’

index.html.3        100%[===================>]     161  --.-KB/s    in 0s      

2024-10-30 17:06:12 (20.4 MB/s) - ‘index.html.3’ saved [161/161]

 
wget server2 (172.64.3.10)

--2024-10-30 17:06:12--  http://172.64.3.10/
Connecting to 172.64.3.10:80... connected.
HTTP request sent, awaiting response... 200 OK
Length: 161 [text/html]
Saving to: ‘index.html.4’

index.html.4        100%[===================>]     161  --.-KB/s    in 0s      

2024-10-30 17:06:12 (19.0 MB/s) - ‘index.html.4’ saved [161/161]

Destination Net Unreachable

PING 8.8.8.8 (8.8.8.8) 56(84) bytes of data.
From 8.8.8.8 icmp_seq=1 Destination Net Unreachable
From 8.8.8.8 icmp_seq=2 Destination Net Unreachable
From 8.8.8.8 icmp_seq=3 Destination Net Unreachable

--- 8.8.8.8 ping statistics ---
3 packets transmitted, 0 received, +3 errors, 100% packet loss, time 2004ms
