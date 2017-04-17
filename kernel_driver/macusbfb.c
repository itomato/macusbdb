/*
Framebuffer driver for the USB Macintosh display adapter
Copyright (C) 2010 Jeroen Domburg

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/fb.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <asm/atomic.h>

#define DRIVER_AUTHOR "Jeroen Domburg, jeroen@spritesmods.com"
#define DRIVER_DESC "Mac-display USB framebuffer driver"

#define VENDOR_ID	0x1234
#define PRODUCT_ID	0x55aa

#define MACFB_HEIGHT 342
#define MACFB_WIDTH 512

#define usb_buffer_alloc usb_alloc_coherent
#define usb_buffer_free usb_free_coherent

#define BUF_MAX_SIZE 32768

/* table of devices that work with this driver */
static struct usb_device_id id_table [] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{ },
};
MODULE_DEVICE_TABLE (usb, id_table);

struct macusbfbdev {
	struct usb_device *	udev;
	struct fb_info * fbinfo;
	unsigned char *fb;
	unsigned char *pixels;
	unsigned char *oldpixels;
	unsigned char *xferbuff;
	dma_addr_t xferbuffDmaAddr;
	struct usb_anchor submitted;
	int gpioVals;
	atomic_t writeInProgress;
	atomic_t changedWhileWriting;
};

static struct fb_fix_screeninfo macusbfb_fbfix __initdata = {
	.id = "macusbfb",
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_TRUECOLOR,
	.line_length = MACFB_WIDTH*2,
	.accel = FB_ACCEL_NONE,
};

static struct fb_var_screeninfo macusbfb_fbvar __initdata = {
	.xres = MACFB_WIDTH,
	.yres = MACFB_HEIGHT,
	.xres_virtual = MACFB_WIDTH,
	.yres_virtual = MACFB_HEIGHT,
	.bits_per_pixel = 16,
		.red = { 11, 5, 0 },
      	.green = { 4, 6, 0 },
      	.blue = { 0, 5, 0 },
	.left_margin = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.vmode = FB_VMODE_NONINTERLACED,
};



static void sendFramebufferOver(struct fb_info *info);

static void macusbfb_write_bulk_callback(struct urb *urb)
{

	struct macusbfbdev *dev;
	int status = urb->status;

	dev = urb->context;

	// sync/async unlink faults aren't errors 
	if (status &&
	    !(status == -ENOENT ||
	      status == -ECONNRESET ||
              status == -ESHUTDOWN)) {
		dbg("USBLCD: %s - nonzero write bulk status received: %d",
		    __func__, status);
	}

	/* free up our allocated buffer */
//	usb_buffer_free(urb->dev, BUF_MAX_SIZE,
//			urb->transfer_buffer, urb->transfer_dma);



	//Is the picture on the 'tube holy and perfect now?
	if (atomic_read(&dev->changedWhileWriting)!=0) {
		// *sigh,* something has changed. Let's do it again.
		atomic_set(&dev->changedWhileWriting, 0);
		sendFramebufferOver(dev->fbinfo);
//		printk(KERN_INFO "Macusbfb: USB transfer complete but frame has changed!!\n");
		return;
	} else {
//		printk(KERN_INFO "Macusbfb: USB transfer complete! Image is perfect now.\n");
	}

	atomic_set(&dev->writeInProgress, 0);


//	up(&dev->limit_sem);
}


static int createCmdStream(unsigned char* newbuff, unsigned char* oldbuff, unsigned char *data) {
	int p=0;
	int datalen=0;
	int x,y;
	int starting; //If we send data, will it be in a new packet?
	int sizePos; //pos of size indicator of packet
	int sizeCnt; //size of packet
	int addr;

	for (y=0; y<342; y++) {
		sizeCnt=0;
		starting=1;
		for (x=0; x<64; x++) {
			if (newbuff[p]==oldbuff[p]) {
				if (sizeCnt!=0) {
					data[sizePos]=sizeCnt;
					sizeCnt=0;
				}
				starting=1;
				p++;
			} else {
				oldbuff[p]=newbuff[p];
				if (starting) {
					addr=(y*64)+x;
//					printk(KERN_INFO "%i\n", addr);
					data[datalen++]=0xfa;
					data[datalen++]=(addr>>8);
					data[datalen++]=addr&0xff;
					sizePos=datalen;
					sizeCnt=0;
					data[datalen++]=1;
					starting=0;
				}
				data[datalen++]=newbuff[p++];
				sizeCnt++;
			}
		}
		if (sizeCnt!=0) {
			data[sizePos]=sizeCnt;
		}
	}
	return datalen;
}


