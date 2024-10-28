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

MODULE_AUTHOR ("EE187/2019");
MODULE_DESCRIPTION("Driver for Matrix multiplication IP.");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_ALIAS("custom:matrix multiplication");

#define DEVICE_NAME "matmul"
#define DRIVER_NAME "matmul_driver"

#define BUFF_SIZE 128

//*******************FUNCTION PROTOTYPES************************************
static int matmul_probe(struct platform_device *pdev);
static int matmul_open(struct inode *i, struct file *f);
static int matmul_close(struct inode *i, struct file *f);
static ssize_t matmul_read(struct file *f, char __user *buf, size_t len, loff_t *off);
static ssize_t matmul_write(struct file *f, const char __user *buf, size_t length, loff_t *off);
static int __init matmul_init(void);
static void __exit matmul_exit(void);
static int matmul_remove(struct platform_device *pdev);

//*********************GLOBAL VARIABLES*************************************
struct matmul_info {
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;
};

static struct cdev *my_cdev;
static dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static int int_cnt;
static struct matmul_info *vp = NULL;

static struct file_operations my_fops =
	{
		.owner = THIS_MODULE,
		.open = matmul_open,
		.release = matmul_close,
		.read = matmul_read,
		.write = matmul_write
	};
static struct of_device_id matmul_of_match[] = {
	{ .compatible = "xlnx_matmul", },
	{ /* end of list */ },
};

static struct platform_driver matmul_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table	= matmul_of_match,
	},
	.probe		= matmul_probe,
	.remove	= matmul_remove,
};

MODULE_DEVICE_TABLE(of, matmul_of_match);


//***************************************************************************
// PROBE AND REMOVE
static int matmul_probe(struct platform_device *pdev)
{
	struct resource *r_mem;
	int rc = 0;
	int i = 0;

	printk(KERN_INFO "Probing\n");
	// Get phisical register adress space from device tree
	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r_mem) {
		printk(KERN_ALERT "matmul_probe: Failed to get reg resource\n");
		return -ENODEV;
	}
	// Get memory for structure matmul_info
	vp = (struct matmul_info *) kmalloc(sizeof(struct matmul_info), GFP_KERNEL);
	if (!vp) {
		printk(KERN_ALERT "matmul_probe: Could not allocate memory for structure matmul_info\n");
		return -ENOMEM;
	}
	// Put phisical adresses in matmul_info structure
	vp->mem_start = r_mem->start;
	vp->mem_end = r_mem->end;
		
	// Reserve that memory space for this driver
	if (!request_mem_region(vp->mem_start,vp->mem_end - vp->mem_start + 1, DRIVER_NAME))
	{
		printk(KERN_ALERT "matmul_probe: Could not lock memory region at %p\n",(void *)vp->mem_start);
		rc = -EBUSY;
		goto error1;
	}		
	// Remap phisical to virtual adresses

	vp->base_addr = ioremap(vp->mem_start, vp->mem_end - vp->mem_start + 1);
	if (!vp->base_addr) {
		printk(KERN_ALERT "matmul_probe: Could not allocate memory\n");
		rc = -EIO;
		goto error2;
	}

	printk(KERN_NOTICE "matmul_probe: matmul platform driver registered\n");
	return 0;//ALL OK
 error2:
	release_mem_region(vp->mem_start, vp->mem_end - vp->mem_start + 1);
 error1:
	return rc;

}

static int matmul_remove(struct platform_device *pdev)
{
	int i = 0;
	// Exit Device Module
	for (i = 0; i < (256*144); i++) 
	{ 
		iowrite32(0, vp->base_addr + i*4); 
	} 
	printk(KERN_INFO "matmul_remove: matmul remove in process");
	iounmap(vp->base_addr);
	release_mem_region(vp->mem_start, vp->mem_end - vp->mem_start + 1);
	kfree(vp);
	printk(KERN_INFO "matmul_remove: matmul driver removed");
	return 0;
}

//***************************************************
// IMPLEMENTATION OF FILE OPERATION FUNCTIONS
static int matmul_open(struct inode *i, struct file *f)
{
	//printk(KERN_INFO "matmul opened\n");
	return 0;
}

static int matmul_close(struct inode *i, struct file *f)
{
	//printk(KERN_INFO "matmul closed\n");
	return 0;
}

