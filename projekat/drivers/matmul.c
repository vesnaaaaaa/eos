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
#define DEVICE_NAME "matmul"
#define NUM_DEVICES 4

#define BUFF_SIZE 128
#define BRAM_LINE_SIZE 7
#define MATMUL_REG_SIZE_BYTES 20
#define BRAM_SIZE_BYTES (4 * BRAM_LINE_SIZE * BRAM_LINE_SIZE)

#define MATMUL_ADDR_OFFSET 0
#define BRAM_A_ADDR_OFFSET (MATMUL_ADDR_OFFSET + MATMUL_REG_SIZE_BYTES)
#define BRAM_B_ADDR_OFFSET (BRAM_A_ADDR_OFFSET + BRAM_SIZE_BYTES)
#define BRAM_C_ADDR_OFFSET (BRAM_B_ADDR_OFFSET + BRAM_SIZE_BYTES)

enum Device {
  MATMUL = 0,
  BRAM_A = 1,
  BRAM_B = 2,
  BRAM_C = 3,
};

// Structure to hold device-specific info
struct matmul_info {
  unsigned long mem_start;
  unsigned long mem_end;
  void __iomem *base_addr;
};

struct matrix_dims {
    int m;
    int p;
    int ready;
};

// Global variables
static struct class *matmul_class;
static struct device *matmul_device;
static struct cdev *matmul_cdev;
static struct matmul_info *dev_info;
static dev_t first_dev_id;

static struct matrix_dims mat_dims;

static int device_open(struct inode *i, struct file *f);
static int device_close(struct inode *i, struct file *f);
static int matmul_remove(struct platform_device *pdev);
static int matmul_probe(struct platform_device *pdev);
static int __init matmul_init(void);
static void __exit matmul_exit(void);

static ssize_t matmul_write(struct file *f, const char __user *buf,
                            size_t length, loff_t *off);
static ssize_t matmul_read(struct file *f, char __user *buf, size_t len,
                           loff_t *off);

// Common open/close operations
static int device_open(struct inode *i, struct file *f) { return 0; }
static int device_close(struct inode *i, struct file *f) { return 0; }

// File operations
static struct file_operations matmul_fops = {.owner = THIS_MODULE,
                                             .open = device_open,
                                             .release = device_close,
                                             .write = matmul_write,
                                             .read = matmul_read};

// Platform driver matching
static struct of_device_id matmul_of_match[] = {
    {.compatible = "xlnx_bram_a"}, {.compatible = "xlnx_bram_b"},
    {.compatible = "xlnx_bram_c"}, {.compatible = "xlnx_matmul"},
    {/* end of list */},
};
MODULE_DEVICE_TABLE(of, matmul_of_match);

// Implementation of device-specific operations
static ssize_t bram_a_write(const char __user *buf, size_t length) {
  char buff[BUFF_SIZE];
  int ret = 0;
  void __iomem *bram_a_base_addr = dev_info->base_addr + BRAM_A_ADDR_OFFSET;

  ret = copy_from_user(buff, buf, length);
  if (ret) {
    printk("copy from user failed \n");
    return -EFAULT;
  }

  int first_row_length = 0;
  int current_row_length = 0;
  int current_number = 0;
  int mat_element_count = 0;

  for (int i = 0; i < length - 1; i++) {
    char e = buff[i];

    if (e == ',' || e == ';') {
      if (current_number > 4095) {
        printk(KERN_ERR "All matrix elements must be between 0 and 4095.\n");
        return -EFAULT;
      } else {
	if (first_row_length > 0) {
	    int current_col = mat_element_count / first_row_length + 1;
	    if (current_col > BRAM_LINE_SIZE) {
		printk(KERN_ERR "Matrix dimensions too large (maximum allowed size 7x7).\n");
		return -EFAULT;
	    }
	}
	if (current_row_length >= BRAM_LINE_SIZE) {
            printk(KERN_ERR "Matrix dimensions too large (maximum allowed size 7x7).\n");
            return -EFAULT;
	}

        int offset = mat_element_count * 4;
        iowrite32(current_number, bram_a_base_addr + offset);

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
        printk(KERN_ERR
               "All matrix elements must be numbers between 0 and 4095!\n");
        return -EFAULT;
      }
    }
  }

  return length;
}

