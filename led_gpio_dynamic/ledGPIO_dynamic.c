
#include <linux/module.h> // loadable kernel module
#include <linux/gpio.h> //provides all gpio functionality
#include <linux/cdev.h> //handle character device creation
#include <linux/uaccess.h> //provide user to kernel interaction function => copy_to_user() and copy_rom_user()
#include <linux/device.h> // allows for device class creation and prvides struct device and struct class
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/delay.h>  // For msleep()//IMPLEMENTS DELAY
#include <linux/slab.h> // this provides kmalloc() krealloc() kfree().
#include <linux/ioctl.h>


//PROTOTYPE
static int add_ledGPIO_instance(void);

//leds define an led pin as pin 
//#define led_gpio 4
 int led_gpio = -1; //DEFUALT VALUE IS NO PIN IF NO PARAMETER IS PASSED TO THE MODULE
#define device_name "ledGPIODriver"
#define INITIAL_INSTANCES 1//JUST INCASE

//COMMANDS TO CHECK OUT FOR
char *highcommand = "HIGH";
char *lowcommand =  "LOW";
char *invcommand =  "INVERT";
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
         char rpi_pin_description[15];
         int ret = 0;
         ret = snprintf(rpi_pin_description, sizeof(rpi_pin_description), "rpi-gpio-%d", led_gpio);
         if(ret<0){
            //error doing the necessary pin definition update
             printk(KERN_ALERT "Ending this unable to prperly define pin %d", led_gpio);
             return -1;
         }
    if(gpio_request(led_gpio, rpi_pin_description))
    {
        printk(KERN_ALERT "Cannot locate GPIO %d", led_gpio);
        return -1;
    }
    if(gpio_direction_output(led_gpio, 0)){
         printk(KERN_ALERT "Cannot set GPIO 4 s output");
         gpio_free(led_gpio);
        return -1;
    }
    printk(KERN_INFO "GPIO setup successful***********\n");
    gpio_set_value(led_gpio,0);
    ledgpiostate = OFF;
    return 0; // Successful initialization

}
/** variable for timer */
//static struct timer_list my_timer; //this is a global definition that persists beyond this kernel module(its available in kernel space i think)


void timer_callback(struct timer_list * data) {
	//gpio_set_value(4, 0); /* Turn LED off */
    int current_state = gpio_get_value(led_gpio);
    gpio_set_value(led_gpio, !current_state);
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
//static struct cdev ledGPIO_dev;
//we define a pointer to a pointer that will hold a pointer to the instanced of character devices created
static struct cdev **ledGPIO_cdevs = NULL; 
static struct class *ledGPIO_class = NULL;

static int num_instances = 0;
//-----------------------------------------------------------------//
/*
@brief: in order to eable a module to take up paramenter during loading we we ill use these=> check below
module_param(led_gpio, int, 0660);  // Pass GPIO pin number as a parameter
MODULE_PARM_DESC(led_gpio, "GPIO pin number for LED control");

This macro allows the led_gpio variable to be passed as a parameter when the kernel module is loaded. Let's break it down:

led_gpio: The variable that stores the GPIO pin number for the LED.
int: The data type of the parameter (in this case, an integer).
0660: The file permission bits, defining who can read and write the parameter:
0: No permission
6: Read and write permission
So, 0660 means the owner and group can read/write; others have no access.
    666 - accesss by every one but causes issues in compilation asn is not allowed
MODULE_PARM_DESC(led_gpio, "GPIO pin number for LED control");
This macro provides a description of the parameter for documentation purposes. It helps anyone inspecting the module (using tools like modinfo) to understand what the parameter does

*/
module_param(led_gpio, int, 0644); //see descrition of working above
MODULE_PARM_DESC(led_gpio, "GPIO pin number for LED control");

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
    //get the minor number of the instance => the opened device instance
    int minor = iminor(inodep);
    pr_info("Device opened my Minor Number is : %d \n", minor);
    return 0;
}

