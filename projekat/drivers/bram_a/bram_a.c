#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/device.h>

#include <linux/io.h> //iowrite ioread
#include <linux/slab.h>//kmalloc kfree
#include <linux/platform_device.h>//platform driver
#include <linux/of.h>//of_match_table
#include <linux/ioport.h>//ioremap

#include "bram_common.h"

MODULE_AUTHOR ("EE187/2019");
MODULE_DESCRIPTION("Driver for BRAM A IP.");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_ALIAS("custom:bram a");

#define DEVICE_NAME "bram_a"
#define DRIVER_NAME "bram_a_driver"

#define BUFF_SIZE 256


//*******************FUNCTION PROTOTYPES************************************
static int bram_a_probe(struct platform_device *pdev);
static int bram_a_open(struct inode *i, struct file *f);
static int bram_a_close(struct inode *i, struct file *f);
static ssize_t bram_a_read(struct file *f, char __user *buf, size_t len, loff_t *off);
static ssize_t bram_a_write(struct file *f, const char __user *buf, size_t length, loff_t *off);
static int __init bram_a_init(void);
static void __exit bram_a_exit(void);
static int bram_a_remove(struct platform_device *pdev);

//*********************GLOBAL VARIABLES*************************************
struct bram_a_info {
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;
};

static struct cdev *my_cdev;
static dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static int int_cnt;
static struct bram_a_info *vp = NULL;

static struct file_operations my_fops =
	{
		.owner = THIS_MODULE,
		.open = bram_a_open,
		.release = bram_a_close,
		.read = bram_a_read,
		.write = bram_a_write
	};
static struct of_device_id bram_a_of_match[] = {
	{ .compatible = "xlnx_bram_a", },
	{ /* end of list */ },
};

static struct platform_driver bram_a_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table	= bram_a_of_match,
	},
	.probe		= bram_a_probe,
	.remove	= bram_a_remove,
};

MODULE_DEVICE_TABLE(of, bram_a_of_match);


//***************************************************************************
// PROBE AND REMOVE
static int bram_a_probe(struct platform_device *pdev)
{
	struct resource *r_mem;
	int rc = 0;
	int i = 0;

	printk(KERN_INFO "Probing\n");
	// Get phisical register adress space from device tree
	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r_mem) {
		printk(KERN_ALERT "bram_a_probe: Failed to get reg resource\n");
		return -ENODEV;
	}
	// Get memory for structure bram_a_info
	vp = (struct bram_a_info *) kmalloc(sizeof(struct bram_a_info), GFP_KERNEL);
	if (!vp) {
		printk(KERN_ALERT "bram_a_probe: Could not allocate memory for structure bram_a_info\n");
		return -ENOMEM;
	}
	// Put phisical adresses in bram_a_info structure
	vp->mem_start = r_mem->start;
	vp->mem_end = r_mem->end;
		
	// Reserve that memory space for this driver
	if (!request_mem_region(vp->mem_start,vp->mem_end - vp->mem_start + 1, DRIVER_NAME))
	{
		printk(KERN_ALERT "bram_a_probe: Could not lock memory region at %p\n",(void *)vp->mem_start);
		rc = -EBUSY;
		goto error1;
	}		
	// Remap phisical to virtual adresses

	vp->base_addr = ioremap(vp->mem_start, vp->mem_end - vp->mem_start + 1);
	if (!vp->base_addr) {
		printk(KERN_ALERT "bram_a_probe: Could not allocate memory\n");
		rc = -EIO;
		goto error2;
	}

	printk(KERN_NOTICE "bram_a_probe: bram_a platform driver registered\n");
	return 0;//ALL OK
 error2:
	release_mem_region(vp->mem_start, vp->mem_end - vp->mem_start + 1);
 error1:
	return rc;

}

static int bram_a_remove(struct platform_device *pdev)
{
	int i = 0;
	// Exit Device Module
	for (i = 0; i < (256*144); i++) 
	{ 
		iowrite32(0, vp->base_addr + i*4); 
	} 
	printk(KERN_INFO "bram_a_remove: bram_a remove in process");
	iounmap(vp->base_addr);
	release_mem_region(vp->mem_start, vp->mem_end - vp->mem_start + 1);
	kfree(vp);
	printk(KERN_INFO "bram_a_remove: bram_a driver removed");
	return 0;
}

