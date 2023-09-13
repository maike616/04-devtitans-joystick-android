/*  Snes Controller Driver
 *  A snes controller driver which uses the gpios of the raspberry pi
 *  to poll the information which will be transfered to the input subsystem
 *  event handlers
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/input.h>	// get into the input subsystem
#include <linux/delay.h>	// sleep function

/*
 * Copyright (c) 2007 Dmitry Torokhov
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/input.h>
#include <linux/workqueue.h>

/**
 * struct input_polled_dev - simple polled input device
 * @private: private driver data.
 * @open: driver-supplied method that prepares device for polling
 *	(enabled the device and maybe flushes device state).
 * @close: driver-supplied method that is called when device is no
 *	longer being polled. Used to put device into low power mode.
 * @poll: driver-supplied method that polls the device and posts
 *	input events (mandatory).
 * @poll_interval: specifies how often the poll() method should be called.
 *	Defaults to 500 msec unless overridden when registering the device.
 * @poll_interval_max: specifies upper bound for the poll interval.
 *	Defaults to the initial value of @poll_interval.
 * @poll_interval_min: specifies lower bound for the poll interval.
 *	Defaults to 0.
 * @input: input device structure associated with the polled device.
 *	Must be properly initialized by the driver (id, name, phys, bits).
 *
 * Polled input device provides a skeleton for supporting simple input
 * devices that do not raise interrupts but have to be periodically
 * scanned or polled to detect changes in their state.
 */
struct input_polled_dev {
	void *private;

	void (*open)(struct input_polled_dev *dev);
	void (*close)(struct input_polled_dev *dev);
	void (*poll)(struct input_polled_dev *dev);
	unsigned int poll_interval; /* msec */
	unsigned int poll_interval_max; /* msec */
	unsigned int poll_interval_min; /* msec */

	struct input_dev *input;

/* private: */
	struct delayed_work work;

	bool devres_managed;
};

struct input_polled_dev *input_allocate_polled_device(void);
struct input_polled_dev *devm_input_allocate_polled_device(struct device *dev);
void input_free_polled_device(struct input_polled_dev *dev);
int input_register_polled_device(struct input_polled_dev *dev);
void input_unregister_polled_device(struct input_polled_dev *dev);


#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/module.h>

MODULE_AUTHOR("Dmitry Torokhov <dtor@mail.ru>");
MODULE_DESCRIPTION("Generic implementation of a polled input device");
MODULE_LICENSE("GPL v2");

static void input_polldev_queue_work(struct input_polled_dev *dev)
{
	unsigned long delay;

	delay = msecs_to_jiffies(dev->poll_interval);
	if (delay >= HZ)
		delay = round_jiffies_relative(delay);

	queue_delayed_work(system_freezable_wq, &dev->work, delay);
}

static void input_polled_device_work(struct work_struct *work)
{
	struct input_polled_dev *dev =
		container_of(work, struct input_polled_dev, work.work);

	dev->poll(dev);
	input_polldev_queue_work(dev);
}

static int input_open_polled_device(struct input_dev *input)
{
	struct input_polled_dev *dev = input_get_drvdata(input);

	if (dev->open)
		dev->open(dev);

	/* Only start polling if polling is enabled */
	if (dev->poll_interval > 0) {
		dev->poll(dev);
		input_polldev_queue_work(dev);
	}

	return 0;
}

static void input_close_polled_device(struct input_dev *input)
{
	struct input_polled_dev *dev = input_get_drvdata(input);

	cancel_delayed_work_sync(&dev->work);

	if (dev->close)
		dev->close(dev);
}

/* SYSFS interface */

static ssize_t input_polldev_get_poll(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", polldev->poll_interval);
}

static ssize_t input_polldev_set_poll(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);
	struct input_dev *input = polldev->input;
	unsigned int interval;
	int err;

	err = kstrtouint(buf, 0, &interval);
	if (err)
		return err;

	if (interval < polldev->poll_interval_min)
		return -EINVAL;

	if (interval > polldev->poll_interval_max)
		return -EINVAL;

	mutex_lock(&input->mutex);

	polldev->poll_interval = interval;

	if (input->users) {
		cancel_delayed_work_sync(&polldev->work);
		if (polldev->poll_interval > 0)
			input_polldev_queue_work(polldev);
	}

	mutex_unlock(&input->mutex);

	return count;
}

