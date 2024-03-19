#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/slab.h>

#define DEV_NAME "shmem"

static void *shared_page = NULL;

static int shmem_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long pfn = page_to_pfn(virt_to_page(shared_page));
    int ret = remap_pfn_range(vma, vma->vm_start, pfn, PAGE_SIZE, vma->vm_page_prot);

    if (ret) {
        printk(KERN_ERR "Failed to map physical page to user space\n");
        return ret;
    }

    printk(KERN_INFO "Shared page mapped to user space\n");
    return 0;
}

static struct file_operations shmem_fops = {
    .mmap = shmem_mmap,
};

static int __init shmem_init(void)
{
    int major_number;
    struct class *shmem_class;
    struct device *shmem_device;

    // Register character device
    major_number = register_chrdev(0, DEV_NAME, &shmem_fops);
    if (major_number < 0) {
        printk(KERN_ERR "Failed to register character device\n");
        return major_number;
    }

    // Create device class
    shmem_class = class_create(THIS_MODULE, DEV_NAME "_class");
    if (IS_ERR(shmem_class)) {
        unregister_chrdev(major_number, DEV_NAME);
        printk(KERN_ERR "Failed to create device class\n");
        return PTR_ERR(shmem_class);
    }

    // Create device node
    shmem_device = device_create(shmem_class, NULL, MKDEV(major_number, 0), NULL, DEV_NAME);
    if (IS_ERR(shmem_device)) {
        class_destroy(shmem_class);
        unregister_chrdev(major_number, DEV_NAME);
        printk(KERN_ERR "Failed to create device node\n");
        return PTR_ERR(shmem_device);
    }

    
    // Allocate a single physical page for shared memory
    shared_page = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!shared_page) {
        printk(KERN_ERR "Failed to allocate shared memory\n");
        return -ENOMEM;
    }
    
    printk(KERN_INFO "shmem module initialized\n");
    return 0;
}

static void __exit shmem_exit(void)
{
    // Cleanup on module exit
    device_destroy(class_create(THIS_MODULE, DEV_NAME "_class"), MKDEV(register_chrdev(0, DEV_NAME, &shmem_fops), 0));
    class_destroy(class_create(THIS_MODULE, DEV_NAME "_class"));
    unregister_chrdev(register_chrdev(0, DEV_NAME, &shmem_fops), DEV_NAME);

    // Free the allocated shared memory page
    if (shared_page)
        kfree(shared_page);
	
    printk(KERN_INFO "shmem module exited\n");
}

module_init(shmem_init);
module_exit(shmem_exit);