static void convert16to1bit(unsigned char* in, unsigned char* out) {
	int x,y,p,tresh, val;
	unsigned char *outp=out;
	u16 *inp=(u16 *)in;
	for (y=0; y<342; y++) {
		for (x=0; x<512; x+=8) {
			for (p=0; p<8; p++) {
				//Tresh should start at 32 but seemingly my Xorg interprets
				//the 565-mode I pass to it as 555...
				tresh=16;
				if (p&1) tresh-=8;
				if (y&1) tresh+=4;
				val=(((*inp)>>5)&0x3F);
				*outp<<=1;
				if (val<tresh) *outp|=1;
				inp++;
			}
			outp++;
		}
	}
}

static void convert24to1bit(unsigned char* in, unsigned char* out) {
	int x,y,p,tresh;
	unsigned char *outp=out;
	unsigned char *inp=in+1; //point at g field
	for (y=0; y<342; y++) {
		for (x=0; x<512; x+=8) {
			for (p=0; p<8; p++) {
				tresh=128;
				if (p&1) tresh-=64;
				if (y&1) tresh+=32;
				*outp<<=1;
				if (*inp<tresh) *outp|=1;
				inp+=3;
			}
			outp++;
		}
	}
}


static void sendFramebufferOver(struct fb_info *info) {
	struct urb *usbreq;
	int retval;
	int count=0;
	struct macusbfbdev *dev=info->par;
	unsigned char *buf=dev->xferbuff;


	/* create a urb, and a buffer for it, and copy the data to the urb */
	usbreq = usb_alloc_urb(0, GFP_KERNEL);
	if (!usbreq) {
		printk(KERN_INFO "Macusbfb: Can't allocate urb!\n");
		goto err_no_buf;
	}


	convert16to1bit(dev->fb, dev->pixels);
	count=createCmdStream(dev->pixels, dev->oldpixels, buf);

	/* initialize the urb properly */
	usbreq->transfer_dma=dev->xferbuffDmaAddr;
	usb_fill_bulk_urb(usbreq, dev->udev,
			  usb_sndbulkpipe(dev->udev, 3),
			  buf, count, macusbfb_write_bulk_callback, dev);
	usbreq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	usb_anchor_urb(usbreq, &dev->submitted);
	
	/* send the data out the bulk port */
	retval = usb_submit_urb(usbreq, GFP_KERNEL);
	if (retval) {
		err("macusbfb: failed submitting write urb, error %d", retval);
		goto error_unanchor;
	}
	
	/* release our reference to this urb, the USB core will eventually free it entirely */
	usb_free_urb(usbreq);

	return;

error_unanchor:
	usb_unanchor_urb(usbreq);

	usb_buffer_free(dev->udev, count, buf, usbreq->transfer_dma);
	usb_free_urb(usbreq);
err_no_buf:
//	up(&dev->limit_sem);
	return;
}

static void macusbfb_dpy_deferred_io(struct fb_info *info, struct list_head *pagelist) {
	struct macusbfbdev *dev=info->par;

	if (atomic_read(&dev->writeInProgress)!=0) {
//		printk(KERN_INFO "Macusbfb: Deferred io handler called but write still in progress!\n");
		atomic_set(&dev->changedWhileWriting, 1);
		return;
	}
	atomic_set(&dev->writeInProgress, 1);
	sendFramebufferOver(info);
}

static void macusbfb_fillrect(struct fb_info *info,
				   const struct fb_fillrect *rect)
{
	struct macusbfbdev *dev = info->par;
	return;
	sys_fillrect(info, rect);
	if (atomic_read(&dev->writeInProgress)!=0) {
		atomic_set(&dev->changedWhileWriting, 1);
		return;
	}
	atomic_set(&dev->writeInProgress, 1);
	sendFramebufferOver(info);
}

