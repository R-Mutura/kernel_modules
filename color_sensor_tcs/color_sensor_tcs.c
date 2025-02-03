/*
module to run TCS34725 color sensor and see its operation.
from rpi using the i2c bus
*/

#include <linux/module.h> // It contains macros and functions for working with loadable modules, such as module_init() and module_exit()
#include <linux/cdev.h> //provides structures and functions for character device management in the kernel
#include <linux/uaccess.h> //provides functions like copy_to_user() and copy_from_user(),
#include <linux/fs.h> // functions related to the Linux Virtual File System (VFS). It includes definitions for file operations (e.g., read, write, open, release)
#include <linux/kernel.h> //It includes functions like printk()
#include <linux/slab.h> //header provides the kernel’s memory management functions, specifically for dynamic memory allocation. Functions like kmalloc() and kfree()
#include <linux/init.h> //provides macros and functions for module initialization and cleanup. The macros __init and __exit
#include <linux/delay.h>

//we will b utilizing i2c-apis
#include <linux/i2c.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("R-Mutura");
MODULE_DESCRIPTION("I2C Driver for TCS34725 COLOR Sensor");

//1. creating all items of a device driver as needed.
#define Device_name "TCSColorSns"

static dev_t TCSDevice_major;
static struct cdev tcsDevice;
static struct class *tcsClass;

// i2c store variables
int red_data, blu_data, grn_data;
// i2c prototype
int tcs_rgb_read(void);
static int init_tcs_color_sensor(void);

//define file operations for the device driver
static int open_TCSColorSns_drv(struct inode *inodep, struct file *filep){
  pr_info("TCSColorSns driver opened\n");
  return 0;
}
static int close_TCSColorSns_drv(struct inode *inodep, struct file *filep){
  pr_info("TCSColorSns driver Closed\n");
  return 0;
}
static ssize_t read_TCSColorSns_drv(struct file *filep, char *user_buffer, size_t count, loff_t *offset){
  pr_info("TCSColorSns drv exposing data to user\n");
  char out_string[100];
  if(tcs_rgb_read() == 0){
    printk( "writing to user space the following red: %d, Grn: %d, Blu: %d", red_data,blu_data,grn_data);
    
    // int rgb_buffer [3];
    // rgb_buffer [0] = red_data;
    // rgb_buffer [1] = blu_data;
    // rgb_buffer [2] = grn_data;

    scnprintf(out_string, sizeof(out_string), "RGB: %d, %d, %d\n", red_data, grn_data, blu_data);
     
      // Ensure offset and count checks
    if (*offset >= sizeof(out_string)) {
        return 0;  // End of file reached
    }

    if (count > sizeof(out_string) - *offset) {
        count = sizeof(out_string) - *offset;
    }

    if(copy_to_user(user_buffer, out_string, sizeof(out_string))== 0)
    {
         printk(KERN_INFO "TCSColorSns_drv: RGB data successfully sent to user\n");
        *offset += count;
        return count;  // Return the number of bytes copied
    } else {
        // Failed to copy all data
        printk(KERN_ERR "TCSColorSns_drv: Failed to send RGB data to user\n");
        return -EFAULT;
    }
    

    
  }
  else{

  }
  return 0;
}

static ssize_t write_TCSColorSns_drv(struct file *filep, const char *user_buffer, size_t count, loff_t *offset){
  char kernel_buffer[100];  // Kernel buffer to store user data

    pr_info("TCSColorSns drv: Getting data from user\n");

    // Ensure that the number of bytes does not exceed the buffer size
    if (count > sizeof(kernel_buffer) - 1)
        count = sizeof(kernel_buffer) - 1;

    // Copy data from user space to kernel space
    if (copy_from_user(kernel_buffer, user_buffer, count)) {
        pr_err("TCSColorSns drv: Failed to copy data from user\n");
        return -EFAULT;  // Return error if copy fails
    }

    kernel_buffer[count] = '\0';  // Null-terminate the string for safety
    pr_info("TCSColorSns drv: Received data: %s\n", kernel_buffer);

    // Return the number of bytes written successfully
    return count;
}

static struct file_operations fops = {
  .owner = THIS_MODULE,
  .open  = open_TCSColorSns_drv,
  .release  = close_TCSColorSns_drv,
  .write = write_TCSColorSns_drv,
  .read  = read_TCSColorSns_drv,
};
//to define all the i2c functionality.
#define I2C_BUS_ADAPTER 1
//#define SLAVE_DEVICE_NAME    "BMP280_Driver"
#define TCS34725_SLAVE_ADDRESS  0x29

static struct i2c_board_info tcs_board_info = {
   I2C_BOARD_INFO(Device_name, TCS34725_SLAVE_ADDRESS)
};
//board id should be 0x5d

