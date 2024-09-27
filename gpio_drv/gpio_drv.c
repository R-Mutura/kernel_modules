#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/gpio.h>



MODULE_LICENSE("GPL");
dev_t Major_num;  // dev_t type for device number
#define dev_name "mygpio_driver"

char buffer [256];
#define buffer_size 256


static struct class *my_class;
static struct cdev my_cdev;
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

static ssize_t read_device(struct file *filep, char *user_buffer, size_t count, loff_t *offset){
     int bytes_to_copy;
    int bytes_read = 0;
    //copy to user the data
    //check that offset is not zero
    if(*offset >= buffer_size || buffer[*offset] == 0 ){
        return 0;
        }
     //calculates size to copy to user
       bytes_to_copy = min(count, buffer_size-*offset);
     if(copy_to_user(user_buffer, buffer+*offset, bytes_to_copy))
     {
         return -EFAULT;  // Error in copying data
     }
        *offset += bytes_to_copy;
    bytes_read += bytes_to_copy;
    
    return bytes_read; 
}

static ssize_t write_device(struct file *filep, const char *user_buffer, size_t count, loff_t *offset){
     int to_copy;
     char value;
    //copy from user the data and save to buffer
    to_copy = min(count, sizeof(buffer)-1);
    //copy_from_user(&value, user_buffer, to_copy);
    if(copy_from_user(buffer, user_buffer, to_copy)){//this copies each char one by one
             return -EFAULT;  // Error in copying data
        }
        buffer[to_copy] = "\0";
    switch(buffer[0])
    {
        case '0':
          gpio_set_value(4,0);
        break;

        case '1':
             gpio_set_value(4,1);
        break;

        default:
           printk(KERN_INFO "invalid inpt\n");
        break;
    }
    
    //buffer[count] = "\0";
    return count;
    
}
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open_devfile,
    .release = close_devfile,
    .read = read_device,
    .write = write_device,
};
static int __init dev_Init(void){
    int retval;
    printk(KERN_ALERT "Device file driver Module initialized\n");
    //we add device class to enable self registration of the device to /dev folder
    /*
    * Define the Device Structure: Use the struct cdev to represent your character device.

                Register the Character Device: Use alloc_chrdev_region or register_chrdev_region to allocate a major and minor number for your device.

                Create the Device Class: Use class_create to create a device class in sysfs.

                Create the Device Node: Use device_create to create the actual device node in /dev.

                Implement the Cleanup Code: Ensure that you clean up properly by unregistering the device and freeing resources when the module is removed.

                    */

    
    //allocate dev number
    if(alloc_chrdev_region(&Major_num, 0 , 1, dev_name) < 0){
        printk(KERN_ALERT "Failed to allocate major number\n");
        return -1; // Return error if allocation fails
    }
    //
      printk(KERN_ALERT "Device registered successfully Major: %d : Minor %d\n",Major_num, 0 );
   //if successful then  create a device class for the device insysfs
   my_class = class_create(THIS_MODULE, dev_name);
   if(IS_ERR(my_class))
   {
    //IF THERE IS ERRO THE UNREGISTER THE DEVICE AS A CLEAN UP OF USE GO TO TO GO TO EXIT WHIH CONTAIN ALL CLEANUPS
    unregister_chrdev_region(Major_num, 1);
    printk(KERN_ALERT "UNABLE TO SETUT DEVICE CLASS!!\n");
     return -1; // Return error if allocation fails
   }

   //then if all passs init the cdev_device
   // Initialize character device structure with file operations
   cdev_init(&my_cdev, &fops);

   if(cdev_add(&my_cdev, MKDEV(MAJOR(Major_num), 0),1)< 0)
   {
    class_destroy(my_class);
    unregister_chrdev_region(Major_num, 1);
    printk(KERN_ALERT "UNABLE to register CDEV - char_device!!\n");
    return -1;
   }
   // Create a device node in /dev
   if(device_create(my_class, NULL, MKDEV(MAJOR(Major_num),0), NULL, dev_name) ==NULL)
   {
    //cleanup all prev setup and registrations code
    cdev_del(&my_cdev);
    class_destroy(my_class);
    unregister_chrdev_region(Major_num, 1);
    
    printk(KERN_ALERT "failed to create device node!!\n");
    return -1;
   }
   //if all these pass
    printk(KERN_INFO "simple_char_device registered with major number %d\n", Major_num);

    //init a GPIO - gpio 4
    if(gpio_request(4,"rpi-gpio-4"))
    {
        printk(KERN_ALERT "Cannot locate GPIO 4");
        goto AddError;
    }
    if(gpio_direction_output(4, 0)){
         printk(KERN_ALERT "Cannot set GPIO 4 s output");
        goto gpio4Error;
    }
    printk(KERN_INFO "GPIO setup successful***********\n");
    return 0; // Successful initialization

gpio4Error:
        gpio_free(4);
AddError:
        device_destroy(my_class,  MKDEV(MAJOR(Major_num), 0));
FileError:
        class_destroy(my_class);
ClassError:
        unregister_chrdev_region(Major_num, 1); // Unregister the major number
        return -1;
}

static void __exit dev_exit(void){
    gpio_set_value(4, 0);
    gpio_free(4);
    device_destroy(my_class,  MKDEV(MAJOR(Major_num), 0)); // Remove the device from /dev
    cdev_del(&my_cdev);               // Delete the character device
    class_destroy(my_class);          // Destroy the device class
    unregister_chrdev_region(Major_num, 1); // Unregister the major number
    printk(KERN_INFO "simple_char_device unregistered\n");
     printk(KERN_ALERT "char Device file driver Gone bye\n");
     
}

module_init(dev_Init);
module_exit(dev_exit);