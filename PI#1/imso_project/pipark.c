#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/atomic.h>

#include "mfrc522_lib.h"
#include "pipark.h"

#define debug

#ifdef debug
    #define PDEBUG(fmt, args...) printk(fmt, ##args)
#endif

#define DEV_NAME "pipark"

#define MFRC_RST 5
#define MFRC_LED 27
#define MOTOR1 18
#define MOTOR2 23
#define MOTOR3 24
#define MOTOR4 25
#define US_TRIG 19
#define US_ECHO 26
#define LED_GREEN 20
#define LED_RED 16

const char auth[10] = {0xB3, 0x8D, 0xE9, 0x53, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

struct task_struct *us_task;

atomic_t barrier_flag = ATOMIC_INIT(0);

static byte uidBuf[10];
static byte uidBuf_comp[10];

int cw_signal[8][4] = {
    {1, 0, 0, 0}, 
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

int ccw_signal[8][4] = {
    {1, 0, 0, 1},
    {0, 0, 0, 1}, 
    {0, 0, 1, 1},
    {0, 0, 1, 0},
    {0, 1, 1, 0},
    {0, 1, 0, 0},
    {1, 1, 0, 0},
    {1, 0, 0, 0}
};

void motor_set_signal(int direction, int step) {
    int *signal = direction == 1 ? cw_signal[step] : ccw_signal[step];
    gpio_set_value(MOTOR1, signal[0]);
    gpio_set_value(MOTOR2, signal[1]);
    gpio_set_value(MOTOR3, signal[2]);
    gpio_set_value(MOTOR4, signal[3]);
}

void motor_rotate(int degree, int delay, int direction) {
    int step, i, j;
    int rpt = 8 * (degree / 45);

    for (i = 0; i < rpt; i++) {
        for (j = 0; j < 8; j++) {
            for (step = 0; step < 8; step++) {
            motor_set_signal(direction, step);
            udelay(delay);
            }
        }
    }
}

int us_get_dist(void) {

	struct timeval time;
	int Ts = 0,Tf = 0;
	int Td = 0;

	gpio_set_value(US_TRIG,0);
	udelay(5);
	gpio_set_value(US_TRIG,1);
	udelay(10);
	gpio_set_value(US_TRIG,0);

	while(gpio_get_value(US_ECHO) == 0)
	{
		do_gettimeofday(&time);
		Ts = time.tv_usec;
	}

	while(gpio_get_value(US_ECHO) == 1)
	{
		do_gettimeofday(&time);
		Tf = time.tv_usec;
	}


	Td = (Tf-Ts);
	return Td/58;
}

int us_detect(void *data) {
    int dist;
    while (!kthread_should_stop()) {
        dist = us_get_dist();
        printk("Distance : %d\n", dist);
        if (dist < 7) {
            if (atomic_read(&barrier_flag) == 1) {
                motor_rotate(90, 1500, 1);
                atomic_set(&barrier_flag, 0);
            }
            
        }
        msleep(500);
    }

    return 0;
}

void isAuthorized(void) {
    atomic_set(&barrier_flag, 1);
    gpio_set_value(LED_GREEN, 1);
    motor_rotate(90, 1500, 0);
    msleep(1000);
    gpio_set_value(LED_GREEN, 0);
}

void isUnauthorized(void) {
    gpio_set_value(LED_RED, 1);
    msleep(1000);
    gpio_set_value(LED_RED, 0);
}

static int pipark_open(struct inode *inode,struct file *filp) {
	return 0;
}

static int pipark_release(struct inode *inode,struct file *filp) {
    return 0;
}

static long pipark_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {

    long ret;
	
    switch (cmd) {
        case READ_GPIO:
            gpio_direction_input(arg);
            return gpio_get_value(arg);
            break;
        case WRITE_GPIO_0:
            gpio_direction_output(arg, 0);
            //gpio_set_value(arg, 0);
            break;
        case WRITE_GPIO_1:
            gpio_direction_output(arg, 1);
            //gpio_set_value(arg, 1);
            break;
        case WRITE_UID:
            memcpy(uidBuf_comp, uidBuf, 10);
            if (ret = copy_from_user(uidBuf, (const void *)arg, 10)) {
                printk("%ld bytes of uid copy failed\n", ret);
            }
            if (atomic_read(&barrier_flag) == 0) {
                if (*uidBuf == *auth) {
                    atomic_set(&barrier_flag, 1);
                    isAuthorized();
                }
                else {
                    isUnauthorized();
                }
            }
            break;
        /* case AUTHORIZED:
            gpio_set_value(LED_GREEN, 1);
            motor_rotate(90, 1500, 0);
            msleep(1000);
            gpio_set_value(LED_GREEN, 0);
            break;
        case UNAUTHORIZED:
            gpio_set_value(LED_RED, 1);
            msleep(1000);
            gpio_set_value(LED_RED, 0);
            break; */
        default:
            return -EINVAL;
    }
    return 0;
}


static struct file_operations pipark_fops = {
	.open = pipark_open,
	.release = pipark_release,
	.unlocked_ioctl = pipark_ioctl
};


static dev_t dev_num;
static struct cdev *cd_cdev;


static int pipark_init(void) {
	
	alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
    cd_cdev = cdev_alloc();
    cdev_init(cd_cdev, &pipark_fops);
    cdev_add(cd_cdev, dev_num, 1);

    gpio_request_one(MFRC_RST, GPIOF_OUT_INIT_LOW, "mfrc_rst");
    gpio_request_one(MOTOR1, GPIOF_OUT_INIT_LOW, "motor1");
	gpio_request_one(MOTOR2, GPIOF_OUT_INIT_LOW, "motor2");
	gpio_request_one(MOTOR3, GPIOF_OUT_INIT_LOW, "motor3");
	gpio_request_one(MOTOR4, GPIOF_OUT_INIT_LOW, "motor4");
    gpio_request_one(US_TRIG, GPIOF_OUT_INIT_LOW, "us_trig");
    gpio_request_one(US_ECHO, GPIOF_IN, "us_echo");
    gpio_request_one(LED_RED, GPIOF_OUT_INIT_LOW, "led_red");
    gpio_request_one(LED_GREEN, GPIOF_OUT_INIT_LOW, "led_white");

    us_task = kthread_create(us_detect, NULL, "us_daemon");
    if (IS_ERR(us_task)) {
        us_task = NULL;
    }
    wake_up_process(us_task);
   
    return 0;
}

static void pipark_exit(void) {

	cdev_del(cd_cdev);
    unregister_chrdev_region(dev_num, 1);
    gpio_free(MFRC_RST);
    gpio_free(MOTOR1);
    gpio_free(MOTOR2);
    gpio_free(MOTOR3);
    gpio_free(MOTOR4);
    gpio_free(US_TRIG);
    gpio_free(US_ECHO);

    if (us_task) {
        kthread_stop(us_task);
    }
}

module_init(pipark_init);
module_exit(pipark_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SeungHyun Cho");