static struct i2c_adapter *tcs_i2c_adapter = NULL;
static struct i2c_client   *tcs_i2c_client  = NULL;

//create device i2c id
static struct i2c_device_id  tcs_board_i2c_id[] = {
    {Device_name, 0},
    {}
};
//create device driver
struct i2c_driver tcs_board_i2c_driver = {
  .driver ={
    .name = Device_name,
    .owner = THIS_MODULE,
  },
  .id_table = tcs_board_i2c_id,
};
static int read_sensor_id(void) {
    // Reading the ID from register 0x12
    int ret;
    char myid;

    ret = i2c_smbus_read_byte_data(tcs_i2c_client, 0x12);
    if (ret < 0) {
        printk(KERN_ERR "Failed to read from TCS34725 color sensor: %d\n", ret);
        return ret; // Return the error code
    }

    myid = (char)ret; // Cast the result to char

    // Log the ID; note that myid is a byte, not a string
    printk(KERN_INFO "TCSDevice color sensor ID = 0x%02X\n", myid);
    return 0; // Return success
}
static int init_tcs_color_sensor(void)
{
  int ret;
    char myid;

    ret = i2c_smbus_read_byte_data(tcs_i2c_client, 0x12);
    if (ret < 0) {
        printk(KERN_ERR "Failed to read from TCS34725 color sensor: %d\n", ret);
        return ret; // Return the error code
    }

    myid = (char)ret; // Cast the result to char

    // Log the ID; note that myid is a byte, not a string
    printk(KERN_INFO "TCSDevice color sensor ID = 0x%02X\n", myid);
    //the sensor is connected and working well.
    //1. set the command register

    //1. set the enable register to 0b00000011 to enable RGBC AND INTERNAL OSCLLATOR
    ret = i2c_smbus_write_byte_data(tcs_i2c_client, 0x00, 0x03);
    if (ret < 0) {
        printk(KERN_ERR "Failed to write to TCS34725 color sensor: %d\n", ret);
        return ret; // Return the error code
    }
    
    return 0; // Return success
  
}
#define STATUS_REG 0x13
 int tcs_rgb_read(void){
  int ret;
  char redbuffer[2], blubuffer[2], grnbuffer[2];
  //1. check the status of the status register 0x13
  ret = i2c_smbus_read_byte_data(tcs_i2c_client, STATUS_REG);
  printk(KERN_INFO "Status register: 0x%02X\n", ret);
  if(ret< 0){
     printk(KERN_ERR "Failed to read from TCS34725 color sensor: %d\n", ret);
        return ret; // Return the error code
  }
  if((char)ret & 0x01) 
  {
    //means his bit is asserted and we can read the valid bit of rgb
    //perform a read word to capture all the data in the high and low registers
     ret = i2c_smbus_read_i2c_block_data(tcs_i2c_client,0x16, 2, redbuffer);
    
    if (ret < 0) {
        printk(KERN_ERR "Failed to read red data from TCS34725: %d\n", ret);
        return -1;
    } else {
        // Ensure we handle the data correctly
         red_data = (redbuffer[0] << 8) | redbuffer[1];  // Combine the two bytes into an integer
        
        // Print the red data value
       // printk(KERN_INFO "Red Data: %d (Hex: 0x%04X)\n", red_data, red_data);
    }
    msleep(50);
    ret = i2c_smbus_read_i2c_block_data(tcs_i2c_client,0x1A, 2, blubuffer);
    
    if (ret < 0) {
        printk(KERN_ERR "Failed to read blue data from TCS34725: %d\n", ret);
        return -1;
    } else {
        // Ensure we handle the data correctly
         blu_data = (blubuffer[0] << 8) | blubuffer[1];  // Combine the two bytes into an integer

        // Print the red data value
        //printk(KERN_INFO "Blue Data: %d (Hex: 0x%04X)\n", blu_data, blu_data);
    }
    msleep(50);
    ret = i2c_smbus_read_i2c_block_data(tcs_i2c_client,0x18, 2, grnbuffer);
    
    if (ret < 0) {
        printk(KERN_ERR "Failed to read green data from TCS34725: %d\n", ret);
        return -1;
    } else {
        // Ensure we handle the data correctly
         grn_data = (grnbuffer[0] << 8) | grnbuffer[1];  // Combine the two bytes into an integer

        // Print the red data value
       // printk(KERN_INFO "Green Data: %d (Hex: 0x%04X)\n", grn_data, grn_data);
    }
    // After reading each color
    printk(KERN_INFO "Red buffer: 0x%02X%02X\n", redbuffer[0], redbuffer[1]);
    printk(KERN_INFO "Green buffer: 0x%02X%02X\n", grnbuffer[0], grnbuffer[1]);
    printk(KERN_INFO "Blue buffer: 0x%02X%02X\n", blubuffer[0], blubuffer[1]);

    //reset the buffer the data and clear 
    //ret = i2c_smbus_write_byte_data(tcs_i2c_client, 0x14, 0x00);
    //ret = i2c_smbus_write_byte_data(tcs_i2c_client, 0x15, 0x00);
    if (ret < 0) {
        printk(KERN_ERR "Failed to write to TCS34725 color sensor: %d\n", ret);
        return 0; // Return the error code
    }
  }
  else{

  }
  return 0;
}

