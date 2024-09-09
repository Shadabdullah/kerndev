
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/slab.h>

#define DEVICE_NAME "shad"
#define CLASS_NAME "circular_queue_character_device"

#define IOCTL_SET_SIZE_OF_QUEUE _IOW('a', 'a', int *)
#define IOCTL_PUSH_DATA _IOW('a', 'b', struct data *)
#define IOCTL_POP_DATA _IOR('a', 'c', struct data *)

struct data {
    int length;
    char *data;
};

struct queue_character_device {
    char **queue;
    int *lengths;
    int front, rear, current_size_of_queue, maxsize_of_queue;
    int total_data_length;
    wait_queue_head_t queue_wait_queue;
    struct mutex queue_mutex_lock;
};

static int major;
static struct class *queue_class = NULL;
static struct device *queue_device = NULL;
static struct queue_character_device *queue_char_device = NULL;

static int device_open(struct inode *inode, struct file *file) {
    file->private_data = queue_char_device;
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    return 0;
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct queue_character_device *queue_char_device = file->private_data;
    struct data user_data;
    int returned_value  = 0;

    switch (cmd) {
        case IOCTL_SET_SIZE_OF_QUEUE:
            mutex_lock(&queue_char_device->queue_mutex_lock);

            if (copy_from_user(&queue_char_device->maxsize_of_queue, (int *)arg, sizeof(int))) {
                mutex_unlock(&queue_char_device->queue_mutex_lock);
                return -EFAULT;
            }

            queue_char_device->queue = kmalloc_array(queue_char_device->maxsize_of_queue, sizeof(char *), GFP_KERNEL);
            queue_char_device->lengths = kmalloc_array(queue_char_device->maxsize_of_queue, sizeof(int), GFP_KERNEL);
            queue_char_device->front = queue_char_device->rear = queue_char_device->current_size_of_queue = queue_char_device->total_data_length = 0;

            mutex_unlock(&queue_char_device->queue_mutex_lock);
            break;

        case IOCTL_PUSH_DATA:
            wait_event_interruptible(queue_char_device->queue_wait_queue, queue_char_device->current_size_of_queue < queue_char_device->maxsize_of_queue);

            mutex_lock(&queue_char_device->queue_mutex_lock);
            if (copy_from_user(&user_data, (struct data *)arg, sizeof(struct data))) {
                mutex_unlock(&queue_char_device->queue_mutex_lock);
                return -EFAULT;
            }

            queue_char_device->queue[queue_char_device->rear] = kmalloc(user_data.length, GFP_KERNEL);
            if (copy_from_user(queue_char_device->queue[queue_char_device->rear], user_data.data, user_data.length)) {
                kfree(queue_char_device->queue[queue_char_device->rear]);
                mutex_unlock(&queue_char_device->queue_mutex_lock);
                return -EFAULT;
            }

            queue_char_device->lengths[queue_char_device->rear] = user_data.length;
            queue_char_device->total_data_length += user_data.length;
            queue_char_device->rear = (queue_char_device->rear + 1) % queue_char_device->maxsize_of_queue;
            queue_char_device->current_size_of_queue++;

            mutex_unlock(&queue_char_device->queue_mutex_lock);
            wake_up_interruptible(&queue_char_device->queue_wait_queue);

            break;

        case IOCTL_POP_DATA:
            if (copy_from_user(&user_data, (struct data *)arg, sizeof(struct data))) {
                return -EFAULT;
            }

            returned_value = wait_event_interruptible(queue_char_device->queue_wait_queue, queue_char_device->total_data_length >= user_data.length);
            if (returned_value)
                return returned_value;

            mutex_lock(&queue_char_device->queue_mutex_lock);

            int remaining = user_data.length;
            int copied = 0;
            char *user_buf = user_data.data;

            while (remaining > 0) {
                int to_copy = min(remaining, queue_char_device->lengths[queue_char_device->front]);
                
                if (copy_to_user(user_buf + copied, queue_char_device->queue[queue_char_device->front], to_copy)) {
                    mutex_unlock(&queue_char_device->queue_mutex_lock);
                    return -EFAULT;
                }

                copied += to_copy;
                remaining -= to_copy;
                queue_char_device->total_data_length -= to_copy;

                if (to_copy == queue_char_device->lengths[queue_char_device->front]) {
                    kfree(queue_char_device->queue[queue_char_device->front]);
                    queue_char_device->front = (queue_char_device->front + 1) % queue_char_device->maxsize_of_queue;
                    queue_char_device->current_size_of_queue--;
                } else {
                    memmove(queue_char_device->queue[queue_char_device->front], queue_char_device->queue[queue_char_device->front] + to_copy, queue_char_device->lengths[queue_char_device->front] - to_copy);
                    queue_char_device->lengths[queue_char_device->front] -= to_copy;
                }
            }

            mutex_unlock(&queue_char_device->queue_mutex_lock);
            wake_up_interruptible(&queue_char_device->queue_wait_queue);
            break;

        default:
            return -EINVAL;
    }

    return returned_value;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
};

static int __init queue_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "some error occurred while registering major number\n");
        return major;
    }

    queue_class = class_create(CLASS_NAME);
    if (IS_ERR(queue_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(queue_class);
    }

    queue_device = device_create(queue_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(queue_device)) {
        class_destroy(queue_class);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(queue_device);
    }

    queue_char_device = kmalloc(sizeof(struct queue_character_device), GFP_KERNEL);
    init_waitqueue_head(&queue_char_device->queue_wait_queue);
    mutex_init(&queue_char_device->queue_mutex_lock);

    printk(KERN_INFO "Circular Queue device registeration done\n");
    return 0;
}

static void __exit queue_exit(void) {
    device_destroy(queue_class, MKDEV(major, 0));
    class_unregister(queue_class);
    class_destroy(queue_class);
    unregister_chrdev(major, DEVICE_NAME);
    kfree(queue_char_device);
    printk(KERN_INFO "Circula queue device  unregistered\n");
}

module_init(queue_init);
module_exit(queue_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shad");
MODULE_DESCRIPTION("A dynamic circular queue character device with fully blocking feature");
MODULE_VERSION(".1");