static void macusbfb_copyarea(struct fb_info *info,
				   const struct fb_copyarea *area)
{
	struct macusbfbdev *dev = info->par;
	sys_copyarea(info, area);
	if (atomic_read(&dev->writeInProgress)!=0) {
		atomic_set(&dev->changedWhileWriting, 1);
		return;
	}
	atomic_set(&dev->writeInProgress, 1);
	sendFramebufferOver(info);
}

static void macusbfb_imageblit(struct fb_info *info,
				const struct fb_image *image)
{
	struct macusbfbdev *dev = info->par;
	sys_imageblit(info, image);
	if (atomic_read(&dev->writeInProgress)!=0) {
		atomic_set(&dev->changedWhileWriting, 1);
		return;
	}
	atomic_set(&dev->writeInProgress, 1);
	sendFramebufferOver(info);
}


static struct fb_deferred_io macusbfb_defio = {
	.delay		= HZ/30,
	.deferred_io	= macusbfb_dpy_deferred_io,
};

static struct fb_ops macusbfb_fbops = {
	.owner = THIS_MODULE,
	.fb_read = fb_sys_read,
	.fb_write = fb_sys_write,
	.fb_fillrect = macusbfb_fillrect,
	.fb_copyarea = macusbfb_copyarea,
	.fb_imageblit = macusbfb_imageblit,
};


#define MYDEV_ATTR_SIMPLE_UNSIGNED(name, gpiono)			\
static ssize_t show_attr_##name(struct device *dev, 		\
	struct device_attribute *attr, char *buf) 				\
{															\
	struct usb_interface *intf = to_usb_interface(dev);		\
	struct macusbfbdev *mydev = usb_get_intfdata(intf);		\
															\
	return sprintf(buf, "%u\n", mydev->gpioVals&(1<<gpiono)?1:0); \
}															\
															\
static ssize_t set_attr_##name(struct device *dev, 			\
	struct device_attribute *attr, const char *buf, size_t count) \
{															\
	struct usb_interface *intf = to_usb_interface(dev);		\
	struct macusbfbdev  *mydev = usb_get_intfdata(intf);	\
															\
	int val = simple_strtoul(buf, NULL, 10);				\
	setGpio(mydev, gpiono, val);							\
															\
	return count;											\
}															\
static DEVICE_ATTR(name, S_IWUGO | S_IRUGO, show_attr_##name, set_attr_##name);


static int setGpio(struct macusbfbdev *dev, int gpiono, int val) {
	unsigned char *buffer;
	buffer = kmalloc(8, GFP_KERNEL);
	if (!buffer) {
		dev_err(&dev->udev->dev, "out of memory\n");
		return -1;
	}
	
	if (val) dev->gpioVals|=(1<<gpiono); else dev->gpioVals&=!(1<<gpiono);
	//ToDo: This times out. See if that's the drivers or the firmwares problem.
	return usb_control_msg(dev->udev,
				usb_sndctrlpipe(dev->udev, 0),
				0x1, //1 -> modify gpio
				0x40, //vendor specific reqtype
				val,
				gpiono,
				buffer,	
				8,
				100);
}

MYDEV_ATTR_SIMPLE_UNSIGNED(fan, 0);
MYDEV_ATTR_SIMPLE_UNSIGNED(display, 1);
MYDEV_ATTR_SIMPLE_UNSIGNED(fddeject, 2);
MYDEV_ATTR_SIMPLE_UNSIGNED(hdled, 3);

static struct attribute *dev_attrs[] = {
	&dev_attr_fan.attr,
	&dev_attr_display.attr,
	&dev_attr_fddeject.attr,
	&dev_attr_hdled.attr,
	NULL
};

static struct attribute_group dev_attr_grp = {
	.attrs = dev_attrs,
};

static int macusbfb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
	struct usb_device *udev = interface_to_usbdev(interface);
	struct macusbfbdev *dev = NULL;
	int retval = -ENOMEM;
 	struct fb_info *info;

	dev = kzalloc(sizeof(struct macusbfbdev), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&interface->dev, "Out of memory\n");
		goto error_mem;
	}
	init_usb_anchor(&dev->submitted);

	//ToDo: oom handling

	dev->fb = vmalloc(roundup((MACFB_WIDTH*MACFB_HEIGHT)*2, PAGE_SIZE));
	if (!dev->fb) goto error_mem;
	dev->pixels = vmalloc((MACFB_WIDTH*MACFB_HEIGHT)/8);
	if (!dev->pixels) goto error_mem;
	dev->oldpixels = vmalloc((MACFB_WIDTH*MACFB_HEIGHT)/8);
	if (!dev->oldpixels) goto error_mem;

	dev->udev = usb_get_dev(udev);
	usb_set_intfdata (interface, dev);

	//Allocate fb stuff
	//The display is 1bit, but nothing is compatible with that so we'll emulate a 16-bit one.
	info = framebuffer_alloc(sizeof(struct macusbfbdev*), NULL);
	if (!info) goto error;
	dev->xferbuff=usb_buffer_alloc(dev->udev, BUF_MAX_SIZE, GFP_KERNEL, &dev->xferbuffDmaAddr);
	if (!dev->xferbuff) {
		printk(KERN_INFO "Macusbfb: Error alloccing xfer buff!\n");
		goto error2;
	}

	info->par=dev;
	info->screen_base = (char __force __iomem *) dev->fb;
	printk("Screen base = %x\n", (int)info->screen_base);
	info->screen_size = MACFB_WIDTH*MACFB_HEIGHT*2;
	info->fbops = &macusbfb_fbops;
	info->fix = macusbfb_fbfix;
	info->fix.smem_len=(MACFB_WIDTH*MACFB_HEIGHT)*2;
	info->var = macusbfb_fbvar;
	info->pseudo_palette = NULL;
	info->flags = FBINFO_FLAG_DEFAULT|FBINFO_VIRTFB;

	//Init deferred IO
	info->fbdefio = &macusbfb_defio;
	fb_deferred_io_init(info);

