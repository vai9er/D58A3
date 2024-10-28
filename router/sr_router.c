/**********************************************************************
 * file:  sr_router.c
 * date:  Mon Feb 18 12:50:42 PST 2002
 * Contact: casado@stanford.edu
 *
 * Description:
 *
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing.
 *
 **********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_arpcache.h"
#include "sr_utils.h"

/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr)
{
    /* REQUIRES */
    assert(sr);

    /* Initialize cache and cache cleanup thread */
    sr_arpcache_init(&(sr->cache));

    pthread_attr_init(&(sr->attr));
    pthread_attr_setdetachstate(&(sr->attr), PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_t thread;

    pthread_create(&thread, &(sr->attr), sr_arpcache_timeout, sr);
    
    /* Add initialization code here! */

} /* -- sr_init -- */

struct sr_rt *sr_longest_prefix(struct sr_instance* sr, uint32_t ip)
{
	struct sr_rt *table = NULL;
  struct sr_rt *longest= NULL;
	unsigned long maxlen = 0;
	
	for (table = sr->routing_table; table != NULL; table = table->next) {

    
		if (((table->dest.s_addr & table->mask.s_addr) == (ip & table->mask.s_addr)) && (maxlen <= table->mask.s_addr)) {
			maxlen = table->mask.s_addr;
			longest = table;
		}
	}
	return longest;
}


int send_icmp(struct sr_instance* sr, uint8_t *eth, uint8_t *ip, uint8_t * packet, uint8_t icmp_type, uint8_t icmp_code)
{
  /*Cast Headers*/
  sr_ethernet_hdr_t *eth_hdr = (sr_ethernet_hdr_t *)(eth);
  sr_ip_hdr_t *ip_hdr = (sr_ip_hdr_t *)(ip);

  /*Extract IP Info*/
  uint32_t srcip = ip_hdr->ip_dst;
  uint32_t desip = ip_hdr->ip_src;
  uint16_t ip_id = htons(ip_hdr->ip_id) + 1;

  /*Get Data*/
  uint8_t *data = packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);
  uint16_t data_len = htons(ip_hdr->ip_len) - sizeof(sr_ip_hdr_t) - sizeof(sr_icmp_hdr_t);
               
  /*Build ICMP Header*/
	sr_icmp_hdr_t *new_icmp_hdr;

	uint32_t icmp_len = sizeof(sr_icmp_hdr_t) + data_len;
  
	new_icmp_hdr = malloc(sizeof(sr_icmp_hdr_t));
	new_icmp_hdr->icmp_type = icmp_type;
	new_icmp_hdr->icmp_code = icmp_code;
  new_icmp_hdr->icmp_sum = 0;
	new_icmp_hdr->icmp_sum = cksum(new_icmp_hdr, sizeof(sr_icmp_hdr_t));

  /*Build IP Header*/
	sr_ip_hdr_t *new_ip_hdr;

	new_ip_hdr = (sr_ip_hdr_t*) malloc(sizeof(sr_ip_hdr_t));

  new_ip_hdr->ip_hl = ip_hdr->ip_hl;
  new_ip_hdr->ip_v = ip_hdr->ip_v;
	new_ip_hdr->ip_tos = ip_hdr->ip_tos;
	new_ip_hdr->ip_len = htons(sizeof(sr_ip_hdr_t) + icmp_len);
	new_ip_hdr->ip_id = htons(ip_id);
	new_ip_hdr->ip_off = htons(IP_DF);
	new_ip_hdr->ip_ttl = 64;
	new_ip_hdr->ip_p = ip_protocol_icmp;
	new_ip_hdr->ip_src = srcip;
	new_ip_hdr->ip_dst = desip;
  new_ip_hdr->ip_sum = 0;
	new_ip_hdr->ip_sum = cksum(new_ip_hdr, sizeof(sr_ip_hdr_t));

  /*Build Ethernet Header*/
	sr_ethernet_hdr_t *new_eth_hdr;
	new_eth_hdr = (sr_ethernet_hdr_t*)malloc(sizeof(sr_ethernet_hdr_t));

	memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost, ETHER_ADDR_LEN);
	memcpy(new_eth_hdr->ether_shost, eth_hdr->ether_dhost, ETHER_ADDR_LEN);
	new_eth_hdr->ether_type = htons(ethertype_ip);


  /*Assemble Packet*/
	uint32_t len = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + icmp_len;
	uint8_t* new_packet = malloc(len);
	memcpy(new_packet, new_eth_hdr, sizeof(sr_ethernet_hdr_t));
	memcpy(new_packet + sizeof(sr_ethernet_hdr_t), new_ip_hdr, sizeof(sr_ip_hdr_t));
	memcpy(new_packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t), new_icmp_hdr, icmp_len);
  memcpy(new_packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t)+ sizeof(sr_icmp_hdr_t), data, data_len);


  /*Run LPM*/
	struct sr_rt *rt = sr_longest_prefix(sr, desip);
	if (rt == NULL){
    return -1;
  }

  int res = sr_send_packet(sr, new_packet, len, rt->interface);

  /*DEBUG REMOVE BEFORE SUBMIT*/
  printf("NEW! \n");
  print_hdrs(new_packet, len);

	free(new_icmp_hdr);
	free(new_ip_hdr);
	free(new_eth_hdr);
	free(new_packet);

  return res;
}



