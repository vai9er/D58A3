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

def tcp(target_ip):   
    tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp_socket.connect((target_ip, 80))
    print("Connected to TCP server at {}:{}".format(target_ip, 80))

    message = "Hello, TCP!"
    tcp_socket.sendto(message, (target_ip, 80))
    print("Sent TCP message")

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

    tcp_socket.close()
    icmp_socket.close()



if __name__ == '__main__':
    iface = 'client-eth0'  
    src_mac = 'F6:0F:F4:40:CB:6E'  
    src_ip = "10.0.1.100" 
    target_mac = '5E:E5:48:C2:40:FC' 
    target_ip =  "10.0.1.1"  
    
    print("Send UDP to router eth1 (192.168.2.1)")
    target_ip =  "192.168.2.1"  

    udp(target_ip)

    print(" ")

    print("Send UDP to router eth2 (172.64.3.1)")
    target_ip =  "172.64.3.1"  

    udp(target_ip)

    print(" ")

    print("Send UDP to  router eth3 (10.0.1.1)")
    target_ip =  "10.0.1.1"  
    
    udp(target_ip)

    print(" ")

    print("Send ARP request")
    # Create an ARP request packet
    packet = create_arp_packet(src_mac, src_ip, target_mac, target_ip, op_code=1)  # op_code=1 for request

    # Send the packet
    sock = send_arp_packet(iface, packet)

    listen_for_arp_reply(sock, target_ip)

    print(" ")

    print("Send ARP reply")
    # Create an ARP request packet
    packet = create_arp_packet(src_mac, src_ip, target_mac, target_ip, op_code=2)  # op_code=1 for request

    # Send the packet
    sock = send_arp_packet(iface, packet)

    print(" ")

    print("Send invalid checksum IMCP to router eth1 (192.168.2.1)")
    target_ip =  "192.168.2.1"  

    bad_ping(target_ip)

    print(" ")

    print("Send invalid checksum IMCP to router eth2 (172.64.3.1)")
    target_ip =  "172.64.3.1"  

    bad_ping(target_ip)

    print(" ")

    print("Send invalid checksum IMCP to  router eth3 (10.0.1.1)")
    target_ip =  "10.0.1.1"  

    bad_ping(target_ip)

    print(" ")


    #ROUTER TESTS
    print("Ping router eth1 (192.168.2.1)")
    subprocess.call(["ping", "-c", "3", "192.168.2.1"])

    print(" ")

    print("Ping router eth2 (172.64.3.1)")
    subprocess.call(["ping", "-c", "3", "172.64.3.1"])

    print(" ")

    print("Ping router eth3 (10.0.1.1)")
    subprocess.call(["ping", "-c", "3", "10.0.1.1"])

    print(" ")

    print("Traceroute router eth1 (192.168.2.1)")
    subprocess.call(["traceroute", "-n", "192.168.2.1"])
    print(" ")
    print("Traceroute router  eth2 (172.64.3.1)")
    subprocess.call(["traceroute", "-n", "172.64.3.1"])

    print(" ")

    print("Traceroute router eth3 (10.0.1.1)")
    subprocess.call(["traceroute", "-n","10.0.1.1"])

    print(" ")

    #SERVER TESTS

    print("Ping server1 (192.168.2.2)")
    subprocess.call(["ping", "-c", "3", "192.168.2.2"])

    print(" ")

    print("Ping server2 (172.64.3.10)")
    subprocess.call(["ping", "-c", "3", "172.64.3.10"])

    print(" ")

    print("Traceroute  server1 (192.168.2.2)")
    subprocess.call(["traceroute", "-n", "192.168.2.2"])

    print(" ")

    print("Traceroute  server2 (172.64.3.10)")
    subprocess.call(["traceroute", "-n","172.64.3.10"])

    print(" ")

    print("wget server1 (192.168.2.2)")
    subprocess.call(["wget", "http://192.168.2.2"])

    print(" ")

    print("wget server2 (172.64.3.10)")
    subprocess.call(["wget", "http://172.64.3.10"])


    print("Destination Net Unreachable")
    subprocess.call(["ping", "-c", "3", "8.8.8.8"])
   
    print(" ")

    """
    print("Destination host Unreachable")
    subprocess.call(["link", "server1", "sw0", "down"])
    subprocess.call(["ping", "-c", "3", "192.168.2.2"])
    subprocess.call(["link", "server1", "sw0", "up"])

     print("Send TCP to router eth1 (192.168.2.1)")
    target_ip =  "192.168.2.1"  

    tcp(target_ip)

    print(" ")

    print("Send TCP to router eth2 (172.64.3.1)")
    target_ip =  "172.64.3.1"  

    tcp(target_ip)

    print(" ")

    print("Send TCP to router eth3 (10.0.1.1)")
    target_ip =  "10.0.1.1"  
    
    tcp(target_ip)

    print(" ")
    """


