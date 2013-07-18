#include <math.h>
#include "toolsj2k.h"

UINT ThreadJ (void * pParam);

void ipxwr(BRDCTXJ2K *card, int addr, int data)
{
	m2ddWrInt32(card->m2c,0xe00310,addr);
	m2ddWrInt32(card->m2c,0xe00314,data);
	m2ddWrInt32(card->m2c,0xe00318,0);
	while (m2ddRdInt32(card->m2c,0xe00318)) Sleep(10);
}

int ipxrd(BRDCTXJ2K *card, int addr)
{
	m2ddWrInt32(card->m2c,0xe00310,addr);
	m2ddWrInt32(card->m2c,0xe00318,1);
	while (m2ddRdInt32(card->m2c,0xe00318)) Sleep(10);
	return m2ddRdInt32(card->m2c,0xe00314);
}

int OpenJ2K(BRDCTXJ2K *card, char *brd)
{
	if      (brd[2]=='2') card->board=1;
	else if (brd[2]=='3') card->board=2;
	else if (brd[2]=='4') card->board=3;
	else                  card->board=0;

	unsigned long mapping[20];
	int physicalAddress,virtualAddress,physicalAddressUpper,virtualAddressUpper,i,btype;
	unsigned long result1,result2,controlword,Buffer_Megs;

	card->bufsize = card->dimx*card->dimy;
	if      (card->colorformat==0) card->bufsize*=2;
	else if (card->colorformat==1) card->bufsize*=3;
	else if (card->colorformat==2) card->bufsize*=4;
	else if (card->colorformat==3) card->bufsize*=4;

	if      (card->bitdepth==8)   card->bufsize*=1;
	else if (card->bitdepth==10) {card->bufsize*=4;card->bufsize/=3;}
	else if (card->bitdepth==12) {card->bufsize*=3;card->bufsize/=2;}
	else if (card->bitdepth==16)  card->bufsize*=2;

	card->numbufs = max(2,(256*1024*1024-card->bufsize-BUFOFFSETJ2K)/2/card->bufsize);
	card->numbufs = min(NUMBUFSJ2K,card->numbufs);

	m2ddWriteInt32(card->board,0xe000e0,0xffff3fff);
	Sleep(100);
//Remove DRAM reset
	m2ddWriteInt32(card->board,0xe000e0,0xfbff3fff);
	Sleep(100);
	m2ddWriteInt32(card->board,0xe000e0,0xf9ff3fff);
	Sleep(100);
	m2ddWriteInt32(card->board,0xe000e0,0xf0ff3fff);
	Sleep(1000);
//Remove Decoder resets
	m2ddWriteInt32(card->board,0xe000e0,0x80f33fff);
	Sleep(100);

	if (m2ddMapMemory(card->board,mapping) == 1) {
		printf("Can't find Merlin Card. Press any key to exit.\n");
		return 0;
	}
	physicalAddress      = mapping[0];
	virtualAddress       = mapping[2];
	Buffer_Megs          = mapping[4];
	
	card->m2c = (M2CONTEXT*)virtualAddress;
	printf("\nOpening JPEG2000 Hardware Libraray");
	printf("\nDriver Rev = 0x%x\n",result1 = card->m2c->Revision);

	if ((result1 < DRVREVMIN)||(result1 > DRVREVMAX)) {
		printf("This program only supports Driver rev 0x%02x thru 0x%02x.\nInstall the correct Driver if you want to use this program.",DRVREVMIN,DRVREVMAX);
		return 0;
	}

	printf("Driver Buf = %d MBs\n",result1 = Buffer_Megs);
	if ((result1*1024*1024) < (card->numbufs*card->bufsize*2+card->bufsize+BUFOFFSETJ2K)) {
		printf("This program requires at least %d MBs.\nIncrease your buffer setting if you want to use this program.\nPress any key.",(card->numbufs*card->bufsize*2+card->bufsize+BUFOFFSETJ2K));
		return 0;
	}

	card->m2c->board = card->board;
	result1 = m2ddRdInt32(card->m2c,0xe00008);
	if ((result1&0xff000000)!=0x22000000) {
		printf("This program only supports MerlinHD boards.\nPress any key.");
		return 0;
	}

	printf("FPGA Rev = 0x%x\n",result1&0xff);

	result2 = m2ddRdInt32(card->m2c,0xe00048);
	btype = (result1&0xffff0000)|(result2&0xff);
	if      ((btype&0xffff00ff)==0x22020011) printf("MerlinIP PCIe Gen 1 @ x1 - Max Transfer Rate = 250 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020021) printf("MerlinIP PCIe Gen 1 @ x2 - Max Transfer Rate = 500 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020031) printf("MerlinIP PCIe Gen 1 @ x3 - Max Transfer Rate = 750 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020041) printf("MerlinIP PCIe Gen 1 @ x4 - Max Transfer Rate = 1,000 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020051) printf("MerlinIP PCIe Gen 1 @ x5 - Max Transfer Rate = 1,250 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020061) printf("MerlinIP PCIe Gen 1 @ x6 - Max Transfer Rate = 1,500 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020071) printf("MerlinIP PCIe Gen 1 @ x7 - Max Transfer Rate = 1,750 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020081) printf("MerlinIP PCIe Gen 1 @ x8 - Max Transfer Rate = 2,000 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020012) printf("MerlinIP PCIe Gen 2 @ x1 - Max Transfer Rate = 500 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020022) printf("MerlinIP PCIe Gen 2 @ x2 - Max Transfer Rate = 1,000 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020032) printf("MerlinIP PCIe Gen 2 @ x3 - Max Transfer Rate = 1,500 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020042) printf("MerlinIP PCIe Gen 2 @ x4 - Max Transfer Rate = 2,000 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020052) printf("MerlinIP PCIe Gen 2 @ x5 - Max Transfer Rate = 2,500 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020062) printf("MerlinIP PCIe Gen 2 @ x6 - Max Transfer Rate = 3,000 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020072) printf("MerlinIP PCIe Gen 2 @ x7 - Max Transfer Rate = 3,500 MB/s\n");
	else if ((btype&0xffff00ff)==0x22020082) printf("MerlinIP PCIe Gen 2 @ x8 - Max Transfer Rate = 4,000 MB/s\n");


//	m2ddWriteInt32(card->board,0xe000e0,0x00ffffff);

//source dma

	m2ddWriteInt32(card->board,0xe0015c+0x40,0);          //set upper address to 0
	m2ddWriteInt32(card->board,0xe00140+0x40,0);          //reset dma channel
	m2ddReadInt32(card->board,0xe00140+0x40,result1);
	while ((result1&0xff000000) != 0) {				 //wait until DMA ctrl idle
		Sleep(10);
		m2ddReadInt32(card->board,0xe00140+0x40,result1);
	}

	card->dstdes = (descripterj2k*)(virtualAddress + DESCOFFSETRJ2K);

	card->TblpPtr =      ((physicalAddress + TABLOFFSETJ2K + 0x1fff)&0xffffe000);
	card->TblvPtr = (int*)(virtualAddress + (card->TblpPtr - physicalAddress));
	for (i=0; i<0x800; i++) card->TblvPtr[i] = 0;

	//init Descriptor Pointers, Sample Physical pointers, Sample Virtual Pointers
	for (i=0;i<NUMDESCJ2K;i++) card->DstDesPtr[i] = physicalAddress+DESCOFFSETRJ2K+(sizeof(descripterj2k)*i);
	for (i=0;i<card->numbufs;i++) {
		card->DstBufpPtr[i] =       physicalAddress+BUFOFFSETJ2K+(card->bufsize*i); 
		card->DstBufvPtr[i] = (int*)(virtualAddress+BUFOFFSETJ2K+(card->bufsize*i)); 
	}

	//init Descriptors
	for (i=0;i<NUMDESCJ2K;i++) {
		card->dstdes[i].NextDscrptr = card->DstDesPtr[(i+1)%NUMDESCJ2K];
		card->dstdes[i].BufPtr      = 0;
		card->dstdes[i].DMACnt      = 0;
		card->dstdes[i].BufPtrUpper = 0;
		card->dstdes[i].PTime       = 0;
		card->dstdes[i].status      = 0;
	}

	m2ddWriteInt32(card->board,0xe0014c+0x40,card->DstDesPtr[0]); //set last dma descriptor to 0
	m2ddWriteInt32(card->board,0xe00150+0x40,card->DstDesPtr[0]); //set first dma descriptor to 0

//destination dma

	m2ddWriteInt32(card->board,0xe0017c+0x40,0);          //set upper address to 0
	m2ddWriteInt32(card->board,0xe00160+0x40,0);          //reset dma channel
	m2ddReadInt32(card->board,0xe00160+0x40,result1);
	while ((result1&0xff000000) != 0) {				 //wait until DMA ctrl idle
		Sleep(10);
		m2ddReadInt32(card->board,0xe00160+0x40,result1);
	}

	card->srcdes = (descripterj2k*)(virtualAddress + DESCOFFSETPJ2K);

	//init Descriptor Pointers, Sample Physical pointers, Sample Virtual Pointers
	for (i=0;i<NUMDESCJ2K;i++) card->SrcDesPtr[i] = physicalAddress+DESCOFFSETPJ2K+(sizeof(descripterj2k)*i);
	for (i=0;i<card->numbufs;i++) {
		card->SrcBufpPtr[i] =       physicalAddress+BUFOFFSETJ2K+((card->numbufs+1)*card->bufsize)+(card->bufsize*i); 
		card->SrcBufvPtr[i] = (int*)(virtualAddress+BUFOFFSETJ2K+((card->numbufs+1)*card->bufsize)+(card->bufsize*i)); 
	}

	//init Descriptors
	for (i=0;i<NUMDESCJ2K;i++) {
		card->srcdes[i].NextDscrptr = card->SrcDesPtr[(i+1)%NUMDESCJ2K];
		card->srcdes[i].BufPtr      = 0;
		card->srcdes[i].DMACnt      = 0;
		card->srcdes[i].BufPtrUpper = 0;
		card->srcdes[i].PTime       = 0;
		card->srcdes[i].status      = 0;
	}

	m2ddWriteInt32(card->board,0xe0016c+0x40,card->SrcDesPtr[0]); //set last dma descriptor to 0
	m2ddWriteInt32(card->board,0xe00170+0x40,card->SrcDesPtr[0]); //set first dma descriptor to 0

	int wrword;
	if      (card->bitdepth==8)    wrword  = 0x0;
	else if (card->bitdepth==10)   wrword  = 0x1;
	else if (card->bitdepth==12)   wrword  = 0x2;
	else if (card->bitdepth==16)   wrword  = 0x3;
	if      (card->colorformat==0) wrword |= 0x0;
	else if (card->colorformat==1) wrword |= 0x4;
	else if (card->colorformat==2) wrword |= 0x8;
	else if (card->colorformat==3) wrword |= 0xc;
	if      (card->byteswap)       wrword |= 0x10;
	if      (card->xyzconvert)     wrword |= 0x20;
	if      (card->yuvconvert)     wrword |= 0x40;

	m2ddWrInt32(card->m2c,0xe0031c,wrword);

//Start PRP and POP
	ipxwr(card,0,0x10);
	ipxwr(card,0x100,0x12);
//	ipxwr(card,0x10,0x73009000);
	Sleep(1000);

	Startj2k(card);

	card->postcntsrc=0;
	card->postcntdst=0;
	card->checkcntsrc=0;
	card->checkcntdst=0;
	for (int i=0;i<card->numbufs;i++) {
		PostBufj2k_dst(card,card->postcntdst++,card->bufsize);
	}

	card->donetj=0;
	card->Callback=0;
	AfxBeginThread(ThreadJ, card, THREAD_PRIORITY_TIME_CRITICAL );

	return 1;
}


