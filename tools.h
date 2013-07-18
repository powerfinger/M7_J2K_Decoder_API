#ifndef _TOOLS_H_
#define _TOOLS_H_
#include "stdafx.h"
// Define control codes for Merlin driver
//

#define DRVREVMIN 0x32
#define DRVREVMAX 0x35
#define F1REV 0x91
#define F2REV 0x91
#define F1REVHD1 0x0803
#define F1REVHD2 0x0104
//#define F1REVHD2 0x0303

typedef enum {
	STATUS_PASS,
    STATUS_FAIL,
	STATUS_NOMEMORY
} M2DDSTATUS;

M2DDSTATUS m2ddReadInt32(int boardID,DWORD address,DWORD& data);
M2DDSTATUS m2ddWriteInt32(int boardID,DWORD address,DWORD data);
M2DDSTATUS m2ddMapMemory(int boardID, DWORD *mapping);
//M2DDSTATUS m2ddMapMemory(int boardID, DWORD *physicalAddress,DWORD *virtualAddress, DWORD *physicalAddressUpper,DWORD *virtualAddressUpper);
//M2DDSTATUS m2ddMapMemory2(int boardID, DWORD *physicalAddress,DWORD *virtualAddress, DWORD *physicalAddressUpper,DWORD *virtualAddressUpper);
M2DDSTATUS m2ddSendEventHandle(int boardID,HANDLE hEvent);

#ifndef __merlinioctl__h_
#define __merlinioctl__h_

#define MERLIN_IOCTL_READ CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_DATA)
#define MERLIN_IOCTL_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define MERLIN_IOCTL_MAP_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA)
#define MERLIN_IOCTL_SEND_EVENT_HANDLE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_WRITE_DATA)
#define MERLIN_IOCTL_MAP_MEMORY2 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_READ_DATA)
#endif

#define SHMEMSIZE 0x8000
#define MAXCBSIZE 0x1000
#define MAXMBSIZE 0x400

typedef struct _M2CONTEXT {
	ULONG CommandQueue1[0x400]; // Storage for an outgoing commands
	volatile ULONG CommandReady1;        // Software/Hardware ownership flags
	volatile ULONG InterruptNumber;      // Running count of Interrupts
	volatile ULONG DBufOffset;           // Pointer to the start of first DMA address
	volatile ULONG i2cCommandCnt;        // Running count of i2c commands sent by software (updated by application)  
	volatile ULONG i2cMessageCnt;        // Running count of i2c messages received (updated by ISR)  
    volatile ULONG i2cReadBuffer[0x40];  // buffer used to receive data for i2c read command from C-Cube 
    volatile ULONG CBSize[2];              // size of the Command buffer
    volatile ULONG MBSize[2];              // size of the Message buffer
    volatile ULONG BufferNAddress[1021]; // misc. share memory buffers (See BufferNAddress Offsets below) 
	volatile ULONG MsgErrCnt;  
	volatile ULONG Buffer_Megs2;  
	volatile ULONG FieldCntChk;  
	volatile ULONG SumError;             // Summer Error Flag (updated by ISR)  
	volatile ULONG UnderRunError;        // Buffer UnderRun / OverRun error flag (updated by ISR)  
	volatile ULONG board;                // Board number storage (updated by application)  
	volatile ULONG Revision;             // Driver revision number (set by Driver)  
	volatile ULONG FieldCnt;             // Field count at last interrupt (updated by ISR)
	volatile ULONG LineCnt;              // Field count at last interrupt (updated by ISR)  
	volatile ULONG LSIAudCnt;            // LSI audio sample count at last interrupt (updated by ISR)  
	volatile ULONG D1AudCnt;             // DMA1 audio sample count at last interrupt (updated by ISR)
	volatile ULONG D2AudCnt;             // DMA2 audio sample count at last interrupt (updated by ISR)
	volatile ULONG VITCREG1;             // VITC Register 1 at last interrupt (updated by ISR)
	volatile ULONG VITCREG2;             // VITC Register 2 at last interrupt (updated by ISR)
	volatile ULONG LTCREG1;              // LTC Register 1 at last interrupt (updated by ISR)  
	volatile ULONG LTCREG2;              // LTC Register 2 at last interrupt (updated by ISR)  
	volatile ULONG Scratch[0x80];        // Scratch pad memory (set and read by applications)
	ULONG CommandQueue2[0x400]; // 
	volatile ULONG CommandReady2;
	ULONG CommandQueue3[0x400];
	volatile ULONG CommandReady3;
	ULONG CommandQueue4[0x400];
	volatile ULONG CommandReady4;
	unsigned char DecoderAQ[0x1000];
	short DecoderAPH;
	short DecoderAPT;
	unsigned char DecoderBQ[0x1000];
	short DecoderBPH;
	short DecoderBPT;
	volatile ULONG Buffer_Megs;  
	volatile ULONG Scratch2[0x80];
    volatile ULONG TCInHead;
    volatile ULONG TCInTail;
    volatile ULONG FCntInData[0x40];
    volatile ULONG VITCInData[0x40];
    volatile ULONG LTCInData[0x40];
	volatile ULONG TCOutCtrl;
    volatile ULONG TCOutHead;
    volatile ULONG TCOutTail;
    volatile ULONG FCntOutData[0x40];
    volatile ULONG VITCOutData[0x40];
    volatile ULONG LTCOutData[0x40];
} M2CONTEXT, *PM2CONTEXT;

