#include "types.h"
#include "core.h"
//#include "COM_DEF.h"
//#include "CRC16_8005.h"

//#include "hardware.h"

#include "SEGGER_RTT.h"
#include "hw_conf.h"
#include "hardware.h"

#ifdef CPU_SAME53	
#define MAN_TRANSMIT_V2
#elif defined(CPU_XMC48)
#define MAN_TRANSMIT_V2
#endif

#ifndef MAN_SERCOM_NUM

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <manch_imp.h>

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#else

static byte stateManTrans = 0;
static MTB *manTB = 0;
static bool trmBusy = false;
static bool trmTurbo = false;
static bool rcvBusy = false;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_SAME53	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#ifdef MAN_SERCOM_NUM

		#define MANT_GEN					GEN_1M
		#define MANT_GEN_CLK				GEN_1M_CLK 
		#define GCLK_MANT					CONCAT3(GCLK_SERCOM,MAN_SERCOM_NUM,_CORE)
		#define PID_MANT					CONCAT2(PID_SERCOM,MAN_SERCOM_NUM)

		#define MANTSPI						CONCAT2(HW::SPI,MAN_SERCOM_NUM)

	#else
		#error  Must defined MANT_TC or MANT_TCC
	#endif
	
	#define BAUD2CLK(x)						((u32)(MANT_GEN_CLK/(2*(x))+0.5)-1)

	inline void MANT_ClockEnable()			{ HW::GCLK->PCHCTRL[GCLK_MANT] = MANT_GEN|GCLK_CHEN; HW::MCLK->ClockEnable(PID_MANT); }

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifndef WIN32

static const u16 manbaud[5] = { BAUD2CLK(20833), BAUD2CLK(41666), BAUD2CLK(62500), BAUD2CLK(83333), BAUD2CLK(104166) };//0:20833Hz, 1:41666Hz,2:62500Hz,3:83333Hz

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static u16 GetTrmBaudRate(byte i)
{
	if (i >= ArraySize(manbaud)) { i = ArraySize(manbaud) - 1; };

	return (manbaud[i]+1)/2;
}

#endif 

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef asadWIN32

#pragma push
//#pragma O3
//#pragma Otime

static __irq void ManTrmIRQ()
{
	static u32 tw = 0;
	static u16 count = 0;
	//static byte i = 0;
	static const u16* data = 0;
	static u16 len = 0;
	static bool cmd = false;

	Pin_ManTrmIRQ_Set();

	switch (stateManTrans)
	{
		case 0:	// 1-st sync imp 

			data = manTB->data1;
			len = manTB->len1;
			cmd = false;
			stateManTrans++;

		case 1:

			if (len == 0)
			{
				data = manTB->data2;
				len = manTB->len2;
				manTB->len2 = 0;
			};

			if (len != 0)
			{
				tw = ((u32)(*data) << 1) | (CheckParity(*data) & 1);

				data++;
				len--;

				count = 17;

				u32 tadd = (cmd) ? trmHalfPeriod : 0;

				MANT_SET_CR(trmHalfPeriod3+tadd);

				if (tw & 0x10000)
				{
					MANT_SET_PR(trmHalfPeriod7+tadd); //US2MT(96);
					stateManTrans += 2;
				}
				else
				{
					MANT_SET_PR(trmHalfPeriod6+tadd); //US2MT(72);
					stateManTrans++;
				};

				MANT_SHADOW_SYNC();

				tw <<= 1;
				count--;
			}
			else
			{
				stateManTrans = 4;
			};

			break;

		case 2:	

			MANT_SET_CR(trmHalfPeriod);

			if (count == 0)
			{
				MANT_SET_PR(trmHalfPeriod2);
				cmd = false;
				stateManTrans = 1;
			}
			else
			{
				if (tw & 0x10000)
				{
					MANT_SET_PR(trmHalfPeriod3);

					if (count == 1)
					{
						cmd = true;
						stateManTrans = 1;
					}
					else
					{
						stateManTrans++;
					};
				}
				else
				{
					MANT_SET_PR(trmHalfPeriod2);
				};

				tw <<= 1;
				count--;
			};

			MANT_SHADOW_SYNC();

			break;

		case 3: 

			if (tw & 0x10000)
			{
				MANT_SET_CR(trmHalfPeriod);
				MANT_SET_PR(trmHalfPeriod2);

				tw <<= 1;
				count--;

				if (count == 0)
				{
					cmd = true;
					stateManTrans = 1;
				};
			}
			else
			{
				tw <<= 1;
				count--;

				MANT_SET_CR(trmHalfPeriod2);

				if (tw & 0x10000)
				{
					MANT_SET_PR(trmHalfPeriod4);
					
					if (count == 1)
					{
						cmd = true;
						stateManTrans = 1;
					};
				}
				else
				{
					MANT_SET_PR(trmHalfPeriod3);

					if (count == 0)
					{
						cmd = false;
						stateManTrans = 1;
					}
					else
					{
						stateManTrans--;
					}
				};

//				if (count != 0)
				{
					tw <<= 1;
					count--;
				};
			};

			MANT_SHADOW_SYNC();

			break;

		case 4:

			stateManTrans++;
			break;

		case 5:

			stateManTrans = 0;

			ManDisableTransmit();

			manTB->ready = true;
			trmBusy = false;

			break;


	}; // 	switch (stateManTrans)

	ManEndIRQ();

	Pin_ManTrmIRQ_Clr();
}

