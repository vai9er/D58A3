Ben Wilson 1007289024

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
('Sender MAC:', 'd2:76:bb:86:f6:62')
('Sender IP:', '10.0.1.1')
 
Send ARP reply
ARP sent
 
Send invalid checksum IMCP to router eth1 (192.168.2.1)
Request timed out.
 
Send invalid checksum IMCP to router eth2 (172.64.3.1)
Request timed out.
 
Send invalid checksum IMCP to  router eth3 (10.0.1.1)
Request timed out.
 
Ping router eth1 (192.168.2.1)
PING 192.168.2.1 (192.168.2.1) 56(84) bytes of data.
64 bytes from 192.168.2.1: icmp_seq=1 ttl=64 time=15.0 ms
64 bytes from 192.168.2.1: icmp_seq=2 ttl=64 time=33.1 ms
64 bytes from 192.168.2.1: icmp_seq=3 ttl=64 time=16.7 ms

--- 192.168.2.1 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2006ms
rtt min/avg/max/mdev = 14.956/21.588/33.095/8.167 ms
 
Ping router eth2 (172.64.3.1)
PING 172.64.3.1 (172.64.3.1) 56(84) bytes of data.
64 bytes from 172.64.3.1: icmp_seq=1 ttl=64 time=58.7 ms
64 bytes from 172.64.3.1: icmp_seq=2 ttl=64 time=16.1 ms
64 bytes from 172.64.3.1: icmp_seq=3 ttl=64 time=33.2 ms

--- 172.64.3.1 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2002ms
rtt min/avg/max/mdev = 16.089/35.980/58.700/17.510 ms
 
Ping router eth3 (10.0.1.1)
PING 10.0.1.1 (10.0.1.1) 56(84) bytes of data.
64 bytes from 10.0.1.1: icmp_seq=1 ttl=64 time=16.7 ms
64 bytes from 10.0.1.1: icmp_seq=2 ttl=64 time=9.53 ms
64 bytes from 10.0.1.1: icmp_seq=3 ttl=64 time=12.1 ms

--- 10.0.1.1 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2008ms
rtt min/avg/max/mdev = 9.528/12.769/16.701/2.968 ms
 
Traceroute router eth1 (192.168.2.1)
traceroute to 192.168.2.1 (192.168.2.1), 30 hops max, 60 byte packets
 1  192.168.2.1  17.734 ms  41.259 ms *
 
Traceroute router  eth2 (172.64.3.1)
traceroute to 172.64.3.1 (172.64.3.1), 30 hops max, 60 byte packets
 1  172.64.3.1  261.210 ms  275.023 ms  298.189 ms
 
Traceroute router eth3 (10.0.1.1)
traceroute to 10.0.1.1 (10.0.1.1), 30 hops max, 60 byte packets
 1  10.0.1.1  227.661 ms  243.885 ms  251.286 ms
 
Ping server1 (192.168.2.2)
PING 192.168.2.2 (192.168.2.2) 56(84) bytes of data.

--- 192.168.2.2 ping statistics ---
3 packets transmitted, 0 received, 100% packet loss, time 2003ms

 
Ping server2 (172.64.3.10)
PING 172.64.3.10 (172.64.3.10) 56(84) bytes of data.
64 bytes from 172.64.3.10: icmp_seq=1 ttl=63 time=1139 ms
64 bytes from 172.64.3.10: icmp_seq=2 ttl=63 time=130 ms
64 bytes from 172.64.3.10: icmp_seq=3 ttl=63 time=59.0 ms

--- 172.64.3.10 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2013ms
rtt min/avg/max/mdev = 58.958/442.572/1139.076/493.348 ms, pipe 2
 
Traceroute  server1 (192.168.2.2)
traceroute to 192.168.2.2 (192.168.2.2), 30 hops max, 60 byte packets
 1  10.0.1.1  26.725 ms  77.715 ms  80.703 ms
 2  192.168.2.2  418.729 ms  436.800 ms  453.591 ms
 
Traceroute  server2 (172.64.3.10)
traceroute to 172.64.3.10 (172.64.3.10), 30 hops max, 60 byte packets
 1  10.0.1.1  77.560 ms  135.854 ms  133.490 ms
 2  172.64.3.10  367.692 ms  411.505 ms  429.457 ms
 
wget server1 (192.168.2.2)
--2024-10-30 16:10:55--  http://192.168.2.2/
Connecting to 192.168.2.2:80... connected.
HTTP request sent, awaiting response... 200 OK
Length: 161 [text/html]
Saving to: ‘index.html.7’

index.html.7        100%[===================>]     161  --.-KB/s    in 0s      

2024-10-30 16:10:55 (6.43 MB/s) - ‘index.html.7’ saved [161/161]

 
wget server2 (172.64.3.10)
--2024-10-30 16:10:55--  http://172.64.3.10/
Connecting to 172.64.3.10:80... connected.
HTTP request sent, awaiting response... 200 OK
Length: 161 [text/html]
Saving to: ‘index.html.8’

index.html.8        100%[===================>]     161  --.-KB/s    in 0s      

2024-10-30 16:10:56 (7.06 MB/s) - ‘index.html.8’ saved [161/161]

Destination Net Unreachable
PING 8.8.8.8 (8.8.8.8) 56(84) bytes of data.
From 8.8.8.8 icmp_seq=1 Destination Net Unreachable
From 8.8.8.8 icmp_seq=2 Destination Net Unreachable
From 8.8.8.8 icmp_seq=3 Destination Net Unreachable

--- 8.8.8.8 ping statistics ---
3 packets transmitted, 0 received, +3 errors, 100% packet loss, time 2005ms