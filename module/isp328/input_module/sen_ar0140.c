/**
 * @file sen_ar0140.c
 * Aptina AR0140 sensor driver
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
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
#include "isp_input_inf.h"
#include "isp_api.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1280
#define DEFAULT_IMAGE_HEIGHT    720
#define DEFAULT_XCLK            27000000
#define DELAY_CODE              0xFFFF

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

static uint binning = 0;
module_param(binning, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(binning, "sensor binning");

static uint inv_clk = 0;
module_param(inv_clk, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "invert clock, 0:Disable, 1: Enable");

static uint fps = 30;
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "sensor frame rate");

static uint wdr = 0;
module_param(wdr, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(wdr, "WDR mode");


//=============================================================================
// global
//=============================================================================
#define SENSOR_NAME         "AR0140"
#define SENSOR_MAX_WIDTH    1344
#define SENSOR_MAX_HEIGHT   848
//#define FPGA
#define WDR_EXP_LIMIT

#define ID_AR0140 0x51
#define ID_AR0141 0x151

static sensor_dev_t *g_psensor = NULL;

typedef struct sensor_info {
    bool is_init;
    u32 img_w;
    u32 img_h;
    int row_bin;
    int col_bin;
    int mirror;
    int flip;
    u32 hb;
    u32 t_row;          // unit is 1/10 us
    u32 sensor_id;
} sensor_info_t;

typedef struct sensor_reg {
    u16  addr;
    u16  val;
} sensor_reg_t;

static sensor_reg_t sensor_init_regs[] = {    
    //[HDR (Parallel) 720p30 - maximize frame length]
    {DELAY_CODE, 200},
    {0x301A, 0x0001},   // RESET_REGISTER
    {DELAY_CODE, 200},
    {0x301A, 0x10D8},   // RESET_REGISTER
    {DELAY_CODE, 200},

    // [sequencer_hidy_0828_AR0140.i]
    {0x3088, 0x8000},   // SEQ_CTRL_PORT
    {0x3086, 0x4558},   // SEQ_DATA_PORT
    {0x3086, 0x6E9B},   // SEQ_DATA_PORT
    {0x3086, 0x4A31},   // SEQ_DATA_PORT
    {0x3086, 0x4342},   // SEQ_DATA_PORT
    {0x3086, 0x8E03},   // SEQ_DATA_PORT
    {0x3086, 0x2714},   // SEQ_DATA_PORT
    {0x3086, 0x4578},   // SEQ_DATA_PORT
    {0x3086, 0x7B3D},   // SEQ_DATA_PORT
    {0x3086, 0xFF3D},   // SEQ_DATA_PORT
    {0x3086, 0xFF3D},   // SEQ_DATA_PORT
    {0x3086, 0xEA27},   // SEQ_DATA_PORT
    {0x3086, 0x043D},   // SEQ_DATA_PORT
    {0x3086, 0x1027},   // SEQ_DATA_PORT
    {0x3086, 0x0527},   // SEQ_DATA_PORT
    {0x3086, 0x1535},   // SEQ_DATA_PORT
    {0x3086, 0x2705},   // SEQ_DATA_PORT
    {0x3086, 0x3D10},   // SEQ_DATA_PORT
    {0x3086, 0x4558},   // SEQ_DATA_PORT
    {0x3086, 0x2704},   // SEQ_DATA_PORT
    {0x3086, 0x2714},   // SEQ_DATA_PORT
    {0x3086, 0x3DFF},   // SEQ_DATA_PORT
    {0x3086, 0x3DFF},   // SEQ_DATA_PORT
    {0x3086, 0x3DEA},   // SEQ_DATA_PORT
    {0x3086, 0x2704},   // SEQ_DATA_PORT
    {0x3086, 0x6227},   // SEQ_DATA_PORT
    {0x3086, 0x288E},   // SEQ_DATA_PORT
    {0x3086, 0x0036},   // SEQ_DATA_PORT
    {0x3086, 0x2708},   // SEQ_DATA_PORT
    {0x3086, 0x3D64},   // SEQ_DATA_PORT
    {0x3086, 0x7A3D},   // SEQ_DATA_PORT
    {0x3086, 0x0444},   // SEQ_DATA_PORT
    {0x3086, 0x2C4B},   // SEQ_DATA_PORT
    {0x3086, 0x8F00},   // SEQ_DATA_PORT
    {0x3086, 0x4372},   // SEQ_DATA_PORT
    {0x3086, 0x719F},   // SEQ_DATA_PORT
    {0x3086, 0x6343},   // SEQ_DATA_PORT
    {0x3086, 0x166F},   // SEQ_DATA_PORT
    {0x3086, 0x9F92},   // SEQ_DATA_PORT
    {0x3086, 0x1244},   // SEQ_DATA_PORT
    {0x3086, 0x1663},   // SEQ_DATA_PORT
    {0x3086, 0x4316},   // SEQ_DATA_PORT
    {0x3086, 0x9326},   // SEQ_DATA_PORT
    {0x3086, 0x0426},   // SEQ_DATA_PORT
    {0x3086, 0x848E},   // SEQ_DATA_PORT
    {0x3086, 0x0327},   // SEQ_DATA_PORT
    {0x3086, 0xFC5C},   // SEQ_DATA_PORT
    {0x3086, 0x0D57},   // SEQ_DATA_PORT
    {0x3086, 0x5417},   // SEQ_DATA_PORT
    {0x3086, 0x0955},   // SEQ_DATA_PORT
    {0x3086, 0x5649},   // SEQ_DATA_PORT
    {0x3086, 0x5F53},   // SEQ_DATA_PORT
    {0x3086, 0x0553},   // SEQ_DATA_PORT
    {0x3086, 0x0728},   // SEQ_DATA_PORT
    {0x3086, 0x6C4C},   // SEQ_DATA_PORT
    {0x3086, 0x0928},   // SEQ_DATA_PORT
    {0x3086, 0x2C72},   // SEQ_DATA_PORT
    {0x3086, 0xAD7C},   // SEQ_DATA_PORT
    {0x3086, 0xA928},   // SEQ_DATA_PORT
    {0x3086, 0xA879},   // SEQ_DATA_PORT
    {0x3086, 0x6026},   // SEQ_DATA_PORT
    {0x3086, 0x9C5C},   // SEQ_DATA_PORT
    {0x3086, 0x1B45},   // SEQ_DATA_PORT
    {0x3086, 0x4845},   // SEQ_DATA_PORT
    {0x3086, 0x0845},   // SEQ_DATA_PORT
    {0x3086, 0x8826},   // SEQ_DATA_PORT
    {0x3086, 0xBE8E},   // SEQ_DATA_PORT
    {0x3086, 0x0127},   // SEQ_DATA_PORT
    {0x3086, 0xF817},   // SEQ_DATA_PORT
    {0x3086, 0x0227},   // SEQ_DATA_PORT
    {0x3086, 0xFA17},   // SEQ_DATA_PORT
    {0x3086, 0x095C},   // SEQ_DATA_PORT
    {0x3086, 0x0B17},   // SEQ_DATA_PORT
    {0x3086, 0x1026},   // SEQ_DATA_PORT
    {0x3086, 0xBA5C},   // SEQ_DATA_PORT
    {0x3086, 0x0317},   // SEQ_DATA_PORT
    {0x3086, 0x1026},   // SEQ_DATA_PORT
    {0x3086, 0xB217},   // SEQ_DATA_PORT
    {0x3086, 0x065F},   // SEQ_DATA_PORT
    {0x3086, 0x2888},   // SEQ_DATA_PORT
    {0x3086, 0x9060},   // SEQ_DATA_PORT
    {0x3086, 0x27F2},   // SEQ_DATA_PORT
    {0x3086, 0x1710},   // SEQ_DATA_PORT
    {0x3086, 0x26A2},   // SEQ_DATA_PORT
    {0x3086, 0x26A3},   // SEQ_DATA_PORT
    {0x3086, 0x5F4D},   // SEQ_DATA_PORT
    {0x3086, 0x2808},   // SEQ_DATA_PORT
    {0x3086, 0x1927},   // SEQ_DATA_PORT
    {0x3086, 0xFA84},   // SEQ_DATA_PORT
    {0x3086, 0x69A0},   // SEQ_DATA_PORT
    {0x3086, 0x785D},   // SEQ_DATA_PORT
    {0x3086, 0x2888},   // SEQ_DATA_PORT
    {0x3086, 0x8710},   // SEQ_DATA_PORT
    {0x3086, 0x8C82},   // SEQ_DATA_PORT
    {0x3086, 0x8926},   // SEQ_DATA_PORT
    {0x3086, 0xB217},   // SEQ_DATA_PORT
    {0x3086, 0x036B},   // SEQ_DATA_PORT
    {0x3086, 0x9C60},   // SEQ_DATA_PORT
    {0x3086, 0x9417},   // SEQ_DATA_PORT
    {0x3086, 0x2926},   // SEQ_DATA_PORT
    {0x3086, 0x8345},   // SEQ_DATA_PORT
    {0x3086, 0xA817},   // SEQ_DATA_PORT
    {0x3086, 0x0727},   // SEQ_DATA_PORT
    {0x3086, 0xFB17},   // SEQ_DATA_PORT
    {0x3086, 0x2945},   // SEQ_DATA_PORT
    {0x3086, 0x881F},   // SEQ_DATA_PORT
    {0x3086, 0x1708},   // SEQ_DATA_PORT
    {0x3086, 0x27FA},   // SEQ_DATA_PORT
    {0x3086, 0x5D87},   // SEQ_DATA_PORT
    {0x3086, 0x108C},   // SEQ_DATA_PORT
    {0x3086, 0x8289},   // SEQ_DATA_PORT
    {0x3086, 0x170E},   // SEQ_DATA_PORT
    {0x3086, 0x4826},   // SEQ_DATA_PORT
    {0x3086, 0x9A28},   // SEQ_DATA_PORT
    {0x3086, 0x884C},   // SEQ_DATA_PORT
    {0x3086, 0x0B79},   // SEQ_DATA_PORT
    {0x3086, 0x1730},   // SEQ_DATA_PORT
    {0x3086, 0x2692},   // SEQ_DATA_PORT
    {0x3086, 0x1709},   // SEQ_DATA_PORT
    {0x3086, 0x9160},   // SEQ_DATA_PORT
    {0x3086, 0x27F2},   // SEQ_DATA_PORT
    {0x3086, 0x1710},   // SEQ_DATA_PORT
    {0x3086, 0x2682},   // SEQ_DATA_PORT
    {0x3086, 0x2683},   // SEQ_DATA_PORT
    {0x3086, 0x5F4D},   // SEQ_DATA_PORT
    {0x3086, 0x2808},   // SEQ_DATA_PORT
    {0x3086, 0x1927},   // SEQ_DATA_PORT
    {0x3086, 0xFA84},   // SEQ_DATA_PORT
    {0x3086, 0x69A1},   // SEQ_DATA_PORT
    {0x3086, 0x785D},   // SEQ_DATA_PORT
    {0x3086, 0x2888},   // SEQ_DATA_PORT
    {0x3086, 0x8710},   // SEQ_DATA_PORT
    {0x3086, 0x8C80},   // SEQ_DATA_PORT
    {0x3086, 0x8A26},   // SEQ_DATA_PORT
    {0x3086, 0x9217},   // SEQ_DATA_PORT
    {0x3086, 0x036B},   // SEQ_DATA_PORT
    {0x3086, 0x9D95},   // SEQ_DATA_PORT
    {0x3086, 0x2603},   // SEQ_DATA_PORT
    {0x3086, 0x5C01},   // SEQ_DATA_PORT
    {0x3086, 0x4558},   // SEQ_DATA_PORT
    {0x3086, 0x8E00},   // SEQ_DATA_PORT
    {0x3086, 0x2798},   // SEQ_DATA_PORT
    {0x3086, 0x170A},   // SEQ_DATA_PORT
    {0x3086, 0x4A65},   // SEQ_DATA_PORT
    {0x3086, 0x4316},   // SEQ_DATA_PORT
    {0x3086, 0x6643},   // SEQ_DATA_PORT
    {0x3086, 0x165B},   // SEQ_DATA_PORT
    {0x3086, 0x4316},   // SEQ_DATA_PORT
    {0x3086, 0x5943},   // SEQ_DATA_PORT
    {0x3086, 0x168E},   // SEQ_DATA_PORT
    {0x3086, 0x0327},   // SEQ_DATA_PORT
    {0x3086, 0x9C45},   // SEQ_DATA_PORT
    {0x3086, 0x7817},   // SEQ_DATA_PORT
    {0x3086, 0x0727},   // SEQ_DATA_PORT
    {0x3086, 0x9D17},   // SEQ_DATA_PORT
    {0x3086, 0x225D},   // SEQ_DATA_PORT
    {0x3086, 0x8710},   // SEQ_DATA_PORT
    {0x3086, 0x2808},   // SEQ_DATA_PORT
    {0x3086, 0x530D},   // SEQ_DATA_PORT
    {0x3086, 0x8C80},   // SEQ_DATA_PORT
    {0x3086, 0x8A45},   // SEQ_DATA_PORT
    {0x3086, 0x5823},   // SEQ_DATA_PORT
    {0x3086, 0x1708},   // SEQ_DATA_PORT
    {0x3086, 0x8E01},   // SEQ_DATA_PORT
    {0x3086, 0x2798},   // SEQ_DATA_PORT
    {0x3086, 0x8E00},   // SEQ_DATA_PORT
    {0x3086, 0x2644},   // SEQ_DATA_PORT
    {0x3086, 0x5C05},   // SEQ_DATA_PORT
    {0x3086, 0x1244},   // SEQ_DATA_PORT
    {0x3086, 0x4B71},   // SEQ_DATA_PORT
    {0x3086, 0x759E},   // SEQ_DATA_PORT
    {0x3086, 0x8B85},   // SEQ_DATA_PORT
    {0x3086, 0x0143},   // SEQ_DATA_PORT
    {0x3086, 0x7271},   // SEQ_DATA_PORT
    {0x3086, 0xA346},   // SEQ_DATA_PORT
    {0x3086, 0x4316},   // SEQ_DATA_PORT
    {0x3086, 0x6FA3},   // SEQ_DATA_PORT
    {0x3086, 0x9612},   // SEQ_DATA_PORT
    {0x3086, 0x4416},   // SEQ_DATA_PORT
    {0x3086, 0x4643},   // SEQ_DATA_PORT
    {0x3086, 0x1697},   // SEQ_DATA_PORT
    {0x3086, 0x2604},   // SEQ_DATA_PORT
    {0x3086, 0x2684},   // SEQ_DATA_PORT
    {0x3086, 0x8E03},   // SEQ_DATA_PORT
    {0x3086, 0x27FC},   // SEQ_DATA_PORT
    {0x3086, 0x5C0D},   // SEQ_DATA_PORT
    {0x3086, 0x5754},   // SEQ_DATA_PORT
    {0x3086, 0x1709},   // SEQ_DATA_PORT
    {0x3086, 0x5556},   // SEQ_DATA_PORT
    {0x3086, 0x495F},   // SEQ_DATA_PORT
    {0x3086, 0x5305},   // SEQ_DATA_PORT
    {0x3086, 0x5307},   // SEQ_DATA_PORT
    {0x3086, 0x286C},   // SEQ_DATA_PORT
    {0x3086, 0x4C09},   // SEQ_DATA_PORT
    {0x3086, 0x282C},   // SEQ_DATA_PORT
    {0x3086, 0x72AE},   // SEQ_DATA_PORT
    {0x3086, 0x7CAA},   // SEQ_DATA_PORT
    {0x3086, 0x28A8},   // SEQ_DATA_PORT
    {0x3086, 0x7960},   // SEQ_DATA_PORT
    {0x3086, 0x269C},   // SEQ_DATA_PORT
    {0x3086, 0x5C1B},   // SEQ_DATA_PORT
    {0x3086, 0x4548},   // SEQ_DATA_PORT
    {0x3086, 0x4508},   // SEQ_DATA_PORT
    {0x3086, 0x4588},   // SEQ_DATA_PORT
    {0x3086, 0x26BE},   // SEQ_DATA_PORT
    {0x3086, 0x8E01},   // SEQ_DATA_PORT
    {0x3086, 0x27F8},   // SEQ_DATA_PORT
    {0x3086, 0x1702},   // SEQ_DATA_PORT
    {0x3086, 0x27FA},   // SEQ_DATA_PORT
    {0x3086, 0x1709},   // SEQ_DATA_PORT
    {0x3086, 0x5C0B},   // SEQ_DATA_PORT
    {0x3086, 0x1710},   // SEQ_DATA_PORT
    {0x3086, 0x26BA},   // SEQ_DATA_PORT
    {0x3086, 0x5C03},   // SEQ_DATA_PORT
    {0x3086, 0x1710},   // SEQ_DATA_PORT
    {0x3086, 0x26B2},   // SEQ_DATA_PORT
    {0x3086, 0x1706},   // SEQ_DATA_PORT
    {0x3086, 0x5F28},   // SEQ_DATA_PORT
    {0x3086, 0x8898},   // SEQ_DATA_PORT
    {0x3086, 0x6027},   // SEQ_DATA_PORT
    {0x3086, 0xF217},   // SEQ_DATA_PORT
    {0x3086, 0x1026},   // SEQ_DATA_PORT
    {0x3086, 0xA226},   // SEQ_DATA_PORT
    {0x3086, 0xA35F},   // SEQ_DATA_PORT
    {0x3086, 0x4D28},   // SEQ_DATA_PORT
    {0x3086, 0x081A},   // SEQ_DATA_PORT
    {0x3086, 0x27FA},   // SEQ_DATA_PORT
    {0x3086, 0x8469},   // SEQ_DATA_PORT
    {0x3086, 0xA578},   // SEQ_DATA_PORT
    {0x3086, 0x5D28},   // SEQ_DATA_PORT
    {0x3086, 0x8887},   // SEQ_DATA_PORT
    {0x3086, 0x108C},   // SEQ_DATA_PORT
    {0x3086, 0x8289},   // SEQ_DATA_PORT
    {0x3086, 0x26B2},   // SEQ_DATA_PORT
    {0x3086, 0x1703},   // SEQ_DATA_PORT
    {0x3086, 0x6BA4},   // SEQ_DATA_PORT
    {0x3086, 0x6099},   // SEQ_DATA_PORT
    {0x3086, 0x1729},   // SEQ_DATA_PORT
    {0x3086, 0x2683},   // SEQ_DATA_PORT
    {0x3086, 0x45A8},   // SEQ_DATA_PORT
    {0x3086, 0x1707},   // SEQ_DATA_PORT
    {0x3086, 0x27FB},   // SEQ_DATA_PORT
    {0x3086, 0x1729},   // SEQ_DATA_PORT
    {0x3086, 0x4588},   // SEQ_DATA_PORT
    {0x3086, 0x2017},   // SEQ_DATA_PORT
    {0x3086, 0x0827},   // SEQ_DATA_PORT
    {0x3086, 0xFA5D},   // SEQ_DATA_PORT
    {0x3086, 0x8710},   // SEQ_DATA_PORT
    {0x3086, 0x8C82},   // SEQ_DATA_PORT
    {0x3086, 0x8917},   // SEQ_DATA_PORT
    {0x3086, 0x0E48},   // SEQ_DATA_PORT
    {0x3086, 0x269A},   // SEQ_DATA_PORT
    {0x3086, 0x2888},   // SEQ_DATA_PORT
    {0x3086, 0x4C0B},   // SEQ_DATA_PORT
    {0x3086, 0x7917},   // SEQ_DATA_PORT
    {0x3086, 0x3026},   // SEQ_DATA_PORT
    {0x3086, 0x9217},   // SEQ_DATA_PORT
    {0x3086, 0x099A},   // SEQ_DATA_PORT
    {0x3086, 0x6027},   // SEQ_DATA_PORT
    {0x3086, 0xF217},   // SEQ_DATA_PORT
    {0x3086, 0x1026},   // SEQ_DATA_PORT
    {0x3086, 0x8226},   // SEQ_DATA_PORT
    {0x3086, 0x835F},   // SEQ_DATA_PORT
    {0x3086, 0x4D28},   // SEQ_DATA_PORT
    {0x3086, 0x081A},   // SEQ_DATA_PORT
    {0x3086, 0x27FA},   // SEQ_DATA_PORT
    {0x3086, 0x8469},   // SEQ_DATA_PORT
    {0x3086, 0xAB78},   // SEQ_DATA_PORT
    {0x3086, 0x5D28},   // SEQ_DATA_PORT
    {0x3086, 0x8887},   // SEQ_DATA_PORT
    {0x3086, 0x108C},   // SEQ_DATA_PORT
    {0x3086, 0x808A},   // SEQ_DATA_PORT
    {0x3086, 0x2692},   // SEQ_DATA_PORT
    {0x3086, 0x1703},   // SEQ_DATA_PORT
    {0x3086, 0x6BA6},   // SEQ_DATA_PORT
    {0x3086, 0xA726},   // SEQ_DATA_PORT
    {0x3086, 0x035C},   // SEQ_DATA_PORT
    {0x3086, 0x0145},   // SEQ_DATA_PORT
    {0x3086, 0x588E},   // SEQ_DATA_PORT
    {0x3086, 0x0027},   // SEQ_DATA_PORT
    {0x3086, 0x9817},   // SEQ_DATA_PORT
    {0x3086, 0x0A4A},   // SEQ_DATA_PORT
    {0x3086, 0x0A43},   // SEQ_DATA_PORT
    {0x3086, 0x160B},   // SEQ_DATA_PORT
    {0x3086, 0x438E},   // SEQ_DATA_PORT
    {0x3086, 0x0327},   // SEQ_DATA_PORT
    {0x3086, 0x9C45},   // SEQ_DATA_PORT
    {0x3086, 0x7817},   // SEQ_DATA_PORT
    {0x3086, 0x0727},   // SEQ_DATA_PORT
    {0x3086, 0x9D17},   // SEQ_DATA_PORT
    {0x3086, 0x225D},   // SEQ_DATA_PORT
    {0x3086, 0x8710},   // SEQ_DATA_PORT
    {0x3086, 0x2808},   // SEQ_DATA_PORT
    {0x3086, 0x530D},   // SEQ_DATA_PORT
    {0x3086, 0x8C80},   // SEQ_DATA_PORT
    {0x3086, 0x8A45},   // SEQ_DATA_PORT
    {0x3086, 0x5817},   // SEQ_DATA_PORT
    {0x3086, 0x088E},   // SEQ_DATA_PORT
    {0x3086, 0x0127},   // SEQ_DATA_PORT
    {0x3086, 0x988E},   // SEQ_DATA_PORT
    {0x3086, 0x0076},   // SEQ_DATA_PORT
    {0x3086, 0xAC77},   // SEQ_DATA_PORT
    {0x3086, 0xAC46},   // SEQ_DATA_PORT
    {0x3086, 0x4416},   // SEQ_DATA_PORT
    {0x3086, 0x16A8},   // SEQ_DATA_PORT
    {0x3086, 0x7A26},   // SEQ_DATA_PORT
    {0x3086, 0x445C},   // SEQ_DATA_PORT
    {0x3086, 0x0512},   // SEQ_DATA_PORT
    {0x3086, 0x444B},   // SEQ_DATA_PORT
    {0x3086, 0x7175},   // SEQ_DATA_PORT
    {0x3086, 0xA24A},   // SEQ_DATA_PORT
    {0x3086, 0x0343},   // SEQ_DATA_PORT
    {0x3086, 0x1604},   // SEQ_DATA_PORT
    {0x3086, 0x4316},   // SEQ_DATA_PORT
    {0x3086, 0x5843},   // SEQ_DATA_PORT
    {0x3086, 0x165A},   // SEQ_DATA_PORT
    {0x3086, 0x4316},   // SEQ_DATA_PORT
    {0x3086, 0x0643},   // SEQ_DATA_PORT
    {0x3086, 0x1607},   // SEQ_DATA_PORT
    {0x3086, 0x4316},   // SEQ_DATA_PORT
    {0x3086, 0x8E03},   // SEQ_DATA_PORT
    {0x3086, 0x279C},   // SEQ_DATA_PORT
    {0x3086, 0x4578},   // SEQ_DATA_PORT
    {0x3086, 0x7B17},   // SEQ_DATA_PORT
    {0x3086, 0x078B},   // SEQ_DATA_PORT
    {0x3086, 0x8627},   // SEQ_DATA_PORT
    {0x3086, 0x9D17},   // SEQ_DATA_PORT
    {0x3086, 0x2345},   // SEQ_DATA_PORT
    {0x3086, 0x5822},   // SEQ_DATA_PORT
    {0x3086, 0x1708},   // SEQ_DATA_PORT
    {0x3086, 0x8E01},   // SEQ_DATA_PORT
    {0x3086, 0x2798},   // SEQ_DATA_PORT
    {0x3086, 0x8E00},   // SEQ_DATA_PORT
    {0x3086, 0x2644},   // SEQ_DATA_PORT
    {0x3086, 0x5C05},   // SEQ_DATA_PORT
    {0x3086, 0x1244},   // SEQ_DATA_PORT
    {0x3086, 0x4B8D},   // SEQ_DATA_PORT
    {0x3086, 0x602C},   // SEQ_DATA_PORT
    {0x3086, 0x2C2C},   // SEQ_DATA_PORT
    {0x3086, 0x2C00},   // SEQ_DATA_PORT

    // [PLL_configuration_Parallel]
    {0x302A, 0x0006},   // VT_PIX_CLK_DIV
    {0x302C, 0x0001},   // VT_SYS_CLK_DIV
    {0x302E, 0x0004},   // PRE_PLL_CLK_DIV
    {0x3030, 0x0042},   // PLL_MULTIPLIER
    {0x3036, 0x000C},   // OP_PIX_CLK_DIV
    {0x3038, 0x0001},   // OP_SYS_CLK_DIV

    // [720P 54fps_configuration]
    {0x3004, 0x0016},   // X_ADDR_ST
    {0x3002, 0x001c},   // Y_ADDR_ST
    {0x3008, 0x0515},   // X_ADDR_EN
    {0x3006, 0x033b},   // Y_ADDR_EN
    {0x300A, 0x0336},   // FRAME_LEN    //54.7 fps
    {0x300C, 0x0672},   // LINE_LENG
    {0x3012, 0x002D},   // COARSE_IN
    {0x30A2, 0x0001},   // X_ODD_INC
    {0x30A6, 0x0001},   // Y_ODD_INC
    {0x3040, 0x0000},   // READ_MODE

    //[AR0140 Rev2 Optimized Settings] 20141224
    {0x3044, 0x0400}, //Manufacturer-specific
    {0x3052, 0xA134}, //Manufacturer-specific
    {0x3092, 0x010F}, //Manufacturer-specific
    {0x30FE, 0x0080}, //Manufacturer-specific
    {0x3ECE, 0x40FF}, //Manufacturer-specific
    {0x3ED0, 0xFF40}, //Manufacturer-specific
    {0x3ED2, 0xA906}, //Manufacturer-specific
    {0x3ED4, 0x001F}, //Manufacturer-specific
    {0x3ED6, 0x638F}, //Manufacturer-specific
    {0x3ED8, 0xCC99}, //Manufacturer-specific
    {0x3EDA, 0x0888}, //Manufacturer-specific
    {0x3EDE, 0x8878}, //Manufacturer-specific
    {0x3EE0, 0x7744}, //Manufacturer-specific
    {0x3EE2, 0x4463}, //Manufacturer-specific
    {0x3EE4, 0xAAE0}, //Manufacturer-specific
    {0x3EE6, 0x1400}, //Manufacturer-specific
    {0x3EEA, 0xA4FF}, //Manufacturer-specific
    {0x3EEC, 0x80F0}, //Manufacturer-specific
    {0x3EEE, 0x0000}, //Manufacturer-specific
    {0x31E0, 0x1701}, //Manufacturer-specific

    // [Companding_enabled_16to12]
    {0x31AC, 0x100C},   // DATA_FORMA
    {0x31D0, 0x0001},   // COMPANDING

    // [HDR Mode 16x Setup]
    // [Linear Mode Full Resolution]
    {0x301A, 0x10D8},    // RESET_REGISTER
    {DELAY_CODE, 200},
    {0x3082, 0x0029},    // OPERATION_MODE_CTRL

    // [Motion Compensation On]
    {0x318C, 0xC000},   // HDR_MC_CTRL2

    // [ADACD Enabled]
    {0x320A, 0x0080},   // ADACD_PEDESTA
    {0x3206, 0x0A06},   // ADACD_NOISE_F
    {0x3206, 0x0A06},   // ADACD_NOISE_F
    {0x3208, 0x1A12},   // ADACD_NOISE_F
    {0x3208, 0x1A12},   // ADACD_NOISE_F
    {0x3202, 0x00A0},   // ADACD_NOISE_M
    {0x3200, 0x0002},   // ADACD_CONTROL


    // [Companding_enabled_16to12]
    {0x31AC, 0x100C},   // DATA_FORMA
    {0x31D0, 0x0001},   // COMPANDING
    // [HDR Mode 16x Setup]
    {0x318A, 0x0E74},   // HDR_MC_CTRL1
    {0x3192, 0x0400},   // HDR_MC_CTRL5
    {0x3198, 0x183C},   // HDR_MC_CTRL8
    {0x318E, 0x0800},   // HDR_MC_CTRL3
    {0x3194, 0x0BB8},   // HDR_MC_CTRL6
    {0x3196, 0x0E74},   // HDR_MC_CTRL7

    //[Disable Embedded Data and Stats]
    {0x3064, 0x1802},   // SMIA_TEST

    // [HDR (Parallel) 720p30 - maximize frame length]
    {0x300A, 0x05D4},   // FRAME_LENGTH_LINES
    {0x300C, 0x0672},   // LINE_LENGTH_PCK
#ifdef FPGA
    {0x30B0, 0x5300},   // DIGITAL_TEST  bypass
#endif
    {0x30BA, 0x012C},   // DIGITAL_CTRL
    {0x31AE, 0x0301},   // SERIAL_FORMAT
    {0x301A, 0x10DC},   // RESET_REGISTER
    {0x301A, 0x10DC},   // RESET_REGISTER
    {DELAY_CODE, 200},

    // [Auto Exposure Trigger]
    {0x3064, 0x1802},
    {DELAY_CODE, 200},
    {0x3064, 0x1802},

    {0x3056, 0x00B0},   // GREEN1_GAIN
    {0x305C, 0x00B0},   // GREEN2_GAIN
    {0x305A, 0x00B0},   // RED_GAIN
    {0x3058, 0x00B0},   // BLUE_GAIN
    {0x3012, 0x0038},   // COARSE_INTEGRATION_TIME

};
#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))

static sensor_reg_t sensor_init_regs_ar0141[] = {
    //[Linear (Parallel) 1280x800 54fps]
    {DELAY_CODE, 200},
    {0x301A, 0x0001}, 	// RESET_REGISTER
    {DELAY_CODE, 200},
    {0x301A, 0x10D8}, 	// RESET_REGISTER
    {DELAY_CODE, 200},
    
    // [sequencer_ers_0828_AR0141.i]
    {0x3088, 0x8000}, 	// SEQ_CTRL_PORT
    {0x3086, 0x4558}, 	// SEQ_DATA_PORT
    {0x3086, 0x6E9B}, 	// SEQ_DATA_PORT
    {0x3086, 0x4A31}, 	// SEQ_DATA_PORT
    {0x3086, 0x4342}, 	// SEQ_DATA_PORT
    {0x3086, 0x8E03}, 	// SEQ_DATA_PORT
    {0x3086, 0x2714}, 	// SEQ_DATA_PORT
    {0x3086, 0x4578}, 	// SEQ_DATA_PORT
    {0x3086, 0x7B3D}, 	// SEQ_DATA_PORT
    {0x3086, 0xFF3D}, 	// SEQ_DATA_PORT
    {0x3086, 0xFF3D}, 	// SEQ_DATA_PORT
    {0x3086, 0xEA27}, 	// SEQ_DATA_PORT
    {0x3086, 0x043D}, 	// SEQ_DATA_PORT
    {0x3086, 0x1027}, 	// SEQ_DATA_PORT
    {0x3086, 0x0527}, 	// SEQ_DATA_PORT
    {0x3086, 0x1535}, 	// SEQ_DATA_PORT
    {0x3086, 0x2705}, 	// SEQ_DATA_PORT
    {0x3086, 0x3D10}, 	// SEQ_DATA_PORT
    {0x3086, 0x4558}, 	// SEQ_DATA_PORT
    {0x3086, 0x2704}, 	// SEQ_DATA_PORT
    {0x3086, 0x2714}, 	// SEQ_DATA_PORT
    {0x3086, 0x3DFF}, 	// SEQ_DATA_PORT
    {0x3086, 0x3DFF}, 	// SEQ_DATA_PORT
    {0x3086, 0x3DEA}, 	// SEQ_DATA_PORT
    {0x3086, 0x2704}, 	// SEQ_DATA_PORT
    {0x3086, 0x6227}, 	// SEQ_DATA_PORT
    {0x3086, 0x288E}, 	// SEQ_DATA_PORT
    {0x3086, 0x0036}, 	// SEQ_DATA_PORT
    {0x3086, 0x2708}, 	// SEQ_DATA_PORT
    {0x3086, 0x3D64}, 	// SEQ_DATA_PORT
    {0x3086, 0x7A3D}, 	// SEQ_DATA_PORT
    {0x3086, 0x0444}, 	// SEQ_DATA_PORT
    {0x3086, 0x2C4B}, 	// SEQ_DATA_PORT
    {0x3086, 0x8F01}, 	// SEQ_DATA_PORT
    {0x3086, 0x4372}, 	// SEQ_DATA_PORT
    {0x3086, 0x719F}, 	// SEQ_DATA_PORT
    {0x3086, 0x4643}, 	// SEQ_DATA_PORT
    {0x3086, 0x166F}, 	// SEQ_DATA_PORT
    {0x3086, 0x9F92}, 	// SEQ_DATA_PORT
    {0x3086, 0x1244}, 	// SEQ_DATA_PORT
    {0x3086, 0x1646}, 	// SEQ_DATA_PORT
    {0x3086, 0x4316}, 	// SEQ_DATA_PORT
    {0x3086, 0x9326}, 	// SEQ_DATA_PORT
    {0x3086, 0x0426}, 	// SEQ_DATA_PORT
    {0x3086, 0x848E}, 	// SEQ_DATA_PORT
    {0x3086, 0x0327}, 	// SEQ_DATA_PORT
    {0x3086, 0xFC5C}, 	// SEQ_DATA_PORT
    {0x3086, 0x0D57}, 	// SEQ_DATA_PORT
    {0x3086, 0x5417}, 	// SEQ_DATA_PORT
    {0x3086, 0x0955}, 	// SEQ_DATA_PORT
    {0x3086, 0x5649}, 	// SEQ_DATA_PORT
    {0x3086, 0x5F53}, 	// SEQ_DATA_PORT
    {0x3086, 0x0553}, 	// SEQ_DATA_PORT
    {0x3086, 0x0728}, 	// SEQ_DATA_PORT
    {0x3086, 0x6C4C}, 	// SEQ_DATA_PORT
    {0x3086, 0x0928}, 	// SEQ_DATA_PORT
    {0x3086, 0x2C72}, 	// SEQ_DATA_PORT
    {0x3086, 0xA37C}, 	// SEQ_DATA_PORT
    {0x3086, 0x9728}, 	// SEQ_DATA_PORT
    {0x3086, 0xA879}, 	// SEQ_DATA_PORT
    {0x3086, 0x6026}, 	// SEQ_DATA_PORT
    {0x3086, 0x9C5C}, 	// SEQ_DATA_PORT
    {0x3086, 0x1B45}, 	// SEQ_DATA_PORT
    {0x3086, 0x4845}, 	// SEQ_DATA_PORT
    {0x3086, 0x0845}, 	// SEQ_DATA_PORT
    {0x3086, 0x8826}, 	// SEQ_DATA_PORT
    {0x3086, 0xBE8E}, 	// SEQ_DATA_PORT
    {0x3086, 0x0127}, 	// SEQ_DATA_PORT
    {0x3086, 0xF817}, 	// SEQ_DATA_PORT
    {0x3086, 0x0227}, 	// SEQ_DATA_PORT
    {0x3086, 0xFA17}, 	// SEQ_DATA_PORT
    {0x3086, 0x095C}, 	// SEQ_DATA_PORT
    {0x3086, 0x0B17}, 	// SEQ_DATA_PORT
    {0x3086, 0x1026}, 	// SEQ_DATA_PORT
    {0x3086, 0xBA5C}, 	// SEQ_DATA_PORT
    {0x3086, 0x0317}, 	// SEQ_DATA_PORT
    {0x3086, 0x1026}, 	// SEQ_DATA_PORT
    {0x3086, 0xB217}, 	// SEQ_DATA_PORT
    {0x3086, 0x065F}, 	// SEQ_DATA_PORT
    {0x3086, 0x2888}, 	// SEQ_DATA_PORT
    {0x3086, 0x9060}, 	// SEQ_DATA_PORT
    {0x3086, 0x27F2}, 	// SEQ_DATA_PORT
    {0x3086, 0x1710}, 	// SEQ_DATA_PORT
    {0x3086, 0x26A2}, 	// SEQ_DATA_PORT
    {0x3086, 0x26A3}, 	// SEQ_DATA_PORT
    {0x3086, 0x5F4D}, 	// SEQ_DATA_PORT
    {0x3086, 0x2808}, 	// SEQ_DATA_PORT
    {0x3086, 0x1A27}, 	// SEQ_DATA_PORT
    {0x3086, 0xFA84}, 	// SEQ_DATA_PORT
    {0x3086, 0x69A0}, 	// SEQ_DATA_PORT
    {0x3086, 0x785D}, 	// SEQ_DATA_PORT
    {0x3086, 0x2888}, 	// SEQ_DATA_PORT
    {0x3086, 0x8710}, 	// SEQ_DATA_PORT
    {0x3086, 0x8C82}, 	// SEQ_DATA_PORT
    {0x3086, 0x8926}, 	// SEQ_DATA_PORT
    {0x3086, 0xB217}, 	// SEQ_DATA_PORT
    {0x3086, 0x036B}, 	// SEQ_DATA_PORT
    {0x3086, 0x9C60}, 	// SEQ_DATA_PORT
    {0x3086, 0x9417}, 	// SEQ_DATA_PORT
    {0x3086, 0x2926}, 	// SEQ_DATA_PORT
    {0x3086, 0x8345}, 	// SEQ_DATA_PORT
    {0x3086, 0xA817}, 	// SEQ_DATA_PORT
    {0x3086, 0x0727}, 	// SEQ_DATA_PORT
    {0x3086, 0xFB17}, 	// SEQ_DATA_PORT
    {0x3086, 0x2945}, 	// SEQ_DATA_PORT
    {0x3086, 0x8820}, 	// SEQ_DATA_PORT
    {0x3086, 0x1708}, 	// SEQ_DATA_PORT
    {0x3086, 0x27FA}, 	// SEQ_DATA_PORT
    {0x3086, 0x5D87}, 	// SEQ_DATA_PORT
    {0x3086, 0x108C}, 	// SEQ_DATA_PORT
    {0x3086, 0x8289}, 	// SEQ_DATA_PORT
    {0x3086, 0x170E}, 	// SEQ_DATA_PORT
    {0x3086, 0x4826}, 	// SEQ_DATA_PORT
    {0x3086, 0x9A28}, 	// SEQ_DATA_PORT
    {0x3086, 0x884C}, 	// SEQ_DATA_PORT
    {0x3086, 0x0B79}, 	// SEQ_DATA_PORT
    {0x3086, 0x1730}, 	// SEQ_DATA_PORT
    {0x3086, 0x2692}, 	// SEQ_DATA_PORT
    {0x3086, 0x1709}, 	// SEQ_DATA_PORT
    {0x3086, 0x9160}, 	// SEQ_DATA_PORT
    {0x3086, 0x27F2}, 	// SEQ_DATA_PORT
    {0x3086, 0x1710}, 	// SEQ_DATA_PORT
    {0x3086, 0x2682}, 	// SEQ_DATA_PORT
    {0x3086, 0x2683}, 	// SEQ_DATA_PORT
    {0x3086, 0x5F4D}, 	// SEQ_DATA_PORT
    {0x3086, 0x2808}, 	// SEQ_DATA_PORT
    {0x3086, 0x1A27}, 	// SEQ_DATA_PORT
    {0x3086, 0xFA84}, 	// SEQ_DATA_PORT
    {0x3086, 0x69A1}, 	// SEQ_DATA_PORT
    {0x3086, 0x785D}, 	// SEQ_DATA_PORT
    {0x3086, 0x2888}, 	// SEQ_DATA_PORT
    {0x3086, 0x8710}, 	// SEQ_DATA_PORT
    {0x3086, 0x8C80}, 	// SEQ_DATA_PORT
    {0x3086, 0x8A26}, 	// SEQ_DATA_PORT
    {0x3086, 0x9217}, 	// SEQ_DATA_PORT
    {0x3086, 0x036B}, 	// SEQ_DATA_PORT
    {0x3086, 0x9D95}, 	// SEQ_DATA_PORT
    {0x3086, 0x2603}, 	// SEQ_DATA_PORT
    {0x3086, 0x5C01}, 	// SEQ_DATA_PORT
    {0x3086, 0x4558}, 	// SEQ_DATA_PORT
    {0x3086, 0x8E00}, 	// SEQ_DATA_PORT
    {0x3086, 0x2798}, 	// SEQ_DATA_PORT
    {0x3086, 0x170A}, 	// SEQ_DATA_PORT
    {0x3086, 0x4A0A}, 	// SEQ_DATA_PORT
    {0x3086, 0x4316}, 	// SEQ_DATA_PORT
    {0x3086, 0x0B43}, 	// SEQ_DATA_PORT
    {0x3086, 0x5B43}, 	// SEQ_DATA_PORT
    {0x3086, 0x1659}, 	// SEQ_DATA_PORT
    {0x3086, 0x4316}, 	// SEQ_DATA_PORT
    {0x3086, 0x8E03}, 	// SEQ_DATA_PORT
    {0x3086, 0x279C}, 	// SEQ_DATA_PORT
    {0x3086, 0x4578}, 	// SEQ_DATA_PORT
    {0x3086, 0x1707}, 	// SEQ_DATA_PORT
    {0x3086, 0x279D}, 	// SEQ_DATA_PORT
    {0x3086, 0x1722}, 	// SEQ_DATA_PORT
    {0x3086, 0x5D87}, 	// SEQ_DATA_PORT
    {0x3086, 0x1028}, 	// SEQ_DATA_PORT
    {0x3086, 0x0853}, 	// SEQ_DATA_PORT
    {0x3086, 0x0D8C}, 	// SEQ_DATA_PORT
    {0x3086, 0x808A}, 	// SEQ_DATA_PORT
    {0x3086, 0x4558}, 	// SEQ_DATA_PORT
    {0x3086, 0x1708}, 	// SEQ_DATA_PORT
    {0x3086, 0x8E01}, 	// SEQ_DATA_PORT
    {0x3086, 0x2798}, 	// SEQ_DATA_PORT
    {0x3086, 0x8E00}, 	// SEQ_DATA_PORT
    {0x3086, 0x76A2}, 	// SEQ_DATA_PORT
    {0x3086, 0x77A2}, 	// SEQ_DATA_PORT
    {0x3086, 0x4644}, 	// SEQ_DATA_PORT
    {0x3086, 0x1616}, 	// SEQ_DATA_PORT
    {0x3086, 0x967A}, 	// SEQ_DATA_PORT
    {0x3086, 0x2644}, 	// SEQ_DATA_PORT
    {0x3086, 0x5C05}, 	// SEQ_DATA_PORT
    {0x3086, 0x1244}, 	// SEQ_DATA_PORT
    {0x3086, 0x4B71}, 	// SEQ_DATA_PORT
    {0x3086, 0x759E}, 	// SEQ_DATA_PORT
    {0x3086, 0x8B86}, 	// SEQ_DATA_PORT
    {0x3086, 0x184A}, 	// SEQ_DATA_PORT
    {0x3086, 0x0343}, 	// SEQ_DATA_PORT
    {0x3086, 0x1606}, 	// SEQ_DATA_PORT
    {0x3086, 0x4316}, 	// SEQ_DATA_PORT
    {0x3086, 0x0743}, 	// SEQ_DATA_PORT
    {0x3086, 0x1604}, 	// SEQ_DATA_PORT
    {0x3086, 0x4316}, 	// SEQ_DATA_PORT
    {0x3086, 0x5843}, 	// SEQ_DATA_PORT
    {0x3086, 0x165A}, 	// SEQ_DATA_PORT
    {0x3086, 0x4316}, 	// SEQ_DATA_PORT
    {0x3086, 0x4558}, 	// SEQ_DATA_PORT
    {0x3086, 0x8E03}, 	// SEQ_DATA_PORT
    {0x3086, 0x279C}, 	// SEQ_DATA_PORT
    {0x3086, 0x4578}, 	// SEQ_DATA_PORT
    {0x3086, 0x7B17}, 	// SEQ_DATA_PORT
    {0x3086, 0x0727}, 	// SEQ_DATA_PORT
    {0x3086, 0x9D17}, 	// SEQ_DATA_PORT
    {0x3086, 0x2245}, 	// SEQ_DATA_PORT
    {0x3086, 0x5822}, 	// SEQ_DATA_PORT
    {0x3086, 0x1710}, 	// SEQ_DATA_PORT
    {0x3086, 0x8E01}, 	// SEQ_DATA_PORT
    {0x3086, 0x2798}, 	// SEQ_DATA_PORT
    {0x3086, 0x8E00}, 	// SEQ_DATA_PORT
    {0x3086, 0x1710}, 	// SEQ_DATA_PORT
    {0x3086, 0x1244}, 	// SEQ_DATA_PORT
    {0x3086, 0x4B8D}, 	// SEQ_DATA_PORT
    {0x3086, 0x602C}, 	// SEQ_DATA_PORT
    {0x3086, 0x2C2C}, 	// SEQ_DATA_PORT
    {0x3086, 0x2C00}, 	// SEQ_DATA_PORT
    
    // [Linear (Parallel) 1280x800 54fps]
    {0x302A, 0x0006}, 	// VT_PIX_CLK_DIV
    {0x302C, 0x0001}, 	// VT_SYS_CLK_DIV
    {0x302E, 0x0004}, 	// PRE_PLL_CLK_DIV
    {0x3030, 0x0042}, 	// PLL_MULTIPLIER
    {0x3036, 0x000C}, 	// OP_PIX_CLK_DIV
    {0x3038, 0x0001}, 	// OP_SYS_CLK_DIV

    // [1280x800_configuration]
    {0x3002, 0x001C}, 	// Y_ADDR_START
    {0x3004, 0x0016}, 	// X_ADDR_START
    {0x3006, 0x033B}, 	// Y_ADDR_END
    {0x3008, 0x0515}, 	// X_ADDR_END
    {0x300A, 0x0336}, 	// FRAME_LENGTH_LINES
    {0x300C, 0x0672}, 	// LINE_LENGTH_PCK

    {0x305E, 0x0080}, 	// GLOBAL_GAIN

    // [AR0141 Rev1 Optimized settings]
    {0x3044, 0x0400}, 	//DARK_CONTROL
    {0x3052, 0xA124}, 	//OTPM_CFG
    {0x3092, 0x010F}, 	//ROW_NOISE_CONTROL
    {0x30FE, 0x0080}, 	//NOISE_PEDESTAL
    {0x3ECE, 0x40FF}, 	//DAC_LD_2_3
    {0x3ED0, 0xFF40}, 	//DAC_LD_4_5
    {0x3ED2, 0xA906}, 	//DAC_LD_6_7
    {0x3ED4, 0x001F}, 	//DAC_LD_8_9
    {0x3ED6, 0x638F}, 	//DAC_LD_10_11
    {0x3ED8, 0xCC99}, 	//DAC_LD_12_13
    {0x3EDA, 0x0888}, 	//DAC_LD_14_15
    {0x3EDE, 0x8878}, 	//DAC_LD_18_19
    {0x3EE0, 0x7744}, 	//DAC_LD_20_21
    {0x3EE2, 0x4463}, 	//DAC_LD_22_23
    {0x3EE4, 0xAAE0}, 	//DAC_LD_24_25
    /** 0x3EE6 work around "black band"artifact, but become foggy **/
    {0x3EE6, 0x1400}, 	//DAC_LD_26_27
    {0x3EEA, 0xA4FF}, 	//DAC_LD_30_31
    {0x3EEC, 0x80F0}, 	//DAC_LD_32_33
    {0x3EEE, 0x0000}, 	//DAC_LD_34_35
    {0x31E0, 0x1701}, 	//PIX_DEF_ID
    {0x304C, 0x2000}, 	//OTPM_RECORD
    {0x304A, 0x0210}, 	//OTPM_CONTROL
    {DELAY_CODE, 200},

    {0x3064, 0x1802},   // SMIA_TEST

    // [Linear Mode Setup]
    {0x3082, 0x0001}, 	//OPERATION_MODE_CTRL
    
    // [ADACD Disabled]
    {0x3200, 0x0000}, 	// ADACD_CONTROL
    
    // [Linear Mode Setup]
    {0x31D0, 0x0000}, 	// COMPANDING
    
    // [Linear (Parallel) 1280x800 54fps]
    {0x30B0, 0x0000}, 	// DIGITAL_TEST
    {0x30BA, 0x012C}, 	// DIGITAL_CTRL
    {0x31AC, 0x0C0C}, 	// DATA_FORMAT_BITS
    {0x31AE, 0x0301}, 	// SERIAL_FORMAT
    {0x301A, 0x10DC}, 	// RESET_REGISTER
    {DELAY_CODE, 33},
};
#define NUM_OF_INIT_REGS_AR0141 (sizeof(sensor_init_regs_ar0141) / sizeof(sensor_reg_t))

