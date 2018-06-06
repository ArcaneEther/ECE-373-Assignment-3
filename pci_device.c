/*
  ECE 373
  Instructors PJ Waskiewicz Jr. and Shannon Nelson
  Higgins, Jeremy
  Assignment #3 - PCI Device Driver
  
  The purpose of this program is to write a PCI
  Device Driver with minimal File Operations.
*/


/* Include Required Libraries. */
#include <linux/init.h>    /* Required for module init function. */
#include <linux/module.h>  /* Required for module init and exit functions. */
#include <linux/errno.h>   /* Required for function error codes. */
#include <linux/pci.h>     /* Required for PCI related functions. */
#include <linux/uaccess.h> /* Required for copy_to_user() and copy_from_user(). */
#include <linux/types.h>   /* Required for ? */
#include <linux/kdev_t.h>  /* Required for ? */
#include <linux/fs.h>      /* Required for ? */
#include <linux/cdev.h>    /* Required for ? */
#include <linux/usb.h>     /* Required for ? */
#include <linux/slab.h>    /* Required for ? */


/* Defines. */
#define PCI_DEVICE_e1000 0x100e   /* PCI Device Number. */
#define PE_REG_LEDS 0xE00         /* LEDCTL Register Offset. */
#define PE_LED_MASK 0xC           /* What is this Mask used for? */
#define DEVICE_COUNT 1		  /* Number of devices to allocate. */
#define DEVICE_NAME "char_device" /* Char Device name. */


/* Define Global Variables. */
static char *pe_driver_name = "pci_device";
static struct cdev cdev;
static dev_t mydev_node;
static int led_status;


/* Define Device Table Struct. */
static const struct pci_device_id pe_pci_tbl[] = {
  {PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_e1000), 0, 0, 0},

  /* Required final entry. */
  {0, }
};


/* Setting Module Information. */
MODULE_AUTHOR("Jeremy Higgins");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_DEVICE_TABLE(pci, pe_pci_tbl);


/* Define ?pes? Struct. What does "pes" stand for? */
struct pes{
  struct pci_dev *pdev;
  void *hardware_address;
};

struct pes *pe;

/* Module Probe Function. */ 
static int pe_probe(struct pci_dev *pdev, const struct pci_device_id *ent){
  int err;
  u32 ioremap_len;
  u32 led_status = 0;
  
  /* Initialize Device. */
  err = pci_enable_device_mem(pdev);
  if(err){return err;}
  
  /* Set up for high or low DMA. */
  err = dma_set_mask(&pdev->dev, DMA_BIT_MASK(64));
  if(err){dev_err(&pdev->dev, "DMA configuration failed: 0x%x\n", err); goto err_dma;}
  
  /* Set up PCI connections. */
  err = pci_request_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM), pe_driver_name);
  if(err){dev_info(&pdev->dev, "pci_request_selected_regions failed %d\n", err); goto err_pci_reg;}
  
  /* Enable bus-mastering on this device with architecture specificity. */
  pci_set_master(pdev);
  
  /* Request zeroed Kernel Memory. */
  pe = kzalloc(sizeof(*pe), GFP_KERNEL);
  if(!pe){err = -ENOMEM; goto err_pe_alloc;}
  
  /* Assigning pdev to the returned Kernel Memory. */
  pe->pdev = pdev;
  
  /* Copy pdev's members to pe. */
  pci_set_drvdata(pdev, pe);
  
  /* Find the size of the memory to be mapped? */
  ioremap_len = min_t(int, pci_resource_len(pdev, 0), 1024);
  
  /* Map hardware address to the returned Kernel Memory. */
  pe->hardware_address = ioremap(pci_resource_start(pdev, 0), ioremap_len);
  if(!pe->hardware_address){
    err = -EIO;
    dev_info(&pdev->dev, "ioremap(0x%04x, 0x%04x) failed: 0x%x\n",
             (unsigned int)pci_resource_start(pdev, 0),
             (unsigned int)pci_resource_len(pdev, 0), err);
    goto err_ioremap;
  }
  
  /* Collect & Echo Initial LED Status. */
  led_status = readl(pe->hardware_address + PE_REG_LEDS);
  dev_info(&pdev->dev, "Initial LED Status = 0x%x\n", led_status);
  
  /* Exit on success. */
  return(0);

  /* Error mapping Memory. */  
  err_ioremap:
    kfree(pe);

  /* Error getting Kernel Memory. */    
  err_pe_alloc:
    pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));

  /* Error setting up PCI connections. */    
  err_pci_reg:
    /* No OP. */
  
  /* Error configuring DMA. */
  err_dma:
    pci_disable_device(pdev);
    
  /* Exit on failure. */    
  return(err);
} /* Module Probe Function end. */


/* Module Remove Function. */
static void pe_remove(struct pci_dev *pdev){
  /* Collect the PCI Device's data for removal. */
  struct pes *pe = pci_get_drvdata(pdev);
  
  /* Unmap PCI Device from Memory. */
  iounmap(pe->hardware_address);
  
  /* Free allocated Memory. */
  kfree(pe);

  /* Release PCI Device. */  
  pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
  
  /* Disable PCI Device. */
  pci_disable_device(pdev);
} /* Module Remove Function ends. */


