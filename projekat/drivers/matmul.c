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
#define MAX_MATRIX_VALUE 4095
#define BRAM_LINE_SIZE 7
#define MATMUL_REG_SIZE_BYTES 20
#define BRAM_SIZE_BYTES (4 * BRAM_LINE_SIZE * BRAM_LINE_SIZE)

// Device minor numbers.
enum Device {
  MATMUL = 0,
  BRAM_A = 1,
  BRAM_B = 2,
  BRAM_C = 3,
};

// Structure to hold device-specific info
struct dev_info {
  // Start address of physical memory for this device
  unsigned long mem_start;
  // End address of phyiscal memory for this device
  unsigned long mem_end;
  // Base address of virtual memory region mapped to physical
  void __iomem *base_addr;
};

struct matrix_dims {
    int n;
    int p;
    int ready;
};

// Global variables
static struct class *matmul_class;
static struct device *matmul_device;
static struct cdev *matmul_cdev;

static struct dev_info *bram_a_dev_info;
static struct dev_info *bram_b_dev_info;
static struct dev_info *bram_c_dev_info;
static struct dev_info *matmul_dev_info;

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
  void __iomem *bram_a_base_addr = bram_a_dev_info->base_addr;

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
      if (current_number > MAX_MATRIX_VALUE) {
        printk(KERN_ERR "All matrix elements must be between 0 and %d.\n", MAX_MATRIX_VALUE);
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
               "All matrix elements must be numbers between 0 and %d!\n", MAX_MATRIX_VALUE);
        return -EFAULT;
      }
    }
  }

  printk(KERN_INFO "BRAM A write OK\n");
  return length;
}

static ssize_t bram_b_write(const char __user *buf, size_t length) {
  char buff[BUFF_SIZE];
  int ret = 0;
  void __iomem *bram_b_base_addr = bram_b_dev_info->base_addr;

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
      if (current_number > MAX_MATRIX_VALUE) {
        printk(KERN_ERR "All matrix elements must be between 0 and %d.\n", MAX_MATRIX_VALUE);
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
               "All matrix elements must be numbers between 0 and %d!\n", MAX_MATRIX_VALUE);
        return -EFAULT;
      }
    }
  }

  printk(KERN_INFO "BRAM B write OK\n");
  return length;
}

static ssize_t bram_c_read(char __user *buf, size_t len) {
  static int endRead = 0;
  int ret;
  int row;
  int col;
  int bram_pos;
  int mat_val;
  int mat_val_strlen;
  char mat_val_buff[5];
  int bram_c_read_pos;
  char bram_c_read_buff[BUFF_SIZE];
  void __iomem *bram_c_base_addr = bram_c_dev_info->base_addr;

  if (endRead == 1) {
    endRead = 0;
    return 0;
  }

  if (mat_dims.ready == 1) {
      // We know the matrix format, write a formatted string.
      bram_c_read_pos = 0;
      for (row = 0; row < mat_dims.n; row++) {
          for (col = 0; col < mat_dims.p; col++) {
              bram_pos = row * mat_dims.n + col;
              mat_val = ioread32(bram_c_base_addr + bram_pos * 4);
	      if (mat_val > MAX_MATRIX_VALUE) {
                  printk(KERN_ERR "Unexpected matrix value %d. All matrix elements should be numbers between 0 and %d!\n", mat_val, MAX_MATRIX_VALUE);
		  return -EFAULT;
	      }
	      sprintf(mat_val_buff, "%d", mat_val);
	      mat_val_strlen = strlen(mat_val_buff);
	      memcpy(&bram_c_read_buff[bram_c_read_pos], mat_val_buff, mat_val_strlen); 
	      bram_c_read_pos += mat_val_strlen;
              if (col == (mat_dims.p - 1)) {
        	  bram_c_read_buff[bram_c_read_pos] = ';';
              } else {
        	  bram_c_read_buff[bram_c_read_pos] = ',';
              }
              bram_c_read_pos += 1;
          }
      }
      bram_c_read_buff[bram_c_read_pos + 1] = '\n';
      bram_c_read_buff[bram_c_read_pos + 2] = '\0';
  } else {
      // We dont know the matrix format yet, write values without formatting.
      for (int i = 0; i < 49; i++) {
          bram_c_read_buff[i] = ((char)ioread32(bram_c_base_addr + i * 4)) + '0';
      }
      bram_c_read_buff[50] = '\n';
      bram_c_read_buff[51] = '\0';
  }

  len = 64;
  ret = copy_to_user(buf, bram_c_read_buff, len);
  if (ret)
    return -EFAULT;

  endRead = 1;
  return len;
}

