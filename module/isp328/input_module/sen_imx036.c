/**
 * @file sen_imx036.c
 * Sony IMX036 sensor driver
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <asm/io.h>

#define PFX "sen_imx036"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"
#include "spi_common.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1280
#define DEFAULT_IMAGE_HEIGHT    720
#define DEFAULT_XCLK            37125000

static ushort sensor_w = DEFAULT_IMAGE_WIDTH;
module_param(sensor_w, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_w, "sensor image width");

static ushort sensor_h = DEFAULT_IMAGE_HEIGHT;
module_param(sensor_h, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_h, "sensor image height");

static uint sensor_xclk = DEFAULT_XCLK;
module_param(sensor_xclk, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_xclk, "sensor external clock frequency");

static uint mirror = 0;
module_param(mirror, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mirror, "sensor mirror function");

static uint flip = 0;
module_param(flip, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(flip, "sensor flip function");

static uint ch_num = 4;
module_param(ch_num, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_num, "channel number");

static uint fps = 30;
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "sensor frame rate");

static uint inv_clk = 0;
module_param(inv_clk, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "invert clock, 0:Disable, 1: Enable");

//=============================================================================
// global
//============================================================================
#define SENSOR_NAME     "IMX036"
#define H_CYCLE         2200
#define V_CYCLE         1125
#define H_VALID         2048
#define V_VALID         1080
static sensor_dev_t *g_psensor = NULL;

typedef struct sensor_info {
    int is_init;
    u32 HMax;
    u32 VMax;
    u32 t_row;          // unit: 1/10 us
    int htp;
    int mirror;
    int flip;
} sensor_info_t;

typedef struct sensor_reg {
    u8 id;
    u8 addr;
    u8 val;
} sensor_reg_t;

static sensor_reg_t sensor_init_regs[] = {

    //Sony new provide
    //{0x02, 0x00, 0x55},   //Standby
    //{0x02, 0x00, 0x54},     // Operation
    {0x02, 0x00, 0x54},
    {0x02, 0x01, 0x73},     // WinMode[6:4], Flip[0]
    {0x02, 0x02, 0x00},     // Mode[5:0]
    {0x02, 0x03, 0x08},     // DBIT[3]  => 1080p@15fps
    //{0x02, 0x03, 0x08},       // DBIT[3] => 1080p@30fps
    //{0x02, 0x04, 0x11},       // PLL setting => 1080p@15fps
    {0x02, 0x04, 0x0d},     // PLL setting => 1080p@30fps

    {0x02, 0x05, 0x00},     // Analog Gain
    {0x02, 0x06, 0x00},     // Analog Gain

    {0x02, 0x07, 0x98},     // HMAX
    {0x02, 0x08, 0x08},     // HMAX

    {0x02, 0x09, 0x65},     // VMAX
    {0x02, 0x0A, 0x04},     // VMAX
    {0x02, 0x0B, 0x00},     // VMAX

    {0x02, 0x0C, 0x00},
    {0x02, 0x0D, 0x00},
    {0x02, 0x0E, 0x00},
    {0x02, 0x0F, 0x00},
    {0x02, 0x10, 0x00},
    {0x02, 0x11, 0x00},
    {0x02, 0x12, 0x00},
    {0x02, 0x13, 0x00},
    {0x02, 0x14, 0x00},
    {0x02, 0x15, 0x00},
    {0x02, 0x16, 0x00},
    {0x02, 0x17, 0x0D},
    {0x02, 0x18, 0x00},
    {0x02, 0x19, 0xE0},
    {0x02, 0x1A, 0x00},
    {0x02, 0x1B, 0x00},
    {0x02, 0x1C, 0x0F},
    {0x02, 0x1D, 0x00},
    {0x02, 0x1E, 0x00},
    {0x02, 0x1F, 0x3c/*0xF0*/},            //Black level setting
    {0x02, 0x20, 0x00},
    {0x02, 0x21, 0x13},
    {0x02, 0x22, 0x5C},
    {0x02, 0x23, 0x00},
    {0x02, 0x24, 0x00},
    //{0x02, 0x25, 0x47},       //CMOSMD[2:1]
    {0x02, 0x25, 0x47},     //CMOSMD[2:1]=3h
    {0x02, 0x26, 0x00},
    {0x02, 0x27, 0xD6},
    {0x02, 0x28, 0x00},
    //{0x02, 0x29, 0x00}, // HS /VS output High(always)
    {0x02, 0x29, 0xc0},   // HS / VS output
    {0x02, 0x2A, 0x23},
    {0x02, 0x2B, 0x10},
    {0x02, 0x2C, 0x00},
    //{0x02, 0x2D, 0x88},
    {0x02, 0x2D, 0x08},   // Start Master mode operation
    {0x02, 0x2E, 0x06},
    {0x02, 0x2F, 0x06},
    {0x02, 0x30, 0x24},
    {0x02, 0x31, 0x04},
    {0x02, 0x32, 0x86},
    {0x02, 0x33, 0x01},
    {0x02, 0x34, 0x00},
    {0x02, 0x35, 0x00},
    {0x02, 0x36, 0x00},
    {0x02, 0x37, 0x00},
    {0x02, 0x38, 0x40},
    {0x02, 0x39, 0x00},
    {0x02, 0x3A, 0x00},
    {0x02, 0x3B, 0xC0},
    {0x02, 0x3C, 0x00},
    {0x02, 0x3D, 0x00},
    {0x02, 0x3E, 0xB8},
    {0x02, 0x3F, 0x08},
    {0x02, 0x40, 0x40},
    {0x02, 0x41, 0x00},
    {0x02, 0x42, 0xC0},
    {0x02, 0x43, 0x16},
    {0x02, 0x44, 0x00},
    {0x02, 0x45, 0x00},
    {0x02, 0x46, 0x00},
    {0x02, 0x47, 0x00},
    {0x02, 0x48, 0x00},
    {0x02, 0x49, 0x00},
    {0x02, 0x4A, 0x00},
    {0x02, 0x4B, 0x00},
    {0x02, 0x4C, 0x00},
    {0x02, 0x4D, 0x00},
    {0x02, 0x4E, 0x00},
    {0x02, 0x4F, 0x00},
    {0x02, 0x50, 0x00},
    {0x02, 0x51, 0x00},
    {0x02, 0x52, 0x00},
    {0x02, 0x53, 0x00},
    {0x02, 0x54, 0x00},
    {0x02, 0x55, 0x00},
    {0x02, 0x56, 0x00},
    {0x02, 0x57, 0x00},
    {0x02, 0x58, 0x00},
    {0x02, 0x59, 0x00},
    {0x02, 0x5A, 0x00},
    {0x02, 0x5B, 0x20},
    {0x02, 0x5C, 0x80},
    {0x02, 0x5D, 0x00},
    {0x02, 0x5E, 0xFF},
    {0x02, 0x5F, 0x02},
    {0x02, 0x60, 0x00},

    {0x02, 0x61, 0x40}, // Digital Gain

    {0x02, 0x62, 0x00},
    {0x02, 0x63, 0x00},
    {0x02, 0x64, 0x00},
    {0x02, 0x65, 0x00},
    {0x02, 0x66, 0x00},
    {0x02, 0x67, 0x00},
    {0x02, 0x68, 0x01},
    {0x02, 0x69, 0x00},
    {0x02, 0x6A, 0x01},
    {0x02, 0x6B, 0x09},
    {0x02, 0x6C, 0xA0},
    {0x02, 0x6D, 0x20},
    {0x02, 0x6E, 0x00},
    {0x02, 0x6F, 0x00},
    {0x02, 0x70, 0x80},
    {0x02, 0x71, 0x00},
    {0x02, 0x72, 0x00},
    {0x02, 0x73, 0xB1},
    {0x02, 0x74, 0x01},
    {0x02, 0x75, 0x70},
    {0x02, 0x76, 0x60},
    {0x02, 0x77, 0x00},
    {0x02, 0x78, 0x00},
    {0x02, 0x79, 0x00},
    {0x02, 0x7A, 0x00},
    {0x02, 0x7B, 0x00},
    {0x02, 0x7C, 0x00},
    {0x02, 0x7D, 0x00},
    {0x02, 0x7E, 0x00},
    {0x02, 0x7F, 0x00},
    {0x02, 0x80, 0x00},
    {0x02, 0x81, 0x00},
    {0x02, 0x82, 0x00},
    {0x02, 0x83, 0x00},
    {0x02, 0x84, 0x82},
    {0x02, 0x85, 0x00},
    {0x02, 0x86, 0x02},
    {0x02, 0x87, 0x00},
    {0x02, 0x88, 0x2C},
    {0x02, 0x89, 0x2C},
    {0x02, 0x8A, 0x00},
    {0x02, 0x8B, 0x00},
    {0x02, 0x8C, 0xF2},
    {0x02, 0x8D, 0x19},
    {0x02, 0x8E, 0x00},
    {0x02, 0x8F, 0x05},
    {0x02, 0x90, 0x20},
    {0x02, 0x91, 0x00},
    {0x02, 0x92, 0x00},
    {0x02, 0x93, 0x40},
    {0x02, 0x94, 0x00},
    {0x02, 0x95, 0x00},
    {0x02, 0x96, 0x30},
    {0x02, 0x97, 0x00},
    {0x02, 0x98, 0x00},
    {0x02, 0x99, 0x00},
    {0x02, 0x9A, 0x18},
    {0x02, 0x9B, 0x00},
    {0x02, 0x9C, 0x00},
    {0x02, 0x9D, 0x00},
    {0x02, 0x9E, 0x00},
    {0x02, 0x9F, 0x00},
    {0x02, 0xA0, 0x00},
    {0x02, 0xA1, 0x00},
    {0x02, 0xA2, 0x00},
    {0x02, 0xA3, 0x02},
    {0x02, 0xA4, 0x04},
    {0x02, 0xA5, 0x0F},
    {0x02, 0xA6, 0x00},
    {0x02, 0xA7, 0x01},
    {0x02, 0xA8, 0x48},
    {0x02, 0xA9, 0x05},
    {0x02, 0xAA, 0x00},
    {0x02, 0xAB, 0x00},
    {0x02, 0xAC, 0x00},
    {0x02, 0xAD, 0xFF},
    {0x02, 0xAE, 0x01},
    {0x02, 0xAF, 0xFF},
    {0x02, 0xB0, 0x02},
    {0x02, 0xB1, 0x7F},
    {0x02, 0xB2, 0x03},
    {0x02, 0xB3, 0xBF},
    {0x02, 0xB4, 0x03},
    {0x02, 0xB5, 0x00},
    {0x02, 0xB6, 0x00},
    {0x02, 0xB7, 0x0A},
    {0x02, 0xB8, 0x00},
    {0x02, 0xB9, 0x00},
    {0x02, 0xBA, 0x00},
    {0x02, 0xBB, 0x00},
    {0x02, 0xBC, 0x00},
    {0x02, 0xBD, 0x00},
    {0x02, 0xBE, 0x00},
    {0x02, 0xBF, 0x00},
    {0x02, 0xC0, 0x00},
    {0x02, 0xC1, 0x00},
    {0x02, 0xC2, 0x00},
    {0x02, 0xC3, 0x00},
    {0x02, 0xC4, 0x00},
    {0x02, 0xC5, 0x00},
    {0x02, 0xC6, 0x00},
    {0x02, 0xC7, 0x00},
    {0x02, 0xC8, 0x00},
    {0x02, 0xC9, 0x00},
    {0x02, 0xCA, 0x00},
    {0x02, 0xCB, 0x00},
    {0x02, 0xCC, 0x00},
    {0x02, 0xCD, 0x00},
    {0x02, 0xCE, 0x00},
    {0x02, 0xCF, 0x00},
    {0x02, 0xD0, 0x00},
    {0x02, 0xD1, 0x00},
    {0x02, 0xD2, 0x00},
    {0x02, 0xD3, 0x10},
    {0x02, 0xD4, 0x00},
    {0x02, 0xD5, 0x10},
    {0x02, 0xD6, 0x40},
    {0x02, 0xD7, 0x20},
    {0x02, 0xD8, 0x66},
    {0x02, 0xD9, 0x0E},
    {0x02, 0xDA, 0x01},
    {0x02, 0xDB, 0x06},
    {0x02, 0xDC, 0x06},
    {0x02, 0xDD, 0x0C},
    {0x02, 0xDE, 0x10},
    {0x02, 0xDF, 0x00},
    {0x02, 0xE0, 0x7C},
    {0x02, 0xE1, 0x00},
    {0x02, 0xE2, 0x00},
    {0x02, 0xE3, 0xFF},
    {0x02, 0xE4, 0xFF},
    {0x02, 0xE5, 0xFF},
    {0x02, 0xE6, 0xFF},
    {0x02, 0xE7, 0xFF},
    {0x02, 0xE8, 0xFF},
    {0x02, 0xE9, 0x00},
    {0x02, 0xEA, 0xC9},
    {0x02, 0xEB, 0x00},
    {0x02, 0xEC, 0x00},
    {0x02, 0xED, 0x00},
    {0x02, 0xEE, 0x12},
    {0x02, 0xEF, 0x01},
    {0x02, 0xF0, 0x00},
    {0x02, 0xF1, 0x00},
    {0x02, 0xF2, 0x00},
    {0x02, 0xF3, 0x00},
    {0x02, 0xF4, 0x00},
    {0x02, 0xF5, 0x00},
    {0x02, 0xF6, 0x00},
    {0x02, 0xF7, 0x00},
    {0x02, 0xF8, 0x00},
    {0x02, 0xF9, 0x00},
    {0x02, 0xFA, 0x00},
    {0x02, 0xFB, 0x00},
    {0x02, 0xFC, 0x00},
    {0x02, 0xFD, 0x00},
    {0x02, 0xFE, 0x00},
    {0x03, 0x00, 0x01},
    {0x03, 0x01, 0xA0},
    {0x03, 0x02, 0x3F},
    {0x03, 0x03, 0x01},
    {0x03, 0x04, 0xF0},
    {0x03, 0x05, 0x00},
    {0x03, 0x06, 0xF0},
    {0x03, 0x07, 0xA0},
    {0x03, 0x08, 0x10},
    {0x03, 0x09, 0x10},
    {0x03, 0x0A, 0x24},
    {0x03, 0x0B, 0x42},
    {0x03, 0x0C, 0xFA},
    {0x03, 0x0D, 0x43},
    {0x03, 0x0E, 0x43},
    {0x03, 0x0F, 0x46},
    {0x03, 0x10, 0x04},
    {0x03, 0x11, 0x41},
    {0x03, 0x12, 0x35},
    {0x03, 0x13, 0x41},
    {0x03, 0x14, 0x35},
    {0x03, 0x15, 0x51},
    {0x03, 0x16, 0x49},
    {0x03, 0x17, 0x41},
    {0x03, 0x18, 0x35},
    {0x03, 0x19, 0x0 },
    {0x03, 0x1A, 0x00},
    {0x03, 0x1B, 0x0 },
    {0x03, 0x1C, 0x00},
    {0x03, 0x1D, 0x00},
    {0x03, 0x1E, 0x00},
    {0x03, 0x1F, 0x09},
    {0x03, 0x20, 0xC1},
    {0x03, 0x21, 0x10},
    {0x03, 0x22, 0x01},
    {0x03, 0x23, 0x60},
    {0x03, 0x24, 0x44},
    {0x03, 0x25, 0xFA},
    {0x03, 0x26, 0x43},
    {0x03, 0x27, 0x43},
    {0x03, 0x28, 0x01},
    {0x03, 0x29, 0x40},
    {0x03, 0x2A, 0x43},
    {0x03, 0x2B, 0xFA},
    {0x03, 0x2C, 0x43},
    {0x03, 0x2D, 0x43},
    {0x03, 0x2E, 0x4A},
    {0x03, 0x2F, 0xB4},
    {0x03, 0x30, 0x3F},
    {0x03, 0x31, 0x01},
    {0x03, 0x32, 0xD0},
    {0x03, 0x33, 0x00},
    {0x03, 0x34, 0x00},
    {0x03, 0x35, 0x10},
    {0x03, 0x36, 0x01},
    {0x03, 0x37, 0xFA},
    {0x03, 0x38, 0x13},
    {0x03, 0x39, 0x00},
    {0x03, 0x3A, 0xFA},
    {0x03, 0x3B, 0x13},
    {0x03, 0x3C, 0x00},
    {0x03, 0x3D, 0x01},
    {0x03, 0x3E, 0xC0},
    {0x03, 0x3F, 0x02},
    {0x03, 0x40, 0x02},
    {0x03, 0x41, 0x10},
    {0x03, 0x42, 0x00},
    {0x03, 0x43, 0xEE},
    {0x03, 0x44, 0xF0},
    {0x03, 0x45, 0x0E},
    {0x03, 0x46, 0x21},
    {0x03, 0x47, 0x10},
    {0x03, 0x48, 0x00},
    {0x03, 0x49, 0x01},
    {0x03, 0x4A, 0x00},
    {0x03, 0x4B, 0x28},
    {0x03, 0x4C, 0x00},
    {0x03, 0x4D, 0x31},
    {0x03, 0x4E, 0x00},
    {0x03, 0x4F, 0x31},
    {0x03, 0x50, 0x00},
    {0x03, 0x51, 0x16},
    {0x03, 0x52, 0x01},
    {0x03, 0x53, 0x16},
    {0x03, 0x54, 0x01},
    {0x03, 0x55, 0x01},
    {0x03, 0x56, 0x00},
    {0x03, 0x57, 0x36},
    {0x03, 0x58, 0x00},
    {0x03, 0x59, 0x0F},
    {0x03, 0x5A, 0x00},
    {0x03, 0x5B, 0xFA},
    {0x03, 0x5C, 0x03},
    {0x03, 0x5D, 0x0E},
    {0x03, 0x5E, 0x00},
    {0x03, 0x5F, 0xF1},
    {0x03, 0x60, 0x00},
    {0x03, 0x61, 0x02},
    {0x03, 0x62, 0x00},
    {0x03, 0x63, 0xF9},
    {0x03, 0x64, 0x03},
    {0x03, 0x65, 0x02},
    {0x03, 0x66, 0x00},
    {0x03, 0x67, 0x01},
    {0x03, 0x68, 0x00},
    {0x03, 0x69, 0x34},
    {0x03, 0x6A, 0x00},
    {0x03, 0x6B, 0x2D},
    {0x03, 0x6C, 0x00},
    {0x03, 0x6D, 0xFB},
    {0x03, 0x6E, 0x00},
    {0x03, 0x6F, 0xF6},
    {0x03, 0x70, 0x00},
    {0x03, 0x71, 0x01},
    {0x03, 0x72, 0x00},
    {0x03, 0x73, 0x2D},
    {0x03, 0x74, 0x00},
    {0x03, 0x75, 0x03},
    {0x03, 0x76, 0x00},
    {0x03, 0x77, 0x01},
    {0x03, 0x78, 0x00},
    {0x03, 0x79, 0x2F},
    {0x03, 0x7A, 0x00},
    {0x03, 0x7B, 0x2D},
    {0x03, 0x7C, 0x00},
    {0x03, 0x7D, 0xF8},
    {0x03, 0x7E, 0x00},
    {0x03, 0x7F, 0xF6},
    {0x03, 0x80, 0x00},
    {0x03, 0x81, 0x01},
    {0x03, 0x82, 0x00},
    {0x03, 0x83, 0xF6},
    {0x03, 0x84, 0x00},
    {0x03, 0x85, 0x0B},
    {0x03, 0x86, 0x00},
    {0x03, 0x87, 0x16},
    {0x03, 0x88, 0x00},
    {0x03, 0x89, 0x21},
    {0x03, 0x8A, 0x00},
    {0x03, 0x8B, 0x00},
    {0x03, 0x8C, 0x00},
    {0x03, 0x8D, 0x00},
    {0x03, 0x8E, 0x00},
    {0x03, 0x8F, 0x00},
    {0x03, 0x90, 0x00},
    {0x03, 0x91, 0x00},
    {0x03, 0x92, 0x00},
    {0x03, 0x93, 0x00},
    {0x03, 0x94, 0x00},
    {0x03, 0x95, 0x00},
    {0x03, 0x96, 0x00},
    {0x03, 0x97, 0x00},
    {0x03, 0x98, 0xF7},
    {0x03, 0x99, 0x00},
    {0x03, 0x9A, 0x0B},
    {0x03, 0x9B, 0x01},
    {0x03, 0x9C, 0x06},
    {0x03, 0x9D, 0x00},
    {0x03, 0x9E, 0x0E},
    {0x03, 0x9F, 0x00},
    {0x03, 0xA0, 0x00},
    {0x03, 0xA1, 0x00},
    {0x03, 0xA2, 0x00},
    {0x03, 0xA3, 0x00},
    {0x03, 0xA4, 0x00},
    {0x03, 0xA5, 0x00},
    {0x03, 0xA6, 0x00},
    {0x03, 0xA7, 0x00},
    {0x03, 0xA8, 0x00},
    {0x03, 0xA9, 0x00},
    {0x03, 0xAA, 0x00},
    {0x03, 0xAB, 0x00},
    {0x03, 0xAC, 0x16},
    {0x03, 0xAD, 0x00},
    {0x03, 0xAE, 0x07},
    {0x03, 0xAF, 0x00},
    {0x03, 0xB0, 0x16},
    {0x03, 0xB1, 0x00},
    {0x03, 0xB2, 0x07},
    {0x03, 0xB3, 0x00},
    {0x03, 0xB4, 0x0E},
    {0x03, 0xB5, 0x00},
    {0x03, 0xB6, 0x0B},
    {0x03, 0xB7, 0x01},
    {0x03, 0xB8, 0x0B},
    {0x03, 0xB9, 0x01},
    {0x03, 0xBA, 0x0E},
    {0x03, 0xBB, 0x00},
    {0x03, 0xBC, 0x0E},
    {0x03, 0xBD, 0x00},
    {0x03, 0xBE, 0x0B},
    {0x03, 0xBF, 0x01},
    {0x03, 0xC0, 0x0F},
    {0x03, 0xC1, 0x01},
    {0x03, 0xC2, 0x13},
    {0x03, 0xC3, 0x01},
    {0x03, 0xC4, 0x12},
    {0x03, 0xC5, 0x00},
    {0x03, 0xC6, 0x16},
    {0x03, 0xC7, 0x00},
    {0x03, 0xC8, 0xF7},
    {0x03, 0xC9, 0x30},
    {0x03, 0xCA, 0x11},
    {0x03, 0xCB, 0x06},
    {0x03, 0xCC, 0x60},
    {0x03, 0xCD, 0x01},
    {0x03, 0xCE, 0x36},
    {0x03, 0xCF, 0x00},
    {0x03, 0xD0, 0xEF},
    {0x03, 0xD1, 0x60},
    {0x03, 0xD2, 0x13},
    {0x03, 0xD3, 0xFB},
    {0x03, 0xD4, 0x03},
    //{0x03, 0xD5, 0x5E},
    //{0x03, 0xD6, 0x08},
    //{0x03, 0xD7, 0x62},
    {0x03, 0xD5, 0x24}, //1080p, 30fps
    {0x03, 0xD6, 0x04}, //1080p, 30fps
    {0x03, 0xD7, 0x28}, //1080p, 30fps

    {0x03, 0xD8, 0x04},
    //{0x03, 0xD8, 0x08},
    {0x03, 0xD9, 0x00},
    //{0x03, 0xD9, 0x04}, //1080p, 30fps

    {0x03, 0xDA, 0x00},
    {0x03, 0xDB, 0x00},
    {0x03, 0xDC, 0x00},
    {0x03, 0xDD, 0x3F},
    {0x03, 0xDE, 0x00},
    {0x03, 0xDF, 0xEE},
    {0x03, 0xE0, 0x00},
    {0x03, 0xE1, 0x3F},
    {0x03, 0xE2, 0x01},
    {0x03, 0xE3, 0xFA},
    {0x03, 0xE4, 0x03},
    {0x03, 0xE5, 0x00},
    {0x03, 0xE6, 0x00},
    {0x03, 0xE7, 0x00},
    {0x03, 0xE8, 0x00},
    {0x03, 0xE9, 0x00},
    {0x03, 0xEA, 0x00},
    {0x03, 0xEB, 0x00},
    {0x03, 0xEC, 0x00}

};
#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))

