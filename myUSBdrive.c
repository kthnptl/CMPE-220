#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define USB_DEMO_MINOR_BASE     100

static struct mutex usb_mutex;

//Structure for all device specific stuff:
struct usb_demo {
	struct usb_device    *udev;
	struct usb_interface *interface;
	unsigned char        *bulk_in_buffer;
	size_t               bulk_in_size;
	__u8                 bulk_in_endpoint_addr;
	__u8                 bulk_out_endpoint_addr;
	struct kref          kref;
};


static void usb_delete(struct kref *krf) {
	struct usb_demo *dev = container_of(krf , struct usb_demo, kref);

	usb_put_dev(dev->udev);
	kfree(dev->bulk_in_buffer);
	kfree(dev);

	return ;
}

static struct usb_driver my_usb_driver;

//Open Callback (file operation):
static int file_open(struct inode *inode, struct file *filp) {
	struct usb_demo      *dev;
	struct usb_interface *interface;
	int subminor;
	int retval = 0;

	subminor = iminor(inode);

	interface = usb_find_interface(&my_usb_driver, subminor);
	if(!interface) {
		printk(KERN_ALERT "CMPE 220: Cannot find interface information for minor: %d\n", subminor);
		retval = -ENODEV;
		goto exit;
	}

	dev = usb_get_intfdata(interface);
	if(!dev) {
		printk(KERN_ALERT "CMPE 220: Cannot find device for given interface\n");
		retval = -ENODEV;
		goto exit;
	}

	//Increment usage count:
	kref_get(&dev->kref);

	filp->private_data = dev;

exit:

	return retval;
}

//Close Callback (file operation):
static int file_close(struct inode *inode, struct file *filp) {
	struct usb_demo *dev = (struct usb_demo *) filp->private_data;

	if(dev == NULL) {
		return -ENODEV;
	}

	kref_put(&dev->kref, usb_delete);

	return 0;
}

//Read Callback (file operation):
//usb_bulk_msg:
static ssize_t file_read(struct file *filp, char __user *buffer, size_t count, loff_t *ppos) {
	struct usb_demo *dev = (struct usb_demo *) filp->private_data;
	int retval = 0;

	printk(KERN_ALERT "CMPE 220: usb read method called\n");

	//Blocking bulk read to get data from the device:
	retval = usb_bulk_msg(	dev->udev,
				usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpoint_addr),
				dev->bulk_in_buffer,
				min(dev->bulk_in_size, count),
				(int *)&count,
				(HZ * 10));

	//if read successfu:
	//copy the data to userspace:
	if(!retval) {
		if(copy_to_user(buffer, dev->bulk_in_buffer, count)) {
			retval = -EFAULT;
		} else {
			retval = count;
		}
	}

	return retval;
}

//After the urb is succefully transmitted to the USB Device:
//Or something happens in transmission:
//urb Callback is called by the USB Core:
//static void file_write_callback(struct urb *urb, struct pt_regs *regs) {
static void file_write_callback(struct urb *urb) {
	if(urb->status &&
	    !(urb->status == -ENOENT     ||
	       urb->status == -ECONNRESET ||
	       urb->status == -ESHUTDOWN
	    )
	 ) {
		printk(KERN_ALERT "CMPE 220: nonzero write bulk status received");
	    }

	usb_free_coherent(urb->dev,
			   urb->transfer_buffer_length,
			   urb->transfer_buffer,
			   urb->transfer_dma);

	return ;
}

//Write Callback (file operation):
static ssize_t file_write(struct file *filp, const char __user * buffer, size_t count, loff_t *ppos) {
	struct usb_demo *dev = filp->private_data;
	int             retval = 0;
	struct urb      *urb = NULL;
	char            *buf = NULL;

	printk(KERN_ALERT "CMPE 220: usb write method called\n");

	//if no data to write, exit:
	if(!count) {
		printk(KERN_ALERT "CMPE 220: no data to write\n");
		goto exit;
	}

	//Create a urb:
	//create buffer for it:
	//copy data to the urb:
	urb = usb_alloc_urb(0, GFP_KERNEL);
	if(!urb) {
		printk(KERN_ALERT "CMPE 220: Could not allocate urb\n");
		retval = -ENOMEM;
		goto error;
	}

	buf = usb_alloc_coherent(dev->udev, count, GFP_KERNEL, &urb->transfer_dma);
	if(!buf) {
		printk(KERN_ALERT "CMPE 220: Could not allocate buffer\n");
		retval = -ENOMEM;
		goto error;
	}

	if(copy_from_user(buf, buffer, count)) {
		printk(KERN_ALERT "CMPE 220: copy_from_user failed in file_write\n");
		retval = -EFAULT;
		goto error;
	}

	//Initialize the urb properly:
	usb_fill_bulk_urb(urb,
			   dev->udev,
			   usb_sndbulkpipe(dev->udev, dev->bulk_out_endpoint_addr),
			   buf,
			   count,
			   file_write_callback,
			   dev);

	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	//Send the data out the bulk port:
	retval = usb_submit_urb(urb, GFP_KERNEL);
	if(retval) {
		printk(KERN_ALERT "CMPE 220: usb_submit_urb failed\n");
		goto error;
	}

	usb_free_urb(urb);

exit:
	return count;

error:
	usb_free_coherent(dev->udev, count, buf, urb->transfer_dma);
	usb_free_urb(urb);
	kfree(buf);

	return retval;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open  = file_open,
	.release = file_close,
	.read  = file_read,
	.write = file_write,
};