/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr,
        uint8_t * packet/* lent */,
        unsigned int len,
        char* interface/* lent */)
{
  /* REQUIRES */
  assert(sr);
  assert(packet);
  assert(interface);

  printf("*** -> Received packet of length %d \n",len);


  /*print_hdr_eth(packet);*/
  uint16_t ethernet_type = ethertype(packet);
  fprintf(stderr, "type: %d\n", ethernet_type);
  sr_ethernet_hdr_t *eth_hdr = (sr_ethernet_hdr_t *)packet;


  /* fill in code here */

  /*eth packets have minimum length - check that
  check checksum stuff
  validation as we go*/
  int minlength = sizeof(sr_ethernet_hdr_t);
  if (len < minlength) {
    fprintf(stderr, "Failed to print ETHERNET header, insufficient length\n");
    return;
  }

  uint16_t ethtype = ethertype(packet);
  print_hdr_eth(packet);

  if (ethtype == ethertype_ip) { /* IP */
    minlength += sizeof(sr_ip_hdr_t);
    if (len < minlength) {
      fprintf(stderr, "Failed to print IP header, insufficient length\n");
      return;
    }

    sr_ip_hdr_t *ip_hdr = (sr_ip_hdr_t *)(packet + sizeof(sr_ethernet_hdr_t));
    print_hdr_ip(ip_hdr);
    uint8_t ip_proto = ip_protocol(ip_hdr);

    struct sr_if *interf;
		for (interf = sr->if_list; interf != NULL; interf = interf->next){

      if (interf->ip == ip_hdr->ip_dst) { 

        if(ip_hdr->ip_p == ip_protocol_tcp || ip_hdr->ip_p == ip_protocol_udp) {
          fprintf(stderr, "UDP/TCP \n");
          send_icmp(sr, eth_hdr, ip_hdr, packet, 3, 3);
          return;
        }

        if (ip_proto == ip_protocol_icmp) { /* ICMP */
          minlength += sizeof(sr_icmp_hdr_t);
          if (len < minlength){
            fprintf(stderr, "Failed to print ICMP header, insufficient length\n");
            return;
          } 

          send_icmp(sr, eth_hdr, ip_hdr, packet, 0, 0);
          return;
        }
			}
    }

    /* Forward */
    struct sr_rt *rt_entry = sr_longest_prefix(sr, ip_hdr->ip_dst);
    if (rt_entry == NULL) { /* SEND ICMP UNREACHABLE */ 
      send_icmp(sr, eth_hdr, ip_hdr, packet, 3, 0);
      return;
    }
    char *fwd_interface = rt_entry->interface;

    /* CHECK ARP CACHE */ 
    struct sr_arpentry *arp_entry = sr_arpcache_lookup(&sr->cache, ip_hdr->ip_dst);

    if (arp_entry == NULL) { /* ARP CACHE MISS */
      /* QUEUE UP ARP REQ to be sent */
      sr_arpcache_queuereq(&sr->cache, ip_hdr->ip_dst, packet, len, fwd_interface);
      return;
    }

    sr_ethernet_hdr_t *fwd_eth_hdr;
    fwd_eth_hdr = (sr_ethernet_hdr_t *)malloc(sizeof(sr_ethernet_hdr_t));

    memcpy(fwd_eth_hdr->ether_dhost, arp_entry->mac, ETHER_ADDR_LEN);
    memcpy(fwd_eth_hdr->ether_shost, eth_hdr->ether_dhost, ETHER_ADDR_LEN);

    uint8_t *fwd_packet = (uint8_t *)malloc(len); /* the length of the forwarded packet is the same as the received */

    memcpy(fwd_packet, packet, len); /* except for the ethernet header, everything else is the same (src ip and dst ip stay the same) */
    memcpy(fwd_packet, fwd_eth_hdr, sizeof(sr_ethernet_hdr_t));

    sr_send_packet(sr, fwd_packet, len, fwd_interface);

    free(fwd_packet);
    free(fwd_eth_hdr);
  } else if (ethtype == ethertype_arp) { /* ARP */
    handle_arp(sr, packet, len, interface);
  }  else {
    fprintf(stderr, "Unrecognized Ethernet Type: %d\n", ethtype);
  }

}/* end sr_ForwardPacket */


