#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>

// probe function
static int pen_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	printk(KERN_INFO " USB Drive (%04X:%04X) plugged\n", id->idVendor, id->idProduct);
	return 0;
}

// Disconnect function
static void pen_disconnect(struct usb_interface *interface)
{
	printk(KERN_INFO " USB Drive removed.");
}

// usb_device_id
static struct usb_device_id pen_table[] = 
{
	{USB_DEVICE(0x054c,0x09c2)},
	{}
};
MODULE_DEVICE_TABLE(usb,pen_table);

// usb_driver
static struct usb_driver pen_driver = 
{
	.name = "USB stick driver",
	.id_table = pen_table,		// usb_device_id
	.probe = pen_probe,
	.disconnect = pen_disconnect,
};

static int __init pen_init(void)
{
	int val = -1;
	printk(KERN_INFO "Installing my USB Driver.");
	printk(KERN_INFO "Registering driver in kernal");
	val = usb_register(&pen_driver);
	printk(KERN_INFO "Registration complete.");
	return val;
}

static void __exit pen_exit(void)
{
	printk(KERN_INFO "Uninstalling my USB driver");
	usb_deregister(&pen_driver);
	printk(KERN_INFO "Successfully Uninstalled.");
}

module_init(pen_init);
module_exit(pen_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("we 4");
MODULE_DESCRIPTION("writting our own USB device driver.");
