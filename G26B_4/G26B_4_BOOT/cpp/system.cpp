//#pragma O3
//#pragma Ospace

#include "types.h"
#include "core.h"

#include "system.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_SAME53	

	// Test Pins
	// 3	- PC00	- EN_VCORE
	// 4	- PC01	- FCS3
	// 5	- PC02	- FCS4
	// 6	- PC03
	// 14	- PB07
	// 15	- PB08
	// 16	- PB09
	// 28	- PA10
	// 29	- PA11
	// 32	- PB10
	// 33	- PB11
	// 40	- PC10
	// 41	- PC11
	// 42	- PC12
	// 43	- PC13
	// 44	- PC14
	// 52	- PA16
	// 56	- PC16	- PHY_INT
	// 57	- PC17
	// 59	- PC19
	// 60	- PC20	- GRXDV
	// 61	- PC21
	// 64	- PB16
	// 65	- PB17	- GEN_1M
	// 66	- PB18	- ManRcvIRQ sync true
	// 68	- PB20	- I2C_Handler	
	// 69	- PB21	- ManTrmIRQ
	// 74	- PA24	- ManRcvIRQ
	// 75	- PA25	- main loop
	// 80	- PB24
	// 81	- PB25
	// 82	- PC24
	// 83	- PC25
	// 84	- PC26	- CRC_CCITT_DMA
	// 85	- PC27	- MEM USART READ
	// 86	- PC28	- MEM USART RTS	
	// 87	- PA27
	// 97	- PB00	- nc



	#define GEN_MCK		0
	#define GEN_32K		1
	#define GEN_25M		2
	#define GEN_1M		3
	//#define GEN_500K	4

	#define	NAND_DMACH				0
	#define	COM1_DMACH				1
	#define	COM2_DMACH				2
	#define	COM3_DMACH				3
	#define	NAND_MEMCOPY_DMACH		30
	#define	CRC_DMACH				31

	#define I2C			HW::I2C3
	#define PIO_I2C		HW::PIOA 
	#define PIN_SDA		22 
	#define PIN_SCL		23 
	#define SDA			(1<<PIN_SDA) 
	#define SCL			(1<<PIN_SCL) 

	__align(16) T_HW::DMADESC DmaTable[32];
	__align(16) T_HW::DMADESC DmaWRB[32];

	#define EVENT_NAND_1	0
	#define EVENT_NAND_2	1
	#define EVENT_NAND_3	2
	#define EVENT_MANR_1	3
	#define EVENT_MANR_2	4

	#define nandTC			HW::TC0
	#define nandTCC			HW::TCC0
	#define ManRT			HW::TCC2
	#define ManIT			HW::TCC3
	#define ManTT			HW::TCC4
	#define MltTmr			HW::TCC4
	#define MT(v)			(v)
	#define BOUD2CLK(x)		((u32)(1000000/x+0.5))

	#define MANT_IRQ		TCC4_0_IRQ
	#define MANR_IRQ		TCC2_1_IRQ

	#define PIO_MANCH		HW::PIOB
	#define PIN_MEMTX1		31 
	#define PIN_MEMTX2		30 
	#define PIN_MANCHRX		19 

	#define MEMTX1			(1UL<<PIN_MEMTX1)
	#define MEMTX2			(1UL<<PIN_MEMTX2)
	#define MANCHRX			(1UL<<PIN_MANCHRX) 

	//#define ManRxd()		((PIO_MANCH->IN >> PIN_MANCHRX) & 1)

	#define Pin_ManRcvIRQ_Set()	HW::PIOA->BSET(24)
	#define Pin_ManRcvIRQ_Clr()	HW::PIOA->BCLR(24)

	#define Pin_ManTrmIRQ_Set()	HW::PIOB->BSET(21)		
	#define Pin_ManTrmIRQ_Clr()	HW::PIOB->BCLR(21)		

	#define Pin_ManRcvSync_Set()	HW::PIOB->BSET(18)		
	#define Pin_ManRcvSync_Clr()	HW::PIOB->BCLR(18)		

	#define PIO_NAND_DATA	HW::PIOA
	#define PIO_WP			HW::PIOB 
	#define PIO_ALE			HW::PIOB 
	#define PIO_CLE			HW::PIOB 
	#define PIO_WE_RE		HW::PIOA 
	#define PIO_RB			HW::PIOC
	#define PIO_FCS			HW::PIOC

	#define PIN_WE			8 
	#define PIN_RE			9 

	#define WE				(1<<PIN_WE) 
	#define RE				(1<<PIN_RE) 
	#define ALE				(1<<4) 
	#define CLE				(1<<5) 
	#define WP				(1<<6) 
	#define FCS1			(1<<5) 
	#define FCS2			(1<<6) 
	#define RB				(1<<7) 

	#define PIO_MEM_USART	HW::PIOB 
	#define PIN_MEM_TXD		1 
	#define PIN_MEM_RXD		2 
	#define PIN_MEM_SCK		3 
	#define MEM_TXD			(1<<PIN_MEM_TXD) 
	#define MEM_RXD			(1<<PIN_MEM_RXD) 
	#define MEM_SCK			(1<<PIN_MEM_SCK) 

