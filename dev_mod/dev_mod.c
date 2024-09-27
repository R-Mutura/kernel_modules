#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>



MODULE_LICENSE("GPL");
#define Major_num 64
#define dev_name "mydevice_driver"
//function to open device file
static int open_devfile(struct inode *inodep, struct file *filep){
    printk(KERN_INFO "dEVICE OPENED successfully");
    return 0;
}
//function to close device file
static int close_devfile(struct inode *inodep, struct file *filep){
    printk(KERN_INFO "dEVICE CLOSED successfully");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open_devfile,
    .release = close_devfile,
};
static int __init dev_Init(void){
    int retval;
    printk(KERN_ALERT "Device file driver Module initialized\n");
    //register the char device
    //to see all used device numbers go to /proc/devices
     retval = register_chrdev(Major_num, dev_name, &fops);
    if (retval == 0){
         printk(KERN_ALERT "Device registered successfully Major: %d : Minor %d\n",Major_num, 0 );
    }
    else if(retval > 0){
        printk(KERN_ALERT "Device registered successfully Major: %d : Minor %d\n",retval>>20, retval&0xfffff );
    }
    else {printk(KERN_ALERT "Could not register the char device" );}
   
    return 0;
}

static void __exit dev_exit(void){
    unregister_chrdev(Major_num, dev_name);
     printk(KERN_ALERT "Device file driver Gone bye\n");
     
}

module_init(dev_Init);
module_exit(dev_exit);