/*

BufferNAddress Offsets
======================
0 - 31     = Message Type 5 data (Load Bitstream) (used to keep track of DMA buffers)
32 - 63    = Message Type 6 data (store Bitstream) (used to keep track of DMA buffers)
164        = Decoder A state tracker (used for playlist mode. Application sets to 0 and then track when a clip is played)
165        = Decoder A timecode change flag
166        = Decoder A last timecode
167        = Decoder A play frame count
169 - 180  = Decoder A status messages
264        = Decoder B state tracker (used for playlist mode. Application sets to 0 and then track when a clip is played)
265        = Decoder B timecode change flag
266        = Decoder B last timecode
267        = Decoder B play frame count
269 - 280  = Decoder B status messages
364 - 370  = DV Encoder Status message (decoder microcode)
364 - 463  = Encoder pre and post frame Status message (encoder microcode)
464 - 563  = Encoder Status message (encoder microcode)
564        = Encoder Error message count
565 - 663  = Encoder Error messages
664        = Stream 0 Decoder Error message count
665 - 667  = Last stream 0 Decoder Error message
668        = Stream 1 Decoder Error message count
669 - 671  = Last stream 1 Decoder Error message
672        = Stream 2 Decoder Error message count
673 - 675  = Last stream 2 Decoder Error message
676        = Stream f Decoder Error message count
677 - 679  = Last stream f Decoder Error message
680        = Other Decoder Error message count
681 - 683  = Other Last Decoder Error message
764        = buffer use counter (used to track buffers left until overrun)
800        = encoded bits per frame queue head pointer
801        = encoded bits per frame queue tail pointer
802 - 1000 = encoded bits per frame queue


*/

// other definitions
//

#define PRIORITY 0
#define SCHEDULED 1
#define PRIARRAY 2
#define SCHARRAY 3

#define NTSC 0
#define PAL 1
#define NTSCNDF 2

#define CVIDEO    0
#define SVIDEO    1
#define COMPONENT 2
#define SDI       3
#define FREERUN   4
#define CRGB      5
#define CVIDEO2   6
#define HEADER    7
#define HDSDI1    8
#define HDSDI2    9
#define HDSDI3    10

#define ANA1 0x04
#define AES1 0x05
#define EMB1 0x06
#define EMB2 0x07
#define EMB3 0x08
#define EMB4 0x09
#define MUTE 0x0c
#define DEC1 0x14
#define DEC2 0x15
#define DEC3 0x16
#define DEC4 0x17

#define READ      0x01
#define WRITE     0x00
#define AT17C002  0xae
#define PAGE_SIZE 256
#define MAX_PAGES 2048

#define CBUF_EVENONLY 2
#define CBUF_ODDONLY  1
   
int m2ddRdScratch(M2CONTEXT *m2c,DWORD address);
int m2ddRdInt32(M2CONTEXT *m2c,DWORD address);
M2DDSTATUS m2ddWrInt32(M2CONTEXT *m2c,DWORD address,DWORD data);

struct cmdq {
	int pq[4][0x400];
	int sq[4][0x400];
	int pcnt[4];
	int scnt[4];
};