static DEVICE_ATTR(poll, S_IRUGO | S_IWUSR, input_polldev_get_poll,
					    input_polldev_set_poll);


static ssize_t input_polldev_get_max(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", polldev->poll_interval_max);
}

static DEVICE_ATTR(max, S_IRUGO, input_polldev_get_max, NULL);

static ssize_t input_polldev_get_min(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", polldev->poll_interval_min);
}

static DEVICE_ATTR(min, S_IRUGO, input_polldev_get_min, NULL);

static struct attribute *sysfs_attrs[] = {
	&dev_attr_poll.attr,
	&dev_attr_max.attr,
	&dev_attr_min.attr,
	NULL
};

static struct attribute_group input_polldev_attribute_group = {
	.attrs = sysfs_attrs
};

static const struct attribute_group *input_polldev_attribute_groups[] = {
	&input_polldev_attribute_group,
	NULL
};

/**
 * input_allocate_polled_device - allocate memory for polled device
 *
 * The function allocates memory for a polled device and also
 * for an input device associated with this polled device.
 */
struct input_polled_dev *input_allocate_polled_device(void)
{
	struct input_polled_dev *dev;

	dev = kzalloc(sizeof(struct input_polled_dev), GFP_KERNEL);
	if (!dev)
		return NULL;

	dev->input = input_allocate_device();
	if (!dev->input) {
		kfree(dev);
		return NULL;
	}

	return dev;
}
EXPORT_SYMBOL(input_allocate_polled_device);

struct input_polled_devres {
	struct input_polled_dev *polldev;
};

static int devm_input_polldev_match(struct device *dev, void *res, void *data)
{
	struct input_polled_devres *devres = res;

	return devres->polldev == data;
}

static void devm_input_polldev_release(struct device *dev, void *res)
{
	struct input_polled_devres *devres = res;
	struct input_polled_dev *polldev = devres->polldev;

	dev_dbg(dev, "%s: dropping reference/freeing %s\n",
		__func__, dev_name(&polldev->input->dev));

	input_put_device(polldev->input);
	kfree(polldev);
}

static void devm_input_polldev_unregister(struct device *dev, void *res)
{
	struct input_polled_devres *devres = res;
	struct input_polled_dev *polldev = devres->polldev;

	dev_dbg(dev, "%s: unregistering device %s\n",
		__func__, dev_name(&polldev->input->dev));
	input_unregister_device(polldev->input);

	/*
	 * Note that we are still holding extra reference to the input
	 * device so it will stick around until devm_input_polldev_release()
	 * is called.
	 */
}

/**
 * devm_input_allocate_polled_device - allocate managed polled device
 * @dev: device owning the polled device being created
 *
 * Returns prepared &struct input_polled_dev or %NULL.
 *
 * Managed polled input devices do not need to be explicitly unregistered
 * or freed as it will be done automatically when owner device unbinds
 * from * its driver (or binding fails). Once such managed polled device
 * is allocated, it is ready to be set up and registered in the same
 * fashion as regular polled input devices (using
 * input_register_polled_device() function).
 *
 * If you want to manually unregister and free such managed polled devices,
 * it can be still done by calling input_unregister_polled_device() and
 * input_free_polled_device(), although it is rarely needed.
 *
 * NOTE: the owner device is set up as parent of input device and users
 * should not override it.
 */
struct input_polled_dev *devm_input_allocate_polled_device(struct device *dev)
{
	struct input_polled_dev *polldev;
	struct input_polled_devres *devres;

	devres = devres_alloc(devm_input_polldev_release, sizeof(*devres),
			      GFP_KERNEL);
	if (!devres)
		return NULL;

	polldev = input_allocate_polled_device();
	if (!polldev) {
		devres_free(devres);
		return NULL;
	}

