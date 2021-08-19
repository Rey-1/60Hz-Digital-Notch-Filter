#include "msp.h"

// equation constants to use in  TA1_0_IRQHandler equation
#define B2 1.0
#define B1 -1.965
#define B0 0.9999
#define A2 1.0
#define A1 -1.955
#define A0 0.99

void clock(void);
void configure_NVIC(void);
void configure_Timer_A1(void);
void configure_ADC(void);
void configure_DAC(void);
uint16_t read_ADC(void);
void TA1_0_IRQHandler(void);
void DAC_output(uint16_t convert);

float y0 = 0;// float values for equation in  TA1_0_IRQHandler
float y1 = 0;
float y2 = 0;
float u0 = 0;
float u1 = 0;
float u2 = 0;

void main(void)
{
WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

clock();
configure_ADC();
configure_DAC();
configure_NVIC();
configure_Timer_A1();

while(1){}// empty infinite while loop

}// main loop

void clock(void)// clock function definition
{
// power control
while(PCM-> CTL1 & 0x0000010);
uint32_t key_bits  =  0x695A0000;
uint32_t AM_LDO_VCORE1_bits = 0x00000001;
PCM -> CTL0 = key_bits | AM_LDO_VCORE1_bits;
while(PCM->CTL1 & 0x00000100);
PCM->CTL0 &= 0x0000FFFF;

// Set flash read wait state number
FLCTL -> BANK0_RDCTL &= ~ (BIT(12) | BIT(13) | BIT(14) | BIT(15));
FLCTL -> BANK0_RDCTL |=    BIT(12);
FLCTL -> BANK1_RDCTL &= ~ (BIT(12) | BIT(13) | BIT(14) | BIT(15));
FLCTL -> BANK1_RDCTL |=    BIT(12);

// Enable 48 MHz DCO
CS->KEY = CS_KEY_VAL;   //unlocks CS registers, CSKEY

CS -> CTL0  |= BIT(23);//enables DCO oscillator, DCOENC->DCO ON 1b

CS -> CTL0  &= ~(BIT(18) | BIT(17) | BIT(16));// DCO frequency set to 48 MHz
CS -> CTL0  |= BIT(18) | BIT(16);//DCORSEL->101b

CS -> CTL1  &= ~(BIT2 | BIT1 | BIT0);//Selects MCLK source to DCOCLK
CS -> CTL1  |= BIT1 | BIT0;// SELM -> 011b

CS -> CLKEN |= BIT1;//enables MCLK system clock, MCLK_EN->1b


CS->CTL1 &= ~(BIT(30)|BIT(29)|BIT(28));//SMCLK clock divider to make frequency 750KHz
CS->CTL1 |= BIT(30)|BIT(29);//DIVS -> 110b

// Lock clock system
CS->KEY =0x0;
}

void configure_NVIC(void)// function to configure NVIC registers
{
NVIC -> ISER[0] = 1 << ((TA1_0_IRQn) & 31);
NVIC -> IP[10]  = BIT5;// sets top priority
}

void configure_Timer_A1(void)// function to configure TIMER_A1
{
TIMER_A1 -> CTL     &= ~(BIT(9) | BIT(8));// Timer_A clock source select
TIMER_A1 -> CTL     |=   BIT(9);// TASSEL -> 10b

TIMER_A1 -> CTL     &= ~(BIT(7) | BIT(6));// input clock divider by 1

TIMER_A1 -> CTL     &= ~(BIT(5) | BIT(4));// Mode COntrol MC set to up mode
TIMER_A1 -> CTL     |=   BIT(4);// MC -> 01b

TIMER_A1 -> CCTL[0] &= ~ BIT0;// clear interrupt flag, CCIFG->0b

TIMER_A1 -> CCTL[0] |=   BIT(4);//capture/compare interrupt enabled,CCIE -> 1b

TIMER_A1 -> CCR[0]   =   375;// CCR set to 375
}

