/*
 *  arch/arm/mach-GM/include/mach/platform-GM8126/platform_io.h
 *
 *  Faraday GM platform dependent definitions
 *
 *  Copyright (C) 2005 Faraday Corp. (http://www.faraday-tech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ChangeLog
 *  Add IRQ number definition
 *  Add IP module phy address definition
 *  
 */
#ifndef __PLATFORM_IO_H__
#define __PLATFORM_IO_H__


#define PLATFORM_FIQ_BASE       0
#define PLATFORM_INTERRUPTS     64      /* interrupt controller supports 64 interrupts */
#define CPU_MEM_PA_BASE         0x0     /* the memory physical start address(DDR) */

/*
 * Component counts
 */ 

/* PWMTMR */
#define PWMTMR_COUNT	1
#define PWMTMR_FTPWM010_COUNT	1
/* WDT */
#define WDT_COUNT	1
#define WDT_FTWDT010_COUNT	1
/* GPIO */
#define GPIO_COUNT	3
#define GPIO_FTGPIO010_COUNT	3
/* I2C */
#define I2C_COUNT	1
#define I2C_FTI2C010_COUNT	1 
/* SSP */
#define SSP_COUNT	3
#define SSP_FTSSP010_COUNT	3
/* SDC */
#define SDC_COUNT	1
#define SDC_FTSDC010_COUNT	1
/* NAND */
#define NAND_COUNT	1
#define NAND_FTNAND023_COUNT	1
/* NANDDP */
#define NANDDP_COUNT	1
#define NANDDP_FTNAND023_COUNT	1
/* USB */
#define USB_COUNT       1
#define USB_FOTG2XX_COUNT       1
/* LCD */
#define LCD_COUNT	1
#define LCD_FTLCDC200_COUNT	1
/* MAC */
#define MAC_COUNT	1
#define MAC_FTMAC110_COUNT	1
/* TVE */
#define TVE_COUNT	1
#define TVE_FTTVE100_COUNT	1
/* AES */
#define AES_COUNT	1
#define AES_FTAES020_COUNT	1
/* SCAL */
#define SCAL_COUNT	1
#define SCAL_FTSCAL010_COUNT	1
/* CAP */
#define CAP_COUNT	2
#define CAP_FTCAP020_COUNT	2
/* DI3D */
#define DI3D_COUNT	1
#define DI3D_FTDI3D_COUNT	1
/* H264E */
#define H264E_COUNT	1
#define H264E_FTMCP200_COUNT	1
/* WRAP */
#define WRAP_COUNT	1
#define WRAP_FTWRAP00_COUNT	1
/* MCP100W */
#define MCP100W_COUNT	1
#define MCP100W_FTMCP100_COUNT	1
/* MCP */
#define MCP_COUNT	1
#define MCP_FTMCP100_COUNT	1
/* CT656 */
#define CT656_COUNT	1
#define CT656_FTCT656_COUNT	1
/* ISP_BME */
#define ISP_BME_COUNT	1
#define ISP_BME_FTISPBME_COUNT	1
/* ISP210 */
#define ISP210_COUNT	1
#define ISP210_FTISP210_COUNT	1
/* IR_DET */
#define IR_DET_COUNT	1
#define IR_DET_FTIRDET_COUNT	1
/* RTC */
#define RTC_COUNT	1
#define RTC_FTRTC_COUNT	1

/* ADC */
#define ADC_COUNT	1
#define ADC_WRAP_COUNT	1

/* DMAC020 */
#define DMAC_FTDMAC020_COUNT 1

/* APBBRG */
#define APBBRG_COUNT	1
#define APBBRG_FTAPBBRG020S_COUNT	1

/* DDRC */
#define DDRC_COUNT	1
#define DDRC_FTDDRC010_COUNT	1
/*
 * Interrrupt numbers
 */