	polldev->input->dev.parent = dev;
	polldev->devres_managed = true;

	devres->polldev = polldev;
	devres_add(dev, devres);

	return polldev;
}
EXPORT_SYMBOL(devm_input_allocate_polled_device);

/**
 * input_free_polled_device - free memory allocated for polled device
 * @dev: device to free
 *
 * The function frees memory allocated for polling device and drops
 * reference to the associated input device.
 */
void input_free_polled_device(struct input_polled_dev *dev)
{
	if (dev) {
		if (dev->devres_managed)
			WARN_ON(devres_destroy(dev->input->dev.parent,
						devm_input_polldev_release,
						devm_input_polldev_match,
						dev));
		input_put_device(dev->input);
		kfree(dev);
	}
}
EXPORT_SYMBOL(input_free_polled_device);

/**
 * input_register_polled_device - register polled device
 * @dev: device to register
 *
 * The function registers previously initialized polled input device
 * with input layer. The device should be allocated with call to
 * input_allocate_polled_device(). Callers should also set up poll()
 * method and set up capabilities (id, name, phys, bits) of the
 * corresponding input_dev structure.
 */
int input_register_polled_device(struct input_polled_dev *dev)
{
	struct input_polled_devres *devres = NULL;
	struct input_dev *input = dev->input;
	int error;

	if (dev->devres_managed) {
		devres = devres_alloc(devm_input_polldev_unregister,
				      sizeof(*devres), GFP_KERNEL);
		if (!devres)
			return -ENOMEM;

		devres->polldev = dev;
	}

	input_set_drvdata(input, dev);
	INIT_DELAYED_WORK(&dev->work, input_polled_device_work);

	if (!dev->poll_interval)
		dev->poll_interval = 500;
	if (!dev->poll_interval_max)
		dev->poll_interval_max = dev->poll_interval;

	input->open = input_open_polled_device;
	input->close = input_close_polled_device;

	input->dev.groups = input_polldev_attribute_groups;

	error = input_register_device(input);
	if (error) {
		devres_free(devres);
		return error;
	}

	/*
	 * Take extra reference to the underlying input device so
	 * that it survives call to input_unregister_polled_device()
	 * and is deleted only after input_free_polled_device()
	 * has been invoked. This is needed to ease task of freeing
	 * sparse keymaps.
	 */
	input_get_device(input);

	if (dev->devres_managed) {
		dev_dbg(input->dev.parent, "%s: registering %s with devres.\n",
			__func__, dev_name(&input->dev));
		devres_add(input->dev.parent, devres);
	}

	return 0;
}
EXPORT_SYMBOL(input_register_polled_device);

/**
 * input_unregister_polled_device - unregister polled device
 * @dev: device to unregister
 *
 * The function unregisters previously registered polled input
 * device from input layer. Polling is stopped and device is
 * ready to be freed with call to input_free_polled_device().
 */
void input_unregister_polled_device(struct input_polled_dev *dev)
{
	if (dev->devres_managed)
		WARN_ON(devres_destroy(dev->input->dev.parent,
					devm_input_polldev_unregister,
					devm_input_polldev_match,
					dev));

	input_unregister_device(dev->input);
}
EXPORT_SYMBOL(input_unregister_polled_device);

#include <linux/gpio.h>

MODULE_AUTHOR("Harald Heckmann");
MODULE_DESCRIPTION("SNES Controller driver using RPis GPIOs.");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");


// GPIO numbers
#define GPIO_CLOCK 17
#define GPIO_LATCH 27
#define GPIO_DATA 22


// mapping of buttons numbers to buttons
static const int button_mapping[12] = {BTN_B, BTN_SELECT,\
BTN_START, ABS_HAT0Y, ABS_HAT0Y, ABS_HAT0X, ABS_HAT0X,\
BTN_Y, BTN_B, BTN_TL, BTN_TR};

// struct needed for input.h (lib that actually includes our controller into the
// input subsystem)
static struct input_dev *snes_dev;
// struct needed for input-polldev.h (our device is old and needs to be polled)
static struct input_polled_dev *snes_polled_dev;

