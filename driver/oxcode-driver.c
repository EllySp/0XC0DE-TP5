#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "oxcode"
#define CLASS_NAME "oxcode_class"
#define GPIO_0 17
#define GPIO_1 27

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
static char curr_signal = '0';

static int my_open(struct inode *i, struct file *f)
{
    printk(KERN_INFO "0xC0DE: open()\n");
    return 0;
}

static int my_close(struct inode *i, struct file *f)
{
    printk(KERN_INFO "0xC0DE: close()\n");
    return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    char kbuf[1];

    int value;

    if (curr_signal == '0') {
        value = gpio_get_value(GPIO_0);
    } else if (curr_signal == '1') {
        value = gpio_get_value(GPIO_1);
    } else {
        return -EINVAL;
    }

    kbuf[0] = value ? '1' : '0';

    if (copy_to_user(buf, kbuf, 1))
        return -EINVAL;

    return 1;
}

static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    char kbuf;

    if (len != 1)
        return -EINVAL;

    if (copy_from_user(&kbuf, buf, 1))
        return -EFAULT;

    if (kbuf != '0' && kbuf != '1')
    {
        return -EINVAL;
    }
    curr_signal = kbuf;
    return len;
}

static struct file_operations dev_fops =
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_close,
    .read = my_read,
    .write = my_write
};

static int __init oxcode_init(void)
{
    int ret;
    dev_t dev_no;
    int major;
    struct device *dev_ret;

    if ((ret = alloc_chrdev_region(&dev_no, 0, 1, DEVICE_NAME)) < 0)
    {
        pr_info("0xC0DE: Unable to allocate Mayor number \n");
        return ret;
    }
    major = MAJOR(dev_no);
    dev = MKDEV(major, 0);
    
    pr_info("0xC0DE: device allocated with Mayor number %d\n", major);

    cdev_init(&c_dev, &dev_fops);
    if ((ret = cdev_add(&c_dev, dev, 1)) < 0)
    {
        unregister_chrdev_region(dev, 1);
        pr_info("0xC0DE: Unable to add character device \n");
        return ret;
    }

    if (IS_ERR(cl = class_create(THIS_MODULE, CLASS_NAME)))
    {
        unregister_chrdev_region(dev, 1);
        cdev_del(&c_dev);
        pr_info("0xC0DE: Unable register device class \n");
        return PTR_ERR(cl);
    }

    if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, DEVICE_NAME)))
    {
        class_destroy(cl);
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, 1);
        pr_info("0xC0DE: Unable create device \n");
        return PTR_ERR(dev_ret);
    }
    pr_info("0xC0DE: Device created correclty \n");

    if (gpio_request(GPIO_0, "gpio_0") || gpio_direction_input(GPIO_0)) {
        pr_info("0xC0DE: Error al configurar GPIO_0\n");
        return -1;
    }

    if (gpio_request(GPIO_1, "gpio_1") || gpio_direction_input(GPIO_1)) {
        gpio_free(GPIO_0);
        pr_info("0xC0DE: Error al configurar GPIO_1\n");
        return -1;
    }
    
    return 0;
}

static void __exit oxcode_exit(void)
{
    gpio_free(GPIO_0);
    gpio_free(GPIO_1);
    device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, 1);
}

module_init(oxcode_init);
module_exit(oxcode_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Grupo 0XC0DE - Sistemas de computación");
MODULE_DESCRIPTION("Test");