extern "C"{
int parse_args1(char *initstr);
int parse_args2(char *initstr);
int parse_args3(char *initstr);
int parse_args4(char *initstr);
unsigned long mpegenc1(short buffer[2][1152],unsigned char encbuf[1024]);
unsigned long mpegenc2(short buffer[2][1152],unsigned char encbuf[1024]);
unsigned long mpegenc3(short buffer[2][1152],unsigned char encbuf[1024]);
unsigned long mpegenc4(short buffer[2][1152],unsigned char encbuf[1024]);
unsigned long mpegdec1(char encbuf[4096],int encbufsz,short intbuffer[1152*2],int mtd);
unsigned long mpegdec2(char encbuf[4096],int encbufsz,short intbuffer[1152*2],int mtd);
unsigned long mpegdec3(char encbuf[4096],int encbufsz,short intbuffer[1152*2],int mtd);
unsigned long mpegdec4(char encbuf[4096],int encbufsz,short intbuffer[1152*2],int mtd);
}
_int64 ReadUTS(int board);
void resetreftc(int board);
int loadf2(int board, int key, char *filename);
void SetAudioLevels(M2CONTEXT *m2c, int init, int gain, int vol);
void i2cinit(M2CONTEXT *m2c, int standard, int panelb=0);
void SetVideoInput(M2CONTEXT *m2c, int input);
int PIO(int board, unsigned char *bytes, int cnt);
int PIOCY(int board, unsigned char *bytes, int cnt);
int mld(int board, char *pUxFileName);
void mercmd(int cmd, ...);
void sendqinit(cmdq* cq);
void sendq(int queue,cmdq* cq,M2CONTEXT* m2c,int cnt,int bufmask=0);
void GetPwd(int board, int &pwd1, int &pwd2);
int WritePage(int board, unsigned char addr1,unsigned char addr2, unsigned char *bufptr);
int VerifyPage(int board, unsigned char addr1,unsigned char addr2, unsigned char *bufptr);
int ReadPage(int board, unsigned char addr1,unsigned char addr2, unsigned char *bufptr);
int EEPIO(int board, unsigned char *bytes, int cnt);
int EEPIO_RD1(int board, unsigned char *bytes);
int EEPIO_RD2(int board);
void EEPIO_RD3(int board);
int sc2fcdf (int sc, int palmode, int nondrop);
int fc2scdf (int fc, int palmode, int nondrop);
int scfixdf (int sc, int palmode, int nondrop);
int sc2fc (int sc, int palmode);
int fc2sc (int fc, int palmode);
int scfix (int sc, int palmode);
void WriteAudHdr(char *filename, int samplerate, int channels);
int Set_EPROM_Load_Address(int board, int Address);
int Get_EPROM_Load_Address(int board);
int Set_GTP_Loop_filter(int board, int loop_filter_en);
int ClockInitStatic(int board, int pll, int force, int freqsel);
int ClockInit(int board, int freqsel, int clkval);
int I2C_WR(int board, int device, unsigned char *bytes, int cnt, int mask = 0xffffffff); 
int I2C_RDP(int board, int device, unsigned char *bytes, int cnt, int start, int mask = 0xffffffff);
int DongleReadSerial(int board, unsigned char *bytes, int mask=0xffffffff);
int DongleReadPage(int board, unsigned char *bytes, int mask=0xffffffff);
int DongleWritePage(int board, unsigned char *bytes, int mask=0xffffffff);

struct sharedmem {
	//decoder
	int *venderid;
	int *messbuf;
	int *i2cMcnt;
	int *i2cbufp;
	int *i2cbufv;
	int *ucres;
	int *audspeed;
	int *audplchk;
	int *vaplchk;
	int *vbplchk;
	int *alpplchk;
	int *ipia;
	int *statalpa;
	int *statva;
	int *ipib;
	int *statalpb;
	int *statvb;
	int *statrvp;
	int *statrv;
	int *statra;
	int *statenc;
	int *stataud;
	int *physadd;
	int *bufva;
	int *bufvb;
	int *bufdio;
	int *bufdvr;
	int *bufdvar;
	int *errorrv;
	int *errorpv;
	int *overrun;
	//encoder
	int *encbits;
	int *bufv;
};

typedef struct _BRDCTX {
	M2CONTEXT* m2c;
	DWORD physicalAddressbase,virtualAddressbase;
	DWORD physicalAddress[4],virtualAddress[4];
	DWORD physicalAddressUpper[4],virtualAddressUpper[4];
	DWORD Buffer_Megs[4];
	int *videoaVirtual,videoaPhysical;
	sharedmem shmem;
	HANDLE hEvent;
	cmdq cq;
} BRDCTX, *PBRDCTX;

struct descripters {
	int NextDscrptr;
	int BufPtr;
	int DMACnt;
	int BufPtrUpper;
};

typedef union mulong64 {
	struct {
		unsigned long low;
		unsigned long high;
	} l32;
	_int64 l64;
} MULONG64;

// loader definitions
//

#define _UXLOADER_HEDAER

enum 
{
	SDRAM = 0,
	CBUS = 1
};

typedef struct tagUXFileHeader
{
	unsigned long	MagicNumber;
	unsigned long	NumberOfSections;
	unsigned long	LengthOfInitialPortion;
} UXFILEHEDER, * PUXFILEHEADER;

