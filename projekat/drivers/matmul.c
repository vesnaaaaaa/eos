
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>

MODULE_AUTHOR("EE187/2019");
MODULE_DESCRIPTION("Driver for Matrix Multiplication System");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_ALIAS("custom:matmul");

#define DRIVER_NAME "matmul_driver"
#define NUM_DEVICES 4

#define BUFF_SIZE 128
#define BRAM_LINE_SIZE 7
#define BRAM_SIZE (BRAM_LINE_SIZE * BRAM_LINE_SIZE)

// Structure to hold device-specific info
struct device_info {
    const char* name;
    unsigned long mem_start;
    unsigned long mem_end;
    void __iomem* base_addr;
    struct cdev cdev;
    dev_t dev_id;
    struct device* device;
};

// Global variables
static struct class* matmul_class;
static struct device_info* devices;
static dev_t first_dev_id;

// Function prototypes for each device type
static ssize_t bram_a_write(struct file* f, const char __user* buf, size_t length, loff_t* off);
static ssize_t bram_b_write(struct file* f, const char __user* buf, size_t length, loff_t* off);
static ssize_t bram_c_read(struct file* f, char __user* buf, size_t len, loff_t* off);
static ssize_t matmul_read(struct file* f, char __user* buf, size_t len, loff_t* off);
static ssize_t matmul_write(struct file* f, const char __user* buf, size_t length, loff_t* off);

// Common open/close operations
static int device_open(struct inode* i, struct file* f)
{
    return 0;
}

static int device_close(struct inode* i, struct file* f)
{
    return 0;
}

// File operations for each device type
static struct file_operations bram_a_fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_close,
    .write = bram_a_write,
    .read = NULL
};

static struct file_operations bram_b_fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_close,
    .write = bram_b_write,
    .read = NULL
};

static struct file_operations bram_c_fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_close,
    .read = bram_c_read,
    .write = NULL
};

static struct file_operations matmul_fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_close,
    .read = matmul_read,
    .write = matmul_write
};

// Platform driver matching
static struct of_device_id matmul_of_match[] = {
    { .compatible = "xlnx_bram_a" },
    { .compatible = "xlnx_bram_b" },
    { .compatible = "xlnx_bram_c" },
    { .compatible = "xlnx_matmul" },
    { /* end of list */ },
};
MODULE_DEVICE_TABLE(of, matmul_of_match);

// Implementation of device-specific operations
static ssize_t bram_a_write(struct file* f, const char __user* buf, size_t length, loff_t* off)
{
    char buff[BUFF_SIZE];
    int ret = 0;
    struct device_info* dev_info = &devices[0]; // BRAM A is device 0


	int first_row_length = 0;
    int current_row_length = 0;
    int current_number = 0;
    int mat_element_count = 0;

    ret = copy_from_user(buff, buf, length);
	
    if (ret) {
        printk("copy from user failed \n");
        return -EFAULT;
    }


    for (int i = 0; i < length - 1; i++) {
        char e = buff[i];

        if (e == ',' || e == ';') {
            if (current_number > 4095) {
                printk(KERN_ERR "All matrix elements must be between 0 and 4095.\n");
                return -EFAULT;
            } else {
                int offset = mat_element_count * 4;
                iowrite32(current_number, dev_info->base_addr + offset);

                current_number = 0;
                mat_element_count = mat_element_count + 1;
                current_row_length = current_row_length + 1;
            }

            if (e == ';') {
                if (first_row_length == 0) {
                    first_row_length = current_row_length;
                } else {
                    if (current_row_length != first_row_length) {
                        printk(KERN_ERR "Invalid matrix format.\n");
                        return -EFAULT;
                    }
                }

                current_row_length = 0;
            }
        } else {
            if (e >= '0' && e <= '9') {
                int current_digit = e - '0';
                current_number = current_number * 10 + current_digit;
            } else {
                printk(KERN_ERR "All matrix elements must be numbers between 0 and 4095!\n");
                return -EFAULT;
            }
        }
    }

    return length;
}

