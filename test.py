import socket
import os
import struct
import time
import select
import subprocess
import fcntl


def ip_to_bin(ip):
    return struct.pack('!4B', *[int(x) for x in ip.split('.')])


def mac_to_bin(mac):
    return struct.pack('!6B', *[int(x, 16) for x in mac.split(':')])


def create_arp_packet(src_mac, src_ip, target_mac, target_ip, op_code=1):
    eth_header = mac_to_bin(target_mac) + mac_to_bin(src_mac) + struct.pack('!H', 0x0806)


    arp_header = struct.pack('!HHBBH', 1, 0x0800, 6, 4, op_code)

    arp_data = mac_to_bin(src_mac) + ip_to_bin(src_ip) + mac_to_bin(target_mac) + ip_to_bin(target_ip)


    return eth_header + arp_header + arp_data

def send_arp_packet(iface, packet):
    ETH_P_ARP = 0x0806
    
    s = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(ETH_P_ARP))
    
    s.bind((iface, ETH_P_ARP))
    
    s.send(packet)  
    print("ARP sent")
    return s  

def listen_for_arp_reply(sock, src_ip):
    print("Listening for ARP reply...")
    while True:
        packet, _ = sock.recvfrom(65535)
    
        eth_header = packet[0:14]
        eth_proto = struct.unpack("!H", eth_header[12:14])[0]

        if eth_proto == 0x0806:
            arp_header = packet[14:42]
            arp_op_code = struct.unpack("!H", arp_header[6:8])[0]

            if arp_op_code == 2:
                sender_mac = ":".join(format(ord(b), "02x") for b in arp_header[8:14])
                sender_ip = ".".join(str(ord(b)) for b in arp_header[14:18])
                target_ip = ".".join(str(ord(b)) for b in arp_header[24:28])
                
                if sender_ip  == src_ip:
                    print("Received ARP reply:")
                    print("Sender MAC:", sender_mac)
                    print("Sender IP:", sender_ip)
                    break
    sock.close()


def checksum(source_string):

    sum = 0
    count = 0
    count_to = (len(source_string) / 2) * 2

    while count < count_to:
        this_val = ord(source_string[count + 1]) * 256 + ord(source_string[count])
        sum = sum + this_val
        sum = sum & 0xffffffff
        count = count + 2

    if count_to < len(source_string):
        sum = sum + ord(source_string[len(source_string) - 1])
        sum = sum & 0xffffffff

    sum = (sum >> 16) + (sum & 0xffff)
    sum = sum + (sum >> 16)
    answer = ~sum
    answer = answer & 0xffff
    answer = answer >> 8 | (answer << 8 & 0xff00)
    return answer

def create_packet(id):
    header = struct.pack("!BBHHH", 8, 0, 0, id, 1)
    data = struct.pack("d", time.time()) 

    my_checksum = 0 #checksum(header + data)

    header = struct.pack("!BBHHH", 8, 0, my_checksum, id, 1)
    return header + data

def bad_ping(destination):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_ICMP)
        sock.settimeout(1)
    except socket.error as e:
        print("Socket could not be created. Error: {}".format(e))
        return

    packet_id = os.getpid() & 0xFFFF  
    packet = create_packet(packet_id)

    try:
        sock.sendto(packet, (destination, 1))  
        send_time = time.time()


        while True:
            start_time = time.time()
            ready = sock.recvfrom(1024)
            recv_time = time.time()
            icmp_header = ready[0][20:28]
            r_type, r_code, r_checksum, r_id, r_sequence = struct.unpack("!BBHHH", icmp_header)

            if r_id == packet_id and r_type == 0:
                print("Reply from {}: bytes={} time={:.3f} ms".format(destination, len(ready[0]), (recv_time - send_time) * 1000))
                break

            if (recv_time - start_time) > 1:
                print("Request timed out.")
                break

    except socket.timeout:
        print("Request timed out.")
    finally:
        sock.close()

def udp(target_ip):   
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


    message = "Hello, UDP!"
    udp_socket.sendto(message, (target_ip, 12345))
    print("Sent UDP message:", message)

    icmp_socket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.getprotobyname('icmp'))


    icmp_socket.settimeout(5)

    try:
        response, addr = icmp_socket.recvfrom(1024) 
        print("Received ICMP response from:", addr)

        icmp_header = response[20:28] 
        icmp_type, code, checksum, packet_id, sequence = struct.unpack("!BBHHH", icmp_header)

        print("ICMP Type:", icmp_type)
        print("ICMP Code:", code)
    except socket.timeout:
        print("No ICMP response received within timeout period.")


    udp_socket.close()
    icmp_socket.close()


def create_ip_bad_header(source_ip, dest_ip):
    """Create an IP header using given source and destination IPs."""
    version = 4
    ihl = 5  
    version_ihl = (version << 4) + ihl
    tos = 0  
    total_length = 20 + 20 
    packet_id = 54321  
    fragment_offset = 0
    ttl = 64  
    protocol = socket.IPPROTO_TCP 
    header_checksum = 0  
    src_ip = socket.inet_aton(source_ip)
    dst_ip = socket.inet_aton(dest_ip)

   
    ip_header = struct.pack('!BBHHHBBH4s4s', version_ihl, tos, total_length, packet_id,
                            fragment_offset, ttl, protocol, header_checksum, src_ip, dst_ip)

    
    header_checksum = 0 #checksum(ip_header)
    ip_header = struct.pack('!BBHHHBBH4s4s', version_ihl, tos, total_length, packet_id,
                            fragment_offset, ttl, protocol, header_checksum, src_ip, dst_ip)
    return ip_header


