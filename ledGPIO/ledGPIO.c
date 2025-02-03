#include <linux/module.h> // loadable kernel module
#include <linux/gpio.h> //provides all gpio functionality
#include <linux/cdev.h> //handle character device creation
#include <linux/uaccess.h> //provide user to kernel interaction function => copy_to_user() and copy_rom_user()
#include <linux/device.h> // allows for device class creation and prvides struct device and struct class
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/delay.h>  // For msleep()//IMPLEMENTS DELAY


//leds define an led pin as pin 18
#define LEDPIN 4
#define device_name "ledGPIO"

//COMMANDS TO CHECK OUT FOR
char *highcommand = "HIGH";
char *lowcommand = "LOW";
char *invcommand = "INVERT";
char *blnkcommand = "BLINK";


//LED GPIO FUNCTIONALITY IMPLEMENTATIONS

static enum LEDGPIOState {
    OFF = 0,
    ON,
    BLINK
};
enum LEDGPIOState ledgpiostate;
static int ledGPIO_init(void)
{
    //initialize the relevant GPIO PIN

    if(gpio_request(LEDPIN,"rpi-gpio-4"))
    {
        printk(KERN_ALERT "Cannot locate GPIO 4");
        return -1;
    }
    if(gpio_direction_output(LEDPIN, 0)){
         printk(KERN_ALERT "Cannot set GPIO 4 s output");
         gpio_free(LEDPIN);
        return -1;
    }
    printk(KERN_INFO "GPIO setup successful***********\n");
    gpio_set_value(LEDPIN,0);
    ledgpiostate = OFF;
    return 0; // Successful initialization

}
/** variable for timer */
static struct timer_list my_timer;

void timer_callback(struct timer_list * data) {
	//gpio_set_value(4, 0); /* Turn LED off */
    int current_state = gpio_get_value(LEDPIN);
    gpio_set_value(LEDPIN, !current_state);
    pr_info("my state is: %d \n", ledgpiostate);
    //pr_info(ledgpiostate);
    //pr_info("\n");
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000));//reset timer for the next callback
    
     return;
}
static void timer_init(void){
    timer_setup(&my_timer, timer_callback, 0);
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000));
}




//static definitions related to the device
static dev_t MAJOR_NUMBER; //this holds the device major number
static struct cdev ledGPIO_dev;
static struct class *ledGPIO_class;

//define all the functions relating to the operation of a device.
/*
Function Type     |	Key Functions
Initialization	  |  module_init(), module_exit()
Open/Close	      |  open(), release()
Read/Write	      |  read(), write()
IOCTL	          |  unlocked_ioctl()
Interrupt Handling|	request_irq(), free_irq()
Memory Mapping    |  	mmap()
Power Management  |	suspend(), resume()
Kernel Logging	  |    printk(), pr_info()
Sysfs Attributes  |	device_create_file()
Polling	          |  poll()
*/

static int open_LEDGPIO(struct inode *inodep, struct file *filep){
    //inode is => Provides metadata about the file or device, including major/minor numbers. this automatically handled by a this existing in the kernel space
    //file *filep => Represents the open file instance and stores context (such as file offsets and private driver data).
    //open_count++;
    pr_info("Device opened\n");
    return 0;
}

static int release_LEDGPIO(struct inode *inodep, struct file *filep){
    pr_info("Device closed\n");
    return 0;
}

static ssize_t read_LEDGPIO(struct file *filep, char __user *buffer, size_t len, loff_t *offset){
//handles send tp the user space from the kernel

char outputdata[10]; //will be used to store input from user
///1. //here we will be sending the state of the LEDGPIO pin (high or low)(1 or zero)
 //1. read the GPIO STATE
 if(ledgpiostate == ON) snprintf(outputdata, sizeof(outputdata), "HIGH");
 else if(ledgpiostate == OFF) snprintf(outputdata, sizeof(outputdata), "LOW");
 else if(ledgpiostate == BLINK)snprintf(outputdata, sizeof(outputdata), "BLINKING");

//  if(ledgpiostate) outputdata =1;
//  else  outputdata = 0;
   int data_len = strlen(outputdata);

    if(*offset >= data_len){
            return 0; //end of file
    }
    
    if(len > data_len - *offset){
        len = data_len - *offset;
    }

    if(copy_to_user(buffer, outputdata+*offset, len))
    {
        return -EFAULT; //should return 0 if all is okay 
        //this check if the value is a non zero and if so it shows an error occured

    }
    *offset += len;
    return len;
    
}