/* PWMTMR */
#define PWMTMR_FTPWM010_IRQ_COUNT	8
#define PWMTMR_FTPWM010_IRQ0	34
#define PWMTMR_FTPWM010_0_IRQ0	34
#define PWMTMR_FTPWM010_IRQ1	35
#define PWMTMR_FTPWM010_0_IRQ1	35
#define PWMTMR_FTPWM010_IRQ2	36
#define PWMTMR_FTPWM010_0_IRQ2	36
#define PWMTMR_FTPWM010_IRQ3	37
#define PWMTMR_FTPWM010_0_IRQ3	37
#define PWMTMR_FTPWM010_IRQ4	38
#define PWMTMR_FTPWM010_0_IRQ4	38
#define PWMTMR_FTPWM010_IRQ5	42
#define PWMTMR_FTPWM010_0_IRQ5	42
#define PWMTMR_FTPWM010_IRQ6	43
#define PWMTMR_FTPWM010_0_IRQ6	43
#define PWMTMR_FTPWM010_IRQ7	44
#define PWMTMR_FTPWM010_0_IRQ7	44

/* WDT */
#define WDT_FTWDT010_IRQ_COUNT	1
#define WDT_FTWDT010_IRQ	16
#define WDT_FTWDT010_0_IRQ	16

/* GPIO */
#define GPIO_FTGPIO010_IRQ_COUNT	1
#define GPIO_FTGPIO010_IRQ	13
#define GPIO_FTGPIO010_0_IRQ	13
#define GPIO_FTGPIO010_1_IRQ	17
#define GPIO_FTGPIO010_2_IRQ	25

/* I2C */
#define I2C_FTI2C010_IRQ_COUNT	1
#define I2C_FTI2C010_IRQ	18
#define I2C_FTI2C010_0_IRQ	18

/* SSP */
#define SSP_FTSSP010_IRQ_COUNT	1
#define SSP_FTSSP010_IRQ	6
#define SSP_FTSSP010_0_IRQ	6
#define SSP_FTSSP010_1_IRQ	7
#define SSP_FTSSP010_2_IRQ	11

/* SDC */
#define SDC_FTSDC010_IRQ_COUNT	1
#define SDC_FTSDC010_IRQ	15
#define SDC_FTSDC010_0_IRQ	15

/* NAND */
#define NAND_FTNAND023_IRQ_COUNT	1
#define NAND_FTNAND023_IRQ	23
#define NAND_FTNAND023_0_IRQ	23

/* USB */
#define USB_FOTG2XX_IRQ_COUNT	1
#define USB_FOTG2XX_IRQ		4
#define USB_FOTG2XX_0_IRQ	4

/* LCD */
#define LCD_FTLCDC200_IRQ_COUNT	1
#define LCD_FTLCDC200_IRQ	24
#define LCD_FTLCDC200_0_IRQ	24

/* MAC */
#define MAC_FTMAC110_IRQ_COUNT	1
#define MAC_FTMAC110_IRQ	3
#define MAC_FTMAC110_0_IRQ	3

/* AES */
#define AES_FTAES020_IRQ_COUNT	1
#define AES_FTAES020_IRQ	19
#define AES_FTAES020_0_IRQ	19

/* SCAL */
#define SCAL_FTSCAL010_IRQ_COUNT	1
#define SCAL_FTSCAL010_IRQ	12
#define SCAL_FTSCAL010_0_IRQ	12

/* CAP */
#define CAP_FTCAP020_IRQ_COUNT	1
#define CAP_FTCAP020_IRQ	32
#define CAP_FTCAP020_0_IRQ	32
#define CAP_FTCAP020_1_IRQ	33

/* DI3D */
#define DI3D_FTDI3D_IRQ_COUNT	1
#define DI3D_FTDI3D_IRQ	41
#define DI3D_FTDI3D_0_IRQ	41

/* H264E */
#define H264E_FTMCP200_IRQ_COUNT	1
#define H264E_FTMCP200_IRQ	29
#define H264E_FTMCP200_0_IRQ	29

