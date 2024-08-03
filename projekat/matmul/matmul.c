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

MODULE_AUTHOR ("FTN");
MODULE_DESCRIPTION("Test Driver for VGA controller IP.");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_ALIAS("custom:vga controller");

#define DEVICE_NAME "vga"
#define DRIVER_NAME "vga_driver"
#define BUFF_SIZE 20


//*******************FUNCTION PROTOTYPES************************************
static int vga_probe(struct platform_device *pdev);
static int vga_open(struct inode *i, struct file *f);
static int vga_close(struct inode *i, struct file *f);
static ssize_t vga_read(struct file *f, char __user *buf, size_t len, loff_t *off);
static ssize_t vga_write(struct file *f, const char __user *buf, size_t length, loff_t *off);
static int __init vga_init(void);
static void __exit vga_exit(void);
static int vga_remove(struct platform_device *pdev);

//*********************GLOBAL VARIABLES*************************************
struct vga_info {
  unsigned long mem_start;
  unsigned long mem_end;
  void __iomem *base_addr;
};

static struct cdev *my_cdev;
static dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static int int_cnt;
static struct vga_info *vp = NULL;

static struct file_operations my_fops =
  {
    .owner = THIS_MODULE,
    .open = vga_open,
    .release = vga_close,
    .read = vga_read,
    .write = vga_write
  };
static struct of_device_id vga_of_match[] = {
  { .compatible = "vga_bram_ctrl", },
  { /* end of list */ },
};

static struct platform_driver vga_driver = {
  .driver = {
    .name = DRIVER_NAME,
    .owner = THIS_MODULE,
    .of_match_table	= vga_of_match,
  },
  .probe		= vga_probe,
  .remove	= vga_remove,
};

MODULE_DEVICE_TABLE(of, vga_of_match);


//***************************************************************************
// PROBE AND REMOVE
static int vga_probe(struct platform_device *pdev)
{
  struct resource *r_mem;
  int rc = 0;
  int i = 0;

  printk(KERN_INFO "Probing\n");
  // Get phisical register adress space from device tree
  r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!r_mem) {
    printk(KERN_ALERT "vga_probe: Failed to get reg resource\n");
    return -ENODEV;
  }
  // Get memory for structure vga_info
  vp = (struct vga_info *) kmalloc(sizeof(struct vga_info), GFP_KERNEL);
  if (!vp) {
    printk(KERN_ALERT "vga_probe: Could not allocate memory for structure vga_info\n");
    return -ENOMEM;
  }
  // Put phisical adresses in vga_info structure
  vp->mem_start = r_mem->start;
  vp->mem_end = r_mem->end;
    
  // Reserve that memory space for this driver
  if (!request_mem_region(vp->mem_start,vp->mem_end - vp->mem_start + 1, DRIVER_NAME))
  {
    printk(KERN_ALERT "vga_probe: Could not lock memory region at %p\n",(void *)vp->mem_start);
    rc = -EBUSY;
    goto error1;
  }    
  // Remap phisical to virtual adresses

  vp->base_addr = ioremap(vp->mem_start, vp->mem_end - vp->mem_start + 1);
  if (!vp->base_addr) {
    printk(KERN_ALERT "vga_probe: Could not allocate memory\n");
    rc = -EIO;
    goto error2;
  }

  printk(KERN_NOTICE "vga_probe: VGA platform driver registered\n");
  return 0;//ALL OK
 error2:
  release_mem_region(vp->mem_start, vp->mem_end - vp->mem_start + 1);
 error1:
  return rc;

}

static int vga_remove(struct platform_device *pdev)
{
  int i = 0;
  // Exit Device Module
  for (i = 0; i < (256*144); i++) 
  { 
    iowrite32(0, vp->base_addr + i*4); 
  } 
  printk(KERN_INFO "VGA_remove: Vga remove in process");
  iounmap(vp->base_addr);
  release_mem_region(vp->mem_start, vp->mem_end - vp->mem_start + 1);
  kfree(vp);
  printk(KERN_INFO "VGA_remove: Vga driver removed");
  return 0;
}

