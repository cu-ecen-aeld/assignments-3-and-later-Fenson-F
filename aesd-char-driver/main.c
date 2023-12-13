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


#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/errno.h>

#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Fenson-F"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{   
     /**
     * TODO: handle open
     */

    PDEBUG("open");

    struct aesd_dev *dev;

    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);

    filp->private_data = dev; 
   
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */

     //nothing to add since flip is a pointer
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    //my vars
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *eptr;
    size_t cbuffer_offset = 0;
    int readsize = count;
    ssize_t ret;

    if(*f_pos >= dev->size) {
        goto out;
    }
    
    ret=mutex_lock_interruptible(&dev->lock);

    if(ret != 0) {
        PDEBUG("Failure: Mutex could not lock \n");
        retval = -ERESTARTSYS;
        goto out;
    }

    //find offset
    eptr = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->cbuffer, *f_pos, &cbuffer_offset);
    if(eptr == NULL)
    {
        PDEBUG("Failure: Could not find entry offset in read \n");
        mutex_unlock(&dev->lock);
        goto out;
    }

    //calculate size to read and comparing it to size counted to avoid reading more then need
    //using cbuffer_offset rather then *f_pos as shown in textbook example
    if(cbuffer_offset + count > dev->size) {
        readsize = dev->size - cbuffer_offset;
    }

    ret = copy_to_user(buf, eptr->buffptr+cbuffer_offset, readsize);
    if (ret != 0){
        PDEBUG("Failure: Could not copy to user in read \n");
        mutex_unlock(&dev->lock);
        retval = -EFAULT;
        goto out;
    }

    mutex_unlock(&dev->lock);
    *f_pos += count;
    PDEBUG("Success: aesd_read read %d bytes of %ld\n", readsize, count);
    return count;
    
    out:
        return retval;

}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write

     copy_from_user (__user,count);
     */
    struct aesd_dev *dev;
    struct aesd_buffer_entry new_entry;
    char *userdata;
    ssize_t ret;
    size_t index = 0;
    size_t entry_size = 0;
    ssize_t newlinefound =  0;

    dev = filp->private_data;
    //get data from user first
    userdata = kmalloc(count, GFP_KERNEL);
    if(userdata == NULL){
        PDEBUG("Failure: Could not allocate memory for write data \n");
        goto out;
    }

    ret = copy_from_user(userdata, buf, count);
    if(ret != 0){
        retval = -EFAULT;
        kfree(userdata);
        goto out;
    }

    ret=mutex_lock_interruptible(&dev->lock);
    if(ret != 0) {
        PDEBUG("Failure: Mutex could not lock \n");
        retval = -ERESTARTSYS;
        goto out;
    }
    //check for newline

    while((index < count)){
        if(userdata[index] == '\n'){
            newlinefound = 1;
            break;
        }

        //still want to add 1 even if \n has been found to account for it
        index++;
    }

    if(newlinefound == 1){
        entry_size = index +1;
    }
    else {
        entry_size = count;
    }

    //reallocate buffer now that size is known

    if(dev->size == 0) {
        dev->wbuffer = kmalloc(entry_size, GFP_KERNEL);
        if( dev->wbuffer== NULL) {
        PDEBUG("Failure: Could not allocate buffer size \n");
        kfree(userdata);
        mutex_unlock(&dev->lock);
        goto out;}
    } 
    else {
        dev->wbuffer = krealloc(dev->wbuffer, (dev->size + entry_size), GFP_KERNEL);
        if( dev->wbuffer== NULL) {
            PDEBUG("Failure: Could not reallocate buffer size \n");
            kfree(userdata);
            mutex_unlock(&dev->lock);
            goto out;
        } 
    }

    memcpy(dev->wbuffer + dev->size, userdata, entry_size);
    dev->size += entry_size;
    //kfree(userdata);

    if(newlinefound == 1){

        new_entry.size = dev->size;
        new_entry.buffptr=dev->wbuffer;

        aesd_circular_buffer_add_entry(&dev->cbuffer, &new_entry);
        //kfree(new_entry);

        //dev->wbuffer= NULL;
        dev->size = 0;
        newlinefound = 0;

    }

    retval = entry_size;

    *f_pos += retval;

    mutex_unlock(&dev->lock);
    
    if(userdata!=NULL){
        kfree(userdata);
    }
    return retval;

    out:
    if(userdata!=NULL){
        kfree(userdata);
    }
    return retval;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
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
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */

    mutex_init(&aesd_device.lock);
    aesd_circular_buffer_init(&aesd_device.cbuffer);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    mutex_destroy(&aesd_device.lock);

    //from aesd-circular-buffer header
    uint8_t index;
    struct aesd_buffer_entry *eptr;

    AESD_CIRCULAR_BUFFER_FOREACH(eptr,&aesd_device.cbuffer,index) { 
            kfree(eptr->buffptr);
    }

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
