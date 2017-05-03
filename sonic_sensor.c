 
/* Necessary includes for device drivers */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/jiffies.h> /* jiffies */
#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */

#include <asm/gpio.h>
#include <asm/irq.h>
#include <linux/sched.h>
#include <linux/ctype.h>
#include <asm/checksum.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/arch/pxa-regs.h>
#include <linux/interrupt.h>
#include <asm-arm/arch/hardware.h>
#include <linux/delay.h>
#include <linux/time.h>
#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#define MYGPIO 117
#define MYGPIO1 118

MODULE_LICENSE("Dual BSD/GPL");

static void set_pwm(int set);
static int mygpio_open(struct inode *inode, struct file *filp);
static int mygpio_release(struct inode *inode, struct file *filp);
static irqreturn_t gpio_irq(int irq, void *dev_id, struct pt_regs *regs);
static irqreturn_t gpio_irq1(int irq, void *dev_id, struct pt_regs *regs);
static int mygpio_init(void);
static void mygpio_exit(void);
static void timer_1s(unsigned long data);
char name[2][128];
int touserdata;
unsigned long starttime;
unsigned long time[2];
unsigned long loadtime;
int pid[2];
char comm[2][128];
int exist[2];
int num,set;
char tbuf[256];
int n,count,diff;
int init_time,led[4],btn[2];
struct timeval tv;
struct timeval tv1;

static int mygpio_major = 61;
struct file_operations mygpio_fops = {
	open: mygpio_open,
	release: mygpio_release

};

module_init(mygpio_init);
module_exit(mygpio_exit);

static struct timer_list * fasync_timer;

/********88888888**************************/
static irqreturn_t gpio_irq(int irq, void *dev_id, struct pt_regs *regs)
{
	//time[1]=jiffies;
	//printk(KERN_INFO "rec %ld \n", jiffies);
	//printk(KERN_INFO "Get! %ld \n", time[1]-time[0]);
	do_gettimeofday(&tv);
	//printk(KERN_ALERT "usec1 %d\n", tv.tv_usec);	
	return IRQ_HANDLED;
}
static irqreturn_t gpio_irq1(int irq, void *dev_id, struct pt_regs *regs)
{
	//time[1]=jiffies;
	//printk(KERN_INFO "rec %ld \n", jiffies);
	//printk(KERN_INFO "Get! %ld \n", time[1]-time[0]);
	do_gettimeofday(&tv1);
	//printk(KERN_ALERT "usec2 %d\n", tv1.tv_usec);
	diff = 	tv1.tv_usec-tv.tv_usec;
	if (diff <1350){
		set = 0;
	}		
	else if (diff>1350 && diff <4000){
		set = 1;	
	}
	else if (diff>4000 && diff <9000){
		set = 2;
	}
	else {
		set = 3;
	}
	printk(KERN_ALERT "diff %d\n\n", tv1.tv_usec-tv.tv_usec);
	return IRQ_HANDLED;
}



static int mygpio_init(void) {
	int result;
	int irq;
	int irq1;
	num=1500;
	n=2;
	printk(KERN_INFO "Inserting mygpio module\n");
	fasync_timer = (struct timer_list *) kmalloc(sizeof(struct timer_list), GFP_KERNEL);	
	result = register_chrdev(mygpio_major, "mygpio", &mygpio_fops);
	if (result < 0)
	{
		printk(KERN_ALERT
			"mygpio: cannot obtain major number %d\n", mygpio_major);
		return result;
	}
	/* Check if all right */

	gpio_direction_input(117);
	gpio_direction_output(9,1);

	if (1) {
		pxa_gpio_mode(GPIO16_PWM0_MD);
		pxa_set_cken(CKEN0_PWM0, 1);
		PWM_CTRL0 = 0;
		PWM_PWDUTY0 = 0x3ff;
		PWM_PERVAL0 = 0x3ff;
	} else {
		PWM_CTRL0 = 0;
		PWM_PWDUTY0 = 0x0;
		PWM_PERVAL0 = 0x3FF;
		pxa_set_cken(CKEN0_PWM0, 0);
	}

	gpio_direction_input(17);
	gpio_direction_input(101);
	led[0] = 1;
	led[1] = 1;
	led[2] = 1;
	led[3] = 1;
	btn[0] = 0;
	btn[1] = 0;
	gpio_direction_output(28,led[0]);
	gpio_direction_output(29,led[1]);
	gpio_direction_output(30,led[2]);
	gpio_direction_output(31,led[3]);

	pxa_gpio_mode(MYGPIO | GPIO_IN);
		irq = IRQ_GPIO(MYGPIO);
	pxa_gpio_mode(MYGPIO1 | GPIO_IN);
		irq1=IRQ_GPIO(MYGPIO1);

	if (request_irq(irq, &gpio_irq, SA_INTERRUPT | SA_TRIGGER_RISING, "mygpio", NULL) != 0 ) {
                printk (KERN_INFO  "irq not acquired \n" );
                return -1;
        }else{
                printk (KERN_INFO  "irq %d acquired successfully \n", irq );

		setup_timer(fasync_timer, timer_1s, 1);
		mod_timer(fasync_timer, jiffies + msecs_to_jiffies(100));
	}

	if (request_irq(irq1, &gpio_irq1, SA_INTERRUPT | SA_TRIGGER_FALLING, "mygpio1", NULL) != 0 ) {
                printk (KERN_INFO  "irq1 not acquired \n" );
                return -1;
        }else{
                printk (KERN_INFO  "irq1 %d acquired successfully \n", irq1 );
	}
	pxa_gpio_mode(GPIO16_PWM0_MD);

	return 0;
}


