#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>

/* Vendor HAL headers */
#include <tr6560/tri_comdef.h>
#include <tr6560/hal/tri_hal_chip.h>
#include <tr6560/kbasic/tri_hlp.h>

/**
 * The Triductor HAL symbols are not exported directly to other modules.
 * Instead, the BSP exports "hcall_" wrappers designed for IPC.
 * These wrappers expect a specific buffer layout:
 * [ struct tri_hlp_exhead_s ] [ Actual Function Arguments ]
 */

typedef tri_int32 (*hcall_temp_get_t)(tri_void *data, tri_uint32 size, tri_uint32 *outlen);

static hcall_temp_get_t hcall_temp_get;

/**
 * temp_show - Read temperature from HAL and expose via seq_file
 */
static int temp_show(struct seq_file *m, void *v)
{
	struct {
		struct tri_hlp_exhead_s head;
		tri_int32 val;
	} __packed msg;
	tri_uint32 outlen = 0;
	tri_uint32 status;

	if (unlikely(!hcall_temp_get)) {
		seq_puts(m, "Error: HAL symbol not resolved\n");
		return 0;
	}

	memset(&msg, 0, sizeof(msg));

	/* 
	 * The hcall wrapper for tri_kernel_hal_sensor_temp_get(tri_int32 *temp)
	 * skips the header and passes the address of the subsequent data 
	 * as the 'temp' pointer.
	 */
	status = hcall_temp_get(&msg, sizeof(msg), &outlen);

	if (status == 0)
		seq_printf(m, "%d\n", msg.val);
	else
		seq_printf(m, "Error: HAL returned %u\n", status);

	return 0;
}

static int temp_open(struct inode *inode, struct file *file)
{
	return single_open(file, temp_show, NULL);
}

static const struct proc_ops temp_fops = {
	.proc_open    = temp_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int __init bpi_temp_init(void)
{
	/* 
	 * We use kallsyms_lookup_name because the hcall symbols are exported 
	 * but often not visible to modpost during external module builds 
	 * in this specific BSP environment.
	 */
	hcall_temp_get = (hcall_temp_get_t)kallsyms_lookup_name("hcall_tri_kernel_hal_sensor_temp_get");

	if (!hcall_temp_get) {
		pr_err("Failed to resolve hcall_tri_kernel_hal_sensor_temp_get\n");
		return -ENODEV;
	}

	if (!proc_create("bpi_temp", 0444, NULL, &temp_fops)) {
		pr_err("Failed to create /proc/bpi_temp\n");
		return -ENOMEM;
	}

	pr_info("Module loaded (HAL at %p)\n", hcall_temp_get);
	return 0;
}

static void __exit bpi_temp_exit(void)
{
	remove_proc_entry("bpi_temp", NULL);
	pr_info("Module unloaded\n");
}

module_init(bpi_temp_init);
module_exit(bpi_temp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Rios");
MODULE_DESCRIPTION("Expose Triductor HAL temp via /proc/bpi_temp");
MODULE_VERSION("1.0");