/* WRAP */
#define WRAP_FTWRAP00_IRQ_COUNT	1
#define WRAP_FTWRAP00_IRQ	29
#define WRAP_FTWRAP00_0_IRQ	29

/* MCP100W */
#define MCP100W_FTMCP100_IRQ_COUNT	1
#define MCP100W_FTMCP100_IRQ	31
#define MCP100W_FTMCP100_0_IRQ	31

/* MCP */
#define MCP_FTMCP100_IRQ_COUNT	1
#define MCP_FTMCP100_IRQ	31
#define MCP_FTMCP100_0_IRQ	31

/* ISP_BME */
#define ISP_BME_FTISPBME_IRQ_COUNT  1
#define ISP_BME_FTISPBME_IRQ    45
#define ISP_BME_FTISPBME_0_IRQ  45

/* ISP210 */
#define ISP210_FTISP210_IRQ_COUNT	3
#define ISP210_FTISP210_IRQ0	27
#define ISP210_FTISP210_0_IRQ0	27
#define ISP210_FTISP210_IRQ1	28
#define ISP210_FTISP210_0_IRQ1	28
#define ISP210_FTISP210_IRQ2	30
#define ISP210_FTISP210_0_IRQ2	30

/* IR_DET */
#define IR_DET_FTIRDET_IRQ_COUNT	1
#define IR_DET_FTIRDET_IRQ	39
#define IR_DET_FTIRDET_0_IRQ	39

/* RTC */
#define RTC_FTRTC_IRQ_COUNT	1
#define RTC_FTRTC_IRQ	5
#define RTC_FTRTC_0_IRQ	5

/* ADC */
#define ADC_WRAP_IRQ_COUNT	1
#define ADC_WRAP_IRQ	40
#define ADC_WRAP_0_IRQ	40

/* DMAC020 */
#define DMAC_FTDMAC020_IRQ_COUNT    1
#define DMAC_FTDMAC020_IRQ  1

/* APBBRG */
#define APBBRG_FTAPBBRG020S_IRQ_COUNT	1
#define APBBRG_FTAPBBRG020S_IRQ		2

/*
 * Base addresses
 */
 
/* PWMTMR */
#define PWMTMR_FTPWM010_PA_COUNT	1
#define PWMTMR_FTPWM010_PA_BASE	    0x98B00000
#define PWMTMR_FTPWM010_PA_LIMIT	0x98B0FFFF
#define PWMTMR_FTPWM010_PA_SIZE	    0x00010000
#define PWMTMR_FTPWM010_0_PA_BASE	0x98B00000
#define PWMTMR_FTPWM010_0_PA_LIMIT	0x98B0FFFF
#define PWMTMR_FTPWM010_0_PA_SIZE	0x00010000

/* WDT */
#define WDT_FTWDT010_PA_COUNT	1
#define WDT_FTWDT010_PA_BASE	0x99200000
#define WDT_FTWDT010_PA_LIMIT	0x99200FFF
#define WDT_FTWDT010_PA_SIZE	0x00001000
#define WDT_FTWDT010_0_PA_BASE	0x99200000
#define WDT_FTWDT010_0_PA_LIMIT	0x99200FFF
#define WDT_FTWDT010_0_PA_SIZE	0x00001000

/* GPIO */
#define GPIO_FTGPIO010_PA_COUNT	1
#define GPIO_FTGPIO010_PA_BASE	0x99400000
#define GPIO_FTGPIO010_PA_LIMIT	0x99400FFF
#define GPIO_FTGPIO010_PA_SIZE	0x00001000
#define GPIO_FTGPIO010_0_PA_BASE	0x99400000
#define GPIO_FTGPIO010_0_PA_LIMIT	0x99400FFF
#define GPIO_FTGPIO010_0_PA_SIZE	0x00001000
#define GPIO_FTGPIO010_1_PA_BASE	0x99700000
#define GPIO_FTGPIO010_1_PA_LIMIT	0x99700FFF
#define GPIO_FTGPIO010_1_PA_SIZE	0x00001000
#define GPIO_FTGPIO010_2_PA_BASE	0x99800000
#define GPIO_FTGPIO010_2_PA_LIMIT	0x99800FFF
#define GPIO_FTGPIO010_2_PA_SIZE	0x00001000

