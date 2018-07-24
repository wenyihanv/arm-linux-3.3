/**
 * @file vcap_fcs.c
 *  vcap300 FCS(False color suppression) Control Interface
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.1 $
 * $Date: 2012/12/11 09:29:10 $
 *
 * ChangeLog:
 *  $Log: vcap_fcs.c,v $
 *  Revision 1.1  2012/12/11 09:29:10  jerson_l
 *  1. add FCS support
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

#include "bits.h"
#include "vcap_proc.h"
#include "vcap_dev.h"
#include "vcap_dbg.h"
#include "vcap_fcs.h"

static int fcs_proc_ch[VCAP_DEV_MAX] = {[0 ... (VCAP_DEV_MAX - 1)] = 0};

static struct proc_dir_entry *vcap_fcs_proc_root[VCAP_DEV_MAX]   = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_fcs_proc_ch[VCAP_DEV_MAX]     = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_fcs_proc_param[VCAP_DEV_MAX]  = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_fcs_proc_enable[VCAP_DEV_MAX] = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};

void vcap_fcs_init(struct vcap_dev_info_t *pdev_info)
{
    int i;
    u32 tmp;

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(!pdev_info->ch[i].active)
            continue;

        /* LV0, LV1, LV2 */
        tmp = (VCAP_FCS_LV0_THRED_DEFAULT)     |
              (VCAP_FCS_LV1_THRED_DEFAULT<<12) |
              (VCAP_FCS_LV2_THRED_DEFAULT<<20);
        VCAP_REG_WR(pdev_info, ((i == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_FCS_OFFSET(0x00) : VCAP_CH_FCS_OFFSET(i, 0x00)), tmp);

        /* LV3, LV4, Grey */
        tmp = (VCAP_FCS_LV3_THRED_DEFAULT)    |
              (VCAP_FCS_LV4_THRED_DEFAULT<<8) |
              (VCAP_FCS_GREY_THRED_DEFAULT<<16);
        VCAP_REG_WR(pdev_info, ((i == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_FCS_OFFSET(0x04) : VCAP_CH_FCS_OFFSET(i, 0x04)), tmp);
    }
}

int vcap_fcs_enable(struct vcap_dev_info_t *pdev_info, int ch)
{
    unsigned long flags;

    spin_lock_irqsave(&pdev_info->lock, flags); ///< use spin lock because temp_param would be access in lli tasklet

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        if(!pdev_info->ch[ch].temp_param.comm.fcs_en) {
            pdev_info->ch[ch].temp_param.comm.fcs_en = 1;   ///< lli tasklet will apply this config and sync to current
        }
    }
    else {
        /* ch is disable, direct write to register */
        u32 tmp;

        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x04) : VCAP_CH_SUBCH_OFFSET(ch, 0x04)));
        if(!(tmp & BIT8)) {
            tmp |= BIT8;
            VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x04) : VCAP_CH_SUBCH_OFFSET(ch, 0x04)), tmp);

            pdev_info->ch[ch].param.comm.fcs_en = pdev_info->ch[ch].temp_param.comm.fcs_en = 1;
        }
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_fcs_disable(struct vcap_dev_info_t *pdev_info, int ch)
{
    unsigned long flags;

    spin_lock_irqsave(&pdev_info->lock, flags); ///< use spin lock because temp_param would be access in lli tasklet

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        if(pdev_info->ch[ch].temp_param.comm.fcs_en) {
            pdev_info->ch[ch].temp_param.comm.fcs_en = 0;   ///< lli tasklet will apply this config and sync to current
        }
    }
    else {
        /* ch is disable, direct write to register */
        u32 tmp;

        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x04) : VCAP_CH_SUBCH_OFFSET(ch, 0x04)));
        if(tmp & BIT8) {
            tmp &= ~BIT8;
            VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x04) : VCAP_CH_SUBCH_OFFSET(ch, 0x04)), tmp);

            pdev_info->ch[ch].param.comm.fcs_en = pdev_info->ch[ch].temp_param.comm.fcs_en = 0;
        }
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