typedef struct gain_set {
    u8  coarse_gain;
    u8  fine_gain;
    u16 dig_gain;
    u8  dcg_gain;
    u16 gain_x; // UFIX 7.6
} gain_set_t;

static gain_set_t gain_table[] =
{
    {0,  0, 139, 0,   69},{0,  1, 139, 0,   73},{0,  2, 139, 0,   78},{0,  3, 139, 0,   82},
    {0,  4, 139, 0,   86},{0,  5, 139, 0,   91},{0,  6, 139, 0,   95},{0,  7, 139, 0,   99},
    {0,  8, 139, 0,  104},{0,  9, 139, 0,  108},{0, 10, 139, 0,  112},{0, 11, 139, 0,  117},
    {0, 12, 139, 0,  121},{0, 13, 139, 0,  125},{0, 14, 139, 0,  130},{0, 15, 139, 0,  134},
    {0, 15, 145, 0,  140},{0, 15, 155, 0,  150},{1,  0, 160, 0,  160},{1,  1, 160, 0,  170},
    {1,  2, 160, 0,  180},{1,  3, 160, 0,  190},{1,  4, 160, 0,  200},{1,  5, 160, 0,  210},
    {1,  6, 160, 0,  220},{1,  7, 160, 0,  230},{1,  8, 160, 0,  240},{1,  9, 160, 0,  250},
    {1, 10, 160, 0,  260},{1, 11, 160, 0,  270},{1, 12, 160, 0,  280},{1, 13, 160, 0,  290},
    {1, 14, 160, 0,  300},{1, 15, 160, 0,  310},{2,  0, 160, 0,  320},{2,  1, 160, 0,  340},
    {2,  2, 160, 0,  360},{2,  3, 160, 0,  380},{2,  4, 160, 0,  400},{2,  5, 160, 0,  420},
    {2,  6, 160, 0,  440},{2,  7, 160, 0,  460},{2,  8, 160, 0,  480},{2,  9, 160, 0,  500},
    {2,  9, 164, 0,  512},{1,  8, 140, 1,  525},{1,  9, 140, 1,  546},{1, 10, 140, 1,  568},
    {1, 11, 140, 1,  590},{1, 12, 140, 1,  612},{1, 13, 140, 1,  634},{1, 14, 140, 1,  656},
    {1, 15, 140, 1,  678},{1, 15, 146, 1,  707},{1, 15, 152, 1,  736},{2,  0, 154, 1,  770},
    {2,  1, 154, 1,  818},{2,  2, 154, 1,  866},{2,  3, 154, 1,  914},{2,  4, 154, 1,  962},
    {2,  5, 154, 1, 1010},{2,  6, 154, 1, 1058},{2,  7, 154, 1, 1106},{2,  8, 154, 1, 1155},
    {2,  9, 154, 1, 1203},{2,  9, 160, 1, 1250},{2,  9, 170, 1, 1328},{2,  9, 180, 1, 1406},
    {2,  9, 190, 1, 1484},{2,  9, 200, 1, 1562},{2,  9, 210, 1, 1640},{2,  9, 220, 1, 1718},
    {2,  9, 230, 1, 1796},{2,  9, 240, 1, 1875},{2,  9, 250, 1, 1953},{2,  9, 260, 1, 2031},
    {2,  9, 270, 1, 2109},{2,  9, 280, 1, 2187},{2,  9, 290, 1, 2265},{2,  9, 300, 1, 2343},
    {2,  9, 310, 1, 2421},{2,  9, 320, 1, 2500},{2,  9, 330, 1, 2578},{2,  9, 340, 1, 2656},
    {2,  9, 350, 1, 2734},{2,  9, 360, 1, 2812},{2,  9, 370, 1, 2890},{2,  9, 380, 1, 2968},
    {2,  9, 390, 1, 3046},{2,  9, 400, 1, 3125},{2,  9, 410, 1, 3203},{2,  9, 420, 1, 3281},
    {2,  9, 430, 1, 3359},{2,  9, 440, 1, 3437},{2,  9, 450, 1, 3515},{2,  9, 460, 1, 3593},
    {2,  9, 470, 1, 3671},{2,  9, 480, 1, 3750},
};
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

