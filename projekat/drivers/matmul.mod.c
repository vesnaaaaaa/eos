#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
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
	{ 0x367fcc51, "module_layout" },
	{ 0x9bd226fc, "platform_driver_unregister" },
	{ 0x37a0cba, "kfree" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x7fbb56eb, "class_destroy" },
	{ 0x4ce2483a, "device_destroy" },
	{ 0xc6e25dc2, "cdev_del" },
	{ 0xf7795eed, "__platform_driver_register" },
	{ 0x6bfecf8d, "cdev_add" },
	{ 0xbe15ede5, "cdev_init" },
	{ 0xfff45d34, "device_create" },
	{ 0xec8c17d0, "__class_create" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xcbb0ae81, "kmem_cache_alloc_trace" },
	{ 0x58f94a7a, "kmalloc_caches" },
	{ 0x56470118, "__warn_printk" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0xa78af5f3, "ioread32" },
	{ 0xde80cd09, "ioremap" },
	{ 0x85bd1608, "__request_region" },
	{ 0xbf4a7700, "platform_get_resource" },
	{ 0x92997ed8, "_printk" },
	{ 0x1035c7c2, "__release_region" },
	{ 0x77358855, "iomem_resource" },
	{ 0xedc03953, "iounmap" },
	{ 0x4a453f53, "iowrite32" },
	{ 0x226a0d14, "device_match_fwnode" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xbdfb6dbb, "__fentry__" },
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

MODULE_INFO(srcversion, "EAF288F7AB6183FB6E730DB");
