
#include "contiki.h"
#include "net/rime/rime.h"
#include "platform/sky/dev/light-sensor.h"
#include "dev/button-sensor.h"
#include "core/lib/sensors.h"
#include "dev/leds.h"
#include "stdio.h"
/*---------------------------------------------------------------------------*/
PROCESS(example_unicast_process, "Example unicast");
AUTOSTART_PROCESSES(&example_unicast_process);
/*---------------------------------------------------------------------------*/
static struct unicast_conn uc;
char hops = 126;
char sequence=0;
linkaddr_t returnAddr;
//preinitialise to prevent dividing by 0
int values[10]={32,45,168,221,50,45,87,96,103,130};
int index=0;
static struct broadcast_conn broadcast;
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  printf("broadcast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
	char* message = (char *)packetbuf_dataptr();

	if(message[0]=='W' && hops != 126){
		hops = 126;
		broadcast_send(&broadcast);
	}

	if(message[0]=='H'){
		if((message[1]-'0')<hops){
			hops = (message[1] - '0') + 1;
			returnAddr.u8[0] = from->u8[0];
	    		returnAddr.u8[1] = from->u8[1];
			char output[3];
			output[0]='H';
			output[1]='0'+hops;
			output[2]='\0';
			packetbuf_copyfrom(output, 3);
			
	     		broadcast_send(&broadcast);
	    		
			printf("my hop is %d\n", hops);
		}
	}
	
	if(message[0]=='D'){
		if((message[1]-'0')>sequence || (sequence==9 && message[1]=='0')){
	sequence = message[1] - '0';
	if(sequence==9 && message[1]=='0'){sequence = 0;}
	//printf("sequence: %d\n", sequence);
	broadcast_send(&broadcast);
	int val=0;
	int i=0;
	while(i<10){
		val += values[i];
		
		i++;		
	}
 	val = val/10;
	//printf("Light=%dlux \n", val); 
	char output[4];
	
	sprintf(output, "%d", val);
	
    	packetbuf_copyfrom(output, 4);


	if(!linkaddr_cmp(&returnAddr, &linkaddr_node_addr)) {
     		unicast_send(&uc, &returnAddr);
		printf("data reply sent\n");
    	}
	}
	printf("sequence: %d\n", sequence);
}	
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};

static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  printf("unicast message received from %d.%d: '%s'\n",
	 from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
	char* output;
	
	output = packetbuf_dataptr();
	
    	packetbuf_copyfrom(output, 4);


	if(!linkaddr_cmp(&returnAddr, &linkaddr_node_addr)) {
     		unicast_send(&uc, &returnAddr);
		printf("data reply sent\n");
    	}
}
static const struct unicast_callbacks unicast_callbacks = {recv_uc};

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_unicast_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)
    
  PROCESS_BEGIN();
  broadcast_open(&broadcast, 129, &broadcast_call);	//activate sensors
  SENSORS_ACTIVATE(light_sensor);
  unicast_open(&uc, 146, &unicast_callbacks);
  while(1) {
    	static struct etimer et;
    	linkaddr_t addr;
    
    	etimer_set(&et, CLOCK_SECOND);
    	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));


	values[index] = light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR);
	index++;
	
	if(index==10){index=0;}
	
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/