static ssize_t matmul_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{		
	static int endRead = 0;
	int ret;
	int len;
	
	unsigned int ready = 0;
	unsigned int start = 0;
	unsigned int n = 0;
	unsigned int m = 0;
	unsigned int p = 0;
	
	char buff[MATMUL_READ_LEN];
	
	if (endRead){
		endRead = 0;
		return 0;
	}

	ready = ioread32(vp->base_addr);
	start = ioread32(vp->base_addr+4);
	n = ioread32(vp->base_addr+8);
	m = ioread32(vp->base_addr+12);
	p = ioread32(vp->base_addr+16);

	len = sprintf(buff, "ready=%d;start=%d;n=%d;m=%d;p=%d\n", ready,start,n,m,p);
	
	ret = copy_to_user(buf, buff, len);
	if(ret)
		return -EFAULT;
	
	endRead = 1;

	return len;
}

static ssize_t matmul_write(struct file *f, const char __user *buf, size_t length, loff_t *off)
{	
	// Upisujemo jednu od dve stvari:
	// 1) Dimenzije matrica A i B u formatu dim=n,m,p
	// 2) Komanda start u formatu:
	//		a) start=1
	//		b) start=0
	//		c) start=trigger
	char buff[BUFF_SIZE];
	int ret = 0;
	int p1 = 0;
	int p2 = 0;
	int p3 = 0;
	
	ret = copy_from_user(buff, buf, length);	
	if(ret){
		printk("copy from user failed \n");
		return -EFAULT;
	}	
	buff[length] = '\0';
	
	ret = sscanf(buff, "dim=%d,%d,%d", &p1, &p2, &p3);	
	if (ret == 3) {
	
	}
	ret = sscanf(buff, "start=%d", &p1);	
	if (ret == 1) {
	
	}
 
	
	printk(KERN_ERR "Wrong write format, expected:\n");
	printk(KERN_ERR "    1) dim=n,m,p\n");
	printk(KERN_ERR "    2) start=0 or start=1\n");
	return -EINVAL; //parsing error
}

//***************************************************
// HELPER FUNCTIONS (STRING TO INTEGER)


//***************************************************
// INIT AND EXIT FUNCTIONS OF THE DRIVER

static int __init matmul_init(void)
{

	int ret = 0;
	int_cnt = 0;

	printk(KERN_INFO "matmul_init: Initialize Module \"%s\"\n", DEVICE_NAME);
	ret = alloc_chrdev_region(&my_dev_id, 0, 1, "matmul_region");
	if (ret)
	{
		printk(KERN_ALERT "matmul_init: Failed CHRDEV!.\n");
		return -1;
	}
	printk(KERN_INFO "matmul_init: Successful allocation of CHRDEV!\n");
	my_class = class_create(THIS_MODULE, "matmul_drv");
	if (my_class == NULL)
	{
		printk(KERN_ALERT "matmul_init: Failed class create!.\n");
		goto fail_0;
	}
	printk(KERN_INFO "matmul_init: Successful class chardev create!.\n");
	my_device = device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id),0), NULL, "matmul");
	if (my_device == NULL)
	{
		goto fail_1;
	}

	printk(KERN_INFO "matmul_init: Device created.\n");

	my_cdev = cdev_alloc();	
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;
	ret = cdev_add(my_cdev, my_dev_id, 1);
	if (ret)
	{
		printk(KERN_ERR "matmul_init: Failed to add cdev\n");
		goto fail_2;
	}
	printk(KERN_INFO "matmul_init: Init Device \"%s\".\n");

	return platform_driver_register(&matmul_driver);

 fail_2:
	device_destroy(my_class, MKDEV(MAJOR(my_dev_id),0));
 fail_1:
	class_destroy(my_class);
 fail_0:
	unregister_chrdev_region(my_dev_id, 1);
	return -1;

} 

static void __exit matmul_exit(void)			
{

	platform_driver_unregister(&matmul_driver);
	cdev_del(my_cdev);
	device_destroy(my_class, MKDEV(MAJOR(my_dev_id),0));
	class_destroy(my_class);
	unregister_chrdev_region(my_dev_id, 1);
	printk(KERN_INFO "matmul_exit: Exit Device Module \"%s\".\n", DEVICE_NAME);
}

module_init(matmul_init);
module_exit(matmul_exit);

