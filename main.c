// Runs on LM4F120/TM4C123
// Index implementation of a Moore finite state machine to operate a traffic light.  
//Ajay Katoch
// east/west red light connected to PB5
// east/west yellow light connected to PB4
// east/west green light connected to PB3
// north/south facing red light connected to PB2
// north/south facing yellow light connected to PB1
// north/south facing green light connected to PB0
// pedestrian detector connected to PE2 (1=pedestrian present)
// north/south car detector connected to PE1 (1=car present)
// east/west car detector connected to PE0 (1=car present)
// "walk" light connected to PF3 (built-in green LED)
// "don't walk" light connected to PF1 (built-in red LED)

#include "tm4c123gh6pm.h"
#define SENSOR  (*((volatile unsigned long *)0x4002400C)) //traffic buttons
#define LIGHT   (*((volatile unsigned long *)0x400050FC))
#define PED_LIGHT (*((volatile unsigned long *)0x40025028))

unsigned long S; //index of current state in FSM array
unsigned long Input;

//FSM data structure	
typedef struct Stype{
	// struct of a single state in the FSM
	unsigned long TrafficOut; // output for traffic lights (portB)
	unsigned long WalkOut; // output for pedestrian lights (portF)
	unsigned long Time; // delay time 
	unsigned long Next[8]; // next state
} SType;


//fsm of states
SType FSM[11]={
		{0x0C,0x02,20,{0,0,1,1,1,1,1,1}},
		{0x14,0x02,30,{1,0,2,2,4,4,2,2}},
		{0x21,0x02,20,{2,3,2,3,3,3,3,3}},
		{0x22,0x02,30,{3,0,2,0,4,0,4,4}},
		{0x24,0x08,20,{4,5,5,5,4,5,5,5}},
		{0x24,0x00,5,{4,6,6,6,4,6,6,6}},
		{0x24,0x02,5,{4,7,7,7,4,7,7,7}},
		{0x24,0x00,5,{4,8,8,8,4,8,8,8}},
		{0x24,0x02,5,{4,9,9,9,4,9,9,9}},
		{0x24,0x00,5,{4,10,10,10,4,10,10,10}},
		{0x24,0x02,5,{5,0,2,0,4,0,2,0}}
	};

void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;               // disable SysTick during setup
  NVIC_ST_CTRL_R = 0x00000005;      // enable SysTick with core clock
}

// The delay parameter is in units of the 80 MHz core clock. (12.5 ns)
void SysTick_Wait(unsigned long delay){
  NVIC_ST_RELOAD_R = delay-1;  // number of counts to wait
  NVIC_ST_CURRENT_R = 0;       // any value written to CURRENT clears
  while((NVIC_ST_CTRL_R&0x00010000)==0){ // wait for count flag
  }
}
// 800000*12.5ns equals 10ms, counts in delay * 10ms
void SysTick_Wait10ms(unsigned long delay){
  unsigned long i;
  for(i=0; i<delay; i++){
    SysTick_Wait(800000);  // wait 10ms
  }
}

void portBEFInit(void){
	volatile unsigned long delay;
	
	SYSCTL_RCGC2_R |= 0x32; //clock gate register enable for ports B,F and E
	delay = SYSCTL_RCGC2_R; //3-5 ms delay
	
	//Port E setup (input port: 2 car sensors and 1 walk sensor )
	GPIO_PORTE_AMSEL_R &=~0x07; //Disable analog function on pins PE2-0
	GPIO_PORTE_PCTL_R &= ~0x000000FF; //Enable Regular GPIO
        GPIO_PORTE_DIR_R &= ~0x07; //Configure pins PE2-0 to inputs   
        GPIO_PORTE_AFSEL_R &= ~0x07; //Regular functions for PE2-0
        GPIO_PORTE_DEN_R |= 0x07;  //Enable digital on PE2-0

	//PortF setup (output port: walk signal LED)
	GPIO_PORTF_AMSEL_R &=~0x0A; //Disable analog function on pins PF3 and PF1
	GPIO_PORTF_PCTL_R &= ~0x000000FF; //Enable Regular GPIO
        GPIO_PORTF_DIR_R |= 0x0A; //Configure pins PF3 and PF1 to outputs    
        GPIO_PORTF_AFSEL_R &= ~0x0A; //Regular functions for PF3 and PF1
        GPIO_PORTF_DEN_R |= 0x0A;  //Enable digital on PF3 and PF1
	
	//Port B setup (output port: traffic LED lights for south and west)
        GPIO_PORTB_AMSEL_R &= ~0x3F; //disable analog PB5-0
        GPIO_PORTB_PCTL_R &= ~0x00FFFFFF; //Enable regular GPIO
        GPIO_PORTB_DIR_R |= 0x3F;  //configure pins PB5-0 as outputs  
        GPIO_PORTB_AFSEL_R &= ~0x3F; //regular function on PB5-x
        GPIO_PORTB_DEN_R |= 0x3F; //enable digital on PB5-0
        S = 0;  //default start state
	
}

int main(void){ 
  
  SysTick_Init();
  portBEFInit();
	
  while(1){
    GPIO_PORTB_DATA_R = FSM[S].TrafficOut;  // set lights
    GPIO_PORTF_DATA_R = FSM[S].WalkOut;
    SysTick_Wait10ms(FSM[S].Time);
    S = FSM[S].Next[SENSOR]; //next state based on input
  }
}