//=============================================================================
// I2C
//=============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            (0x20 >> 1)
#include "i2c_common.c"

//=============================================================================
// internal functions
//=============================================================================
static int read_reg(u32 addr, u32 *pval)
{
    struct i2c_msg  msgs[2];
    unsigned char   tmp[2], tmp2[2];

    tmp[0]        = (addr >> 8) & 0xFF;
    tmp[1]        = addr & 0xFF;
    msgs[0].addr  = I2C_ADDR;
    msgs[0].flags = 0;
    msgs[0].len   = 2;
    msgs[0].buf   = tmp;

    tmp2[0]       = 0;
    msgs[1].addr  = I2C_ADDR;
    msgs[1].flags = 1;
    msgs[1].len   = 2;
    msgs[1].buf   = tmp2;

    if (sensor_i2c_transfer(msgs, 2))
        return -1;

    *pval = (tmp2[0] << 8) | tmp2[1];
    return 0;
}

static int write_reg(u32 addr, u32 val)
{
    struct i2c_msg  msgs;
    unsigned char   buf[4];

    buf[0]     = (addr >> 8) & 0xFF;
    buf[1]     = addr & 0xFF;
    buf[2]     = (val >> 8) & 0xFF;
    buf[3]     = val & 0xFF;
    msgs.addr  = I2C_ADDR;
    msgs.flags = 0;
    msgs.len   = 4;
    msgs.buf   = buf;

    return sensor_i2c_transfer(&msgs, 1);
}