//Klont?!?
	dev->fbinfo=info;
	atomic_set(&dev->writeInProgress, 0);
	atomic_set(&dev->changedWhileWriting, 0);

	if (sysfs_create_group(&interface->dev.kobj, &dev_attr_grp)) goto error;

	if (register_framebuffer(info) < 0) goto fberr;
	printk(KERN_INFO "fb%d: %s frame buffer device\n", info->node, info->fix.id);
	dev_info(&interface->dev, "MacUsbFb device now attached\n");



	return 0;
//ToDo: look more closely at this: this doesn;t free everything.
fberr:
	framebuffer_release(info);
	printk(KERN_INFO "Macusbfb: Error initing fb!\n");

error2:

error:
	printk(KERN_INFO "Macusbfb: Error initing!\n");
	usb_set_intfdata (interface, NULL);
	usb_put_dev(dev->udev);
	kfree(dev);
error_mem:
	printk(KERN_INFO "Macusbfb: Couldn't allocate framebuffer or usb memory!\n");
	return retval;
}

static void macusbfb_disconnect(struct usb_interface *interface)
{
//ToDo: _Really_ clean everything up!
	struct macusbfbdev *dev;
 	struct fb_info *info;

	dev = usb_get_intfdata (interface);
	info=dev->fbinfo;

	if (info) {
		fb_deferred_io_cleanup(info);
		unregister_framebuffer(info);
		framebuffer_release(info);
	}

	sysfs_remove_group(&interface->dev.kobj, &dev_attr_grp);

	/* first remove the files, then set the pointer to NULL */
	usb_set_intfdata (interface, NULL);

	usb_put_dev(dev->udev);

	kfree(dev);

	dev_info(&interface->dev, "MacUsbFb now disconnected\n");
}

static struct usb_driver macusbfb_driver = {
	.name =		"macusbfb",
	.probe =	macusbfb_probe,
	.disconnect =	macusbfb_disconnect,
	.id_table =	id_table,
};

static int __init macusbfb_init(void)
{
	int retval = 0;

	retval = usb_register(&macusbfb_driver);
	if (retval)
		err("usb_register failed. Error number %d", retval);
	return retval;
}

static void __exit macusbfb_exit(void)
{
	usb_deregister(&macusbfb_driver);
}

module_init (macusbfb_init);
module_exit (macusbfb_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