//global read data

static int __init TCSColorSns_init(void){
  pr_alert("Hello from TCS34725 COLOR Sensor driver");
  //lets create a device 

  if(alloc_chrdev_region(&TCSDevice_major, 0 , 1, Device_name )< 0)
  {
    printk(KERN_ALERT "Failed to allocate major number to TCSDevice_driver\n");
     return -1;
  }
  //if major number is allocated then create class
   tcsClass = class_create(THIS_MODULE, Device_name);
   if(IS_ERR(tcsClass)){
    unregister_chrdev_region(TCSDevice_major, 1);
    printk(KERN_ALERT "Failed to create class for TCSDevice_driver\n");
     return -1;
   }
   //if class is created successfully then init cdev
   cdev_init(&tcsDevice, &fops);
   //then add char_device
   if(cdev_add(&tcsDevice, MKDEV(MAJOR(TCSDevice_major), 0), 1)< 0){//register a character device with the kernel so that user-space programs can access it
    class_destroy(tcsClass);
    unregister_chrdev_region(TCSDevice_major, 1);
    printk(KERN_ALERT "UNABLE to register TCSDevice - device!!\n");
    return -1;
   }
   //IF cdev add is successfull then we do a device_create
    if(device_create(tcsClass, NULL, MKDEV(MAJOR(TCSDevice_major),0), NULL, Device_name) ==NULL)
        {
          cdev_del(&tcsDevice);
          class_destroy(tcsClass);
          unregister_chrdev_region(TCSDevice_major, 1);
          printk(KERN_ALERT "UNABLE to create device class of TCSDevice - device!!\n");
          return -1;
        }
         printk(KERN_INFO "TCSDevice color sensor driver registered with major number %d\n", TCSDevice_major);
    // Successful initialization

    //perform I2C initializations here
    tcs_i2c_adapter = i2c_get_adapter(I2C_BUS_ADAPTER);
    if(tcs_i2c_adapter != NULL){ //WE CAN PROCEED TO OTHER INITS
      tcs_i2c_client = i2c_new_device(tcs_i2c_adapter, &tcs_board_info);
      if(tcs_i2c_client != NULL ){
          i2c_add_driver(&tcs_board_i2c_driver);

      }
      else {
            // Cleanup on failure
            pr_err("Failed to create new I2C device\n");
            i2c_put_adapter(tcs_i2c_adapter);
            return -1;
        }
      i2c_put_adapter(tcs_i2c_adapter);
    }
    else {
        printk(KERN_ALERT "Failed to get I2C adapter\n");
        return -1;
    }

    printk(KERN_INFO "TCSDevice color sensor i2c initialization SUCCESSFULL\n", TCSDevice_major);
    // Successful I2C initialization
    //LETS DO A READ OF THE SENSOR ID
     if (init_tcs_color_sensor() < 0) {
        printk(KERN_ALERT "Error reading TCS34725 sensor ID\n");
    }

    tcs_rgb_read();
  return 0;
}

static void __exit TCSColorSns_exit(void){
  if (tcs_i2c_client) {
        i2c_unregister_device(tcs_i2c_client); // Unregister I2C client only if it's initialized
        i2c_del_driver(&tcs_board_i2c_driver);
        tcs_i2c_client = NULL;
        tcs_i2c_adapter = NULL;
    }

    device_destroy(tcsClass, TCSDevice_major);
        class_destroy(tcsClass);
        tcsClass = NULL;  // Set to NULL after destroying
  // if (tcsClass) {
  //       device_destroy(tcsClass, TCSDevice_major);
  //       class_destroy(tcsClass);
  //       tcsClass = NULL;  // Set to NULL after destroying
  //   }

    // Cleanup character device
    cdev_del(&tcsDevice);
  unregister_chrdev_region(TCSDevice_major, 1);
          
  pr_info("BYE BYE from TCS34725 COLOR Sensor driver");
  pr_alert("BYE BYE from TCS34725 COLOR Sensor driver");
  return;
}

module_init(TCSColorSns_init);
module_exit(TCSColorSns_exit);