static struct usb_class_driver demo_class = {
	.name = "usb/demo%d",
	.fops = &fops,
	.minor_base = USB_DEMO_MINOR_BASE,
};


/*------------------------------- #include<usb.h> -------------------------------*/
/* This function is called automatically when we insert an USB device provided this driver
*  is installed successfully. if this function is not being called one of the reasons can be that 
*  some other USB driver has access to USB bus and probe function of its driver is being called 
*  everytime when we insert USB. You can check which driver is working and which is being called 
*  by a debugging methode provided by Linux. Terminal command for this is dmesg. It lists out all
*  the debug messages printed out by different drivers. Try " dmesg | grep -i <your tag here> " 
*  command for filtering debug messages which has tag same as you mentioned in command. Thus, make  
*  sure you keep tag in your message. Anything before ":" in printk() is counted as tag.
*/
static int probe(struct usb_interface *interface,
			   const struct usb_device_id *id) {
	struct usb_demo                *dev = NULL;
	struct usb_host_interface      *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	size_t buffer_size;
	int    i;
	int    retval = -ENOMEM;
	struct usb_device *connected_device;

	//Allocate memory for our device and initialize:
	dev = kmalloc(sizeof(struct usb_demo), GFP_KERNEL);

	if(dev == NULL) {
		printk(KERN_ALERT "CMPE 220: Out Of Memory. Cannot allocate memory\n");
		goto error;
	}

	memset(dev, 0x00, sizeof(*dev));
	kref_init(&dev->kref);

	connected_device = interface_to_usbdev(interface);
	dev->udev = usb_get_dev(connected_device);
	dev->interface = interface;

	//Setup endpoint information:
	//first bulk-in, bulk-oou endpoints:
	iface_desc = interface->cur_altsetting;

	//Print some Interface information:
	printk(KERN_ALERT "CMPE-220: Name: %s \n",connected_device->product);
	printk(KERN_ALERT "CMPE-220: Manufacturer: %s\n",connected_device->manufacturer);
	printk(KERN_ALERT "CMPE-220: serial: %s",connected_device->serial);
	printk(KERN_ALERT "CMPE 220: USB interface %d probed\n", iface_desc->desc.bInterfaceNumber);
	printk(KERN_ALERT "CMPE 220: bNumEndpoints 0x%02x\n", iface_desc->desc.bNumEndpoints);
	printk(KERN_ALERT "CMPE 220: bInterfaceClass 0x%02x\n", iface_desc->desc.bInterfaceClass);

	for(i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
		endpoint = &iface_desc->endpoint[i].desc;

		//Print some Endpoint information:
		printk(KERN_ALERT "CMPE 220: Endpoint[%d]: bEndpointAddress 0x%02x\n", i, endpoint->bEndpointAddress);
		printk(KERN_ALERT "CMPE 220: Endpoint[%d]: bmAttributes     0x%02x\n", i, endpoint->bmAttributes);
		printk(KERN_ALERT "CMPE 220: Endpoint[%d]: wMaxPacketSize   0x%04x (%d)\n",
		       		  i, endpoint->wMaxPacketSize, endpoint->wMaxPacketSize);

		if(!dev->bulk_in_endpoint_addr &&
		    (endpoint->bEndpointAddress & USB_DIR_IN) &&
		    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
		      USB_ENDPOINT_XFER_BULK)) {
			//This case satisfies all conditions of
			//bulk in endpoint:

			buffer_size = endpoint->wMaxPacketSize;
			dev->bulk_in_size = buffer_size;
			dev->bulk_in_endpoint_addr = endpoint->bEndpointAddress;
			dev->bulk_in_buffer = kmalloc(buffer_size, GFP_KERNEL);

			if(!dev->bulk_in_buffer) {
				printk(KERN_ALERT "CMPE 220: Could not allocate bulk in buffer\n");
				goto error;
			}
			
		}

		if(!dev->bulk_out_endpoint_addr &&
		    !(endpoint->bEndpointAddress & USB_DIR_IN) &&
		    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
		    USB_ENDPOINT_XFER_BULK)) {
			//This case satisfies all conditions of
			//bulk out endpoint:

			dev->bulk_out_endpoint_addr = endpoint->bEndpointAddress;

		}
	}

	if(!dev->bulk_in_endpoint_addr && !dev->bulk_out_endpoint_addr) {
		printk(KERN_ALERT "CMPE 220: Could not find either bulk in endpoint or"
				  " bulk out endpoint\n");

		goto error;
	}

	//Save pointer to our data to this device:
	//Very good feature provided by USB Core;
	usb_set_intfdata(interface, dev);

	//Register the device to the USB Core:
	//Ready to be used:
	retval = usb_register_dev(interface, &demo_class);

	if(retval) {
		printk(KERN_ALERT "CMPE 220: Error registering Driver to the USB Core\n");
		printk(KERN_ALERT "CMPE 220: Not able to get minor for the Device\n");
		//Unset the pointer to our data on the device:
		usb_set_intfdata(interface, NULL);
		goto error;
	}

	printk(KERN_ALERT "CMPE 220: USB Device now attached to USBDemo-%d", interface->minor);
	printk(KERN_ALERT "CMPE 220: USB Device: Vendor ID: %04x, Product ID: %04x\n",
			 id->idVendor, id->idProduct);
	return 0;