typedef struct gain_set {
    u16 analog;
    u8  digit;
    u32 gain_x;
} gain_set_t;

static const gain_set_t gain_table[] = {
    {0x000, 0x00,   64}, {0x023, 0x00,   66}, {0x044, 0x00,   69}, {0x065, 0x00,   71}, {0x084, 0x00,   73},
    {0x0a2, 0x00,   76}, {0x0c0, 0x00,   79}, {0x0dc, 0x00,   82}, {0x0f7, 0x00,   84}, {0x112, 0x00,   87},
    {0x12b, 0x00,   90}, {0x144, 0x00,   94}, {0x15b, 0x00,   97}, {0x172, 0x00,  100}, {0x189, 0x00,  104},
    {0x19e, 0x00,  107}, {0x1b3, 0x00,  111}, {0x1c7, 0x00,  115}, {0x1da, 0x00,  119}, {0x1ed, 0x00,  123},
    {0x1ff, 0x00,  128}, {0x210, 0x00,  132}, {0x221, 0x00,  137}, {0x231, 0x00,  142}, {0x241, 0x00,  147},
    {0x250, 0x00,  152}, {0x25f, 0x00,  157}, {0x26d, 0x00,  163}, {0x27b, 0x00,  168}, {0x288, 0x00,  174},
    {0x295, 0x00,  180}, {0x2a1, 0x00,  187}, {0x2ad, 0x00,  193}, {0x2b8, 0x00,  200}, {0x2c4, 0x00,  207},
    {0x2ce, 0x00,  214}, {0x2d9, 0x00,  222}, {0x2e3, 0x00,  230}, {0x2ec, 0x00,  238}, {0x2f6, 0x00,  246},
    {0x2ff, 0x00,  255}, {0x308, 0x00,  264}, {0x310, 0x00,  273}, {0x318, 0x00,  283}, {0x320, 0x00,  293},
    {0x328, 0x00,  303}, {0x32f, 0x00,  313}, {0x336, 0x00,  324}, {0x33d, 0x00,  336}, {0x344, 0x00,  348},
    {0x34a, 0x00,  360}, {0x350, 0x00,  373}, {0x356, 0x00,  386}, {0x35c, 0x00,  399}, {0x361, 0x00,  413},
    {0x367, 0x00,  428}, {0x36c, 0x00,  443}, {0x371, 0x00,  458}, {0x376, 0x00,  474}, {0x37b, 0x00,  491},
    {0x37f, 0x00,  508}, {0x383, 0x00,  526}, {0x388, 0x00,  545}, {0x38c, 0x00,  564}, {0x390, 0x00,  584},
    {0x394, 0x00,  604}, {0x397, 0x00,  625}, {0x39b, 0x00,  647}, {0x39e, 0x00,  670}, {0x3a2, 0x00,  694},
    {0x3a5, 0x00,  718}, {0x3a8, 0x00,  743}, {0x3ab, 0x00,  769}, {0x3ae, 0x00,  796}, {0x3b1, 0x00,  824},
    {0x3b3, 0x00,  853}, {0x3b6, 0x00,  883}, {0x3b8, 0x00,  914}, {0x3bb, 0x00,  947}, {0x3bd, 0x00,  980},
    {0x3bf, 0x00, 1014}, {0x3bd, 0x01, 1014}, {0x3bf, 0x01, 1050}, {0x3bd, 0x02, 1050}, {0x3bf, 0x02, 1087},
    {0x3bd, 0x03, 1087}, {0x3bf, 0x03, 1125}, {0x3bd, 0x04, 1125}, {0x3bf, 0x04, 1165}, {0x3bd, 0x05, 1165},
    {0x3bf, 0x05, 1206}, {0x3bd, 0x06, 1206}, {0x3bf, 0x06, 1248}, {0x3bd, 0x07, 1248}, {0x3bf, 0x07, 1292},
    {0x3bd, 0x08, 1292}, {0x3bf, 0x08, 1337}, {0x3bd, 0x09, 1337}, {0x3bf, 0x09, 1384}, {0x3bd, 0x0a, 1384},
    {0x3bf, 0x0a, 1433}, {0x3bd, 0x0b, 1433}, {0x3bf, 0x0b, 1483}, {0x3bd, 0x0c, 1483}, {0x3bf, 0x0c, 1535},
    {0x3bd, 0x0d, 1535}, {0x3bf, 0x0d, 1589}, {0x3bd, 0x0e, 1589}, {0x3bf, 0x0e, 1645}, {0x3bd, 0x0f, 1645},
    {0x3bf, 0x0f, 1703}, {0x3bd, 0x10, 1703}, {0x3bf, 0x10, 1763}, {0x3bd, 0x11, 1763}, {0x3bf, 0x11, 1825},
    {0x3bd, 0x12, 1825}, {0x3bf, 0x12, 1889}, {0x3bd, 0x13, 1889}, {0x3bf, 0x13, 1955}, {0x3bd, 0x14, 1955},
    {0x3bf, 0x14, 2024}, {0x3bd, 0x15, 2024}, {0x3bf, 0x15, 2095}, {0x3bd, 0x16, 2095}, {0x3bf, 0x16, 2169},
    {0x3bd, 0x17, 2169}, {0x3bf, 0x17, 2245}, {0x3bd, 0x18, 2245}, {0x3bf, 0x18, 2324}, {0x3bd, 0x19, 2324},
    {0x3bf, 0x19, 2405}, {0x3bd, 0x1a, 2405}, {0x3bf, 0x1a, 2490}, {0x3bd, 0x1b, 2490}, {0x3bf, 0x1b, 2577},
    {0x3bd, 0x1c, 2577}, {0x3bf, 0x1c, 2668}, {0x3bd, 0x1d, 2668}, {0x3bf, 0x1d, 2762}, {0x3bd, 0x1e, 2762},
    {0x3bf, 0x1e, 2859}, {0x3bd, 0x1f, 2859}, {0x3bf, 0x1f, 2959}, {0x3bd, 0x20, 2959}, {0x3bf, 0x20, 3063},
    {0x3bd, 0x21, 3063}, {0x3bf, 0x21, 3171}, {0x3bd, 0x22, 3171}, {0x3bf, 0x22, 3282}, {0x3bd, 0x23, 3282},
    {0x3bf, 0x23, 3398}, {0x3bd, 0x24, 3398}, {0x3bf, 0x24, 3517}, {0x3bd, 0x25, 3517}, {0x3bf, 0x25, 3641},
    {0x3bd, 0x26, 3641}, {0x3bf, 0x26, 3769}, {0x3bd, 0x27, 3769}, {0x3bf, 0x27, 3901}, {0x3bd, 0x28, 3901},
    {0x3bf, 0x28, 4038}, {0x3bd, 0x29, 4038}, {0x3bf, 0x29, 4180}, {0x3bd, 0x2a, 4180}, {0x3bf, 0x2a, 4327},
    {0x3bd, 0x2b, 4327}, {0x3bf, 0x2b, 4479}, {0x3bd, 0x2c, 4479}, {0x3bf, 0x2c, 4636}, {0x3bd, 0x2d, 4636},
    {0x3bf, 0x2d, 4799}, {0x3bd, 0x2e, 4799}, {0x3bf, 0x2e, 4968}, {0x3bd, 0x2f, 4968}, {0x3bf, 0x2f, 5143},
    {0x3bd, 0x30, 5143}, {0x3bf, 0x30, 5323}, {0x3bd, 0x31, 5323}, {0x3bf, 0x31, 5510}, {0x3bd, 0x32, 5510},
    {0x3bf, 0x32, 5704}, {0x3bd, 0x33, 5704}, {0x3bf, 0x33, 5904}, {0x3bd, 0x34, 5904}, {0x3bf, 0x34, 6112},
    {0x3bd, 0x35, 6112}, {0x3bf, 0x35, 6327}, {0x3bd, 0x36, 6327}, {0x3bf, 0x36, 6549}, {0x3bd, 0x37, 6549},
    {0x3bf, 0x37, 6779}, {0x3bd, 0x38, 6779}, {0x3bf, 0x38, 7017}, {0x3bd, 0x39, 7017}, {0x3bf, 0x39, 7264},
    {0x3bd, 0x3a, 7264}, {0x3bf, 0x3a, 7519}, {0x3bd, 0x3b, 7519},
    {0x3bf, 0x3b, 7784}, {0x3bd, 0x3c, 7784}, {0x3bf, 0x3c, 8057}
};
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