static void mygpio_exit(void) {
	del_timer(fasync_timer);
	pxa_gpio_set_value(29,0);//IN 2
	pxa_gpio_set_value(31,0);//IN 4
	set_pwm(0);
	/* Freeing the major number */
	unregister_chrdev(mygpio_major, "mygpio");
//	/**/	remove_proc_entry("mygpio", &proc_root);
	//if (fasync_timer)
	//	kfree(fasync_timer);
	printk(KERN_ALERT "Removing sonic_sensor module\n");
	free_irq(IRQ_GPIO(MYGPIO), NULL);
	free_irq(IRQ_GPIO(MYGPIO1), NULL);

}

static int mygpio_open(struct inode *inode, struct file *filp) {
	return 0;
}

static int mygpio_release(struct inode *inode, struct file *filp) {
	return 0;
}

static void set_pwm(int set)
{
//	printk(KERN_ALERT"pwm_set(%d)\n", brightness);

	switch (set) 
	{
		case 3:	// Max brightness
			pxa_set_cken(CKEN0_PWM0, 1);
			PWM_CTRL0 = 0;
			PWM_PWDUTY0 = 0x3ff;
			PWM_PERVAL0 = 0x3ff;
			break;
		
		case 2: // middle brightness
	      		pxa_set_cken(CKEN0_PWM0, 1);
	      		PWM_CTRL0 = 0;
	      		PWM_PWDUTY0 = 0x3ff / 1.5;
	      		PWM_PERVAL0 = 0x3ff;
	      		break;

		case 1: // low brightness
	      		pxa_set_cken(CKEN0_PWM0, 1);
	      		PWM_CTRL0 = 0;
	      		PWM_PWDUTY0 = 0x3ff / 2.5;
	      		PWM_PERVAL0 = 0x3ff;
	      		break;

		case 0: // Turn off
			PWM_CTRL0 = 0;
			PWM_PWDUTY0 = 0x0;
			PWM_PERVAL0 = 0x3FF;
			pxa_set_cken(CKEN0_PWM0, 0);
			break;
	}
}

static void timer_1s(unsigned long data) {
	int i,j,flag;
	int IN_1,IN_2,IN_3,IN_4;

	mod_timer(fasync_timer, jiffies + msecs_to_jiffies(100));
	btn[0]= pxa_gpio_get_value(17);
	btn[1]= pxa_gpio_get_value(101);
	if (btn[0]==0 ){
		if (diff <1350){
			set_pwm(0);
		}		
		else if (diff>1350 && diff <2500){
			set_pwm(1);	
		}
		else if (diff>2500 && diff <4000){
			set_pwm(2);	
		}
		else {
			set_pwm(3);
		}
	}
	else{
		set_pwm(0);
	}
	pxa_gpio_set_value(29,btn[1]);//IN 2
	pxa_gpio_set_value(31,btn[1]);//IN 4


	

	printk(KERN_INFO "flag is %d\n", flag);
	//printk(KERN_INFO "count is %d\n", count);
	//if (count> 5){
//	printk(KERN_INFO "1s\n");
	pxa_gpio_set_value(9, 0);
	udelay(20);
	pxa_gpio_set_value(9, 1);
	udelay(20);
	pxa_gpio_set_value(9, 0);
	//time[0]=jiffies;
	//printk(KERN_INFO "out\n");
	//count =0;
	//}
	//else{
	//	count++;
	//}
	
}