static u32 get_pclk(u32 xclk)
{
    u32 vt_pix_clk_div, vt_sys_clk_div, pre_pll_clk_div, pll_multiplier, digital_test;
    u32 pclk;

    read_reg(0x302a, &vt_pix_clk_div);
    read_reg(0x302c, &vt_sys_clk_div);
    read_reg(0x302e, &pre_pll_clk_div);
    read_reg(0x3030, &pll_multiplier);
    read_reg(0x30B0, &digital_test);     //digital_test[14] , bypass all

    if(digital_test & BIT14){
        pclk = xclk;
    }else{
        pclk = xclk * pll_multiplier / vt_pix_clk_div / vt_sys_clk_div / pre_pll_clk_div;
    }

    printk("pclk(%d) XCLK(%d)\n", pclk, xclk);
    return pclk;
}

static void adjust_vblk(int fps)
{
    int Vblk; //V blanking lines, H blanking
    int img_w, img_h;
    u32 x_start, x_end, y_start, y_end;
    int line_length_pck, frame_h;;

    if (fps > 54)
        fps = 54;
    read_reg(0x3008, &x_end);
    read_reg(0x3004, &x_start);
    read_reg(0x3006, &y_end);
    read_reg(0x3002, &y_start);
    read_reg(0x300C, &line_length_pck);

    img_w = x_end - x_start + 1;
    img_h = y_end - y_start + 1;

    Vblk = ((g_psensor->pclk / fps)/ line_length_pck - img_h);

    if(Vblk < 16)  //min. V blanking
        Vblk = 16;

    frame_h = Vblk+img_h;

    write_reg(0x300A, (u32)frame_h);
    g_psensor->fps = fps;
}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 line_length_pck;

    g_psensor->pclk = get_pclk(g_psensor->xclk);

    read_reg(0X300C, &line_length_pck);
    // unit: 1/10us; 1/10 us =x, x= 10*us
    pinfo->t_row = line_length_pck * 10000 / (g_psensor->pclk / 1000);
}