void PostBufj2k_src(BRDCTXJ2K *card, int bufnum, int framesize)
{
	card->srcdes[bufnum%NUMDESCJ2K].BufPtr = card->SrcBufpPtr[bufnum%card->numbufs];
	card->srcdes[bufnum%NUMDESCJ2K].DMACnt = (SIGBYTEP<<24)|framesize/8;
	card->srcdes[bufnum%NUMDESCJ2K].PTime  = 0;
	card->srcdes[bufnum%NUMDESCJ2K].status = 0;

	m2ddWriteInt32(card->board,0xe0016c+0x40,card->SrcDesPtr[(bufnum+1)%NUMDESCJ2K]); //set last dma descriptor
}

void PostBufj2k_dst(BRDCTXJ2K *card, int bufnum, int framesize)
{
	card->dstdes[bufnum%NUMDESCJ2K].BufPtr = card->DstBufpPtr[bufnum%card->numbufs];
	card->dstdes[bufnum%NUMDESCJ2K].DMACnt = (SIGBYTER<<24)|framesize/8;
	card->dstdes[bufnum%NUMDESCJ2K].PTime  = 0;
	card->dstdes[bufnum%NUMDESCJ2K].status = 0;

	m2ddWriteInt32(card->board,0xe0014c+0x40,card->DstDesPtr[(bufnum+1)%NUMDESCJ2K]); //set last dma descriptor
}

