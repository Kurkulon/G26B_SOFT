#include "ComPort.h"
#include "CRC16.h"

#include <windows.h>
#include <Share.h>
#include <conio.h>
#include <stdio.h>
#include <commctrl.h>

#include "types.h"

#define SGUID	0x3FE0C8D103E14728	
#define MGUID	0xA6FE828844CB84FF	
#define BOOT_MAN_REQ_WORD			0xA800
#define BOOT_MAN_REQ_MASK 			0xFF00

const unsigned __int64 masterGUID = MGUID;
const unsigned __int64 slaveGUID = SGUID;
//const u32	initCRC32 = 0x94979e45;
//const u32	xoutCRC32 = 0x33605738;

//#define BOOTSIZE 0x4000
//#define FLASH0 (0x00400000+BOOTSIZE)

static u16 PAGESIZE = 256;
#define PAGEDWORDS (256>>2)

static u32 _flashPages[] = {
#include "ak.ldr.h"
};

static byte bufFlashPages[sizeof(_flashPages) + 2];


HWND	hWnd;
HWND	btAccept;
HWND	cbCom;
HWND	prBar;

static u32 crcErr = 0;

static bool run = true;
static bool start = false;


const word winWidth = 768;
const word winHeight = 400;

const COLORREF bgColor = RGB(212, 208, 200);
const COLORREF txtColor = RGB(0, 0, 0);

HFONT font1;
HFONT font2;


int x,y,Depth;

HDC memdc;
HBITMAP membm;

static char pressedKey = 0;

const char lpAPPNAME[] = "G26B_1_AK90 Flash Loader";

int screenWidth = 0, screenHeight = 0;

LRESULT CALLBACK WindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static byte pallete[256*3] = {0};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
struct ReqHS { unsigned __int64 guid; u16 crc; };
struct RspHS { unsigned __int64 guid; u16 crc; };

