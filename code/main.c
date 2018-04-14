//#include "allwinner_h3_core/pio_ports.h"
#include <linux/kernel.h>	// We are doing kernel programming
#include <linux/module.h>	/* Specifically a module */
#include <linux/gpio.h>
#include <linux/hrtimer.h>	/* header for kernel high resolution timer */
#include <linux/moduleparam.h>

#define BLINK_PIN_NR 20		/* PA20 */

static int set_blink_interval(const char *val, const struct kernel_param *kp);
static int get_blink_interval(char *buffer, const struct kernel_param *kp);

static struct hrtimer gpio_blink_timer;
static int gpio_value;
static ktime_t gpio_timer_interval;
static long gpio_blink_interval_s;		/* s - stands for seconds */
static const struct kernel_param_ops kp_ops = 
{
	.set = &set_blink_interval,
	.get = &get_blink_interval
};

static int set_blink_interval(const char *val, const struct kernel_param *kp)
{
	int ret = param_set_ulong(val, kp);					/* use helper for write variable */

	if(ret < 0)
	{
		printk(KERN_INFO "Blinging interval set failed %d\n", ret);
	}

	gpio_timer_interval = ktime_set(gpio_blink_interval_s, 0);

	return ret;
}

static int get_blink_interval(char *buffer, const struct kernel_param *kp)
{
	int ret = param_get_ulong(buffer, kp);

	if(ret < 0)
	{
		printk(KERN_INFO "Blinging interval get failed %d\n", ret);
	}

	return ret;
}

static enum hrtimer_restart gpio_blink_timer_callback(struct hrtimer *param)
{
	gpio_value ^= 0x01;
	gpio_set_value(BLINK_PIN_NR, gpio_value);

	/* reset the timer */
	hrtimer_forward_now(&gpio_blink_timer, gpio_timer_interval);
	return HRTIMER_RESTART;
}

/* Init function called when module is loaded
** Returns 0, when successfully loaded, otherwise nonzero will be returned */
static int blink_module_init(void)
{
	int err = 0;
	gpio_value = 0;
	gpio_blink_interval_s = 5;
	gpio_timer_interval = ktime_set(gpio_blink_interval_s, 0);

	printk(KERN_ALERT "Blink module init\n");
	err = gpio_request(BLINK_PIN_NR, "blink_led");

	if(err)
	{
		printk(KERN_ALERT "gpio_request failed %d\n", err);
		return err;
	}

	err = gpio_direction_output(BLINK_PIN_NR, GPIOF_INIT_LOW);

	if(err)
	{
		printk(KERN_ALERT "gpio_direction_output failed %d\n", err);
		gpio_free(BLINK_PIN_NR);
		return err;
	}

	hrtimer_init(&gpio_blink_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	gpio_blink_timer.function = &gpio_blink_timer_callback;
	hrtimer_start(&gpio_blink_timer, gpio_timer_interval, HRTIMER_MODE_REL);	/* timer will be relatiove to the current time */

	return 0;
}

/* Exit function, called when module is removed from memory */
static void blink_module_exit(void)
{
	hrtimer_cancel(&gpio_blink_timer);
	gpio_set_value(BLINK_PIN_NR, 0);
	gpio_free(BLINK_PIN_NR);

	printk(KERN_ALERT "Blink module unloaded\n");
}

module_init(blink_module_init);			/* Set entry point for kernel module (module_init is a macro) */
module_exit(blink_module_exit);			/* Set exit point from a module (module_exit is a macro) */

/* Show blinking interval in sysfs */
/* module_param macro shoult not use with module_param_cb 
module_param(gpio_blink_interval_s, long, 0660); */

/* Set module param callback function */
/* 1 - file name
*  2 - pointer to kernel_param_ops structure
*  3 - pointer to a variable containing parameter value
*  4 - permissions on file */
module_param_cb(gpio_blink_interval_s, &kp_ops, &gpio_blink_interval_s, 0660);
MODULE_PARM_DESC(gpio_blink_interval_s, "Blink interval in seconds");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mks-a");
MODULE_DESCRIPTION("Blink module will blink LED, which is connected to PIN37 (GPIOC21)");