static ssize_t write_LEDGPIO(struct file *filep, const char __user *user_buffer, size_t count, loff_t *offset){
    //we are reading data from the userspace to the kernel
   char local_buffer[30];
   int bufferSize = sizeof(local_buffer);
    if(*offset >= bufferSize){ //! Ensure we don't write past the buffer
        return  -EFAULT;
    }

    if(*offset + count > bufferSize){
        count = bufferSize - *offset; 
    }

    int bytes_written = count - copy_from_user(local_buffer + *offset, user_buffer, count);
    *offset += bytes_written;

     pr_info("Wrote %zu bytes to device\n", bytes_written);

     //set the state of the led pins here
     if(strstr(local_buffer, highcommand)){
        pr_alert("CHANGING PIN STATE TO HIGH!!\n");
        //check if the previous state was blink and stop it first
        if(ledgpiostate == BLINK){
             del_timer_sync(&my_timer);
             pr_info("stopped blinking\n");
        }
        gpio_set_value(LEDPIN, 1);

        ledgpiostate = ON;
     }
     else if(strstr(local_buffer, lowcommand)){
        pr_alert("CHANGING PIN STATE TO LOW!!\n");
        if(ledgpiostate == BLINK){
             del_timer_sync(&my_timer);
             pr_info("stopped blinking\n");
        }
        gpio_set_value(LEDPIN, 0);
        ledgpiostate = OFF;
     }
     else if(strstr(local_buffer, invcommand)){
        pr_alert("INVERTING PIN!!");
        if(ledgpiostate == ON)
        {
             gpio_set_value(LEDPIN, 0);
             ledgpiostate = OFF;
        }
        else if(ledgpiostate == OFF){
            gpio_set_value(LEDPIN, 1);
            ledgpiostate = ON;
        }
        else if((ledgpiostate == BLINK)){
            del_timer_sync(&my_timer);
             pr_info("stopped blinking\n");
            gpio_set_value(LEDPIN, 0);
             ledgpiostate = OFF;
        }
     }
     else if(strstr(local_buffer, blnkcommand)){
        pr_alert("BLINKING the pin");
        ledgpiostate = BLINK;
        timer_init();//STARTT THE BLINKING PROCESS
        /*
        *@brief we a re going to use timer interrupt for non-blocking blinks
        */
       // initialize the timer

        // int blink_counts = 0;
        // while (1) {
        //     gpio_set_value(LEDPIN, 1);  // Set GPIO high (LED on)
        //     msleep(500);  // Wait for 500 milliseconds
        //     gpio_set_value(LEDPIN, 0);  // Set GPIO low (LED off)
        //     msleep(500);  // Wait for 500 milliseconds
        //   blink_counts++;
        //   if(blink_counts>=20) break;
        // }
     }
     

    return bytes_written;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open_LEDGPIO,
    .release = release_LEDGPIO,
    .write = write_LEDGPIO,
    .read = read_LEDGPIO,
};
static int __init moduleLEDGPIOInit(void){
  
  int retval; //this hold the state of initialization shoud return 0 if all is well
  pr_info("initializing LED driver\n");
  
  if(alloc_chrdev_region(&MAJOR_NUMBER, 0, 1, device_name)<0)
  {
    pr_alert("failed to allocate major number");
    return -1;
  }
  pr_alert("LEDGPIO success :Major number: %d : Minor %d\n", MAJOR_NUMBER, 0);

  ledGPIO_class = class_create(THIS_MODULE, device_name); //tjis will ass a class to the name /sys/class/ledGPIO_class
  
  if(IS_ERR(ledGPIO_class))
  {
    //unregister the major number related to the device
    unregister_chrdev_region(MAJOR_NUMBER,1);
    printk(KERN_ALERT "UNABLE TO SETUT DEVICE CLASS!!\n");
     return -1; // Return error if allocation fails
  }
  pr_alert("LEDGPIO_CLASS success \n", MAJOR_NUMBER, 0);

  cdev_init(&ledGPIO_dev, &fops); //initialize the character device
  //the add the character device to /dev
  if(cdev_add(&ledGPIO_dev, MKDEV(MAJOR(MAJOR_NUMBER), 0), 1)<0){
    class_destroy(ledGPIO_class);
    unregister_chrdev_region(MAJOR_NUMBER, 1);
    printk(KERN_ALERT "UNABLE to add n register CDEV - char_device!!\n");
    return -1;
  }
//CREATE THE DEVICE NODE IN /dev
 if(device_create(ledGPIO_class, NULL, MKDEV(MAJOR(MAJOR_NUMBER),0),NULL,device_name) == NULL){
    //if it return null undo all previous operations
    cdev_del(&ledGPIO_dev);
    class_destroy(ledGPIO_class);
    unregister_chrdev_region(MAJOR_NUMBER, 1);
    printk(KERN_ALERT "could not create device in /dev!!\n");
    return -1;
 }
 //if all these pass
    printk(KERN_INFO "ledGPIO_device registered with major number %d\n", MAJOR_NUMBER);
     
     //init gpio
     ledGPIO_init();

 return 0;
}

static void __exit moduleLEDGPIOExit(void){
    // undo all previous operations
     del_timer_sync(&my_timer);
    gpio_set_value(LEDPIN, 0);  // Turn off the LED
    gpio_free(LEDPIN);
    device_destroy(ledGPIO_class,  MKDEV(MAJOR(MAJOR_NUMBER), 0));
    cdev_del(&ledGPIO_dev);
    class_destroy(ledGPIO_class);
    unregister_chrdev_region(MAJOR_NUMBER, 1);
    printk("all are removed\n");
    printk(KERN_ALERT "char Device file driver Gone bye\n");
    return;
}

module_init(moduleLEDGPIOInit);
module_exit(moduleLEDGPIOExit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("R-Mutura Macharia");
MODULE_DESCRIPTION("LED DEVICE DRIVER FOR EASY LED USES FROM USERSPACE");