typedef struct tagUXSectionHeader
{
	unsigned char	DataType;
	unsigned char	Flags;
	unsigned short	UnusedFlags;
	unsigned long	SectionLength;
	unsigned long	SectionStart;
	unsigned long	SectionChksum;
	// Data of variable length follows this section header.
} UXSECTIONHDR, * PUXSECTIONHDR;

#define SDRAM_CLK_CONTROL_INIT 0x0
#define SDRAM_CONTROL_INIT 0x15
#define kReset 1
#define kResetTimeLimit 1000
#define SDRAM_ClkControl 2
#define SDRAM_Control 3
#define SERIAL_ROM_PRESENT 0

// Bits 1 and 2 of flag
#define SECTION_FLAG_DATA_BLOCK			0x00
#define	SECTION_FLAG_ADDR_DATA_PAIR		0x02
#define SECTION_FLAG_WAIT_FOR_DATA		0x04
#define SECTION_FLAG_LOOP_DELAY			0x06


// Register definitions
#define SDAM_START		0x00
#define SDRAM_END		0x007FFFFF
#define CBUS_START		0x00FC0000
#define RMEM_START		0x00FE0000


BOOL									// Return TRUE on success, else FALSE.
M2ReadDataBlock(						// Read block of data. Byte, WORD or DWORD.
    int     boardID,
	DWORD	address,					// Address where data is to be written.
	BYTE *	data,						// Pointer to data.
	DWORD	length);						// Number of bytes to be read.
BOOL									// Return TRUE on success, else FALSE.
M2WriteDataBlock(						// Write block of data. Byte, WORD or DWORD.
    int     boardID,
	DWORD	address,					// Address where data is to be written.
	BYTE *	data,						// Pointer to data.
	DWORD	length);						// Number of bytes to be written.
BOOL					// Return TRUE on success and FALSE for failure
LoadSectionData(				// Load the section data.
	int             boardID,
	PUXSECTIONHDR	pUxSectionHdr,	// Ux section header pointer.
	PBYTE			pData);		// Pointer to the section data.

LONG					// Return the error code.
LoaderGetLastError(				// Return the last error.
	PSTR	pszErrorText);		// Get the error text.

BOOL					// Return TRUE on success else FALSE.
InitSDRAM(						// Initializw SDRAM with the byte.
	DWORD	dwStartAddress,		// Start address.
	DWORD	dwEndAddress,		// End location.
	BYTE	Data);				// Data to be written to SDRAM.


BOOL					// Return TRUE on success else FALSE.
ReverseByteOrdering(				// Convert from Big Endian to Little Endian.
	PVOID	pBuffer,			// Pointer to the buffer to be converted.
	DWORD	SizeOfBuffer,		// Size of the buffer in bytes.
	BYTE	FormatConversion);


int CheckSectionAddrDataPair(PBYTE pData); 
// filter to check for chip reset. Adds 600ms delay
// in case Serial ROM is present. Parses for SDRAM 
// clock control and control register writes. Makes 
// accommidations for other SDRAM initialization values.

#define ByteSwap(x) (((x<<24)&0xff000000)|((x<<8)&0xff0000)|((x>>8)&0xff00)|((x>>24)&0xff))
//#define BitSwap(x) (((x<<7)&0x80)|((x<<5)&0x40)|((x<<3)&0x20)|((x<<1)&0x10)|((x>>1)&0x08)|((x>>3)&0x04)|((x>>5)&0x02)|((x>>7)&0x01))
#define FRAMESIZE25PAL 144000
#define FRAMESIZE25NTSC 120000
#define FRAMESIZE50PAL 288000
#define FRAMESIZE50NTSC 240000
#define FRAMESIZEAUDPAL 15360
#define FRAMESIZEAUDNTSC 12816

#define BUFSIZEMPEGV 262144
#define BUFSIZEMPEGA 1152
#define HEADERSIZEAUD 24
#define BUFSIZEMPEGAB 0xfc00

#define MPEG1   3
#define MPEG420 4
#define MPEG422 5
#define MPEGIMX 6
#define DV411   7
#define DV420   8
#define DV422   9
#define DV25MPEG420 10
#define DV25MPEG422 11
#define DV50MPEG422 12
#define MPEGDV      13
#define AVC420      14

enum MERLIN_OUTPUT_FILE_FORMAT {
    MERLIN_ES	= 0,				// elementary stream
    MERLIN_PS,						// program stream
    MERLIN_TS,						// transport stream
    MERLIN_AVI						// avi file format
};

#endif _TOOLS_H_
