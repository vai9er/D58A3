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

  /* fill in code here */
  uint16_t ethernet_type = ethertype(packet);

  /*eth packets have minimum length - check that*/
  if(len < sizeof(sr_ethernet_hdr_t)){
    fprintf(stderr, "TOO SHORT");
    return;
  }

  printf("check COMPLETE \n\n");

  sr_ethernet_hdr_t * ethernet_hdr = (struct sr_ethernet_hdr *)packet;
  sr_arp_hdr_t* arp_hdr = get_arp_header(packet);


  if (ethernet_hdr->ether_type == htons(ethertype_arp)) {
      printf("WE HAVE A REQUEST \n\n");
      handle_arp(sr, packet, len, interface);
  } 
  else {
      printf("NOT ARP PACKET");
  }


}/* end sr_ForwardPacket */


void handle_arp(struct sr_instance* sr,
        uint8_t * packet/* lent */,
        unsigned int len,
        char* interface/* lent */)
{

    if (len < sizeof(struct sr_ethernet_hdr) + sizeof(struct sr_arp_hdr)) {
        fprintf(stderr , "** PACKET TOO SHORT \n");
        return -1;
    }

    sr_arp_hdr_t *arp_hdr;
    arp_hdr = (struct sr_arp_hdr *)(packet + sizeof(struct sr_ethernet_hdr));
    struct sr_if* iface = sr_get_interface(sr, interface);

    /* check if arp is to myself */
    if (arp_hdr->ar_tip != iface->ip) {
        return 0;
    }

    /* check if the arp is an arp request and the target is me */
    if (arp_hdr->ar_op == htons(arp_op_request)) {  
        arp_request(sr, packet, len, interface);
    }

    /* if the arp is an arp reply to me 
     * if (arp_hdr->ar_op == htons(arp_op_reply)) {
      *  
     * }  
     */
    return 0;
}

/* handle arp request to me */
void arp_request(struct sr_instance* sr,
        uint8_t * packet, 
        unsigned int len,
        char* interface) 
{
    sr_ethernet_hdr_t *ethernet_hdr;
    sr_arp_hdr_t *arp_hdr;
    struct sr_if* iface;
    uint8_t *pkt_copy;

    iface = sr_get_interface(sr, interface);

    /*copy of packet*/
    pkt_copy = (uint8_t *)malloc(len);
    memcpy(pkt_copy, packet, len);  

    ethernet_hdr = (sr_ethernet_hdr_t *)pkt_copy;
    arp_hdr = (sr_arp_hdr_t *)(pkt_copy + sizeof(struct sr_ethernet_hdr));

    /* update arp header */
    arp_hdr->ar_op = htons(arp_op_reply);
    memcpy(arp_hdr->ar_tha, arp_hdr->ar_sha, ETHER_ADDR_LEN);
    arp_hdr->ar_tip = arp_hdr->ar_sip;

    memcpy(arp_hdr->ar_sha, iface->addr, ETHER_ADDR_LEN);
    arp_hdr->ar_sip = iface->ip;

    /* update eth*/
    memcpy(ethernet_hdr->ether_dhost, ethernet_hdr->ether_shost, ETHER_ADDR_LEN);
    memcpy(ethernet_hdr->ether_shost, iface->addr, ETHER_ADDR_LEN);

    sr_send_packet(sr, pkt_copy, len, interface);
    free(pkt_copy);

    return;
}


/*------------------------------------------------------
    * go thru interface list and find our interface 
    * struct sr_if* iface = sr->if_list;
    * while (iface) {
    *    if (iface->ip == arp_hdr->ar_tip) {
    *        break;  Found the interface matching the target IP
    *    }
    *    iface = iface->next;
    * }

    * if (!iface) {
    *     Target IP is not ours; ignore the ARP request 
    *    return;
    *}
    *printf("HELLOOOOOO\n\n");
    * build a reply to send 
    *unsigned int reply_len = sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t);
    *uint8_t* reply_packet = (uint8_t*) malloc(reply_len);
    *if (!reply_packet) {
    *    fprintf(stderr, "Failed to allocate memory for ARP reply.\n");
    *    return;
    *}
    *
    * Ethernet header 
    * sr_ethernet_hdr_t* reply_eth_hdr = get_ethernet_hdr(reply_packet);
    * memcpy(reply_eth_hdr->ether_dhost, eth_hdr->ether_shost, ETHER_ADDR_LEN);
    * memcpy(reply_eth_hdr->ether_shost, iface->addr, ETHER_ADDR_LEN);
    * reply_eth_hdr->ether_type = htons(ethertype_arp);
    * 
    *  ARP header
    * sr_arp_hdr_t* reply_arp_hdr = get_arp_header(reply_packet);
    * reply_arp_hdr->ar_hrd = htons(arp_hrd_ethernet);
    * reply_arp_hdr->ar_pro = htons(ethertype_ip);
    * reply_arp_hdr->ar_hln = ETHER_ADDR_LEN;
    * reply_arp_hdr->ar_pln = sizeof(uint32_t);
    * reply_arp_hdr->ar_op = htons(arp_op_reply);
    * 
    * fill hardware access
    * memcpy(reply_arp_hdr->ar_sha, iface->addr, ETHER_ADDR_LEN);
    * 
    * sender ip address is the interface ip
    * reply_arp_hdr->ar_sip = iface->ip;
    * memcpy(reply_arp_hdr->ar_tha, arp_hdr->ar_sha, ETHER_ADDR_LEN);
    * reply_arp_hdr->ar_tip = arp_hdr->ar_sip;
    * 
    *
    * ARP reply
    * if (sr_send_packet(sr, reply_packet, reply_len, iface->name) < 0) {
    *    fprintf(stderr, "Failed to send ARP reply.\n");
    * }
    * 
    * free(reply_packet);
    * ---------------------------------------------------------------- */