static ssize_t matmul_read_(char __user *buf, size_t len) {
  static int endRead = 0;
  int ret;
  char buff[BUFF_SIZE];
  void __iomem *matmul_base_addr = matmul_dev_info->base_addr;

  if (endRead == 1) {
    endRead = 0;
    return 0;
  }

  unsigned int ready_reg = ioread32(matmul_base_addr);
  unsigned int start_reg = ioread32(matmul_base_addr + 4);
  unsigned int n_reg = ioread32(matmul_base_addr + 8);
  unsigned int m_reg = ioread32(matmul_base_addr + 12);
  unsigned int p_reg = ioread32(matmul_base_addr + 16);

  len = sprintf(buff, "ready=%d;start=%d;n=%d;m=%d;p=%d\n", ready_reg, start_reg, n_reg, m_reg, p_reg);
  ret = copy_to_user(buf, buff, len);

  if (ret)
    return -EFAULT;

  endRead = 1;
  return len;
}

static ssize_t matmul_write_(const char __user *buf, size_t length) {
  void __iomem *matmul_base_addr = matmul_dev_info->base_addr;
  char buff[BUFF_SIZE];
  int ret = 0;
  int n_reg = 0;
  int m_reg = 0;
  int p_reg = 0;
  int start_reg = 0;
  
  ret = copy_from_user(buff, buf, length);
  if (ret) {
    printk("copy from user failed \n");
    return -EFAULT;
  }
  buff[length] = '\0';
  
  ret = sscanf(buff, "dim=%d,%d,%d", &n_reg, &m_reg, &p_reg);
  if (ret == 3) {
    iowrite32(n_reg, matmul_base_addr + 8);
    iowrite32(m_reg, matmul_base_addr + 12);
    iowrite32(p_reg, matmul_base_addr + 16);
    mat_dims.n = n_reg;
    mat_dims.p = p_reg;
    mat_dims.ready = 1;
    printk(KERN_INFO "MATMUL write OK\n");
    return length;
  }

  ret = sscanf(buff, "start=%d", &start_reg);
  if (ret == 1) {
    iowrite32(start_reg, matmul_base_addr + 4);
    printk(KERN_INFO "MATMUL write OK\n");
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

static int bram_a_dev_probe(struct platform_device *pdev) {
  struct resource *r_mem;
  int ret = 0;

  // Get physical register address space from device tree
  r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!r_mem) {
    printk(KERN_ALERT "bram_a_dev_probe: Failed to get reg resource\n");
    return -ENODEV;
  }
  
  // Allocate memory for dev_info structure
  bram_a_dev_info = (struct dev_info *)kmalloc(sizeof(struct dev_info), GFP_KERNEL);
  if (bram_a_dev_info == NULL) {
    printk(KERN_ALERT "bram_a_dev_probe: Could not allocate memory\n");
    return -ENOMEM;
  }

  // Put phisical adresses in dev_info structure
  bram_a_dev_info->mem_start = r_mem->start;
  bram_a_dev_info->mem_end = r_mem->end;

  // Reserve that memory space for this driver
  if (!request_mem_region(bram_a_dev_info->mem_start,
                          bram_a_dev_info->mem_end - bram_a_dev_info->mem_start + 1,
                          DEVICE_NAME)) {
    printk(KERN_ALERT "bram_a_dev_probe: Could not lock memory region at %p\n",
           (void *)bram_a_dev_info->mem_start);
    ret = -EBUSY;
    goto error1;
  }

  // Remap physical to virtual addresses
  bram_a_dev_info->base_addr =
      ioremap(bram_a_dev_info->mem_start, bram_a_dev_info->mem_end - bram_a_dev_info->mem_start + 1);
  if (!bram_a_dev_info->base_addr) {
    printk(KERN_ALERT "bram_a_dev_probe: Could remap memory\n");
    ret = -EIO;
    goto error2;
  }

  printk(KERN_NOTICE "bram_a_dev_probe: Bram A device registered\n");
  return 0;

error2:
  release_mem_region(bram_a_dev_info->mem_start,
                     bram_a_dev_info->mem_end - bram_a_dev_info->mem_start + 1);
  kfree(bram_a_dev_info);
error1:
  return ret;
}

static int bram_b_dev_probe(struct platform_device *pdev) {
  struct resource *r_mem;
  int ret = 0;

  // Get physical register address space from device tree
  r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!r_mem) {
    printk(KERN_ALERT "bram_b_dev_probe: Failed to get reg resource\n");
    return -ENODEV;
  }
  
  // Get memory for structure matmul_info
  bram_b_dev_info = (struct dev_info *)kmalloc(sizeof(struct dev_info), GFP_KERNEL);
  if (bram_b_dev_info == NULL) {
    printk(KERN_ALERT "bram_b_dev_probe: Could not allocate memory\n");
    return -ENOMEM;
  }

  // Put phisical adresses in matmul_info structure
  bram_b_dev_info->mem_start = r_mem->start;
  bram_b_dev_info->mem_end = r_mem->end;

  // Reserve that memory space for this driver
  if (!request_mem_region(bram_b_dev_info->mem_start,
                          bram_b_dev_info->mem_end - bram_b_dev_info->mem_start + 1,
                          DEVICE_NAME)) {
    printk(KERN_ALERT "bram_b_dev_probe: Could not lock memory region at %p\n",
           (void *)bram_b_dev_info->mem_start);
    ret = -EBUSY;
    goto error1;
  }

  // Remap physical to virtual addresses
  bram_b_dev_info->base_addr =
      ioremap(bram_b_dev_info->mem_start, bram_b_dev_info->mem_end - bram_b_dev_info->mem_start + 1);
  if (!bram_b_dev_info->base_addr) {
    printk(KERN_ALERT "bram_b_dev_probe: Could not remap memory\n");
    ret = -EIO;
    goto error2;
  }

  printk(KERN_NOTICE "bram_b_dev_probe: Bram B device registered\n");
  return 0;

error2:
  release_mem_region(bram_b_dev_info->mem_start,
                     bram_b_dev_info->mem_end - bram_b_dev_info->mem_start + 1);
  kfree(bram_b_dev_info);
error1:
  return ret;
}