/* I2C */
#define I2C_FTI2C010_PA_COUNT	1
#define I2C_FTI2C010_PA_BASE	0x99600000
#define I2C_FTI2C010_PA_LIMIT	0x99600FFF
#define I2C_FTI2C010_PA_SIZE	0x00001000
#define I2C_FTI2C010_0_PA_BASE	0x99600000
#define I2C_FTI2C010_0_PA_LIMIT	0x99600FFF
#define I2C_FTI2C010_0_PA_SIZE	0x00001000

/* SSP */
#define SSP_FTSSP010_PA_COUNT	1
#define SSP_FTSSP010_PA_BASE	0x98200000
#define SSP_FTSSP010_PA_LIMIT	0x98200FFF
#define SSP_FTSSP010_PA_SIZE	0x00001000
#define SSP_FTSSP010_0_PA_BASE	0x98200000
#define SSP_FTSSP010_0_PA_LIMIT	0x98200FFF
#define SSP_FTSSP010_0_PA_SIZE	0x00001000
#define SSP_FTSSP010_1_PA_BASE	0x98600000
#define SSP_FTSSP010_1_PA_LIMIT	0x98600FFF
#define SSP_FTSSP010_1_PA_SIZE	0x00001000
#define SSP_FTSSP010_2_PA_BASE	0x98A00000
#define SSP_FTSSP010_2_PA_LIMIT	0x98A00FFF
#define SSP_FTSSP010_2_PA_SIZE	0x00001000

/* SDC */
#define SDC_FTSDC010_PA_COUNT	1
#define SDC_FTSDC010_PA_BASE	0x92800000
#define SDC_FTSDC010_PA_LIMIT	0x92800FFF
#define SDC_FTSDC010_PA_SIZE	0x00001000
#define SDC_FTSDC010_0_PA_BASE	0x92800000
#define SDC_FTSDC010_0_PA_LIMIT	0x92800FFF
#define SDC_FTSDC010_0_PA_SIZE	0x00001000

/* NAND */
#define NAND_FTNAND023_PA_COUNT	1
#define NAND_FTNAND023_PA_BASE	0x92C00000
#define NAND_FTNAND023_PA_LIMIT	0x92CFFFFF
#define NAND_FTNAND023_PA_SIZE	0x00100000
#define NAND_FTNAND023_0_PA_BASE	0x92C00000
#define NAND_FTNAND023_0_PA_LIMIT	0x92CFFFFF
#define NAND_FTNAND023_0_PA_SIZE	0x00100000

/* NANDDP */
#define NANDDP_FTNAND023_PA_COUNT	1
#define NANDDP_FTNAND023_PA_BASE	0x96000000
#define NANDDP_FTNAND023_PA_LIMIT	0x960FFFFF
#define NANDDP_FTNAND023_PA_SIZE	0x00100000
#define NANDDP_FTNAND023_0_PA_BASE	0x96000000
#define NANDDP_FTNAND023_0_PA_LIMIT	0x960FFFFF
#define NANDDP_FTNAND023_0_PA_SIZE	0x00100000
#define NANDDP_FTNAND023_VA_COUNT	1
#define NANDDP_FTNAND023_VA_BASE	0xF9600000
#define NANDDP_FTNAND023_VA_LIMIT	0xF960FFFF
#define NANDDP_FTNAND023_VA_SIZE	0x00010000
#define NANDDP_FTNAND023_0_VA_BASE	0xF9600000
#define NANDDP_FTNAND023_0_VA_LIMIT	0xF960FFFF
#define NANDDP_FTNAND023_0_VA_SIZE	0x00010000