static int set_size(u32 width, u32 height)
{
    int ret = 0;
    u32 x_start=0, x_end=0, y_start=0, y_end=0;
    int sen_width, sen_height;
    u32 RegReadMode;
    // check size
    if ((width * (1 + binning) > SENSOR_MAX_WIDTH) || (height * (1 + binning) > SENSOR_MAX_HEIGHT)) {
        isp_err("Exceed maximum sensor resolution!\n");
        return -1;
    }

    if(binning){
        sen_width = width * 2;
        sen_height = height * 2;
        x_start = (0 + (SENSOR_MAX_WIDTH - sen_width)/2) & ~BIT0;
        y_start = (2 + (SENSOR_MAX_HEIGHT - sen_height)/2) & ~BIT0;
        x_end = (sen_width - 1 + x_start + 2) | BIT0;
        y_end = (sen_height - 1 + y_start + 2) | BIT0;
        //Set skip
        write_reg(0X30A2, 0x3);
        write_reg(0X30A6, 0x3);
        //Set binning average
        read_reg(0x3040, &RegReadMode);
        RegReadMode |= (BIT12 | BIT13);
        write_reg(0X3040, RegReadMode);
    }else{
        x_start = ((SENSOR_MAX_WIDTH - width)/2) & ~BIT0;
        y_start = ((SENSOR_MAX_HEIGHT - height)/2) & ~BIT0;
        x_end = (width - 1 + x_start);
        y_end = (height- 1 + y_start);
    }
    write_reg(0X3006, y_end);
    write_reg(0x3008, x_end);
    write_reg(0X3002, y_start);
    write_reg(0X3004, x_start);

    calculate_t_row();

    // update sensor information
    g_psensor->out_win.x = x_start;
    g_psensor->out_win.y = y_start;

    g_psensor->out_win.width = (x_end - x_start + 1) / (1 + binning);
    g_psensor->out_win.height = (y_end - y_start + 1) / (1 + binning);

    g_psensor->img_win.x = 0;
    g_psensor->img_win.y = 0;
    g_psensor->img_win.width = width ;
    g_psensor->img_win.height = height;

    return ret;
}

