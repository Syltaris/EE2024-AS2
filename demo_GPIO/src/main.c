/*****************************************************************************
 *   GPIO Speaker & Tone Example
 *
 *   CK Tham, EE2024
 *
 ******************************************************************************/
#include "lpc17xx_ssp.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"

#include "rgb.h"
#include "led7seg.h"
#include "acc.h"
#include "oled.h"

static unsigned int count = 0;
static int monitor_symbols[]  = {'0', '1', '2', '3', '4', '5', '6' ,'7' ,'8' ,'9' , 'A', 'B', 'C', 'D', 'E', 'F'};
static int8_t x = 0;
static int8_t y = 0;
static int8_t z = 0;
static int8_t xOff = 0;
static int8_t yOff = 0;
static int8_t zOff = 0;

typedef enum { false, true } bool;
volatile bool sampleSensors = false;

void calibrateAccel();
void init_oled();

static void init_ssp(void)
{
	SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;

	/*
	 * Initialize SPI pin connect
	 * P0.7 - SCK;
	 * P0.8 - MISO
	 * P0.9 - MOSI
	 * P2.2 - SSEL - used as GPIO
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 8;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 9;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);

	SSP_ConfigStructInit(&SSP_ConfigStruct);

	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);

	// Enable SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);

}

static void init_i2c(void)
{
	PINSEL_CFG_Type PinCfg;

	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);

	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000);

	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE);
}

static void start_7seg(){

	// default value of PCLK = CCLK/4
	// CCLK is derived from PLL0 output signal / (CCLKSEL + 1) [CCLKCFG register]

    LPC_SC->PCONP |= (1<<2);					/* Power ON Timer1 */

    LPC_TIM1->MCR  = (1<<0) | (1<<1);     		/* Clear COUNT on MR0 match and Generate Interrupt */
    LPC_TIM1->PR   = 0;      					/* Update COUNT every (value + 1) of PCLK  */
    LPC_TIM1->MR0  = 40000000;                 /* Value of COUNT that triggers interrupts */
    LPC_TIM1->TCR  = (1 <<0);                   /* Start timer by setting the Counter Enable */

    NVIC_EnableIRQ(TIMER1_IRQn);				/* Enable Timer1 interrupt */

}

int main (void) {

	init_ssp();
	init_i2c();

	led7seg_init();
	acc_init();
	oled_init();

	start_7seg();

	// Enable GPIO Interrupt P2.10
	LPC_GPIOINT->IO2IntEnF |= 1<<10;

	// Enable EINT3 interrupt
	NVIC_EnableIRQ(EINT3_IRQn);

	calibrateAccel();

	acc_read(&x, &y, &z);
	int initX = x;
	int initY = y;
//	int initZ = z;

	int posX = 48;
	int posY = 32;

	init_oled();

	while (1){
		acc_read(&x, &y, &z);

		if (x > initX + 5)
			posX--;
		if (x < initX - 5)
			posX++;
		if (y > initY + 5)
			posY++;
		if (y < initY - 5)
			posY--;

		posX = posX % 96;
		posY = posY % 64;
		if (posX < 0)
			posX = 96;
		if (posY < 0)
			posY = 64;

		oled_putPixel(posX,posY,OLED_COLOR_WHITE);
	}

}


void TIMER1_IRQHandler(void)
{
	unsigned int isrMask;

    isrMask = LPC_TIM1->IR;
    LPC_TIM1->IR = isrMask;         /* Clear the Interrupt Bit */

    led7seg_setChar(monitor_symbols[count], 0);

    if (count == 5 || count == 10 || count == 15)
    	sampleSensors = true;

    count = (count + 1) % 16;

}

void EINT3_IRQHandler(void)
{
	// Determine whether GPIO Interrupt P2.10 has occurred
	if ((LPC_GPIOINT->IO2IntStatF>>10)& 0x1)
	{
		oled_clearScreen(OLED_COLOR_BLACK);

        // Clear GPIO Interrupt P2.10
        LPC_GPIOINT->IO2IntClr = 1<<10;
	}
}

void calibrateAccel(){
	acc_read(&x, &y, &z);
	xOff = 0-x;
	yOff = 0-y;
	zOff = 64-z;
}

void init_oled(){
	oled_clearScreen(OLED_COLOR_BLACK);
	oled_putString(0,20,"Tempe: ",OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0,30,"Light: ",OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0,40,"Accel: ",OLED_COLOR_WHITE, OLED_COLOR_BLACK);
}
