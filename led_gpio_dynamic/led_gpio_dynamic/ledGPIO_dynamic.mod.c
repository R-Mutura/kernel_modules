#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x2e712c61, "module_layout" },
	{ 0x58aa99e6, "param_ops_int" },
	{ 0x729486cf, "class_destroy" },
	{ 0x239b261c, "class_remove_file_ns" },
	{ 0xc43b67c, "device_destroy" },
	{ 0x97934ecf, "del_timer_sync" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x94176683, "__class_create" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xeb9398df, "cdev_del" },
	{ 0x37a0cba, "kfree" },
	{ 0xe9b5efda, "device_create" },
	{ 0x24fc62bc, "cdev_add" },
	{ 0x61e59031, "cdev_init" },
	{ 0xfe990052, "gpio_free" },
	{ 0xb14a4af0, "gpiod_direction_output_raw" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xc358aaf8, "snprintf" },
	{ 0xbdd25cf8, "kmem_cache_alloc_trace" },
	{ 0x5b1e6398, "kmalloc_caches" },
	{ 0x8c8569cb, "kstrtoint" },
	{ 0x51a910c0, "arm_copy_to_user" },
	{ 0x97255bdf, "strlen" },
	{ 0x9904c93e, "gpiod_set_raw_value" },
	{ 0x45b62e8e, "gpio_to_desc" },
	{ 0x86332725, "__stack_chk_fail" },
	{ 0x5f754e5a, "memset" },
	{ 0x2cfde9a2, "warn_slowpath_fmt" },
	{ 0x1e6d26a8, "strstr" },
	{ 0xae353d77, "arm_copy_from_user" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xc5850110, "printk" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "778B7130C9AC363ACE40582");