int BufferRdyj2k_src(BRDCTXJ2K *card, int bufnum)
{
	if  (card->srcdes[bufnum%NUMDESCJ2K].status == 0) return 0;
	else 		                                   return 1;
}

int BufferRdyj2k_dst(BRDCTXJ2K *card, int bufnum)
{
	if  (card->dstdes[bufnum%NUMDESCJ2K].status == 0) return 0;
	else 		                                   return 1;
}

void Startj2k(BRDCTXJ2K *card)
{
   	m2ddWriteInt32(card->board,0xe00140+0x40,card->TblpPtr + 0x89);
   	m2ddWriteInt32(card->board,0xe00160+0x40,0x9);
}

void CloseJ2K(BRDCTXJ2K *card)
{
	unsigned long result1;

	printf("\nClosing JPEG2000 Hardware Libraray\n");
	card->donetj=1;
	while (card->donetj!=2) Sleep(10);
	m2ddWriteInt32(card->board,0xe00140+0x40,0);          //reset dma channel
	m2ddReadInt32(card->board,0xe00140+0x40,result1);
	while ((result1&0xff000000) != 0) {				 //wait until DMA ctrl idle
		Sleep(10);
		m2ddReadInt32(card->board,0xe00140+0x40,result1);
	}

	m2ddWriteInt32(card->board,0xe00160+0x40,0);          //reset dma channel
	m2ddReadInt32(card->board,0xe00160+0x40,result1);
	while ((result1&0xff000000) != 0) {				 //wait until DMA ctrl idle
		Sleep(10);
		m2ddReadInt32(card->board,0xe00160+0x40,result1);
	}
}

