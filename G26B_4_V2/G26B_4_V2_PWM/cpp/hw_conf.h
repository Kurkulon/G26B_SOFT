#ifndef HW_CONF_H__20_04_2022__16_00
#define HW_CONF_H__20_04_2022__16_00

#include "types.h"
#include "core.h"

#define MCK_MHz 200UL
#define MCK (MCK_MHz*1000000)
#define NS2CLK(x) (((x)*MCK_MHz+500)/1000)
#define US2CLK(x) ((x)*MCK_MHz)
#define MS2CLK(x) ((x)*MCK_MHz*1000)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//#define FRAM_SPI_MAINVARS_ADR 0
//#define FRAM_SPI_SESSIONS_ADR 0x200

#define FRAM_I2C_MAINVARS_ADR 0
#define FRAM_I2C_SESSIONS_ADR 0x200

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define __CONCAT1(s1)			s1
#define __CONCAT2(s1,s2)		s1##s2
#define __CONCAT3(s1,s2,s3)		s1##s2##s3

#define CONCAT1(s1)			__CONCAT1(s1)
#define CONCAT2(s1,s2)		__CONCAT2(s1,s2)
#define CONCAT3(s1,s2,s3)	__CONCAT3(s1,s2,s3)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_SAME53	

	// Test Pins
	// 6	- PB05	
	// 9	- PB06
	// 10	- PB07	
	// 11	- PB08	
	// 12	- PB09	
	// 14	- PA05	
	// 19	- PA10	
	// 20	- PA11	
	// 31	- PA14	
	// 32	- PA15	
	// 35	- PA16
	// 36	- PA17	
	// 37	- PA18	
	// 38	- PA19	
	// 39	- PB16
	// 40	- PB17
	// 41	- PA20	
	// 42	- PA21	
	// 46	- PA25	- main loop
	// 50	- PB23	- PwmDmaIRQ

	// 	- ManTrmIRQ2	
	// 	- ManRcvIRQ	
	// 	- ManRcvIRQ sync true	


	// ++++++++++++++	GEN	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define GEN_MCK				0
	#define GEN_32K				1
	#define GEN_25M				2
	#define GEN_1M				3
	//#define GEN_500K			4
	//#define GEN_EXT32K		5

	#define GEN_MCK_CLK			MCK
	#define GEN_32K_CLK			32768
	#define GEN_25M_CLK			25000000
	#define GEN_1M_CLK			1000000
	#define GEN_EXT32K_CLK		32768

	// ++++++++++++++	SERCOM	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	//#define SERCOM_0			0
	//#define SERCOM_1			1
	#define I2C_SERCOM_NUM		2
	#define UART0_SERCOM_NUM	3
	//#define SERCOM_4			4
	//#define SERCOM_5			5
	//#define SERCOM_6			6
	//#define SERCOM_7			7


	// ++++++++++++++	DMA	0...31	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define	PWMLA_DMA				DMA_CH0
	#define	PWMHA_DMA				DMA_CH1
	#define	PWMLB_DMA				DMA_CH2
	#define	PWMHB_DMA				DMA_CH3
	#define	DACTC_DMA				DMA_CH4
	#define	UART0_DMA				DMA_CH5
	//#define	NAND_MEMCOPY_DMA	DMA_CH6
	#define	I2C_DMACH				DMA_CH7
	//#define	DSP_DMATX			DMA_CH8
	//#define	DSP_DMARX			DMA_CH9
	#define	CRC_DMA					DMA_CH31

	// ++++++++++++++	EVENT 0...31	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define EVENT_PWM_SYNC		0
	#define EVENT_PWMDMA		1
	#define EVENT_PWMCOUNT		2
	#define EVENT_MANR_1		3
	#define EVENT_MANR_2		4

	// ++++++++++++++	TC	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	//#define					TC0
	//#define					TC1
	//#define MANT_TC			TC2
	#define MANI_TC				TC3
	//#define 					TC4
	//#define 					TC5
	//#define 					TC6
	//#define NAND_TC			TC7

	#define GEN_TC0_TC1			GEN_MCK
	#define GEN_TC2_TC3			GEN_MCK
	#define GEN_TC4_TC5			GEN_MCK
	#define GEN_TC6_TC7			GEN_MCK

	#define CLK_TC0_TC1			GEN_MCK_CLK
	#define CLK_TC2_TC3			GEN_MCK_CLK
	#define CLK_TC4_TC5			GEN_MCK_CLK
	#define CLK_TC6_TC7			GEN_MCK_CLK

	#define GEN_TC0				GEN_TC0_TC1
	#define GEN_TC1				GEN_TC0_TC1
	#define GEN_TC2				GEN_TC2_TC3
	#define GEN_TC3				GEN_TC2_TC3
	#define GEN_TC4				GEN_TC4_TC5
	#define GEN_TC5				GEN_TC4_TC5
	#define GEN_TC6				GEN_TC6_TC7
	#define GEN_TC7				GEN_TC6_TC7

	#define GCLK_TC0			GCLK_TC0_TC1
	#define GCLK_TC1			GCLK_TC0_TC1
	#define GCLK_TC2			GCLK_TC2_TC3
	#define GCLK_TC3			GCLK_TC2_TC3
	#define GCLK_TC4			GCLK_TC4_TC5
	#define GCLK_TC5			GCLK_TC4_TC5
	#define GCLK_TC6			GCLK_TC6_TC7
	#define GCLK_TC7			GCLK_TC6_TC7

	#define CLK_TC0				CLK_TC0_TC1
	#define CLK_TC1				CLK_TC0_TC1
	#define CLK_TC2				CLK_TC2_TC3
	#define CLK_TC3				CLK_TC2_TC3
	#define CLK_TC4				CLK_TC4_TC5
	#define CLK_TC5				CLK_TC4_TC5
	#define CLK_TC6				CLK_TC6_TC7
	#define CLK_TC7				CLK_TC6_TC7

	// ++++++++++++++	TCC	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define PWM_TCC				TCC0
	#define MANR_TCC			TCC1
	#define PWMDMA_TCC			TCC2
	#define PWMCOUNT_TCC		TCC3
	#define MANT_TCC			TCC4

	#define GEN_TCC0_TCC1		GEN_MCK
	#define GEN_TCC2_TCC3		GEN_MCK
	#define GEN_TCC4			GEN_MCK

	#define CLK_TCC0_TCC1		GEN_MCK_CLK
	#define CLK_TCC2_TCC3		GEN_MCK_CLK
	#define CLK_TCC4			GEN_MCK_CLK

	#define GEN_TCC0			GEN_TCC0_TCC1
	#define GEN_TCC1			GEN_TCC0_TCC1
	#define GEN_TCC2			GEN_TCC2_TCC3
	#define GEN_TCC3			GEN_TCC2_TCC3


	#define GCLK_TCC0			GCLK_TCC0_TCC1
	#define GCLK_TCC1			GCLK_TCC0_TCC1
	#define GCLK_TCC2			GCLK_TCC2_TCC3
	#define GCLK_TCC3			GCLK_TCC2_TCC3


	#define CLK_TCC0			CLK_TCC0_TCC1
	#define CLK_TCC1			CLK_TCC0_TCC1
	#define CLK_TCC2			CLK_TCC2_TCC3
	#define CLK_TCC3			CLK_TCC2_TCC3

	// ++++++++++++++	I2C	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//	#define I2C					CONCAT2(HW::I2C,I2C_SERCOM_NUM)
	#define PIO_I2C				HW::PIOA 
	#define PIN_SDA				12 
	#define PIN_SCL				13 
	#define SDA					(1<<PIN_SDA) 
	#define SCL					(1<<PIN_SCL) 
	#define I2C_PMUX_SDA		PORT_PMUX_C 
	#define I2C_PMUX_SCL		PORT_PMUX_C 
	#define I2C_GEN_SRC			GEN_MCK
	#define I2C_GEN_CLK			GEN_MCK_CLK
	#define I2C_BAUDRATE		400000 

	// ++++++++++++++	MANCH	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define PIO_MANCH		HW::PIOB
	#define PIN_L1			14 
	#define PIN_L2			15 
	#define MANCH_PMUX		PORT_PMUX_F
	#define L1_WO_NUM		0
	#define L2_WO_NUM		1

	#define L1				(1UL<<PIN_L1)
	#define L2				(1UL<<PIN_L2)
	#define H1				0
	#define H2				0

	#define PIO_RXD			HW::PIOB
	#define PIN_RXD			13
	#define RXD				(1UL<<PIN_RXD)

	#define Pin_ManRcvIRQ_Set()		HW::PIOB->BSET(11)
	#define Pin_ManRcvIRQ_Clr()		HW::PIOB->BCLR(11)

	#define Pin_ManTrmIRQ_Set()		HW::PIOB->BSET(10)		
	#define Pin_ManTrmIRQ_Clr()		HW::PIOB->BCLR(10)		

	#define Pin_ManRcvSync_Set()	HW::PIOB->BSET(12)		
	#define Pin_ManRcvSync_Clr()	HW::PIOB->BCLR(12)		

	// ++++++++++++++	VCORE	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define PIO_ENVCORE		HW::PIOA
	#define PIN_ENVCORE		27
	#define ENVCORE			(1<<PIN_ENVCORE) 
	
	// ++++++++++++++	USART	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define PIO_UTXD0			HW::PIOA 
	#define PIO_URXD0			HW::PIOA 
	#define PIO_RTS0			HW::PIOA 

	#define PMUX_UTXD0			PORT_PMUX_C
	#define PMUX_URXD0			PORT_PMUX_C 

	#define UART0_TXPO			USART_TXPO_0 
	#define UART0_RXPO			USART_RXPO_1 

	#define PIN_UTXD0			22 
	#define PIN_URXD0			23 
	#define PIN_RTS0			24 

	#define UTXD0				(1<<PIN_UTXD0) 
	#define URXD0				(1<<PIN_URXD0) 
	#define RTS0				(1<<PIN_RTS0) 

	#define UART0_GEN_SRC		GEN_MCK
	#define UART0_GEN_CLK		GEN_MCK_CLK

	// ++++++++++++++	PWM		++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define PIO_DRVEN			HW::PIOA
	#define PIO_WF_PWM			HW::PIOA
	#define PIO_PWM				HW::PIOA
	#define PIO_POL				HW::PIOB
	#define PIO_GEN				HW::PIOB
	#define PIO_DAC0			HW::PIOA
	#define PIO_ADC0_01			HW::PIOA

	#define GENA_WO_NUM			1
	#define GENB_WO_NUM			0

	#define PWMLA_WO_NUM		0
	#define PWMHA_WO_NUM		1
	#define PWMLB_WO_NUM		2
	#define PWMHB_WO_NUM		3

	#define PIN_DRVEN			0 
	#define PIN_WF_PWM			1
	#define PIN_PWMLA			8
	#define PIN_PWMLB			9
	#define PIN_PWMHA			10
	#define PIN_PWMHB			11
	#define PIN_POLWLA			0
	#define PIN_POLWHA			1
	#define PIN_POLWLB			2
	#define PIN_POLWHB			3
	#define PIN_GENA			31
	#define PIN_GENB			30
	#define PIN_DAC0			2
	#define PIN_ADC0_01			3

	#define DRVEN				(1UL<<PIN_DRVEN		) 
	#define WF_PWM				(1UL<<PIN_WF_PWM	) 
	#define PWMLA				(1UL<<PIN_PWMLA		) 
	#define PWMHA				(1UL<<PIN_PWMHA		) 
	#define PWMLB				(1UL<<PIN_PWMLB		) 
	#define PWMHB				(1UL<<PIN_PWMHB		) 
	#define POLWLA				(1UL<<PIN_POLWLA	) 
	#define POLWHA				(1UL<<PIN_POLWHA	) 
	#define POLWLB				(1UL<<PIN_POLWLB	) 
	#define POLWHB				(1UL<<PIN_POLWHB	) 
	#define GENA				(1UL<<PIN_GENA		) 
	#define GENB				(1UL<<PIN_GENB		) 
	#define DAC0				(1UL<<PIN_DAC0		) 
	#define ADC0_01				(1UL<<PIN_ADC0_01	) 

	#define PMUX_GENA			PORT_PMUX_E
	#define PMUX_GENB			PORT_PMUX_E

	#define PMUX_DAC0			PORT_PMUX_B
	#define PMUX_ADC0_01		PORT_PMUX_B

	#define PMUX_PWMLA			PORT_PMUX_F
	#define PMUX_PWMHA			PORT_PMUX_F
	#define PMUX_PWMLB			PORT_PMUX_F
	#define PMUX_PWMHB			PORT_PMUX_F

	#define PWM_EXTINT			(PIN_URXD0&15)


	// ++++++++++++++	PIO INIT	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define PIOA_INIT_DIR		(DRVEN|WF_PWM|PWMLA|PWMLB|ENVCORE|PA05|PA10|PA11|PA14|PA15|PA16|PA17|PA18|PA19|PA20|PA21|PA25)
	#define PIOA_INIT_SET		(DRVEN|ENVCORE)
	#define PIOA_INIT_CLR		(WF_PWM|PWMLA|PWMLB|PA05|PA10|PA11|PA14|PA15|PA16|PA17|PA18|PA19|PA20|PA21|PA25)
	#define PIOA_TEST_MASK		(DRVEN|WF_PWM|PWMLA|PWMLB|SDA|SCL|UTXD0|RTS0|ENVCORE|PA05|PA10|PA11|PA14|PA15|PA16|PA17|PA18|PA19|PA20|PA21|PA25)

	#define PIOB_INIT_DIR		(POLWLA|POLWHA|POLWLB|POLWHB|PWMHA|PWMHB|L1|L2|GENA|GENB|PB05|PB06|PB07|PB08|PB09|PB12|PB16|PB17|PB23)
	#define PIOB_INIT_SET		(0)
	#define PIOB_INIT_CLR		(POLWLA|POLWHA|POLWLB|POLWHB|PWMHA|PWMHB|L1|L2|GENA|GENB|PB05|PB06|PB07|PB08|PB09|PB12|PB16|PB17|PB23)
	#define PIOB_TEST_MASK		(POLWLA|POLWHA|POLWLB|POLWHB|PWMHA|PWMHB|L1|L2|GENA|GENB|PB05|PB06|PB07|PB08|PB09|PB12|PB16|PB17|PB23)

	#define PIOC_INIT_DIR		(0)
	#define PIOC_INIT_SET		(0)
	#define PIOC_INIT_CLR		(0)
	#define PIOC_TEST_MASK		(0)

	#define TEST_PIN_DELAY		(MCK_MHz)

	#define Pin_MainLoop_Set()	HW::PIOA->BSET(25)
	#define Pin_MainLoop_Clr()	HW::PIOA->BCLR(25)


#elif defined(WIN32) //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define BAUD2CLK(x)				(x)
	#define MT(v)					(v)

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




#endif // HW_CONF_H__20_04_2022__16_00
