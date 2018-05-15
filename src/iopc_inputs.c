#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <linux/platform_device.h>

#include <asm/uaccess.h>
#include <asm/io.h>

MODULE_LICENSE("Dual BSD/GPL");

#define SET_KEYBOARD_KEY	0x10
#define SET_MOUSE_KEY		0x11

struct iopc_input_t {
	uint8_t dev_type;
	uint8_t input_key;
};

static struct input_dev *iopc_input_dev;
static struct platform_device *iopc_dev;

static long iopc_input_ioctl(struct file *fp, unsigned int cmd, unsigned long __arg)
{
	void __user *argp = (void __user *)__arg;
	struct iopc_input_t val;

	switch(cmd) {
		case SET_KEYBOARD_KEY:
			copy_from_user(&val, argp, sizeof(val));
			printk("keyboard-dev_type:%x, key:%x\n", val.dev_type, val.input_key);
			break;
		case SET_MOUSE_KEY:
			copy_from_user(&val, argp, sizeof(val));
			printk("mouse-dev_type:%x, key:%x\n", val.dev_type, val.input_key);
			break;
		default:
			printk("default-dev_type:%x, key:%x\n", val.dev_type, val.input_key);
			break;
	}
	return 0;
}

#define TOUCH		1
#define MOUSE		2
#define KEYBOARD	3
static ssize_t write_virmouse(struct device *dev, struct device_attribute *attr, const char *buffer, size_t count)
{
	int class, x, y, key;
	sscanf(buffer, "%d%d%d%d", &class, &x, &y, &key);
	switch(class) {
		case TOUCH:
			if(key <= 0)
				break;
			if (key == 1) {
				input_report_key(iopc_input_dev, BTN_TOUCH, 0);
				input_report_abs(iopc_input_dev, ABS_X, x);
				input_report_abs(iopc_input_dev, ABS_Y, y);
			}
			if (key == 2) {
				input_report_abs(iopc_input_dev, ABS_X, x);
				input_report_abs(iopc_input_dev, ABS_Y, y);
				input_report_key(iopc_input_dev, BTN_TOUCH, 1);
			}
			break;
		case MOUSE:
			if ( key <= 0)
				break;
			input_report_rel(iopc_input_dev, REL_X, x);
			input_report_rel(iopc_input_dev, REL_Y, y);
			if (key==1) {
				input_report_key(iopc_input_dev, BTN_LEFT, 1);//left click press down 
				input_report_key(iopc_input_dev, BTN_LEFT, 0);//left click up
			}
			if (key==2) {
				input_report_key(iopc_input_dev, BTN_MIDDLE, 1);
				input_report_key(iopc_input_dev, BTN_MIDDLE, 0);
			}
			if (key==3) {
				input_report_key(iopc_input_dev, BTN_RIGHT, 1);
				input_report_key(iopc_input_dev, BTN_RIGHT, 0);
			}
			if (key==4) {
				input_report_key(iopc_input_dev, BTN_LEFT, 1);//left click press down 
				input_report_key(iopc_input_dev, BTN_LEFT, 0);//left click up
				input_report_key(iopc_input_dev, BTN_LEFT, 1);//left click press down 
				input_report_key(iopc_input_dev, BTN_LEFT, 0);//left click up
			}
			if (key==5) {
				input_report_key(iopc_input_dev, BTN_MIDDLE, 1);
				input_report_key(iopc_input_dev, BTN_MIDDLE, 0);
				input_report_key(iopc_input_dev, BTN_MIDDLE, 1);
				input_report_key(iopc_input_dev, BTN_MIDDLE, 0);
			}
			if (key==6) {
				input_report_key(iopc_input_dev, BTN_RIGHT, 1);
				input_report_key(iopc_input_dev, BTN_RIGHT, 0);
				input_report_key(iopc_input_dev, BTN_RIGHT, 1);
				input_report_key(iopc_input_dev, BTN_RIGHT, 0);
			}
			break;
		case KEYBOARD:
			break;
		default:
			break;
	}
	input_sync(iopc_input_dev);

	return count;
}

static const struct file_operations iopc_input_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= iopc_input_ioctl,
};

static struct miscdevice iopc_input_miscdev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "iopcinput",
	.fops	= &iopc_input_fops, 

};

DEVICE_ATTR(iopcevent, 0644, NULL, write_virmouse);

static struct attribute *iopc_attrs[] = {
	        &dev_attr_iopcevent.attr,
		        NULL
};
static struct attribute_group iopc_attr_group = {
	        .attrs = iopc_attrs,
};

static int iopc_inputs_init(void)
{
	int err = 0;
	printk(KERN_INFO "iopc keyboard/mouse module init...\n");
	misc_register(&iopc_input_miscdev);

	iopc_dev = platform_device_register_simple("iopcmouse", -1, NULL, 0);
	if (!iopc_dev){
		err = -ENOMEM;
		return err;
	}
	sysfs_create_group(&iopc_dev->dev.kobj, &iopc_attr_group);

	iopc_input_dev = input_allocate_device();
	if(!iopc_input_dev) {
		printk(KERN_ERR "%s: Not enough memory for input device\n", __FILE__);
		err = -ENOMEM;
		return err;
	}

        iopc_input_dev->name = "iopc keyboard/mouse";
	iopc_input_dev->phys = "iopc";
        iopc_input_dev->id.vendor  = 0x1111;
        iopc_input_dev->id.product = 0x1001;
        iopc_input_dev->id.version = 0x0001;

        iopc_input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL) | BIT_MASK(EV_ABS);
        iopc_input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
        iopc_input_dev->keybit[BIT_WORD(BTN_LEFT)] = BIT_MASK(BTN_LEFT) | BIT_MASK(BTN_MIDDLE) | BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_TOUCH);
        iopc_input_dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y);
        iopc_input_dev->absbit[0] = BIT_MASK(ABS_X) | BIT_MASK(ABS_Y);

        //iopc_input_dev->open  = iopc_input_open;
        //iopc_input_dev->close = iopc_input_close;

        err = input_register_device(iopc_input_dev);


	return err;
}

static void iopc_inputs_exit(void)
{
	printk(KERN_INFO "Goodbye\n");
	input_unregister_device(iopc_input_dev);
}

module_init(iopc_inputs_init);
module_exit(iopc_inputs_exit);