/* USB */
#define USB_FOTG2XX_PA_COUNT	1
#define USB_FOTG2XX_PA_BASE	0x92200000
#define USB_FOTG2XX_PA_LIMIT	0x92200FFF
#define USB_FOTG2XX_PA_SIZE	0x00001000
#define USB_FOTG2XX_0_PA_BASE	0x92200000
#define USB_FOTG2XX_0_PA_LIMIT	0x92200FFF
#define USB_FOTG2XX_0_PA_SIZE	0x00001000

/* LCD */
#define LCD_FTLCDC200_PA_COUNT	1
#define LCD_FTLCDC200_PA_BASE	0x98700000
#define LCD_FTLCDC200_PA_LIMIT	0x9870CFFF
#define LCD_FTLCDC200_PA_SIZE	0x0000D000
#define LCD_FTLCDC200_0_PA_BASE	0x98700000
#define LCD_FTLCDC200_0_PA_LIMIT	0x9870CFFF
#define LCD_FTLCDC200_0_PA_SIZE	0x0000D000

/* MAC */
#define MAC_FTMAC110_PA_COUNT	1
#define MAC_FTMAC110_PA_BASE	0x92500000
#define MAC_FTMAC110_PA_LIMIT	0x92500FFF
#define MAC_FTMAC110_PA_SIZE	0x00001000
#define MAC_FTMAC110_0_PA_BASE	0x92500000
#define MAC_FTMAC110_0_PA_LIMIT	0x92500FFF
#define MAC_FTMAC110_0_PA_SIZE	0x00001000

/* TVE */
#define TVE_FTTVE100_PA_COUNT	1
#define TVE_FTTVE100_PA_BASE	0x92B00000
#define TVE_FTTVE100_PA_LIMIT	0x92B00FFF
#define TVE_FTTVE100_PA_SIZE	0x00001000
#define TVE_FTTVE100_0_PA_BASE	0x92B00000
#define TVE_FTTVE100_0_PA_LIMIT	0x92B00FFF
#define TVE_FTTVE100_0_PA_SIZE	0x00001000

/* AES */
#define AES_FTAES020_PA_COUNT	1
#define AES_FTAES020_PA_BASE	0x92700000
#define AES_FTAES020_PA_LIMIT	0x92700FFF
#define AES_FTAES020_PA_SIZE	0x00001000
#define AES_FTAES020_0_PA_BASE	0x92700000
#define AES_FTAES020_0_PA_LIMIT	0x92700FFF
#define AES_FTAES020_0_PA_SIZE	0x00001000

/* SCAL */
#define SCAL_FTSCAL010_PA_COUNT	1
#define SCAL_FTSCAL010_PA_BASE	0x92400000
#define SCAL_FTSCAL010_PA_LIMIT	0x92400FFF
#define SCAL_FTSCAL010_PA_SIZE	0x00001000
#define SCAL_FTSCAL010_0_PA_BASE	0x92400000
#define SCAL_FTSCAL010_0_PA_LIMIT	0x92400FFF
#define SCAL_FTSCAL010_0_PA_SIZE	0x00001000

/* CAP */
#define CAP_FTCAP020_PA_COUNT	1
#define CAP_FTCAP020_PA_BASE	0x90B00000
#define CAP_FTCAP020_PA_LIMIT	0x90B00FFF
#define CAP_FTCAP020_PA_SIZE	0x00001000
#define CAP_FTCAP020_0_PA_BASE	0x90B00000
#define CAP_FTCAP020_0_PA_LIMIT	0x90B00FFF
#define CAP_FTCAP020_0_PA_SIZE	0x00001000
#define CAP_FTCAP020_1_PA_BASE	0x90C00000
#define CAP_FTCAP020_1_PA_LIMIT	0x90C00FFF
#define CAP_FTCAP020_1_PA_SIZE	0x00001000

/* DI3D */
#define DI3D_FTDI3D_PA_COUNT	1
#define DI3D_FTDI3D_PA_BASE	    0x91400000
#define DI3D_FTDI3D_PA_LIMIT	0x91400FFF
#define DI3D_FTDI3D_PA_SIZE	    0x00001000
#define DI3D_FTDI3D_0_PA_BASE	0x91400000
#define DI3D_FTDI3D_0_PA_LIMIT	0x91400FFF
#define DI3D_FTDI3D_0_PA_SIZE	0x00001000

