#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/pci.h>


#define PCI_DEVICE_RTL8101E 0x8136
#define RT_LED_MASK 0xC
#define RT_REG_CONFIG1 0x0052

static char *rt_driver_name = "Realtek_poker";

static const struct pci_device_id rt_pci_tbl[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_REALTEK, PCI_DEVICE_RTL8101E), 0, 0, 0 },
	/* more device ids can be listed here */

	/* required last entry */
	{0, }
};
MODULE_DEVICE_TABLE(pci, rt_pci_tbl);
MODULE_LICENSE("Dual BSD/GPL");

static int new_leds;
module_param(new_leds, int, 0);

struct rts {
	struct pci_dev *pdev;
	void *hw_addr;
};

static int __devinit rt_probe(struct pci_dev *pdev,
			      const struct pci_device_id *ent)
{
	struct rts *rt;
	u32 ioremap_len;
	u8 config1;
	int err;

	err = pci_enable_device_mem(pdev);
	if (err)
		return err;

	/* set up for high or low dma */
	err = dma_set_mask(&pdev->dev, DMA_BIT_MASK(64));
	if (err) {
		dev_err(&pdev->dev, "DMA configuration failed: 0x%x\n", err);
		goto err_dma;
	}

	/* set up pci connections */
	err = pci_request_selected_regions(pdev, pci_select_bars(pdev,
					   IORESOURCE_MEM), rt_driver_name);
	if (err) {
		dev_info(&pdev->dev, "pci_request_selected_regions failed %d\n", err);
		goto err_pci_reg;
	}

	pci_set_master(pdev);

	rt = kzalloc(sizeof(*rt), GFP_KERNEL);
	if (!rt) {
		err = -ENOMEM;
		goto err_rt_alloc;
	}
	rt->pdev = pdev;
	pci_set_drvdata(pdev, rt);

	/* map device memory */
	ioremap_len = min_t(int, pci_resource_len(pdev, 0), 1024);
	rt->hw_addr = ioremap(pci_resource_start(pdev, 0), ioremap_len);
	if (!rt->hw_addr) {
		err = -EIO;
		dev_info(&pdev->dev, "ioremap(0x%04x, 0x%04x) failed: 0x%x\n",
			 (unsigned int)pci_resource_start(pdev, 0),
			 (unsigned int)pci_resource_len(pdev, 0), err);
		goto err_ioremap;
	}

	config1 = readb(rt->hw_addr + RT_REG_CONFIG1);
	dev_info(&pdev->dev, "config1 = 0x%02x\n", config1);

	if (new_leds) {
		config1 = (config1 & ~RT_REG_CONFIG1) | (new_leds & RT_LED_MASK);
		writeb(config1, (rt->hw_addr + RT_REG_CONFIG1));
		dev_info(&pdev->dev, "new config1 = 0x%02x\n", config1);
	}

	return 0;

err_ioremap:
	kfree(rt);
err_rt_alloc:
	pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
err_pci_reg:
err_dma:
	pci_disable_device(pdev);
	return err;
}

static void __devexit rt_remove(struct pci_dev *pdev)
{
	struct rts *rt = pci_get_drvdata(pdev);

	/* unmap device from memory */
	iounmap(rt->hw_addr);

	/* free any allocated memory */
	kfree(rt);

	pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
	pci_disable_device(pdev);
}

static struct pci_driver rt_driver = {
	.name     = "pci_realtek",
	.id_table = rt_pci_tbl,
	.probe    = rt_probe,
	.remove   = rt_remove,
};

static int __init hello_init(void)
{
	int ret;
	printk(KERN_INFO "Hello, kernel\n");
	ret = pci_register_driver(&rt_driver);
	return ret;
}

static void __exit hello_exit(void)
{
	printk(KERN_INFO "Goodbye, kernel\n");
	pci_unregister_driver(&rt_driver);
}

module_init(hello_init);
module_exit(hello_exit);