u32 vcap_fcs_get_param(struct vcap_dev_info_t *pdev_info, int ch, VCAP_FCS_PARAM_T param_id)
{
    u32 value;

    switch(param_id) {
        case VCAP_FCS_PARAM_LV0_THRED:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_FCS_OFFSET(0x00) : VCAP_CH_FCS_OFFSET(ch, 0x00)));
            value &= 0xfff;
            break;
        case VCAP_FCS_PARAM_LV1_THRED:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_FCS_OFFSET(0x00) : VCAP_CH_FCS_OFFSET(ch, 0x00)));
            value = (value>>12) & 0xff;
            break;
        case VCAP_FCS_PARAM_LV2_THRED:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_FCS_OFFSET(0x00) : VCAP_CH_FCS_OFFSET(ch, 0x00)));
            value = (value>>20) & 0xff;
            break;
        case VCAP_FCS_PARAM_LV3_THRED:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_FCS_OFFSET(0x04) : VCAP_CH_FCS_OFFSET(ch, 0x04)));
            value &= 0xff;
            break;
        case VCAP_FCS_PARAM_LV4_THRED:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_FCS_OFFSET(0x04) : VCAP_CH_FCS_OFFSET(ch, 0x04)));
            value = (value>>8) & 0xff;
            break;
        case VCAP_FCS_PARAM_GREY_THRED:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_FCS_OFFSET(0x04) : VCAP_CH_FCS_OFFSET(ch, 0x04)));
            value = (value>>16) & 0xfff;
            break;
        default:
            value = 0;
            break;
    }

    return value;
}

int vcap_fcs_set_param(struct vcap_dev_info_t *pdev_info, int ch, VCAP_FCS_PARAM_T param_id, u32 data)
{
    u32 tmp;

    switch(param_id) {
        case VCAP_FCS_PARAM_LV0_THRED:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_FCS_OFFSET(0x00));
                tmp &= ~0xfff;
                tmp |= (data & 0xfff);
                VCAP_REG_WR(pdev_info, VCAP_CAS_FCS_OFFSET(0x00), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_FCS_OFFSET(ch, 0x00));
                tmp &= ~0xfff;
                tmp |= (data & 0xfff);
                VCAP_REG_WR(pdev_info, VCAP_CH_FCS_OFFSET(ch, 0x00), tmp);
            }
            break;
        case VCAP_FCS_PARAM_LV1_THRED:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_FCS_OFFSET(0x00));
                tmp &= ~(0xff<<12);
                tmp |= ((data & 0xff)<<12);
                VCAP_REG_WR(pdev_info, VCAP_CAS_FCS_OFFSET(0x00), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_FCS_OFFSET(ch, 0x00));
                tmp &= ~(0xff<<12);
                tmp |= ((data & 0xff)<<12);
                VCAP_REG_WR(pdev_info, VCAP_CH_FCS_OFFSET(ch, 0x00), tmp);
            }
            break;
        case VCAP_FCS_PARAM_LV2_THRED:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_FCS_OFFSET(0x00));
                tmp &= ~(0xff<<20);
                tmp |= ((data & 0xff)<<20);
                VCAP_REG_WR(pdev_info, VCAP_CAS_FCS_OFFSET(0x00), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_FCS_OFFSET(ch, 0x00));
                tmp &= ~(0xff<<20);
                tmp |= ((data & 0xff)<<20);
                VCAP_REG_WR(pdev_info, VCAP_CH_FCS_OFFSET(ch, 0x00), tmp);
            }
            break;
        case VCAP_FCS_PARAM_LV3_THRED:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_FCS_OFFSET(0x04));
                tmp &= ~0xff;
                tmp |= (data & 0xff);
                VCAP_REG_WR(pdev_info, VCAP_CAS_FCS_OFFSET(0x04), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_FCS_OFFSET(ch, 0x04));
                tmp &= ~0xff;
                tmp |= (data & 0xff);
                VCAP_REG_WR(pdev_info, VCAP_CH_FCS_OFFSET(ch, 0x04), tmp);
            }
            break;
        case VCAP_FCS_PARAM_LV4_THRED:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_FCS_OFFSET(0x04));
                tmp &= ~(0xff<<8);
                tmp |= ((data & 0xff)<<8);
                VCAP_REG_WR(pdev_info, VCAP_CAS_FCS_OFFSET(0x04), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_FCS_OFFSET(ch, 0x04));
                tmp &= ~(0xff<<8);
                tmp |= ((data & 0xff)<<8);
                VCAP_REG_WR(pdev_info, VCAP_CH_FCS_OFFSET(ch, 0x04), tmp);
            }
            break;
        case VCAP_FCS_PARAM_GREY_THRED:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_FCS_OFFSET(0x04));
                tmp &= ~(0xfff<<16);
                tmp |= ((data & 0xfff)<<16);
                VCAP_REG_WR(pdev_info, VCAP_CAS_FCS_OFFSET(0x04), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_FCS_OFFSET(ch, 0x04));
                tmp &= ~(0xfff<<16);
                tmp |= ((data & 0xfff)<<16);
                VCAP_REG_WR(pdev_info, VCAP_CH_FCS_OFFSET(ch, 0x04), tmp);
            }
            break;
        default:
            break;
    }

    return 0;
}