//============================================================================
// SPI
//============================================================================
#define SPI_CLK_SET()   isp_api_spi_clk_set_value(1)
#define SPI_CLK_CLR()   isp_api_spi_clk_set_value(0)
#define SPI_DI_READ()   isp_api_spi_di_get_value()
#define SPI_DO_SET()    isp_api_spi_do_set_value(1)
#define SPI_DO_CLR()    isp_api_spi_do_set_value(0)
#define SPI_CS_SET()    isp_api_spi_cs_set_value(1)
#define SPI_CS_CLR()    isp_api_spi_cs_set_value(0)

//============================================================================
// internal functions
//============================================================================
static void _delay(void)
{
    int i;
    for (i=0; i<0x1f; i++);
}

static int imx036_read_reg(u8 id, u8 addr, u32 *pval)
{
    int i;
    u32 tmp;

    _delay();
    SPI_CS_CLR();
    _delay();

    // address phase
    tmp = (addr << 8) | (0x80 | id);
    for (i=0; i<16; i++) {
        SPI_CLK_CLR();
        _delay();
        if (tmp & BIT0)
            SPI_DO_SET();
        else
            SPI_DO_CLR();
        tmp >>= 1;
        SPI_CLK_SET();
        _delay();
    }

    // data phase
    *pval = 0;
    for (i=0; i<8; i++) {
        SPI_CLK_CLR();
        _delay();
        SPI_CLK_SET();
        _delay();
        if (SPI_DI_READ() & BIT0)
            *pval |= (1 << i);
    }
    SPI_CS_SET();

    return 0;
}