static ssize_t bram_b_write(const char __user *buf, size_t length) {
  char buff[BUFF_SIZE];
  int ret = 0;
  void __iomem *bram_b_base_addr = dev_info->base_addr + BRAM_B_ADDR_OFFSET;

  ret = copy_from_user(buff, buf, length);
  if (ret) {
    printk("copy from user failed \n");
    return -EFAULT;
  }

  int first_row_length = 0;
  int current_row_length = 0;
  int current_number = 0;
  int mat_element_count = 0;

  for (int i = 0; i < length - 1; i++) {
    char e = buff[i];

    if (e == ',' || e == ';') {
      if (current_number > 4095) {
        printk(KERN_ERR "All matrix elements must be between 0 and 4095.\n");
        return -EFAULT;
      } else {
	if (first_row_length > 0) {
	    int current_col = mat_element_count / first_row_length + 1;
	    if (current_col > BRAM_LINE_SIZE) {
		printk(KERN_ERR "Matrix dimensions too large (maximum allowed size 7x7).\n");
		return -EFAULT;
	    }
	}
	if (current_row_length >= BRAM_LINE_SIZE) {
            printk(KERN_ERR "Matrix dimensions too large (maximum allowed size 7x7).\n");
            return -EFAULT;
	}

        int offset = mat_element_count * 4;
        iowrite32(current_number, bram_b_base_addr + offset);

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
        printk(KERN_ERR
               "All matrix elements must be numbers between 0 and 4095!\n");
        return -EFAULT;
      }
    }
  }

  return length;
}

static ssize_t bram_c_read(char __user *buf, size_t len) {
  static int endRead = 0;
  int ret;
  int row;
  int col;
  int bram_pos;
  int mat_pos;
  char c;
  char buff[BUFF_SIZE];
  void __iomem *bram_c_base_addr = dev_info->base_addr + BRAM_C_ADDR_OFFSET;

  if (endRead) {
    endRead = 0;
    return 0;
  }

  if (mat_dims.ready == 1) {
      mat_pos = 0;
      for (row = 0; row < mat_dims.m; row++) {
	  for (col = 0; col < mat_dims.p; col++) {
              bram_pos = row * mat_dims.m + col;
              c = ((char)ioread32(bram_c_base_addr + bram_pos * 4)) + '0';
	      buff[mat_pos] = c;
	      mat_pos += 1;
	      if (col == (mat_dims.p - 1)) {
		  buff[mat_pos] = ';';
	      } else {
		  buff[mat_pos] = ',';
	      }
	      mat_pos += 1;
	  }
      }
      buff[mat_pos + 1] = '\n';
      buff[mat_pos + 2] = '\0';
  } else {
      for (int i = 0; i < 49; i++) {
          buff[i] = ((char)ioread32(bram_c_base_addr + i * 4)) + '0';
      }
      buff[50] = '\n';
      buff[51] = '\0';
  }

  len = 64;
  ret = copy_to_user(buf, buff, len);
  if (ret)
    return -EFAULT;

  endRead = 1;
  return len;
}

static ssize_t matmul_read_(char __user *buf, size_t len) {
  static int endRead = 0;
  int ret;
  char buff[BUFF_SIZE];
  void __iomem *matmul_base_addr = dev_info->base_addr + MATMUL_ADDR_OFFSET;

  if (endRead == 1) {
    endRead = 0;
    return 0;
  }

  unsigned int ready = ioread32(matmul_base_addr);
  unsigned int start = ioread32(matmul_base_addr + 4);
  unsigned int n = ioread32(matmul_base_addr + 8);
  unsigned int m = ioread32(matmul_base_addr + 12);
  unsigned int p = ioread32(matmul_base_addr + 16);

  len = sprintf(buff, "ready=%d;start=%d;n=%d;m=%d;p=%d\n", ready, start, n, m,
                p);
  ret = copy_to_user(buf, buff, len);

  if (ret)
    return -EFAULT;

  endRead = 1;
  return len;
}

