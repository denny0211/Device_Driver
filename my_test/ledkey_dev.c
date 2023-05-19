#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>

#define TIME_STEP	timeval  //KERNEL HZ=100
#define   LED_DEV_NAME            "ledkeydev"
#define   LED_DEV_MAJOR            240      
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))

static unsigned long ledvalue = 15;
static char * twostring = NULL;
static int timeval = 100;	//f=100HZ, T=1/100 = 10ms, 100*10ms = 1Sec

module_param(ledvalue, ulong ,0);
module_param(twostring,charp,0);
module_param(timeval, int, 0);

typedef struct
{
	struct timer_list timer;
	unsigned long 	  led;
} __attribute__((packed)) KERNEL_TIMER_MANAGER;

static KERNEL_TIMER_MANAGER* ptrmng = NULL;
void kerneltimer_timeover(unsigned long arg);
void key_read(unsigned char* key_data);
void led_write(unsigned char data);

int led[4] = {
	IMX_GPIO_NR(1, 16),   //16
	IMX_GPIO_NR(1, 17),	  //17
	IMX_GPIO_NR(1, 18),   //18
	IMX_GPIO_NR(1, 19),   //19
};
int key[8] = {
	IMX_GPIO_NR(1, 20),   //20
	IMX_GPIO_NR(1, 21),	  //21
	IMX_GPIO_NR(4, 8),    //104
	IMX_GPIO_NR(4, 9),    //105
	IMX_GPIO_NR(4, 5),    //101
	IMX_GPIO_NR(7, 13),	  //205
	IMX_GPIO_NR(1, 7),    //7
	IMX_GPIO_NR(1, 8),    //8
};

void kerneltimer_registertimer(KERNEL_TIMER_MANAGER* pdata, unsigned long timeover)
{
	init_timer(&(pdata->timer));
	pdata->timer.expires = get_jiffies_64() + timeover;  //10ms *100 = 1sec
	pdata->timer.data = (unsigned long)pdata;
	pdata->timer.function = kerneltimer_timeover;
	add_timer(&(pdata->timer));
}
void kerneltimer_timeover(unsigned long arg)
{
	KERNEL_TIMER_MANAGER* pdata = NULL;
	if (arg)
	{
		pdata = (KERNEL_TIMER_MANAGER*)arg;
		led_write(pdata->led & 0x0f);
		pdata->led = ~(pdata->led);

		kerneltimer_registertimer(pdata, TIME_STEP);
	}
}

void key_read(unsigned char* key_data)
{
	int i;
	unsigned long data = 0;
	unsigned long temp;
	for (i = 0; i < ARRAY_SIZE(key); i++)
	{
		temp = gpio_get_value(key[i]);
		if (temp)
		{
			data = i + 1;
			break;
		}
	}
	*key_data = data;
	return;
}
void led_write(unsigned char data)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(led); i++) {
		gpio_set_value(led[i], (data >> i) & 0x01);
	}

}

static int ledkey_request(void)
{
	int ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(led); i++) {
		ret = gpio_request(led[i], "gpio led");
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n", led[i], ret);
			break;
		} 
		gpio_direction_output(led[i], 0);
	}
	for (i = 0; i < ARRAY_SIZE(key); i++) {
		ret = gpio_request(key[i], "gpio key");
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n", key[i], ret);
			break;
		} 
		gpio_direction_input(key[i]);
	}
	return ret;
}
static void ledkey_free(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(led); i++){
		gpio_free(led[i]);
	}
	for (i = 0; i < ARRAY_SIZE(key); i++){
		gpio_free(key[i]);
	}
	if (timer_pending(&(ptrmng->timer)))
		del_timer(&(ptrmng->timer));
	if (ptrmng != NULL)
	{
		kfree(ptrmng);
	}
}
int kerneltimer_init(void)
{
	ptrmng = (KERNEL_TIMER_MANAGER*)kmalloc(sizeof(KERNEL_TIMER_MANAGER), GFP_KERNEL);
	if (ptrmng == NULL) return -ENOMEM;
	memset(ptrmng, 0, sizeof(KERNEL_TIMER_MANAGER));
	ptrmng->led = ledvalue;
	kerneltimer_registertimer(ptrmng, TIME_STEP);
	return 0;
}

int leddev_open (struct inode *inode, struct file *filp)
{
    int num0 = MAJOR(inode->i_rdev); 
    int num1 = MINOR(inode->i_rdev); 
    printk( "leddev open -> major : %d\n", num0 );
    printk( "leddev open -> minor : %d\n", num1 );

    return 0;
}

ssize_t leddev_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char kbuf;
	int ret;
	key_read(&kbuf);     
	ret=copy_to_user(buf,&kbuf,count);
	if(ret < 0)
		return -ENOMEM;
    return count;
}
ssize_t leddev_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	char kbuf;
	int ret;
	ret=copy_from_user(&kbuf,buf,count);
	if(ret < 0) return -ENOMEM;
	ptrmng->led = kbuf;
	//led_write(kbuf);
    return count;
}

struct file_operations leddev_fops =
{
    .owner    = THIS_MODULE,
    .open     = leddev_open,     
    .read     = leddev_read,     
    .write    = leddev_write,     
};

int leddev_init(void)
{
    int result;

    printk( "leddev leddev_init \n" );    

    result = register_chrdev( LED_DEV_MAJOR, LED_DEV_NAME, &leddev_fops);
    if (result < 0) return result;

	result = ledkey_request();
	if(result < 0) return result;     /* Device or resource busy */
	
	result = kerneltimer_init();
	if (result < 0) return result;

    return 0;
}
void leddev_exit(void)
{
    printk( "leddev leddev_exit \n" );    
    unregister_chrdev( LED_DEV_MAJOR, LED_DEV_NAME );
	ledkey_free();
}

module_init(leddev_init);
module_exit(leddev_exit);

MODULE_AUTHOR("YDW");
MODULE_DESCRIPTION("test module");
MODULE_LICENSE("Dual BSD/GPL");