static int imx036_write_reg(u8 id, u8 addr, u32 val)
{
    int i;
    u32 tmp;

    _delay();
    SPI_CS_CLR();
    _delay();

    // address phase & data phase
    tmp = (val << 16) | (addr << 8) | id;
    for (i=0; i<24; i++) {
        SPI_CLK_CLR();
        _delay();
        if (tmp & BIT0)
            SPI_DO_SET();
        else
            SPI_DO_CLR();
        tmp >>= 1;
        SPI_CLK_SET();
        _delay();
    }
    SPI_CS_SET();

    return 0;
}

static int read_reg(u32 addr, u32 *pval)
{
    u8 id, r_addr;

    id = (addr >> 8) & 0xFF;
    r_addr = addr & 0xFF;
    return imx036_read_reg(id, r_addr, pval);
}

static int write_reg(u32 addr, u32 val)
{
    u8 id, r_addr;

    id = (addr >> 8) & 0xFF;
    r_addr = addr & 0xFF;
    return imx036_write_reg(id, r_addr, val);
}

static u32 get_pclk(u32 xclk)
{
    u32 val, pclk;
    xclk = xclk;

    imx036_read_reg(0x2, 0x4, &val);
    if (val == 0x11)        //1080p, 15 fps
        pclk = 37125000;
    else if(val == 0xd)     //1080p, 30 fps
        pclk = 74250000;

    printk("**imx036 pclk** = %d\n", pclk);
    return pclk;
}

