#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/access.h>


MODULE_LICENSE("GPL");

static int __init helloInit(void){
    printk(KERN_ALERT "Hello world Module initialized\n");
 
    return 0;
}

static void __exit hello_exit(void){
     printk(KERN_ALERT "ello world Module Gone bye\n");
}

module_init(helloInit);
module_exit(hello_exit);