#include "types.h"
#include "core.h"
#include "time.h"
#include "CRC16_8005.h"
#include "list.h"
#include "PointerCRC.h"

#include "hardware.h"
#include "SEGGER_RTT.h"
#include "hw_conf.h"
#include "hw_rtm.h"
#include "hw_nand.h"
#include "DMA.h"
#include "manch.h"
#include <math.h>

#ifdef WIN32

#include <windows.h>
#include <Share.h>
#include <conio.h>
#include <stdarg.h>
#include <stdio.h>
#include <intrin.h>
#include "CRC16_CCIT.h"
#include "list.h"

static HANDLE handleNandFile;
static const char nameNandFile[] = "NAND_FLASH_STORE.BIN";

static HANDLE handleWriteThread;
static HANDLE handleReadThread;

static byte nandChipSelected = 0;

static u64 curNandFilePos = 0;
//static u64 curNandFileBlockPos = 0;
static u32 curBlock = 0;
static u32 curRawBlock = 0;
static u16 curPage = 0;
static u16 curCol = 0;

static OVERLAPPED	_overlapped;
static u32			_ovlReadedBytes = 0;
static u32			_ovlWritenBytes = 0;

static void* nandEraseFillArray;
static u32 nandEraseFillArraySize = 0;
static byte nandReadStatus = 0x41;
static u32 lastError = 0;


static byte fram_I2c_Mem[0x10000];
static byte fram_SPI_Mem[0x40000];

static bool fram_spi_WREN = false;

static u16 crc_ccit_result = 0;


struct BlockBuffer { BlockBuffer *next; u32 block; u32 prevBlock; u32 writed; u32 data[((NAND_PAGE_SIZE+NAND_SPARE_SIZE) << NAND_PAGE_BITS) >> 2]; };

static BlockBuffer _blockBuf[16];

static List<BlockBuffer> freeBlockBuffer;
static List<BlockBuffer> rdBlockBuffer;
static List<BlockBuffer> wrBlockBuffer;

static BlockBuffer *curNandBlockBuffer[4] = { 0 };

static volatile bool busyWriteThread = false;

#elif defined(CPU_SAME53)

//__align(16) T_HW::DMADESC DmaTable[32];
//__align(16) T_HW::DMADESC DmaWRB[32];

#elif defined(CPU_XMC48)

//#pragma O3
//#pragma Otime

#endif 

static bool busy_CRC_CCITT_DMA = false;

const float pi = 3.14159265358979f;
extern u16 curFireVoltage;
u16 waveBuffer[1000] = {0};

#define pwmPeriodUS 6
//u16 waveAmp = 0;
u16 waveFreq = 3000; //Hz
u16 waveLen = 50;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//static void InitVectorTable();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifndef WIN32

__forceinline 	void EnableVCORE()	{ PIO_ENVCORE->CLR(ENVCORE); 	}
__forceinline 	void DisableVCORE()	{ PIO_ENVCORE->SET(ENVCORE); 	}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/*----------------------------------------------------------------------------
  Initialize the system
 *----------------------------------------------------------------------------*/
#ifndef WIN32