void configure_ADC(void)// function to configure ADC
{
// ADC Registers unlocked ADC turned off
ADC14 -> CTL0 &= ~BIT1;

// Pin Select
P6 -> DIR  &= ~BIT0;// input
P6 -> SEL1 |= BIT1;// configures A15
P6 -> SEL0 |= BIT1;// configures A15

//ADC14PDIV -> 011b clock predivide = 4
ADC14 -> CTL0 &= ~(BIT(31)|BIT(30));
ADC14 -> CTL0 |=  BIT(30);

//ADC14SHx -> ADC14SC
ADC14 -> CTL0 &= ~(BIT(29)|BIT(28)|BIT(27));

//ADC14SHP -> SAMPCON set to sampling timer
ADC14 -> CTL0 |= (BIT(26));

//ADC14PDIVx -> 011b clock divider = 8
ADC14 -> CTL0 |=  (BIT(24)|BIT(23)|BIT(22));

//ADC14SSELx -> 011b clock source select = MCLK
ADC14 -> CTL0 &= ~(BIT(21)|BIT(20)|BIT(19));
ADC14 -> CTL0 |=  (BIT(20)|BIT(19));


//ADC14SHT0x -> 0011b sample and hold time  = 32
ADC14 -> CTL0 &= ~(BIT(11)|BIT(10)|BIT(9)|BIT(8));

ADC14 -> CTL0 |=  (BIT(9)|BIT(8));

ADC14 -> CTL0 |= BIT4;// ADC14 ON

//ADC14INCHx, A15 setting
ADC14 -> MCTL[0] &= ~(BIT4|BIT3|BIT2|BIT1|BIT0);
ADC14 -> MCTL[0] |=  (BIT3);
ADC14 -> MCTL[0] |=  (BIT2);
ADC14 -> MCTL[0] |=  (BIT1);
ADC14 -> MCTL[0] |=  (BIT0);

// single ended conversion start address MEM0
ADC14 -> CTL1 &= ~(BIT(20));
ADC14 -> CTL1 &= ~(BIT(19));
ADC14 -> CTL1 &= ~(BIT(18));
ADC14 -> CTL1 &= ~(BIT(17));
ADC14 -> CTL1 &= ~(BIT(16));

// 14 BIT conversion
ADC14 -> CTL1 |= BIT(5) | BIT(4);

//unsigned result
//ADC14 -> CTL1 &= ~BIT3;

// ENABLE CONVERSION
ADC14 -> CTL0 |= BIT(1);
}

void configure_DAC(void)// function to configure DAC
{
P10 ->  DIR  |= BIT3|BIT2|BIT1|BIT0;
P7  ->  DIR  |= BIT7|BIT6|BIT5|BIT4|BIT3|BIT2|BIT1|BIT0;
P10 ->  DIR  |=  BIT4 | BIT5;  // CSA and WR
P10 ->  OUT  &= ~BIT4;        // CSA low, choose CSA
P6  ->  OUT  |=   BIT3;      // CSB high, OFF
}

uint16_t read_ADC(void)// function that reads ADC
{
    // Start ADC14 conversion
ADC14-> CTL0 |= BIT0;// ADC14SC -> start sample and conversion
ADC14-> CTL0 |= BIT1;// ADC14ENC -> ADC14 enabled
while(ADC14->CTL0 & ADC14_CTL0_BUSY)// ADC busy
return ADC14->MEM[0];
}

void TA1_0_IRQHandler(void)// TIMER_A1 interrupt routine
{
u0 = read_ADC();// reads ADC
u0 = u0 * 4096 / 16384;// Divides ADC value

//y = -a1*y1-a0*y2+b2*u+b1*u1+b0*u2
// a0 =  0.995
// a1 = -1.96
// b0 =  1
// b1 = -1.965
// b2 =  1

y0 = (-A1*y1) - (A0*y2) + (B2*u0) + (B1*u1) + (B0*u2);// Equation notches 60Hz
                                                      

DAC_output((unsigned int)y0);// output goes to DAC so it can be picked up on the ADALM2000
y2 = y1;
y1 = y0;
u2 = u1;
u1 = u0;
TIMER_A1 -> CCTL[0] &= ~BIT0;// clear flags
}

void DAC_output(uint16_t convert)
{
P10 -> OUT  |= BIT5;   // write disable, high
P7  -> OUT  = (convert << 8) >> 8;   // DAC output for pin 7.0-7.7
P10 -> OUT  =  convert >> 8; // DAC output for pin 10.0-10.3
P10 -> OUT  &= ~BIT5;   // write enabled, low
}