def create_tcp_header():
    src_port = 12345  
    dst_port = 80
    seq_num = 0
    ack_num = 0
    offset_reserved = (5 << 4) 
    tcp_flags = 2  #
    window = socket.htons(5840)  
    checksum = 0  
    urgent_pointer = 0

    tcp_header = struct.pack('!HHLLBBHHH', src_port, dst_port, seq_num, ack_num, offset_reserved, tcp_flags,
                             window, checksum, urgent_pointer)
    return tcp_header

def send_badIP(target_ip):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_RAW)
    except socket.error as e:
        print("Socket could not be created. Error Code: " + str(e[0]) + " Message " + e[1])
        return

    ip_header = create_ip_bad_header("10.0.1.100", target_ip)
    tcp_header = create_tcp_header()
    packet = ip_header + tcp_header

    sock.sendto(packet, (target_ip, 0))


if __name__ == '__main__':
    iface = 'client-eth0'  
    src_mac = 'F6:0F:F4:40:CB:6E'  
    src_ip = "10.0.1.100" 
    target_mac = '5E:E5:48:C2:40:FC' 
    target_ip =  "10.0.1.1"  
    
    print("Send UDP to router eth1 (192.168.2.1)\n")
    target_ip =  "192.168.2.1"  

    udp(target_ip)

    print(" ")

    print("Send UDP to router eth2 (172.64.3.1)\n")
    target_ip =  "172.64.3.1"  

    udp(target_ip)

    print(" ")

    print("Send UDP to router eth3 (10.0.1.1)\n")
    target_ip =  "10.0.1.1"  
    
    udp(target_ip)

    print(" ")


    print("Send UDP to server1 (192.168.2.2)\n")
    target_ip =  "192.168.2.2"  

    udp(target_ip)

    print(" ")

    print("Send UDP to server2 (172.64.3.10)\n")
    target_ip =  "172.64.3.10"  

    udp(target_ip)

    print(" ")


    """
    print("Send ARP request\n")
    packet = create_arp_packet(src_mac, src_ip, target_mac, target_ip, op_code=1) 


    sock = send_arp_packet(iface, packet)

    listen_for_arp_reply(sock, target_ip)

    print(" ")
    """


    print("Send invalid checksum ICMP to router eth1 (192.168.2.1)\n")
    target_ip =  "192.168.2.1"  

    bad_ping(target_ip)

    print(" ")

    print("Send invalid checksum ICMP to router eth2 (172.64.3.1)\n")
    target_ip =  "172.64.3.1"  

    bad_ping(target_ip)

    print(" ")

    print("Send invalid checksum ICMP to  router eth3 (10.0.1.1)\n")
    target_ip =  "10.0.1.1"  

    bad_ping(target_ip)

    print(" ")

    print("Send invalid checksum IP to router eth1 (192.168.2.1)\n")
    target_ip =  "192.168.2.1"  

    send_badIP(target_ip)

    print(" ")

    print("Send invalid checksum IP to router eth2 (172.64.3.1)\n")
    target_ip =  "172.64.3.1"  

    send_badIP(target_ip)

    print(" ")

    print("Send invalid checksum IP to  router eth3 (10.0.1.1)\n")
    target_ip =  "10.0.1.1"  

    send_badIP(target_ip)

    print(" ")


    print("Ping router eth1 (192.168.2.1)\n")
    subprocess.call(["ping", "-c", "3", "192.168.2.1"])

    print(" ")

    print("Ping router eth2 (172.64.3.1)\n")
    subprocess.call(["ping", "-c", "3", "172.64.3.1"])

    print(" ")

    print("Ping router eth3 (10.0.1.1)\n")
    subprocess.call(["ping", "-c", "3", "10.0.1.1"])

    print(" ")

    print("Traceroute router eth1 (192.168.2.1)\n")
    subprocess.call(["traceroute", "-n", "192.168.2.1"])

    print(" ")

    print("Traceroute router  eth2 (172.64.3.1)\n")
    subprocess.call(["traceroute", "-n", "172.64.3.1"])

    print(" ")

    print("Traceroute router eth3 (10.0.1.1)\n")
    subprocess.call(["traceroute", "-n","10.0.1.1"])

    print(" ")

    print("Ping server1 (192.168.2.2)\n")
    subprocess.call(["ping", "-c", "3", "192.168.2.2"])

    print(" ")

    print("Ping server2 (172.64.3.10)\n")
    subprocess.call(["ping", "-c", "3", "172.64.3.10"])

    print(" ")

    print("Traceroute  server1 (192.168.2.2)\n")
    subprocess.call(["traceroute", "-n", "192.168.2.2"])

    print(" ")

    print("Traceroute  server2 (172.64.3.10)\n")
    subprocess.call(["traceroute", "-n","172.64.3.10"])

    print(" ")

    print("wget server1 (192.168.2.2)\n")
    subprocess.call(["wget", "http://192.168.2.2"])

    print(" ")

    print("wget server2 (172.64.3.10)\n")
    subprocess.call(["wget", "http://172.64.3.10"])


    print("Destination Net Unreachable\n")
    subprocess.call(["ping", "-c", "3", "8.8.8.8"])