static u32 get_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 coarse_integration_time;

    if (!g_psensor->curr_exp) {
        read_reg(0x3012, &coarse_integration_time);
        g_psensor->curr_exp = coarse_integration_time * pinfo->t_row / 1000;
    }

    return g_psensor->curr_exp;

}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 coarse_integration_time;

    coarse_integration_time = MAX(1, exp * 1000 / pinfo->t_row);

    write_reg(0x3012, coarse_integration_time);

    g_psensor->curr_exp = MAX(1, coarse_integration_time * pinfo->t_row / 1000);

    return ret;

}

static u32 get_gain(void)
{
    u32 coarse_gain, fine_gain, dig_gain, dcg_gain;
    u32 reg_ang;

    if (g_psensor->curr_gain==0) {
        read_reg(0x3100, &dcg_gain);
        read_reg(0x3060, &reg_ang);
        read_reg(0x305E, &dig_gain);

        coarse_gain = 1<<((reg_ang>>4) & 0x03);

        fine_gain = reg_ang& 0xF;

        dig_gain = ((dig_gain >> 7) & 0x0f)* 128 + (dig_gain & 0x7f);

        if (dcg_gain)
           dcg_gain = 5;
        else
           dcg_gain = 2;

        //fine_gain=16x, dig_gain=128x, dig_gain=2x
        g_psensor->curr_gain = ((coarse_gain * (1*16 + fine_gain) * dig_gain * dcg_gain) << GAIN_SHIFT) / 16 / 128 / 2;
    }
    
    return g_psensor->curr_gain;
}

