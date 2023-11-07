#include "hardware.h"
//#include "options.h"
//#include "hw_emac.h"
#include "CRC16.h"
#include "ComPort.h"
#include "CRC16_CCIT.h"
#include "list.h"
#include "PointerCRC.h"
#include "SEGGER_RTT.h"
#include "hw_conf.h"
#include "hw_com.h"

#ifdef WIN32

#include <conio.h>
//#include <stdio.h>

//static const bool __WIN32__ = true;

#else

//static const bool __WIN32__ = false;

//#pragma diag_suppress 546,550,177

#endif

enum { VERSION = 0x101 };

//#pragma O3
//#pragma Otime

#ifndef _DEBUG
	static const bool __debug = false;
#else
	static const bool __debug = true;
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct MainVars // NonVolatileVars  
{
	u32 timeStamp;

	u16 numDevice;
	u16 numMemDevice;

	u16 freq;
	u16 fireVoltage;
	u16 fireAmp;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static MainVars mv;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u32 req40_count1 = 0;
u32 req40_count2 = 0;
u32 req40_count3 = 0;

u32 fps;
i16 tempClock = 0;
i16 cpu_temp = 0;
u32 i2cResetCount = 0;

//inline u16 ReverseWord(u16 v) { __asm	{ rev16 v, v };	return v; }

//static void* eepromData = 0;
//static u16 eepromWriteLen = 0;
//static u16 eepromReadLen = 0;
//static u16 eepromStartAdr = 0;

//static MTB mtb;

//static u16 manBuf[16];

//static u16 manRcvData[10];
static u16 manTrmData[50];
static u16 manTrmBaud = 0;
//static u16 memTrmBaud = 0;

u16 txbuf[128 + 512 + 16];


//static u16 mode = 0;

//static TM32 imModeTimeout;

//static u16 motoRcvCount = 0;

u16 curFireVoltage = 300;
//u16 dstFireVoltage = 0;

//static u32 dspMMSEC = 0;
//static u32 shaftMMSEC = 0;

//const u16 dspReqWord = 0xA900;
//const u16 dspReqMask = 0xFF00;

static u16 manReqWord = 0xA800;
static u16 manReqMask = 0xFF00;

//static u16 memReqWord = 0x3E00;
//static u16 memReqMask = 0xFF00;

//static u16 numDevice = 0;
static u16 verDevice = VERSION;

//static u16 numMemDevice = 0;
//static u16 verMemDevice = 0x100;

//static u32 manCounter = 0;
//static u32 fireCounter = 0;

//static byte mainModeState = 0;
//static byte dspStatus = 0;

//static bool cmdWriteStart_00 = false;
//static bool cmdWriteStart_10 = false;
//static bool cmdWriteStart_20 = false;

//static u32 dspRcv40 = 0;
//static u32 dspRcv50 = 0;
//static u16 dspRcvCount = 0;
//static u16 dspRcv30 = 0;
//static u16 dspRcv01 = 0;


//static u32 rcvCRCER = 0;

//static u32 chnlCount[4] = {0};

//static u32 crcErr02 = 0;
//static u32 crcErr03 = 0;
//static u32 crcErr06 = 0;
//static u32 wrtErr06 = 0;

//static u32 notRcv02 = 0;
//static u32 lenErr02 = 0;

//static i16 ax = 0, ay = 0, az = 0, at = 0;
i16 temperature = 0;
i16 cpuTemp = 0;
i16 temp = 0;

static byte svCount = 0;


//struct AmpTimeMinMax { bool valid; u16 ampMax, ampMin, timeMax, timeMin; };

//static AmpTimeMinMax sensMinMax[2] = { {0, 0, ~0, 0, ~0}, {0, 0, ~0, 0, ~0} };
//static AmpTimeMinMax sensMinMaxTemp[2] = { {0, 0, ~0, 0, ~0}, {0, 0, ~0, 0, ~0} };

//static u32 testDspReqCount = 0;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void SaveMainParams()
{
	svCount = 1;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void SetNumDevice(u16 num)
{
	mv.numDevice = num;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern u16 GetNumDevice()
{
	return mv.numDevice;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern u16 GetVersionDevice()
{
	return verDevice;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static void Response_0(u16 rw, MTB &mtb)
//{
//	__packed struct Rsp {u16 rw; u16 device; u16 session; u32 rcvVec; u32 rejVec; u32 wrVec; u32 errVec; u16 wrAdr[3]; u16 numDevice; u16 version; u16 temp; byte status; byte flags; RTC rtc; };
//
//	Rsp &rsp = *((Rsp*)&txbuf);
//
//	rsp.rw = rw;
//	rsp.device = GetDeviceID();  
//	rsp.session = FLASH_Session_Get();	  
//	rsp.rcvVec =  FLASH_Vectors_Recieved_Get();
//	rsp.rejVec = FLASH_Vectors_Rejected_Get();
//	rsp.wrVec = FLASH_Vectors_Saved_Get();
//	rsp.errVec = FLASH_Vectors_Errors_Get();
//	*((__packed u64*)rsp.wrAdr) = FLASH_Current_Adress_Get();
//	rsp.temp = temp*5/2;
//	rsp.status = FLASH_Status();
//
//	GetTime(&rsp.rtc);
//
//	mtb.data1 = txbuf;
//	mtb.len1 = sizeof(rsp)/2;
//	mtb.data2 = 0;
//	mtb.len2 = 0;
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static void InitRmemList()
//{
//	for (u16 i = 0; i < ArraySize(r02); i++)
//	{
//		freeR01.Add(&r02[i]);
//	};
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static u32 InitRspMan_00(__packed u16 *data)
{
	__packed u16 *start = data;

	*(data++)	= (manReqWord & manReqMask) | 0;
	*(data++)	= mv.numDevice;
	*(data++)	= verDevice;
	
	return data - start;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_00(u16 *data, u16 len, MTB* mtb)
{
	if (data == 0 || len == 0 || len > 2 || mtb == 0) return false;

	InitRspMan_00(manTrmData);

	mtb->data1 = manTrmData;
	mtb->len1 = 3;
	mtb->data2 = 0;
	mtb->len2 = 0;

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static u32 InitRspMan_10(__packed u16 *data)
{
	__packed u16 *start = data;

	*(data++)	= (manReqWord & manReqMask) | 0x10;		//	1. ответное слово
	*(data++)	= 0;									//	2. И1И2П1П2. КУ
	*(data++)	= 0;									//	3. И1И2П1П2. Шаг оцифровки
	*(data++)	= 0;									//	4. И1И2П1П2. Длина оцифровки
	*(data++)	= 0; 									//	5. И1И2П1П2. Задержка оцифровки
	*(data++)	= 0;									//	6. И1И2П1П2. Фильтр
	*(data++)	= 0;									//	7. И1И2П1П2. Упаковка
	*(data++)	= 0;									//	8. И3П1П2. КУ
	*(data++)	= 0;									//	9. И3П1П2. Шаг оцифровки
	*(data++)	= 0;									//	10. И3П1П2. Длина оцифровки
	*(data++)	= 0;									//	11. И3П1П2. Задержка оцифровки
	*(data++)	= 0;									//	12. И3П1П2. Фильтр
	*(data++)	= 0;									//	13. И3П1П2. Упаковка
	*(data++)	= mv.fireVoltage;						//	14. И3П1П2. Напряжение излучателя (0...300 В)
	*(data++)	= mv.freq;								//	15. И3П1П2. Частота излучателя (1000...30000 Гц)
	*(data++)	= mv.fireAmp;							//	16. И3П1П2. Амплитуда излучателя (0...3000 В)

	return data - start;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_10(u16 *data, u16 len, MTB* mtb)
{
	if (data == 0 || len == 0 || len > 2 || mtb == 0) return false;

	len = InitRspMan_10(manTrmData);

	mtb->data1 = manTrmData;
	mtb->len1 = len;
	mtb->data2 = 0;
	mtb->len2 = 0;

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static u32 InitRspMan_20(u16 *req, __packed u16 *data)
{
	__packed u16 *start = data;

	*(data++)	= req[0];				//1. ответное слово
	*(data++)	= 0;					//2-3. счётчик срабатываний
	*(data++)	= 0;					//2-3. счётчик срабатываний
	*(data++)	= 0;					//4. Температура в приборе(0.1гр) (short)
	*(data++)  	= 0;					//5. Статус
	*(data++)  	= 0;					//6. Счётчик ошибок И3П1П2 по RS485
	*(data++)  	= curFireVoltage;		//7. Напряжение излучателя измеренное И3П1П2 (В)
	*(data++)  	= mv.fireAmp;			//8. Аплитуда излучателя измеренная И3П1П2 (В)
	*(data++)	= temp; 				//9. Температура излучателя измеренная И3П1П2(0.1гр) (short)

	return data - start;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_20(u16 *data, u16 len, MTB* mtb)
{
	if (data == 0 || len == 0 || len > 2 || mtb == 0) return false;

	len = InitRspMan_20(data, manTrmData);

	mtb->data1 = manTrmData;
	mtb->len1 = len;
	mtb->data2 = 0;
	mtb->len2 = 0;

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static bool RequestMan_80(u16 *data, u16 len, MTB* mtb)
//{
//	if (data == 0 || len < 3 || len > 4 || mtb == 0) return false;
//
//	switch (data[1])
//	{
//		case 1:
//
//			mv.numDevice = data[2];
//
//			break;
//
//		case 2:
//
//			manTrmBaud = data[2] - 1;	//SetTrmBoudRate(data[2]-1);
//
//			break;
//	};
//
//	manTrmData[0] = (manReqWord & manReqMask) | 0x80;
//
//	mtb->data1 = manTrmData;
//	mtb->len1 = 1;
//	mtb->data2 = 0;
//	mtb->len2 = 0;
//
//	return true;
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_90(u16 *data, u16 len, MTB* mtb)
{
	if (data == 0 || len < 3 || len > 4 || mtb == 0) return false;

	switch(data[1])
	{
		case 0x17:	mv.fireVoltage		= MIN(data[2], 300);			break;
		case 0x18:	mv.freq				= LIM(data[2], 2000, 10000);	break;
		case 0x19:	mv.fireAmp			= MIN(data[2], 1500);			break;

		default:

			return false;
	};

	manTrmData[0] = (manReqWord & manReqMask) | 0x90;

	mtb->data1 = manTrmData;
	mtb->len1 = 1;
	mtb->data2 = 0;
	mtb->len2 = 0;

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan_F0(u16 *data, u16 len, MTB* mtb)
{
	if (data == 0 || len == 0 || len > 2 || mtb == 0) return false;

	SaveMainParams();

	manTrmData[0] = (manReqWord & manReqMask) | 0xF0;

	mtb->data1 = manTrmData;
	mtb->len1 = 1;
	mtb->data2 = 0;
	mtb->len2 = 0;

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestMan(u16 *buf, u16 len, MTB* mtb)
{
	if (buf == 0 || len == 0 || mtb == 0) return false;

	bool r = false;

	byte i = (buf[0]>>4)&0xF;

	switch (i)
	{
		case 0: 	r = RequestMan_00(buf, len, mtb); break;
		case 1: 	r = RequestMan_10(buf, len, mtb); break;
		case 2: 	r = RequestMan_20(buf, len, mtb); break;
//		case 3:		r = RequestMan_30(buf, len, mtb); break;
//		case 8: 	r = RequestMan_80(buf, len, mtb); break;
		case 9:		r = RequestMan_90(buf, len, mtb); break;
		case 0xF:	r = RequestMan_F0(buf, len, mtb); break;
	};

	if (r) { mtb->baud = manTrmBaud; };

	return r;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static void UpdateMan()
//{
//	static MTB mtb;
//	static MRB mrb;
//
//	static byte i = 0;
//
//	static RTM tm;
//
//
////	u16 c;
//
//	switch (i)
//	{
//		case 0:
//
////			HW::P5->BSET(7);
//
//			mrb.data = manRcvData;
//			mrb.maxLen = ArraySize(manRcvData);
//			RcvManData(&mrb);
//
//			i++;
//
//			break;
//
//		case 1:
//
//			ManRcvUpdate();
//
//			if (mrb.ready)
//			{
//				tm.Reset();
//
//				if (mrb.OK && mrb.len > 0 && ((manRcvData[0] & manReqMask) == manReqWord && RequestMan(manRcvData, mrb.len, &mtb)))
//				{
//					i++;
//				}
//				else
//				{
//					i = 0;
//				};
//			}
//			else if (mrb.len > 0)
//			{
//
//			};
//
//			break;
//
//		case 2:
//
//			if (tm.Check(US2RT(100)))
//			{
//				//manTrmData[0] = 1;
//				//manTrmData[1] = 0;
//				//mtb.len1 = 2;
//				//mtb.data1 = manTrmData;
//				SendManData(&mtb);
//
//				i++;
//			};
//
//			break;
//
//		case 3:
//
//			if (mtb.ready)
//			{
//				i = 0;
//			};
//
//			break;
//
//	};
//}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateCom()
{
	static byte i = 0;
	static CTM32 ctm;
	static ComPort::WriteBuffer wb;
	static ComPort::ReadBuffer rb;
	static MTB mtb;
	static u16 buf[16];
	
	switch(i)
	{
		case 0:

			rb.data = buf;
			rb.maxLen = sizeof(buf);
			comdsp.Read(&rb, MS2COM(5000), US2COM(100));
			i++;

			break;

		case 1:

			if (!comdsp.Update())
			{
				if (rb.recieved && rb.len > 0)
				{
					ctm.Reset();

					if (RequestMan(buf, rb.len/2, &mtb))
					{
						wb.data = (void*)mtb.data1;
						wb.len = mtb.len1*2;
						comdsp.Write(&wb);
						i++;
					}
					else
					{
						i = 0;
					};
				}
				else
				{
					i = 0;
				};
			};

			break;

		case 2:

			if (!comdsp.Update())
			{
				i = ((buf[0]&0xF4) == 0x24) ? (i+1) : 0;
			};

			break;

		case 3:

			PrepareFire(mv.freq, mv.fireAmp, true);

			i++;

			break;

		case 4:

			if (IsFireOK() || ctm.Check(MS2CLK(50)))
			{
				DisableFire();

				i = 0;
			};

			break;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void MainMode()
{
	//static Ptr<REQ> rq;
	////static Ptr<UNIBUF> flwb;
	//static TM32 tm;
	//static RspDsp30 *rsp = 0;

	//switch (mainModeState)
	//{
	//	case 0:

	//		rq = readyR01.Get();

	//		if (rq.Valid())
	//		{
	//			rsp = (RspDsp30*)(rq->rsp->GetDataPtr());

	//			mainModeState++;
	//		};

	//		break;

	//	case 1:
	//	{
	//		byte n = rsp->h.rw & 15;

	//		if (n == 0)
	//		{
	//			if (RequestFlashWrite_20()) mainModeState++;
	//		}
	//		else
	//		{
	//			mainModeState++;
	//		};

	//		break;
	//	};

	//	case 2:
	//	{
	//		RequestFlashWrite(rq->rsp, rsp->h.rw, false);

	//		byte n = rsp->h.rw & 15;

	//		manVec30[n] = rq->rsp;

	//		rq.Free();

	//		mainModeState++;

	//		break;
	//	};

	//	case 3:

	//		if (cmdWriteStart_00)
	//		{
	//			cmdWriteStart_00 = !RequestFlashWrite_00();
	//		}
	//		else if (cmdWriteStart_10)
	//		{
	//			cmdWriteStart_10 = !RequestFlashWrite_10();
	//		};

	//		mainModeState = 0;

	//		break;
	//};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateTemp()
{
	static byte i = 0;

	static DSCI2C dsc, dsc2;

//	static byte reg = 0;
	static u16 rbuf = 0;
	static byte buf[10];

	static TM32 tm;

	switch (i)
	{
		case 0:

			if (tm.Check(100))
			{
#ifndef WIN32
				if (!__debug) { HW::ResetWDT(); };
#endif
				buf[0] = 0;

				dsc.adr = 0x49;
				dsc.wdata = buf;
				dsc.wlen = 1;
				dsc.rdata = &rbuf;
				dsc.rlen = 2;
				dsc.wdata2 = 0;
				dsc.wlen2 = 0;

				if (I2C_AddRequest(&dsc))
				{
					i++;
				};
			};

			break;

		case 1:

			if (dsc.ready)
			{
				if (dsc.ack && dsc.readedLen == dsc.rlen)
				{
					i32 t = (i16)ReverseWord(rbuf);

					temp = (t * 10 + 64) / 128;
				};

				i = 0;
			};

			break;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateHV()
{
	static byte i = 0;
	static DSCI2C dsc;
	static byte wbuf[4];
	static byte rbuf[4];
	static TM32 tm;
	//static CTM32 ctm;
	static i32 filtFV = 0;
	//static i32 filtMV = 0;
	static u16 correction = 0x200;
	static u16 dstFV = 0;

	//if (!ctm.Check(US2CCLK(50))) return;

	switch (i)
	{
		case 0:

			if (tm.Check(10))
			{
				wbuf[0] = 2;
				wbuf[1] = 0;
				wbuf[2] = 0;

				dsc.adr = 0x48;
				dsc.wdata = wbuf;
				dsc.wlen = 3;
				dsc.rdata = 0;
				dsc.rlen = 0;
				dsc.wdata2 = 0;
				dsc.wlen2 = 0;

				I2C_AddRequest(&dsc);

				i++;
			};

			break;

		case 1:

			if (dsc.ready)
			{
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

				i++;
			};

			break;

		case 2:

			if (dsc.ready)
			{
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

				i++;
			};

			break;

		case 3:

			if (dsc.ready)
			{
				wbuf[0] = 5;	
				wbuf[1] = 0;	
				wbuf[2] = 0;	

				dsc.adr = 0x48;
				dsc.wdata = wbuf;
				dsc.wlen = 3;
				dsc.rdata = 0;
				dsc.rlen = 0;
				dsc.wdata2 = 0;
				dsc.wlen2 = 0;

				I2C_AddRequest(&dsc);

				i++;
			};

			break;

		case 4:

			if (dsc.ready)
			{
				curFireVoltage = GetCurFireVoltage();

				u16 t = mv.fireVoltage;

				if (t > curFireVoltage)
				{
					if (correction < 0x3FF)
					{
						correction += 1;
					};
				}
				else if (t < curFireVoltage)
				{
					if (correction > 0)
					{
						correction -= 1;
					};
				};

				dstFV += (i16)mv.fireVoltage - (dstFV+4)/8;

				t = (dstFV+4)/8;

				u32 k = (0x1E00 + correction) >> 3;

				t = (k*t+128) >> 10;

				if (t > 344) t = 344;

				t = ~(((u32)t * (65535*16384/344)) / 16384); 

				wbuf[0] = 8;	
				wbuf[1] = t>>8;
				wbuf[2] = t;

				dsc.adr = 0x48;
				dsc.wdata = wbuf;
				dsc.wlen = 3;
				dsc.rdata = rbuf;
				dsc.rlen = 0;
				dsc.wdata2 = 0;
				dsc.wlen2 = 0;

				I2C_AddRequest(&dsc);

				i++;
			};

			break;

		case 5:

			if (dsc.ready)
			{
				i = 0;
			};

			break;


	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static void UpdateI2C()
//{
//	if (!comdsp.Update())
//	{
//		if (I2C_Update())
//		{
//			comdsp.InitHW();
//
//			i2cResetCount++;
//		};
//	};
//}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitMainVars()
{
	mv.numDevice		= 0;
	mv.numMemDevice		= 0;
	mv.freq				= 3000; 
	mv.fireAmp			= 0;
	mv.fireVoltage		= 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void LoadVars()
{
	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_CYAN "Load Vars ... ");

	static DSCI2C dsc;
	static u16 romAdr = 0;
	
	byte buf[sizeof(mv)*2+4];

	bool c2 = false;

	bool loadVarsOk = false;

	romAdr = ReverseWord(FRAM_I2C_MAINVARS_ADR);

	dsc.wdata = &romAdr;
	dsc.wlen = sizeof(romAdr);
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;
	dsc.rdata = buf;
	dsc.rlen = sizeof(buf);
	dsc.adr = 0x50;

	if (I2C_AddRequest(&dsc))
	{
		while (!dsc.ready) { I2C_Update(); };
	};

	PointerCRC p(buf);

	for (byte i = 0; i < 2; i++)
	{
		p.CRC.w = 0xFFFF;
		p.ReadArrayB(&mv, sizeof(mv));
		p.ReadW();

		if (p.CRC.w == 0) { c2 = true; break; };
	};

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_WHITE "FRAM I2C - "); SEGGER_RTT_WriteString(0, (c2) ? (RTT_CTRL_TEXT_BRIGHT_GREEN "OK\n") : (RTT_CTRL_TEXT_BRIGHT_RED "ERROR\n"));

	loadVarsOk = c2;

	if (!loadVarsOk)
	{
		InitMainVars();

		svCount = 2;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void SaveVars()
{
	static DSCI2C dsc;
	static u16 romAdr = 0;
	static byte buf[sizeof(mv) * 2 + 8];

	static byte i = 0;
	static TM32 tm;

	PointerCRC p(buf);

	switch (i)
	{
		case 0:

			if (svCount > 0)
			{
				svCount--;
				i++;
			};

			break;

		case 1:

			mv.timeStamp = GetMilliseconds();

			for (byte j = 0; j < 2; j++)
			{
				p.CRC.w = 0xFFFF;
				p.WriteArrayB(&mv, sizeof(mv));
				p.WriteW(p.CRC.w);
			};

			romAdr = ReverseWord(FRAM_I2C_MAINVARS_ADR);

			dsc.wdata = &romAdr;
			dsc.wlen = sizeof(romAdr);
			dsc.wdata2 = buf;
			dsc.wlen2 = p.b-buf;
			dsc.rdata = 0;
			dsc.rlen = 0;
			dsc.adr = 0x50;

			tm.Reset();

			I2C_AddRequest(&dsc);

			i++;

			break;

		case 2:

			if (dsc.ready || tm.Check(100))
			{
				i = 0;
			};

			break;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateParams()
{
	static byte i = 0;

	#define CALL(p) case (__LINE__-S): p; break;

	enum C { S = (__LINE__+3) };
	switch(i++)
	{
		CALL( MainMode()				);
		CALL( UpdateTemp()				);
		CALL( UpdateCom(); 				);
		CALL( SaveVars();				);
		CALL( UpdateHV();				);
	};

	i = (i > (__LINE__-S-3)) ? 0 : i;

	#undef CALL
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateMisc()
{
	static byte i = 0;

	#define CALL(p) case (__LINE__-S): p; break;

	enum C { S = (__LINE__+3) };
	switch(i++)
	{
		CALL( UpdateHardware();	);
		CALL( UpdateParams();	);
	};

	i = (i > (__LINE__-S-3)) ? 0 : i;

	#undef CALL
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static void Update()
//{
//	UpdateHardware();
//	UpdateParams();
//}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_SAME53

	#define FPS_PIN_SET()	HW::PIOA->BSET(25)
	#define FPS_PIN_CLR()	HW::PIOA->BCLR(25)

#elif defined(CPU_XMC48)

	#define FPS_PIN_SET()	HW::P2->BSET(13)
	#define FPS_PIN_CLR()	HW::P2->BCLR(13)

#elif defined(WIN32)

	#define FPS_PIN_SET()	
	#define FPS_PIN_CLR()	

#endif

//static ComPort com1;

int main()
{
	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_WHITE "main() start ...\n");

	TM32 tm;

	//__breakpoint(0);

	InitHardware();

	LoadVars();

#ifndef WIN32

	comdsp.Connect(ComPort::ASYNC, 1250000, 2, 2);

#endif

	u32 fc = 0;

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_WHITE "Main Loop start ...\n");

	while (1)
	{
		Pin_MainLoop_Set();

		UpdateMisc();

		Pin_MainLoop_Clr();

		PIO_PWML->BTGL(PIN_PWMLA);

		fc++;

		if (tm.Check(1000))
		{
			fps = fc; fc = 0; 

			PrepareFire(mv.freq, mv.fireAmp, false);

#ifdef WIN32

			extern u32 txThreadCount;

			Printf(0, 0, 0xFC, "FPS=%9i", fps);
			Printf(0, 1, 0xF0, "%lu", testDspReqCount);
			Printf(0, 2, 0xF0, "%lu", txThreadCount);
#endif
		};

#ifdef WIN32

		UpdateDisplay();

		static TM32 tm2;

		byte key = 0;

		if (tm2.Check(50))
		{
			if (_kbhit())
			{
				key = _getch();

				if (key == 27) break;
			};

			if (key == 'w')
			{
				FLASH_WriteEnable();
			}
			else if (key == 'e')
			{
				FLASH_WriteDisable();
			}
			else if (key == 'p')
			{
				NAND_FullErase();
			};
		};

#endif

	}; // while (1)

#ifdef WIN32

	NAND_FlushBlockBuffers();

	I2C_Destroy();
	SPI_Destroy();

	//_fcloseall();

#endif

}