static int release_LEDGPIO(struct inode *inodep, struct file *filep){
    int minor = iminor(inodep);
    pr_info("Device closed my Minor Number is : %d \n", minor);
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
   size_t data_len = strlen(outputdata);

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
    
    /*
    @brief: we are making changes to how timer is created called and handled for flexibility
           when we use the timer to blink the leds it crashed when ther is more than one led to blink
            this is coz we had initaliy declared a global timer used by all leds causing multiple access thuc kernel crash
    */
   //we need to obtain the minor number of the node/led meodule instance accessing this driver
   int myMinor = iminor(file_inode(filep)); // this returns the minor number of this device
   //let us also access the private data of the node -> we will use it to store the GPIOPIN assosiated with the instance

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

      //HANDLE INITIALIZATION COMMAND FROM USER
        
        if(strstr(local_buffer, "led_gpio")){ //this acts as pin intialization call 
            //*local_buffer = "led_gpio=4";
             //means this contain led_gpio setting
              char *token;
              char *rest = local_buffer;
              char *key, *value;

              // Extract key-value pair
                key = strsep(&rest, "=");  // Extract key ("led_gpio")
                value = strsep(&rest, "=");  // Extract value ("4")
   
        if (value ) {
           led_gpio = simple_strtol(value, NULL, 10);
            pr_info("Value: %s\n", led_gpio); // Extracted value
            //let us also access the private data of the node -> we will use it to store the GPIOPIN assosiated with the instance
            //store the pin in private data
            filep->private_data = (void *)(uintptr_t) led_gpio; //it is set for this instance of the node
                        
        } else {
            pr_info("No value found after '='.\n");
            return -1;
        }
    

                     printk(KERN_ALERT "INTILIAZING GPIO: %d AS AN LED PIN\n", led_gpio);

                     ledGPIO_init(); //pin initialization is done here
        }
        //check if the pin is intatiated
        if(led_gpio < 0)
        {
             pr_info("please set the GPIO PIN TO USE");
             return -1;
        }
     pr_info("Wrote %zu bytes to device\n", bytes_written);
     
     //set the state of the led pins here
     if(strstr(local_buffer, highcommand)){
        pr_alert("CHANGING PIN STATE TO HIGH!!\n");
        //check if the previous state was blink and stop it first
        if(ledgpiostate == BLINK){
             del_timer_sync(&my_timer);
             pr_info("stopped blinking\n");
        }
        gpio_set_value(led_gpio, 1);

        ledgpiostate = ON;
     }
     else if(strstr(local_buffer, lowcommand)){
        pr_alert("CHANGING PIN STATE TO LOW!!\n");
        if(ledgpiostate == BLINK){
             del_timer_sync(&my_timer);
             pr_info("stopped blinking\n");
        }
        gpio_set_value(led_gpio, 0);
        ledgpiostate = OFF;
     }
     else if(strstr(local_buffer, invcommand)){
        pr_alert("INVERTING PIN!!");
        if(ledgpiostate == ON)
        {
             gpio_set_value(led_gpio, 0);
             ledgpiostate = OFF;
        }
        else if(ledgpiostate == OFF){
            gpio_set_value(led_gpio, 1);
            ledgpiostate = ON;
        }
        else if((ledgpiostate == BLINK)){
            del_timer_sync(&my_timer);
             pr_info("stopped blinking\n");
            gpio_set_value(led_gpio, 0);
             ledgpiostate = OFF;
        }
     }
     else if(strstr(local_buffer, blnkcommand)){
        pr_alert("BLINKING the pin");
        ledgpiostate = BLINK;
        //we are going to use the minor number and pin number stored in private_data to create a timer and intialize it for blinking
        //1. get the minor number which is alread gitten above  *myMinor*
        //2.create a timer_list variable
        static struct timer_lists 
        timer_init();//START THE BLINKING PROCESS
        /*
        *@brief we a re going to use timer interrupt for non-blocking blinks
        */
       // initialize the timer

        // int blink_counts = 0;
        // while (1) {
        //     gpio_set_value(led_gpio, 1);  // Set GPIO high (LED on)
        //     msleep(500);  // Wait for 500 milliseconds
        //     gpio_set_value(led_gpio, 0);  // Set GPIO low (LED off)
        //     msleep(500);  // Wait for 500 milliseconds
        //   blink_counts++;
        //   if(blink_counts>=20) break;
        // }
     }
     

    return bytes_written;
}
// Define ioctl commands
#define LEDGPIO_MAGIC 'L'
// #define LEDGPIO_ON  _IO(LEDGPIO_MAGIC, 0)
// #define LEDGPIO_OFF _IO(LEDGPIO_MAGIC, 1)
// #define LEDGPIO_BLINK _IO(LEDGPIO_MAGIC, 2)
// #define LEDGPIO_INVERT _IO(LEDGPIO_MAGIC, 3)
#define LEDGPIO_SET_PIN _IOW(LEDGPIO_MAGIC, 0, int)

