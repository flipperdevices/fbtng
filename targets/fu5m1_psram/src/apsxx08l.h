#pragma once

//https://www.apmemory.com/wp-content/uploads/APM_PSRAM_OPI_Xccela-APS12808L-3OBMx-v1.1b-PKG.pdf

/* Default dummy clocks cycles */
#define APSXX08L_DUMMY_CLOCK_CYCLES_READ                 8
#define APSXX08L_DUMMY_CLOCK_CYCLES_WRITE                4

#define APS3208L_RAM_SIZE               0x400000               /* 32 MBits => 4 MBytes */
#define APS6408L_RAM_SIZE               0x800000               /* 64 MBits => 8 MBytes */
#define APS12808L_RAM_SIZE              0x1000000              /* 128 MBits => 16 MBytes */
#define APS25608L_RAM_SIZE              0x2000000              /* 256 MBits => 32 MBytes */

/******************************************************************************
  * @brief  APS6408 Registers
  ****************************************************************************/
/* Mode Register 0 R/W*/
#define APSXX08L_MR0_ADDRESS           	 	0x00000000U

#define APSXX08L_MR0_DRIVE_STRENGTH_MSK 		0x03U       		/*!< Drive Strength                      */
#define APSXX08L_MR0_DS_HALF            		(0x00U << 0)     	/*!< Drive Strength : Full (50 ohm)      */
#define APSXX08L_MR0_DS_QUART           		(0x01U << 0)       	/*!< Drive Strength : Half (100 ohm)     */
#define APSXX08L_MR0_DS_EIGHT           		(0x02U << 0)       	/*!< Drive Strength : 1/4 (200 ohm)      */
#define APSXX08L_MR0_DS_SIXTEEN         		(0x03U << 0)       	/*!< Drive Strength : 1/8 (400 ohm)      */

#define APSXX08L_MR0_READ_LATENCY_CODE_MSK	0x3CU       		    /*!< Read Latency Code                   */
#define APSXX08L_MR0_RLC_3              		(0x00U << 2)        /*!< Read Latency Code : 3               */
#define APSXX08L_MR0_RLC_4              		(0x01U << 2)       	/*!< Read Latency Code : 4               */
#define APSXX08L_MR0_RLC_5              		(0x02U << 2)       	/*!< Read Latency Code : 5               */
#define APSXX08L_MR0_RLC_6              		(0x08U << 2)       	/*!< Read Latency Code : 6               */
#define APSXX08L_MR0_RLC_8              		(0x09U << 2)       	/*!< Read Latency Code : 8               */
#define APSXX08L_MR0_RLC_10             		(0x0AU << 2)       	/*!< Read Latency Code : 10              */

#define APSXX08L_MR0_LATENCY_TYPE_MSK   		0x20U       		/*!< Latency Type                        */

/* Mode Register 1 R*/
#define APSXX08L_MR1_ADDRESS            		0x00000001U

#define APSXX08L_MR1_VENDOR_ID_MSK      		0x1FU       		/*!< Vendor Identifier                   */
#define APSXX08L_MR1_VENDOR_ID_APM      		(0x0DU << 0)        /*!< Vendor Identifier : APM             */

/* May not be supported in this series */
#define APSXX08L_MR1_ULP                	    (0x01U << 7)        /*!< Ultra Low Power Device              */

/* Mode Register 2 R*/
#define APSXX08L_MR2_ADDRESS            		0x00000002U

#define APSXX08L_MR2_DENSITY_MSK           	0x07U       		/*!< Device Density                      */
#define APSXX08L_MR2_DENSITY_32_MB     		(0x01U << 0)       	/*!< Device Density : 32 Mb              */
#define APSXX08L_MR2_DENSITY_64_MB     		(0x03U << 0)       	/*!< Device Density : 64 Mb              */
#define APSXX08L_MR2_DENSITY_128_MB     	(0x05U << 0)       	/*!< Device Density : 128 Mb             */
#define APSXX08L_MR2_DENSITY_256_MB     	(0x07U << 0)       	/*!< Device Density : 256 Mb             */