static ssize_t bram_b_write(struct file* f, const char __user* buf, size_t length, loff_t* off)
{
    char buff[BUFF_SIZE];
    int ret = 0;
    struct device_info* dev_info = &devices[1]; // BRAM B is device 1

	int first_row_length = 0;
    int current_row_length = 0;
    int current_number = 0;
    int mat_element_count = 0;

    ret = copy_from_user(buff, buf, length);
    if (ret) {
        printk("copy from user failed \n");
        return -EFAULT;
    }

   

    for (int i = 0; i < length - 1; i++) {
        char e = buff[i];

        if (e == ',' || e == ';') {
            if (current_number > 4095) {
                printk(KERN_ERR "All matrix elements must be between 0 and 4095.\n");
                return -EFAULT;
            } else {
                int offset = mat_element_count * 4;
                iowrite32(current_number, dev_info->base_addr + offset);

                current_number = 0;
                mat_element_count = mat_element_count + 1;
                current_row_length = current_row_length + 1;
            }

            if (e == ';') {
                if (first_row_length == 0) {
                    first_row_length = current_row_length;
                } else {
                    if (current_row_length != first_row_length) {
                        printk(KERN_ERR "Invalid matrix format.\n");
                        return -EFAULT;
                    }
                }

                current_row_length = 0;
            }
        } else {
            if (e >= '0' && e <= '9') {
                int current_digit = e - '0';
                current_number = current_number * 10 + current_digit;
            } else {
                printk(KERN_ERR "All matrix elements must be numbers between 0 and 4095!\n");
                return -EFAULT;
            }
        }
    }

    return length;
}

static ssize_t bram_c_read(struct file* f, char __user* buf, size_t len, loff_t* off)
{
    static int endRead = 0;
    int ret;
    char buff[BUFF_SIZE];
    struct device_info* dev_info = &devices[2]; // BRAM C is device 2

    if (endRead) {
        endRead = 0;
        return 0;
    }

    for (int i = 0; i < 49; i++) {
        buff[i] = ioread32(dev_info->base_addr + i * 4) + '0';
    }
    buff[50] = '\0';

    len = 64;
    ret = copy_to_user(buf, buff, len);
    if (ret)
        return -EFAULT;

    endRead = 1;
    return len;
}

static ssize_t matmul_read(struct file* f, char __user* buf, size_t len, loff_t* off)
{
    static int endRead = 0;
    int ret;
    struct device_info* dev_info = &devices[3]; // MatMul is device 3
    char buff[BUFF_SIZE];

	unsigned int ready = ioread32(dev_info->base_addr);
    unsigned int start = ioread32(dev_info->base_addr + 4);
    unsigned int n = ioread32(dev_info->base_addr + 8);
    unsigned int m = ioread32(dev_info->base_addr + 12);
    unsigned int p = ioread32(dev_info->base_addr + 16);

    if (endRead == 1) {
        endRead = 0;
        return 0;
    }


    len = sprintf(buff, "ready=%d;start=%d;n=%d;m=%d;p=%d\n", ready, start, n, m, p);
    ret = copy_to_user(buf, buff, len);

    if (ret)
        return -EFAULT;

    endRead = 1;
    return len;
}

static ssize_t matmul_write(struct file* f, const char __user* buf, size_t length, loff_t* off)
{
    char buff[BUFF_SIZE];
    int ret = 0;
    int p1 = 0;
    int p2 = 0;
    int p3 = 0;
    struct device_info* dev_info = &devices[3]; // MatMul is device 3

    ret = copy_from_user(buff, buf, length);
    if (ret) {
        printk("copy from user failed \n");
        return -EFAULT;
    }
    buff[length] = '\0';

    ret = sscanf(buff, "dim=%d,%d,%d", &p1, &p2, &p3);
    if (ret == 3) {
        iowrite32(p1, dev_info->base_addr + 8);
        iowrite32(p2, dev_info->base_addr + 12);
        iowrite32(p3, dev_info->base_addr + 16);
        return length;
    }

    ret = sscanf(buff, "start=%d", &p1);
    if (ret == 1) {
        iowrite32(p1, dev_info->base_addr + 4);
        return length;
    }

    printk(KERN_ERR "Wrong write format, expected:\n");
    printk(KERN_ERR "    1) dim=n,m,p\n");
    printk(KERN_ERR "    2) start=0 or start=1\n");
    return -EINVAL;
}