/* H264E */
#define H264E_FTMCP200_PA_COUNT	1
#define H264E_FTMCP200_PA_BASE	0x94000000
#define H264E_FTMCP200_PA_LIMIT	0x940FFFFF
#define H264E_FTMCP200_PA_SIZE	0x00100000
#define H264E_FTMCP200_0_PA_BASE	0x94000000
#define H264E_FTMCP200_0_PA_LIMIT	0x940FFFFF
#define H264E_FTMCP200_0_PA_SIZE	0x00100000

/* WRAP */
#define WRAP_FTWRAP00_PA_COUNT	1
#define WRAP_FTWRAP00_PA_BASE	0x94100000
#define WRAP_FTWRAP00_PA_LIMIT	0x94100FFF
#define WRAP_FTWRAP00_PA_SIZE	0x00001000
#define WRAP_FTWRAP00_0_PA_BASE	0x94100000
#define WRAP_FTWRAP00_0_PA_LIMIT	0x94100FFF
#define WRAP_FTWRAP00_0_PA_SIZE	0x00001000

/* MCP100W */
#define MCP100W_FTMCP100_PA_COUNT	1
#define MCP100W_FTMCP100_PA_BASE	0x94200000
#define MCP100W_FTMCP100_PA_LIMIT	0x94200FFF
#define MCP100W_FTMCP100_PA_SIZE	0x00001000
#define MCP100W_FTMCP100_0_PA_BASE	0x94200000
#define MCP100W_FTMCP100_0_PA_LIMIT	0x94200FFF
#define MCP100W_FTMCP100_0_PA_SIZE	0x00001000

/* MCP */
#define MCP_FTMCP100_PA_COUNT	1
#define MCP_FTMCP100_PA_BASE	0x94300000
#define MCP_FTMCP100_PA_LIMIT	0x943FFFFF
#define MCP_FTMCP100_PA_SIZE	0x00100000
#define MCP_FTMCP100_0_PA_BASE	0x94300000
#define MCP_FTMCP100_0_PA_LIMIT	0x943FFFFF
#define MCP_FTMCP100_0_PA_SIZE	0x00100000

/* CT656 */
#define CT656_FTCT656_PA_COUNT	1
#define CT656_FTCT656_PA_BASE	0x92A00000
#define CT656_FTCT656_PA_LIMIT	0x92A00FFF
#define CT656_FTCT656_PA_SIZE	0x00001000
#define CT656_FTCT656_0_PA_BASE	0x92A00000
#define CT656_FTCT656_0_PA_LIMIT	0x92A00FFF
#define CT656_FTCT656_0_PA_SIZE	0x00001000

/* ISP_BME */
#define ISP_BME_FTISPBME_PA_COUNT	1
#define ISP_BME_FTISPBME_PA_BASE	0x92D00000
#define ISP_BME_FTISPBME_PA_LIMIT	0x92DFFFFF
#define ISP_BME_FTISPBME_PA_SIZE	0x00100000
#define ISP_BME_FTISPBME_0_PA_BASE	0x92D00000
#define ISP_BME_FTISPBME_0_PA_LIMIT	0x92DFFFFF
#define ISP_BME_FTISPBME_0_PA_SIZE	0x00100000

/* ISP210 */
#define ISP210_FTISP210_PA_COUNT	1
#define ISP210_FTISP210_PA_BASE	    0x92300000
#define ISP210_FTISP210_PA_LIMIT	0x923FFFFF
#define ISP210_FTISP210_PA_SIZE	    0x00100000
#define ISP210_FTISP210_0_PA_BASE	0x92300000
#define ISP210_FTISP210_0_PA_LIMIT	0x923FFFFF
#define ISP210_FTISP210_0_PA_SIZE	0x00100000

