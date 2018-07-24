/*******************************************************************************
* Copyright  Grain-Media, Inc. 2011-2012.  All rights reserved.                *
*------------------------------------------------------------------------------*
* Name: txdt250cqr.c                                                           *
* Description: txdt250cqr control code                                         *
*                                                                              *
* Author: giggs_hu                                                             *
*******************************************************************************/

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <mach/ftpmu010.h>
#include "txdt250cqr.h"


//==============================================================================
//
//                              DEFINES
//
//==============================================================================

/* Definition for Driver Version */
#define txdt250cqr_VERSION        "0.1.0"
#define BIT0    0
#define BIT1    1
#define BIT7    7

#ifdef CONFIG_PLATFORM_GM8287
/* Definition for Serial Command Control PIN */
#define txdt250cqr_CS_PIN         5   /* GPIO0_5 */
#define txdt250cqr_SCL_PIN        4   /* GPIO0_4 */
#define txdt250cqr_SDA_PIN        7   /* GPIO0_7 */
#define txdt250cqr_RST_PIN        3   /* GPIO1_3 */
#elif defined(CONFIG_PLATFORM_GM8136)
#define txdt250cqr_CS_PIN         28   /* GPIO0_28 */
#define txdt250cqr_SCL_PIN        31   /* GPIO0_31 */
#define txdt250cqr_SDA_PIN        29   /* GPIO0_29 */
#define txdt250cqr_RST_PIN        24   /* GPIO1_24 */
#endif

static pmuReg_t pmu_reg_8136[] = {
#ifdef CONFIG_PLATFORM_GM8136
    {0x44, 0x1 << 30, 0x0, 0x1 << 30, 0x1 << 30},   /* enhance tv clock slew rate */
    {0x58, 0x3 << 16 | 0x3 << 14 | 0x3 << 12 | 0x3 << 10, 0x0, 0x0, 0x3 << 16 | 0x3 << 14 | 0x3 << 12 | 0x3 << 10},
    {0x5C, 0xFFFF, 0x0, 0xFFFF, 0xFFFF},
    {0x64, 0x3 << 8 | 0x3 << 2, 0x0, 0x3 << 8 | 0x0 << 2, 0x3 << 8 | 0x3 << 2},
#endif
};

static pmuRegInfo_t pmu_reg_info_8136 = {
    "txdt250cqr",
    ARRAY_SIZE(pmu_reg_8136),
    ATTR_TYPE_NONE,
    &pmu_reg_8136[0]
};
static int txdt250cqr_fd = -1;

//==============================================================================
//
//                              TYPE DEFINES
//
//==============================================================================

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================
/* Definition for Serial Command Data Format */
typedef struct txdt250cqr_sc_data {
  unsigned char reg;
  unsigned char value;
} txdt250cqr_SC_DATA_t;

//==============================================================================
//
//                              EXTERNAL VARIABLES REFERENCE
//
//==============================================================================

static txdt250cqr_SC_DATA_t txdt250cqr_DEFAULT_CFG[] = {
    // Reset
    {0x05, 0x1E},
    {0x05, 0x5C},

    // CCIR656 mode
    {0x04, 0x68},

    // Brightness & Contrast
    {0x0D, 0x41},
    {0x03, 0x42},

    // R B sub-brightness
    {0x0F, 0x40}, // R
    {0x11, 0x40}, // B

    // R B sub-contrast
    {0x0E, 0x40}, // R
    {0x10, 0x40}, // B

    {0x05, 0x5E},

    // Change standby to normal mode
    {0x2B, 0x01}

};

u32 txdt250cqr_SC_DATA_NUM = sizeof(txdt250cqr_DEFAULT_CFG)/sizeof(txdt250cqr_SC_DATA_t);

//==============================================================================
//
//                       HELPER FUNCTIONS
//
//==============================================================================
static __inline void delay(int tick)
{
    return mdelay((tick * HZ) / 1000);
}

//==============================================================================
//
//                      PRIVATE FUNCTIONS
//
//==============================================================================
static s32 txdt250cqr_pinmux_init(void)
{
    txdt250cqr_fd = ftpmu010_register_reg(&pmu_reg_info_8136);
    if (txdt250cqr_fd < 0) {
        printk("txdt250cqr_pinmux_init fail! \n");
        return -1;
    }
    gpio_direction_output(txdt250cqr_RST_PIN, 1);
    gpio_set_value(txdt250cqr_RST_PIN, 1);
    delay(20);
    gpio_set_value(txdt250cqr_RST_PIN, 0);
    delay(20);
    gpio_set_value(txdt250cqr_RST_PIN, 1);
    delay(20);

    /* Config CS pin direction to output and active high */
    gpio_direction_output(txdt250cqr_CS_PIN, 1);

    /* Config SCL pin direction to output and active high */
    gpio_direction_output(txdt250cqr_SCL_PIN, 1);

    /* Config SDA pin direction to input */
    gpio_direction_output(txdt250cqr_SDA_PIN, 0);

    return 0;
}

