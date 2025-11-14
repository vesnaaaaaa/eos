#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

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
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x92997ed8, "_printk" },
	{ 0x5f754e5a, "memset" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0xbb94430, "__class_create" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x314b20c8, "scnprintf" },
	{ 0x6c4c7ca5, "device_create" },
	{ 0x817e3c4f, "class_destroy" },
	{ 0xe12cc958, "cdev_alloc" },
	{ 0xb6c4848c, "cdev_add" },
	{ 0x758d4816, "device_destroy" },
	{ 0xc13ee17, "__platform_driver_register" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xedc03953, "iounmap" },
	{ 0x4384eb42, "__release_region" },
	{ 0x37a0cba, "kfree" },
	{ 0xc94d8e3b, "iomem_resource" },
	{ 0x53a2254b, "platform_get_resource" },
	{ 0xaa42a42f, "kmalloc_trace" },
	{ 0xae9849dd, "__request_region" },
	{ 0xe97c4103, "ioremap" },
	{ 0x56e838a0, "kmalloc_caches" },
	{ 0x7d3f43d5, "platform_driver_unregister" },
	{ 0x4bcbe5aa, "cdev_del" },
	{ 0x69d6813f, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cxlnx_bram_a");
MODULE_ALIAS("of:N*T*Cxlnx_bram_aC*");
MODULE_ALIAS("of:N*T*Cxlnx_bram_b");
MODULE_ALIAS("of:N*T*Cxlnx_bram_bC*");
MODULE_ALIAS("of:N*T*Cxlnx_bram_c");
MODULE_ALIAS("of:N*T*Cxlnx_bram_cC*");
MODULE_ALIAS("of:N*T*Cxlnx_matmul");
MODULE_ALIAS("of:N*T*Cxlnx_matmulC*");
