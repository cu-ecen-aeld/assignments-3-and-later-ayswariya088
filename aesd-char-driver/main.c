/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major = 0; // use dynamic major
int aesd_minor = 0;

MODULE_AUTHOR("Ayswariya Kannan");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                  loff_t *f_pos)
{
    ssize_t retval = 0;

    // entry and offset for circular buffer
    struct aesd_buffer_entry *read_index = NULL; // var to saveaddress of the read index returned from reading function
    ssize_t read_offset = 0;                     // offset for reading is zero in the case
    ssize_t unread_count = 0;

    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);

    // get the skull device from file structure
    dev = (struct aesd_dev *)filp->private_data;

    // put error checks here, if count is zero, all other parameters are not null
    if (filp == NULL || buf == NULL || f_pos == NULL)
    {
        return -EFAULT; // bad address
    }

    // lock on mutex here, preferrable interruptable, check for error
    if (mutex_lock_interruptible(&dev->lock))
    {
        PDEBUG(KERN_ERR "could not acquire mutex lock");
        return -ERESTARTSYS; // check this
    }
    // find the read entry, and offset for given f_pos
    read_index = aesd_circular_buffer_find_entry_offset_for_fpos(&(dev->circle_buff), *f_pos, &read_offset);
    if (read_index == NULL)
    {
        goto error_path;
    }
    else
    {

        /*check if count is greater that current max read size, then limit
          max_read_size = entry_size - read_offset
        */
        if (count > (read_index->size - read_offset))
            count = read_index > size - read_offset;
    }
    // now read using copy_to_user
    unread_count = copy_to_user(buf, (read_index->buffptr + read_offset), count);

    // return whatever is copied and update fpos accordingly
    retval = count - unread_count;
    *f_pos += retval;

error_path:
    mutex_unlock(&(dev->lock));

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                   loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);
    struct aesd_dev *dev;
    // const char *write_entry = NULL;
    ssize_t unwritten_count = 0;
    // check arguement errors
    if (count == 0)
        return 0;
    if (filp == NULL || buf == NULL || f_pos == NULL)
        return -EFAULT;

    // cast the aesd_device from private data
    dev = (struct aesd_dev *)filp->private_data;

    // lock the mutex
    if (mutex_lock_interruptible(&(dev->lock)))
    {
        PDEBUG(KERN_ERR "could not acquire mutex lock");
        return -ERESTARTSYS;
    }

    // allocate the buffer using kmalloc, store address in buffptr
    if (dev->circle_buff_entry.size == 0)
    {

        dev->circle_buff_entry.buffptr = kmalloc(count * sizeof(char), GFP_KERNEL);

        if (dev->circle_buff_entry.buffptr == NULL)
        {
            PDEBUG("kmalloc error");
            goto error_path_write;
        }
    }
    // realloc if already allocated
    else
    {

        dev->circle_buff_entry.buffptr = krealloc(dev->circle_buff_entry.buffptr, (dev->circle_buff_entry.size + count) * sizeof(char), GFP_KERNEL);

        if (dev->circle_buff_entry.buffptr == NULL)
        {
            PDEBUG("krealloc error");
            goto exit_error;
        }
    }

    // copy data from user space buffer to current command
    unwritten_count = copy_from_user((void *)(dev->circle_buff_entry.buffptr + dev->circle_buff_entry.size),
                                     buf, count);
    retval = count - unwritten_count; // actual bytes written
    dev->circle_buff_entry.size += retval;

    // Check for \n in command, if found add the entry in circular buffer
    if (memchr(dev->circle_buff_entry.buffptr, '\n', dev->circle_buff_entry.size))
    {

        aesd_circular_buffer_add_entry(&dev->circle_buff, &dev->circle_buff_entry);

        // clear entry parameters
        dev->circle_buff_entry.buffptr = NULL;
        dev->circle_buff_entry.size = 0;
    }

    PDEBUG("not doing k_free for now");

    // handle errors
error_path_write:
    mutex_unlock(&dev->lock);

    return retval;
}
struct file_operations aesd_fops = {
    .owner = THIS_MODULE,
    .read = aesd_read,
    .write = aesd_write,
    .open = aesd_open,
    .release = aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}

int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
                                 "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0)
    {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device, 0, sizeof(struct aesd_dev));

    // Initialize the mutex and circular buffer
    mutex_init(&aesd_device.lock);
    aesd_circular_buffer_init(&aesd_device.circle_buff);

    result = aesd_setup_cdev(&aesd_device);

    if (result)
    {
        unregister_chrdev_region(dev, 1);
    }
    return result;
}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    // free circular buffer entries
    struct aesd_buffer_entry *entry = NULL;
    uint8_t index = 0;
    // free the buff_entry buffpte
    kfree(aesd_device.circle_buff_entry.buffptr);

    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.circle_buff, index)
    {
        if (entry->buffptr != NULL)
        {
            kfree(entry->buffptr);
        }
    }
    unregister_chrdev_region(devno, 1);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