static ssize_t vcap_fcs_proc_ch_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ch;
    char value_str[8] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &ch);

    down(&pdev_info->sema_lock);

    if(ch != fcs_proc_ch[pdev_info->index])
        fcs_proc_ch[pdev_info->index] = ch;

    up(&pdev_info->sema_lock);

    return count;
}

static ssize_t vcap_fcs_proc_param_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, param_id;
    u32  value;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &param_id, &value);

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(fcs_proc_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || fcs_proc_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                down(&pdev_info->ch[i].sema_lock);
                vcap_fcs_set_param(pdev_info, i, param_id, value);
                up(&pdev_info->ch[i].sema_lock);
            }
        }
    }

    up(&pdev_info->sema_lock);

    return count;
}

static ssize_t vcap_fcs_proc_enable_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, value;
    char value_str[8] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &value);

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(fcs_proc_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || fcs_proc_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                down(&pdev_info->ch[i].sema_lock);
                if(value)
                    vcap_fcs_enable(pdev_info, i);
                else
                    vcap_fcs_disable(pdev_info, i);
                up(&pdev_info->ch[i].sema_lock);
            }
        }
    }

    up(&pdev_info->sema_lock);

    return count;
}

static int vcap_fcs_proc_ch_show(struct seq_file *sfile, void *v)
{
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    down(&pdev_info->sema_lock);

    seq_printf(sfile, "FCS_Control_CH: %d (0~%d, other for all)\n", fcs_proc_ch[pdev_info->index], VCAP_CHANNEL_MAX-1);

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_fcs_proc_param_show(struct seq_file *sfile, void *v)
{
    int i;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(fcs_proc_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || fcs_proc_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                down(&pdev_info->ch[i].sema_lock);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n=== [CH#%02d] FCS Parameter ===\n", i);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]LV0_THRED : 0x%x\n",
                                       VCAP_FCS_PARAM_LV0_THRED, vcap_fcs_get_param(pdev_info, i, VCAP_FCS_PARAM_LV0_THRED));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]LV1_THRED : 0x%x\n",
                                       VCAP_FCS_PARAM_LV1_THRED, vcap_fcs_get_param(pdev_info, i, VCAP_FCS_PARAM_LV1_THRED));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]LV2_THRED : 0x%x\n",
                                       VCAP_FCS_PARAM_LV2_THRED, vcap_fcs_get_param(pdev_info, i, VCAP_FCS_PARAM_LV2_THRED));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]LV3_THRED : 0x%x\n",
                                       VCAP_FCS_PARAM_LV3_THRED, vcap_fcs_get_param(pdev_info, i, VCAP_FCS_PARAM_LV3_THRED));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]LV4_THRED : 0x%x\n",
                                       VCAP_FCS_PARAM_LV4_THRED, vcap_fcs_get_param(pdev_info, i, VCAP_FCS_PARAM_LV4_THRED));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]GREY_THRED: 0x%x\n",
                                       VCAP_FCS_PARAM_GREY_THRED, vcap_fcs_get_param(pdev_info, i, VCAP_FCS_PARAM_GREY_THRED));
                up(&pdev_info->ch[i].sema_lock);

                if(mbuf_point >= VCAP_DEV_MSG_THRESHOLD) {
                    seq_printf(sfile, mbuf);
                    mbuf_point = 0;
                }
            }
        }
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    seq_printf(sfile, mbuf);

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_fcs_proc_enable_show(struct seq_file *sfile, void *v)
{
    int i;
    u32 tmp;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(fcs_proc_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || fcs_proc_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                down(&pdev_info->ch[i].sema_lock);
                tmp = VCAP_REG_RD(pdev_info, ((i == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x04) : VCAP_CH_SUBCH_OFFSET(i, 0x04)));

                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n=== [CH#%02d] ===\n", i);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " FCS_Enable: %d\n", ((tmp & BIT8)>>8));
                up(&pdev_info->ch[i].sema_lock);

                if(mbuf_point >= VCAP_DEV_MSG_THRESHOLD) {
                    seq_printf(sfile, mbuf);
                    mbuf_point = 0;
                }
            }
        }
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    seq_printf(sfile, mbuf);

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_fcs_proc_ch_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_fcs_proc_ch_show, PDE(inode)->data);
}

