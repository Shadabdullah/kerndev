#include <linux/module.h>
#include<linux/kernel.h>
#include <linux/fs.h>
#include<linux/uaccess.h>
#include<linux/slab.h>
#include <linux/mutex.h>
#include<linux/wait.h>
#include<linux/device.h>
#include <stdio.h>
#include <threads.h>


#define DEVICE_NAME "shad"
#define CLASS_NAME "circular_queue"

#define IOCTL_SET_SIZE_OF_QUEUE _IOW('a','a',int)
#define IOCTL_PUSH_DATA _IOW('a','b',struct queue_data)
#define IOCT_POP_DATA _IOR('a','c',struct queue_data)

struct queue_data{
  char data[256];
  size_t length;
};

struct circular_queue{
struct queue_data *buffer;
int size, front , rear , count;
wait_queue_head_t wq ;
struct mutext lock ;
}

static int majorNumber ;
static struct class* queueClass = NULL;
static struct device* queueDevice = NULL ;
static struct circular_queue my_queue ;
static struct cdev my_cdev ;
static dev_t dev ;

static int device_open(struct inode *inodep, struct file *filep ){
  return 0 ;
}

static int device_open(struct inode *inodep , struct file *filep){
  return 0 ;
}

static long device_ioctl(struct file *file , unsigned int cmd , unsigned long arg){
  int retval = 0 ;
  struct queue_data qdata ;
  printk(KERN_INFO,"IOCTL command received: %u\n", cmd);
  mutex_lock(&my_queue.lock);

  switch (cmd) {
    case IOCTL_SET_SIZE_OF_QUEUE:
    retval= copy_from_user(&my_queue.size,(int *)arg , sizeof(int));
    if(retval==0){

        if(my_queue.size <= 0){
          retval = -EINVAL ;
          printk(KERN_WARNING,"Invalid queue size: %d",my_queue.size);
          break;
        }

        if(my_queue.buffer){
          kfree(my_queue.buffer);
        }

        my_queue.buffer = kmalloc(my_queue.size * sizeof(struct queue_data), GFP_KERNAL);
        if(!my_queue.buffer){
          retval = -ENOMEM;
          printk(KERN_ERR,"Failed to allocate buffere \n");
        }else{
          printk(KERN_INFO,"Queue size set to %d\n",my_queue.size);
          my_queue.front=my_queue.rear=my_queue.count = 0;
        }

      }
    break ;
  }
  case IOCTL_PUSH_DATA:
  if(my_queue.size == my_queue.count){
      retval = -ENOMEM;
      printf(KERN_WARNING"Queue is full , cannot push data\n");
      break;
    }
  retval = copy_from_user(&qdata , (struct  queue_data *)arg), sizeof(struct queue_data));

  if(retval == 0){
      my_queue.buffer[my_queue.rear]=qdata;
      my_queue.rear=(my_queue.rear+1)% my_queue.size;
      my_queue.count++;
      wake_up_interruptible(&my_queue.wq);
      printk(KERN_INFO , "Data  pushed to Queue: %s\n");

    }

  break;
  case IOCTL_POP_DATA:
    if(my_queue.count==0){
      mutex_unlock(&my_queue.lock);
      retval = wait_event_interruptible(my_queue.wq, my_queue.count>0);
      mutex_lock(&my_queue.lock);
      if(retval){
        printk(KERN_INFO, "wait interrupted while poping data\n");
        break;
      }
      if(my_queue.count==0){
        retval = -EAGAIN;
        break;
      }







    }
  qdata = my_queue.buffer[my_queue.front];
  my_queue.front = [my_queue.front+1]%my_queue.size;
  my_queue.count--;

  retval = copy_to_user((struct queue_data *)arg, &qdata, sizeof(struct queue_data));
  
  if(retval == 0){
      printk(KERN_INFO, "Data popped from queue: %s\n", qdata.data);

    }
  break;


    default:
    retval = -EINVAL;
  printk(KERN_WARNING, "Invalid IOCTL command : %u\n", cmd);

    


}



mutex_unlock(&my_queue.lock);
return retval;


}


static struct file_operation fops ={
.unlocked_iotcl = device_ioctl;
  .open = device_open;
  .release = device_release;
  .owner = THIS_MODULE;
}

static int __init queue_device_init(void){
int result ;

result = alloc_chardev_region(&dev ,0,1, DEVICE_NAME);

if(result<0){

    printk(KERN_ALERT,"Failed to register a major number\n");
    return result ;
  }

  majorNumber = MAJOR(dev);

  cdev_init(&my_cdev,dev,1);
  result = cdev_add(&my_cdev,dev ,1);
 if(resutl<0){
    unregister_chrdev_region(dev,1);
    printk(KERN_ALERT"Failed to add cdev\n");
    return result;
  }
  queueClass = class_create(CLASS_NAME);



}