static inline void reset_pins(void) {
	// all bits equal to zero will be ignored
	gpio_set_value(GPIO_CLOCK, 1);
	gpio_set_value(GPIO_LATCH, 0);
	//*gpio_sethigh = (1 << GPIO_CLOCK);
	//*gpio_setlow = (1 << GPIO_LATCH);
}

static void init_pins(void) {
	// set function of Pin GPIO_CLOCK (GPIO 17) to output
	// clear bits: 21, 22, 23
	gpio_direction_output(GPIO_CLOCK, 0);
	//*gpio_setfunction1 &= ~(7 << 21);
	// set bits: 21 (001 = OUTPUT)
	//*gpio_setfunction1 |= (1 << 21);
	// set function of Pin GPIO_LATCH (GPIO 27) to output
	// clear bits: 21, 22, 23
	gpio_direction_output(GPIO_LATCH, 0);
	//*gpio_setfunction2 &= ~(7 << 21);
	// set bits: 21 (001 = OUTPUT)
	//*gpio_setfunction2 |= (1 << 21);
	// set function of Pin GPIO_DATA (GPIO 22) to input 
	// clear bits: 3, 4, 5
	gpio_direction_input(GPIO_DATA);
	//*gpio_setfunction2 &= ~(7 << 3);
	// set bits: (000 = INPUT)
	// here we would set the bits but there is nothing to set
	// reset pins now
	reset_pins();
}

static inline void uninit_pins(void) {
	// all we want to do here is to make sure the output pins are toggled off
	gpio_set_value(GPIO_CLOCK, 0);
	gpio_set_value(GPIO_LATCH, 0);
	//*gpio_setlow = (1 << GPIO_CLOCK);
	//*gpio_setlow = (1 << GPIO_LATCH);
}

static void poll_snes(void) {
	int current_button = 0;
	uint16_t data = 0;


	// rise LATCH
	gpio_set_value(GPIO_LATCH, 1);
	//*gpio_sethigh = (1 << GPIO_LATCH);
	// wait for the controller to react
	usleep_range(12, 18);
	// pull latch down again
	//*gpio_setlow = (1 << GPIO_LATCH);
	gpio_set_value(GPIO_LATCH, 0);
	// wait for the controller to react
	usleep_range(6, 12);
	// begin a loop over the 16 clock cylces and poll the data

	for (current_button=0; current_button < 16; current_button++) {
		uint8_t gpio_state = gpio_get_value(GPIO_DATA);
		// we read (data in active low)
		if (gpio_state == 0) {
			printk(KERN_DEBUG "button %u has been pressed\n", current_button);
			data |= (1 << current_button);
		}

		// do one clockcycle (with delays for the controller)
		gpio_set_value(GPIO_CLOCK, 0);
		//*gpio_setlow = (1 << GPIO_CLOCK);
		usleep_range(6,12);
		gpio_set_value(GPIO_CLOCK, 1);
		//*gpio_sethigh = (1 << GPIO_CLOCK);
		usleep_range(6,12);
	}

	// now we're going to report the polled data (only 12 buttons, 4 undefined
	// will be ignored - they're pulled to ground anyways)
	for (current_button=0; current_button < 12; current_button++) {
		// if the button is a direction from the DPAD, then the data will get
		// special threatment
		if (current_button >= 3 && current_button <= 6) {
			switch (current_button) {
				case 3:
					if ((data & (1 << current_button)) != 0) {
						input_event(snes_dev, EV_ABS, button_mapping[current_button], -1);
						// if we press up the button down is not required to be checked anymore
						++current_button;
					}	
					continue;
				case 4:
					if ((data & (1 << current_button)) != 0) {
						input_event(snes_dev, EV_ABS, button_mapping[current_button], 1);
					} else {
						input_event(snes_dev, EV_ABS, button_mapping[current_button], 0);
					}
					continue;
				case 5:
					if ((data & (1 << current_button)) != 0) {
						input_event(snes_dev, EV_ABS, button_mapping[current_button], -1);
						// if we press left the button right is not required to be checked anymore
						++current_button;
					}	
					continue;
				case 6:
					if ((data & (1 << current_button)) != 0) {
						input_event(snes_dev, EV_ABS, button_mapping[current_button], 1);
					} else {
						input_event(snes_dev, EV_ABS, button_mapping[current_button], 0);
					}
					continue;
			}
		}

		if ((data & (1 << current_button)) != 0) {
			input_event(snes_dev, EV_KEY, button_mapping[current_button], 1);
		} else {
			input_event(snes_dev, EV_KEY, button_mapping[current_button], 0);
		}

	}

	input_sync(snes_dev);
}

