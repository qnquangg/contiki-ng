/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

 #include "contiki.h"
 #include "net/routing/routing.h"
 #include "net/netstack.h"
 #include "net/ipv6/simple-udp.h"
 #include "net/ipv6/uiplib.h"
 #include "sys/clock.h"
 #include <stdio.h>
 
 #include "sys/log.h"
 #define LOG_MODULE "Unicast"
 #define LOG_LEVEL LOG_LEVEL_INFO
 
 #define UDP_PORT 1903
 
 static struct simple_udp_connection udp_conn;
 static uint32_t total_receive = 0;
 
 PROCESS(sink_process, "Receiver / RPL Member");
 AUTOSTART_PROCESSES(&sink_process);
 /*---------------------------------------------------------------------------*/
 static void
 udp_rx_callback(struct simple_udp_connection *c,
                 const uip_ipaddr_t *sender_addr,
                 uint16_t sender_port,
                 const uip_ipaddr_t *receiver_addr,
                 uint16_t receiver_port,
                 const uint8_t *data,
                 uint16_t datalen)
 {
   total_receive++;
 
   LOG_INFO_("Received from sink ");
//    LOG_INFO_6ADDR(sender_addr);
   LOG_INFO_("with message: '%.*s', total_received %d \n", datalen, (char *)data, total_receive);
 }
 /*---------------------------------------------------------------------------*/
 PROCESS_THREAD(sink_process, ev, data)
 {
   PROCESS_BEGIN();
 
   /* Initialize UDP connection */
   simple_udp_register(&udp_conn, UDP_PORT, NULL,
                       UDP_PORT, udp_rx_callback);
 
   while (1)
   {
     PROCESS_WAIT_EVENT();
   }
 
   PROCESS_END();
 }
 /*---------------------------------------------------------------------------*/
 