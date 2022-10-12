#ifndef MAIN_H__27_03_2020__09_53
#define MAIN_H__27_03_2020__09_53

#include "types.h"
#include "list.h"

#define VERSION			0x0101

extern u32 FLASH_Read(u32 addr, byte *data, u32 size);
//extern bool IAP_EraseSector(u32 sa);


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//struct MB : public PtrObj<MB>
//{
//	u32			len;
//	u32			data[64];
//
//	virtual	u32		MaxLen() { return sizeof(data); }
//
//	MB() : len(0) {}
//};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct FLWB
{
	u32		adr;
	u32 	dataLen;
	u32 	dataOffset;
	byte	data[0];
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//template<int LEN> struct MEMB : public MB
//{
//	u32 exData[(LEN-sizeof(data)+3)>>2];
//
//protected:
//
//	PTR_LIST_FRIENDS(MEMB);
//
//	static List<MEMB> freeList;
//	static	MEMB*	Alloc()	{ return freeList.Get(); }
//	virtual void	Free()	{ freeList.Add(this); }
//
//public:
//
//	virtual	u32		MaxLen() { return sizeof(data)+sizeof(exData); }
//
//					MEMB() { freeList.Init(); freeList.Add(this); }
//};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//extern MEMB* AllocMemBuffer();
//extern void FreeMemBuffer(MEMB* wb);
extern bool RequestFlashWrite(MB* b);
//extern void InitFlashWrite();

inline u32 GetFlashWriteError() { extern u32 flash_write_error; return flash_write_error; }
inline u32 GetFlashWriteOK() { extern u32 flash_write_ok; return flash_write_ok; }

#endif // MAIN_H__27_03_2020__09_53