extern "C" void SystemInit()
{
	//u32 i;
	using namespace CM4;
	using namespace HW;

//	__breakpoint(0);

	SEGGER_RTT_Init();

	SEGGER_RTT_WriteString(0, RTT_CTRL_CLEAR);

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "SystemInit ... ");

	#ifdef CPU_SAME53	

		HW::PIOA->DIRSET =	PIOA_INIT_DIR;
		HW::PIOA->CLR(		PIOA_INIT_CLR);
		HW::PIOA->SET(		PIOA_INIT_SET);

		HW::PIOB->DIRSET =	PIOB_INIT_DIR;
		HW::PIOB->CLR(		PIOB_INIT_CLR);
		HW::PIOB->SET(		PIOB_INIT_SET);

		HW::PIOC->DIRSET =	PIOC_INIT_DIR;
		HW::PIOC->CLR(		PIOC_INIT_CLR);
		HW::PIOC->SET(		PIOC_INIT_SET);

		Pin_MainLoop_Set();

		if ((HW::SUPC->BOD33 & BOD33_ENABLE) == 0)
		{
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
		};

		Pin_MainLoop_Clr();

		OSCCTRL->XOSC[1] = XOSC_ENABLE|XOSC_ONDEMAND; // RUNSTDBY|ENABLE

		Pin_MainLoop_Set();

		OSCCTRL->DPLL[0].CTRLA = 0; while ((OSCCTRL->DPLL[0].SYNCBUSY & DPLLSYNCBUSY_ENABLE) != 0);

		OSCCTRL->DPLL[0].CTRLB = DPLL_REFCLK_XOSC1|DPLL_DIV(24);	// 0x70010; // XOSC clock source division factor 50 = 2*(DIV+1), XOSC clock reference
		OSCCTRL->DPLL[0].RATIO = DPLL_LDR((MCK*2+500000)/1000000-1)|DPLL_LDRFRAC(0);	// 47; // Loop Divider Ratio = 200, Loop Divider Ratio Fractional Part = 0

		OSCCTRL->DPLL[0].CTRLA = DPLL_ONDEMAND|DPLL_ENABLE; 

		Pin_MainLoop_Clr();

		HW::GCLK->GENCTRL[GEN_MCK] = GCLK_DIV(0)|GCLK_SRC_DPLL0|GCLK_GENEN;

		Pin_MainLoop_Set();

		HW::MCLK->AHBMASK |= AHB_DMAC;
		HW::DMAC->CTRL = 0;
		HW::DMAC->CTRL = DMAC_SWRST;
		HW::DMAC->DBGCTRL = DMAC_DBGRUN;
		HW::DMAC->BASEADDR	= DmaTable;
		HW::DMAC->WRBADDR	= DmaWRB;
		HW::DMAC->CTRL = DMAC_DMAENABLE|DMAC_LVLEN0|DMAC_LVLEN1|DMAC_LVLEN2|DMAC_LVLEN3;

		Pin_MainLoop_Clr();

		if ((CMCC->SR & CMCC_CSTS) == 0)
		{
			CMCC->CTRL = CMCC_CEN;
		};

		Pin_MainLoop_Set();
		Pin_MainLoop_Clr();
		Pin_MainLoop_Set();
		Pin_MainLoop_Clr();

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

		SCU_POWER->PWRSET = SCU_POWER_PWRSET_HIB_Msk;	while((SCU_POWER->PWRSTAT & SCU_POWER_PWRSTAT_HIBEN_Msk) == 0);
		SCU_RESET->RSTCLR = SCU_RESET_RSTCLR_HIBRS_Msk;	while((SCU_RESET->RSTSTAT & SCU_RESET_RSTSTAT_HIBRS_Msk) != 0);

		if (SCU_HIBERNATE->OSCULCTRL != OSCULCTRL_MODE_BYPASS)
		{
			while (SCU_GENERAL->MIRRSTS & SCU_GENERAL_MIRRSTS_OSCULCTRL_Msk);	SCU_HIBERNATE->OSCULCTRL = OSCULCTRL_MODE_BYPASS;
			while (SCU_GENERAL->MIRRSTS & SCU_GENERAL_MIRRSTS_HDCR_Msk);		SCU_HIBERNATE->HDCR |= SCU_HIBERNATE_HDCR_ULPWDGEN_Msk;
			while (SCU_GENERAL->MIRRSTS & SCU_GENERAL_MIRRSTS_HDCLR_Msk);		SCU_HIBERNATE->HDCLR = SCU_HIBERNATE_HDCLR_ULPWDG_Msk;
		};

		while (SCU_GENERAL->MIRRSTS & SCU_GENERAL_MIRRSTS_HDCR_Msk);	SCU_HIBERNATE->HDCR |= SCU_HIBERNATE_HDCR_RCS_Msk | SCU_HIBERNATE_HDCR_STDBYSEL_Msk;


		//P2->ModePin10(	G_PP	);
		//P2->BSET(10);

		P0->ModePin0(	G_PP	);
		P0->ModePin1(	G_PP	);
		P0->ModePin2(	HWIO1	);
		P0->ModePin3(	HWIO1	);
		P0->ModePin4(	HWIO1	);
		P0->ModePin5(	HWIO1	);
		P0->ModePin6(	I1DPD	);
		P0->ModePin7(	HWIO1	);
		P0->ModePin8(	HWIO1	);
		P0->ModePin9(	G_PP	);
		P0->ModePin10(	G_PP	);
		P0->ModePin11(	G_PP	);
		P0->ModePin12(	G_PP	);
		P0->ModePin13(	G_PP	);
		P0->ModePin14(	G_PP	);
		P0->ModePin15(	G_PP	);

		P0->PPS = 0;

		P1->ModePin0(	G_PP	);
		P1->ModePin1(	G_PP	);
		P1->ModePin2(	G_PP	);
		P1->ModePin3(	G_PP	);
		P1->ModePin4(	I2DPU	);
		P1->ModePin5(	A2PP	);
		P1->ModePin6(	G_PP	);
		P1->ModePin7(	G_PP	);
		P1->ModePin8(	G_PP	);
		P1->ModePin9(	G_PP	);
		P1->ModePin10(	I2DPU	);
		P1->ModePin11(	I2DPU	);
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
		P2->ModePin6(	G_PP	);
		P2->ModePin7(	A1PP	);
		P2->ModePin8(	A1PP	);
		P2->ModePin9(	A1PP	);
		P2->ModePin10(	G_PP	);
		P2->ModePin11(	G_PP	);
		P2->ModePin12(	G_PP	);
		P2->ModePin13(	G_PP	);
		//P2->ModePin14(	A2PP	);
		//P2->ModePin15(	I2DPU	);

		P2->PPS = 0;

		P3->ModePin0(	HWIO1	);
		P3->ModePin1(	HWIO1	);
		P3->ModePin2(	G_PP	);
		P3->ModePin3(	G_PP	);
		P3->ModePin4(	G_PP	);
		P3->ModePin5(	HWIO1	);
		P3->ModePin6(	HWIO1	);
		P3->ModePin7(	G_PP	);
		P3->ModePin8(	G_PP	);
		P3->ModePin9(	G_PP	);
		P3->ModePin10(	G_PP	);
		P3->ModePin11(	G_PP	);
		P3->ModePin12(	G_PP	);
		P3->ModePin13(	G_PP	);
		P3->ModePin14(	I2DPU	);
		P3->ModePin15(	A2PP	);

		P3->PPS = 0;

		P4->ModePin0(	G_PP	);
		P4->ModePin1(	G_PP	);
		P4->ModePin2(	G_PP	);
		P4->ModePin3(	G_PP	);
		P4->ModePin4(	G_PP	);
		P4->ModePin5(	G_PP	);
		P4->ModePin6(	I2DPU	);
		P4->ModePin7(	A1PP	);

		P4->PPS = 0;

		P5->ModePin0(	HWIO0	);
		P5->ModePin1(	G_PP	);
		P5->ModePin2(	A1OD	);
		P5->ModePin3(	G_PP	);
		P5->ModePin4(	G_PP	);
		P5->ModePin5(	G_PP	);
		P5->ModePin6(	G_PP	);
		P5->ModePin7(	G_PP	);
		//P5->ModePin8(	A2PP	);
		//P5->ModePin9(	G_PP	);
		P5->ModePin10(	G_PP	);
		//P5->ModePin11(	G_PP	);

		P5->PPS = 0;


		P6->ModePin0(	G_PP	);
		P6->ModePin1(	G_PP	);
		P6->ModePin2(	G_PP	);
		P6->ModePin3(	I2DPU	);
		P6->ModePin4(	A2PP	);
		P6->ModePin5(	G_PP	);
		P6->ModePin6(	G_PP	);

		P6->PPS = 0;

		P14->ModePin0(	I2DPU	);
		P14->ModePin1(	I2DPU	);
		P14->ModePin2(	I2DPU	);
		P14->ModePin3(	I2DPU	);
		P14->ModePin4(	I2DPU	);
		P14->ModePin5(	I2DPU	);
		P14->ModePin6(	I2DPU	);
		P14->ModePin7(	I2DPU	);
		P14->ModePin8(	I2DPU	);
		P14->ModePin9(	I2DPU	);
		P14->ModePin12(	I0DNP	);
		P14->ModePin13(	I2DPU	);
		P14->ModePin14(	I2DPU	);
		P14->ModePin15(	I2DPU	);

		P14->PPS = 0;
		P14->PDISC = (1<<0);

		P15->ModePin2(	I2DPU	);
		P15->ModePin3(	I2DPU	);
		P15->ModePin4(	I2DPU	);
		P15->ModePin5(	I2DPU	);
		P15->ModePin6(	I2DPU	);
		P15->ModePin7(	I2DPU	);
		P15->ModePin8(	I2DPU	);
		P15->ModePin9(	I1DPD	);
		P15->ModePin12(	I2DPU	);
		P15->ModePin13(	I2DPU	);
		P15->ModePin14(	I2DPU	);
		P15->ModePin15(	I2DPU	);

		P15->PPS = 0;
		P15->PDISC = 0;

		HW::Peripheral_Enable(PID_DMA0);
		HW::Peripheral_Enable(PID_DMA1);

		//HW::DLR->SRSEL0 = SRSEL0(10,11,0,0,0,0,0,0);
		//HW::DLR->SRSEL1 = SRSEL1(0,0,0,0);

		HW::DLR->DRL0 = DRL0_USIC0_SR0;
		HW::DLR->DRL1 = DRL1_USIC1_SR0;
		HW::DLR->DRL2 = DRL2_USIC0_SR1;
		HW::DLR->DRL9 = DRL9_USIC2_SR1;

		HW::DLR->LNEN |= (1<<0)|(1<<2)|(1<<9);

	#endif

	#if ((__FPU_PRESENT == 1) && (__FPU_USED == 1))
		CM4::SCB->CPACR |= ((3UL << 10*2) |                 /* set CP10 Full Access */
							(3UL << 11*2)  );               /* set CP11 Full Access */
	#else
		CM4::SCB->CPACR = 0;
	#endif

  /* Enable unaligned memory access - SCB_CCR.UNALIGN_TRP = 0 */
	CM4::SCB->CCR &= ~(SCB_CCR_UNALIGN_TRP_Msk);

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_GREEN "OK\n");
}

