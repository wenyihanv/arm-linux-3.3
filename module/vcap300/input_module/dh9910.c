/**
 * @file dh9910.c
 * VCAP300 DH9910 Input Device Driver
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#include "vcap_input.h"
#include "vcap_dbg.h"
#include "dahua/dh9910.h"       ///< from module/include/front_end/hdcvi

#define DEV_MAX                 DH9910_DEV_MAX
#define DEV_VPORT_MAX           DH9910_DEV_VOUT_MAX    ///< VOUT#0, VOUT#1, VOUT#2, VOUT#3
#define DEV_CH_MAX              DH9910_DEV_CH_MAX
#define DEFAULT_TYPE            VCAP_INPUT_TYPE_CVI
#define DEFAULT_NORM_WIDTH      1280
#define DEFAULT_NORM_HEIGHT     720
#define DEFAULT_FRAME_RATE      25
#define DEFAULT_SPEED           VCAP_INPUT_SPEED_P
#define DEFAULT_ORDER           VCAP_INPUT_ORDER_ANYONE
#define DEFAULT_DATA_RANGE      VCAP_INPUT_DATA_RANGE_256
#define DEFAULT_TIMEOUT         ((1000/DEFAULT_FRAME_RATE)*2)

static ushort vport[DEV_MAX] = {0x4321, 0x8765, 0, 0};
module_param_array(vport, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vport, "VPort Link => VOUT0[0:3], VOUT1[3:7], VOUT2[8:11], VOUT3[12:15], 0:None, 1~8:X_CAP#0~7, 9:X_CAPCAS");

static int yc_swap[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 0x0000};
module_param_array(yc_swap, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(yc_swap, "0: None, 1:YC Swap, 2:CbCr Swap, 3:YC and CbCr Swap");

static int inv_clk[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 0x0000};
module_param_array(inv_clk, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "Invert Clock, 0:Disable, 1: Enable");

static int data_swap[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 0};
module_param_array(data_swap, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(data_swap, "0: None, 1:Lo8Bit, 2:Byte, 3:Lo8Bit+Byte");

struct dh9910_dev_t {
    int                     index;                     ///< device index
    struct semaphore        lock;                      ///< device locker
    int                     vfmt[DEV_CH_MAX];          ///< record current video format
    int                     vlos[DEV_CH_MAX];          ///< record current video loss status
    int                     frame_rate[DEV_CH_MAX];    ///< record current video frame rate

    struct vcap_input_dev_t *port[DEV_VPORT_MAX];      ///< device video output port configuration
};

static struct dh9910_dev_t    dh9910_dev[DEV_MAX];
static struct proc_dir_entry *dh9910_proc_root[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9910_proc_norm[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9910_proc_timeout[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = NULL};

static int dh9910_module_get(void)
{
    return try_module_get(THIS_MODULE);
}

static void dh9910_module_put(void)
{
    module_put(THIS_MODULE);
}

static int dh9910_vfmt_notify_handler(int id, int ch, struct dh9910_video_fmt_t *vfmt)
{
    int vout, dev_ch;
    int norm_switch = 0;
    struct vcap_input_dev_t *pinput = NULL;
    struct dh9910_dev_t     *pdev = &dh9910_dev[id];

    if((id >= DEV_MAX) || (ch >= DEV_CH_MAX) || !vfmt)
        return -1;

    down(&pdev->lock);

    vout = dh9910_get_vout_id(pdev->index, ch);
    if(vout >= DEV_VPORT_MAX)
        goto exit;

    pinput = pdev->port[vout];

    /* check video format */
    if(pdev->vfmt[ch] != vfmt->fmt_idx) {
        pdev->vfmt[ch] = vfmt->fmt_idx;
        if(pinput->mode == VCAP_INPUT_MODE_BYPASS) {
            dev_ch = 0;
            if(vfmt->prog == 0) {   ///< interlace
                switch(dh9910_get_vout_format(pdev->index)) {
                    case DH9910_VOUT_FORMAT_BT656_DUAL_HEADER:
                        pinput->interface = VCAP_INPUT_INTF_SDI8BIT_INTERLACE;
                        norm_switch++;
                        break;
                    case DH9910_VOUT_FORMAT_BT1120:
                        pinput->interface = VCAP_INPUT_INTF_BT1120_INTERLACE;
                        norm_switch++;
                        break;
                    case DH9910_VOUT_FORMAT_BT656:
                        pinput->interface = VCAP_INPUT_INTF_BT656_INTERLACE;
                        norm_switch++;
                        break;
                    default:
                        break;
                }

                if(pinput->speed != VCAP_INPUT_SPEED_I) {
                    pinput->speed       = VCAP_INPUT_SPEED_I;
                    pinput->field_order = VCAP_INPUT_ORDER_ODD_FIRST;
                }
            }
            else {
                switch(dh9910_get_vout_format(pdev->index)) {
                    case DH9910_VOUT_FORMAT_BT656_DUAL_HEADER:
                        pinput->interface = VCAP_INPUT_INTF_SDI8BIT_PROGRESSIVE;
                        norm_switch++;
                        break;
                    case DH9910_VOUT_FORMAT_BT1120:
                        pinput->interface = VCAP_INPUT_INTF_BT1120_PROGRESSIVE;
                        norm_switch++;
                        break;
                    case DH9910_VOUT_FORMAT_BT656:
                        pinput->interface = VCAP_INPUT_INTF_BT656_PROGRESSIVE;
                        norm_switch++;
                        break;
                    default:
                        break;
                }

                if(vfmt->frame_rate == 50 || vfmt->frame_rate == 60) {
                    if(pinput->speed != VCAP_INPUT_SPEED_2P) {
                        pinput->speed       = VCAP_INPUT_SPEED_2P;
                        pinput->field_order = VCAP_INPUT_ORDER_ANYONE;
                    }
                }
                else {
                    if(pinput->speed != VCAP_INPUT_SPEED_P) {
                        pinput->speed       = VCAP_INPUT_SPEED_P;
                        pinput->field_order = VCAP_INPUT_ORDER_ANYONE;
                    }
                }
            }

            if((vfmt->width > 0) && (vfmt->width != pinput->norm.width)) {
                pinput->norm.width = vfmt->width;
                norm_switch++;
            }

            if((vfmt->height > 0) && (vfmt->height != pinput->norm.height)) {
                pinput->norm.height = vfmt->height;
                norm_switch++;
            }

            if(norm_switch) {
                pinput->norm.mode++;    ///< update mode to inform norm switched
                vcap_input_device_notify(pinput->vi_idx, dev_ch, VCAP_INPUT_NOTIFY_NORM_CHANGE);
            }

            /* check frame rate */
            if((vfmt->frame_rate > 0) && (pdev->frame_rate[ch] != vfmt->frame_rate)) {
                pdev->frame_rate[ch] = pinput->frame_rate = pinput->max_frame_rate = vfmt->frame_rate;
                pinput->timeout_ms = (1000/pinput->frame_rate)*2;   ///< base on current frame rate
                if(pinput->timeout_ms < 40)
                    pinput->timeout_ms = 40;
                vcap_input_device_notify(pinput->vi_idx, dev_ch, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
            }

            /* update channel parameter */
            pinput->ch_param[dev_ch].mode       = pinput->norm.mode;
            pinput->ch_param[dev_ch].width      = pinput->norm.width;
            pinput->ch_param[dev_ch].height     = pinput->norm.height;
            pinput->ch_param[dev_ch].prog       = vfmt->prog;
            pinput->ch_param[dev_ch].frame_rate = pinput->frame_rate;
            pinput->ch_param[dev_ch].timeout_ms = pinput->timeout_ms;
            pinput->ch_param[dev_ch].speed      = pinput->speed;
        }
        else {  ///< 2CH Dual Edge Mode
            dev_ch = dh9910_get_vout_seq_id(pdev->index, ch)*2;  ///< vout_seq_id would be 0 and 1, the input device channel use 0 and 2 for 2ch dual edge mode
            if(dev_ch >= VCAP_INPUT_DEV_CH_MAX)
                goto exit;

            if(pinput->ch_param[dev_ch].prog != vfmt->prog) {
                pinput->ch_param[dev_ch].prog = vfmt->prog;
                norm_switch++;
            }

            if((vfmt->width > 0) && (vfmt->width != pinput->ch_param[dev_ch].width)) {
                pinput->ch_param[dev_ch].width = vfmt->width;
                norm_switch++;
            }

            if((vfmt->height > 0) && (vfmt->height != pinput->ch_param[dev_ch].height)) {
                pinput->ch_param[dev_ch].height = vfmt->height;
                norm_switch++;
            }

            /* check norm */
            if(norm_switch) {
                pinput->ch_param[dev_ch].mode++;    ///< update mode to inform norm switched
                vcap_input_device_notify(pinput->vi_idx, dev_ch, VCAP_INPUT_NOTIFY_NORM_CHANGE);
            }

            /* check frame rate */
            if((vfmt->frame_rate > 0) && (pdev->frame_rate[ch] != vfmt->frame_rate)) {
                pdev->frame_rate[ch] = pinput->ch_param[dev_ch].frame_rate = vfmt->frame_rate;
                pinput->ch_param[dev_ch].timeout_ms = (1000/vfmt->frame_rate)*2;   ///< base on current frame rate
                if(pinput->ch_param[dev_ch].timeout_ms < 40)
                    pinput->ch_param[dev_ch].timeout_ms = 40;
                vcap_input_device_notify(pinput->vi_idx, dev_ch, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
            }

            /* check speed */
            if(vfmt->prog) {
                if(vfmt->frame_rate == 50 || vfmt->frame_rate == 60)
                    pinput->ch_param[dev_ch].speed = VCAP_INPUT_SPEED_2P;
                else
                    pinput->ch_param[dev_ch].speed = VCAP_INPUT_SPEED_P;
            }
            else {
                pinput->ch_param[dev_ch].speed = VCAP_INPUT_SPEED_I;
            }
        }
    }

exit:
    up(&pdev->lock);

    return 0;
}

static int dh9910_vlos_notify_handler(int id, int ch, int vlos)
{
    int vout, dev_ch;
    struct dh9910_dev_t *pdev = &dh9910_dev[id];

    if((id >= DEV_MAX) || (ch >= DEV_CH_MAX))
        return -1;

    down(&pdev->lock);

    if(pdev->vlos[ch] != vlos) {
        pdev->vlos[ch] = vlos;

        vout = dh9910_get_vout_id(id, ch);
        if((vout < DEV_VPORT_MAX) && pdev->port[vout]) {
            if((pdev->port[vout])->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID)
                dev_ch = dh9910_get_vout_seq_id(pdev->index, ch)*2;  ///< vout_seq_id would be 0 and 1, the input device channel use 0 and 2 for 2ch dual edge mode
            else
                dev_ch = dh9910_get_vout_seq_id(pdev->index, ch);

            if(dev_ch < VCAP_INPUT_DEV_CH_MAX) {
                (pdev->port[vout])->ch_vlos[dev_ch] = vlos;
                vcap_input_device_notify((pdev->port[vout])->vi_idx, dev_ch, ((vlos == 0) ? VCAP_INPUT_NOTIFY_SIGNAL_PRESENT : VCAP_INPUT_NOTIFY_SIGNAL_LOSS));
            }
        }
    }

    up(&pdev->lock);

    return 0;
}

static int dh9910_device_init(int id, int norm)
{
    int i, j, ret = 0;
    struct vcap_input_dev_t *pinput;

    if((id >= DEV_MAX) || (id >= dh9910_get_device_num()))
        return -1;

    /* Update input device norm */
    for(i=0; i<DEV_VPORT_MAX; i++) {
        pinput = dh9910_dev[id].port[i];
        if(!pinput)
            continue;

        /* vi parameter */
        if(pinput->norm.mode != norm) {
            pinput->norm.mode   = norm;
            pinput->norm.width  = DEFAULT_NORM_WIDTH;
            pinput->norm.height = (pinput->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID) ? (DEFAULT_NORM_HEIGHT*2) : DEFAULT_NORM_HEIGHT;
            pinput->frame_rate  = pinput->max_frame_rate = DEFAULT_FRAME_RATE;
            pinput->timeout_ms  = DEFAULT_TIMEOUT;
        }

        /* channel parameter */
        for(j=0; j<VCAP_INPUT_DEV_CH_MAX; j++) {
            if((pinput->mode == VCAP_INPUT_MODE_BYPASS) && (j > 0))
                continue;

            if((pinput->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID) && ((j%2) != 0))    ///< 2ch dual edge, use channel 0 and 2
                continue;

            if(pinput->ch_param[j].mode != norm) {
                pinput->ch_param[j].mode       = norm;
                pinput->ch_param[j].width      = DEFAULT_NORM_WIDTH;
                pinput->ch_param[j].height     = DEFAULT_NORM_HEIGHT;
                pinput->ch_param[j].frame_rate = DEFAULT_FRAME_RATE;
                pinput->ch_param[j].timeout_ms = DEFAULT_TIMEOUT;
                pinput->ch_param[j].speed      = DEFAULT_SPEED;
                pinput->ch_param[j].prog       = (pinput->ch_param[0].speed == VCAP_INPUT_SPEED_I) ? 0 : 1;
            }
        }
    }

    return ret;
}

static int dh9910_proc_norm_show(struct seq_file *sfile, void *v)
{
    int i, vout, vout_ch, dev_ch;
    struct dh9910_dev_t *pdev = (struct dh9910_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9910.%d]\n", pdev->index);
    seq_printf(sfile, "--------------------------\n");
    seq_printf(sfile, "CVI   VCH   Width   Height\n");
    seq_printf(sfile, "--------------------------\n");

    for(i=0; i<DEV_CH_MAX; i++) {
        vout = dh9910_get_vout_id(pdev->index, i);
        if((vout < DEV_VPORT_MAX) && pdev->port[vout]) {
            vout_ch = dh9910_get_vout_seq_id(pdev->index, i);
            if((pdev->port[vout])->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID)
                dev_ch = vout_ch*2;   ///< vout_ch_id would be 0 and 1, the input device channel use 0 and 2 for 2ch dual edge mode
            else
                dev_ch = vout_ch;

            if(dev_ch >= VCAP_INPUT_DEV_CH_MAX)
                goto unknown;

            seq_printf(sfile, "%-5d %-5d %-7d %-7d\n",
                       i,
                       dh9910_get_vch_id_ext(pdev->index, vout, vout_ch),
                       (pdev->port[vout])->ch_param[dev_ch].width,
                       (pdev->port[vout])->ch_param[dev_ch].height);
        }
        else {
unknown:
            seq_printf(sfile, "%-5s %-5s %-7s %-7s\n", "-", "-", "-", "-");
        }
    }
    seq_printf(sfile, "\n");

    up(&pdev->lock);

    return 0;
}

static int dh9910_proc_timeout_show(struct seq_file *sfile, void *v)
{
    int i, vout, vout_ch, dev_ch;
    struct dh9910_dev_t *pdev = (struct dh9910_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9910.%d]\n", pdev->index);
    seq_printf(sfile, "-------------------\n");
    seq_printf(sfile, "CVI   VCH   Timeout\n");
    seq_printf(sfile, "-------------------\n");
    for(i=0; i<DEV_CH_MAX; i++) {
        vout = dh9910_get_vout_id(pdev->index, i);
        if((vout < DEV_VPORT_MAX) && pdev->port[vout]) {
            vout_ch = dh9910_get_vout_seq_id(pdev->index, i);
            if((pdev->port[vout])->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID)
                dev_ch = vout_ch*2; ///< vout_seq_id would be 0 and 1, the input device channel use 0 and 2 for 2ch dual edge mode
            else
                dev_ch = vout_ch;

            if(dev_ch >= VCAP_INPUT_DEV_CH_MAX)
                goto unknown;

            seq_printf(sfile, "%-5d %-5d %-7d\n",
                       i,
                       dh9910_get_vch_id_ext(pdev->index, vout, vout_ch),
                       (pdev->port[vout])->ch_param[dev_ch].timeout_ms);
        }
        else {
unknown:
            seq_printf(sfile, "%-5s %-5s %-7s\n", "-", "-", "-");
        }
    }
    seq_printf(sfile, "\n");

    up(&pdev->lock);

    return 0;
}

static int dh9910_proc_norm_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9910_proc_norm_show, PDE(inode)->data);
}