void handle_arp(struct sr_instance* sr, uint8_t* packet, unsigned int len, char* interface) {
    if (len < sizeof(struct sr_ethernet_hdr) + sizeof(struct sr_arp_hdr)) {
        fprintf(stderr, "** PACKET TOO SHORT \n");
        return;
    }

    sr_arp_hdr_t* arp_hdr = (sr_arp_hdr_t*)(packet + sizeof(struct sr_ethernet_hdr));
    struct sr_if* iface = sr_get_interface(sr, interface);

    if (arp_hdr->ar_tip != iface->ip) {
        return;
    }

    if (arp_hdr->ar_op == htons(arp_op_request)) {
        printf("WE HAVE A REQUEST \n\n");

        sr_ethernet_hdr_t* ethernet_hdr = (sr_ethernet_hdr_t*)packet;
        
        uint8_t* pkt_copy = (uint8_t*)malloc(len);
        memcpy(pkt_copy, packet, len);

        sr_arp_hdr_t* new_arp_hdr = (sr_arp_hdr_t*)(pkt_copy + sizeof(struct sr_ethernet_hdr));

        sr_ethernet_hdr_t* new_eth_hdr = (sr_ethernet_hdr_t*)pkt_copy;
        memcpy(new_eth_hdr->ether_dhost, ethernet_hdr->ether_shost, ETHER_ADDR_LEN);
        memcpy(new_eth_hdr->ether_shost, iface->addr, ETHER_ADDR_LEN);
        new_eth_hdr->ether_type = htons(ethertype_arp);

        new_arp_hdr->ar_hrd = arp_hdr->ar_hrd;
        new_arp_hdr->ar_pro = arp_hdr->ar_pro;
        new_arp_hdr->ar_hln = arp_hdr->ar_hln;
        new_arp_hdr->ar_pln = arp_hdr->ar_pln;
        new_arp_hdr->ar_op = htons(arp_op_reply);
        memcpy(new_arp_hdr->ar_tha, arp_hdr->ar_sha, ETHER_ADDR_LEN);
        new_arp_hdr->ar_tip = arp_hdr->ar_sip;
        memcpy(new_arp_hdr->ar_sha, iface->addr, ETHER_ADDR_LEN);
        new_arp_hdr->ar_sip = iface->ip;

        printf("Send ARP reply packet: \n");
        print_hdrs(pkt_copy, len);
        sr_send_packet(sr, pkt_copy, len, interface);
        printf("ARP reply sent\n");
        free(pkt_copy);
    }

    else if (arp_hdr->ar_op == htons(arp_op_reply)) {

        uint32_t sender_ip = arp_hdr->ar_sip;
        unsigned char sender_mac[ETHER_ADDR_LEN];
        memcpy(sender_mac, arp_hdr->ar_sha, ETHER_ADDR_LEN);

        /* insert the senders IP and MAC into the ARP cache */
        struct sr_arpreq *req = sr_arpcache_insert(&(sr->cache), sender_mac, sender_ip);

        /* if there is a pending ARP request, process it */
        if (req) {
            struct sr_packet *packet = req->packets;
            while (packet) {
                /* get eth header and update it with destincation and source MAC */
                sr_ethernet_hdr_t *eth_hdr = (sr_ethernet_hdr_t *)(packet->buf);

                memcpy(eth_hdr->ether_dhost, sender_mac, ETHER_ADDR_LEN);
                struct sr_if* iface = sr_get_interface(sr, packet->iface);
                memcpy(eth_hdr->ether_shost, iface->addr, ETHER_ADDR_LEN);

                sr_send_packet(sr, packet->buf, packet->len, packet->iface);

                packet = packet->next;
            }
            /* destroy the arp request*/
            sr_arpreq_destroy(&(sr->cache), req);
        }
    }
}