// Probe function
static int matmul_probe(struct platform_device* pdev)
{
    struct resource* r_mem;
    int rc = 0;
    //const char* device_name;
    int device_index;

    // Determine which device we're probing based on compatible string
    if (device_match_fwnode(&pdev->dev, &matmul_of_match[0]))
        device_index = 0; // BRAM A
    else if (device_match_fwnode(&pdev->dev, &matmul_of_match[1]))
        device_index = 1; // BRAM B
    else if (device_match_fwnode(&pdev->dev, &matmul_of_match[2]))
        device_index = 2; // BRAM C
    else if (device_match_fwnode(&pdev->dev, &matmul_of_match[3]))
        device_index = 3; // MatMul
    else
        return -ENODEV;

    // Get physical register address space
    r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!r_mem) {
        printk(KERN_ALERT "Failed to get reg resource for device %d\n", device_index);
        return -ENODEV;
    }

    // Store physical addresses
    devices[device_index].mem_start = r_mem->start;
    devices[device_index].mem_end = r_mem->end;

    // Reserve memory region
    if (!request_mem_region(devices[device_index].mem_start,
            devices[device_index].mem_end - devices[device_index].mem_start + 1,
            DRIVER_NAME)) {
        printk(KERN_ALERT "Could not lock memory region at %p\n",
            (void*)devices[device_index].mem_start);
        rc = -EBUSY;
        goto error1;
    }

    // Remap physical to virtual addresses
    devices[device_index].base_addr = ioremap(devices[device_index].mem_start,
        devices[device_index].mem_end - devices[device_index].mem_start + 1);
    if (!devices[device_index].base_addr) {
        printk(KERN_ALERT "Could not allocate memory for device %d\n", device_index);
        rc = -EIO;
        goto error2;
    }

    printk(KERN_NOTICE "Device %d platform driver registered\n", device_index);
    return 0;

error2:
    release_mem_region(devices[device_index].mem_start,
        devices[device_index].mem_end - devices[device_index].mem_start + 1);
error1:
    return rc;
}

static int matmul_remove(struct platform_device* pdev)
{
    int device_index;

    // Determine which device we're removing
    if (device_match_fwnode(&pdev->dev, &matmul_of_match[0]))
        device_index = 0;
    else if (device_match_fwnode(&pdev->dev, &matmul_of_match[1]))
        device_index = 1;
else if (device_match_fwnode(&pdev->dev, &matmul_of_match[2]))
        device_index = 2;
    else if (device_match_fwnode(&pdev->dev, &matmul_of_match[3]))
        device_index = 3;
    else
        return -ENODEV;

    // Clear memory
    for (int i = 0; i < (256 * 144); i++) {
        iowrite32(0, devices[device_index].base_addr + i * 4);
    }

    iounmap(devices[device_index].base_addr);
    release_mem_region(devices[device_index].mem_start,
        devices[device_index].mem_end - devices[device_index].mem_start + 1);

    printk(KERN_INFO "Device %d driver removed\n", device_index);
    return 0;
}

static struct platform_driver matmul_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = matmul_of_match,
    },
    .probe = matmul_probe,
    .remove = matmul_remove,
};

