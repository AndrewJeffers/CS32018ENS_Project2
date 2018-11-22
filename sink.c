
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
char dataMatrix[10][10];
int index=0;
int index2=0;
/*code fails to compile if this section is removed*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  printf("broadcast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*upon revieving a unicast, this section of code executes*/
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  printf("unicast message received from %d.%d: '%s'\n",
	 from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
	//datamatrix stores a maximum of 10 cycles of sensor averages
	dataMatrix[index][(from->u8[0])-2] = atoi(packetbuf_dataptr());
	index2++;
	if(index2==10){
		index++;
		index2=0;
		if(index==10)index=0;
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
	
  while(1) {
    	/* Delay 10 seconds to allow sensors time to send data */
    	etimer_set(&et, CLOCK_SECOND * 10);
    	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	
   	packetbuf_copyfrom("H0", 3);
    	broadcast_send(&broadcast);
    	printf("Tree build sent\n");
	
	//print data matrix
	int i=0, j=0;
	while(i<10){
		while(j<10){
			printf("%d  ", dataMatrix[i][j]);	
			j++;			
			}
		printf("\n");
		i++;
		j=0;
	}	
	
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/