static int set_size(u32 width, u32 height)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 val_l, val_m, val_h;

    if ((width > 1920) || (height > 1080))
        return -EINVAL;

    imx036_read_reg(0x2, 0x07, &val_l);
    imx036_read_reg(0x2, 0x08, &val_h);
    pinfo->HMax = (val_h << 8) | val_l;

    imx036_read_reg(0x2, 0x09, &val_l);
    imx036_read_reg(0x2, 0x0A, &val_m);
    imx036_read_reg(0x2, 0x0B, &val_h);
    pinfo->VMax = (val_h << 16) | (val_m << 8) | val_l;

    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    g_psensor->out_win.width = H_CYCLE;
    g_psensor->out_win.height = V_CYCLE;

    g_psensor->img_win.x = (72 + ((H_VALID - width) >> 1)) &~BIT0;
    g_psensor->img_win.y = (20 + ((V_VALID - height) >> 1)) &~BIT0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    g_psensor->pclk = get_pclk(g_psensor->xclk);
    pinfo->t_row = (pinfo->HMax * 10000) / (g_psensor->pclk / 1000);
    g_psensor->min_exp = (pinfo->t_row + 500) / 1000;

    return ret;
}

static u32 get_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 val_l, val_m, val_h;
    u32 lines, vmax, shs1;

    if (!g_psensor->curr_exp) {
        imx036_read_reg(0x2, 0x09, &val_l);
        imx036_read_reg(0x2, 0x0A, &val_m);
        imx036_read_reg(0x2, 0x0B, &val_h);
        vmax = (val_h << 16) | (val_m << 8) | val_l;

        imx036_read_reg(0x2, 0x0C, &val_l);
        imx036_read_reg(0x2, 0x0D, &val_m);
        imx036_read_reg(0x2, 0x0E, &val_h);
        shs1 = (val_h << 16) | (val_m << 8) | val_l;
        // exposure = (VMAX - SHS1 - Toffset)H, Toffset = 806/HMAX =~ 0.8H
        lines = vmax - shs1 - 1;
        g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 lines, shs1;

    lines = ((exp * 1000)+pinfo->t_row/2) / pinfo->t_row;

    //printk("exp=%d, lines=%d, VMax=%d\n", exp, lines, pinfo->VMax);

    if (lines == 0)
        lines = 1;

    if (lines > pinfo->VMax) {
        imx036_write_reg(0x2, 0x09, (lines & 0xFF));
        imx036_write_reg(0x2, 0x0A, ((lines >> 8) & 0xFF));
        imx036_write_reg(0x2, 0x0B, ((lines >> 16) & 0xFF));
        imx036_write_reg(0x2, 0x0C, 0x0);
        imx036_write_reg(0x2, 0x0D, 0x0);
        //write_reg(0x2, 0x0E, 0x0);
    } else {
        imx036_write_reg(0x2, 0x09, (pinfo->VMax & 0xFF));
        imx036_write_reg(0x2, 0x0A, ((pinfo->VMax >> 8) & 0xFF));
        imx036_write_reg(0x2, 0x0B, ((pinfo->VMax >> 16) & 0xFF));

        shs1 = pinfo->VMax - lines;
        imx036_write_reg(0x2, 0x0C, (shs1 & 0xFF));
        imx036_write_reg(0x2, 0x0D, ((shs1 >> 8) & 0xFF));
        //write_reg(0x2, 0x0E, ((shs1 >> 16) & 0xFF));
    }

    g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;

    //printk("g_psensor->curr_exp=%d\n", g_psensor->curr_exp);

    return ret;
}