//***************************************************
// IMPLEMENTATION OF FILE OPERATION FUNCTIONS
static int bram_a_open(struct inode *i, struct file *f)
{
	//printk(KERN_INFO "bram_a opened\n");
	return 0;
}

static int bram_a_close(struct inode *i, struct file *f)
{
	//printk(KERN_INFO "bram_a closed\n");
	return 0;
}

static ssize_t bram_a_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	//printk("bram_a read\n");
	return 0;
}

static ssize_t bram_a_write(struct file *f, const char __user *buf, size_t length, loff_t *off)
{
	char buff[BUFF_SIZE];
	int ret = 0;
	ret = copy_from_user(buff, buf, length);
	
	if(ret){
		printk("copy from user failed \n");
		return -EFAULT;
	}
	
	int first_row_length = 0;
	int current_row_length = 0;
	int current_number = 0;
	int mat_element_count = 0;
	
	for(int i=0; i < length-1; i++) {
		char e = buff[i];
		
		if(e == ',' || e == ';'){
			if(current_number > 4095){
				printk(KERN_ERR "Svi elementi matrice moraju biti brojevi izmedju 0 i 4095.\n");
				return -EFAULT;
			}
			else {
				int offset = mat_element_count*4;
				iowrite32(current_number, vp->base_addr + offset);
				
				current_number = 0;
				mat_element_count = mat_element_count+1;
				current_row_length = current_row_length+1;
			}
			
			if (e == ';'){
				if(first_row_length == 0){
					first_row_length = current_row_length;
				}
				else{
					if(current_row_length != first_row_length){
						printk(KERN_ERR "Format matrice je neispravan.\n");
						return -EFAULT;
					}
				}
				
				current_row_length = 0;
			}
		}
		else{
			if(e >= '0' && <= '9'){
				int current_digit = e - '0';
				current_number = current_number*10 + current_digit;
			}
			else {
				printk(KERN_ERR "Svi elementi matrice moraju biti brojevi izmedju 0 i 4095!\n");
				return -EFAULT;
			}
		}
	}
	
	return length;
}

//***************************************************
// INIT AND EXIT FUNCTIONS OF THE DRIVER

static int __init bram_a_init(void)
{

	int ret = 0;
	int_cnt = 0;

	printk(KERN_INFO "bram_a_init: Initialize Module \"%s\"\n", DEVICE_NAME);
	ret = alloc_chrdev_region(&my_dev_id, 0, 1, "bram_a_region");
	if (ret)
	{
		printk(KERN_ALERT "bram_a_init: Failed CHRDEV!.\n");
		return -1;
	}
	printk(KERN_INFO "bram_a_init: Successful allocation of CHRDEV!\n");
	my_class = class_create(THIS_MODULE, "bram_a_drv");
	if (my_class == NULL)
	{
		printk(KERN_ALERT "bram_a_init: Failed class create!.\n");
		goto fail_0;
	}
	printk(KERN_INFO "bram_a_init: Successful class chardev create!.\n");
	my_device = device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id),0), NULL, "bram_a");
	if (my_device == NULL)
	{
		goto fail_1;
	}

	printk(KERN_INFO "bram_a_init: Device created.\n");

	my_cdev = cdev_alloc();	
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;
	ret = cdev_add(my_cdev, my_dev_id, 1);
	if (ret)
	{
		printk(KERN_ERR "bram_a_init: Failed to add cdev\n");
		goto fail_2;
	}
	printk(KERN_INFO "bram_a_init: Init Device \"%s\".\n");

	return platform_driver_register(&bram_a_driver);

 fail_2:
	device_destroy(my_class, MKDEV(MAJOR(my_dev_id),0));
 fail_1:
	class_destroy(my_class);
 fail_0:
	unregister_chrdev_region(my_dev_id, 1);
	return -1;

} 

static void __exit bram_a_exit(void)			
{

	platform_driver_unregister(&bram_a_driver);
	cdev_del(my_cdev);
	device_destroy(my_class, MKDEV(MAJOR(my_dev_id),0));
	class_destroy(my_class);
	unregister_chrdev_region(my_dev_id, 1);
	printk(KERN_INFO "bram_a_exit: Exit Device Module \"%s\".\n", DEVICE_NAME);
}

module_init(bram_a_init);
module_exit(bram_a_exit);