static long ledgpio_ioctl(struct file *filep, unsigned int cmd, unsigned long arg){
     int ret = 0;
     if (_IOC_TYPE(cmd) != LEDGPIO_MAGIC) {
        return -EINVAL;  // Invalid magic number
    }
     switch (cmd) {
     case LEDGPIO_SET_PIN:
            pr_info("LEDGPIO: Setting GPIO pin to %ld\n", arg);
            if (arg >= 0 && arg <= 53) {  // Assuming a GPIO range (adjust as necessary)
                led_gpio = arg;
                add_ledGPIO_instance();
                //gpio_request(led_gpio, "ledgpio");
                //gpio_direction_output(led_gpio, 0);
            } else {
                ret = -EINVAL;
            }
            break;

             default:
            pr_err("LEDGPIO: Invalid command\n");
            ret = -EINVAL;
            break;

     }
     return ret;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open_LEDGPIO,
    .release = release_LEDGPIO,
    .write = write_LEDGPIO,
    .read = read_LEDGPIO,
    .unlocked_ioctl = ledgpio_ioctl,  // Add this line for ioctl support
};

static int add_ledGPIO_instance(void){
    int ret;
    struct device *dev;

    num_instances++;
    ledGPIO_cdevs = krealloc(ledGPIO_cdevs, num_instances*sizeof(struct cdev *), GFP_KERNEL);
    if(!ledGPIO_cdevs){
        pr_err("Failed to reallocate memory for GPIO instances\n");
        return -ENOMEM;
    }
    //if the memory allocation/reallocation happenes successfully then we proceed
    ledGPIO_cdevs[num_instances-1] = kzalloc(sizeof(struct cdev *), GFP_KERNEL);
    //LETS CHECK IF THIS MEMORY ALLOCATION AND GETTING OF THE POINTER WAS SUCCESSFULLY DONE  STORED IN THE LEDGPIO[ARRAY POSITION OF THE INSTANCE -1 ]
    if(!ledGPIO_cdevs[num_instances-1])
    {
        pr_err("Failed to allocate memory for a particular cdev\n");
        return -ENOMEM;
    }

    cdev_init(ledGPIO_cdevs[num_instances-1], &fops); //initialize the character device
    //we makes sure that this instance is tied to this_mocule(driver) like below
    ledGPIO_cdevs[num_instances-1]->owner = THIS_MODULE;
    //ADD THE CHAR DEVICE TO THE KERNEL
    ret = cdev_add(ledGPIO_cdevs[num_instances-1], MKDEV(MAJOR(MAJOR_NUMBER),num_instances-1), 1);
     if (ret) {
        pr_err("Failed to add cdev for device %d\n", num_instances - 1);
        return ret;
    }
    // Create device node in /dev
     dev = device_create(ledGPIO_class,NULL,MKDEV(MAJOR(MAJOR_NUMBER), num_instances-1), NULL, "ledGPIO_dyna%d", num_instances - 1 );
     if(IS_ERR(dev)){
        pr_err("Failed to create device node\n");
        //CLEAN UP ON THE CDVE ADD AND cdev_init tasks above
        cdev_del(ledGPIO_cdevs[num_instances-1]);
        kfree(ledGPIO_cdevs[num_instances-1]);
        return PTR_ERR(dev);
     }

    pr_info("Registered GPIO device %d\n", num_instances - 1);
    return 0;
    //if we here all went well an the GPIO DEVIC E INSTANCE IS CREATED IN A DYNAMIC WAY.

}

