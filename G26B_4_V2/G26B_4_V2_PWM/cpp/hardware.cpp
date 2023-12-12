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

static i16 sinArr[256] = {0};

#define pwmPeriodUS 6
//u16 waveAmp = 0;
//u16 waveFreq = 3000; //Hz
u16 waveLen = 2;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//static void InitVectorTable();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifndef WIN32

__forceinline 	void EnableVCORE()	{ PIO_ENVCORE->CLR(ENVCORE); 	}
__forceinline 	void DisableVCORE()	{ PIO_ENVCORE->SET(ENVCORE); 	}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <system_imp.h>

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void WDT_Init()
{
	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "WDT Init ... ");

	#ifdef CPU_SAME53	

		HW::MCLK->APBAMASK |= APBA_WDT;

		HW::WDT->CONFIG = WDT_WINDOW_CYC512|WDT_PER_CYC1024;
	
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
//	#error  Must defined PWMDMA_TCC
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

	#define PWMCOUNT_EVENT_GEN			CONCAT3(EVGEN_, PWMCOUNT_TCC, _OVF)
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


	#define PWM_CC_NUM					CONCAT2(PWM_TCC,_CC_NUM)

	#define PWMLA_CC_NUM				(PWMLA_WO_NUM % PWM_CC_NUM)
	#define PWMHA_CC_NUM				(PWMHA_WO_NUM % PWM_CC_NUM)
	#define PWMLB_CC_NUM				(PWMLB_WO_NUM % PWM_CC_NUM)
	#define PWMHB_CC_NUM				(PWMHB_WO_NUM % PWM_CC_NUM)

	#define PWM_EVENT_GEN				CONCAT3(EVGEN_, PWM_TCC, _OVF)
	#define PWM_EVSYS_USER				CONCAT3(EVSYS_USER_, PWM_TCC, _EV_0)

	#define PWM_DMCH_TRIGSRC			CONCAT3(DMCH_TRIGSRC_, PWM_TCC, _OVF)

	inline void PWM_ClockEnable()		{ HW::GCLK->PCHCTRL[GCLK_PWM] = PWM_GEN|GCLK_CHEN; HW::MCLK->ClockEnable(PID_PWM); }

#else
	#error  Must defined PWM_TCC
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef WFG_TC

	#define WFGTC						HW::WFG_TC
	#define WFG_GEN						CONCAT2(GEN_,WFG_TC)
	#define WFG_GEN_CLK					CONCAT2(CLK_,WFG_TC) 
	#define WFG_IRQ						CONCAT2(WFG_TC,_IRQ)
	#define GCLK_WFG					CONCAT2(GCLK_,WFG_TC)
	#define PID_WFG						CONCAT2(PID_,WFG_TC)

	#if (WFG_GEN_CLK > 100000000)
			#define WFG_PRESC_NUM		8
	#elif (WFG_GEN_CLK > 50000000)
			#define WFG_PRESC_NUM		4
	#elif (WFG_GEN_CLK > 20000000)
			#define WFG_PRESC_NUM		2
	#elif (WFG_GEN_CLK > 10000000)
			#define WFG_PRESC_NUM		1
	#elif (WFG_GEN_CLK > 5000000)
			#define WFG_PRESC_NUM		1
	#else
			#define WFG_PRESC_NUM		1
	#endif

	#define WFG_PRESC_DIV				CONCAT2(TC_PRESCALER_DIV,WFG_PRESC_NUM)
	#define US2WFG(v)					(((v)*(WFG_GEN_CLK/WFG_PRESC_NUM)+500000)/1000000)

	#define WFG_EVSYS_USER				CONCAT3(EVSYS_USER_, WFG_TC, _EVU)

	inline void WFG_ClockEnable()		{ HW::GCLK->PCHCTRL[GCLK_WFG] = WFG_GEN|GCLK_CHEN; HW::MCLK->ClockEnable(PID_WFG); }

#else
	#error  Must defined PWM_TCC
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef DAC_TC

	#define DACTC						HW::DAC_TC
	#define DACTC_GEN					CONCAT2(GEN_,DAC_TC)
	#define DACTC_GEN_CLK				CONCAT2(CLK_,DAC_TC) 
	#define DACTC_IRQ					CONCAT2(DAC_TC,_IRQ)
	#define GCLK_DACTC					CONCAT2(GCLK_,DAC_TC)
	#define PID_DACTC					CONCAT2(PID_,DAC_TC)

	#if (DACTC_GEN_CLK > 100000000)
			#define DACTC_PRESC_NUM		8
	#elif (DACTC_GEN_CLK > 50000000)
			#define DACTC_PRESC_NUM		4
	#elif (DACTC_GEN_CLK > 20000000)
			#define DACTC_PRESC_NUM		2
	#elif (DACTC_GEN_CLK > 10000000)
			#define DACTC_PRESC_NUM		1
	#elif (DACTC_GEN_CLK > 5000000)
			#define DACTC_PRESC_NUM		1
	#else
			#define DACTC_PRESC_NUM		1
	#endif

	#define DACTC_PRESC_DIV				CONCAT2(TC_PRESCALER_DIV,DACTC_PRESC_NUM)
	#define US2DACTC(v)					(((v)*(DACTC_GEN_CLK/DACTC_PRESC_NUM)+500000)/1000000)

	inline void DACTC_ClockEnable()		{ HW::GCLK->PCHCTRL[GCLK_DACTC] = DACTC_GEN|GCLK_CHEN; HW::MCLK->ClockEnable(PID_DACTC); }

