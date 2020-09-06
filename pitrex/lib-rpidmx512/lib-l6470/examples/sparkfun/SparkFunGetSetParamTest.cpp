/**
 * https://github.com/sparkfun/L6470-AutoDriver/blob/master/Libraries/Arduino/examples/SparkFunGetSetParamTest/SparkFunGetSetParamTest.ino
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "bcm2835.h"
#include "autodriver.h"
#include "l6470constants.h"

#define GPIO_BUSY_IN	RPI_V2_GPIO_P1_35
#define GPIO_RESET_OUT 	RPI_V2_GPIO_P1_38

void pv(float v);
void pv(bool v);
void pv(uint8_t v);
void pv(unsigned long v);
void pv(int v);

void test(float v1, float v2);
void test(int v1, int v2);
void test(bool v1, bool v2);

static char name[255];
static bool pass = true;

int main(int argc, char **argv) {
	unsigned long temp;
	bool tempBool;
	uint8_t tempByte;
	float tempFloat;
	int tempInt;
	int tempInt2;

	/***************************************************************************/
	/*                                                                         */
	if (getuid() != 0) {
		fprintf(stderr, "Error: Not started with 'root'\n");
		return -1;
	}

	if (bcm2835_init() != 1) {
		fprintf(stderr, "bcm2835_init() failed\n");
		return -2;
	}

	// Start by setting up the pins and the SPI peripheral.
	//  The library doesn't do this for you!
	bcm2835_spi_begin();

	bcm2835_gpio_fsel(GPIO_RESET_OUT, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_set(GPIO_RESET_OUT);

	// This low/high is a reset of the L6470 chip on the Autodriver board, and is a good thing to do at
	// the start of any Autodriver sketch, to be sure you're starting the Autodriver from a known state.
	bcm2835_gpio_clr(GPIO_RESET_OUT);
	bcm2835_delayMicroseconds(10000);
	bcm2835_gpio_set(GPIO_RESET_OUT);
	bcm2835_delayMicroseconds(10000);
	/*                                                                         */
	/***************************************************************************/

	AutoDriver board(0, BCM2835_SPI_CS0, GPIO_RESET_OUT);

	// first check  board config register, should be 0x2E88 on bootup
	temp = board.getParam(L6470_PARAM_CONFIG);
	printf("Config reg value: %4x\n", (int) temp);

	// Now check the status of the board. Should be 0x7c03
	temp = board.getStatus();
	printf("Status reg value: %4x\n", (int) temp);

	// set and get all configuration values here to make sure the
	//  conversions are working and comms are up properly.
	strcpy(name, "LoSpdOpt");
	tempBool = board.getLoSpdOpt();
	pv(tempBool);
	tempBool = ~tempBool;
	board.setLoSpdOpt(tempBool);
	test(tempBool, board.getLoSpdOpt());

	strcpy(name, "MinSpeed");
	tempFloat = board.getMinSpeed();
	pv(tempFloat);
	// be careful about rounding
	tempFloat = (tempFloat == 23.8418788909) ? 47.6837577818 : 23.8418788909;
	board.setMinSpeed(tempFloat);
	test(tempFloat, board.getMinSpeed());

	strcpy(name, "StepMode");
	tempByte = board.getStepMode();
	pv(tempByte);
	tempByte = (tempByte == 0) ? 1 : 0;
	board.configStepMode(tempByte);
	test(tempByte, board.getStepMode());

	strcpy(name, "MaxSpeed");
	tempFloat = board.getMaxSpeed();
	pv(tempFloat);
	// be careful about rounding
	tempFloat = (tempFloat == 152.587890625) ? 305.17578125 : 152.587890625;
	board.setMaxSpeed(tempFloat);
	test(tempFloat, board.getMaxSpeed());

	strcpy(name, "FullSpeed");
	tempFloat = board.getFullSpeed();
	pv(tempFloat);
	// be careful about rounding
	tempFloat = (tempFloat == 160.21728515625) ? 312.80517578125 : 160.21728515625;
	board.setFullSpeed(tempFloat);
	test(tempFloat, board.getFullSpeed());

	strcpy(name, "Acc");
	tempFloat = board.getAcc();
	pv(tempFloat);
	// be careful about rounding
	tempFloat = (tempFloat == 72.76008090920998) ? 145.52016181841995 : 72.76008090920998;
	board.setAcc(tempFloat);
	test(tempFloat, board.getAcc());

	strcpy(name, "Dec");
	tempFloat = board.getDec();
	pv(tempFloat);
	// be careful about rounding
	tempFloat = (tempFloat == 72.76008090920998) ? 145.52016181841995 : 72.76008090920998;
	board.setDec(tempFloat);
	test(tempFloat, board.getDec());

	strcpy(name, "OCThreshold");
	tempByte = board.getOCThreshold();
	pv(tempByte);
	tempByte = (tempByte == L6470_OCD_TH_375mA) ? L6470_OCD_TH_750mA : L6470_OCD_TH_375mA;
	board.setOCThreshold(tempByte);
	test(tempByte, board.getOCThreshold());

	strcpy(name, "PWMFreqDivisor");
	tempInt = board.getPWMFreqDivisor();
	tempInt2 = board.getPWMFreqMultiplier();
	pv(tempInt);
	tempInt = (tempInt == L6470_CONFIG_PWM_INT_DIV_1) ? L6470_CONFIG_PWM_INT_DIV_2 : L6470_CONFIG_PWM_INT_DIV_1;
	board.setPWMFreq(tempInt, tempInt2);
	test(tempInt, board.getPWMFreqDivisor());

	strcpy(name, "PWMFreqMultiplier");
	pv(tempInt2);
	tempInt2 = (tempInt2 == L6470_CONFIG_PWM_DEC_MUL_1) ? L6470_CONFIG_PWM_DEC_MUL_2 : L6470_CONFIG_PWM_DEC_MUL_1;
	board.setPWMFreq(tempInt, tempInt2);
	test(tempInt2, board.getPWMFreqMultiplier());

	strcpy(name, "SlewRate");
	tempInt = board.getSlewRate();
	pv(tempInt);
	tempInt = (tempInt == L6470_CONFIG_POW_SR_110V_us) ? L6470_CONFIG_POW_SR_260V_us : L6470_CONFIG_POW_SR_110V_us;
	board.setSlewRate(tempInt);
	test(tempInt, board.getSlewRate());

	strcpy(name, "OCShutdown");
	tempInt = board.getOCShutdown();
	pv(tempInt);
	tempInt = (tempInt == L6470_CONFIG_OC_SD_ENABLE) ? L6470_CONFIG_OC_SD_DISABLE : L6470_CONFIG_OC_SD_ENABLE;
	board.setOCShutdown(tempInt);
	test(tempInt, board.getOCShutdown());

	strcpy(name, "VoltageComp");
	tempInt = board.getVoltageComp();
	pv(tempInt);
	tempInt = (tempInt == TL6470_CONFIG_VS_COMP_ENABLE) ? TL6470_CONFIG_VS_COMP_DISABLE : TL6470_CONFIG_VS_COMP_ENABLE;
	board.setVoltageComp(tempInt);
	test(tempInt, board.getVoltageComp());

	strcpy(name, "SwitchMode");
	tempInt = board.getSwitchMode();
	pv(tempInt);
	tempInt = (tempInt == TL6470_CONFIG_SW_MODE_USER) ? TL6470_CONFIG_SW_MODE_HARD_STOP : TL6470_CONFIG_SW_MODE_USER;
	board.setSwitchMode(tempInt);
	test(tempInt, board.getSwitchMode());

	strcpy(name, "OscMode");
	tempInt = board.getOscMode();
	pv(tempInt);
	tempInt = (tempInt == L6470_CONFIG_OSC_INT_16MHZ) ? L6470_CONFIG_OSC_INT_16MHZ_OSCOUT_2MHZ : L6470_CONFIG_OSC_INT_16MHZ;
	board.setOscMode(tempInt);
	test(tempInt, board.getOscMode());

	strcpy(name, "AccK");
	tempByte = board.getAccKVAL();
	pv(tempByte);
	tempByte = (tempByte == 0) ? 1 : 0;
	board.setAccKVAL(tempByte);
	test(tempByte, board.getAccKVAL());

	strcpy(name, "DecK");
	tempByte = board.getDecKVAL();
	pv(tempByte);
	tempByte = (tempByte == 0) ? 1 : 0;
	board.setDecKVAL(tempByte);
	test(tempByte, board.getDecKVAL());

	strcpy(name, "RunK");
	tempByte = board.getRunKVAL();
	pv(tempByte);
	tempByte = (tempByte == 0) ? 1 : 0;
	board.setRunKVAL(tempByte);
	test(tempByte, board.getRunKVAL());

	strcpy(name, "HoldK");
	tempByte = board.getHoldKVAL();
	pv(tempByte);
	tempByte = (tempByte == 0) ? 1 : 0;
	board.setHoldKVAL(tempByte);
	test(tempByte, board.getHoldKVAL());

	printf("Passed? %s\n", pass ? "True" : "False");
}

void pv(float v) {
	printf("%s %f\n", name, v);
}

void pv(bool v) {
	printf("%s %s\n", name, v ? "True" : "False");
}

void pv(uint8_t v) {
	printf("%s %d\n", name, (int) v);
}

void pv(unsigned long v) {
	printf("%s %lu\n", name, v);
}

void pv(int v) {
	printf("%s %d\n", name, v);
}

void test(float v1, float v2) {
	if (abs(v1 - v2) > 0.1) {
		printf("!!! %s failed\n", name);
		printf("Expected %f Got %f\n", v1, v2);
		pass = false;
	} else {
		printf("%s passed r/w test!\n", name);
	}
}

void test(int v1, int v2) {
	if (v1 != v2) {
		printf("!!! %s failed\n", name);
		printf("Expected %d Got %d\n", v1, v2);
		pass = false;
	} else {
		printf("%s passed r/w test!\n", name);
	}
}

void test(bool v1, bool v2) {
	if (v1 != v2) {
		printf("!!! %s failed\n", name);
		printf("Expected %s Got %s\n", v1 ? "True" : "False", v2 ? "True" : "False");
		pass = false;
	} else {
		printf("%s passed r/w test!\n", name);
	}
}

