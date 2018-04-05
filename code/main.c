#include <linux/kernel.h>	// We are doing kernel programming
#include <linux/module.h>	// Specifically a module
#include <linux/gpio.h>
#include <linux/hrtimer.h>	// header for kernel high resolution timer

#define BLINK_PIN_NR 20		//PA20

static struct hrtimer gpio_blink_timer;
static int gpio_value;
static ktime_t gpio_timer_interval;

enum hrtimer_restart gpio_blink_timer_callback(struct hrtimer* param)
{
	gpio_value ^= 0x01;
	gpio_set_value(BLINK_PIN_NR, gpio_value);

	// reset the timer
	hrtimer_forward_now(&gpio_blink_timer, gpio_timer_interval);
	return HRTIMER_RESTART;
}

// Init function called when module is loaded
// Returns 0, when successfully loaded, otherwise nonzero will be returned
static int blink_module_init(void)
{
	int err = 0;
	gpio_value = 0;
	gpio_timer_interval = ktime_set(5, 0);

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
		return err;
	}

	hrtimer_init(&gpio_blink_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	gpio_blink_timer.function = &gpio_blink_timer_callback;
	hrtimer_start(&gpio_blink_timer, gpio_timer_interval, HRTIMER_MODE_REL);	// timer will be relatiove to the current time

	return 0;
}

// Exit function, called when module is removed from memory
static void blink_module_exit(void)
{
	hrtimer_cancel(&gpio_blink_timer);
	gpio_set_value(BLINK_PIN_NR, 0);
	gpio_free(BLINK_PIN_NR);

	printk(KERN_ALERT "Blink module unloaded\n");
}

module_init(blink_module_init);			// Set entry point for kernel module (module_init is a macro)
module_exit(blink_module_exit);			// Set exit point from a module (module_exit is a macro)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maksim Aleksejev");
MODULE_DESCRIPTION("Blink module will blink LED, which is connected to PIN37 (GPIOC21)");