#else
	#error  Must defined DAC_TC
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
	HW::EVSYS->CH[EVENT_PWM_SYNC].CHANNEL = EVGEN_NONE;
	PIO_URXD0->SetWRCONFIG(URXD0, PMUX_URXD0|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN); 
	PWMTCC->CTRLA &= ~TCC_ENABLE;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static __irq void PwmCountIRQ()
{
	HW::PIOB->BSET(23);

	PIO_DRVEN->CLR(DRVEN);
	PWMTCC->CTRLBSET = TCC_CMD_STOP;
	//WFGTC->CTRLBSET = TC_CMD_STOP;
//	pwmStopTime = GetMilliseconds();
	PWMCOUNTTCC->INTFLAG = ~0;
	pwmstat = true;
	DisableFire();
	PWM_DMA.Disable();

	HW::PIOB->BCLR(23);
}

#pragma pop

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void PrepareFire(u16 waveFreq, u16 waveAmp, bool pwm)
{
	static u16 prevFreq = 0;
	static u16 prevAmp = 0;

	u16 hi, mid, lo, amp;
	volatile void *dst;

	HW::PIOA->BSET(21);

	pwmstat = false;
	ppwmdata = waveBuffer; 
	PIO_DRVEN->CLR(DRVEN);
	PWMTCC->PERBUF = US2PWM(pwmPeriodUS);

	//if (waveFreq != prevFreq || waveAmp != prevAmp)
	{
		prevFreq = waveFreq;
		prevAmp = waveAmp;

		waveLen = ((1000000 + waveFreq/2) / waveFreq + pwmPeriodUS/2) / pwmPeriodUS;

		waveAmp /= 8;

		const u16 FV = MAX(curFireVoltage, 25);

		if (pwm)
		{
			hi = US2PWM(pwmPeriodUS-0.5);
			mid = US2PWM(pwmPeriodUS/2);
			lo = US2PWM(0.5);
			amp = (((u32)waveAmp*US2PWM(pwmPeriodUS)+FV/2)/FV+1)/2;
			dst = &(PWMTCC->CCBUF[0]);
		}
		else
		{
			hi = 4095;
			mid = 0x7FF;
			lo = 0;
			amp = 64;
			dst = &(HW::DAC->DATA[0]);
		};

		const u16 ki = 256 * ArraySize(sinArr) / waveLen;

		for (u32 i = 0; i < waveLen; i++)
		{
			u16 t = mid + (amp*sinArr[(i*ki+127)>>8])/2048;		
			waveBuffer[i] = LIM(t, lo, hi);
		};
	};

	PWMCOUNTTCC->PER = waveLen;
	PWMCOUNTTCC->CC[0] = waveLen;

	PWMTCC->CTRLA |= TCC_ENABLE;

	PWM_DMA.WritePeripheral(waveBuffer, dst, waveLen, DMCH_TRIGACT_BURST|PWM_DMCH_TRIGSRC, DMDSC_BEATSIZE_HWORD); 

	PIO_URXD0->SetWRCONFIG(URXD0, PORT_PMUX_A|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN); 
	HW::EVSYS->CH[EVENT_PWM_SYNC].CHANNEL = (EVGEN_EIC_EXTINT_0+PWM_EXTINT)|EVSYS_PATH_ASYNCHRONOUS;
	HW::EVSYS->SWEVT = 1;

	HW::PIOA->BCLR(21);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void Init_PWM()
{
	PWM_ClockEnable();
	PWMCOUNT_ClockEnable();

	PIO_DRVEN->SET(DRVEN);
	PIO_DRVEN->DIRSET	= DRVEN;
	PIO_WF_PWM->DIRSET	= WF_PWM;
	PIO_PWML->DIRSET	= PWMLA|PWMLB;
	PIO_PWMH->DIRSET	= PWMHA|PWMHB;
	PIO_POL->DIRSET		= POLWLA|POLWHA|POLWLB|POLWHB;

	PIO_PWML->SetWRCONFIG(PWMLA, PMUX_PWMLA|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);
	PIO_PWMH->SetWRCONFIG(PWMHA, PMUX_PWMHA|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);
	PIO_PWML->SetWRCONFIG(PWMLB, PMUX_PWMLB|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);
	PIO_PWMH->SetWRCONFIG(PWMHB, PMUX_PWMHB|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);

	PIO_WF_PWM->SET(WF_PWM);
	PIO_POL->CLR(POLWLA|POLWHA|POLWLB|POLWHB);

	PWMTCC->CTRLA = PWM_PRESC_DIV;
	PWMTCC->WAVE = TCC_WAVEGEN_NPWM|TCC_SWAP0;
	PWMTCC->DRVCTRL = TCC_NRE0|TCC_NRE1|TCC_NRE4|TCC_NRE5|TCC_NRV4|TCC_NRV5;//(TCC_INVEN0 << PWMHA_WO_NUM)|(TCC_INVEN0 << PWMHB_WO_NUM);
	PWMTCC->WEXCTRL = 0x01010F02;
	PWMTCC->PER = US2PWM(pwmPeriodUS)-1;
	PWMTCC->CCBUF[0] = US2PWM(pwmPeriodUS/2); 
	PWMTCC->EVCTRL = TCC_OVFEO|TCC_TCEI0|TCC_EVACT0_RETRIGGER;
	PWMTCC->INTENCLR = ~0;

	HW::GCLK->PCHCTRL[EVENT_PWM_SYNC+GCLK_EVSYS0] = GCLK_GEN(GEN_MCK)|GCLK_CHEN;
	HW::GCLK->PCHCTRL[EVENT_PWM+GCLK_EVSYS0] = GCLK_GEN(GEN_MCK)|GCLK_CHEN;
	HW::GCLK->PCHCTRL[EVENT_PWMCOUNT+GCLK_EVSYS0] = GCLK_GEN(GEN_MCK)|GCLK_CHEN;

	HW::EIC->CTRLA = 0; while(HW::EIC->SYNCBUSY);

	HW::EIC->EVCTRL |= EIC_EXTINT0<<PWM_EXTINT;
	HW::EIC->SetConfig(PWM_EXTINT, 0, EIC_SENSE_FALL);
	HW::EIC->INTENCLR = EIC_EXTINT0<<PWM_EXTINT;
	HW::EIC->CTRLA = EIC_ENABLE;

	HW::EVSYS->CH[EVENT_PWM_SYNC].CHANNEL = (EVGEN_EIC_EXTINT_0+PWM_EXTINT)|EVSYS_PATH_ASYNCHRONOUS;
	HW::EVSYS->USER[PWM_EVSYS_USER] = EVENT_PWM_SYNC+1;

	HW::EVSYS->CH[EVENT_PWM].CHANNEL = PWM_EVENT_GEN|EVSYS_PATH_ASYNCHRONOUS;
	HW::EVSYS->USER[PWMCOUNT0_EVSYS_USER] = EVENT_PWM+1;

	PWMCOUNTTCC->CTRLA = PWMCOUNT_PRESC_DIV;
	PWMCOUNTTCC->WAVE = TCC_WAVEGEN_NPWM;
	PWMCOUNTTCC->PER = waveLen;
	PWMCOUNTTCC->CC[0] = waveLen;
	PWMCOUNTTCC->EVCTRL = TCC_TCEI0|TCC_EVACT0_COUNT|TCC_TCEI1|TCC_EVACT1_RETRIGGER;
	PWMCOUNTTCC->INTENCLR = ~0;
	PWMCOUNTTCC->INTENSET = TCC_OVF;
	PWMCOUNTTCC->CTRLA |= TCC_ENABLE;
	PWMCOUNTTCC->CTRLBSET = TCC_ONESHOT;

	HW::EVSYS->CH[EVENT_PWMCOUNT].CHANNEL = PWMCOUNT_EVENT_GEN|EVSYS_PATH_ASYNCHRONOUS;
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

	for (u32 i = 0; i < waveLen; i++) waveBuffer[i] = mid;
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

static void Init_WaveFormGen()
{
	WFG_ClockEnable();
	//DACTC_ClockEnable();


	PIO_GEN->DIRSET		= GENA|GENB;

	PIO_GEN->SetWRCONFIG(GENA, PMUX_GENA|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);
	PIO_GEN->SetWRCONFIG(GENB, PMUX_GENB|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);

	WFGTC->CTRLA = WFG_PRESC_DIV|TC_MODE_COUNT8;
	WFGTC->WAVE = TC_WAVEGEN_NPWM;
	WFGTC->DRVCTRL = TC_INVEN0 << GENB_WO_NUM;

	WFGTC->PER8 = US2WFG(pwmPeriodUS)-1;
	WFGTC->CC8[GENA_WO_NUM] = US2WFG(pwmPeriodUS/2); 
	WFGTC->CC8[GENB_WO_NUM] = US2WFG(pwmPeriodUS/2); 

	//WFGTC->EVCTRL = TC_TCEI|TC_EVACT_RETRIGGER;
	//HW::EVSYS->USER[WFG_EVSYS_USER] = EVENT_PWM_SYNC+1;

	WFGTC->INTENCLR = ~0;

	WFGTC->CTRLA |= TC_ENABLE;
	WFGTC->CTRLBSET = TC_CMD_RETRIGGER;


	PIO_DAC0->SetWRCONFIG(DAC0, PORT_PMUX_B|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);

	HW::GCLK->GENCTRL[GEN_DAC] = GCLK_DIV(GEN_DAC_DIV)|GCLK_SRC_DPLL0|GCLK_GENEN;
	HW::GCLK->PCHCTRL[GCLK_DAC] = GEN_DAC|GCLK_CHEN; 
	HW::MCLK->ClockEnable(PID_DAC); 

	HW::DAC->CTRLA = DAC_SWRST;

	while(HW::DAC->SYNCBUSY);

	HW::DAC->CTRLB = DAC_REFSEL_VREFAU;
	HW::DAC->EVCTRL = 0;
	HW::DAC->INTENCLR = ~0;
	HW::DAC->DACCTRL[0] = DAC_ENABLE|DAC_CC12M|DAC_RUNSTDBY|DAC_REFRESH(1);
	HW::DAC->DACCTRL[1] = 0;
	HW::DAC->CTRLA = DAC_ENABLE;

	while(HW::DAC->SYNCBUSY);

	HW::DAC->DATA[0] = 0x7FF;


	//DACTC->CTRLA = DACTC_PRESC_DIV|TC_MODE_COUNT8;
	//DACTC->WAVE = TC_WAVEGEN_NPWM;

	//DACTC->PER8 = US2DACTC(10)-1;
	//DACTC->EVCTRL = 0;

	//DACTC->INTENCLR = ~0;

	//DACTC->CTRLA |= TC_ENABLE;
	//DACTC->CTRLBSET = TC_CMD_RETRIGGER;

	const float k = 2*pi/ArraySize(sinArr);

	for (u32 i = 0; i < ArraySize(sinArr); i++)
	{
		sinArr[i] = 1861*sin(i*k);		
	};

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void Init_ADC()
{
	HW::GCLK->PCHCTRL[GCLK_ADC0] = GEN_MCK|GCLK_CHEN; 
	HW::GCLK->PCHCTRL[GCLK_ADC1] = GEN_MCK|GCLK_CHEN; 
	HW::MCLK->ClockEnable(PID_ADC0); 
	HW::MCLK->ClockEnable(PID_ADC1); 

	PIO_ADC300->SetWRCONFIG(ADC300, PORT_PMUX_B|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);
	PIO_ADCWFB->SetWRCONFIG(ADCWFB, PORT_PMUX_B|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);

	HW::ADC0->CTRLB = ADC_RESSEL_16BIT|ADC_FREERUN;
	HW::ADC1->CTRLB = ADC_RESSEL_12BIT|ADC_FREERUN;

	HW::ADC0->REFCTRL = ADC_REFSEL_VDDANA|ADC_REFCOMP;
	HW::ADC1->REFCTRL = ADC_REFSEL_VDDANA|ADC_REFCOMP;

	HW::ADC0->INPUTCTRL = ADC_MUXPOS_AIN7;
	HW::ADC1->INPUTCTRL = ADC_MUXPOS_AIN6;

	HW::ADC0->AVGCTRL = ADC_SAMPLENUM_1024;
	HW::ADC1->AVGCTRL = ADC_SAMPLENUM_1;

	HW::ADC0->SAMPCTRL = ADC_SAMPLEN(0);
	HW::ADC1->SAMPCTRL = ADC_SAMPLEN(0);

	HW::ADC0->CTRLA = ADC_PRESCALER_DIV16|ADC_ENABLE; 
	HW::ADC1->CTRLA = ADC_PRESCALER_DIV16|ADC_ENABLE; 

	HW::ADC0->SWTRIG = ADC_START;
	HW::ADC1->SWTRIG = ADC_START;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u16 GetCurFireVoltage()
{
	return (HW::ADC0->RESULT * 31938) >> 22;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

	InitManRecieve();

	InitManTransmit();

	EnableVCORE();
	
	Init_ADC();
	Init_PWM();
	Init_WaveFormGen();

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

	//static byte t = 0;

	if (HW::DAC->SYNCBUSY == 0 && pwmstat) HW::DAC->DATA[0] = /*sinArr[t++]+*/0x7ff;

//	Update_PWM();

	//if (tm.Check(1000))
	//{
	//	HW::EVSYS->SWEVT = 1UL<<EVENT_PWM_SYNC;
	//	PWMTCC->CTRLA |= TCC_ENABLE;
	//};

#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
