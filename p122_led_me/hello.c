#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>

static int ledset = 15;
module_param(ledset, int, 0);

#define DEBUG 1
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))

int led[] = {
        IMX_GPIO_NR(1, 16),   //16
        IMX_GPIO_NR(1, 17),       //17
        IMX_GPIO_NR(1, 18),   //18
        IMX_GPIO_NR(1, 19),   //19
};
static int led_request(void)
{
	    int ret = 0;
		int i=0;

        for (i = 0; i < ARRAY_SIZE(led); i++) {
                ret = gpio_request(led[i], "gpio led");
                if(ret<0){
                       printk("#### FAILED Request gpio %d. error : %d \n", led[i], ret);
					   break;
                }
        }
        return ret;
}
static void led_free(void)
{
		int i=0;
        for (i = 0; i < ARRAY_SIZE(led); i++){
                gpio_free(led[i]);
        }
}

void led_write(unsigned long data)
{
        int i=0;
        for(i = 0; i < ARRAY_SIZE(led); i++){
                gpio_direction_output(led[i], (data >> i ) & 0x01);
                gpio_set_value(led[i], (data >> i ) & 0x01);
        }
#if DEBUG
        printk("#### %s, data = %ld\n", __FUNCTION__, data);
#endif
}
void led_read(unsigned long * led_data)
{
        int i=0;
        unsigned long data=0;
        unsigned long temp;
        for(i=0;i<4;i++)
        {
                gpio_direction_input(led[i]); //error led all turn off
                temp = gpio_get_value(led[i]) << i;
                data |= temp;
        }
#if DEBUG
        printk("#### %s, data = %ld\n", __FUNCTION__, data);
#endif
        *led_data = data;
        led_write(data);
        return;
}

static int hello_init(void)
{
	int ret=0;
	    printk("Hello, world \n");
		ret = led_request();
        if(ret<0)
		{
			return ret;	//gpio_request error
		}
        led_write(ledset);
		return 0;
}

static void hello_exit(void)
{
	    printk("Goodbye, world\n");
        led_write(0);
        led_free();
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("Dual BSD/GPL");


