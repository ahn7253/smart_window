#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<fcntl.h>
#include<unistd.h>
#include<linux/kdev_t.h>
#include<wiringPi.h>
#include<wiringPiI2C.h>
#include<wiringPiSPI.h>
#include<lcd.h>

#define DEV_PATH "/dev/smartWindow_dev"
#define CS_MCP3208 (6)
#define CS_PCF8591 (0X48)
#define ADC_CH0 (0)
#define ADC_CH1 (1)
#define SPI_CHANNEL (1)
#define SPI_SPEED (1000000) //1MHz

#define MOTOR180 (180)
#define MOTOR360 (360)
#define CW (2)
#define CCW (3)

#define LCD_RS (11)
#define LCD_E (10)
#define LCD_D4 (4)
#define LCD_D5 (5)
#define LCD_D6 (7)
#define LCD_D7 (0)

#define GAS_WARNING (1000)
#define WATER_WARNING (100)
#define LIGHT_WARNING (3000)

int waterSensorFD;
int ledFD;
int lcd;
int read_mcp3208_adc(unsigned char adcChannel){
	unsigned char buff[3];
	int adcValue = 0;

	buff[0] = 0x06 | ((adcChannel & 0x07) >> 2);
	buff[1] = ((adcChannel & 0x07) << 6);
	buff[2] = 0x00;

	digitalWrite(CS_MCP3208, 0);
	wiringPiSPIDataRW(SPI_CHANNEL, buff, 3);

	buff[1] = 0x0f & buff[1];
	adcValue = (buff[1] << 8) | buff[2];

	digitalWrite(CS_MCP3208, 1);

	return adcValue;
}
void initSmartWindow(void){
	wiringPiSetup();
	//init spi
	wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED);
	pinMode(CS_MCP3208, OUTPUT);
	//init i2c
	waterSensorFD = wiringPiI2CSetup(CS_PCF8591);
	//init led fd
	if((ledFD = open(DEV_PATH, O_RDWR|O_NONBLOCK))<0){
		perror("open()");
		exit(1);
	}
	//init lcd
	lcd = lcdInit(2, 16, 4, LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7, 0, 0, 0, 0);
	lcdPosition(lcd, 0, 0);
}
void exitSmartWindow(void){
	close(ledFD);	
}
int readWaterLevel(void){
	int value;
	wiringPiI2CWrite(waterSensorFD,0x00);
	value = wiringPiI2CRead(waterSensorFD);
	return value;
}
void runMotor(unsigned int kind, int rotate){
	//kind : MOTOR180, MOTOR360
	//rotate : MOTOR180(0degree,180degree), MOTOR360(CW,CCW)
	const char *direction;
	switch(rotate){
		case 0: direction = "0"; break;
		case 180: direction = "180"; break;
		case CW: direction = "CW"; break;
		case CCW: direction = "CCW"; break;
	}
	write(ledFD,direction,kind);
}
void setWindow(int gasData, int waterData, int *windowIsOpen){
	if(gasData < GAS_WARNING && waterData < WATER_WARNING && !(*windowIsOpen)){
		runMotor(MOTOR180,180);
		*windowIsOpen = 1;
	}
	else if((gasData >= GAS_WARNING || waterData >= WATER_WARNING) && *windowIsOpen) {
		runMotor(MOTOR180,0);
		*windowIsOpen = 0;
	}
}
void brightLED(char* led, int value){
	read(ledFD,led,value);
}
void setLED(int gasData, int waterData, int lightData){
	if(gasData < GAS_WARNING) brightLED("YELLOW",0);
	else brightLED("YELLOW",1);
	if(waterData < WATER_WARNING) brightLED("GREEN",0);
	else brightLED("GREEN",1);
	if(lightData < LIGHT_WARNING) brightLED("RED",0);
	else brightLED("RED",1);
}
void rollUp(void){
	runMotor(MOTOR360,CCW);
	runMotor(MOTOR360,CCW);
}
void rollDown(void){
	runMotor(MOTOR360,CW);
	runMotor(MOTOR360,CW);
}
void setCurtain(int lightData, int *curtainIsUp){
	if(lightData > LIGHT_WARNING && *curtainIsUp) {
		rollDown(); *curtainIsUp = 0;
	}
	else if(lightData <= LIGHT_WARNING && !(*curtainIsUp)){
		rollUp(); *curtainIsUp = 1;
	}
	else;
}
void putLcd(int gasData, int waterData, int lightData){
	char firstRow[20],secondRow[20];
	//set string
	sprintf(firstRow,"G:%d W:%d",gasData,waterData);
	sprintf(secondRow,"L:%d",lightData);
	//set lcd
	lcdClear(lcd);
	lcdPuts(lcd,firstRow);
	lcdPosition(lcd,0,1);
	lcdPuts(lcd,secondRow);
}
void readSensorData(int *gasData, int *waterData, int *lightData){
	*gasData = read_mcp3208_adc(ADC_CH0);
	*lightData = read_mcp3208_adc(ADC_CH1);
	*waterData = readWaterLevel();
}
int main(int argc, char *argv[]){	
	int gasData, waterData, lightData;
	int curtainIsUp = 1, windowIsOpen = 1;
	printf("Smart Window is on\n");
	initSmartWindow();
	while(1){
		//read sensor data
		readSensorData(&gasData, &waterData, &lightData);
		//print all data to terminal
		printf("gas:%d,water:%d,light:%d\n",gasData ,waterData, lightData);
		//set led
		setLED(gasData, waterData, lightData);
		//set lcd
		putLcd(gasData, waterData, lightData);
		//set window
		setWindow(gasData, waterData, &windowIsOpen);
		//set curtain
		setCurtain(lightData, &curtainIsUp);
		sleep(1);
	}
	exitSmartWindow();
	printf("Smart Window is off\n");
	return 0;
}