#elif defined(CPU_XMC48) //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	// Test Pins
	// 1	- P0.1	- main loop
	// 2	- P0.0
	// 3	- P0.10
	// 4	- P0.9
	// 5	- P3.2
	// 40	- P2.15
	// 41	- P2.14
	// 55	- P5.7	- I2C_Handler
	// 57	- P5.1	- ManRcvIRQ
	// 70	- P1.13
	// 71	- P1.12
	// 72	- P1.11
	// 76	- P1.3	- ManTrmIRQ
	// 77	- P1.2	- MEM USART RTS	
	// 79	- P1.0	- MEM USART READ	
	// 80	- P1.9	- CRC_CCITT_DMA
	// 92	- P3.4
	// 94	- P0.11
	// 95	- P0.12
	// 96	- P0.6	- nc



	#define	NAND_DMA		HW::GPDMA0
	#define	NAND_DMACH		HW::GPDMA0_CH7
	#define	NAND_DMA_CHEN	(0x101<<7)
	#define	NAND_DMA_CHST	(1<<7)

	#define	CRC_DMA			HW::GPDMA1
	#define	CRC_DMACH		HW::GPDMA1_CH2
	#define	CRC_DMA_CHEN	(0x101<<2)
	#define	CRC_FCE			HW::FCE_KE3

	#define I2C				HW::USIC2_CH0
	#define PIO_I2C			HW::P5
	#define PIN_SDA			0 
	#define PIN_SCL			2 
	#define SDA				(1<<PIN_SDA) 
	#define SCL				(1<<PIN_SCL) 
	#define I2C_IRQ			USIC2_0_IRQn
	#define I2C_PID			PID_USIC2

	#define ManRT			HW::CCU40_CC43
	#define ManTT			HW::CCU40_CC40
	#define ManCCU			HW::CCU40
	#define ManCCU_PID		PID_CCU40
	#define ManTmr			HW::CCU40_CC41
	#define MT(v)			((u16)((MCK_MHz*(v)+64)/128))
	#define BOUD2CLK(x)		((u32)((MCK/8.0)/x+0.5))

	#define MANT_IRQ		CCU40_0_IRQn
	#define MANR_IRQ		CCU40_2_IRQn

	#define PIO_MANCH		HW::P1
	#define PIN_MEMTX1		7 
	#define PIN_MEMTX2		8 
//	#define PIN_MANCHRX		19 

	#define MEMTX1			(1UL<<PIN_MEMTX1)
	#define MEMTX2			(1UL<<PIN_MEMTX2)