static int vcap_fcs_proc_param_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_fcs_proc_param_show, PDE(inode)->data);
}

static int vcap_fcs_proc_enable_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_fcs_proc_enable_show, PDE(inode)->data);
}

static struct file_operations vcap_fcs_proc_ch_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_fcs_proc_ch_open,
    .write  = vcap_fcs_proc_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_fcs_proc_param_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_fcs_proc_param_open,
    .write  = vcap_fcs_proc_param_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_fcs_proc_enable_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_fcs_proc_enable_open,
    .write  = vcap_fcs_proc_enable_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

void vcap_fcs_proc_remove(int id)
{
    if(vcap_fcs_proc_root[id]) {
        struct proc_dir_entry *proc_root = (struct proc_dir_entry *)vcap_dev_get_dev_proc_root(id);

        if(vcap_fcs_proc_ch[id])
            vcap_proc_remove_entry(vcap_fcs_proc_root[id], vcap_fcs_proc_ch[id]);

        if(vcap_fcs_proc_param[id])
            vcap_proc_remove_entry(vcap_fcs_proc_root[id], vcap_fcs_proc_param[id]);

        if(vcap_fcs_proc_enable[id])
            vcap_proc_remove_entry(vcap_fcs_proc_root[id], vcap_fcs_proc_enable[id]);

        vcap_proc_remove_entry(proc_root, vcap_fcs_proc_root[id]);
    }
}

int vcap_fcs_proc_init(int id, void *private)
{
    int ret = 0;
    struct proc_dir_entry *proc_root = (struct proc_dir_entry *)vcap_dev_get_dev_proc_root(id);

    /* get device proc root */
    if(!proc_root) {
        ret = -EINVAL;
        goto end;
    }

    /* root */
    vcap_fcs_proc_root[id] = vcap_proc_create_entry("fcs", S_IFDIR|S_IRUGO|S_IXUGO, proc_root);
    if(!vcap_fcs_proc_root[id]) {
        vcap_err("create proc node %s/fcs failed!\n", proc_root->name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_fcs_proc_root[id]->owner = THIS_MODULE;
#endif

    /* ch */
    vcap_fcs_proc_ch[id] = vcap_proc_create_entry("ch", S_IRUGO|S_IXUGO, vcap_fcs_proc_root[id]);
    if(!vcap_fcs_proc_ch[id]) {
        vcap_err("create proc node 'fcs/ch' failed!\n");
        ret = -EINVAL;
        goto err;
    }
    vcap_fcs_proc_ch[id]->proc_fops = &vcap_fcs_proc_ch_ops;
    vcap_fcs_proc_ch[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_fcs_proc_ch[id]->owner     = THIS_MODULE;
#endif

    /* param */
    vcap_fcs_proc_param[id] = vcap_proc_create_entry("param", S_IRUGO|S_IXUGO, vcap_fcs_proc_root[id]);
    if(!vcap_fcs_proc_param[id]) {
        vcap_err("create proc node 'fcs/param' failed!\n");
        ret = -EINVAL;
        goto err;
    }
    vcap_fcs_proc_param[id]->proc_fops = &vcap_fcs_proc_param_ops;
    vcap_fcs_proc_param[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_fcs_proc_param[id]->owner     = THIS_MODULE;
#endif

    /* enable */
    vcap_fcs_proc_enable[id] = vcap_proc_create_entry("enable", S_IRUGO|S_IXUGO, vcap_fcs_proc_root[id]);
    if(!vcap_fcs_proc_enable[id]) {
        vcap_err("create proc node 'fcs/enable' failed!\n");
        ret = -EINVAL;
        goto err;
    }
    vcap_fcs_proc_enable[id]->proc_fops = &vcap_fcs_proc_enable_ops;
    vcap_fcs_proc_enable[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_fcs_proc_enable[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    vcap_fcs_proc_remove(id);
    return ret;
}