u16 txdt250cqr_read(u8 reg)
{
    int i;
    u8  bit_val;
    u16  rec_data = 0;
    u16 out_data = 0;

    /*
     * prepare 16 bit output data format
     *
     * [A6][R/W][A5][A4][A3][A2][A1][A0][D7][D6][D5][D4][D3][D2][D1][D0]
     *
     * [R/W]=> 0:write, 1:read
     */
     out_data = ((reg&0x40)<<9) | (1<<14) | ((reg&0x3F)<<8);

    /* config SDA pin direction to output and active high */
     gpio_direction_output(txdt250cqr_SDA_PIN, 1);

     /* active CS to low */
     gpio_set_value(txdt250cqr_CS_PIN, 0);

     for(i=15; i>=0; i--) {
         bit_val = (out_data>>i) & 0x01;

         /* active SCL to low */
         gpio_set_value(txdt250cqr_SCL_PIN, 0);

         /* output register address data */
         if(i >= 8)
             gpio_set_value(txdt250cqr_SDA_PIN, bit_val);
         else {
             /* receive register data */
             if(i==7)
                 gpio_direction_input(txdt250cqr_SDA_PIN); //switch SDA pin to input direction

             //if(gpio_get_value(txdt250cqr_SDA_PIN))
             //    rec_data |= (0x1<<i);
         }

         ndelay(50);

         /* active SCL to high */
         gpio_set_value(txdt250cqr_SCL_PIN, 1);

         //ndelay(50);
         ndelay(20);

         if(i < 8)
         {
             /* receive register data */
             int ret = gpio_get_value(txdt250cqr_SDA_PIN);
             //printf("%d ret = %d\n", i, ret);
             if(ret)
             {
                 rec_data |= (0x1<<i);
             }
          }

          ndelay(30);
       }

     /*active CS to high */
     gpio_set_value(txdt250cqr_CS_PIN, 1);

     ndelay(400);

    return rec_data;
}

int txdt250cqr_write(u8 reg, u8 data)
{
    int i;
    u8  bit_val;
    u16 out_data = 0;

    /*
      * prepare 16 bit output data format
      *
      * [A6][R/W][A5][A4][A3][A2][A1][A0][D7][D6][D5][D4][D3][D2][D1][D0]
      *
      * [R/W]=> 0:write, 1:read
      */
     out_data = ((reg&0x40)<<9) | ((reg&0x3F)<<8) | data;

     /* config SDA pin direction to output and active high */
     gpio_direction_output(txdt250cqr_SDA_PIN, 1);

      /*active CS to low */
      gpio_set_value(txdt250cqr_CS_PIN, 0);

      for(i=15; i>=0; i--) {
           bit_val = (out_data>>i) & 0x01;

           /* active SCL to low */
           gpio_set_value(txdt250cqr_SCL_PIN, 0);

           /* set output data */
           gpio_set_value(txdt250cqr_SDA_PIN, bit_val);

           ndelay(50);

           /* active SCL to high */
           gpio_set_value(txdt250cqr_SCL_PIN, 1);

           ndelay(50);
       }

      /* active CS to high */
       gpio_set_value(txdt250cqr_CS_PIN, 1);

       /* switch SDA pin to input direction */
       gpio_direction_input(txdt250cqr_SDA_PIN);

       ndelay(400);


    return 0;
}


int txdt250cqr_write_cmd(u16 out_data)
{
    int i;
    u8  bit_val;

    /*
      * prepare 16 bit output data format
      *
      * [A6][R/W][A5][A4][A3][A2][A1][A0][D7][D6][D5][D4][D3][D2][D1][D0]
      *
      * [R/W]=> 0:write, 1:read
      */

     /* config SDA pin direction to output and active high */
      gpio_direction_output(txdt250cqr_SDA_PIN, 1);
      /*active CS to low */
      gpio_set_value(txdt250cqr_CS_PIN, 0);

      for(i=15; i>=0; i--) {
           bit_val = (out_data>>i) & 0x01;

           /* active SCL to low */
           gpio_set_value(txdt250cqr_SCL_PIN, 0);

           /* set output data */
           gpio_set_value(txdt250cqr_SDA_PIN, bit_val);

           ndelay(50);

           /* active SCL to high */
           gpio_set_value(txdt250cqr_SCL_PIN, 1);

           ndelay(50);
       }

      /* active CS to high */
       gpio_set_value(txdt250cqr_CS_PIN, 1);

       /* switch SDA pin to input direction */
       gpio_direction_input(txdt250cqr_SDA_PIN);

       ndelay(400);


    return 0;
}

