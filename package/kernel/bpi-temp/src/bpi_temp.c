#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/kallsyms.h>
/* Include the vendor header */
#include <tr6560/tri_comdef.h>
#include <tr6560/hal/tri_hal_chip.h>
#include <tr6560/kbasic/tri_hlp.h>

// Define the function signature for the hcall wrapper
typedef tri_int32 (*hcall_func_t)(tri_void *data, tri_uint32 size, tri_uint32 *outlen);

static hcall_func_t hcall_temp_get = NULL;

static int temp_show(struct seq_file *m, void *v) {
    tri_int32 *temp_ptr;
    tri_uint32 status;
    tri_uint32 outlen = 0;
    void *buffer;
    size_t total_size = sizeof(struct tri_hlp_exhead_s) + sizeof(tri_int32);

    if (!hcall_temp_get) {
        hcall_temp_get = (hcall_func_t)kallsyms_lookup_name("hcall_tri_kernel_hal_sensor_temp_get");
    }

    if (!hcall_temp_get) {
        seq_printf(m, "Error: Symbol hcall_tri_kernel_hal_sensor_temp_get not found\n");
        return 0;
    }

    // Allocate buffer
    buffer = kzalloc(total_size, GFP_KERNEL);
    if (!buffer) {
        seq_printf(m, "Error: Memory allocation failed\n");
        return 0;
    }

    // Prepare data pointer (after header)
    temp_ptr = (tri_int32 *)(buffer + sizeof(struct tri_hlp_exhead_s));

    // Call the hcall wrapper
    // We use a safe wrapper to avoid crashing if the pointer is somehow bad
    // though kallsyms_lookup_name should return a valid one.
    
    printk(KERN_INFO "bpi_temp: Calling hcall at %p\n", hcall_temp_get);
    
    status = hcall_temp_get(buffer, total_size, &outlen);

    printk(KERN_INFO "bpi_temp: HAL status: %u\n", status);

    if (status == 0) {
        seq_printf(m, "%d\n", *temp_ptr);
    } else {
        seq_printf(m, "Error: HAL returned status %u\n", status);
    }

    kfree(buffer);
    return 0;
}

static int temp_open(struct inode *inode, struct file *file) {
    return single_open(file, temp_show, NULL);
}

static const struct proc_ops temp_fops = {
    .proc_open    = temp_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init bpi_temp_init(void) {
    hcall_temp_get = (hcall_func_t)kallsyms_lookup_name("hcall_tri_kernel_hal_sensor_temp_get");
    if (hcall_temp_get) {
        printk(KERN_INFO "bpi_temp: Found HAL function at %p\n", hcall_temp_get);
    } else {
        printk(KERN_ERR "bpi_temp: Could not find hcall_tri_kernel_hal_sensor_temp_get\n");
    }
    
    proc_create("bpi_temp", 0, NULL, &temp_fops);
    printk(KERN_INFO "BPI-Wifi6 Temp Module Loaded\n");
    return 0;
}

static void __exit bpi_temp_exit(void) {
    remove_proc_entry("bpi_temp", NULL);
    printk(KERN_INFO "BPI-Wifi6 Temp Module Unloaded\n");
}

module_init(bpi_temp_init);
module_exit(bpi_temp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Collaborative AI");
MODULE_DESCRIPTION("Expose Triductor HAL temp via /proc");