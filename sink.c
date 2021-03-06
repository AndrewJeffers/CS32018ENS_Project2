
#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "core/lib/sensors.h"
#include "dev/button-sensor.h"
#include "platform/sky/dev/light-sensor.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
AUTOSTART_PROCESSES(&example_broadcast_process);
/*---------------------------------------------------------------------------*/
/*necessery golbal variables*/ 
char dataArray[20]; //data array to store incoming data from sensors
int index=0;	//index to dataArray
char sequence=0;	//data request sequence number, used for syncing the snesors
char counter=0;  //used to send different broadcasts at different intervals
/*code fails to compile if this section is removed*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  printf("broadcast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
		 //recieved broadcasts ignored
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*upon revieving a unicast, this section of code executes*/
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  printf("unicast message received from %d.%d: '%s'\n",
	 from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
	//store revieved data into dataArray
	dataArray[index] = atoi(packetbuf_dataptr());
	index++;
	if(index==19){
		index=0;
	}
}
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  static struct etimer et;
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
  PROCESS_BEGIN();
  broadcast_open(&broadcast, 129, &broadcast_call);
  unicast_open(&uc, 146, &unicast_callbacks);
  int val;
  //activate sensors
  SENSORS_ACTIVATE(light_sensor);
  //on startup, send a hop count to establish the 'network'
  char message[3];
  packetbuf_copyfrom("H0", 3);
  broadcast_send(&broadcast);
  printf("Tree build sent\n");
  while(1) {
    	/* Delay 10 seconds to allow sensors time to send data */
    	etimer_set(&et, CLOCK_SECOND * 10);	
    	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	//every 50 seconds, network maintanace is preformed
	if(counter % 5 == 0){
		// every 200 seconds all nodes have their hop counts reset and the network is re-built
		if(counter % 20 ==0){
			message[0] = 'W';
			packetbuf_copyfrom(message, 3);
	    		broadcast_send(&broadcast);
	    		printf("Hop wipe sent\n");
			etimer_set(&et, CLOCK_SECOND * 4);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et)); //wait 4 seconds for reset to propegate
			counter = 0;
		}
			message[0] = 'H';
			message[1] = '0';
			packetbuf_copyfrom(message, 3);
	    		broadcast_send(&broadcast);
	    		printf("Tree build sent\n");
			
	}else{			//if netword maintanance is not being preformed, request data from sensors.
		printf("counter %d\n", counter);
		message[0]='D';
		message[1]='0'+sequence;		// set current sequence number
		packetbuf_copyfrom(message, 3);
    		broadcast_send(&broadcast);
		printf("Data request sent\n");
		sequence++;
		if(sequence == 10){sequence=0;}		//if sequence gets to 10, reset to 0 as there is only one char space available
		//print data matrix
		int i=0; 
		while(i<20){
			
				printf("%d  ", dataArray[i]);	
				i++;			
				}
			
		printf("\n");
	}counter+=1;	
	
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/