static u32 get_gain(void)
{
    u32 val_l, val_h;
    u32 ang_x, dig_x;
    int i;

    if (g_psensor->curr_gain == 0) {
        imx036_read_reg(0x2, 0x5, &val_l);
        imx036_read_reg(0x2, 0x6, &val_h);
        ang_x = (val_h << 8) | val_l;
        imx036_read_reg(0x2, 0x61, &dig_x);

        for (i=0; i<NUM_OF_GAINSET; i++) {
            if ((gain_table[i].analog == ang_x) && (gain_table[i].digit == dig_x))
                break;
        }
        if (i < NUM_OF_GAINSET)
            g_psensor->curr_gain = gain_table[i].gain_x;
    }

    return g_psensor->curr_gain;
}

static int set_gain(u32 gain)
{
    int ret = 0;
    u32 ang_x, dig_x;
    int i;

    // search most suitable gain into gain table
    ang_x = gain_table[0].analog;
    dig_x = gain_table[0].digit;

    for (i=0; i<NUM_OF_GAINSET; i++) {
        if (gain_table[i].gain_x > gain)
            break;
    }
    if (i > 0) {
        ang_x = gain_table[i-1].analog;
        dig_x = gain_table[i-1].digit;
    }

    imx036_write_reg(0x2, 0x06, (ang_x >> 8));
    imx036_write_reg(0x2, 0x05, (ang_x & 0xFF));
    imx036_write_reg(0x2, 0x61, dig_x);
    g_psensor->curr_gain = gain_table[i-1].gain_x;

    //printk("g_psensor->curr_gain=%d\n",g_psensor->curr_gain);

    return ret;
}