//***************************************************
// IMPLEMENTATION OF FILE OPERATION FUNCTIONS
static int vga_open(struct inode *i, struct file *f)
{
  //printk(KERN_INFO "vga opened\n");
  return 0;
}

static int vga_close(struct inode *i, struct file *f)
{
  //printk(KERN_INFO "vga closed\n");
  return 0;
}

static ssize_t vga_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
  //printk("vga read\n");
  return 0;
}

static ssize_t vga_write(struct file *f, const char __user *buf, size_t length, loff_t *off)
{	
  char buff[BUFF_SIZE];
  int ret = 0;
  unsigned int xpos=0,ypos=0;
  unsigned long long rgb=0;
  unsigned char rgb_buff[10];  
  ret = copy_from_user(buff, buf, length);  
  if(ret){
    printk("copy from user failed \n");
    return -EFAULT;
  }  
  buff[length] = '\0';
  
  
  sscanf(buff,"%d,%d,%s", &xpos, &ypos, rgb_buff);  
  ret = kstrtoull(rgb_buff, 0, &rgb);
 
  if(ret != -EINVAL)//checking for parsing error
  {
    if (xpos > 255)
    {
      printk(KERN_WARNING "VGA_write: X_axis position exceeded, maximum is 255 and minimum 0 \n");
    }
    else if (ypos > 143)
    {
      printk(KERN_WARNING "VGA_write: Y_axis position exceeded, maximum is 143 and minimum 0 \n");
    }
    else
    {
      iowrite32(rgb, vp->base_addr + (256*ypos + xpos)*4);
    }
  }
  else
  {
    printk(KERN_WARNING "VGA_write: Wrong write format, expected \"xpos,ypos,rgb\"\n");
    // return -EINVAL;//parsing error
  }        
  return length;

}

//***************************************************
// HELPER FUNCTIONS (STRING TO INTEGER)


//***************************************************
// INIT AND EXIT FUNCTIONS OF THE DRIVER

static int __init vga_init(void)
{

  int ret = 0;
  int_cnt = 0;

  printk(KERN_INFO "vga_init: Initialize Module \"%s\"\n", DEVICE_NAME);
  ret = alloc_chrdev_region(&my_dev_id, 0, 1, "VGA_region");
  if (ret)
  {
    printk(KERN_ALERT "vga_init: Failed CHRDEV!.\n");
    return -1;
  }
  printk(KERN_INFO "vga_init: Successful allocation of CHRDEV!\n");
  my_class = class_create(THIS_MODULE, "VGA_drv");
  if (my_class == NULL)
  {
    printk(KERN_ALERT "vga_init: Failed class create!.\n");
    goto fail_0;
  }
  printk(KERN_INFO "vga_init: Successful class chardev create!.\n");
  my_device = device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id),0), NULL, "vga");
  if (my_device == NULL)
  {
    goto fail_1;
  }

  printk(KERN_INFO "vga_init: Device created.\n");

  my_cdev = cdev_alloc();	
  my_cdev->ops = &my_fops;
  my_cdev->owner = THIS_MODULE;
  ret = cdev_add(my_cdev, my_dev_id, 1);
  if (ret)
  {
    printk(KERN_ERR "VGA_init: Failed to add cdev\n");
    goto fail_2;
  }
  printk(KERN_INFO "VGA_init: Init Device \"%s\".\n");

  return platform_driver_register(&vga_driver);

 fail_2:
  device_destroy(my_class, MKDEV(MAJOR(my_dev_id),0));
 fail_1:
  class_destroy(my_class);
 fail_0:
  unregister_chrdev_region(my_dev_id, 1);
  return -1;

} 

static void __exit vga_exit(void)  		
{

  platform_driver_unregister(&vga_driver);
  cdev_del(my_cdev);
  device_destroy(my_class, MKDEV(MAJOR(my_dev_id),0));
  class_destroy(my_class);
  unregister_chrdev_region(my_dev_id, 1);
  printk(KERN_INFO "vga_exit: Exit Device Module \"%s\".\n", DEVICE_NAME);
}

module_init(vga_init);
module_exit(vga_exit);

