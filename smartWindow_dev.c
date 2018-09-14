#include<linux/gpio.h>
#include<linux/init.h>
#include<linux/module.h>
#include<linux/delay.h>
#include<linux/fs.h>
#include<linux/string.h>

#define MOTOR180 (18)	//Physical 12
#define MOTOR360 (13)	//Physical 33

#define DEV_NAME "smartWindow_dev"
#define DEV_NUM (240)

#define YELLOW (16)	//Physical 36
#define GREEN (20)	//Physical 38
#define RED (21)	//Physical 40

#define HIGH (1)
#define LOW (0)

MODULE_LICENSE("GPL");

int smartWindow_open(struct inode *pinode,struct file *pfile)
{
	printk(KERN_ALERT "OPEN smartWindow_dev\n");
	return 0;
}
int smartWindow_close(struct inode *pinode, struct file *pfile){
	printk(KERN_ALERT "RELEASE smartWindow_dev\n");
	return 0;
}
ssize_t runMotor(struct file *pfile, const char __user *direction, size_t kind, loff_t *offset){
	//kind - MOTOR180, MOTOR360
	//direction - MOTOR180(0 degree, 180 degree)
	//direction - MOTOR360(CCW(Counter Clock Wise),CW(Clock Wise))
	int i;
	unsigned int gpio = 0;
	unsigned long highTime = 0,lowTime = 0;	
	//set gpio
	if(kind == 180) gpio = MOTOR180;
	else if(kind == 360) gpio = MOTOR360;
	//set direction
	if(!strcmp(direction,"0"))highTime = 500,lowTime = 19500;
	else if(!strcmp(direction,"180"))highTime = 2500,lowTime = 17500;
	else if(!strcmp(direction,"CW"))highTime = 700,lowTime = 19300;
	else if(!strcmp(direction,"CCW"))highTime = 2300,lowTime = 17700;
	else;
	//run motor, 20ms*30=600ms
	for(i=0;i<30;i++){
		gpio_set_value(gpio,HIGH);
		usleep_range(highTime,highTime);
		gpio_set_value(gpio,LOW);
		usleep_range(lowTime,lowTime);
	}
	return 0;
}
ssize_t brightLED(struct file *pfile, char __user *led, size_t value, loff_t *offset){
	//on : value = HIGH, off : value = LOW
	//YELLOW : dust sensor, RED = photo sensor, GREEN = raindrop sensor
	if(!strcmp(led,"RED")) gpio_set_value(RED, value);
	else if(!strcmp(led,"YELLOW")) gpio_set_value(YELLOW, value);
	else if(!strcmp(led,"GREEN")) gpio_set_value(GREEN, value);
	else;
	return 0;
}
struct file_operations fop={
	.owner=THIS_MODULE,
	.open=smartWindow_open,
	.release=smartWindow_close,
	.write=runMotor,
	.read=brightLED,
};
int __init smartWindow_init(void){
	printk(KERN_ALERT "INIT smartWindow\n");
	//led init
	gpio_request(YELLOW,"YELLOW");
	gpio_request(GREEN,"GREEN");
	gpio_request(RED,"RED");
	gpio_direction_output(YELLOW,LOW);
	gpio_direction_output(GREEN,LOW);
	gpio_direction_output(RED,LOW);

	register_chrdev(DEV_NUM, DEV_NAME, &fop);
	return 0;
}
void __exit smartWindow_exit(void){
	printk(KERN_ALERT "EXIT smartWindow_dev\n");
	unregister_chrdev(DEV_NUM,DEV_NAME);
}
module_init(smartWindow_init);
module_exit(smartWindow_exit);
