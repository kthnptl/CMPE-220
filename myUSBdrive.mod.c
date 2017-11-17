#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("usb:v054Cp09C2d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v8564p1000d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0BC2pAB26d*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "0033DDD64B7A1064972ABC7");
