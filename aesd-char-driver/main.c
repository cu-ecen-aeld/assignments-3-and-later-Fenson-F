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
#include "aesd_ioctl.h"
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

    PDEBUG("open \n");
    //printk("Open \n");

    struct aesd_dev *dev;

    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);

    filp->private_data = dev; 
   
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("Release \n");
    /**
     * TODO: handle release
     */

     //nothing to add since flip is a pointer
    
    //printk("Release \n");
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("Read %zu bytes with offset %lld \n",count,*f_pos);
    /**
     * TODO: handle read
     */
    //my vars
    
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *cur_entry_ptr;
    size_t circular_buf_offset;
    ssize_t ret, uncopiedsize;
    int readsize=0;

    //if(*f_pos >= dev->size) {
    //    goto out;
    //}
    
    ret=mutex_lock_interruptible(&dev->lock);
    if(ret != 0) {
        PDEBUG("Failure: Mutex could not lock in read\n");
        //printk(KERN_ALERT, "Failure: Mutex could not lock in read \n");
        retval = -ERESTARTSYS;
        return retval; 
    }

    //find offset
    cur_entry_ptr = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->circular_buffer, *f_pos, &circular_buf_offset);
    if(cur_entry_ptr==NULL)
    {
        PDEBUG("Failure: Null pointer, bad read\n");
        //printk(KERN_ALERT, "Failure: Mutex could not lock in read \n");
        *f_pos = 0;
        mutex_unlock(&dev->lock);
        retval = 0;
        return retval; 
    }
        


    //calculate size to read and compare it to size counted
    //If count is larger, copy ALL. Else, copy max allowed by count

    readsize = cur_entry_ptr->size - circular_buf_offset;

    if(readsize < count) {

        PDEBUG("Read was less then count copying all \n");
        uncopiedsize  = copy_to_user(buf, cur_entry_ptr->buffptr+circular_buf_offset, readsize);
        
        if (uncopiedsize  != 0){
            PDEBUG("Failure: %lu uncopied bytes \n", uncopiedsize);
            //printk(KERN_ALERT, "Failure: %d uncopied bytes \n", uncopiedsize);
            mutex_unlock(&dev->lock);
            retval = -EFAULT;
            return retval;
        }

        retval=readsize;

    }
    else {

        PDEBUG("Read was more then count copying max \n");
        uncopiedsize  = copy_to_user(buf, cur_entry_ptr->buffptr+circular_buf_offset, count);
        if (uncopiedsize  != 0){
            PDEBUG("Failure: %lu uncopied bytes \n", uncopiedsize);
            //printk(KERN_ALERT, "Failure: %d uncopied bytes \n", uncopiedsize);
            mutex_unlock(&dev->lock);
            retval = -EFAULT;
            return retval;
        }
        
        retval = count;
    }

    *f_pos += retval;
    

    mutex_unlock(&dev->lock);
    
    PDEBUG("Success: aesd_read \n");
    //printk(KERN_ALERT, "Success: aesd_read read %d bytes\n", readsize);
    return retval;

}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{

    PDEBUG("write %zu bytes with offset %lld \n",count,*f_pos);
    /**
     * TODO: handle write

     copy_from_user (__user,count);
     */
    
    ssize_t retval = -ENOMEM;
    struct aesd_dev *dev = filp->private_data;
    char *userdata;
    ssize_t ret;
    //size_t entry_size = 0;
    int newlinefound = 0;
    int newline_index;
    int len_write;
    
    PDEBUG("Write %zu bytes with offset %lld \n",count,*f_pos);

    userdata = kmalloc(count, GFP_KERNEL);
    if(userdata == NULL){
        PDEBUG("Failure: Could not allocate memory for write data \n");
        //printk(KERN_ALERT, "Failure, could not allocate memory for write data \n");
        return retval;
    }

    ret = copy_from_user(userdata, buf, count); //check if copy_from_user is okay
    if(ret != 0){
        PDEBUG("Failure: Could not copy data from user \n");
        //printk(KERN_ALERT, "Failure, could not allocate memory for write data \n");
        retval = -EFAULT;
        kfree(userdata);
        return retval;
    }

    ret=mutex_lock_interruptible(&dev->lock); //CHECK DEV LOCK
    if(ret != 0) {
        PDEBUG("Failure: Mutex could not lock \n");
        //printk(KERN_ALERT, "Failure: could not lock mutex \n");
        retval = -ERESTARTSYS;
        return retval;
    }

    dev->wr_buffer = krealloc(dev->wr_buffer, (dev->wr_bsize + count), GFP_KERNEL);
    if( dev->wr_buffer== NULL) {
        PDEBUG("Failure: Could not reallocate buffer size \n");
        //printk(KERN_ALERT, "Failure: Could not reallocate buffer size for write buffer entry \n");
        kfree(userdata);
        mutex_unlock(&dev->lock);
        return retval;
    } 

    newline_index = 0;

    while(newline_index < count )
    {
        if (userdata[newline_index] == '\n')
        {
            newlinefound = 1;
            len_write = newline_index + 1;
            break;
        }
        newline_index++;
    }

    if(newlinefound == 0) {
        len_write = count;
    }
    else {
        // should be already written, but double check
        len_write = newline_index + 1;
    }

    if(dev->wr_bsize == 0){
        dev->wr_buffer = kmalloc(len_write, GFP_KERNEL);
        if(dev->wr_buffer == NULL) {
            kfree(userdata);
            mutex_unlock(&dev->lock);
            return retval;
        }
    }
    else {

        dev->wr_buffer = krealloc(dev->wr_buffer, (dev->wr_bsize + len_write), GFP_KERNEL);
        if(dev->wr_buffer == NULL) {
            kfree(userdata);
            mutex_unlock(&dev->lock);
            return retval;
        }

    }


    memcpy(dev->wr_buffer + dev->wr_bsize, userdata, len_write); //check if memcopy is okay
    dev->wr_bsize = dev->wr_bsize + len_write;
    
    //newlinefound = newline_check(userdata, dev->wr_bsize);
    kfree(userdata);

    
    if(newlinefound != 0){

        PDEBUG("New line found, adding entry to circular buffer \n");
        //printk(KERN_ALERT, "New line found, adding entry to circular buffer \n");
        
        if(dev->circular_buffer.full){
            kfree(dev->circular_buffer.entry[ dev->circular_buffer.in_offs].buffptr);
        }

        struct aesd_buffer_entry *new_entry = kmalloc(sizeof (struct aesd_buffer_entry *), GFP_KERNEL);
        new_entry->size = dev->wr_bsize;
        new_entry->buffptr = dev->wr_buffer;

        aesd_circular_buffer_add_entry(&dev->circular_buffer, new_entry);
        kfree(new_entry);


        //dev->wbuffer= NULL;
        dev->wr_bsize = 0;
        dev->wr_buffer = NULL;
        //newlinefound = 0;

    }

    
    retval = len_write;
    *f_pos += retval;

    mutex_unlock(&dev->lock);
    PDEBUG("Unlocked mutex, finished writing\n");

    return retval;
}

loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
{
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *entry;
    loff_t newpos, retval;
    int index=0;
    int circular_buffer_size=0;
    int ret;

    PDEBUG("llseek type %d with offset of %lld \n", whence, off);

    //lock dev up to ensure correct positioning
    ret=mutex_lock_interruptible(&dev->lock);
    if(ret != 0) {
        PDEBUG("Failure: Mutex could not lock \n");
        //printk(KERN_ALERT, "Failure: could not lock mutex \n");
        return -ERESTARTSYS;
    }

    //find current aesd circular buffer size
    AESD_CIRCULAR_BUFFER_FOREACH(entry,&dev->circular_buffer,index)
    {
        if(entry->buffptr != NULL){
            circular_buffer_size += entry->size;
        }
    }

    //fixed_size_llseek as per lectures
    newpos = fixed_size_llseek(filp, off, whence, circular_buffer_size);
    if (newpos < 0)
    {
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }
    
    //update f_pos, unlock and return new position
    filp->f_pos = newpos;
    retval=newpos;
    mutex_unlock(&dev->lock);
    return retval;

}

long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long retval = 0;
    struct aesd_dev *dev = filp->private_data;
    long ret=0;
    long offset=0;
    struct aesd_seekto seekto;
    

    PDEBUG("ioctl command %u with arg %lu \n", cmd, arg);

    //check if command is validy
    if(_IOC_TYPE(cmd) != AESD_IOC_MAGIC)
    {
        return -ENOTTY;
    }
    else if(_IOC_TYPE(cmd) > AESDCHAR_IOC_MAXNR)
    {
        return -ENOTTY;
    }

    ret=mutex_lock_interruptible(&dev->lock);
    if(ret != 0) {
        PDEBUG("Failure: Mutex could not lock \n");
        //printk(KERN_ALERT, "Failure: could not lock mutex \n");
        return -ERESTARTSYS;
    }

    //check if command is within range
    if(seekto.write_cmd > AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) 
    {
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }
    else if(seekto.write_cmd_offset > dev->circular_buffer.entry[seekto.write_cmd].size)
    {
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }
    mutex_unlock(&dev->lock);
    
    //copy from user before case for easier readability
    ret = copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto));
    
    switch (cmd)
    {
        case AESDCHAR_IOCSEEKTO:
            if (ret !=0)
            {
                return -EFAULT;
            }
            else 
            {
                
                ret=mutex_lock_interruptible(&dev->lock);
                if(ret != 0) 
                {
                    PDEBUG("Failure: Mutex could not lock \n");
                    //printk(KERN_ALERT, "Failure: could not lock mutex \n");
                    return -ERESTARTSYS;
                }

                //find file offset for command
                for(int i=0; seekto.write_cmd; i++)
                {
                    offset +=dev->circular_buffer.entry[i].size;
                }

                //update fpos
                filp->f_pos = offset + seekto.write_cmd_offset;
                
                mutex_unlock(&dev->lock);

                retval = 0;
            }
            
            break;
        
        default:
            return -EINVAL;
            break;
    }
    
    return retval;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .llseek = aesd_llseek,
    .unlocked_ioctl = aesd_ioctl,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        //printk(KERN_ERR "Error %d adding aesd cdev \n", err);
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
        //printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */

    mutex_init(&aesd_device.lock);
    aesd_circular_buffer_init(&aesd_device.circular_buffer);

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
    struct aesd_buffer_entry *entry_ptr;

    AESD_CIRCULAR_BUFFER_FOREACH(entry_ptr,&aesd_device.circular_buffer,index) { 
            kfree(entry_ptr->buffptr);
    }

    //check about cleaning up wr_buff_entry
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