int DecodeJ2K(BRDCTXJ2K *card, void* bufin, int length, void* bufout)
{
	int srdy = 0;
	int drdy = 0;
	char* buftail;
	int framesize = length;

	memcpy(card->SrcBufvPtr[card->postcntsrc%card->numbufs],bufin,length);
	buftail  = (char *)card->SrcBufvPtr[card->postcntsrc%card->numbufs];
	for (int tf = 0; tf < 15; tf++) buftail[framesize+tf] = 0;
	framesize += 15;
	framesize &= 0xfffffff0;

	if (card->Callback) {
		while ((card->postcntsrc-card->checkcntsrc)==card->numbufs) Sleep(10);
		PostBufj2k_src(card,card->postcntsrc++,framesize);
	}
	else {
		PostBufj2k_src(card,card->postcntsrc++,framesize);
		srdy = BufferRdyj2k_src(card,card->checkcntsrc);
		while (!srdy) {
			Sleep(10);
			srdy = BufferRdyj2k_src(card,card->checkcntsrc);
		}
		card->srcdes[card->checkcntsrc%NUMDESCJ2K].status = 0;
		card->checkcntsrc++;

		drdy = BufferRdyj2k_dst(card,card->checkcntdst);
		while (!drdy) {
			Sleep(10);
			drdy = BufferRdyj2k_dst(card,card->checkcntdst);
		}
		memcpy(bufout,card->DstBufvPtr[card->checkcntdst%card->numbufs],card->bufsize);
		card->dstdes[card->checkcntdst%NUMDESCJ2K].status = 0;
		PostBufj2k_dst(card,card->postcntdst++,card->bufsize);
		card->checkcntdst++;
	}

	return 0;
}

int RegisterCallbackJ2K(BRDCTXJ2K *card, void *function)
{
	card->Callback = function;
	return 0;
}


void tj(BRDCTXJ2K *card) {
	int callbackset=0;
	void (*callback)(int*);

	while(card->donetj!=1) {
		if (callbackset) {
			int srdy = BufferRdyj2k_src(card,card->checkcntsrc);
			int drdy = BufferRdyj2k_dst(card,card->checkcntdst);
			if (srdy) {
				card->srcdes[card->checkcntsrc%NUMDESCJ2K].status = 0;
				card->checkcntsrc++;
			}
			if (drdy) {
				card->dstdes[card->checkcntdst%NUMDESCJ2K].status = 0;
				callback(card->DstBufvPtr[card->checkcntdst%card->numbufs]);
				PostBufj2k_dst(card,card->postcntdst++,card->bufsize);
				card->checkcntdst++;
			}
			if (!srdy & !drdy) Sleep(10);
		}
		else {
			if (card->Callback) {
				callbackset=1;
				callback = (void (__cdecl *)(int*))card->Callback;
			}
			Sleep(10);
		}
	}
	card->donetj=2;
}

UINT ThreadJ (void * pParam) { BRDCTXJ2K *card; card = (BRDCTXJ2K*)pParam; tj(card);   return 0;}