static int bram_c_dev_probe(struct platform_device *pdev) {
  struct resource *r_mem;
  int ret = 0;

  // Get physical register address space from device tree
  r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!r_mem) {
    printk(KERN_ALERT "bram_c_dev_probe: Failed to get reg resource\n");
    return -ENODEV;
  }
  
  // Get memory for structure matmul_info
  bram_c_dev_info = (struct dev_info *)kmalloc(sizeof(struct dev_info), GFP_KERNEL);
  if (bram_c_dev_info == NULL) {
    printk(KERN_ALERT "bram_c_dev_probe: Could not allocate memory\n");
    return -ENOMEM;
  }

  // Put phisical adresses in matmul_info structure
  bram_c_dev_info->mem_start = r_mem->start;
  bram_c_dev_info->mem_end = r_mem->end;

  // Reserve that memory space for this driver
  if (!request_mem_region(bram_c_dev_info->mem_start,
                          bram_c_dev_info->mem_end - bram_c_dev_info->mem_start + 1,
                          DEVICE_NAME)) {
    printk(KERN_ALERT "bram_c_dev_probe: Could not lock memory region at %p\n",
           (void *)bram_c_dev_info->mem_start);
    ret = -EBUSY;
    goto error1;
  }

  // Remap physical to virtual addresses
  bram_c_dev_info->base_addr =
      ioremap(bram_c_dev_info->mem_start, bram_c_dev_info->mem_end - bram_c_dev_info->mem_start + 1);
  if (!bram_c_dev_info->base_addr) {
    printk(KERN_ALERT "bram_c_dev_probe: Could not remap memory\n");
    ret = -EIO;
    goto error2;
  }

  printk(KERN_NOTICE "bram_c_dev_probe: Bram C device registered\n");
  return 0;

error2:
  release_mem_region(bram_c_dev_info->mem_start,
                     bram_c_dev_info->mem_end - bram_c_dev_info->mem_start + 1);
  kfree(bram_c_dev_info);
error1:
  return ret;
}