static int get_mirror(void)
{
    return 0;
}

static int set_mirror(int enable)
{
    return 0;
}

static int get_flip(void)
{
    return 0;
}

static int set_flip(int enable)
{
    return 0;
}

static int get_property(enum SENSOR_CMD cmd, unsigned long arg)
{
    int ret = 0;

    switch (cmd) {
    case ID_MIRROR:
        *(int*)arg = get_mirror();
        break;
    case ID_FLIP:
        *(int*)arg = get_flip();
        break;
    default:
        ret = -EFAULT;
        break;
    }

    return ret;
}

static int set_property(enum SENSOR_CMD cmd, unsigned long arg)
{
    int ret = 0;

    switch (cmd) {
    case ID_MIRROR:
        set_mirror((int)arg);
        break;
    case ID_FLIP:
        set_flip((int)arg);
        break;
    default:
        ret = -EFAULT;
        break;
    }

    return ret;
}

static int init(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    int i;

    if (pinfo->is_init)
        return 0;

    for (i=0; i<NUM_OF_INIT_REGS; i++) {
        if (imx036_write_reg(sensor_init_regs[i].id, sensor_init_regs[i].addr, sensor_init_regs[i].val) < 0) {
            isp_err("%s init fail!!", SENSOR_NAME);
            return -EINVAL;
        }
    }

    // get mirror and flip status
    pinfo->mirror = get_mirror();
    pinfo->flip = get_flip();

    // set image size
    ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height);
    if (ret == 0)
        pinfo->is_init = 1;

    // initial exposure and gain
    set_exp(g_psensor->min_exp);
    set_gain(g_psensor->min_gain);

    // get current shutter_width and gain setting
    get_exp();
    get_gain();

    return ret;
}

