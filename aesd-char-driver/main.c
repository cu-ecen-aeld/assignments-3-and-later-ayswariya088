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
 * @reference : http://www.bricktou.com/fs/read_writefixed_size_llseek_src.html  ,
 * https://github.com/cu-ecen-aeld/ldd3/blob/scullc-5-ubuntu/scull/main.c#L394
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h> // file_operations
#include <linux/uaccess.h>
#include "aesdchar.h"
#include "aesd_ioctl.h" //A-9 update

int aesd_major = 0; // use dynamic major
int aesd_minor = 0;

MODULE_AUTHOR("Ayswariya Kannan");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;
/*
 * @function	:  Open call to open the character device
 *
 * @param		:  inode:kernel inode structure,filp:kernel file structure passed
 * @return		:  exit status 0 on success
 *
 */
int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;
    PDEBUG("open");
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    return 0;
}
/*
 * @function	:  release system call
 *
 * @param		:  inode:kernel inode structure,filp:kernel file structure passed
 * @return		:  exit status 0 on success
 *
 */
int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");

    return 0;
}

/*
 * @function	:  read from file at position pos of size_t count
 *
 * @param		:  buf-pointer to store the data read from file,
 *                 count the number of bytes required to be read
 *                 f_pos offset location in kernel buffer from where data need to be read.
 * @return		:  retval :no of bytes successfully read
 *
 */
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                  loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *dev;
    // entry and offset for circular buffer
    struct aesd_buffer_entry *read_index = NULL; // variable to saveaddress of the read index returned from reading function
    ssize_t read_offset = 0;                     // offset for reading is zero in the case
    ssize_t unread_count = 0;                    // unread bytes

    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);

    // get the skull device from the passed file structure
    dev = (struct aesd_dev *)filp->private_data;

    // to check for error
    if (filp == NULL || buf == NULL || f_pos == NULL)
    {
        return -EFAULT;
    }

    // lock the mutex with interruptable option and check for error
    if (mutex_lock_interruptible(&dev->lock))
    {
        PDEBUG(KERN_ERR "mutex lock unsuccessful");
        return -ERESTARTSYS; // error condition
    }

    // find the read index to read from the file
    read_index = aesd_circular_buffer_find_entry_offset_for_fpos(&(dev->circle_buff), *f_pos, &read_offset);
    if (read_index == NULL)
    {
        goto error_path; // to unlock mutex
    }
    else
    {

        /*check if the count is greater that read size of the buffer,if it the
        make count = read_size - read_offset*/
        if (count > (read_index->size - read_offset))
            count = read_index->size - read_offset;
    }
    //  read using copy_to_user
    unread_count = copy_to_user(buf, (read_index->buffptr + read_offset), count);

    // return the read size and update the f_pos
    retval = count - unread_count;
    *f_pos += retval;

error_path:
    mutex_unlock(&(dev->lock));

    return retval;
}

/*
 * @function	:  read from file at position pos of size_t count
 *
 * @param		:  buf-pointer to store the data read from file,
 *                 count the number of bytes required to be read
 *                 f_pos offset location in kernel buffer from where data need to be read.
 * @return		:  retval :no of bytes successfully read
 *
 */
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                   loff_t *f_pos)
{
    struct aesd_dev *dev;
    const char *write_entry = NULL;
    ssize_t retval = -ENOMEM;
    ssize_t unwritten_count = 0;
    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);

    // check for errors
    if (count == 0)
        return 0;
    if (filp == NULL || buf == NULL || f_pos == NULL)
        return -EFAULT;

    // save the aesd_device data from private data
    dev = (struct aesd_dev *)filp->private_data;

    // lock the mutex
    if (mutex_lock_interruptible(&(dev->lock)))
    {
        PDEBUG(KERN_ERR "could not acquire mutex lock");
        return -ERESTARTSYS;
    }

    // if  buffer size is zero, then allocate the buffer using kmalloc, store address in buffptr
    if (dev->circle_buff_entry.size == 0)
    {
        PDEBUG("Allocating buffer");

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
            goto error_path_write;
        }
    }
    PDEBUG("writing to buffer");
    // copy data from user space buffer
    unwritten_count = copy_from_user((void *)(dev->circle_buff_entry.buffptr + dev->circle_buff_entry.size),
                                     buf, count);
    retval = count - unwritten_count; // actual bytes written
    dev->circle_buff_entry.size += retval;

    // if \n character found means end of packet,thus if found add the entry in circular buffer
    if (memchr(dev->circle_buff_entry.buffptr, '\n', dev->circle_buff_entry.size))
    {

        write_entry = aesd_circular_buffer_add_entry(&dev->circle_buff, &dev->circle_buff_entry);
        if (write_entry)
        {
            kfree(write_entry); // free the temporary pointer
        }
        // clear entry parameters
        dev->circle_buff_entry.buffptr = NULL;
        dev->circle_buff_entry.size = 0;
    }

    // handle errors