static int dh9910_proc_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9910_proc_timeout_show, PDE(inode)->data);
}

static struct file_operations dh9910_proc_norm_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9910_proc_norm_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9910_proc_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9910_proc_timeout_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void dh9910_proc_remove(int id)
{
    if(dh9910_proc_root[id]) {
        if(dh9910_proc_norm[id])
            vcap_input_proc_remove_entry(dh9910_proc_root[id], dh9910_proc_norm[id]);

        if(dh9910_proc_timeout[id])
            vcap_input_proc_remove_entry(dh9910_proc_root[id], dh9910_proc_timeout[id]);

        vcap_input_proc_remove_entry(NULL, dh9910_proc_root[id]);
    }
}

static int dh9910_proc_init(struct dh9910_dev_t *pdev)
{
    int ret = 0;
    int id  = pdev->index;
    char name[32];

    /* root */
    sprintf(name, "dh9910.%d", id);
    dh9910_proc_root[id] = vcap_input_proc_create_entry(name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!dh9910_proc_root[id]) {
        vcap_err("create proc node '%s' failed!\n", name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9910_proc_root[id]->owner = THIS_MODULE;
#endif

    /* norm */
    dh9910_proc_norm[id] = vcap_input_proc_create_entry("norm", S_IRUGO|S_IXUGO, dh9910_proc_root[id]);
    if(!dh9910_proc_norm[id]) {
        vcap_err("create proc node '%s/norm' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    dh9910_proc_norm[id]->proc_fops = &dh9910_proc_norm_ops;
    dh9910_proc_norm[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9910_proc_norm[id]->owner     = THIS_MODULE;
#endif

    /* timeout */
    dh9910_proc_timeout[id] = vcap_input_proc_create_entry("timeout", S_IRUGO|S_IXUGO, dh9910_proc_root[id]);
    if(!dh9910_proc_timeout[id]) {
        vcap_err("create proc node '%s/timeout' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    dh9910_proc_timeout[id]->proc_fops = &dh9910_proc_timeout_ops;
    dh9910_proc_timeout[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9910_proc_timeout[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    dh9910_proc_remove(id);
    return ret;
}

static int __init dh9910_input_init(void)
{
    int i, j;
    int dev_num;
    struct vcap_input_dev_t *pinput;
    int ret = 0;

    if(vcap_input_get_version() < VCAP_INPUT_VERSION) {
        vcap_err("Input driver version(%08x) is not compatibility with vcap300_common.ko(%08x)!\n",
                 vcap_input_get_version(), VCAP_INPUT_VERSION);
        return -EFAULT;
    }

    /* clear buffer */
    memset(dh9910_dev, 0, sizeof(struct dh9910_dev_t)*DEV_MAX);

    /* Get DH9910 Device Number */
    dev_num = dh9910_get_device_num();

    for(i=0; i<DEV_MAX; i++) {
        dh9910_dev[i].index = i;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&dh9910_dev[i].lock, 1);
#else
        init_MUTEX(&dh9910_dev[i].lock);
#endif
        if(i >= dev_num)
            continue;

        if(vport[i]) {
            /* register input device */
            for(j=0; j<DEV_VPORT_MAX; j++) {
                if((vport[i]>>(4*j)) & 0xf) {   ///< VPort# Connect to X_CAP#
                    pinput = dh9910_dev[i].port[j] = kzalloc(sizeof(struct vcap_input_dev_t), GFP_KERNEL);
                    if(pinput == NULL) {
                        vcap_err("DH9910#%d.%d allocate vcap_input_dev_t failed\n", i, j);
                        ret = -ENOMEM;
                        goto err;
                    }

                    /* input device name */
                    snprintf(pinput->name, VCAP_INPUT_NAME_SIZE-1, "dh9910.%d.%d", i, j);

                    /* input device parameter setup */
                    pinput->index       = (i*DEV_VPORT_MAX) + j;
                    pinput->vi_idx      = ((vport[i]>>(4*j)) & 0xf) - 1; ///< VI#
                    pinput->vi_src      = ((vport[i]>>(4*j)) & 0xf) - 1; ///< X_CAP#
                    pinput->type        = DEFAULT_TYPE;
                    pinput->mode        = (dh9910_get_vout_mode(i) == DH9910_VOUT_MODE_2CH_DUAL_EDGE) ? VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID : VCAP_INPUT_MODE_BYPASS;
                    pinput->field_order = DEFAULT_ORDER;
                    pinput->data_range  = DEFAULT_DATA_RANGE;
                    pinput->yc_swap     = (yc_swap[i]>>(4*j)) & 0x1;
                    pinput->data_swap   = data_swap[i];
                    pinput->ch_id_mode  = 0;
                    pinput->inv_clk     = (inv_clk[i]>>(4*j)) & 0x1;
                    pinput->init        = dh9910_device_init;
                    pinput->norm.mode   = -1;                   ///< init value
                    pinput->module_get  = dh9910_module_get;
                    pinput->module_put  = dh9910_module_put;

                    /* interface & speed */
                    switch(dh9910_get_vout_format(i)) {
                        case DH9910_VOUT_FORMAT_BT656_DUAL_HEADER:
                            if(pinput->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID) {
                                pinput->interface = VCAP_INPUT_INTF_SDI8BIT_INTERLACE;
                                pinput->speed     = VCAP_INPUT_SPEED_I;
                            }
                            else {
                                pinput->interface = VCAP_INPUT_INTF_SDI8BIT_PROGRESSIVE;
                                pinput->speed     = DEFAULT_SPEED;
                            }
                            break;
                        case DH9910_VOUT_FORMAT_BT1120:
                            if(pinput->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID) {
                                pinput->interface = VCAP_INPUT_INTF_BT1120_INTERLACE;
                                pinput->speed     = VCAP_INPUT_SPEED_I;
                            }
                            else {
                                pinput->interface = VCAP_INPUT_INTF_BT1120_PROGRESSIVE;
                                pinput->speed     = DEFAULT_SPEED;
                            }
                            break;
                        case DH9910_VOUT_FORMAT_BT656:
                        default:
                            if(pinput->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID) {
                                pinput->interface = VCAP_INPUT_INTF_BT656_INTERLACE;
                                pinput->speed     = VCAP_INPUT_SPEED_I;
                            }
                            else {
                                pinput->interface = VCAP_INPUT_INTF_BT656_PROGRESSIVE;
                                pinput->speed     = DEFAULT_SPEED;
                            }
                            break;
                    }

                    /* channel mode */
                    switch(pinput->mode) {
                        case VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID:
                            pinput->probe_chid = VCAP_INPUT_PROBE_CHID_HBLANK;      ///< detect channel id from horizontal blanking
                            pinput->ch_id[0]   = dh9910_get_vch_id_ext(i, j, 0);    ///< vout sequence id 0
                            pinput->ch_id[2]   = dh9910_get_vch_id_ext(i, j, 1);    ///< vout sequence id 1

                            /* channel parameter */
                            pinput->ch_param[0].mode = pinput->ch_param[2].mode = -1; ///< init value
                            break;
                        default:
                            pinput->probe_chid = VCAP_INPUT_PROBE_CHID_DISABLE;
                            pinput->ch_id[0]   = dh9910_get_vch_id_ext(i, j, 0);    ///< vout sequence id 0

                            /* channel parameter */
                            pinput->ch_param[0].mode = -1;  ///< init value
                            break;
                    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
                    sema_init(&pinput->sema_lock, 1);
#else
                    init_MUTEX(&pinput->sema_lock);
#endif
                    ret = vcap_input_device_register(pinput);
                    if(ret < 0) {
                        vcap_err("register DH9910#%d.%d input device failed\n", i, j);
                        goto err;
                    }
                }
            }

            /* device proc init */
            ret = dh9910_proc_init(&dh9910_dev[i]);
            if(ret < 0)
                goto err;

            /* device init */
            ret = dh9910_device_init(i, 0);
            if(ret < 0)
                goto err;

            /* register dh9910 video loss notify handler */
            dh9910_notify_vlos_register(i, dh9910_vlos_notify_handler);

            /* register dh9910 video foramt notify handler */
            dh9910_notify_vfmt_register(i, dh9910_vfmt_notify_handler);

            vcap_info("Register DH9910#%d Input Device\n", i);
        }
    }

    return ret;

err:
    for(i=0; i<DEV_MAX; i++) {
        dh9910_notify_vlos_deregister(i);
        dh9910_notify_vfmt_deregister(i);
        dh9910_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(dh9910_dev[i].port[j]) {
                vcap_input_device_unregister(dh9910_dev[i].port[j]);
                kfree(dh9910_dev[i].port[j]);
            }
        }
    }
    return ret;
}

static void __exit dh9910_input_exit(void)
{
    int i, j;

    for(i=0; i<DEV_MAX; i++) {
        dh9910_notify_vlos_deregister(i);
        dh9910_notify_vfmt_deregister(i);
        dh9910_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(dh9910_dev[i].port[j]) {
                vcap_input_device_unregister(dh9910_dev[i].port[j]);
                kfree(dh9910_dev[i].port[j]);
            }
        }
    }
}

module_init(dh9910_input_init);
module_exit(dh9910_input_exit);

MODULE_DESCRIPTION("Grain Media Video Capture300 DH9910 Input Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