#define APSXX08L_MR2_DEVICE_ID_MSK      	0x18U       		/*!< Device Identifier                   */
#define APSXX08L_MR2_DEVID_GEN_1        	(0x00U << 3)        /*!< Device Identifier : Generation 1    */
#define APSXX08L_MR2_DEVID_GEN_2        	(0x01U << 3)       	/*!< Device Identifier : Generation 2    */
#define APSXX08L_MR2_DEVID_GEN_3        	(0x02U << 3)       	/*!< Device Identifier : Generation 3    */


#define APSXX08L_MR2_GB_MSK                	0x80U               /*!< Good-Die Identifier                  */
#define APSXX08L_MR2_GB_GOOD_DIE_BIT_FALL  	(0x00U <<7)         /*!< Good-Die Identifier : FALL           */
#define APSXX08L_MR2_GB_GOOD_DIE_BIT_PASS   (0x01U <<7)         /*!< Good-Die Identifier : PASS           */

/* Mode Register 3 R*/
#define APSXX08L_MR3_ADDRESS            	0x00000003U

#define APSXX08L_MR3_SRF_MSK            	0x20U       		/*!< Self Refresh Flag                    */
#define APSXX08L_MR3_SRF_SR           		(0x00U << 5)        /*!< Self Refresh Flag : Slow Refresh (allowed via MR4[3]=1, otherwise Fast Refresh)   */
#define APSXX08L_MR3_SRF_FR             	(0x01U << 5)       	/*!< Self Refresh Flag : Fast Refresh     */

#define APSXX08L_MR3_VCC_MSK            	0x40U       		/*!< Self Refresh Flag                    */
#define APSXX08L_MR3_VCC_1_8				(0x00U << 6)		/*!< Operating Voltage Range : 1.8        */
#define APSXX08L_MR3_VCC_3_0				(0x01U << 6)		/*!< Operating Voltage Range : 3.0        */

#define APSXX08L_MR3_RBXEN_MSK              0x80U       		/*!< Row Boundary Crossing Enable         */
#define APSXX08L_MR3_RBXEN_NO_SUPP          (0x00U << 7)    	/*!< Row Boundary Crossing Enable : RBX not supported          */
#define APSXX08L_MR3_RBXEN_SUPP             (0x01U << 7)      	/*!< Row Boundary Crossing Enable : RBX supported via MR8[3]=1 */

/* Mode Register 4 R/W*/
#define APSXX08L_MR4_ADDRESS            	0x00000004U

#define APSXX08L_MR4_PASR               	0x07U       		/*!< Partially Address Space Refresh     */
#define APSXX08L_MR4_PASR_FULL          	(0x00U << 0)        /*!< PASR : Full Array  		000000h-FFFFFFh 16Mx8 128Mb        */
#define APSXX08L_MR4_PASR_BOTTOM_HALF   	(0x01U << 0)        /*!< PASR : Bottom 1/2 Array    000000h-7FFFFFh 8Mx8  64Mb         */
#define APSXX08L_MR4_PASR_BOTTOM_QUART  	(0x02U << 0)        /*!< PASR : Bottom 1/4 Array    000000h-3FFFFFh 4Mx8  32Mb         */
#define APSXX08L_MR4_PASR_BOTTOM_EIGHT  	(0x03U << 0)        /*!< PASR : Bottom 1/8 Array    000000h-1FFFFFh 2Mx8  16Mb         */
#define APSXX08L_MR4_PASR_NONE          	(0x04U << 0)        /*!< PASR : None                       0        0M    0Mb          */
#define APSXX08L_MR4_PASR_TOP_HALF      	(0x05U << 0)        /*!< PASR : Top 1/2 Array       800000h-FFFFFFh 8Mx8  64Mb         */
#define APSXX08L_MR4_PASR_TOP_QUART     	(0x06U << 0)        /*!< PASR : Top 1/4 Array       C00000h-FFFFFFh 4Mx8  32Mb         */
#define APSXX08L_MR4_PASR_TOP_EIGHT     	(0x07U << 0)        /*!< PASR : Top 1/8 Array       E00000h-FFFFFFh 2Mx8  16Mb         */