static void fill_input_dev(void) {
	snes_dev->name = "SNES-Controller";
	// define EV_KEY type events
	set_bit(EV_KEY, snes_dev->evbit);
	set_bit(EV_ABS, snes_dev->evbit);
	set_bit(BTN_B, snes_dev->keybit);
	set_bit(BTN_Y, snes_dev->keybit);
	set_bit(BTN_SELECT, snes_dev->keybit);
	set_bit(BTN_START, snes_dev->keybit);
	set_bit(BTN_X, snes_dev->keybit);
	set_bit(BTN_A, snes_dev->keybit);
	set_bit(BTN_TL, snes_dev->keybit);
	set_bit(BTN_TR, snes_dev->keybit);
	set_bit(ABS_HAT0X, snes_dev->absbit);
	set_bit(ABS_HAT0Y, snes_dev->absbit);

	input_set_abs_params(snes_dev, ABS_HAT0X, -1, 1, 0, 0);
	input_set_abs_params(snes_dev, ABS_HAT0Y, -1, 1, 0, 0);
	
	// I could not find the product id
	snes_dev->id.bustype = BUS_HOST;
	snes_dev->id.vendor = 0x12E1;
	snes_dev->id.product = 0x0001;
	snes_dev->id.version = 0x001;
}


static int __init init_snes(void) {
	int error;

	printk(KERN_INFO "[SNES] initialising\n");
	printk(KERN_DEBUG "[SNES] Initialising GPIO-Pins...\n");
	init_pins();

	printk(KERN_DEBUG "[SNES] Allocating structure input_dev...\n");
	snes_dev = input_allocate_device();
	if (!snes_dev) {
		printk(KERN_ERR "snes.c: Not enough memory\n");
		return -ENOMEM;
	}

	printk(KERN_DEBUG "[SNES] Filling structure input_dev...\n");
	fill_input_dev();

	printk(KERN_DEBUG "[SNES] Allocating structure snes_polled_dev...\n");
	snes_polled_dev = input_allocate_polled_device();
	if (!snes_polled_dev) {
		printk(KERN_ERR "snes.c: Not enough memory\n");
		return -ENOMEM;
	}

	// when you are using input.h, you should define open() and close()
	// functions when your devices needs to be polled, so the polling
	// begins when the virtual joystick file is being read
	// luckily it is already implemented in input-polldev

	printk(KERN_DEBUG "[SNES] Filling structure snes_polled_dev...\n");
	snes_polled_dev->poll = (void*)poll_snes;
	snes_polled_dev->poll_interval = 16; // msec ~ 60 HZ
	snes_polled_dev->poll_interval_max = 32;
	snes_polled_dev->input = snes_dev;

	printk(KERN_DEBUG "[SNES] Registering polled device...\n");
	error = input_register_polled_device(snes_polled_dev);
	if (error) {
		printk(KERN_ERR "snes.c: Failed to register polled device\n");
		input_free_polled_device(snes_polled_dev);
		return error;
	}

	printk(KERN_INFO "[SNES] initialised\n");
	return 0;
}


static void __exit uninit_snes(void) {
	printk(KERN_INFO "[SNES] uninitialising\n");
	uninit_pins();
	input_unregister_polled_device(snes_polled_dev);
	input_free_polled_device(snes_polled_dev);
	input_free_device(snes_dev);
	printk(KERN_INFO "[SNES] Good Bye Kernel :'(\n");
}


module_init(init_snes);
module_exit(uninit_snes);