#endif
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_XMC48

u16 CRC_CCITT_PIO(const void *data, u32 len, u16 init)
{
	CRC_FCE->CRC = init;	//	DataCRC CRC = { init };

	__packed const byte *s = (__packed const byte*)data;

	for ( ; len > 0; len--)
	{
		CRC_FCE->IR = *(s++);
	};

	//if (len > 0)
	//{
	//	CRC_FCE->IR = *(s++)&0xFF;
	//}

	__dsb(15);

	return CRC_FCE->RES;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u16 CRC_CCITT_DMA(const void *data, u32 len, u16 init)
{
	HW::P6->BSET(5);

	CRC_FCE->CRC = init;	//	DataCRC CRC = { init };

	CRC_DMA.MemCopySrcInc(data, &CRC_FCE->IR, len);

	while (!CRC_DMA.CheckMemCopyComplete());

	HW::P6->BCLR(5);

	__dsb(15);

	return (byte)CRC_FCE->RES;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool CRC_CCITT_DMA_Async(const void* data, u32 len, u16 init)
{
	HW::P6->BSET(5);

	CRC_FCE->CRC = init;	//	DataCRC CRC = { init };

	CRC_DMA.MemCopySrcInc(data, &CRC_FCE->IR, len);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool CRC_CCITT_DMA_CheckComplete(u16* crc)
{
	if (CRC_DMA.CheckMemCopyComplete())
	{
		__dsb(15);

		*crc = (byte)CRC_FCE->RES;

		return true;
	}
	else
	{
		return false;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void Init_CRC_CCITT_DMA()
{
	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "Init_CRC_CCITT_DMA ... ");

	HW::Peripheral_Enable(PID_FCE);

	HW::FCE->CLC = 0;
	CRC_FCE->CFG = 0;

	SEGGER_RTT_WriteString(0, "OK\n");
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#elif defined(CPU_SAME53)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u16 CRC_CCITT_DMA(const void *data, u32 len, u16 init)
{
	CRC_DMA.CRC_CCITT(data, len, init);

	while (!CRC_DMA.CheckComplete());

	__dsb(15);

	return CRC_DMA.Get_CRC_CCITT_Result();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool CRC_CCITT_DMA_Async(const void* data, u32 len, u16 init)
{
	if (busy_CRC_CCITT_DMA) return false;

	HW::PIOA->BSET(27);
	
	busy_CRC_CCITT_DMA = true;

	CRC_DMA.CRC_CCITT(data, len, init);

	return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool CRC_CCITT_DMA_CheckComplete(u16* crc)
{
	if (CRC_DMA.CheckComplete())
	{
		__dsb(15);

		*crc = CRC_DMA.Get_CRC_CCITT_Result();

		busy_CRC_CCITT_DMA = false;

		HW::PIOA->BCLR(27);

		return true;
	}
	else
	{
		return false;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void Init_CRC_CCITT_DMA()
{
	//T_HW::DMADESC &dmadsc = DmaTable[CRC_DMACH];
	//T_HW::S_DMAC::S_DMAC_CH	&dmach = HW::DMAC->CH[CRC_DMACH];

	//HW::DMAC->CRCCTRL = DMAC_CRCBEATSIZE_BYTE|DMAC_CRCPOLY_CRC16|DMAC_CRCMODE_CRCGEN|DMAC_CRCSRC(0x3F);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#elif defined(WIN32)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u16 CRC_CCITT_DMA(const void *data, u32 len, u16 init)
{
	return 0;//GetCRC16_CCIT(data, len, init);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool CRC_CCITT_DMA_Async(const void* data, u32 len, u16 init)
{
	crc_ccit_result = 0;//GetCRC16_CCIT(data, len, init);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool CRC_CCITT_DMA_CheckComplete(u16* crc)
{
	if (crc != 0)
	{
		*crc = crc_ccit_result;

		return true;
	}
	else
	{
		return false;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void Init_CRC_CCITT_DMA()
{
	//T_HW::DMADESC &dmadsc = DmaTable[CRC_DMACH];
	//T_HW::S_DMAC::S_DMAC_CH	&dmach = HW::DMAC->CH[CRC_DMACH];

	//HW::DMAC->CRCCTRL = DMAC_CRCBEATSIZE_BYTE|DMAC_CRCPOLY_CRC16|DMAC_CRCMODE_CRCGEN|DMAC_CRCSRC(0x3F);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//void SetClock(const RTC &t)
//{
//	static DSCI2C dsc;
//
//	static byte reg = 0;
//	static u16 rbuf = 0;
//	static byte buf[10];
//
//	buf[0] = 0;
//	buf[1] = ((t.sec/10) << 4)|(t.sec%10);
//	buf[2] = ((t.min/10) << 4)|(t.min%10);
//	buf[3] = ((t.hour/10) << 4)|(t.hour%10);
//	buf[4] = 1;
//	buf[5] = ((t.day/10) << 4)|(t.day%10);
//	buf[6] = ((t.mon/10) << 4)|(t.mon%10);
//
//	byte y = t.year % 100;
//
//	buf[7] = ((y/10) << 4)|(y%10);
//
//	dsc.adr = 0x68;
//	dsc.wdata = buf;
//	dsc.wlen = 8;
//	dsc.rdata = 0;
//	dsc.rlen = 0;
//	dsc.wdata2 = 0;
//	dsc.wlen2 = 0;
//
//	if (SetTime(t))
//	{
//		I2C_AddRequest(&dsc);
//	};
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void WDT_Init()
{
	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "WDT Init ... ");

	#ifdef CPU_SAME53	

		//HW::MCLK->APBAMASK |= APBA_WDT;

		//HW::WDT->CONFIG = WDT_WINDOW_CYC512|WDT_PER_CYC1024;
	
		#ifndef _DEBUG
		HW::WDT->CTRLA = WDT_ENABLE;
		#else
		HW::WDT->CTRLA = 0;
		#endif

		//while(HW::WDT->SYNCBUSY);

	#elif defined(CPU_XMC48)

		#ifndef _DEBUG
	
		//HW::WDT_Enable();

		//HW::WDT->WLB = OFI_FREQUENCY/2;
		//HW::WDT->WUB = (3 * OFI_FREQUENCY)/2;
		//HW::SCU_CLK->WDTCLKCR = 0|SCU_CLK_WDTCLKCR_WDTSEL_OFI;

		//HW::WDT->CTR = WDT_CTR_ENB_Msk|WDT_CTR_DSP_Msk;

		#else

		HW::WDT_Disable();

		#endif

	#endif

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_GREEN "OK\n");
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef WIN32

//extern dword DI;
//extern dword DO;
char pressedKey;
//extern  dword maskLED[16];

//byte _array1024[0x60000]; 

HWND  hMainWnd;

HCURSOR arrowCursor;
HCURSOR handCursor;

HBRUSH	redBrush;
HBRUSH	yelBrush;
HBRUSH	grnBrush;
HBRUSH	gryBrush;

RECT rectLed[10] = { {20, 41, 33, 54}, {20, 66, 33, 79}, {20, 91, 33, 104}, {21, 117, 22, 118}, {20, 141, 33, 154}, {218, 145, 219, 146}, {217, 116, 230, 129}, {217, 91, 230, 104}, {217, 66, 230, 79}, {217, 41, 230, 54}  }; 
HBRUSH* brushLed[10] = { &yelBrush, &yelBrush, &yelBrush, &gryBrush, &grnBrush, &gryBrush, &redBrush, &redBrush, &redBrush, &redBrush };

//int x,y,Depth;

HFONT font1;
HFONT font2;

HDC memdc;
HBITMAP membm;

//HANDLE facebitmap;

static const u32 secBufferWidth = 80;
static const u32 secBufferHeight = 12;
static const u32 fontWidth = 12;
static const u32 fontHeight = 16;


static char secBuffer[secBufferWidth*secBufferHeight*2];

static u32 pallete[16] = {	0x000000,	0x800000,	0x008000,	0x808000,	0x000080,	0x800080,	0x008080,	0xC0C0C0,
							0x808080,	0xFF0000,	0x00FF00,	0xFFFF00,	0x0000FF,	0xFF00FF,	0x00FFFF,	0xFFFFFF };

const char lpAPPNAME[] = "¿—76÷_”œ–";

int screenWidth = 0, screenHeight = 0;

LRESULT CALLBACK WindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static __int64		tickCounter = 0;

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef WIN32

static void InitDisplay()
{
	WNDCLASS		    wcl;

	wcl.hInstance		= NULL;
	wcl.lpszClassName	= lpAPPNAME;
	wcl.lpfnWndProc		= WindowProc;
	wcl.style	    	= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

	wcl.hIcon	    	= NULL;
	wcl.hCursor	    	= NULL;
	wcl.lpszMenuName	= NULL;

	wcl.cbClsExtra		= 0;
	wcl.cbWndExtra		= 0;
	wcl.hbrBackground	= NULL;

	RegisterClass (&wcl);

	int sx = screenWidth = GetSystemMetrics (SM_CXSCREEN);
	int sy = screenHeight = GetSystemMetrics (SM_CYSCREEN);

	hMainWnd = CreateWindowEx (0, lpAPPNAME, lpAPPNAME,	WS_DLGFRAME|WS_POPUP, 0, 0,	640, 480, NULL,	NULL, NULL, NULL);

	if(!hMainWnd) 
	{
		cputs("Error creating window\r\n");
		exit(0);
	};

	RECT rect;

	if (GetClientRect(hMainWnd, &rect))
	{
		MoveWindow(hMainWnd, 0, 0, 641 + 768 - rect.right - 2, 481 - rect.bottom - 1 + 287, true);
	};

	ShowWindow (hMainWnd, SW_SHOWNORMAL);

	font1 = CreateFont(30, 14, 0, 0, 100, false, false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH|FF_DONTCARE, "Lucida Console");
	font2 = CreateFont(fontHeight, fontWidth-2, 0, 0, 100, false, false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH|FF_DONTCARE, "Lucida Console");

	if (font1 == 0 || font2 == 0)
	{
		cputs("Error creating font\r\n");
		exit(0);
	};

	GetClientRect(hMainWnd, &rect);
	HDC hdc = GetDC(hMainWnd);
    memdc = CreateCompatibleDC(hdc);
	membm = CreateCompatibleBitmap(hdc, rect.right - rect.left + 1, rect.bottom - rect.top + 1 + secBufferHeight*fontHeight);
    SelectObject(memdc, membm);
	ReleaseDC(hMainWnd, hdc);


	arrowCursor = LoadCursor(0, IDC_ARROW);
	handCursor = LoadCursor(0, IDC_HAND);

	if (arrowCursor == 0 || handCursor == 0)
	{
		cputs("Error loading cursors\r\n");
		exit(0);
	};

	LOGBRUSH lb;

	lb.lbStyle = BS_SOLID;
	lb.lbColor = RGB(0xFF, 0, 0);

	redBrush = CreateBrushIndirect(&lb);

	lb.lbColor = RGB(0xFF, 0xFF, 0);

	yelBrush = CreateBrushIndirect(&lb);

	lb.lbColor = RGB(0x7F, 0x7F, 0x7F);

	gryBrush = CreateBrushIndirect(&lb);

	lb.lbColor = RGB(0, 0xFF, 0);

	grnBrush = CreateBrushIndirect(&lb);

	for (u32 i = 0; i < sizeof(secBuffer); i+=2)
	{
		secBuffer[i] = 0x20;
		secBuffer[i+1] = 0xF0;
	};
}

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef WIN32

void UpdateDisplay()
{
	static byte curChar = 0;
	//static byte c = 0, i = 0;
	//static byte flashMask = 0;
	static const byte a[4] = { 0x80, 0x80+0x40, 0x80+20, 0x80+0x40+20 };

	MSG msg;

	static dword pt = 0;
	static TM32 tm;

	u32 t = GetTickCount();

	if ((t-pt) >= 2)
	{
		pt = t;

		while(PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			GetMessage (&msg, NULL, 0, 0);

			TranslateMessage (&msg);

			DispatchMessage (&msg);
		};
	};

	static char buf[80];
	static char buf1[sizeof(secBuffer)];

	if (tm.Check(20))
	{
		bool rd = true;

		if (rd)
		{
			//SelectObject(memdc, font1);
			//SetBkColor(memdc, 0x074C00);
			//SetTextColor(memdc, 0x00FF00);
			//TextOut(memdc, 443, 53, buf, 20);
			//TextOut(memdc, 443, 30*1+53, buf+20, 20);
			//TextOut(memdc, 443, 30*2+53, buf+40, 20);
			//TextOut(memdc, 443, 30*3+53, buf+60, 20);

			SelectObject(memdc, font2);

			for (u32 j = 0; j < secBufferHeight; j++)
			{
				for (u32 i = 0; i < secBufferWidth; i++)
				{
					u32 n = (j*secBufferWidth+i)*2;

					u8 t = secBuffer[n+1];

					SetBkColor(memdc, pallete[(t>>4)&0xF]);
					SetTextColor(memdc, pallete[t&0xF]);
					TextOut(memdc, i*fontWidth, j*fontHeight+287, secBuffer+n, 1);
				};
			};

			RedrawWindow(hMainWnd, 0, 0, RDW_INVALIDATE);

		}; // if (rd)

	}; // if ((t-pt) > 10)

}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef WIN32

LRESULT CALLBACK WindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int i;
	HDC hdc;
	PAINTSTRUCT ps;
	bool c;
	static int x, y;
	int x0, y0, r0;
	static char key = 0;
	static char pkey = 0;

	RECT rect;

	static bool move = false;
	static int movex, movey = 0;

//	char *buf = (char*)screenBuffer;

    switch (message)
	{
        case WM_CREATE:

            break;

	    case WM_DESTROY:

		    break;

        case WM_PAINT:

			if (GetUpdateRect(hWnd, &rect, false)) 
			{
				hdc = BeginPaint(hWnd, &ps);

				//printf("Update RECT: %li, %li, %li, %li\r\n", ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);

				c = BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left + 1, ps.rcPaint.bottom - ps.rcPaint.top + 1, memdc, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
	 
				EndPaint(hWnd, &ps);
			};

            break;

        case WM_CHAR:

			pressedKey = wParam;

			if (pressedKey == '`')
			{
				GetWindowRect(hWnd, &rect);

				if ((rect.bottom-rect.top) > 340)
				{
					MoveWindow(hWnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top-fontHeight*secBufferHeight, true); 
				}
				else
				{
					MoveWindow(hWnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top+fontHeight*secBufferHeight, true); 
				};
			};

            break;

		case WM_MOUSEMOVE:

			x = LOWORD(lParam); y = HIWORD(lParam);

			if (move)
			{
				GetWindowRect(hWnd, &rect);
				SetWindowPos(hWnd, HWND_TOP, rect.left+x-movex, rect.top+y-movey, 0, 0, SWP_NOSIZE); 
			};

			return 0;

			break;

		case WM_MOVING:

			return TRUE;

			break;

		case WM_SYSCOMMAND:

			return 0;

			break;

		case WM_LBUTTONDOWN:

			move = true;
			movex = x; movey = y;

			SetCapture(hWnd);

			return 0;

			break;

		case WM_LBUTTONUP:

			move = false;

			ReleaseCapture();

			return 0;

			break;

		case WM_MBUTTONDOWN:

			ShowWindow(hWnd, SW_MINIMIZE);

			return 0;

			break;

		case WM_ACTIVATE:

			if (HIWORD(wParam) != 0 && LOWORD(wParam) != 0)
			{
				ShowWindow(hWnd, SW_NORMAL);
			};

			break;

		case WM_CLOSE:

			//run = false;

			break;
	};
    
	return DefWindowProc(hWnd, message, wParam, lParam);
}

#endif		
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef WIN32

int PutString(u32 x, u32 y, byte c, const char *str)
{
	char *dst = secBuffer+(y*secBufferWidth+x)*2;
	dword i = secBufferWidth-x;

	while (*str != 0 && i > 0)
	{
		*(dst++) = *(str++);
		*(dst++) = c;
		i -= 1;
	};

	return secBufferWidth-x-i;
}

#endif
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef	WIN32

int Printf(u32 xx, u32 yy, byte c, const char *format, ... )
{
	char buf[1024];

	va_list arglist;

    va_start(arglist, format);
    vsprintf(buf, format, arglist);
    va_end(arglist);

	return PutString(xx, yy, c, buf);
}

#endif
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//i32	Get_FRAM_SPI_SessionsAdr() { return FRAM_SPI_SESSIONS_ADR; }

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//i32	Get_FRAM_I2C_SessionsAdr() { return FRAM_I2C_SESSIONS_ADR; }

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef PWMDMA_TCC

	#define PWMDMATCC					HW::PWMDMA_TCC
	#define PWMDMA_GEN					CONCAT2(GEN_,PWMDMA_TCC)
	#define PWMDMA_GEN_CLK				CONCAT2(CLK_,PWMDMA_TCC) 
	#define PWMDMA_IRQ					CONCAT2(PWMDMA_TCC,_0_IRQ)
	#define GCLK_PWMDMA					CONCAT2(GCLK_,PWMDMA_TCC)
	#define PID_PWMDMA					CONCAT2(PID_,PWMDMA_TCC)

	#if (PWMDMA_GEN_CLK > 100000000)
			#define PWMDMA_PRESC_NUM	1
	#elif (PWMDMA_GEN_CLK > 50000000)
			#define PWMDMA_PRESC_NUM	1
	#elif (PWMDMA_GEN_CLK > 20000000)
			#define PWMDMA_PRESC_NUM	1
	#elif (PWMDMA_GEN_CLK > 10000000)
			#define PWMDMA_PRESC_NUM	1
	#elif (PWMDMA_GEN_CLK > 5000000)
			#define PWMDMA_PRESC_NUM	1
	#else
			#define PWMDMA_PRESC_NUM	1
	#endif

	#define PWMDMA_PRESC_DIV			CONCAT2(TCC_PRESCALER_DIV,PWMDMA_PRESC_NUM)
	#define US2PWMDMA(v)				(((v)*(PWMDMA_GEN_CLK/PWMDMA_PRESC_NUM/1000)+500)/1000)

	#define PWMDMA0_EVSYS_USER			CONCAT3(EVSYS_USER_, PWMDMA_TCC, _EV_0)
	#define PWMDMA1_EVSYS_USER			CONCAT3(EVSYS_USER_, PWMDMA_TCC, _EV_1)
	#define PWMDMA_EVENT_GEN			CONCAT3(EVGEN_, PWMDMA_TCC, _OVF)

	#define PWMDMA_DMCH_TRIGSRC			CONCAT3(DMCH_TRIGSRC_, PWMDMA_TCC, _OVF)

	inline void PWMDMA_ClockEnable()	{ HW::GCLK->PCHCTRL[GCLK_PWMDMA] = PWMDMA_GEN|GCLK_CHEN; HW::MCLK->ClockEnable(PID_PWMDMA); }

#else
	#error  Must defined PWMDMA_TCC
#endif


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef PWMCOUNT_TCC

	#define PWMCOUNTTCC					HW::PWMCOUNT_TCC
	#define PWMCOUNT_GEN				CONCAT2(GEN_,PWMCOUNT_TCC)
	#define PWMCOUNT_GEN_CLK			CONCAT2(CLK_,PWMCOUNT_TCC) 
	#define PWMCOUNT_IRQ				CONCAT2(PWMCOUNT_TCC,_0_IRQ)
	#define GCLK_PWMCOUNT				CONCAT2(GCLK_,PWMCOUNT_TCC)
	#define PID_PWMCOUNT				CONCAT2(PID_,PWMCOUNT_TCC)

	#if (PWMCOUNT_GEN_CLK > 100000000)
			#define PWMCOUNT_PRESC_NUM	1
	#elif (PWMCOUNT_GEN_CLK > 50000000)
			#define PWMCOUNT_PRESC_NUM	1
	#elif (PWMCOUNT_GEN_CLK > 20000000)
			#define PWMCOUNT_PRESC_NUM	1
	#elif (PWMCOUNT_GEN_CLK > 10000000)
			#define PWMCOUNT_PRESC_NUM	1
	#elif (PWMCOUNT_GEN_CLK > 5000000)
			#define PWMCOUNT_PRESC_NUM	1
	#else
			#define PWMCOUNT_PRESC_NUM	1
	#endif

	#define PWMCOUNT_PRESC_DIV			CONCAT2(TCC_PRESCALER_DIV,PWMCOUNT_PRESC_NUM)
	#define US2PWMCOUNT(v)				(((v)*(PWMCOUNT_GEN_CLK/PWMCOUNT_PRESC_NUM)+500000)/1000000)

	//#define PWMCOUNT_EVENT_GEN			CONCAT3(EVGEN_, PWMCOUNT_TCC, _OVF)
	#define PWMCOUNT0_EVSYS_USER		CONCAT3(EVSYS_USER_, PWMCOUNT_TCC, _EV_0)
	#define PWMCOUNT1_EVSYS_USER		CONCAT3(EVSYS_USER_, PWMCOUNT_TCC, _EV_1)

	inline void PWMCOUNT_ClockEnable()	{ HW::GCLK->PCHCTRL[GCLK_PWMCOUNT] = PWMCOUNT_GEN|GCLK_CHEN; HW::MCLK->ClockEnable(PID_PWMCOUNT); }

#else
	#error  Must defined PWMCOUNT_TCC
#endif


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef PWM_TCC

	#define PWMTCC						HW::PWM_TCC
	#define PWM_GEN						CONCAT2(GEN_,PWM_TCC)
	#define PWM_GEN_CLK					CONCAT2(CLK_,PWM_TCC) 
	#define PWM_IRQ						CONCAT2(PWM_TCC,_0_IRQ)
	#define GCLK_PWM					CONCAT2(GCLK_,PWM_TCC)
	#define PID_PWM						CONCAT2(PID_,PWM_TCC)

	#if (PWM_GEN_CLK > 100000000)
			#define PWM_PRESC_NUM		4
	#elif (PWM_GEN_CLK > 50000000)
			#define PWM_PRESC_NUM		2
	#elif (PWM_GEN_CLK > 20000000)
			#define PWM_PRESC_NUM		1
	#elif (PWM_GEN_CLK > 10000000)
			#define PWM_PRESC_NUM		1
	#elif (PWM_GEN_CLK > 5000000)
			#define PWM_PRESC_NUM		1
	#else
			#define PWM_PRESC_NUM		1
	#endif

	#define PWM_PRESC_DIV				CONCAT2(TCC_PRESCALER_DIV,PWM_PRESC_NUM)
	#define US2PWM(v)					(((v)*(PWM_GEN_CLK/PWM_PRESC_NUM/1000)+500)/1000)

	#define PWM_EVSYS_USER				CONCAT3(EVSYS_USER_, PWM_TCC, _EV_0)

	#define PWM_CC_NUM					CONCAT2(PWM_TCC,_CC_NUM)

	#define PWMLA_CC_NUM				(PWMLA_WO_NUM % PWM_CC_NUM)
	#define PWMHA_CC_NUM				(PWMHA_WO_NUM % PWM_CC_NUM)
	#define PWMLB_CC_NUM				(PWMLB_WO_NUM % PWM_CC_NUM)
	#define PWMHB_CC_NUM				(PWMHB_WO_NUM % PWM_CC_NUM)

	#define PWM_DMCH_TRIGSRC			CONCAT3(DMCH_TRIGSRC_, PWM_TCC, _OVF)

	inline void PWM_ClockEnable()		{ HW::GCLK->PCHCTRL[GCLK_PWM] = PWM_GEN|GCLK_CHEN; HW::MCLK->ClockEnable(PID_PWM); }

#else
	#error  Must defined PWM_TCC
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static u32 pwmStopTime = 0;
static u16 *ppwmdata = waveBuffer;
static bool pwmstat = true;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#pragma push
#pragma O3
#pragma Otime

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool IsFireOK()
{
	return pwmstat;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void DisableFire()
{
	PIO_URXD0->SetWRCONFIG(URXD0, PMUX_URXD0|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN); 
	PWMTCC->CTRLA &= ~TCC_ENABLE;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static __irq void PwmCountIRQ()
{
	PIO_DRVEN->CLR(DRVEN);
	PWMDMATCC->CTRLBSET = TCC_CMD_STOP;
	PWMTCC->CTRLBSET = TCC_CMD_STOP;
//	pwmStopTime = GetMilliseconds();
	PWMCOUNTTCC->INTFLAG = ~0;
	pwmstat = true;
	DisableFire();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static __irq void PwmDmaIRQ()
{
	HW::PIOB->BSET(23);
//	if (PWMDMATCC->INTFLAG & TCC_OVF)
	{
		u16 t = *(ppwmdata++);
		PWMTCC->CCBUF[0] = t;
		PWMTCC->CCBUF[1] = t;
		PWMTCC->CCBUF[2] = t;
		PWMTCC->CCBUF[3] = t;
		PWMTCC->INTFLAG = ~0;//TCC_OVF;
	};

	HW::PIOB->BCLR(23);
}

#pragma pop
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void PrepareFire(u16 waveAmp)
{
	pwmstat = false;
	ppwmdata = waveBuffer; 
	PIO_DRVEN->CLR(DRVEN);
	PWMTCC->PERBUF = US2PWM(pwmPeriodUS);
	//PWMLA_DMA.WritePeripheral(waveBuffer, &(PWMTCC->CCBUF[PWMLA_WO_NUM]), 50, DMCH_TRIGACT_BURST|PWMDMA_DMCH_TRIGSRC, DMDSC_BEATSIZE_HWORD); 
	//PWMHA_DMA.WritePeripheral(waveBuffer, &(PWMTCC->CCBUF[PWMHA_WO_NUM]), 10, DMCH_TRIGACT_BURST|PWMDMA_DMCH_TRIGSRC, DMDSC_BEATSIZE_HWORD); 
	//PWMLB_DMA.WritePeripheral(waveBuffer, &(PWMTCC->CCBUF[PWMLB_WO_NUM]), 10, DMCH_TRIGACT_BURST|PWMDMA_DMCH_TRIGSRC, DMDSC_BEATSIZE_HWORD); 
	//PWMHB_DMA.WritePeripheral(waveBuffer, &(PWMTCC->CCBUF[PWMHB_WO_NUM]), 10, DMCH_TRIGACT_BURST|PWMDMA_DMCH_TRIGSRC, DMDSC_BEATSIZE_HWORD); 

	waveLen = ((1000000 + waveFreq/2) / waveFreq + pwmPeriodUS/2) / pwmPeriodUS;

	PWMCOUNTTCC->PER = waveLen;
	PWMCOUNTTCC->CC[0] = waveLen;

	const float k = 2*pi/waveLen;
	const u16 hi = US2PWM(pwmPeriodUS-0.5);
	const u16 mid = US2PWM(pwmPeriodUS/2);
	const u16 lo = US2PWM(0.5);
	const u16 FV = MAX(curFireVoltage, 25);
	const u16 amp = (((u32)waveAmp*US2PWM(pwmPeriodUS)+FV/2)/FV+1)/2;

	for (u32 i = 0; i < waveLen; i++)
	{
		u16 t = mid + amp*sin(i*k);		
		//u16 t = mid + amp*sin(i*k)*(1-cos(i*k));
		waveBuffer[i] = LIM(t, lo, hi);
	};

	PWMTCC->CTRLA |= TCC_ENABLE;

	PIO_URXD0->SetWRCONFIG(URXD0, PORT_PMUX_A|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN); 
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void Init_PWM()
{
	PWM_ClockEnable();
	PWMDMA_ClockEnable();
	PWMCOUNT_ClockEnable();

	PIO_DRVEN->SET(DRVEN);
	PIO_DRVEN->DIRSET	= DRVEN;
	PIO_WF_PWM->DIRSET	= WF_PWM;
	PIO_PWM->DIRSET		= PWMLA|PWMHA|PWMLB|PWMHB;
	PIO_POL->DIRSET		= POLWLA|POLWHA|POLWLB|POLWHB;

	PIO_PWM->SetWRCONFIG(PWMLA, PMUX_PWMLA|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);
	PIO_PWM->SetWRCONFIG(PWMHA, PMUX_PWMHA|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);
	PIO_PWM->SetWRCONFIG(PWMLB, PMUX_PWMLB|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);
	PIO_PWM->SetWRCONFIG(PWMHB, PMUX_PWMHB|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);

	PIO_WF_PWM->SET(WF_PWM);
	PIO_POL->SET(POLWLA|POLWLB);

	PWMTCC->CTRLA = PWM_PRESC_DIV;
	PWMTCC->WAVE = TCC_WAVEGEN_NPWM|TCC_POL0|TCC_POL3;
	PWMTCC->DRVCTRL = TCC_NRE0|TCC_NRE1|TCC_NRE2|TCC_NRE3|TCC_NRV0|TCC_NRV2;//(TCC_INVEN0 << PWMHA_WO_NUM)|(TCC_INVEN0 << PWMHB_WO_NUM);
	PWMTCC->WEXCTRL = 0;
	PWMTCC->PER = US2PWM(pwmPeriodUS)-1;
	PWMTCC->CCBUF[PWMLA_WO_NUM] = US2PWM(pwmPeriodUS/2); 
	PWMTCC->CCBUF[PWMHA_WO_NUM] = US2PWM(pwmPeriodUS/2); 
	PWMTCC->CCBUF[PWMLB_WO_NUM] = US2PWM(pwmPeriodUS/2); 
	PWMTCC->CCBUF[PWMHB_WO_NUM] = US2PWM(pwmPeriodUS/2); 
	PWMTCC->EVCTRL = TCC_OVFEO|TCC_TCEI0|TCC_EVACT0_RETRIGGER;
	PWMTCC->INTENCLR = ~0;
	PWMTCC->INTENSET = TCC_OVF;
//	PWMTCC->CTRLA |= TCC_ENABLE;
	//PWMTCC->CTRLBSET = TCC_ONESHOT;

	HW::GCLK->PCHCTRL[EVENT_PWM_SYNC+GCLK_EVSYS0] = GCLK_GEN(GEN_MCK)|GCLK_CHEN;
	HW::GCLK->PCHCTRL[EVENT_PWMDMA+GCLK_EVSYS0] = GCLK_GEN(GEN_MCK)|GCLK_CHEN;
	//HW::GCLK->PCHCTRL[EVENT_PWMCOUNT+GCLK_EVSYS0] = GCLK_GEN(GEN_MCK)|GCLK_CHEN;

	HW::EIC->CTRLA = 0; while(HW::EIC->SYNCBUSY);

	HW::EIC->EVCTRL |= EIC_EXTINT0<<PWM_EXTINT;
	HW::EIC->SetConfig(PWM_EXTINT, 0, EIC_SENSE_FALL);
	HW::EIC->INTENCLR = EIC_EXTINT0<<PWM_EXTINT;
	HW::EIC->CTRLA = EIC_ENABLE;

	HW::EVSYS->CH[EVENT_PWM_SYNC].CHANNEL = (EVGEN_EIC_EXTINT_0+PWM_EXTINT)|EVSYS_PATH_ASYNCHRONOUS;
	HW::EVSYS->USER[PWM_EVSYS_USER] = EVENT_PWM_SYNC+1;

	PWMDMATCC->CTRLA = PWMDMA_PRESC_DIV;
	PWMDMATCC->WAVE = TCC_WAVEGEN_NPWM;
	PWMDMATCC->PER = US2PWMDMA(pwmPeriodUS)-1;
	PWMDMATCC->CC[0] = US2PWMDMA(1);
	PWMDMATCC->DRVCTRL = TCC_NRE0;
	PWMDMATCC->EVCTRL = TCC_OVFEO|TCC_TCEI0|TCC_EVACT0_RETRIGGER;
	PWMDMATCC->INTENCLR = ~0;
	//PWMDMATCC->INTENSET = TCC_OVF;
	PWMDMATCC->CTRLA |= TCC_ENABLE;
	//PWMDMATC->CTRLBSET = TC_CMD_RETRIGGER;

	VectorTableExt[PWM_IRQ] = PwmDmaIRQ;
	CM4::NVIC->CLR_PR(PWM_IRQ);
	CM4::NVIC->SET_ER(PWM_IRQ);

	HW::EVSYS->CH[EVENT_PWMDMA].CHANNEL = PWMDMA_EVENT_GEN|EVSYS_PATH_ASYNCHRONOUS;
	HW::EVSYS->USER[PWMDMA0_EVSYS_USER] = EVENT_PWM_SYNC+1;
	HW::EVSYS->USER[PWMDMA1_EVSYS_USER] = EVENT_PWMCOUNT+1;

	PWMCOUNTTCC->CTRLA = PWMCOUNT_PRESC_DIV;
	PWMCOUNTTCC->WAVE = TCC_WAVEGEN_NPWM;
	PWMCOUNTTCC->PER = waveLen;
	PWMCOUNTTCC->CC[0] = waveLen;
	PWMCOUNTTCC->EVCTRL = TCC_TCEI0|TCC_EVACT0_COUNT|TCC_TCEI1|TCC_EVACT1_RETRIGGER;
	PWMCOUNTTCC->INTENCLR = ~0;
	PWMCOUNTTCC->INTENSET = TCC_OVF;
	PWMCOUNTTCC->CTRLA |= TCC_ENABLE;
	PWMCOUNTTCC->CTRLBSET = TCC_ONESHOT;

	//HW::EVSYS->CH[EVENT_PWMCOUNT].CHANNEL = PWMCOUNT_EVENT_GEN|EVSYS_PATH_ASYNCHRONOUS;
	HW::EVSYS->USER[PWMCOUNT0_EVSYS_USER] = EVENT_PWMDMA+1;
	HW::EVSYS->USER[PWMCOUNT1_EVSYS_USER] = EVENT_PWM_SYNC+1;

	VectorTableExt[PWMCOUNT_IRQ] = PwmCountIRQ;
	CM4::NVIC->CLR_PR(PWMCOUNT_IRQ);
	CM4::NVIC->SET_ER(PWMCOUNT_IRQ);

	HW::PIOA->SetWRCONFIG(1<<14,  PORT_PMUX_F|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN); // PWMDMATCC WO[0]
	HW::PIOB->SetWRCONFIG(1<<16,  PORT_PMUX_F|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN); // PWMCOUNTTCC WO[0]

	//DACTC_DMA.WritePeripheral(waveBuffer, HW::DAC

	const float k = 2*pi/waveLen;
	const u16 hi = US2PWM(pwmPeriodUS-0.5);
	const u16 mid = US2PWM(pwmPeriodUS/2);
	const u16 lo = US2PWM(0.5);
	const u16 amp = 0;//US2PWM(pwmPeriodUS);

	for (u32 i = 0; i < waveLen; i++)
	{
		//u16 t = mid + (amp*0.375f)*sin(i*k)*(1-cos(i*k));
		u16 t = mid + (amp*0.5f)*sin(i*k);
		waveBuffer[i] = LIM(t, lo, hi);
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void Init_HV()
{
	static DSCI2C dsc;
	static byte wbuf[4];
	static TM32 tm;

	wbuf[0] = 3;	
	wbuf[1] = 1;	
	wbuf[2] = 0;	

	dsc.adr = 0x48;
	dsc.wdata = wbuf;
	dsc.wlen = 3;
	dsc.rdata = 0;
	dsc.rlen = 0;
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;

	I2C_AddRequest(&dsc);

	tm.Reset();	while (!dsc.ready && !tm.Check(10)) I2C_Update();

	wbuf[0] = 4;	
	wbuf[1] = 1;	
	wbuf[2] = 1;	

	dsc.adr = 0x48;
	dsc.wdata = wbuf;
	dsc.wlen = 3;
	dsc.rdata = 0;
	dsc.rlen = 0;
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;

	I2C_AddRequest(&dsc);

	tm.Reset();	while (!dsc.ready && !tm.Check(10)) I2C_Update();

	wbuf[0] = 8;	
	wbuf[1] = ~0;
	wbuf[2] = ~0;

	dsc.adr = 0x48;
	dsc.wdata = wbuf;
	dsc.wlen = 3;
	dsc.rdata = 0;
	dsc.rlen = 0;
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;

	I2C_AddRequest(&dsc);

	tm.Reset();	while (!dsc.ready && !tm.Check(10)) I2C_Update();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void InitHardware()
{
	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "Hardware Init ... \n");

#ifdef CPU_SAME53	
	
	using namespace HW;

	#ifdef _DEBUG

		SEGGER_RTT_printf(0, "HW::WDT->CTRLA = 0x%02X\n", (u32)(HW::WDT->CTRLA));

		SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_GREEN "Disable WDT ... ");

		HW::MCLK->APBAMASK |= APBA_WDT;

		while(HW::WDT->SYNCBUSY);

		if (HW::WDT->CTRLA & WDT_ENABLE)
		{
			HW::WDT->CTRLA = 0;
			
			while(HW::WDT->SYNCBUSY);
		};

		SEGGER_RTT_WriteString(0, "OK\n");

	#endif

//	HW::PIOA->BSET(13);

	HW::GCLK->GENCTRL[GEN_32K]	= GCLK_DIV(1)	|GCLK_SRC_OSCULP32K	|GCLK_GENEN;

	HW::GCLK->GENCTRL[GEN_1M]	= GCLK_DIV(25)	|GCLK_SRC_XOSC1		|GCLK_GENEN		|GCLK_OE;

	HW::GCLK->GENCTRL[GEN_25M]	= GCLK_DIV(1)	|GCLK_SRC_XOSC1		|GCLK_GENEN;

//	HW::GCLK->GENCTRL[GEN_500K] = GCLK_DIV(50)	|GCLK_SRC_XOSC1		|GCLK_GENEN;

	//PIO_32kHz->SetWRCONFIG(1UL<<PIN_32kHz, PORT_PMUX_M|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX|PORT_PULLEN);

	//HW::GCLK->GENCTRL[GEN_EXT32K]	= GCLK_DIV(1)	|GCLK_SRC_GCLKIN	|GCLK_GENEN		;


	HW::MCLK->APBAMASK |= APBA_EIC;
	HW::GCLK->PCHCTRL[GCLK_EIC] = GCLK_GEN(GEN_MCK)|GCLK_CHEN;

	EIC->CTRLA = EIC_SWRST; while(EIC->SYNCBUSY);

	HW::MCLK->APBBMASK |= APBB_EVSYS;
	EVSYS->CTRLA = EVSYS_SWRST;

	HW::GCLK->PCHCTRL[GCLK_SERCOM_SLOW]		= GCLK_GEN(GEN_32K)|GCLK_CHEN;	// 32 kHz

#endif

	Init_time(MCK);
	I2C_Init();
	//Init_HV();

	RTT_Init();

#ifndef WIN32

	//InitManRecieve();

	//InitManTransmit();

	EnableVCORE();
	
	Init_PWM();
	//Init_WaveFormGen();

	WDT_Init();

#else

	InitDisplay();

#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void UpdateHardware()
{
#ifndef WIN32

//	static TM32 tm;

	I2C_Update();
//	Update_PWM();

	//if (tm.Check(1000))
	//{
	//	HW::EVSYS->SWEVT = 1UL<<EVENT_PWM_SYNC;
	//	PWMTCC->CTRLA |= TCC_ENABLE;
	//};

#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
