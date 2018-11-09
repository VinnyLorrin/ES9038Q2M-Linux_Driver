/*
 * Driver for hifibunny3 Q2M
 *
 * Author: Satoru Kawase
 *      Copyright 2018
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef _SND_SOC_hifibunny3
#define _SND_SOC_hifibunny3
/* ES9038Q2M register space */

#define ES9038Q2M_SYSTEM_SETTING    			0x00
#define ES9038Q2M_INPUT_CONFIG   			0x01
#define ES9038Q2M_MIXING				0x02
#define ES9038Q2M_AUTOMUTE_TIME   			0x04
#define ES9038Q2M_AUTOMUTE_LEVEL   			0x05
#define ES9038Q2M_DEEMP_DOP	    			0x06
#define ES9038Q2M_FILTER	   			0x07
#define ES9038Q2M_GPIO_CONFIG      			0x08
#define ES9038Q2M_MASTER_MODE    			0x0A
#define ES9038Q2M_DPLL   				0x0C
#define ES9038Q2M_THD_COMPENSATION   			0x0D
#define ES9038Q2M_SOFT_START	   			0x0E
#define ES9038Q2M_VOLUME1	   			0x0F
#define ES9038Q2M_VOLUME2	   			0x10
#define ES9038Q2M_MASTERTRIM0	   			0x11
#define ES9038Q2M_MASTERTRIM1	   			0x12
#define ES9038Q2M_MASTERTRIM2	  			0x13
#define ES9038Q2M_MASTERTRIM3	   			0x14
#define ES9038Q2M_INPUT_SEL	   			0x15
#define ES9038Q2M_2_HARMONIC_COMPENSATION_0	    	0x16
#define ES9038Q2M_2_HARMONIC_COMPENSATION_1	    	0x17
#define ES9038Q2M_3_HARMONIC_COMPENSATION_0	    	0x18
#define ES9038Q2M_3_HARMONIC_COMPENSATION_1	    	0x19
#define ES9038Q2M_GENERAL_CONFIG_0			0x1B
#define	ES9038Q2M_GPIO_INV				0x1D
#define ES9038Q2M_CP_CLK_0				0x1E
#define	ES9038Q2M_CP_CLK_1				0x1F
#define ES9038Q2M_INT_MASK				0x21
#define	ES9038Q2M_NCO_0					0x22
#define ES9038Q2M_NCO_1					0x23
#define	ES9038Q2M_NCO_2					0x24
#define	ES9038Q2M_NCO_3					0x25
#define ES9038Q2M_GENERAL_CONFIG_1			0x27
#define ES9038Q2M_program_FIR_ADDR	      		0x28
#define ES9038Q2M_program_FIR_DATA_0	   		0x29
#define ES9038Q2M_program_FIR_DATA_1	   		0x2A
#define ES9038Q2M_program_FIR_DATA_2	   		0x2B
#define ES9038Q2M_program_FIR_CONTROL	   		0x2C
#define ES9038Q2M_AUTO_CAL				0x2D
#define	ES9038Q2M_ADC_CONFIG				0x2E

#define ES9038Q2M_ADC_FTR_SCALE_0			0x2F
#define ES9038Q2M_ADC_FTR_SCALE_1			0x30
#define ES9038Q2M_ADC_FTR_SCALE_2			0x31
#define ES9038Q2M_ADC_FTR_SCALE_3			0x32
#define ES9038Q2M_ADC_FTR_SCALE_4			0x33
#define ES9038Q2M_ADC_FTR_SCALE_5			0x34

#define ES9038Q2M_ADC_FBQ_SCALE1_0			0x35
#define ES9038Q2M_ADC_FBQ_SCALE1_1			0x36

#define ES9038Q2M_ADC_FBQ_SCALE2_0			0x37
#define ES9038Q2M_ADC_FBQ_SCALE2_1			0x38

#define ES9038Q2M_Chip_ID					0x40

#define ES9038Q2M_SYSCLK_MCLK_HZ			49152000

#endif