#define APSXX08L_MR4_RF_MSK            		0x08U       		/*!< Refresh Frequency           		*/
#define APSXX08L_MR4_RF_FR              	(0x00U << 3)       	/*!< Refresh Frequency : Fast Refresh   */
#define APSXX08L_MR4_RF_SR              	(0x01U << 3)       	/*!< Refresh Frequency : Enables Slow Refresh when temperature allows*/

#define APSXX08L_MR4_WRITE_LATENCY_CODE_MSK	0xE0U       		/*!< Write Latency Code                  */
#define APSXX08L_MR4_WLC_3              	(0x00U << 5)        /*!< Write Latency Code : 3 Fmax (MHz) 66 */
#define APSXX08L_MR4_WLC_4              	(0x04U << 5)        /*!< Write Latency Code : 4 Fmax (MHz) 109*/
#define APSXX08L_MR4_WLC_5              	(0x02U << 5)        /*!< Write Latency Code : 5 Fmax (MHz) 133*/

/* Mode Register 6 W*/
/* May not be supported in this series */
#define APSXX08L_MR6_ADDRESS            	0x00000006U

#define APSXX08L_MR6_HALF_SLEEP_MSK     	0xF0U       		/*!< Half Sleep                          */
#define APSXX08L_MR6_HS_HALF_SPEED_MODE 	(0x0FU << 4)        /*!< Half Sleep : Half Speed Mode        */
#define APSXX08L_MR6_HS_DEEP_POWER_DOWN 	(0x0CU << 4)        /*!< Half Sleep : Deep Power Down Mode   */

/* Mode Register 8 R/W*/
#define APSXX08L_MR8_ADDRESS           		0x00000008U

#define APSXX08L_MR8_BL_MSK             	0x03U       	   /*!< Burst Length                         */
#define APSXX08L_MR8_BL_16_BYTES        	(0x00U << 0)       /*!< Burst Length : 16 Byte/Word Wrap     */
#define APSXX08L_MR8_BL_32_BYTES        	(0x01U << 0)       /*!< Burst Length : 32 Byte/Word Wrap     */
#define APSXX08L_MR8_BL_64_BYTES        	(0x02U << 0)       /*!< Burst Length : 64 Byte/Word Wrap     */
#define APSXX08L_MR8_BL_1K_BYTES        	(0x03U << 0)       /*!< Burst Length : 1K Byte/Word Wrap     */

#define APSXX08L_MR8_BT_MSK                 0x04U       	   /*!< Burst Type                          */
#define APSXX08L_MR8_BT_OFF					(0X00 << 2)		   /*!< Burst Type : Hybrid Wrap OFF		*/
#define APSXX08L_MR8_BT_ON					(0x01 << 2)		   /*!< Burst Type : Hybrid Wrap On			*/

#define APSXX08L_MR8_RBX_MSK            	0x08U              /*!< Row Boundary Crossing Read Enable   */
#define APSXX08L_MR8_RBX_OFF				(0X00 << 3)		   /*!< Row Boundary Crossing Read Enable : Reads stay within the 1K column address space*/
#define APSXX08L_MR8_RBX_ON					(0x01 << 3)		   /*!< Row Boundary Crossing Read Enable : Reads cross row at 1K boundaries*/

/******************************************************************************
  * @brief  APSXX08L Commands
  ****************************************************************************/

/* Read Operations */
#define APSXX08L_READ_CMD               	0x0000U       	/*!< Synchronous Read                    */
#define APSXX08L_READ_LINEAR_BURST_CMD  	0x2020U        	/*!< Linear Burst Read                   */

/* Write Operations */
#define APSXX08L_WRITE_CMD              	0x8080U        	/*!< Synchronous Write                   */
#define APSXX08L_WRITE_LINEAR_BURST_CMD 	0xA0A0U        	/*!< Linear Burst Write                  */

/* Reset Operations */
#define APSXX08L_RESET_CMD              	0xFFU       	/*!< Global Reset                        */

/* Register Operations */
#define APSXX08L_READ_REG_CMD           	0x40U       	/*!< Mode Register Read                  */
#define APSXX08L_WRITE_REG_CMD          	0xC0U       	/*!< Mode Register Write                 */