static ssize_t matmul_write_(const char __user *buf, size_t length) {
  void __iomem *matmul_base_addr = dev_info->base_addr + MATMUL_ADDR_OFFSET;
  char buff[BUFF_SIZE];
  int ret = 0;
  int m_ = 0;
  int n_ = 0;
  int p_ = 0;

  ret = copy_from_user(buff, buf, length);
  if (ret) {
    printk("copy from user failed \n");
    return -EFAULT;
  }
  buff[length] = '\0';

  ret = sscanf(buff, "dim=%d,%d,%d", &m_, &n_, &p_);
  if (ret == 3) {
    iowrite32(m_, matmul_base_addr + 8);
    iowrite32(n_, matmul_base_addr + 12);
    iowrite32(p_, matmul_base_addr + 16);
    mat_dims.m = m_;
    mat_dims.p = p_;
    mat_dims.ready = 1;
    return length;
  }

  ret = sscanf(buff, "start=%d", &m_);
  if (ret == 1) {
    iowrite32(m_, matmul_base_addr + 4);
    return length;
  }

  printk(KERN_ERR "Wrong write format, expected:\n");
  printk(KERN_ERR "    1) dim=n,m,p\n");
  printk(KERN_ERR "    2) start=0 or start=1\n");
  return -EINVAL;
}

// Implementation of driver level operations
static ssize_t matmul_read(struct file *f, char __user *buf, size_t len,
                           loff_t *off) {
  ssize_t ret;
  dev_t current_dev_num = f->f_inode->i_rdev;
  int minor = MINOR(current_dev_num);

  switch (minor) {
  case MATMUL:
    ret = matmul_read_(buf, len);
    break;
  case BRAM_C:
    ret = bram_c_read(buf, len);
    break;
  default:
    printk(KERN_ERR
           "Attempted read operation on device that does not support it.\n");
    ret = -EFAULT;
    break;
  }

  return ret;
}

static ssize_t matmul_write(struct file *f, const char __user *buf,
                            size_t length, loff_t *off) {
  ssize_t ret;
  dev_t current_dev_num = f->f_inode->i_rdev;
  int minor = MINOR(current_dev_num);

  switch (minor) {
  case MATMUL:
    ret = matmul_write_(buf, length);
    break;
  case BRAM_A:
    ret = bram_a_write(buf, length);
    break;
  case BRAM_B:
    ret = bram_b_write(buf, length);
    break;
  default:
    printk(KERN_ERR
           "Attempted write operation on device that does not support it.\n");
    ret = -EFAULT;
    break;
  }

  return ret;
}

// Probe function
static int matmul_probe(struct platform_device *pdev) {
  struct resource *r_mem;
  int ret = 0;

  // Get phisical register adress space from device tree
  r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!r_mem) {
    printk(KERN_ALERT "matmul_probe: Failed to get reg resource\n");
    return -ENODEV;
  }

  // Get memory for structure matmul_info
  dev_info =
      (struct matmul_info *)kmalloc(sizeof(struct matmul_info), GFP_KERNEL);
  if (dev_info == NULL) {
    printk(KERN_ALERT "matmul_probe: Could not allocate matmul device\n");
    return -ENOMEM;
  }

  // Put phisical adresses in matmul_info structure
  dev_info->mem_start = r_mem->start;
  dev_info->mem_end = r_mem->end;

  // Reserve that memory space for this driver
  if (!request_mem_region(dev_info->mem_start,
                          dev_info->mem_end - dev_info->mem_start + 1,
                          DEVICE_NAME)) {
    printk(KERN_ALERT "matmul_probe: Could not lock memory region at %p\n",
           (void *)dev_info->mem_start);
    ret = -EBUSY;
    goto error1;
  }

  // Remap phisical to virtual adresses
  dev_info->base_addr =
      ioremap(dev_info->mem_start, dev_info->mem_end - dev_info->mem_start + 1);
  if (!dev_info->base_addr) {
    printk(KERN_ALERT "matmul_probe: Could not allocate memory\n");
    ret = -EIO;
    goto error2;
  }

  printk(KERN_NOTICE "matmul_probe: Matmul platform driver registered\n");
  return 0;