struct Request
{
	union
	{
		struct  { u16 rw; u16 crc; } F5;											// запрос контрольной суммы и длины программы во флэш-памяти
		struct  { u16 rw; u16 stAdr; u16 len; byte data[256]; u16 crc; } F6;		// запись страницы во флэш
		struct  { u16 rw; word crc; } F7;											// перезагрузить блэкфин
	};
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct Response
{
	union
	{
		struct  { u16 rw; u16 flashLen; u32 startAdr; u16 flashCRC; u16 crc; } F5;	// запрос контрольной суммы и длины программы во флэш-памяти
		struct  { u16 rw; u16 res; u16 crc; } F6;									// запись страницы во флэш
	};
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static ComPort com1;
static ComPort::ReadBuffer rb = {};
static ComPort::WriteBuffer wb;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static ReqHS reqHS;
static RspHS rspHS;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static u16 pageAdr = 0;
//static u32 secLen = 0;
static u16 secCRC = 0;
static bool secProgOK = false;
static u32 row = 0;
static u32 count = ArraySize(_flashPages);
static u32 packetnum = 0;
static u32 oknum = 0;
static u32 errornum = 0;
static u32 lenErrors = 0;
static u32 crcErrors = 0;
	static u32 pageErrors = 0;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void Printf(int x, int y, const char *format, ... )
{
	char buf[512];

	va_list arglist;

    va_start(arglist, format);
    int l = vsnprintf_s(buf, sizeof(buf), sizeof(buf)-1, format, arglist);
    va_end(arglist);

	if (l > 0)
	{
		SetBkColor(memdc, bgColor);
		SetTextColor(memdc, txtColor);
		TextOut(memdc, x, y, buf, l);
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool CreateComboCom()
{
	cbCom =	CreateWindowEx(0, "COMBOBOX", "1", CBS_DROPDOWNLIST|WS_CHILDWINDOW|WS_VSCROLL, 10, 10, 200, 100, hWnd, 0, 0, 0); 
//	SendMessage(cbCom, CB_INITSTORAGE, 0, 0);

	com1.BuildPortTable();

	u32 size = com1.GetPortTableSize();
	const u32* p = com1.GetPortTable();

	if (size == 0)
	{
		return false;
	};

    char buf[256];

	for (u32 i = 0; i < size; i++)
	{
        wsprintf(buf, "\\\\.\\COM%lu", p[i]);

		if (SendMessage(cbCom, CB_ADDSTRING, 0, (LPARAM)buf) == CB_ERR)
		{
			return false;
		};
	};

	SendMessage(cbCom, CB_SETCURSEL, 0, 0);
	ShowWindow (cbCom, SW_SHOWNORMAL);


	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitDisplay()
{
	WNDCLASS		    wcl;

	wcl.hInstance		= NULL;
	wcl.lpszClassName	= lpAPPNAME;
	wcl.lpfnWndProc		= WindowProc;
	wcl.style	    	= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

	wcl.hIcon	    	= NULL;
	wcl.hCursor	    	= LoadCursor(0, IDC_ARROW);
	wcl.lpszMenuName	= NULL;

	wcl.cbClsExtra		= 0;
	wcl.cbWndExtra		= 0;
	wcl.hbrBackground	= NULL;

	RegisterClass (&wcl);

	int sx = screenWidth = GetSystemMetrics (SM_CXSCREEN);
	int sy = screenHeight = GetSystemMetrics (SM_CYSCREEN);

	hWnd = CreateWindowEx (0, lpAPPNAME, lpAPPNAME, WS_CAPTION|WS_POPUP|WS_SYSMENU, 0, 0, winWidth, winHeight, 0, 0, 0, 0);

	if(!hWnd) 
	{
		exit(0);
	};

	font1 = CreateFont(14, 8, 0, 0, FW_NORMAL, false, false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "Lucida Console");
	font2 = CreateFont(16, 8, 0, 0, FW_DONTCARE, false, false, false, RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "System");

	RECT rect;

	if (GetClientRect(hWnd, &rect))
	{
		MoveWindow(hWnd, 0, 0, winWidth + winWidth - (rect.right - rect.left), winHeight + winHeight - (rect.bottom - rect.top), true);
	};

	ShowWindow (hWnd, SW_SHOWNORMAL);

	btAccept = CreateWindowEx(0, "BUTTON", "Старт", BS_PUSHBUTTON|WS_CHILDWINDOW, 662, 365, 96, 23, hWnd, 0, 0, 0);

	CreateComboCom();

	ShowWindow (btAccept, SW_SHOWNORMAL);

	InitCommonControls();

	prBar = CreateWindowEx(0, PROGRESS_CLASS, "pb", PBS_SMOOTH|WS_CHILDWINDOW|WS_VISIBLE, 10, 50, 300, 16, hWnd, 0, 0, 0); 

	SendMessage(prBar, PBM_SETRANGE, 0, MAKELPARAM (0, 2048));


	GetClientRect(hWnd, &rect);
	HDC hdc = GetDC(hWnd);
	SelectObject(hdc, CreateSolidBrush(RGB(0,0,0)));
    memdc = CreateCompatibleDC(hdc);
	membm = CreateCompatibleBitmap(hdc, winWidth, winHeight);
    SelectObject(memdc, membm);
	ReleaseDC(hWnd, hdc);

	SelectObject(memdc, font1);

	rect.left = 0; rect.top = 0; rect.right = winWidth; rect.bottom = winHeight;

	FillRect(memdc, &rect, CreateSolidBrush(bgColor));


	//for (dword i = 0; i < 1900; i++) { screenBuffer[i] = 0xFF; };

	//screenBuffer[0] = 0;
	//screenBuffer[32] = 0;


	//SetDIBitsToDevice(memdc, 0, 0, width, height, 0, 0, 0, height, screenBuffer, &bminfo.bmi, DIB_RGB_COLORS);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

LRESULT CALLBACK WindowProc(HWND h, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;

    switch (message)
	{
        case WM_PAINT:

			if (h == hWnd && GetUpdateRect(h, &rect, false))
			{
				hdc = BeginPaint(h, &ps);

				BitBlt(hdc, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, memdc, rect.left, rect.top, SRCCOPY);

				EndPaint(h, &ps);

				return 0;
			};

            break;

		case WM_CLOSE:

			run = false;

            break;

		case WM_COMMAND:

			if (HIWORD(wParam) == BN_CLICKED)
			{
				if ((HWND)lParam == btAccept)
				{
					start = true;
					EnableWindow(btAccept, FALSE);
					EnableWindow(cbCom, FALSE);
				}
				else
				{
//					Get_Cond();
				};

				return 0;
			};

			break;
	};
    
	return DefWindowProc(h, message, wParam, lParam);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateDisplay()
{
	MSG msg;

	if(PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		GetMessage (&msg, NULL, 0, 0);

		TranslateMessage (&msg);

		DispatchMessage (&msg);
	}
	else
	{
		Sleep(1);
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/*
static bool UpdateCheckEraseSector(bool start)
{
	static byte i = 0;

	static Request req;
	static Response rsp;

//	char buf[1024];

	if (start)
	{
		i = 0;
	};

	switch(i)
	{
		case 0:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			
			req.F1.func = 1;
			req.F1.sadr = pageAdr;
			req.F1.len = sizeof(flashPages) - pageAdr;
			req.F1.align += 1;

			req.F1.crc = GetCRC16(&req, sizeof(req.F1) - sizeof(req.F1.crc));

			wb.data = &req;
			wb.len = sizeof(req.F1);
	//		Sleep(5);
			com1.Write(&wb);
			i++;

			break;

		case 1:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (!com1.Update())
			{
				packetnum++;
				rb.data = &rsp;
				rb.maxLen = sizeof(rsp.F1);
				com1.Read(&rb, 5000, 2);
				i++;
			};

			break;

		case 2:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (!com1.Update())
			{
				if (rb.recieved && rb.len == sizeof(rsp.F1) && rsp.F1.sadr == req.F1.sadr && GetCRC16(&rsp, sizeof(rsp.F1)) == 0)
				{
					oknum++;

					secLen = rsp.F1.len;
					secCRC = rsp.F1.sCRC;

					if (sizeof(flashPages) <= pageAdr)
					{
						secLen = 0;
					};

					if (secLen > 0)
					{
						u32 l = sizeof(flashPages) - pageAdr;

						if (secLen > l)
						{
							secLen = l;
						};

						u16 crc = GetCRC16(&flashPages[pageAdr/4], secLen);

						Printf(10, 128+row*16, "Сектор: 0x%08lX, len:0x%08lX, CRC: 0x%04X  0x%04X", pageAdr, secLen, rsp.F1.sCRC, crc);

						row += 1;

						if (crc != rsp.F1.sCRC)
						{
							i++;
						}
						else
						{
							pageAdr += secLen;

							if (sizeof(flashPages) <= pageAdr)
							{
								secLen = 0;

								i = 255;
							}
							else
							{
								i = 0;
							};
						};
					}
					else
					{
						i = 255;
					};
				}
				else
				{
					if (!rb.recieved)
					{ 
						errornum++; 
					}
					else if (rb.len != sizeof(rsp.F1))
					{
						lenErrors++;
					}
					else if (GetCRC16(&rsp, sizeof(rsp.F1)) != 0)
					{
						crcErrors++;
					}
					else if (rsp.F1.sadr != req.F1.sadr)
					{
						pageErrors++;
					};

					i = 0;
				};
			};

			break;

		case 3:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			
			req.F2.func = 2;
			req.F2.sadr = pageAdr;
			req.F2.align += 1;

			req.F2.crc = GetCRC16(&req, sizeof(req.F2) - sizeof(req.F2.crc));

			wb.data = &req;
			wb.len = sizeof(req.F2);
	//		Sleep(5);
			com1.Write(&wb);
			i++;

			break;

		case 4:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (!com1.Update())
			{
				packetnum++;
				rb.data = &rsp;
				rb.maxLen = sizeof(rsp.F2);
				com1.Read(&rb, 3000, 2);
				i++;
			};

			break;

		case 5:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (!com1.Update())
			{
				if (rb.recieved && rb.len == sizeof(rsp.F2) && rsp.F2.sadr == req.F2.sadr && GetCRC16(&rsp, sizeof(rsp.F2)) == 0)
				{
					oknum++;

					if (rsp.F2.status == 0)
					{
						secLen = 0;
					};

					i = 255;
				}
				else
				{
					if (!rb.recieved)
					{ 
						errornum++; 
					}
					else if (rb.len != sizeof(rsp.F2))
					{
						lenErrors++;
					}
					else if (GetCRC16(&rsp, sizeof(rsp.F2)) != 0)
					{
						crcErrors++;
					}
					else if (rsp.F2.sadr != req.F2.sadr)
					{
						pageErrors++;
					};

					i = 3;
				};
			};

			break;

	};

	return (i < 10);
}
*/

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool UpdateProgramSector(bool start)
{
	static byte i = 0;

	static Request req;
	static Response rsp;

	byte *p;

//	char buf[1024];

	if (start)
	{
		i = 0;
	};

	switch(i)
	{
		case 0:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		{	
			req.F6.rw = BOOT_MAN_REQ_WORD|6;
			req.F6.stAdr = pageAdr;

			p = bufFlashPages + pageAdr;

			for (u16 j = 0, t = pageAdr; j < PAGESIZE; j++, t++)
			{
				req.F6.data[j] = (t < sizeof(bufFlashPages)) ? p[j] : ~0;
			};

			req.F6.len = PAGESIZE;

			req.F6.crc = GetCRC16(&req, sizeof(req.F6) - sizeof(req.F6.crc));

			wb.data = &req;
			wb.len = sizeof(req.F6);
			Sleep(3);
			com1.Write(&wb);
			i++;

			break;
		};

		case 1:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (!com1.Update())
			{
				rb.data = &rsp;
				rb.maxLen = sizeof(rsp.F6);
				com1.Read(&rb, 100, 1);
				i++;
			};

			break;

		case 2:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (!com1.Update())
			{
				if (rb.recieved && rb.len == sizeof(rsp.F6) && rsp.F6.rw == req.F6.rw && GetCRC16(&rsp, sizeof(rsp.F6)) == 0)
				{
					oknum++;

					if (rsp.F6.res == 0)
					{
						pageAdr += PAGESIZE;

						secProgOK = true;

						i = (pageAdr >= sizeof(bufFlashPages)) ? 255 : 0; 
					}
					else
					{
						secProgOK = false;

						i = 255;
					};
				}
				else
				{
					if (!rb.recieved)
					{ 
						errornum++; 
					}
					else if (rb.len != sizeof(rsp.F6))
					{
						lenErrors++;
					}
					else if (GetCRC16(&rsp, sizeof(rsp.F6)) != 0)
					{
						crcErrors++;
					};

					i = 0;
				};
			};

			break;
	};

	return (i < 10);
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void UpdateRequest()
{
	static byte i = 0;

	static Request req;
	static Response rsp;
	//static u32 *p = flashPages;


	char buf[1024];

	switch(i)
	{
		case 0:

			if (UpdateProgramSector(true))
			{
				i++;
			};

		case 1:

			if (!UpdateProgramSector(false))
			{
				if (!secProgOK)
				{
					i = 3;
				}
				else
				{
					i++;
				};
			};

			break;

		case 2:

			MessageBox(hWnd, "Программирование завершено", "Ахтунг", MB_OK); 
			run = false;
			start = false;
			i = 6;

			break;

		case 3:

			start = false;
			i++;
			sprintf_s(buf, ArraySize(buf), "Ошибка программирования по адресу 0x%08lX", pageAdr);
			MessageBox(hWnd, buf, "Ахтунг", MB_OK); 

			break;

		case 4:

			break;
	};

	SendMessage(prBar, PBM_SETPOS,  512 * pageAdr / ArraySize(_flashPages), 0);

	RECT rect = { 10, 80, 480, 400 };

	Printf(10, 80, "Запросов: %lu Таймаут: %lu Длина: %lu CRC: %lu PAGE: %lu", packetnum, errornum, lenErrors, crcErrors, pageErrors);
	Printf(10, 96, "OK: %lu      ", oknum);
	Printf(10, 112, "Адрес: 0x%08lX    ", pageAdr);
	RedrawWindow(hWnd, &rect, 0, RDW_INVALIDATE);

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateCom()
{
	static byte i = 0;
	static u32 pt = 0;
	static Request req;
	static Response rsp;

	u32 n;

	if (start) switch(i)
	{
		case 0:

			n = (u32)SendMessage(cbCom, CB_GETCURSEL, 0, 0);

			char buf[1024];

			SendMessage(cbCom, CB_GETLBTEXT, n, (LPARAM)buf);

			if (!com1.Connect(buf, 115200, 2))
			{
				MessageBox(hWnd, "Не удалось открыть COM-порт", "", MB_OK); 
				start = false;
			}
			else
			{
				i++;
			};

			break;

		case 1:

			reqHS.guid = masterGUID;
			reqHS.crc = GetCRC16(&reqHS, sizeof(reqHS) - sizeof(reqHS.crc));
			wb.data = &reqHS;
			wb.len = sizeof(reqHS);
			com1.Write(&wb);
			i++;

		case 2:

			if (!com1.Update())
			{
				rb.data = &rspHS;
				rb.maxLen = sizeof(rspHS);
				com1.Read(&rb, 50, 2);
				i++;
			};

			break;

		case 3:

			if (!com1.Update())
			{
				if (!rb.recieved)
				{
					i = 1;
				}
				else
				{
					if (rb.len == sizeof(rspHS) && GetCRC16(&rspHS, sizeof(rspHS)) == 0 && rspHS.guid == slaveGUID)
					{
						pt = GetTickCount();
						i++;
					}
					else
					{
						i = 1;
					};
				};
			};

			break;

		case 4:

			if ((GetTickCount() - pt) > 100)
			{
				i++;
			};

			break;

		case 5:

			req.F5.rw = BOOT_MAN_REQ_WORD|5;
			req.F5.crc = GetCRC16(&req, sizeof(req.F5) - sizeof(req.F5.crc));
			wb.data = &req;
			wb.len = sizeof(req.F5);
			com1.Write(&wb);
			i++;

		case 6:

			if (!com1.Update())
			{
				rb.data = &rsp;
				rb.maxLen = sizeof(rsp.F5);
				com1.Read(&rb, 100, 2);
				i++;
			};

			break;

		case 7:

			if (!com1.Update())
			{
				if (rb.recieved && rb.len == sizeof(rsp.F5) && rsp.F5.rw == req.F5.rw && GetCRC16(&rsp, sizeof(rsp.F5)) == 0)
				{
					u16 crc = GetCRC16(_flashPages, sizeof(_flashPages));

					if (rsp.F5.flashLen == sizeof(_flashPages) && rsp.F5.flashCRC == crc)
					{
						MessageBox(hWnd, "Прошивки совпадают, программирование не требуется", "Ништяк", MB_OK); 
						run = false;
						i = ~0;
					}
					else
					{
						pt = GetTickCount();

						DataPointer d(bufFlashPages);
						u32 *s = _flashPages;

						for (u32 ii = sizeof(_flashPages)/4; ii != 0; ii--) *(d.d++) = *(s++);
						*d.w = crc;

						i++;
					};
				}
				else
				{
					i = 5;
				};
			};

			break;

		case 8:

			if ((GetTickCount() - pt) > 100)
			{
				i++;
			};

			break;

		case 9:

			UpdateRequest();

			break;

	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//int main()
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	static bool pb = false;
	static dword pt = 0;

	InitDisplay();

	while (run)
	{
		UpdateCom();

		UpdateDisplay();

	};

	return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