/* IR_DET */
#define IR_DET_FTIRDET_PA_COUNT	1
#define IR_DET_FTIRDET_PA_BASE	0x98C00000
#define IR_DET_FTIRDET_PA_LIMIT	0x98C00FFF
#define IR_DET_FTIRDET_PA_SIZE	0x00001000
#define IR_DET_FTIRDET_0_PA_BASE	0x98C00000
#define IR_DET_FTIRDET_0_PA_LIMIT	0x98C00FFF
#define IR_DET_FTIRDET_0_PA_SIZE	0x00001000

/* RTC */
#define RTC_FTRTC_PA_COUNT	1
#define RTC_FTRTC_PA_BASE	0x99900000
#define RTC_FTRTC_PA_LIMIT	0x99900FFF
#define RTC_FTRTC_PA_SIZE	0x00001000
#define RTC_FTRTC_0_PA_BASE	0x99900000
#define RTC_FTRTC_0_PA_LIMIT	0x99900FFF
#define RTC_FTRTC_0_PA_SIZE	0x00001000

/* ADC */
#define ADC_WRAP_PA_COUNT	1
#define ADC_WRAP_PA_BASE	0x98D00000
#define ADC_WRAP_PA_LIMIT	0x98D00FFF
#define ADC_WRAP_PA_SIZE	0x00001000
#define ADC_WRAP_0_PA_BASE	0x98D00000
#define ADC_WRAP_0_PA_LIMIT	0x98D00FFF
#define ADC_WRAP_0_PA_SIZE	0x00001000

/* DMAC020 */
#define DMAC_FTDMAC020_PA_COUNT 1
#define DMAC_FTDMAC020_PA_BASE  0x92600000
#define DMAC_FTDMAC020_PA_LIMIT 0x92600FFF
#define DMAC_FTDMAC020_PA_SIZE  0x0001000

/* APBBRG */
#define APBBRG_FTAPBBRG020S_PA_COUNT	1
#define APBBRG_FTAPBBRG020S_PA_BASE	0x92000000
#define APBBRG_FTAPBBRG020S_PA_LIMIT	0x920FFFFF
#define APBBRG_FTAPBBRG020S_PA_SIZE	0x00100000
#define APBBRG_FTAPBBRG020S_0_PA_BASE	0x92000000
#define APBBRG_FTAPBBRG020S_0_PA_LIMIT	0x920FFFFF
#define APBBRG_FTAPBBRG020S_0_PA_SIZE	0x00100000

/* DDRC */
#define DDRC_FTDDRC010_PA_COUNT	1
#define DDRC_FTDDRC010_PA_BASE	0x99300000
#define DDRC_FTDDRC010_PA_LIMIT	0x99300FFF
#define DDRC_FTDDRC010_PA_SIZE	0x00001000

/*
 * IRQ/FIQ trigger level and trigger mode
 */

#define PLATFORM_IRQ_TRIGGER_MODE2	    0x00000000
#define PLATFORM_IRQ_TRIGGER_LEVEL2	    0xFFFFFFFF
#define PLATFORM_FIQ_TRIGGER_MODE2	    0x00000000
#define PLATFORM_FIQ_TRIGGER_LEVEL2	    0xFFFFFFFF

#ifdef CONFIG_FTINTC010EX /* 2010/03/26: autospec doesn't support extended IRQ/FIQ, had reported it to CTD */
#define PLATFORM_IRQ_TRIGGER_MODEEX2	0x00002000  // set IRQ45 as edge-triggered mode
#define PLATFORM_IRQ_TRIGGER_LEVELEX2	0xFFFFDFFF  // set IRQ45 as rising-edge trigger
#define PLATFORM_FIQ_TRIGGER_MODEEX2	0x00000000
#define PLATFORM_FIQ_TRIGGER_LEVELEX2	0xFFFFFFFF
#endif /* CONFIG_FTINTC010EX */
 
#endif /* __PLATFORM_IO_H__ */