error:

	if(dev) {
		kref_put(&dev->kref, usb_delete);
	}

	return retval;
}


/* This function is being called automatically when we remove an USB device. If probe function 
*  of this driver is working then this function will work for sure. 
*/
static void disconnected(struct usb_interface *interface) {
	struct usb_demo *dev;
	int minor = interface->minor;

	mutex_lock(&usb_mutex);

	dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);

	//give back minor:
	usb_deregister_dev(interface, &demo_class);

	mutex_unlock(&usb_mutex);

	kref_put(&dev->kref, usb_delete);

	printk(KERN_ALERT "CMPE 220: USB Demo #%d now disconnected", minor);
	
	return ;
}

/* This is the list of USB devices supported by this driver.
*  it is in pair of Vendor ID and Product ID of USB device.
*  The last {} blank entry is called termination entry. 
*/

static struct usb_device_id supported_device[] = {

		//{ USB_DEVICE(USB_DEMO_VENDOR_ID, USB_DEMO_PRODUCT_ID)  },
	{USB_DEVICE(0x054c,0x09c2)},	
	{USB_DEVICE(0x8564,0x1000)},
	{USB_DEVICE(0x0bc2,0xab26)},
	{}
};

MODULE_DEVICE_TABLE(usb, supported_device);


/* This is the main structure of device driver, where we register all our functions
*  to the default calling functions. 
*/
static struct usb_driver my_usb_driver = {
	.name = "my_usb_driver",
	.id_table = supported_device,
	.probe = probe,
	.disconnect = disconnected,
};


/* This is a function which plays role in installing the USB driver and registering 
*  it in kernel. To register it in kernel it uses the function given by Linux distro  
*  called usb_resigter(). This returns a value which indicates the result of registration.
*  It returns 0 on succsess and negative values on failure. 
*/

static int __init usb_drv_init(void) {
	int result;

	//Register driver with the USB Subsystem:
	result = usb_register(&my_usb_driver);

	//Init the usb_lock:
	mutex_init(&usb_mutex);

	if(result) {
		printk(KERN_ALERT "CMPE 220: [%s:%d]: usb_register failed\n",
		       __FUNCTION__, __LINE__);
	}

	return result;
}


/* This is a function which plays role in uninstalling the USB driver and deregistering 
*  it in kernel. To deregister it in kernel it uses the function given by Linux distro  
*  called usb_deresigter().
*/
static void __exit usb_drv_exit(void) {
	//Deregisters this driver from the USB Subsystem:
	usb_deregister(&my_usb_driver);
}


/*------------------------------- #include<module.h> -------------------------------*/
/* Here we are registering our driver-installation function to the function provided by
*  Linux distro called module_init(). Important to mention that in Linux drivers are called
*  modules. To see the list of modules running in background can be checked by command lsmod.
*  It shows list of all the modules working in background. Try lsmod | grep -i usb to filer 
*  modules with usb word in it.
*/
module_init(usb_drv_init);

/* Here we are registering our driver-installation function to the function provided by
*  Linux distro called module_exit(). It indicates that whenever user wants to uninstall
*  this driver, call pen_exit function.
*/
module_exit(usb_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("we 4");
MODULE_DESCRIPTION("writting our own USB device driver.");