//	#define MANCHRX			(1UL<<PIN_MANCHRX) 

	//#define ManRxd()		((PIO_MANCH->IN >> PIN_MANCHRX) & 1)

	#define Pin_ManRcvIRQ_Set()		HW::P5->BSET(1);
	#define Pin_ManRcvIRQ_Clr()		HW::P5->BCLR(1);

	#define Pin_ManTrmIRQ_Set()		HW::P1->SET(1<<3);
	#define Pin_ManTrmIRQ_Clr()		HW::P1->CLR(1<<3);

	#define Pin_ManRcvSync_Set()			
	#define Pin_ManRcvSync_Clr()			

	#define PIO_WP			HW::P3 
	#define PIO_RB			HW::P1
	#define PIO_FCS			HW::P4

	#define PIN_WP			3 
	#define PIN_FCS1		1 
	#define PIN_FCS2		0 
	#define PIN_RB			6 

	#define WP				(1<<PIN_WP) 
	#define FCS1			(1<<PIN_FCS1) 
	#define FCS2			(1<<PIN_FCS2) 
	#define RB				(1<<PIN_RB) 

	// SDA_0_0 P1.5
	// SCL_0_0 P0.8 P1.1
	 
	// SDA_0_1 P2.5 P3.13
	// SCL_0_1 P2.4 P3.0 P6.2

	// SDA_1_0 P0.5 P2.14
	// SCL_1_0 P0.11 P5.8

	// SDA_1_1 P3.15 P4.2
	// SCL_1_1 P0.10 P0.13

	// SDA_2_0 P5.0
	// SCL_2_0 P5.2 

	// SDA_2_1 P3.5 
	// SCL_2_1 P4.2 P3.6

	//ARM_IHP VectorTableInt[16] __attribute__((at(0x1FFE8000)));;
	//ARM_IHP VectorTableExt[112] __attribute__((at(0x1FFE8040)));;

	//ARM_IHP * const VectorTableInt = (ARM_IHP*)0x1FFE8000;
	//ARM_IHP * const VectorTableExt = (ARM_IHP*)0x1FFE8040;

	/*******************************************************************************
	 * MACROS
	 *******************************************************************************/
	#define	OFI_FREQUENCY        (24000000UL)  /**< 24MHz Backup Clock (fOFI) frequency. */
	#define OSI_FREQUENCY        (32768UL)    /**< 32KHz Internal Slow Clock source (fOSI) frequency. */  

	#define XMC4800_F144x2048

	#define CHIPID_LOC ((uint8_t *)0x20000000UL)

	#define PMU_FLASH_WS          (NS2CLK(30))	//(0x3U)

	#define OSCHP_FREQUENCY			(25000000U)
	#define FOSCREF					(2500000U)
	#define VCO_NOM					(400000000UL)
	#define VCO_IN_MAX				(5000000UL)

	#define DELAY_CNT_50US_50MHZ  (2500UL)
	#define DELAY_CNT_150US_50MHZ (7500UL)
	#define DELAY_CNT_50US_48MHZ  (2400UL)
	#define DELAY_CNT_50US_72MHZ  (3600UL)
	#define DELAY_CNT_50US_96MHZ  (4800UL)
	#define DELAY_CNT_50US_120MHZ (6000UL)
	#define DELAY_CNT_50US_144MHZ (7200UL)

	#define SCU_PLL_PLLSTAT_OSC_USABLE  (SCU_PLL_PLLSTAT_PLLHV_Msk | \
										 SCU_PLL_PLLSTAT_PLLLV_Msk | \
										 SCU_PLL_PLLSTAT_PLLSP_Msk)


	#define USB_PDIV (4U)
	#define USB_NDIV (79U)


	/*
	//    <o> Backup clock calibration mode
	//       <0=> Factory calibration
	//       <1=> Automatic calibration
	//    <i> Default: Automatic calibration
	*/
	#define FOFI_CALIBRATION_MODE 1
	#define FOFI_CALIBRATION_MODE_FACTORY 0
	#define FOFI_CALIBRATION_MODE_AUTOMATIC 1

	/*
	//    <o> Standby clock (fSTDBY) source selection
	//       <0=> Internal slow oscillator (32768Hz)
	//       <1=> External crystal (32768Hz)
	//    <i> Default: Internal slow oscillator (32768Hz)
	*/
	#define STDBY_CLOCK_SRC 0
	#define STDBY_CLOCK_SRC_OSI 0
	#define STDBY_CLOCK_SRC_OSCULP 1

	/*
	//    <o> PLL clock source selection
	//       <0=> External crystal
	//       <1=> Internal fast oscillator
	//    <i> Default: External crystal
	*/
	#define PLL_CLOCK_SRC 0
	#define PLL_CLOCK_SRC_EXT_XTAL 0
	#define PLL_CLOCK_SRC_OFI 1

	#define PLL_CON1(ndiv, k2div, pdiv) (((ndiv) << SCU_PLL_PLLCON1_NDIV_Pos) | ((k2div) << SCU_PLL_PLLCON1_K2DIV_Pos) | ((pdiv) << SCU_PLL_PLLCON1_PDIV_Pos))

	/* PLL settings, fPLL = 288MHz */
	#if PLL_CLOCK_SRC == PLL_CLOCK_SRC_EXT_XTAL

		#define PLL_K2DIV	((VCO_NOM/MCK)-1)
		#define PLL_PDIV	(((OSCHP_FREQUENCY-VCO_IN_MAX)*2/VCO_IN_MAX+1)/2)
		#define PLL_NDIV	((MCK*(PLL_K2DIV+1)*2/(OSCHP_FREQUENCY/(PLL_PDIV+1))+1)/2-1) // (7U) 

		#define VCO ((OSCHP_FREQUENCY / (PLL_PDIV + 1UL)) * (PLL_NDIV + 1UL))

	#else /* PLL_CLOCK_SRC == PLL_CLOCK_SRC_EXT_XTAL */

		#define PLL_PDIV (1U)
		#define PLL_NDIV (23U)
		#define PLL_K2DIV (0U)

		#define VCO ((OFI_FREQUENCY / (PLL_PDIV + 1UL)) * (PLL_NDIV + 1UL))

	#endif /* PLL_CLOCK_SRC == PLL_CLOCK_SRC_OFI */

	#define PLL_K2DIV_24MHZ  ((VCO / OFI_FREQUENCY) - 1UL)
	#define PLL_K2DIV_48MHZ  ((VCO / 48000000U) - 1UL)
	#define PLL_K2DIV_72MHZ  ((VCO / 72000000U) - 1UL)
	#define PLL_K2DIV_96MHZ  ((VCO / 96000000U) - 1UL)
	#define PLL_K2DIV_120MHZ ((VCO / 120000000U) - 1UL)

	#define SCU_CLK_CLKCLR_ENABLE_USBCLK SCU_CLK_CLKCLR_USBCDI_Msk
	#define SCU_CLK_CLKCLR_ENABLE_MMCCLK SCU_CLK_CLKCLR_MMCCDI_Msk
	#define SCU_CLK_CLKCLR_ENABLE_ETHCLK SCU_CLK_CLKCLR_ETH0CDI_Msk
	#define SCU_CLK_CLKCLR_ENABLE_EBUCLK SCU_CLK_CLKCLR_EBUCDI_Msk
	#define SCU_CLK_CLKCLR_ENABLE_CCUCLK SCU_CLK_CLKCLR_CCUCDI_Msk

	#define SCU_CLK_SYSCLKCR_SYSSEL_OFI      (0U << SCU_CLK_SYSCLKCR_SYSSEL_Pos)
	#define SCU_CLK_SYSCLKCR_SYSSEL_PLL      (1U << SCU_CLK_SYSCLKCR_SYSSEL_Pos)

	#define SCU_CLK_USBCLKCR_USBSEL_USBPLL   (0U << SCU_CLK_USBCLKCR_USBSEL_Pos)
	#define SCU_CLK_USBCLKCR_USBSEL_PLL      (1U << SCU_CLK_USBCLKCR_USBSEL_Pos)

	#define SCU_CLK_ECATCLKCR_ECATSEL_USBPLL (0U << SCU_CLK_ECATCLKCR_ECATSEL_Pos)
	#define SCU_CLK_ECATCLKCR_ECATSEL_PLL    (1U << SCU_CLK_ECATCLKCR_ECATSEL_Pos)

	#define SCU_CLK_WDTCLKCR_WDTSEL_OFI      (0U << SCU_CLK_WDTCLKCR_WDTSEL_Pos)
	#define SCU_CLK_WDTCLKCR_WDTSEL_STANDBY  (1U << SCU_CLK_WDTCLKCR_WDTSEL_Pos)
	#define SCU_CLK_WDTCLKCR_WDTSEL_PLL      (2U << SCU_CLK_WDTCLKCR_WDTSEL_Pos)

	#define SCU_CLK_EXTCLKCR_ECKSEL_SYS      (0U << SCU_CLK_EXTCLKCR_ECKSEL_Pos)
	#define SCU_CLK_EXTCLKCR_ECKSEL_USBPLL   (2U << SCU_CLK_EXTCLKCR_ECKSEL_Pos)
	#define SCU_CLK_EXTCLKCR_ECKSEL_PLL      (3U << SCU_CLK_EXTCLKCR_ECKSEL_Pos)

	#define EXTCLK_PIN_P0_8  (1)
	#define EXTCLK_PIN_P1_15 (2)

	#define __CLKSET    (0x00000000UL)
	#define __SYSCLKCR  (0x00010000UL)
	#define __CPUCLKCR  (0x00000000UL)
	#define __PBCLKCR   (0x00000000UL)
	#define __CCUCLKCR  (0x00000000UL)
	#define __WDTCLKCR  (0x00000000UL)
	#define __EBUCLKCR  (0x00000003UL)
	#define __USBCLKCR  (0x00010005UL)
	#define __ECATCLKCR (0x00000001UL)

	#define __EXTCLKCR (0x01200003UL)
	#define __EXTCLKPIN (0U)

	#define ENABLE_PLL \
		(((__SYSCLKCR & SCU_CLK_SYSCLKCR_SYSSEL_Msk) == SCU_CLK_SYSCLKCR_SYSSEL_PLL) || \
		 ((__ECATCLKCR & SCU_CLK_ECATCLKCR_ECATSEL_Msk) == SCU_CLK_ECATCLKCR_ECATSEL_PLL) || \
		 ((__CLKSET & SCU_CLK_CLKSET_EBUCEN_Msk) != 0) || \
		 (((__CLKSET & SCU_CLK_CLKSET_USBCEN_Msk) != 0) && ((__USBCLKCR & SCU_CLK_USBCLKCR_USBSEL_Msk) == SCU_CLK_USBCLKCR_USBSEL_PLL)) || \
		 (((__CLKSET & SCU_CLK_CLKSET_WDTCEN_Msk) != 0) && ((__WDTCLKCR & SCU_CLK_WDTCLKCR_WDTSEL_Msk) == SCU_CLK_WDTCLKCR_WDTSEL_PLL)))

	#define ENABLE_USBPLL \
		(((__ECATCLKCR & SCU_CLK_ECATCLKCR_ECATSEL_Msk) == SCU_CLK_ECATCLKCR_ECATSEL_USBPLL) || \
		 (((__CLKSET & SCU_CLK_CLKSET_USBCEN_Msk) != 0) && ((__USBCLKCR & SCU_CLK_USBCLKCR_USBSEL_Msk) == SCU_CLK_USBCLKCR_USBSEL_USBPLL)) || \
		 (((__CLKSET & SCU_CLK_CLKSET_MMCCEN_Msk) != 0) && ((__USBCLKCR & SCU_CLK_USBCLKCR_USBSEL_Msk) == SCU_CLK_USBCLKCR_USBSEL_USBPLL)))

	#define SLAD(v)		((v)&0xffff)                /*!< USIC_CH PCR_IICMode: SLAD (Bitfield-Mask: 0xffff)           */
	#define ACK00     	(1<<16UL)                    /*!< USIC_CH PCR_IICMode: ACK00 (Bit 16)                         */
	#define STIM      	(1<<17UL)                    /*!< USIC_CH PCR_IICMode: STIM (Bit 17)                          */
	#define SCRIEN    	(1<<18UL)                    /*!< USIC_CH PCR_IICMode: SCRIEN (Bit 18)                        */
	#define RSCRIEN   	(1<<19UL)                    /*!< USIC_CH PCR_IICMode: RSCRIEN (Bit 19)                       */
	#define PCRIEN    	(1<<20UL)                    /*!< USIC_CH PCR_IICMode: PCRIEN (Bit 20)                        */
	#define NACKIEN   	(1<<21UL)                    /*!< USIC_CH PCR_IICMode: NACKIEN (Bit 21)                       */
	#define ARLIEN    	(1<<22UL)                    /*!< USIC_CH PCR_IICMode: ARLIEN (Bit 22)                        */
	#define SRRIEN    	(1<<23UL)                    /*!< USIC_CH PCR_IICMode: SRRIEN (Bit 23)                        */
	#define ERRIEN    	(1<<24UL)                    /*!< USIC_CH PCR_IICMode: ERRIEN (Bit 24)                        */
	#define SACKDIS   	(1<<25UL)                    /*!< USIC_CH PCR_IICMode: SACKDIS (Bit 25)                       */
	#define HDEL(v)		(((v)&0xF)<<26UL)                    /*!< USIC_CH PCR_IICMode: HDEL (Bit 26)                          */
	#define ACKIEN    	(1<<30UL)                    /*!< USIC_CH PCR_IICMode: ACKIEN (Bit 30)                        */
	//#define MCLK      	(1<<31UL)                    /*!< USIC_CH PCR_IICMode: MCLK (Bit 31)                          */

	#define SLSEL         (0x1UL)                   /*!< USIC_CH PSR_IICMode: SLSEL (Bitfield-Mask: 0x01)            */
	#define WTDF          (0x2UL)                   /*!< USIC_CH PSR_IICMode: WTDF (Bitfield-Mask: 0x01)             */
	#define SCR           (0x4UL)                   /*!< USIC_CH PSR_IICMode: SCR (Bitfield-Mask: 0x01)              */
	#define RSCR          (0x8UL)                   /*!< USIC_CH PSR_IICMode: RSCR (Bitfield-Mask: 0x01)             */
	#define PCR           (0x10UL)                  /*!< USIC_CH PSR_IICMode: PCR (Bitfield-Mask: 0x01)              */
	#define NACK          (0x20UL)                  /*!< USIC_CH PSR_IICMode: NACK (Bitfield-Mask: 0x01)             */
	#define ARL           (0x40UL)                  /*!< USIC_CH PSR_IICMode: ARL (Bitfield-Mask: 0x01)              */
	#define SRR           (0x80UL)                  /*!< USIC_CH PSR_IICMode: SRR (Bitfield-Mask: 0x01)              */
	#define ERR           (0x100UL)                 /*!< USIC_CH PSR_IICMode: ERR (Bitfield-Mask: 0x01)              */
	#define ACK           (0x200UL)                 /*!< USIC_CH PSR_IICMode: ACK (Bitfield-Mask: 0x01)              */
	#define RSIF          (0x400UL)                 /*!< USIC_CH PSR_IICMode: RSIF (Bitfield-Mask: 0x01)             */
	#define DLIF          (0x800UL)                 /*!< USIC_CH PSR_IICMode: DLIF (Bitfield-Mask: 0x01)             */
	#define TSIF          (0x1000UL)                /*!< USIC_CH PSR_IICMode: TSIF (Bitfield-Mask: 0x01)             */
	#define TBIF          (0x2000UL)                /*!< USIC_CH PSR_IICMode: TBIF (Bitfield-Mask: 0x01)             */
	#define RIF           (0x4000UL)                /*!< USIC_CH PSR_IICMode: RIF (Bitfield-Mask: 0x01)              */
	#define AIF           (0x8000UL)                /*!< USIC_CH PSR_IICMode: AIF (Bitfield-Mask: 0x01)              */
	#define BRGIF         (0x10000UL)               /*!< USIC_CH PSR_IICMode: BRGIF (Bitfield-Mask: 0x01)            */

	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define TDF_MASTER_SEND				(0U << 8U)
	#define TDF_SLAVE_SEND				(1U << 8U)
	#define TDF_MASTER_RECEIVE_ACK   	(2U << 8U)
	#define TDF_MASTER_RECEIVE_NACK  	(3U << 8U)
	#define TDF_MASTER_START         	(4U << 8U)
	#define TDF_MASTER_RESTART      	(5U << 8U)
	#define TDF_MASTER_STOP         	(6U << 8U)

	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define __SCTR (SDIR(1) | TRM(3) | FLE(0x3F) | WLE(7))

	#define __CCR (MODE(4))

	#define __BRG (DCTQ(24))

	#define __DX0CR (DSEL(1) | INSW(0) | DFEN(1) | DSEN(1) | DPOL(0) | SFSEL(1) | CM(0) | DXS(0))
	#define __DX1CR (DSEL(0) | INSW(0) | DFEN(1) | DSEN(1) | DPOL(0) | SFSEL(1) | CM(0) | DXS(0))
	#define __DX2CR (DSEL(0) | INSW(0) | DFEN(0) | DSEN(0) | DPOL(0) | SFSEL(0) | CM(0) | DXS(0))
	#define __DX3CR (DSEL(0) | INSW(0) | DFEN(0) | DSEN(0) | DPOL(0) | SFSEL(0) | CM(0) | DXS(0))

	#define __PCR (STIM)

	#define __FDR ((1024 - (((MCK + 400000/2) / 400000 + 8) / 16)) | DM(1))

	#define __TCSR (TDEN(1)|TDSSM(1))

	static void delay(u32 cycles) { for(volatile u32 i = 0UL; i < cycles ;++i) { __nop(); }}

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern "C" void SystemInit()
{
	//u32 i;
	using namespace CM4;
	using namespace HW;

//	__breakpoint(0);

	#ifdef CPU_SAME53	

		HW::PIOA->DIRSET = (1<<25)|(1<<24)|(1<<21);
		HW::PIOA->CLR((1<<25)|(1<<24)|(1<<21));

		//PIO_MEM_USART->SetWRCONFIG(MEM_TXD|MEM_RXD|MEM_SCK, PORT_PMUX(3)|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX|PORT_PULLEN);

		HW::PIOB->DIRSET = (1<<18)|(1<<20)|(1<<21);
		//HW::PIOB->SetWRCONFIG((1<<17), PORT_PMUX(11)|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX;

		HW::PIOC->DIRSET = (1<<15)|(1<<28)|(1<<27)|(1<<26)|(1<<25);
		HW::PIOC->SET((1<<15)/*|(1<<0)*/);

		HW::PIOA->BSET(25);

		HW::SUPC->BOD33 = 0;
		HW::SUPC->BOD33 = BOD33_HYST(15) | BOD33_ACTION_INT| BOD33_LEVEL(200); // Vbod = 1.5 + 6mV * BOD33_LEVEL
		HW::SUPC->BOD33 |= BOD33_ENABLE;

		HW::PIOA->BCLR(25);

		while ((HW::SUPC->STATUS & (SUPC_BOD33RDY|SUPC_B33SRDY)) != (SUPC_BOD33RDY|SUPC_B33SRDY));
		while ((HW::SUPC->STATUS & SUPC_BOD33DET) != 0);

		HW::PIOA->BSET(25);

		HW::SUPC->BOD33 = 0;
		HW::SUPC->BOD33 = BOD33_HYST(15) | BOD33_ACTION_RESET | BOD33_LEVEL(200); // Vbod = 1.5 + 6mV * BOD33_LEVEL
		HW::SUPC->BOD33 |= BOD33_ENABLE;

		HW::PIOA->BCLR(25);

		OSCCTRL->XOSC[1] = XOSC_ENABLE|XOSC_ONDEMAND; // RUNSTDBY|ENABLE

		OSCCTRL->DPLL[0].CTRLA = 0; while ((OSCCTRL->DPLL[0].SYNCBUSY & DPLLSYNCBUSY_ENABLE) != 0);

		OSCCTRL->DPLL[0].CTRLB = DPLL_REFCLK_XOSC1|DPLL_DIV(24);	// 0x70010; // XOSC clock source division factor 50 = 2*(DIV+1), XOSC clock reference
		OSCCTRL->DPLL[0].RATIO = DPLL_LDR((MCK*2+500000)/1000000-1)|DPLL_LDRFRAC(0);	// 47; // Loop Divider Ratio = 200, Loop Divider Ratio Fractional Part = 0

		OSCCTRL->DPLL[0].CTRLA = DPLL_ONDEMAND|DPLL_ENABLE; 

		HW::GCLK->GENCTRL[GEN_MCK] = GCLK_DIV(0)|GCLK_SRC_DPLL0|GCLK_GENEN;

		HW::MCLK->AHBMASK |= AHB_DMAC;
		HW::DMAC->CTRL = 0;
		HW::DMAC->CTRL = DMAC_SWRST;
		HW::DMAC->DBGCTRL = DMAC_DBGRUN;
		HW::DMAC->BASEADDR	= DmaTable;
		HW::DMAC->WRBADDR	= DmaWRB;
		HW::DMAC->CTRL = DMAC_DMAENABLE|DMAC_LVLEN0|DMAC_LVLEN1|DMAC_LVLEN2|DMAC_LVLEN3;

		if ((CMCC->SR & CMCC_CSTS) == 0)
		{
			CMCC->CTRL = CMCC_CEN;
		};

	#elif defined(CPU_XMC48)

		__disable_irq();

//		__DSB();
		__enable_irq();

		HW::FLASH0->FCON = FLASH_FCON_IDLE_Msk | PMU_FLASH_WS;

		/* enable PLL */
		SCU_PLL->PLLCON0 &= ~(SCU_PLL_PLLCON0_VCOPWD_Msk | SCU_PLL_PLLCON0_PLLPWD_Msk);

		SCU_OSC->OSCHPCTRL = OSC_MODE(2) | OSC_OSCVAL(OSCHP_FREQUENCY / FOSCREF - 1UL);

			/* select OSC_HP clock as PLL input */
			SCU_PLL->PLLCON2 = 0;

			/* restart OSC Watchdog */
			SCU_PLL->PLLCON0 &= ~SCU_PLL_PLLCON0_OSCRES_Msk;

			while ((SCU_PLL->PLLSTAT & SCU_PLL_PLLSTAT_OSC_USABLE) != SCU_PLL_PLLSTAT_OSC_USABLE);
		//};

		/* Go to bypass the Main PLL */
		SCU_PLL->PLLCON0 |= SCU_PLL_PLLCON0_VCOBYP_Msk;

		/* disconnect Oscillator from PLL */
		SCU_PLL->PLLCON0 |= SCU_PLL_PLLCON0_FINDIS_Msk;

		/* Setup divider settings for main PLL */
		SCU_PLL->PLLCON1 =  PLL_CON1(PLL_NDIV, PLL_K2DIV_24MHZ, PLL_PDIV);

		/* Set OSCDISCDIS */
		SCU_PLL->PLLCON0 |= SCU_PLL_PLLCON0_OSCDISCDIS_Msk;

		/* connect Oscillator to PLL */
		SCU_PLL->PLLCON0 &= ~SCU_PLL_PLLCON0_FINDIS_Msk;

		/* restart PLL Lock detection */
		SCU_PLL->PLLCON0 |= SCU_PLL_PLLCON0_RESLD_Msk;	while ((SCU_PLL->PLLSTAT & SCU_PLL_PLLSTAT_VCOLOCK_Msk) == 0U);

		/* Disable bypass- put PLL clock back */
		SCU_PLL->PLLCON0 &= ~SCU_PLL_PLLCON0_VCOBYP_Msk;	while ((SCU_PLL->PLLSTAT & SCU_PLL_PLLSTAT_VCOBYST_Msk) != 0U);

		/* Before scaling to final frequency we need to setup the clock dividers */
		SCU_CLK->SYSCLKCR = __SYSCLKCR;
		SCU_CLK->PBCLKCR = __PBCLKCR;
		SCU_CLK->CPUCLKCR = __CPUCLKCR;
		SCU_CLK->CCUCLKCR = __CCUCLKCR;
		SCU_CLK->WDTCLKCR = __WDTCLKCR;
		SCU_CLK->EBUCLKCR = __EBUCLKCR;
		SCU_CLK->USBCLKCR = __USBCLKCR;
		SCU_CLK->ECATCLKCR = __ECATCLKCR;
		SCU_CLK->EXTCLKCR = __EXTCLKCR;

		/* PLL frequency stepping...*/
		/* Reset OSCDISCDIS */
		SCU_PLL->PLLCON0 &= ~SCU_PLL_PLLCON0_OSCDISCDIS_Msk;

		SCU_PLL->PLLCON1 = PLL_CON1(PLL_NDIV, PLL_K2DIV_48MHZ, PLL_PDIV);	delay(DELAY_CNT_50US_48MHZ);

		SCU_PLL->PLLCON1 = PLL_CON1(PLL_NDIV, PLL_K2DIV_72MHZ, PLL_PDIV);	delay(DELAY_CNT_50US_72MHZ);

		SCU_PLL->PLLCON1 = PLL_CON1(PLL_NDIV, PLL_K2DIV_96MHZ, PLL_PDIV);	delay(DELAY_CNT_50US_96MHZ);

		SCU_PLL->PLLCON1 = PLL_CON1(PLL_NDIV, PLL_K2DIV_120MHZ, PLL_PDIV);	delay(DELAY_CNT_50US_120MHZ);

		SCU_PLL->PLLCON1 = PLL_CON1(PLL_NDIV, PLL_K2DIV, PLL_PDIV);			delay(DELAY_CNT_50US_144MHZ);

		/* Enable selected clocks */
		SCU_CLK->CLKSET = __CLKSET;

		//P2->ModePin10(	G_PP	);
		//P2->BSET(10);

		P0->ModePin0(	G_PP	);
		P0->ModePin1(	G_PP	);
		P0->ModePin2(	HWIO1	);
		P0->ModePin3(	HWIO1	);
		P0->ModePin4(	HWIO1	);
		P0->ModePin5(	HWIO1	);
		P0->ModePin6(	G_PP	);
		P0->ModePin7(	HWIO1	);
		P0->ModePin8(	HWIO1	);
		P0->ModePin9(	G_PP	);
		P0->ModePin10(	G_PP	);
		P0->ModePin11(	G_PP	);
		P0->ModePin12(	G_PP	);

		P0->PPS = 0;

		P1->ModePin0(	G_PP	);
		P1->ModePin1(	I2DPU	);
		P1->ModePin2(	G_PP	);
		P1->ModePin3(	G_PP	);
		P1->ModePin4(	I2DPU	);
		P1->ModePin5(	A2PP	);
		P1->ModePin6(	I2DPU	);
		P1->ModePin7(	G_PP	);
		P1->ModePin8(	G_PP	);
		P1->ModePin9(	G_PP	);
		P1->ModePin10(	I2DPU	);
		P1->ModePin11(	G_PP	);
		P1->ModePin12(	G_PP	);
		P1->ModePin13(	G_PP	);
		P1->ModePin14(	HWIO1	);
		P1->ModePin15(	HWIO1	);

		P1->PPS = 0;

		P2->ModePin0(	HWIO0	);
		P2->ModePin1(	I1DPD	);
		P2->ModePin2(	I2DPU	);
		P2->ModePin3(	I1DPD	);
		P2->ModePin4(	I1DPD	);
		P2->ModePin5(	A1PP	);
		P2->ModePin6(	I2DPU	);
		P2->ModePin7(	A1PP	);
		P2->ModePin8(	A1PP	);
		P2->ModePin9(	A1PP	);
		P2->ModePin10(	G_PP	);
		P2->ModePin14(	G_PP	);
		P2->ModePin15(	G_PP	);

		P2->PPS = 0;

		P3->ModePin0(	HWIO1	);
		P3->ModePin1(	HWIO1	);
		P3->ModePin2(	G_PP	);
		P3->ModePin3(	G_PP	);
		P3->ModePin4(	G_PP	);
		P3->ModePin5(	HWIO1	);
		P3->ModePin6(	HWIO1	);

		P3->PPS = 0;

		P4->ModePin0(	G_PP	);
		P4->ModePin1(	G_PP	);

		P4->PPS = 0;

		P5->ModePin0(	HWIO0	);
		P5->ModePin1(	G_PP	);
		P5->ModePin2(	A1OD	);
		P5->ModePin7(	G_PP	);

		P5->PPS = 0;

		P14->ModePin0(	I0DNP	);
		P14->ModePin1(	I2DPU	);
		P14->ModePin2(	I2DPU	);
		P14->ModePin3(	I2DPU	);
		P14->ModePin4(	I2DPU	);
		P14->ModePin5(	I2DPU	);
		P14->ModePin6(	I2DPU	);
		P14->ModePin7(	I2DPU	);
		P14->ModePin8(	I2DPU	);
		P14->ModePin9(	I2DPU	);
		P14->ModePin12(	I2DPU	);
		P14->ModePin13(	I2DPU	);
		P14->ModePin14(	I2DPU	);
		P14->ModePin15(	I2DPU	);

		P14->PPS = 0;
		P14->PDISC = (1<<0);

		P15->ModePin2(	I2DPU	);
		P15->ModePin3(	I2DPU	);
		P15->ModePin8(	I2DPU	);
		P15->ModePin9(	I1DPD	);

		P15->PPS = 0;
		P15->PDISC = 0;

		HW::Peripheral_Enable(PID_DMA0);
		HW::Peripheral_Enable(PID_DMA1);

		//HW::DLR->SRSEL0 = SRSEL0(10,11,0,0,0,0,0,0);
		//HW::DLR->SRSEL1 = SRSEL1(0,0,0,0);

		HW::DLR->DRL0 = DRL0_USIC0_SR0;
		HW::DLR->DRL1 = DRL1_USIC1_SR0;

		HW::DLR->LNEN |= 3;

	#endif

	#if ((__FPU_PRESENT == 1) && (__FPU_USED == 1))
		CM4::SCB->CPACR |= ((3UL << 10*2) |                 /* set CP10 Full Access */
							(3UL << 11*2)  );               /* set CP11 Full Access */
	#else
		CM4::SCB->CPACR = 0;
	#endif

  /* Enable unaligned memory access - SCB_CCR.UNALIGN_TRP = 0 */
	CM4::SCB->CCR &= ~(SCB_CCR_UNALIGN_TRP_Msk);

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