#pragma pop

#endif
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool SendManData(MTB* mtb)
{
	#ifndef WIN32

		if (trmBusy || rcvBusy || mtb == 0 || mtb->data1 == 0 || mtb->len1 == 0)
		{
			return false;
		};

		mtb->ready = false;

		manTB = mtb;

		stateManTrans = 0;

		#ifdef CPU_SAME53	

			MANTSPI->BAUD = GetTrmBaudRate(mtb->baud);

			MNTTCC->CTRLA = MANT_PRESC_DIV;
			MNTTCC->WAVE = TCC_WAVEGEN_NPWM;//|TCC_POL0;
			MNTTCC->DRVCTRL = TCC_INVEN0 << L2_WO_NUM;

			MNTTCC->PER = US2MT(50)-1;
			MNTTCC->CC[L1_CC_NUM] = 0; 
			MNTTCC->CC[L2_CC_NUM] = ~0; 

			MNTTCC->EVCTRL = 0;

			MNTTCC->INTENCLR = ~0;
			MNTTCC->INTENSET = TCC_OVF;
			MNTTCC->INTFLAG = TCC_OVF;

			MNTTCC->CTRLA |= TCC_ENABLE;
			MNTTCC->CTRLBSET = TCC_CMD_RETRIGGER;

			PIO_MANCH->SetWRCONFIG(L1|L2, MANCH_PMUX|PORT_WRPMUX|PORT_WRPINCFG|PORT_PMUXEN);
			PIO_MANCH->CLR(L1|L2);
			PIO_MANCH->DIRSET = L1|L2;

		#endif

		return trmBusy = true;

	#else

		mtb->ready = true;

		return true;

	#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

void InitManTransmit()
{
	using namespace HW;

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_WHITE "Manchester Transmiter Init ... ");

#ifdef CPU_SAME53	

	byte MANT_IRQ = SERCOM0_0_IRQ + (MAN_SERCOM_NUM * 4);

	//VectorTableExt[MANT_IRQ] = ManTrmIRQ2;
	CM4::NVIC->CLR_PR(MANT_IRQ);
	CM4::NVIC->SET_ER(MANT_IRQ);

	MANT_ClockEnable();

	PIO_MAN->SetWRCONFIG(1UL<<PIN_MAN, PORT_PMUX_D|PORT_PMUXEN|PORT_WRPINCFG|PORT_WRPMUX);

	MANTSPI->CTRLA = SPI_SWRST;

	while (MANTSPI->SYNCBUSY);

	MANTSPI->CTRLA = MAN_DOPO|SERCOM_MODE_SPI_MASTER;
	MANTSPI->CTRLB = 0;
	MANTSPI->CTRLC = 0;
	MANTSPI->BAUD = BAUD2CLK(20833);

#elif defined(CPU_XMC48)

	VectorTableExt[MANT_CCU8_IRQ] = ManTrmIRQ2;
	CM4::NVIC->CLR_PR(MANT_CCU8_IRQ);
	CM4::NVIC->SET_ER(MANT_CCU8_IRQ);

	HW::CCU_Enable(MANT_CCU8_PID);

	MANT_CCU8->GCTRL = 0;
	MANT_CCU8->GIDLC = MANT_CCU8_GIDLC;
	MANT_CCU8->GCSS = MANT_OUT_GCSS;
	MANT_CCU8->GCSC = MANT_OUT_GCSC;

	MANT_SET_CHC_L1(MANT_CHC_L1);
	MANT_SET_CHC_L2(MANT_CHC_L2);
	MANT_SET_CHC_H1(MANT_CHC_H1);
	MANT_SET_CHC_H2(MANT_CHC_H2);

	PIO_L1->ModePin(PIN_L1, MANT_L1_PINMODE);
	PIO_L2->ModePin(PIN_L2, MANT_L2_PINMODE);

	#if defined(PIO_H1) && defined(PIO_H2)
		PIO_H1->ModePin(PIN_H1, MANT_H1_PINMODE);
		PIO_H2->ModePin(PIN_H2, MANT_H2_PINMODE);
	#endif

#endif

	SEGGER_RTT_WriteString(0, "OK\n");
}

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // #ifndef MAN_SERCOM_NUM