static int set_gain(u32 gain)
{
    int ret = 0;
    u32 coarse_gain, fine_gain, dig_gain, dcg_gain;
    u32 gain_reg=0;
    int i;

    if (gain > gain_table[NUM_OF_GAINSET - 1].gain_x)
        gain = gain_table[NUM_OF_GAINSET - 1].gain_x;
    else if(gain < gain_table[0].gain_x)
        gain = gain_table[0].gain_x;

    // search most suitable gain into gain table
    for (i=0; i<NUM_OF_GAINSET; i++) {
        if (gain_table[i].gain_x > gain)
            break;
    }

    read_reg(0x3100, &dcg_gain);
    coarse_gain = (gain_table[i-1].coarse_gain<<4) & 0x70; // shift to bit4(Reg 0x3060)
    fine_gain   = (gain_table[i-1].fine_gain)&0xF;
    dig_gain = (gain_table[i-1].dig_gain)&0x7FF;
    dcg_gain = (dcg_gain & 0xfffb) | ((gain_table[i-1].dcg_gain & 0x01) << 2);
    gain_reg = (coarse_gain|fine_gain);

    write_reg(0x3060, gain_reg);
    write_reg(0x305E, dig_gain);
    write_reg(0x3100, dcg_gain);
    
    g_psensor->curr_gain = gain_table[i-1].gain_x;

    return ret;
}

static int get_mirror(void)
{
    u32 reg;

    read_reg(0x3040, &reg);
    return (reg & BIT14) ? 1 : 0;
}

static int set_mirror(int enable)
{
    u32 reg;
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    read_reg(0x3040, &reg);
    if (enable)
        reg |= BIT14;
    else
        reg &=~BIT14;
    write_reg(0x3040, reg);
    pinfo->mirror = enable;

    return 0;
}

static int get_flip(void)
{
    u32 reg;

    read_reg(0x3040, &reg);
    return (reg & BIT15) ? 1 : 0;

}

