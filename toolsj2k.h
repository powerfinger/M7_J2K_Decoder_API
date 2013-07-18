#include "tools.h"

#define SIGBYTER       0x53
#define SIGBYTEP       0x52
#define DESCOFFSETRJ2K 0x08000
#define DESCOFFSETPJ2K 0x0c000
#define TABLOFFSETJ2K  0x10000
#define BUFOFFSETJ2K   0x20000
#define NUMTBLSJ2K     0x400
#define NUMBUFSJ2K     8
#define NUMDESCJ2K     0x80

struct descripterj2k {
	int NextDscrptr;
	int BufPtr;
	int DMACnt;
	int BufPtrUpper;
	int PTime;
	int reserved5;
	int reserver6;
	int reserver7;
	int reserver8;
	int reserver9;
	int reserver10;
	int reserver11;
	int reserved12;
	int status;
	int reserved14;
	int reserved15;
};

typedef struct _BRDCTXJ2K {
	M2CONTEXT* m2c;
    int board;
	int dimx;
	int dimy;
	int bitdepth;
	int colorformat;
	int byteswap;
	int xyzconvert;
	int yuvconvert;
	int bufsize;
	int numbufs;
	int postcntsrc;
	int postcntdst;
	int checkcntsrc;
	int checkcntdst;
	int donetj;
	void* Callback;
	descripterj2k *srcdes;
	int  SrcBufpPtr[NUMBUFSJ2K];
	int *SrcBufvPtr[NUMBUFSJ2K];
	int  SrcDesPtr [NUMDESCJ2K];
	descripterj2k *dstdes;
	int  DstBufpPtr[NUMBUFSJ2K];
	int *DstBufvPtr[NUMBUFSJ2K];
	int  DstDesPtr [NUMDESCJ2K];
	int  TblpPtr;
	int *TblvPtr;
} BRDCTXJ2K, *PBRDCTXJ2K;

int  OpenJ2K            (BRDCTXJ2K *card, char *brd);
int  BufferRdyj2k_src   (BRDCTXJ2K *card, int bufnum);
int  BufferRdyj2k_dst   (BRDCTXJ2K *card, int bufnum);
void PostBufj2k_src     (BRDCTXJ2K *card, int bufnum, int framesize);
void PostBufj2k_dst     (BRDCTXJ2K *card, int bufnum, int framesize);
void Startj2k           (BRDCTXJ2K *card);
void CloseJ2K           (BRDCTXJ2K *card);
int  DecodeJ2K          (BRDCTXJ2K *card, void* bufin, int length, void* bufout);
int  RegisterCallbackJ2K(BRDCTXJ2K *card, void *function);