static int __init moduleLEDGPIOInit(void){
  
  int retval; //this hold the state of initialization shoud return 0 if all is well
  if (led_gpio >= 0) {
        pr_info("Module parameter received: ledgpio=%d\n", led_gpio);
    } else {
        pr_warn("No GPIO pin specified for ledgpio\n");
    }
   retval = alloc_chrdev_region(&MAJOR_NUMBER, 0, INITIAL_INSTANCES, device_name);
  if(retval)
  {
    pr_alert("failed to allocate major number AND MINOR NUMBERS");
    return retval;
  }
  pr_alert("Allocated device numbers with major: %d\n", MAJOR(MAJOR_NUMBER));


  ledGPIO_class = class_create(THIS_MODULE, device_name); //tjis will ass a class to the name /sys/class/ledGPIO_class
  
  if(IS_ERR(ledGPIO_class))
  {
    //unregister the major number related to the device
    unregister_chrdev_region(MAJOR_NUMBER,INITIAL_INSTANCES);
    printk(KERN_ERR "UNABLE TO SETUT DEVICE CLASS!!\n");
     return PTR_ERR(ledGPIO_class); // Return error if allocation fails
  }
  pr_alert("LEDGPIO_CLASS success Major number = %d \n", MAJOR(MAJOR_NUMBER));

  //create an intial instance of the LEDGPIO DRIVER
  int i = 0;
   for (i = 0; i < INITIAL_INSTANCES; i++) {
        if(add_ledGPIO_instance() != 0){
            //!do some cleanup here if we are not able to make the instances as expected.
            //this means the device  is not able to be created so no need to call device_destroy() here
            //cdev_init() and cdev_add is cleaned with cdev_del()
            //their memory has already been freed kfree()
            //*we work on the remaining arguments in thie function to be cleaned
            
            device_destroy(ledGPIO_class, MKDEV(MAJOR(MAJOR_NUMBER), i));
            unregister_chrdev_region(MAJOR_NUMBER,INITIAL_INSTANCES);
            printk(KERN_ERR "UNABLE TO SETUT GPIO INSTANCE!!\n");
            return PTR_ERR(ledGPIO_class); // Return error if allocation fails

        }
    }
   //if all these pass
    printk(KERN_INFO "ledGPIO_instance registered with major number %d\n", MAJOR_NUMBER);
     
     //init gpio
     //ledGPIO_init();

 return 0;
}

static void __exit moduleLEDGPIOExit(void){
    // undo all previous operations
     del_timer_sync(&my_timer);
    //gpio_set_value(led_gpio, 0);  // Turn off the LED
    //gpio_free(led_gpio);
    int i = 0;
    for( i=0; i<num_instances; i++){
    device_destroy(ledGPIO_class, MKDEV(MAJOR(MAJOR_NUMBER), i));
    cdev_del(ledGPIO_cdevs[i]);
    kfree(ledGPIO_cdevs[i]);
    pr_info("Unregistered GPIO device %d\n", i);           
    
    }
    kfree(ledGPIO_cdevs); //FREE THIS SPACE THAT IS HOLDING A POINTER(this was a pointer to an array(basically array is a pinter to a memory space of specified size))

    class_destroy(ledGPIO_class);
    unregister_chrdev_region(MAJOR_NUMBER, num_instances);

    printk("all are removed\n");
    printk(KERN_ALERT "char Device file driver Gone bye\n");
    return;
}

module_init(moduleLEDGPIOInit);
module_exit(moduleLEDGPIOExit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("R-Mutura Macharia");
MODULE_DESCRIPTION("LED DEVICE DRIVER FOR EASY LED USES FROM USERSPACE DYNAMICALLY LOADABLE");
