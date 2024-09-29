/*
* this is a module utilizig the i2c device and module and api that is alreay existing inside
* the rpi-kernel module
* its almost but not exactly lie utilizig the I2C Bus
* Written for testing purposes only
* 29/09/2024
* Eng. R-Mutura
 */

#include <linux/module.h> // It contains macros and functions for working with loadable modules, such as module_init() and module_exit()
#include <linux/cdev.h> //provides structures and functions for character device management in the kernel
#include <linux/uaccess.h> //provides functions like copy_to_user() and copy_from_user(),
#include <linux/fs.h> // functions related to the Linux Virtual File System (VFS). It includes definitions for file operations (e.g., read, write, open, release)
#include <linux/kernel.h> //It includes functions like printk()
#include <linux/slab.h> //header provides the kernel’s memory management functions, specifically for dynamic memory allocation. Functions like kmalloc() and kfree()
#include <linux/init.h> //provides macros and functions for module initialization and cleanup. The macros __init and __exit

//we will b utilizing i2c-apis
#include <linux/i2c.h>
//#include <linux/i2c-dev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("R-Mutura");
MODULE_DESCRIPTION("I2C Driver for BMP Pressure Sensor");
/*****************   HOW TO SETUP I2C *****************************************
* Driver Registration/Unregistration: i2c_add_driver(), i2c_del_driver()
* Reading/Writing Registers (SMBus): i2c_smbus_read_byte_data(), i2c_smbus_write_byte_data()
* Generalized I2C Communication: i2c_transfer()
* Word Data Read/Write: i2c_smbus_read_word_data(), i2c_smbus_write_word_data()
* Block Data Read/Write: i2c_smbus_read_i2c_block_data(), i2c_smbus_write_i2c_block_data()
 */

//Defines for device identificaion
#define I2C_BUS_AVAILABLE     1
#define SLAVE_DEVICE_NAME    "BMP280_Driver"
#define BMP280_SLAVE_ADDRESS  0x76



static dev_t        myDeviceNr;
static struct class *myClass;
static struct cdev  mydevice ;

//create an adapter and a client

static struct i2c_client  * mybmp_i2c_client  = NULL;
static struct i2c_adapter * mybmp_i2c_adapter = NULL;

//DEFINE THE I2C device using i2c_device_id - its a constant since we dont expect it to change
static const struct i2c_device_id bmp_id[] = {
    {SLAVE_DEVICE_NAME, 0}, //device name and optional data
    {}
};

static struct i2c_driver bmp_driver = { //here we st the devices that the driver we are writing can handle
    .driver = {
        .name = SLAVE_DEVICE_NAME, // Driver name
        .owner = THIS_MODULE,
    },
    //.probe = bmp_probe,  // Pointer to the probe function
    //.remove = bmp_remove, // Pointer to the remove function
    .id_table = bmp_id,  // List of supported devices == (id table)
};
//for a more static (not dynamic) registration of I2C to the Bus we will use of_device_id 
//for dynamic loading of driver upon connection of module we can use other device tree descriptors 

static const struct i2c_board_info bmp_i2c_board_info __attribute__((unused)) = {
    I2C_BOARD_INFO(SLAVE_DEVICE_NAME, BMP280_SLAVE_ADDRESS)
};



//describe file operations that the driver can perform
//open device
static int bmp_driver_open(struct inode *inodep, struct file *filep) {
    return 0; // No special operations
}
//read
static ssize_t bmp_driver_read(struct file *filep, char *user_buffer, size_t count, loff_t *offs){
    int to_copy, not_cpoed, delta;
    char out_string [20];
    int temperature;

    return count;
}

static struct file_operations fops={
    .owner = THIS_MODULE,
    .open = bmp_driver_open,
    .read = bmp_driver_read,
};
static int __init bmp_driver_init(void){
    //register the device to the kernel and /dev
    printk(KERN_ALERT "regestering a BMP_I2C DRIVER TO KERNEL\n");
     
     int ret = -1;
	
    //allocate region for device registration
    if(alloc_chrdev_region(&myDeviceNr, 0, 1, SLAVE_DEVICE_NAME)< 0 )
    {
        pr_alert("FAILED TO REGISTER THE DEVICE MAJOR AND MINOE NUMBER");
        return -1;
    }
    pr_info("Device BMP_I2C_DRIVER registered successfully");

    //create a device class
    myClass = class_create(THIS_MODULE, SLAVE_DEVICE_NAME );
    if(IS_ERR(myClass))
    {
        pr_info("unable to create class___");
       goto addClassErr;
    }
   

    //once all these pass
     // Create a device node in /dev

     if(device_create(myClass, NULL, myDeviceNr, NULL, SLAVE_DEVICE_NAME ) ==NULL){
        printk("Can not create device file!\n");
		goto FileError;
     }
    
    cdev_init(&mydevice, &fops);
    if (cdev_add(&mydevice, MKDEV(MAJOR(myDeviceNr), 0), 1) < 0)
    {
        goto addCdevErr;
    }

    printk(KERN_INFO "I2c BMP280 DRIVER SUCCESSFUL regitered major number %d\n");
  //do i2c initializations here
  //register an adapter and client so that we can read them
  mybmp_i2c_adapter =  i2c_get_adapter(I2C_BUS_AVAILABLE); 
    if(mybmp_i2c_adapter != NULL){//THERE IS A DEVICE ON THE BUS
        mybmp_i2c_client = i2c_new_device(mybmp_i2c_adapter, &bmp_i2c_board_info);
        //By calling i2c_acpi_new_device, the device is registered with the I2C core, allowing the kernel's I2C subsystem to manage the device, including enabling communication with it through I2C protocols.
        if(mybmp_i2c_client != NULL){
            if(i2c_add_driver(&bmp_driver) != -1)
            {
                ret = 0;
            }
        
        else{
				printk("Can't add driver...\n");
		}
        }
        i2c_put_adapter(mybmp_i2c_adapter);
    }
    printk("BMP280 Driver added!\n");
    // 1. add i2c driver
      //lets add some i2c reads here and print them out
  __u8 sensor_id = i2c_smbus_read_byte_data(mybmp_i2c_client, 0xD0);
  printk("ID: 0x%x\n", sensor_id);

   /* Initialice the sensor */
    i2c_smbus_write_byte_data(mybmp_i2c_client, 0xf5, 5<<5);
	i2c_smbus_write_byte_data(mybmp_i2c_client, 0xf4, ((5<<5) | (5<<2) | (3<<0)));

  //read device ID
  //read temp/any other register.
    return ret;

addClassErr: 
    unregister_chrdev_region(myDeviceNr,1);
    return -1;
addCdevErr: 
    device_destroy(myClass, myDeviceNr);
    class_destroy(myClass);
    unregister_chrdev_region(myDeviceNr,1);
    printk(KERN_ALERT "UNABLE TO ADD CDEV DEVICE \n");
    return -1;
FileError:
    class_destroy(myClass);
    unregister_chrdev_region(myDeviceNr,1);
    printk(KERN_ALERT "UNABLE TO ADD CDEV DEVICE class \n");
    return -1;
    

}

static void __exit bmp_driver_exit(void){
    //delete all i2c implementations
    i2c_del_driver(&bmp_driver);

    //delete the bones structure of the driver
    device_destroy(myClass, myDeviceNr);
    cdev_del(&mydevice);
    class_destroy(myClass);
    unregister_chrdev_region(&myDeviceNr,1);
    printk("MyDeviceDriver - Goodbye, Kernel!\n");
    return;
}