#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>

#define LED_DEV_NAME "leddevdev"
#define LED_DEV_MAJOR 240

static int leddev_open(struct inode *inode,struct file *filp)
{
	int num = MINOR(inode->i_rdev);
	printk("leddev open-> minor : %d\n", num);
	num = MAJOR(inode->i_rdev);
	printk("leddev open-> major : %d\n", num);
	return 0;
}

static loff_t leddev_llseek(struct file *filp, loff_t off, int whence)
{
	printk("leddev llseek -> off : %08X, whence : %08X\n",(unsigned int )off,whence);
	return 0x23;
}

static ssize_t leddev_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	printk("leddev read -> buf : %08X, count : %08X \n",(unsigned int )buf,count);
	return 0x33;
}

static ssize_t leddev_write (struct file *filp, const char *buf, size_t count, loff_t *f_ps)
{
	printk("leddev write -> buf : %08X, count : %08X \n", (unsigned int)buf,count);
return  0x43;
}

static long leddev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk("leddev ioctl -> cmd : %08X, arg :%08X\n",cmd, (unsigned int)arg);
	return 0x53;
}

static int leddev_release(struct inode *inode, struct file *filp)
{
	printk("leddev release \n");
	return 0;
}

struct file_operations leddev_fops =
{
		.owner = THIS_MODULE,
		.open = leddev_open,
		.read = leddev_read,
		.write = leddev_write,
		.llseek = leddev_llseek,
		.unlocked_ioctl = leddev_ioctl,
		.release = leddev_release,
};

static int leddev_init(void)
{
	int result;
	printk("leddev leddev_init \n");
	result = register_chrdev(LED_DEV_MAJOR,LED_DEV_NAME, &leddev_fops);
	if(result < 0) return result;
	return 0;
}

static void leddev_exit(void)
{
	printk("leddev leddev_exit \n");
	unregister_chrdev(LED_DEV_MAJOR, LED_DEV_NAME);
}
module_init(leddev_init);
module_exit(leddev_exit);
MODULE_LICENSE("Dual BSD/GPL");