//==============================================================================
//
//                      PUBLIC FUNCTIONS
//
//==============================================================================
static int __init panel_txdt250cqr_init(void)
{
    int i;

    gpio_request(txdt250cqr_CS_PIN, "txdt250cqr_CS_PIN");
    gpio_request(txdt250cqr_SCL_PIN, "txdt250cqr_SCL_PIN");
    gpio_request(txdt250cqr_SDA_PIN, "txdt250cqr_SDA_PIN");
    gpio_request(txdt250cqr_RST_PIN, "txdt250cqr_RST_PIN");

     /* Config PinMux */
    if (txdt250cqr_pinmux_init() < 0)
        return -1;

    #ifdef _DEBUG_OTA5182
/*    for(i = 0; i < txdt250cqr_SC_DATA_NUM; i++) {
        if (i < 2)
            continue;

        reg = txdt250cqr_DEFAULT_CFG[i].reg;
        printf("%x, value:%x \n", reg, txdt250cqr_read(reg));
    } */
#endif

    /* Init txdt250cqr */
    for(i = 0; i < txdt250cqr_SC_DATA_NUM; i++) {
        txdt250cqr_write(txdt250cqr_DEFAULT_CFG[i].reg, txdt250cqr_DEFAULT_CFG[i].value);

        if (i == 0)
            mdelay(1);
        if (i == 1)
            mdelay(20);
    }
   /* for(i = 0; i < sizeof(txdt250cqr_DEFAULT_CMD)/sizeof(u16); i++) {
         txdt250cqr_write_cmd(txdt250cqr_DEFAULT_CMD[i]);

         //if (i == 0)
         //    mdelay(1);
         //if (i == 1)
         //    mdelay(20);
    }
    */
#ifdef _DEBUG_OTA5182
    for(i = 0; i < txdt250cqr_SC_DATA_NUM; i++) {
        if (i < 2)
            continue;

        reg = txdt250cqr_DEFAULT_CFG[i].reg;
        printk("%x, value:%x \n", reg, txdt250cqr_read(reg));
    }
#endif

    //reg = reg;
    //val = val;

    printk("* Panel A05DN01 Init .....................[OK]\n");

    return 0;
}

static void __exit panel_txdt250cqr_exit(void)
{
    gpio_free(txdt250cqr_CS_PIN);
    gpio_free(txdt250cqr_SCL_PIN);
    gpio_free(txdt250cqr_SDA_PIN);
    gpio_free(txdt250cqr_RST_PIN);

    if (txdt250cqr_fd >= 0)
        ftpmu010_deregister_reg(txdt250cqr_fd);
}

int panel_txdt250cqr_set_mirror(u32 enable)
{
    u8 val;
    //txdt250cqr_write(0x05, 0x5f);
    val = txdt250cqr_read(0x04);
    if (enable)
    {
        val |= (BIT0|BIT1);
    }
    else
    {
        val &= ~(BIT0|BIT1);
    }
    val=txdt250cqr_write(0x04, val);
    //txdt250cqr_write(0x05, 0x5e);
    return val;
}

int panel_txdt250cqr_get_mirror()
{
    return (txdt250cqr_read(0x04) & BIT0);
}

int panel_txdt250cqr_set_flip(u32 enable)
{
#if 0
    u8 val;

    //txdt250cqr_write(0x05, 0x5f);
    val = txdt250cqr_read(0x04);
    if (enable)
    {
        val |= BIT1;
    }
    else
    {
        val &= ~BIT1;
    }
    val = txdt250cqr_write(0x04, val);
    //txdt250cqr_write(0x05, 0x5e);

    return val;
#endif
    return 1;
}

int panel_txdt250cqr_get_flip()
{
    return (txdt250cqr_read(0x04) & BIT1);
}

int panel_txdt250cqr_set_narrow(u32 enable)
{
    u8 val;

    val = txdt250cqr_read(0x04);
    if (enable)
    {
        val |= BIT7;
    }
    else
    {
        val &= ~BIT7;
    }

    return txdt250cqr_write(0x04, val);
}

int panel_txdt250cqr_get_narrow()
{
    return (txdt250cqr_read(0x04) & BIT7);
}

void panel_txdt250cqr_set_standby(u8 ON_off)
{
	if(ON_off)
		txdt250cqr_write(0x2B, 0x00);
	else
		txdt250cqr_write(0x2B, 0x01);
}

module_init(panel_txdt250cqr_init);
module_exit(panel_txdt250cqr_exit);
MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("txdt250cqr Function Driver");
MODULE_LICENSE("GPL");