static int matmul_dev_probe(struct platform_device *pdev) {
  struct resource *r_mem;
  int ret = 0;

  // Get physical register address space from device tree
  r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!r_mem) {
    printk(KERN_ALERT "matmul_dev_probe: Failed to get reg resource\n");
    return -ENODEV;
  }
  
  // Get memory for structure matmul_info
  matmul_dev_info = (struct dev_info *)kmalloc(sizeof(struct dev_info), GFP_KERNEL);
  if (matmul_dev_info == NULL) {
    printk(KERN_ALERT "matmul_dev_probe: Could not allocate memory\n");
    return -ENOMEM;
  }

  // Put phisical adresses in matmul_info structure
  matmul_dev_info->mem_start = r_mem->start;
  matmul_dev_info->mem_end = r_mem->end;

  // Reserve that memory space for this driver
  if (!request_mem_region(matmul_dev_info->mem_start,
                          matmul_dev_info->mem_end - matmul_dev_info->mem_start + 1,
                          DEVICE_NAME)) {
    printk(KERN_ALERT "matmul_dev_probe: Could not lock memory region at %p\n",
           (void *)matmul_dev_info->mem_start);
    ret = -EBUSY;
    goto error1;
  }

  // Remap physical to virtual addresses
  matmul_dev_info->base_addr =
      ioremap(matmul_dev_info->mem_start, matmul_dev_info->mem_end - matmul_dev_info->mem_start + 1);
  if (!matmul_dev_info->base_addr) {
    printk(KERN_ALERT "matmul_dev_probe: Could not remap memory\n");
    ret = -EIO;
    goto error2;
  }

  printk(KERN_NOTICE "matmul_dev_probe: Matmul device registered\n");
  return 0;

error2:
  release_mem_region(matmul_dev_info->mem_start,
                     matmul_dev_info->mem_end - matmul_dev_info->mem_start + 1);
  kfree(matmul_dev_info);
error1:
  return ret;
}

static int matmul_probe(struct platform_device *pdev) {
  int ret = 0;
  
  // Konstante izvucene iz device tree-a
  if (strcmp(pdev->name, "40000000.axi_bram_ctrl") == 0) {
    ret = bram_a_dev_probe(pdev);
  } else if (strcmp(pdev->name, "42000000.axi_bram_ctrl") == 0) {
    ret = bram_b_dev_probe(pdev);
  } else if (strcmp(pdev->name, "44000000.axi_bram_ctrl") == 0) {
    ret = bram_c_dev_probe(pdev);
  } else if (strcmp(pdev->name, "43c00000.matrix_multiplier") == 0) {
    ret = matmul_dev_probe(pdev);
  } else {
    printk(KERN_ALERT "matmul_probe: Unexpected platform device name: %s\n", pdev->name);
    ret = -ENODEV;
  }

  return ret;
}

static void bram_a_dev_remove(void) {
  // Free resources taken in probe
  iounmap(bram_a_dev_info->base_addr);
  release_mem_region(bram_a_dev_info->mem_start,
                     bram_a_dev_info->mem_end - bram_a_dev_info->mem_start + 1);
  kfree(bram_a_dev_info);
  printk(KERN_WARNING "bram_a_dev_remove: Bram A device removed\n");
}

static void bram_b_dev_remove(void) {
  // Free resources taken in probe
  iounmap(bram_b_dev_info->base_addr);
  release_mem_region(bram_b_dev_info->mem_start,
                     bram_b_dev_info->mem_end - bram_b_dev_info->mem_start + 1);
  kfree(bram_b_dev_info);
  printk(KERN_WARNING "bram_b_dev_remove: Bram B device removed\n");
}

static void bram_c_dev_remove(void) {
  // Free resources taken in probe
  iounmap(bram_c_dev_info->base_addr);
  release_mem_region(bram_c_dev_info->mem_start,
                     bram_c_dev_info->mem_end - bram_c_dev_info->mem_start + 1);
  kfree(bram_c_dev_info);
  printk(KERN_WARNING "bram_c_dev_remove: Bram C device removed\n");
}

static void matmul_dev_remove(void) {
  // Free resources taken in probe
  iounmap(matmul_dev_info->base_addr);
  release_mem_region(matmul_dev_info->mem_start,
                     matmul_dev_info->mem_end - matmul_dev_info->mem_start + 1);
  kfree(matmul_dev_info);
  printk(KERN_WARNING "matmul_dev_remove: Matmul device removed\n");
}

static int matmul_remove(struct platform_device *pdev) {
  // Konstante izvucene iz device tree-a
  if (strcmp(pdev->name, "40000000.axi_bram_ctrl") == 0) {
    bram_a_dev_remove();
  } else if (strcmp(pdev->name, "42000000.axi_bram_ctrl") == 0) {
    bram_b_dev_remove();
  } else if (strcmp(pdev->name, "44000000.axi_bram_ctrl") == 0) {
    bram_c_dev_remove();
  } else if (strcmp(pdev->name, "43c00000.matrix_multiplier") == 0) {
    matmul_dev_remove();
  } else {
    printk(KERN_ALERT "matmul_probe: Unexpected platform device name: %s\n", pdev->name);
    return -ENODEV;
  }


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