error_path_write:
    mutex_unlock(&dev->lock);

    return retval;
}
/*
 * @function	:  to adjust the filp->f_pos according to the offset sent
 *
 * @param		:  write_cmd : for the index no of xommand, write_cmd_offset :offset within the command
 * @return		:  retval :indicating error condition
 *
 */
static long aesd_adjust_file_offset(struct file *filp, unsigned int write_cmd, unsigned int write_cmd_offset)
{
    struct aesd_buffer_entry *buff_entry = NULL;
    struct aesd_dev *dev = NULL;
    uint8_t index = 0;
    long retval = 0;
    PDEBUG("AESDCHAR_IOCSEEKTO command implementation\n");
    if (filp == NULL)
    {

        return -EFAULT;
    }
    dev = filp->private_data;
    if (mutex_lock_interruptible(&(dev->lock)))
    {
        PDEBUG(KERN_ERR "could not acquire mutex lock");
        return -ERESTARTSYS;
    }
    AESD_CIRCULAR_BUFFER_FOREACH(buff_entry, &dev->circle_buff, index); // to get the last index value inside the buffer
    if (write_cmd > index || write_cmd_offset >= (dev->circle_buff.entry[write_cmd].size) || write_cmd > AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    {
        retval = -EINVAL;
    }
    else
    {
        if (write_cmd == 0)
        {
            filp->f_pos += write_cmd_offset;
        }
        else
        {
            for (index = 0; index < write_cmd; index++)
            {
                filp->f_pos += dev->circle_buff.entry[index].size;
            }
            filp->f_pos += write_cmd_offset;
        }
    }
    mutex_unlock(&dev->lock);
    return retval;
}
/*
 * @function	:  ioctl description for AESDCHAR_IOCSEEKTO command
 *
 * @param		: cmd :Command to be passed to ioctl defined in aesd_ioctl.h , arg :any arguments passed with the command
 * @return		:  retval :error condition
 *
 */
long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

    int err = 0;
    long retval = 0;
    struct aesd_seekto seekto;
    if (filp == NULL)
    {

        return -EFAULT;
    }
    PDEBUG("IOCTL sys call\n");
    /*
     * extract the type and number bitfields, and don't decode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */
    if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC)
        return -ENOTTY;
    if (_IOC_NR(cmd) > AESDCHAR_IOC_MAXNR)
        return -ENOTTY;

    /*
     * access_ok is kernel-oriented,for checking "read" and "write" access
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
    if (err)
        return -EFAULT;

    switch (cmd)
    {

    case AESDCHAR_IOCSEEKTO:
       
        if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto))) // check if command from userspace copied to kernel space
           { PDEBUG("Error while copying from userspace\n");
            retval = -EFAULT;  }                                                // if it returns non zero means error
        else
        {
            PDEBUG("Implementing AESDCHAR_IOCSEEKTO\n");
            retval = aesd_adjust_file_offset(filp, seekto.write_cmd, seekto.write_cmd_offset);
        }
        break;

    default: 
        return -ENOTTY;
    }
    return retval;
}
/*
 * @function	:  implementation of llseek in kernel space
 *
 * @param		: offset: offset to check for , whence : int to indicate which type of seek (SEEK_SET, SEEK_CUR, and SEEK_END)
 * @return		:  loff_t seek_pos: file position after seek
 *
 */
loff_t aesd_llseek(struct file *filp, loff_t offset, int whence)
{

    struct aesd_dev *dev = NULL;
    struct aesd_buffer_entry *seek_entry = NULL;
    loff_t buffer_size = 0;
    loff_t seek_pos = 0;
    uint8_t index = 0;

    PDEBUG("llseek implementation\n");

    // checking for error condition
    if (filp == NULL)
    {

        return -EFAULT;
    }

    dev = (struct aesd_dev *)filp->private_data;

    // lock the mutex
    if (mutex_lock_interruptible(&(dev->lock)))
    {
        PDEBUG(KERN_ERR "could not acquire mutex lock");
        return -ERESTARTSYS;
    }

    // getting size of buffer
    AESD_CIRCULAR_BUFFER_FOREACH(seek_entry, &dev->circle_buff, index)
    {
        buffer_size += seek_entry->size;
    }
        //have used the fixed_size_llseek() function 
    seek_pos = fixed_size_llseek(filp, offset, whence, buffer_size);

    mutex_unlock(&dev->lock);

    return seek_pos;
}
struct file_operations aesd_fops =
    {
        .owner = THIS_MODULE,
        .read = aesd_read,
        .write = aesd_write,
        .open = aesd_open,
        .release = aesd_release,
        .llseek = aesd_llseek,
        .unlocked_ioctl = aesd_ioctl};

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
/*
 * @function	: It is used to register the device and initialize the kernel structure
 *
 * @param		: NULL
 * @return		: return value of init function
 *
 */
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
/*
 * @function	: unregister device and deallocated all the kernel data structures
 * @param		: NULL
 * @return		: NULL
 *
 */
void aesd_cleanup_module(void)
{
    // free circular buffer entries
    struct aesd_buffer_entry *entry = NULL;
    uint8_t index = 0;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    // free the circle_buff_entry buffptr
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