static int set_flip(int enable)
{
    u32 reg;
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    read_reg(0x3040, &reg);
    if (enable)
        reg |= BIT15;
    else
        reg &=~BIT15;

    write_reg(0x3040, reg);
    pinfo->flip = enable;

    return 0;
}

static int cal_wdr_min_exp(void)
{
    u32 operation_mode_ctrl;
    int ratio, itemp=0,exp;
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    read_reg(0x3082, &operation_mode_ctrl);
    //Get the T1/T2 ratio.
    itemp = (operation_mode_ctrl>>2)&(BIT0|BIT1);

    if (itemp==0)
        ratio=4;
    else if (itemp==1)
        ratio=8;
    else if (itemp==2)
        ratio=16;
    else
        ratio=32;

    exp= MAX(1, pinfo->t_row * (ratio/2) / 1000);

    return exp;
}

static int cal_wdr_max_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 frame_length_line;
    u32 operation_mode_ctrl;
    int lines, exp, ratio, itemp=0;

    read_reg(0x3082, &operation_mode_ctrl);
    read_reg(0x300A, &frame_length_line);

    //Get the T1/T2 ratio.
    itemp = (operation_mode_ctrl>>2)&(BIT0|BIT1);

    if (itemp==0)
        ratio=4;
    else if (itemp==1)
        ratio=8;
    else if (itemp==2)
        ratio=16;
    else
        ratio=32;

    lines = MIN( 70* ratio, frame_length_line - 71);
    exp = MAX(1, pinfo->t_row * lines / 1000);
    return exp;

}

static int set_wdr_en(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;
    
    if (pinfo->sensor_id == ID_AR0141){
        isp_err("WDR not supported!\n");
        return -1;
    }

    read_reg(0x3082, &reg);//operation_mode_ctrl
    //set to power down mode
    write_reg(0x301A, 0x10D8);   // RESET_REGISTER
    msleep(100);
        
    if (enable){

        reg &= ~BIT0;
        g_psensor->fmt = RAW12_WDR16;
        //[HDR mode]
        write_reg(0x3082, 0x0008);    // OPERATION_MODE_CTRL  //16x16
        //write_reg(0x3190, 0x6BA0);     // [13] enable DLO Motion compensation
        write_reg(0x318C, 0x4040);       // [14] enable 2D MC
        write_reg(0x318A, 0x0E74);     // HDR_MC_CTRL1
        write_reg(0x3192, 0x0400);     // HDR_MC_CTRL5
        write_reg(0x3198, 0x183C);   //Motion detect Q1 set to 60, Q2 set to 24
        write_reg(0x318E, 0x0800);   //Gain before DLO set to 1
        write_reg(0x3194, 0x0BB8);   //T1 barrier set to 3000
        write_reg(0x3196, 0x0E74);   //T2 barrier set to 3700        
        mdelay(100);

#ifdef WDR_EXP_LIMIT
        g_psensor->min_exp = cal_wdr_min_exp();
#else
        g_psensor->min_exp = 2;
#endif
        g_psensor->max_exp = cal_wdr_max_exp(); // 0.5 sec

    }else{
        g_psensor->fmt = RAW12;
        //[Linear mode]
    	write_reg(0x3082, 0x0009);    // OPERATION_MODE_CTRL    
        mdelay(100);
        g_psensor->min_exp = 1;
        g_psensor->max_exp = 5000; // 0.5 sec

    }

    //set to streaming mode
    write_reg(0x301A, 0x10DC);   // RESET_REGISTER
    return 0;
}

static int get_wdr_en(void)
{
    int enable = (g_psensor->fmt & WDR_MASK)?1:0;
    return enable;
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
    case ID_WDR_EN:
        *(int*)arg = get_wdr_en();
        break;
    case ID_FPS:
        *(int*)arg = g_psensor->fps;
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
    case ID_WDR_EN:
        ret = set_wdr_en((int)arg);
        break;
    case ID_FPS:
        adjust_vblk((int)arg);
        break;
    default:
        ret = -EFAULT;
        break;
    }

    return ret;

}


static void get_sensor_id(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;
    int k;
    
    write_reg(0x301A, 0x0078);
    mdelay(100);
    write_reg(0x304C, 0x0200);
    mdelay(100);
    write_reg(0x304A, 0x0010);
    mdelay(100);
    k=0;
    while(k < 11){
        read_reg(0x304A, &reg);
        mdelay(100);
        if (reg & 0x20)
            break;
        k++;
    }
    if (k == 11)
        isp_warn("check sensor timeout\n");
    write_reg(0x304C, 0x2000);
    mdelay(100);
    write_reg(0x304A, 0x0010);
    mdelay(100);
    k=0;
    while(k < 11){
        read_reg(0x304A, &reg);
        mdelay(100);
        if (reg & 0x20)
            break;
        k++;
    }
    if (k == 11)
        isp_warn("check sensor timeout\n");

    read_reg(0x3000, &reg);
    pinfo->sensor_id = reg;
    if (pinfo->sensor_id == ID_AR0141)
        isp_info("AR0141\n");
    else
        isp_info("AR0140\n");
}

static int init(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0, i;
    sensor_reg_t* pInitTable;
    u32 InitTable_Num;

    if (pinfo->is_init)
        return 0;

    get_sensor_id();

    if (pinfo->sensor_id == ID_AR0140){
        pInitTable=sensor_init_regs;
        InitTable_Num=NUM_OF_INIT_REGS;
    }
    else{
        pInitTable=sensor_init_regs_ar0141;
        InitTable_Num=NUM_OF_INIT_REGS_AR0141;
    }
        
    
    for (i=0; i<InitTable_Num; i++) {
        if(pInitTable[i].addr == DELAY_CODE){
            mdelay(pInitTable[i].val);
        }
        else if(write_reg(pInitTable[i].addr, pInitTable[i].val) < 0) {
            isp_err("%s init fail!!\n", SENSOR_NAME);
            return -EINVAL;
        }
    }

    g_psensor->pclk = get_pclk(g_psensor->xclk);

    // get mirror and flip status
    pinfo->mirror = get_mirror();
    pinfo->flip = get_flip();

    set_flip(flip);
    set_mirror(mirror);

    // set image size
    ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height);
    if (ret == 0)
        pinfo->is_init = 1;

    //Must be after set_size()
    adjust_vblk(g_psensor->fps);

    // initial exposure and gain
    set_exp(g_psensor->min_exp);
    set_gain(g_psensor->min_gain);

    // get current shutter_width and gain setting
    get_exp();
    get_gain();

    if (wdr)
        isp_info("WDR mode on\n");
    else
        isp_info("WDR mode off\n");

    return ret;
}

//=============================================================================
// external functions
//=============================================================================
static void ar0140_deconstruct(void)
{
    if ((g_psensor) && (g_psensor->private)) {
        kfree(g_psensor->private);
    }

    sensor_remove_i2c_driver();
    // turn off EXT_CLK
    isp_api_extclk_onoff(0);
    // release CAP_RST
    isp_api_cap_rst_exit();
}

static int ar0140_construct(u32 xclk, u16 width, u16 height)
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
    mdelay(50);

    pinfo = kzalloc(sizeof(sensor_info_t), GFP_KERNEL);
    if (!pinfo) {
        isp_err("failed to allocate private data!\n");
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
    g_psensor->bayer_type = BAYER_GR;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_HIGH;
    g_psensor->inv_clk = inv_clk;
    g_psensor->img_win.x = 0;
    g_psensor->img_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;
    g_psensor->min_exp = 1;
    g_psensor->max_exp = 5000; // 0.5 sec

    g_psensor->min_gain = gain_table[0].gain_x;
    g_psensor->max_gain = gain_table[NUM_OF_GAINSET - 1].gain_x;

    g_psensor->exp_latency = 1;
    g_psensor->gain_latency = 1;
    g_psensor->fps = fps;

    //Call back function
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

    if ((ret = sensor_init_i2c_driver()) < 0)
        goto construct_err;

    if ((ret = init()) < 0)
        goto construct_err;

    return 0;

construct_err:
    ar0140_deconstruct();
    return ret;
}


//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init AR0140_init(void)
{
    int ret = 0;

    // check ISP driver version
    if (isp_get_input_inf_ver() != ISP_INPUT_INF_VER) {
        isp_err("input module version(%x) is not equal to fisp328.ko(%x)!\n",
             ISP_INPUT_INF_VER, isp_get_input_inf_ver());
        ret = -EINVAL;
        goto init_err1;
    }

    g_psensor = kzalloc(sizeof(sensor_dev_t), GFP_KERNEL);
    if (!g_psensor) {
        ret = -ENODEV;
        goto init_err1;
    }

    if ((ret = ar0140_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
        goto init_err2;

    // register sensor device to ISP driver
    if ((ret = isp_register_sensor(g_psensor)) < 0)
        goto init_err2;

    return ret;

init_err2:
    kfree(g_psensor);

init_err1:
    return ret;
}

static void __exit AR0140_exit(void)
{
    isp_unregister_sensor(g_psensor);
    ar0140_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}


module_init(AR0140_init);
module_exit(AR0140_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor AR0140");
MODULE_LICENSE("GPL");