//============================================================================
// external functions
//=============================================================================
static void imx036_deconstruct(void)
{
    if ((g_psensor) && (g_psensor->private)) {
        kfree(g_psensor->private);
    }

    isp_api_spi_exit();

    // turn off EXT_CLK
    isp_api_extclk_onoff(0);
    // release CAP_RST
    isp_api_cap_rst_exit();
}

static int imx036_construct(u32 xclk, u16 width, u16 height)
{
    sensor_info_t *pinfo = NULL;
    int ret = 0;

    // initial CAP_RST
    ret = isp_api_cap_rst_init();
    if (ret < 0)
        return ret;

    // clear CAP_RST
    isp_api_cap_rst_set_value(0);

    // set EXT_CLK frequency and turn on it.
    ret = isp_api_extclk_set_freq(xclk);
    if (ret < 0)
        return ret;
    ret = isp_api_extclk_onoff(1);
    if (ret < 0)
        return ret;
    mdelay(50);

    // set CAP_RST
    isp_api_cap_rst_set_value(1);

    pinfo = kzalloc(sizeof(sensor_info_t), GFP_KERNEL);
    if (!pinfo) {
        isp_err("failed to allocate private data!");
        return -ENOMEM;
    }
    g_psensor->private = pinfo;

    snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, SENSOR_NAME);
    g_psensor->capability = SUPPORT_MIRROR | SUPPORT_FLIP;
    g_psensor->xclk = xclk;
    g_psensor->interface = IF_PARALLEL;
    g_psensor->num_of_lane = 0;
    g_psensor->protocol = 0;
    g_psensor->fmt = RAW12;
    g_psensor->bayer_type = BAYER_BG;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_HIGH;
    g_psensor->inv_clk = inv_clk;
    g_psensor->img_win.x = 72;
    g_psensor->img_win.y = 34;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;
    g_psensor->min_exp = 1;
    g_psensor->max_exp = 5000; // 0.5 sec
    g_psensor->min_gain = 64;
    g_psensor->max_gain = 8057;
    g_psensor->exp_latency = 1;
    g_psensor->gain_latency = 0;
    g_psensor->fps = fps;
    g_psensor->fn_init = init;
    g_psensor->fn_read_reg = read_reg;
    g_psensor->fn_write_reg = write_reg;
    g_psensor->fn_set_size = set_size;
    g_psensor->fn_get_exp = get_exp;
    g_psensor->fn_set_exp = set_exp;
    g_psensor->fn_get_gain = get_gain;
    g_psensor->fn_set_gain = set_gain;
    g_psensor->fn_get_property = get_property;
    g_psensor->fn_set_property = set_property;


    ret = isp_api_spi_init();
    if (ret)
        return ret;

    if ((ret = init()) < 0)
        goto construct_err;

    return 0;

construct_err:
    imx036_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init imx036_init(void)
{
    int ret = 0;

    // check ISP driver version
    if (isp_get_input_inf_ver() != ISP_INPUT_INF_VER) {
        isp_err("input module version(%x) is not equal to fisp328.ko(%x)!",
             ISP_INPUT_INF_VER, isp_get_input_inf_ver());
        ret = -EINVAL;
        goto init_err1;
    }

    g_psensor = kzalloc(sizeof(sensor_dev_t), GFP_KERNEL);
    if (!g_psensor) {
        ret = -ENODEV;
        goto init_err1;
    }

    if ((ret = imx036_construct(sensor_xclk, sensor_w, sensor_h)) < 0){
        printk("imx036_construct Error \n");
        goto init_err2;
        }
    // register sensor device to ISP driver

    if ((ret = isp_register_sensor(g_psensor)) < 0){
        printk("isp_register_sensor Error \n");
        goto init_err2;
     }

    return ret;

init_err2:
    kfree(g_psensor);

init_err1:
    return ret;
}

static void __exit imx036_exit(void)
{
    isp_unregister_sensor(g_psensor);
    imx036_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(imx036_init);
module_exit(imx036_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor IMX036");
MODULE_LICENSE("GPL");