/* Module Open Function. */
static int pe_open(struct inode *inode, struct file *file){
  printk(KERN_INFO "pci_device module opened.\n");
  /* mydev.syscall_val = exam; */
  return(0);
}


/* Module Read Function. */
static ssize_t pe_read(struct file *file, char __user *buffer, size_t len, loff_t *offset){
  int ret;
  
  /* Check if the "offset" is too big. */
  if (*offset >= sizeof(int)){
    return(0);
  }
  
  /* Check if "buf" is NULL. */
  if (!buffer){
    ret = -EINVAL;
    goto out;
  }
  
  /* Read current LED Status. */
  led_status = readl(pe->hardware_address + PE_REG_LEDS);
  
  /* Attempt to copy buffered memory from Kernel to User. */
  if (copy_to_user(buffer, &led_status, sizeof(int))){
    ret = -EFAULT;
    goto out;
  }
  
  /* Return the number of bytes read. */
  ret = sizeof(int);
  *offset += len;
  
  /* Echo on success. */
  printk(KERN_INFO "LED Status sent to user: %x.\n", led_status);
  
  /* An error has occured, exit this function. */
  out:
    return(ret);
}


/* Module Write Function. */
static ssize_t pe_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset){
  unsigned char * kernel_buffer;
  int ret;
  
  /* Check if buffer is NULL. */
  if (!buffer){
    ret = -EINVAL;
    goto out;
  }
  
  /* Check if buffer is not a u32. */
  if(len != sizeof(u32)){
    ret = -EINVAL;
    goto out;
  }
  
  /* Get some memory to copy into. */
  kernel_buffer = kmalloc(len, GFP_KERNEL);
  if (!kernel_buffer){
    ret = -ENOMEM;
    goto out;
  }
  
  /* Copy from the user-provided buffer. */
  if (copy_from_user(kernel_buffer, buffer, len)){
    ret = -EFAULT;
    goto mem_out;
  }
  
  /* Write user's value to LEDCTL. */
  writel(*((u32*)kernel_buffer), (pe->hardware_address + PE_REG_LEDS));

  /* Read current LED Status. */
  led_status = readl(pe->hardware_address + PE_REG_LEDS);
  
  /* Return the number of bytes written. */
  ret = len;
  
  /* Free kernel memory. */
  mem_out:
    kfree(kernel_buffer);
  out:
    return(ret);
}


/* PCI Operations. */
static struct pci_driver pci_ops = {
  .name     = "pci_device",
  .id_table = pe_pci_tbl,
  .probe    = pe_probe,
  .remove   = pe_remove,
}; /* File Operations Structure ends. */


/* Char Operations. */
static struct file_operations char_ops = {
  .owner = THIS_MODULE,
  .open = pe_open,
  .read = pe_read,
  .write = pe_write,
}; /* Char Operations Structure ends. */


/* Module Initialization Function. */
static int __init pe_init(void){
  /* Define Return variable. */
  int ret;
  
  /* Module loaded message. */
  printk(KERN_INFO "%s loaded.\n", pci_ops.name);
  
  /* Register PCI Device. */
  ret = pci_register_driver(&pci_ops);

  /* Dynamically allocate the Character Device. */
  if(alloc_chrdev_region(&mydev_node, 0, DEVICE_COUNT, DEVICE_NAME)){
    /* Echo failure message. */
    printk(KERN_ERR "alloc_chrdev_region() failed!\n");
    
    /* Exit Module Failure. */
    return(-1);
  } /* if ends. */
  
  /* Echo the number of Minor Devices allocated at the Major Number. */
  printk(KERN_INFO "Allocated %d device/s at Major Number %d.\n", DEVICE_COUNT, MAJOR(mydev_node));
  
  /* Initialize the Character Device's File Operations. */
  cdev_init(&cdev, &char_ops);
  
  /* Set the Owner of the Character Device. */
  cdev.owner = THIS_MODULE;
  
  /* Add the Character to the Kernel. */
  if(cdev_add(&cdev, mydev_node, DEVICE_COUNT)){
    /* Echo failure message. */
    printk(KERN_ERR "cdev_add() failed!\n");
    
    /* Device add failed - unregister the Character Device. */
    unregister_chrdev_region(mydev_node, DEVICE_COUNT);
    
    /* Exit Module Failure. */
    return(-1);
  } /* if ends. */
  
  /* Exit Module Success. */
  return(0);
} /* Module Initialization Function ends. */


/* Module Exit Function. */
static void __exit pe_exit(void){
  /* Unregister PCI Device. */
  pci_unregister_driver(&pci_ops);
  printk(KERN_INFO "%s unregistered.\n", pci_ops.name);

  /* Delete the Character Device. */
  cdev_del(&cdev);
  printk(KERN_INFO "Char device deleted.\n");

  /* Unregister the Character Device. */
  unregister_chrdev_region(mydev_node, DEVICE_COUNT);
  printk(KERN_INFO "Char device unregistered.\n");
} /* Module Exit Function ends. */


/* Module Initialization and Exit. */
module_init(pe_init);
module_exit(pe_exit);
