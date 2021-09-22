/*
 * Virtual Foo Device Driver
 *
 * Copyright 2017 Milo Kim <woogyom.kim@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sysfs.h>

#define REG_ID            0x0

#define REG_INIT        0x4
#define HW_ENABLE        BIT(0)

#define REG_CMD            0x8

#define REG_INT_STATUS        0xc
#define IRQ_ENABLED        BIT(0)
#define IRQ_BUF_DEQ        BIT(1)

struct virt_foo {
    struct device *dev;
    void __iomem *base;
    int count_irq;
};

static ssize_t vf_show_id(struct device *dev,
              struct device_attribute *attr, char *buf)
{
    struct virt_foo *vf = dev_get_drvdata(dev);
    u32 val = readl_relaxed(vf->base + REG_ID);

    return scnprintf(buf, PAGE_SIZE, "Chip ID: 0x%.x\n", val);
}

static ssize_t vf_show_cmd(struct device *dev,
               struct device_attribute *attr, char *buf)
{
    struct virt_foo *vf = dev_get_drvdata(dev);
    u32 val = readl_relaxed(vf->base + REG_CMD);

    return scnprintf(buf, PAGE_SIZE, "Command buffer: 0x%.x\n", val);
}

static ssize_t vf_store_cmd(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t len)
{
    struct virt_foo *vf = dev_get_drvdata(dev);
    unsigned long val;

    if (kstrtoul(buf, 0, &val))
        return -EINVAL;

    writel_relaxed(val, vf->base + REG_CMD);

    return len;
}

static DEVICE_ATTR(id, S_IRUGO, vf_show_id, NULL);
static DEVICE_ATTR(cmd, S_IRUGO | S_IWUSR, vf_show_cmd, vf_store_cmd);

static struct attribute *vf_attributes[] = {
    &dev_attr_id.attr,
    &dev_attr_cmd.attr,
    NULL,
};

static const struct attribute_group vf_attr_group = {
    .attrs = vf_attributes,
};

static void vf_init(struct virt_foo *vf)
{
    vf->count_irq = 0;
    writel_relaxed(HW_ENABLE, vf->base + REG_INIT);
}

static irqreturn_t vf_irq_handler(int irq, void *data)
{
    struct virt_foo *vf = (struct virt_foo *)data;
    u32 status;

    status = readl_relaxed(vf->base + REG_INT_STATUS);

    if (status & IRQ_ENABLED){
      dev_info(vf->dev, "HW is enabled\n");
      vf->count_irq++;
    }

    if (status & IRQ_BUF_DEQ){
        dev_info(vf->dev, "Command buffer is dequeued\n");
        vf->count_irq++;
    }
    printk(KERN_INFO "Interrupt Count is: %d", vf->count_irq);
    return IRQ_HANDLED;
}

static int vf_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct resource *res;
    struct virt_foo *vf;
    int ret;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
        return -ENOMEM;

    vf = devm_kzalloc(dev, sizeof(*vf), GFP_KERNEL);
    if (!vf)
        return -ENOMEM;

    vf->dev = dev;
    vf->base = devm_ioremap(dev, res->start, resource_size(res));
    if (!vf->base)
        return -EINVAL;

    res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (res) {
        ret = devm_request_irq(dev, res->start, vf_irq_handler,
                       IRQF_TRIGGER_HIGH, "vf_irq", vf);
        if (ret)
            return ret;
    }

    platform_set_drvdata(pdev, vf);

    vf_init(vf);

    return sysfs_create_group(&dev->kobj, &vf_attr_group);
}

static int vf_remove(struct platform_device *pdev)
{
    struct virt_foo *vf = platform_get_drvdata(pdev);

    sysfs_remove_group(&vf->dev->kobj, &vf_attr_group);
    return 0;
}

static const struct of_device_id vf_of_match[] = {
    { .compatible = "virt-foo", },
    { }
};
MODULE_DEVICE_TABLE(of, vf_of_match);

static struct platform_driver vf_driver = {
    .probe = vf_probe,
    .remove = vf_remove,
    .driver = {
        .name = "virt_foo",
        .of_match_table = vf_of_match,
    },
};
module_platform_driver(vf_driver);

MODULE_DESCRIPTION("Virtual Foo Driver");
MODULE_AUTHOR("Milo Kim");
MODULE_LICENSE("GPL");