error2:
  release_mem_region(dev_info->mem_start,
                     dev_info->mem_end - dev_info->mem_start + 1);
  kfree(dev_info);
error1:
  return ret;
}

static int matmul_remove(struct platform_device *pdev) {
  // Free resources taken in probe
  iounmap(dev_info->base_addr);
  release_mem_region(dev_info->mem_start,
                     dev_info->mem_end - dev_info->mem_start + 1);
  kfree(dev_info);
  printk(KERN_WARNING "matmul_remove: Matmul driver removed\n");

  return 0;
}

static struct platform_driver matmul_driver = {
    .driver =
        {
            .name = DRIVER_NAME,
            .owner = THIS_MODULE,
            .of_match_table = matmul_of_match,
        },
    .probe = matmul_probe,
    .remove = matmul_remove,
};

// Init and exit functions
static int __init matmul_init(void) {
  char buff[BUFF_SIZE] = {0};
  int ret = 0;
  int i;

  // Allocate device numbers
  ret = alloc_chrdev_region(&first_dev_id, 0, NUM_DEVICES, DRIVER_NAME);
  if (ret) {
    printk(KERN_ALERT "Failed to allocate char device region\n");
    return ret;
  }
  printk(KERN_INFO "matmul_init: Dev region allocated\n");

  // Create device class
  matmul_class = class_create(THIS_MODULE, "matmul_class");

  if (matmul_class == NULL) {
    printk(KERN_ALERT "Failed to create device class\n");
    goto fail_chrdev;
  }
  printk(KERN_INFO "matmul_init: Class created\n");

  // Initialize devices
  for (i = 0; i < NUM_DEVICES; i++) {
    printk(KERN_INFO "matmul_init: Created node %d\n", i);
    scnprintf(buff, BUFF_SIZE, "matmul%d", i);
    matmul_device = device_create(matmul_class, NULL,
                                  MKDEV(MAJOR(first_dev_id), i), NULL, buff);
    if (matmul_device == NULL) {
      printk(KERN_ERR "matmul_init: Failed to create device\n");
      goto fail_class;
    }
  }
  printk(KERN_INFO "matmul_init: Device created\n");

  matmul_cdev = cdev_alloc();
  matmul_cdev->ops = &matmul_fops;
  matmul_cdev->owner = THIS_MODULE;
  ret = cdev_add(matmul_cdev, first_dev_id, NUM_DEVICES);
  if (ret) {
    printk(KERN_ERR "matmul_init: Failed to add cdev\n");
    goto fail_device;
  }
  printk(KERN_INFO "matmul_init: Cdev added\n");
  printk(KERN_NOTICE "matmul_init: Hi from matmul\n");

  return platform_driver_register(&matmul_driver);

  printk(KERN_INFO "Matrix multiplication driver initialized\n");
  return 0;

fail_device:
  for (i = 0; i < NUM_DEVICES; i++) {
    device_destroy(matmul_class, MKDEV(MAJOR(first_dev_id), i));
  }
fail_class:
  class_destroy(matmul_class);
fail_chrdev:
  unregister_chrdev_region(first_dev_id, NUM_DEVICES);
  return ret;
}

static void __exit matmul_exit(void) {
  int i;

  // Unregister platform driver
  platform_driver_unregister(&matmul_driver);
  // Clean up device
  cdev_del(matmul_cdev);
  for (i = 0; i < NUM_DEVICES; i++) {
    device_destroy(matmul_class, MKDEV(MAJOR(first_dev_id), i));
  }

  // Clean up class and device numbers
  class_destroy(matmul_class);
  unregister_chrdev_region(first_dev_id, NUM_DEVICES);

  printk(KERN_INFO "Matrix multiplication driver removed\n");
}

module_init(matmul_init);
module_exit(matmul_exit);
