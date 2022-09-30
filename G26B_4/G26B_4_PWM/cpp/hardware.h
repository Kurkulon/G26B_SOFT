#ifndef HARDWARE_H__23_12_2013__11_37
#define HARDWARE_H__23_12_2013__11_37

#include "types.h"
//#include "core.h"
#include "time.h"
#include <i2c.h>
#include "hw_nand.h"
#include "hw_rtm.h"
#include "manch.h"
#include "hw_conf.h"
//#include "flash.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define UNIBUF_LEN (3072)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//struct UNIBUF : public PtrItem<UNIBUF>
//{
//	UNIBUF() : dataOffset(sizeof(VecData::Hdr)), dataLen(0)  { /*freeBufList.Add(this);*/ }
//
//	u16		dataOffset;
//	u16 	dataLen;
//
//	byte	data[UNIBUF_LEN]; // Последние 2 байта CRC16
//
//	void*	GetDataPtr() { return data+dataOffset; }
//};
//
////++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


extern void UpdateHardware();
extern void PrepareFire(u16 waveAmp);

extern u16 CRC_CCITT_PIO(const void *data, u32 len, u16 init = ~0);
extern u16 CRC_CCITT_DMA(const void *data, u32 len, u16 init = ~0);
extern bool CRC_CCITT_DMA_Async(const void* data, u32 len, u16 init = ~0);
extern bool CRC_CCITT_DMA_CheckComplete(u16* crc);

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern bool IsFireOK();
extern void DisableFire();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern void InitHardware();
extern void UpdateHardware();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__forceinline u32 Push_IRQ()
{
	register u32 t;

#ifndef WIN32

	register u32 primask __asm("primask");

	t = primask;

	__disable_irq();

#endif

	return t;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__forceinline void Pop_IRQ(u32 t)
{
#ifndef WIN32

	register u32 primask __asm("primask");

	primask = t;

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef WIN32

extern void I2C_Destroy();
extern void SPI_Destroy();
extern void UpdateDisplay();
extern int PutString(u32 x, u32 y, byte c, const char *str);
extern int Printf(u32 x, u32 y, byte c, const char *format, ... );

extern void NAND_FlushBlockBuffers();
extern void NAND_ReqFlushBlockBuffers();

#endif

#endif // HARDWARE_H__23_12_2013__11_37