// Init and exit functions
static int __init matmul_init(void)
{
    int ret = 0;

    // Allocate memory for device structures
    devices = kmalloc(NUM_DEVICES * sizeof(struct device_info), GFP_KERNEL);
    if (!devices) {
        printk(KERN_ALERT "Failed to allocate memory for devices\n");
        return -ENOMEM;
    }

    // Initialize device names
    devices[0].name = "bram_a";
    devices[1].name = "bram_b";
    devices[2].name = "bram_c";
    devices[3].name = "matmul";

    // Allocate device numbers
    ret = alloc_chrdev_region(&first_dev_id, 0, NUM_DEVICES, "matmul_region");
    if (ret) {
        printk(KERN_ALERT "Failed to allocate char device region\n");
        goto fail_chrdev;
    }

    // Create device class
    matmul_class = class_create(THIS_MODULE, "matmul_class");
    if (IS_ERR(matmul_class)) {
        printk(KERN_ALERT "Failed to create device class\n");
        ret = PTR_ERR(matmul_class);
        goto fail_class;
    }

    // Initialize each device
    for (int i = 0; i < NUM_DEVICES; i++) {
        devices[i].dev_id = MKDEV(MAJOR(first_dev_id), i);

        // Create device node
        devices[i].device = device_create(matmul_class, NULL, devices[i].dev_id,
            NULL, devices[i].name);
        if (IS_ERR(devices[i].device)) {
            printk(KERN_ALERT "Failed to create device %d\n", i);
            ret = PTR_ERR(devices[i].device);
            goto fail_device;
        }

        // Initialize the character device
		
		int matmul_init(void) {
			
		int ret;
		struct file_operations fops = {};		//inicijalizuje se prazna struktura
		int i;
		
		
		for (i = 0; i < NUM_DEVICES; i++) {
        if (i == 0) {
            fops = bram_a_fops;
        } else if (i == 1) {
            fops = bram_b_fops;
        } else if (i == 2) {
            fops = bram_c_fops;
        } else {
            fops = matmul_fops;
        }
	

		cdev_init(&devices[i].cdev, &fops);
        devices[i].cdev.owner = THIS_MODULE; 

        // Add the character device to the system
        ret = cdev_add(&devices[i].cdev, devices[i].dev_id, 1);
        if (ret) {
            printk(KERN_ALERT "Failed to add char device %d\n", i);
            goto fail_cdev_init;
        }
		
    }
	
	return 0;
	
	fail_cdev_init:
    // Čišćenje u slučaju greške
    for (int j = 0; j < i; j++) {
        cdev_del(&devices[j].cdev);
    }
    return ret;
}

    // Register platform driver
	
	int matmul_driver_init(void) {
    int ret;
	
    ret = platform_driver_register(&matmul_driver);
    if (ret) {
        printk(KERN_ALERT "Failed to register platform driver\n");
        goto fail_platform;
    }

    printk(KERN_INFO "Matrix multiplication driver initialized\n");
    return 0;

fail_platform:
    // Clean up cdevs
    for (int i = 0; i < NUM_DEVICES; i++) {
        cdev_del(&devices[i].cdev);
    }
fail_cdev:
    // Clean up devices
    for (int i = 0; i < NUM_DEVICES; i++) {
        if (!IS_ERR_OR_NULL(devices[i].device)) {
            device_destroy(matmul_class, devices[i].dev_id);
        }
    }
fail_device:
    class_destroy(matmul_class);
fail_class:
    unregister_chrdev_region(first_dev_id, NUM_DEVICES);
fail_chrdev:
    kfree(devices);
    return ret;
}

	
	void matmul_exit(void);
{
    // Unregister platform driver
    platform_driver_unregister(&matmul_driver);

    // Clean up each device
    for (int i = 0; i < NUM_DEVICES; i++) {
        cdev_del(&devices[i].cdev);
        device_destroy(matmul_class, devices[i].dev_id);
    }

    // Clean up class and device numbers
    class_destroy(matmul_class);
    unregister_chrdev_region(first_dev_id, NUM_DEVICES);

    // Free allocated memory
    kfree(devices);

    printk(KERN_INFO "Matrix multiplication driver removed\n");
}

module_init(matmul_init);
module_exit(matmul_exit);
