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
	{ 0x6ac5d15e, "spi_unregister_device" },
	{ 0x86332725, "__stack_chk_fail" },
	{ 0x5e7355d9, "spi_write_then_read" },
	{ 0x806f7470, "spi_setup" },
	{ 0x506ec34a, "put_device" },
	{ 0xaedb2f74, "spi_new_device" },
	{ 0x27c40330, "spi_busnum_to_master" },
	{ 0xc5850110, "printk" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "75A45CFF7266500FBC9F148");
