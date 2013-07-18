#include "stdafx.h"
#include "DEVIOCTL.h"
#include "tools.h"
#include <conio.h>
#include <io.h>

_int64 ReadUTS(int board) {
	unsigned long a,b,c;
	m2ddReadInt32(board,0xe0002c,a);     //read upper
	b = a++;
	while (a!=b) {                   //compare last 2 reads of upper
		b = a;
		m2ddReadInt32(board,0xe00028,c); //read lower
		m2ddReadInt32(board,0xe0002c,a); //read upper
	}
	return ((a<<32)|c);
}

void resetreftc(int board)
{
	unsigned long data;
	unsigned long uts1,uts2;
	int i;

	m2ddReadInt32(board,0xe00008,data);
	if ((data>>24)==0x22) return;
	uts2=50000;
	uts1=0;
	while ((uts2-uts1)>40000) {
		m2ddReadInt32(board,0xe00028,uts1);
		m2ddReadInt32(board,0xe00050,data);
		i = (data>>11)&0x3ff;
		while ((i<50)||(i>150)) {
			m2ddReadInt32(board,0xe00028,uts1);
			m2ddReadInt32(board,0xe00050,data);
			i = (data>>11)&0x3ff;
		}
		m2ddWriteInt32(board,0xe00058,0);
		m2ddReadInt32(board,0xe00028,uts2);
	}
}

int loadf2(int board, int key, char *filename) 
{
	unsigned long ack;
	int bytecnt,timeout,i;
	char s[1024];
	unsigned char bytes[4096];

	int prginit = 0x200;
	int prgdone = 0x100;
	int prgccnt = 0x800;
	unsigned long prgword;

	m2ddWriteInt32(board,0xe00004,0xfffffeff);
	Sleep(10);
	m2ddWriteInt32(board,0xe00004,0xfffff0ff+(key<<8));

	timeout = 0;
	m2ddReadInt32(board,0xe00004,ack);
	while (((ack&prginit)==0)&&(timeout!=100)) {
		timeout++;
		Sleep(10);
		m2ddReadInt32(board,0xe00004,ack);
	}
	if (timeout == 100) {
		m2ddWriteInt32(board,0xe00004,0xffffffff);
		return 1; //FPGA Init Error
	}

	FILE *f1;
	if ((f1 = fopen(filename, "rb")) == NULL) {
		m2ddWriteInt32(board,0xe00004,0xffffffff);
		return 2; //File error
	}
	unsigned long bytew[4];
	s[2] = 0;
	while ((bytecnt = fread(bytes, 1, 1024, f1))>0) {
		for (i = 0; i < bytecnt/2; i=i+4) {
			s[0] = bytes[2*i];
			s[1] = bytes[2*i+1];
			sscanf(s,"%x",&bytew[0]);
			s[0] = bytes[2*i+2];
			s[1] = bytes[2*i+3];
			sscanf(s,"%x",&bytew[1]);
			s[0] = bytes[2*i+4];
			s[1] = bytes[2*i+5];
			sscanf(s,"%x",&bytew[2]);
			s[0] = bytes[2*i+6];
			s[1] = bytes[2*i+7];
			sscanf(s,"%x",&bytew[3]);
			prgword = ((bytew[3]<<24)&0xff000000)
					+ ((bytew[2]<<16)&0x00ff0000)
					+ ((bytew[1]<<8) &0x0000ff00)
					+ ((bytew[0])    &0x000000ff);
			ack = 0;
			while ((ack&prgccnt)==0) m2ddReadInt32(board,0xe00004,ack);
			m2ddWriteInt32(board,0xe00008,prgword);
		}
	}
	fclose(f1);

	timeout = 0;
	m2ddReadInt32(board,0xe00004,ack);
	while (((ack&prgdone)==0)&&(timeout!=100)) {
		timeout++;
		Sleep(10);
		m2ddReadInt32(board,0xe00004,ack);
	}

	if (timeout == 100) {
		m2ddWriteInt32(board,0xe00004,0xffffffff);
		return 3; //FPGA Done Error
	}
	Sleep(100);
	m2ddWriteInt32(board,0xe00004,0xffffffff);
	return 0;
}

void SetAudioLevels(M2CONTEXT *m2c, int init, int gain, int vol) 
{
	unsigned char bytes[4096];
	int gainm, volm;

	//new audio IC
	if (init) {
		m2ddWrInt32(m2c,0xe00018,7);
		Sleep(10);
		m2ddWrInt32(m2c,0xe00018,3);
		Sleep(10);
		bytes[0] = 0x9e;bytes[1] = 0x2;bytes[0x02] = 0x08;
		PIOCY(m2c->board,bytes,0x3);
	}
	gainm  = gain*24;
	gainm /=15;
	bytes[0] = 0x9e;bytes[1] = 0x7;	bytes[0x02] = bytes[0x03] = gainm;
	PIOCY(m2c->board,bytes,4);
	volm  = vol*80;
	volm /= 27;
	bytes[0] = 0x9e;bytes[1] = 0xa;	bytes[0x02] = bytes[0x03] = volm;
	PIOCY(m2c->board,bytes,4);

	//old audio IC
	m2ddWrInt32(m2c,0xe0009c,(m2ddRdScratch(m2c,0xe0009c)&0xffff0000)|((vol*256)+gain));
}

void i2cinit(M2CONTEXT *m2c, int standard, int panelb) 
{
	int i;
	unsigned char bytes[4096];

	//init analog input chip

	bytes[0] = 0x42;bytes[1] = 0;
	bytes[0x00+2] = 0x00;bytes[0x01+2] = 0x47;bytes[0x02+2] = 0xC5;bytes[0x03+2] = 0x10;bytes[0x04+2] = 0x90;
	bytes[0x05+2] = 0x90;bytes[0x06+2] = 0xEB;bytes[0x07+2] = 0xE0;bytes[0x08+2] = 0x98;bytes[0x09+2] = 0x40;
	bytes[0x0A+2] = 0x80;bytes[0x0B+2] = 0x44;bytes[0x0C+2] = 0x40;bytes[0x0D+2] = 0x00;bytes[0x0E+2] = 0x89;
	bytes[0x0F+2] = 0x2A;bytes[0x10+2] = 0x0E;bytes[0x11+2] = 0x00;bytes[0x12+2] = 0x00;bytes[0x13+2] = 0x00;
	bytes[0x14+2] = 0x00;bytes[0x15+2] = 0x11;bytes[0x16+2] = 0xFE;bytes[0x17+2] = 0x00;bytes[0x18+2] = 0x40;
	bytes[0x19+2] = 0x80;bytes[0x1A+2] = 0x00;bytes[0x1B+2] = 0x00;bytes[0x1C+2] = 0x00;bytes[0x1D+2] = 0x00;
	PIO(m2c->board,bytes,0x1e+2);

	bytes[0] = 0x42;bytes[1] = 0x23;
	bytes[0x00+2] = 0x00;bytes[0x01+2] = 0x00;bytes[0x02+2] = 0x00;bytes[0x03+2] = 0x00;bytes[0x04+2] = 0x00;
	bytes[0x05+2] = 0x00;bytes[0x06+2] = 0x00;bytes[0x07+2] = 0x00;bytes[0x08+2] = 0x00;bytes[0x09+2] = 0x00;
	bytes[0x0a+2] = 0x00;bytes[0x0b+2] = 0x00;bytes[0x0c+2] = 0x00;
	PIO(m2c->board,bytes,15);

	bytes[0] = 0x42;bytes[1] = 0x83;
	bytes[0x00+2] = 0x01;
	PIO(m2c->board,bytes,3);

	bytes[0] = 0x42;bytes[1] = 0x88;
	bytes[0x00+2] = 0xfe;
	PIO(m2c->board,bytes,3);

	//init analog output chip

	if ((standard == NTSC)||(standard == NTSCNDF)) {
		m2ddWrInt32(m2c,0xe00098,m2ddRdInt32(m2c,0xe00098)&0xfffff7ff);
		for (i=0;i<131;i++) bytes[i] =0;
		bytes[0]   = 0x88;bytes[1]   = 0;

		bytes[42]  = 0x19;bytes[43]  = 0x1d;bytes[47]  = 0xFF;bytes[60]  = 0x3b;
//		bytes[92]  = 0x60;bytes[93]  = 0x7d;bytes[94]  = 0xb2;bytes[95]  = 0x3c;
		bytes[92]  = 0x60;bytes[93]  = 0x64;bytes[94]  = 0x8e;bytes[95]  = 0x30;
//		bytes[96]  = 0x7a;bytes[97]  = 0x3a;bytes[99]  = 0x15;bytes[100] = 0x66;
		bytes[96]  = 0x24;bytes[97]  = 0x24;bytes[99]  = 0x15;bytes[100] = 0x36;
		bytes[101] = 0x1f;bytes[102] = 0x7c;bytes[103] = 0xf0;bytes[104] = 0x21;
		bytes[109] = 0x00;bytes[110] = 0xf9;bytes[112] = 0x30;bytes[113] = 0x00;
		bytes[124] = 0x00;bytes[125] = 0xff;bytes[126] = 0x40;
		if (panelb) bytes[47]  = 0x8F;
		if ((m2ddRdInt32(m2c,0xe00004)&0x80000000)==0) {
			bytes[47]  = 0x38;
			bytes[110] = 0xea;
			if (panelb) bytes[47]  = 0x55;
		}
		PIO(m2c->board,bytes,131);
	}
	else {
		m2ddWrInt32(m2c,0xe00098,m2ddRdInt32(m2c,0xe00098)|0x800);
		for (i=0;i<131;i++) bytes[i] =0;
		bytes[0]   = 0x88;bytes[1]   = 0;
		bytes[42]  = 0x21;bytes[43]  = 0x1d;bytes[47]  = 0xFF;bytes[60]  = 0x3b;
		bytes[92]  = 0x60;bytes[93]  = 0x7d;bytes[94]  = 0xb2;bytes[95]  = 0x3d;
		bytes[96]  = 0x7a;bytes[97]  = 0x3a;bytes[99]  = 0x16;bytes[100] = 0x48;
		bytes[101] = 0xcb;bytes[102] = 0x8a;bytes[103] = 0x09;bytes[104] = 0x2a;
		bytes[109] = 0x00;bytes[110] = 0xfd;bytes[112] = 0x00;bytes[113] = 0x00;
		bytes[124] = 0x00;bytes[125] = 0xff;bytes[126] = 0x40;
		if (panelb) bytes[47]  = 0x8F;
		if ((m2ddRdInt32(m2c,0xe00004)&0x80000000)==0) {
			bytes[47]  = 0x38;
			bytes[110] = 0xf6;
			if (panelb) bytes[47]  = 0x55;
		}
		PIO(m2c->board,bytes,131);
	}
}

void SetVideoInput(M2CONTEXT *m2c, int input)
{
	unsigned char bytes[4096];
	int pal;
	// TODO: Add your control notification handler code here
	pal = m2ddRdInt32(m2c,0xe00098)&0x800;
	if (input == FREERUN) {
		m2ddWrInt32(m2c,0xe000c8,m2ddRdScratch(m2c,0xe000c8)&0xfeffffff);
		m2ddWrInt32(m2c,0xe00098,m2ddRdInt32(m2c,0xe00098)|0x1000);
		m2ddWrInt32(m2c,0xe0009c,(m2ddRdScratch(m2c,0xe0009c)&0xfffbffff)|0x10000);
	}
	else if (input == CVIDEO) {
		m2ddWrInt32(m2c,0xe000c8,m2ddRdScratch(m2c,0xe000c8)&0xfeffffff);
		m2ddWrInt32(m2c,0xe00098,m2ddRdInt32(m2c,0xe00098)|0x1000);
		m2ddWrInt32(m2c,0xe0009c,m2ddRdScratch(m2c,0xe0009c)&0xfffaffff);
		bytes[0] = 0x42;bytes[1] = 0x23;
		bytes[0x00+2] = 0x00;bytes[0x01+2] = 0x00;bytes[0x02+2] = 0x00;bytes[0x03+2] = 0x00;bytes[0x04+2] = 0x00;
		bytes[0x05+2] = 0x00;bytes[0x06+2] = 0x00;bytes[0x07+2] = 0x00;bytes[0x08+2] = 0x00;bytes[0x09+2] = 0x00;
		bytes[0x0a+2] = 0x00;bytes[0x0b+2] = 0x00;bytes[0x0c+2] = 0x00;
		PIO(m2c->board,bytes,15);
		if (pal) {
			bytes[0] = 0x42;bytes[1] = 0x8;
			bytes[0x00+2] = 0x98;
			bytes[0x01+2] = 0x40;
			PIO(m2c->board,bytes,4);
			bytes[0] = 0x42;bytes[1] = 0x2;
			bytes[0x00+2] = 0xc5;
			PIO(m2c->board,bytes,3);
		}
		else {
			bytes[0] = 0x42;bytes[1] = 0x8;
			bytes[0x00+2] = 0x98;
			bytes[0x01+2] = 0x40;
			PIO(m2c->board,bytes,4);
			bytes[0] = 0x42;bytes[1] = 0x2;
			bytes[0x00+2] = 0xc5;
			PIO(m2c->board,bytes,3);
		}
	}
	else if (input == CVIDEO2) {
		m2ddWrInt32(m2c,0xe000c8,m2ddRdScratch(m2c,0xe000c8)|0x01000000);
		m2ddWrInt32(m2c,0xe00098,m2ddRdInt32(m2c,0xe00098)|0x1000);
		m2ddWrInt32(m2c,0xe0009c,m2ddRdScratch(m2c,0xe0009c)&0xfffaffff);
		bytes[0] = 0x42;bytes[1] = 0x23;
		bytes[0x00+2] = 0x00;bytes[0x01+2] = 0x00;bytes[0x02+2] = 0x00;bytes[0x03+2] = 0x00;bytes[0x04+2] = 0x00;
		bytes[0x05+2] = 0x00;bytes[0x06+2] = 0x00;bytes[0x07+2] = 0x00;bytes[0x08+2] = 0x00;bytes[0x09+2] = 0x00;
		bytes[0x0a+2] = 0x00;bytes[0x0b+2] = 0x00;bytes[0x0c+2] = 0x00;
		PIO(m2c->board,bytes,15);
		if (pal) {
			bytes[0] = 0x42;bytes[1] = 0x8;
			bytes[0x00+2] = 0x98;
			bytes[0x01+2] = 0x40;
			PIO(m2c->board,bytes,4);
			bytes[0] = 0x42;bytes[1] = 0x2;
			bytes[0x00+2] = 0xdf;
			PIO(m2c->board,bytes,3);
		}
		else {
			bytes[0] = 0x42;bytes[1] = 0x8;
			bytes[0x00+2] = 0x98;
			bytes[0x01+2] = 0x40;
			PIO(m2c->board,bytes,4);
			bytes[0] = 0x42;bytes[1] = 0x2;
			bytes[0x00+2] = 0xdf;
			PIO(m2c->board,bytes,3);
		}
	}
	else if (input == SVIDEO) {
		m2ddWrInt32(m2c,0xe000c8,m2ddRdScratch(m2c,0xe000c8)&0xfeffffff);
		m2ddWrInt32(m2c,0xe00098,m2ddRdInt32(m2c,0xe00098)|0x1000);
		m2ddWrInt32(m2c,0xe0009c,m2ddRdScratch(m2c,0xe0009c)&0xfffaffff);
		bytes[0] = 0x42;bytes[1] = 0x23;
		bytes[0x00+2] = 0x00;bytes[0x01+2] = 0x00;bytes[0x02+2] = 0x00;bytes[0x03+2] = 0x00;bytes[0x04+2] = 0x00;
		bytes[0x05+2] = 0x00;bytes[0x06+2] = 0x00;bytes[0x07+2] = 0x00;bytes[0x08+2] = 0x00;bytes[0x09+2] = 0x00;
		bytes[0x0a+2] = 0x00;bytes[0x0b+2] = 0x00;bytes[0x0c+2] = 0x00;
		PIO(m2c->board,bytes,15);
		if (pal) {
			bytes[0] = 0x42;bytes[1] = 0x8;
			bytes[0x00+2] = 0x98;
			bytes[0x01+2] = 0xc0;
			PIO(m2c->board,bytes,4);
			bytes[0] = 0x42;bytes[1] = 0x2;
			bytes[0x00+2] = 0xdd;
			PIO(m2c->board,bytes,3);
		}
		else {
			bytes[0] = 0x42;bytes[1] = 0x8;
			bytes[0x00+2] = 0x98;
			bytes[0x01+2] = 0xc0;
			PIO(m2c->board,bytes,4);
			bytes[0] = 0x42;bytes[1] = 0x2;
			bytes[0x00+2] = 0xdd;
			PIO(m2c->board,bytes,3);
		}
	}
	else if (input == SDI) {
		m2ddWrInt32(m2c,0xe000c8,m2ddRdScratch(m2c,0xe000c8)&0xfeffffff);
		m2ddWrInt32(m2c,0xe00098,m2ddRdInt32(m2c,0xe00098)&0xffffefff);
		m2ddWrInt32(m2c,0xe0009c,m2ddRdScratch(m2c,0xe0009c)&0xfffaffff);
		bytes[0] = 0x42;bytes[1] = 0x23;
		bytes[0x00+2] = 0x00;bytes[0x01+2] = 0x00;bytes[0x02+2] = 0x00;bytes[0x03+2] = 0x00;bytes[0x04+2] = 0x00;
		bytes[0x05+2] = 0x00;bytes[0x06+2] = 0x00;bytes[0x07+2] = 0x00;bytes[0x08+2] = 0x00;bytes[0x09+2] = 0x00;
		bytes[0x0a+2] = 0x00;bytes[0x0b+2] = 0x00;bytes[0x0c+2] = 0x00;
		PIO(m2c->board,bytes,15);
		if (pal) {
			bytes[0] = 0x42;bytes[1] = 0x8;
			bytes[0x00+2] = 0x98;
			bytes[0x01+2] = 0x40;
			PIO(m2c->board,bytes,4);
			bytes[0] = 0x42;bytes[1] = 0x2;
			bytes[0x00+2] = 0xc5;
			PIO(m2c->board,bytes,3);
		}
		else {
			bytes[0] = 0x42;bytes[1] = 0x8;
			bytes[0x00+2] = 0x98;
			bytes[0x01+2] = 0x40;
			PIO(m2c->board,bytes,4);
			bytes[0] = 0x42;bytes[1] = 0x2;
			bytes[0x00+2] = 0xc5;
			PIO(m2c->board,bytes,3);
		}
	}
	else if (input == HEADER) {
		m2ddWrInt32(m2c,0xe000c8,m2ddRdScratch(m2c,0xe000c8)&0xfeffffff);
		m2ddWrInt32(m2c,0xe00098,m2ddRdInt32(m2c,0xe00098)&0xffffefff);
		m2ddWrInt32(m2c,0xe0009c,(m2ddRdScratch(m2c,0xe0009c)&0xfffaffff)|0x00040000);
		bytes[0] = 0x42;bytes[1] = 0x23;
		bytes[0x00+2] = 0x00;bytes[0x01+2] = 0x00;bytes[0x02+2] = 0x00;bytes[0x03+2] = 0x00;bytes[0x04+2] = 0x00;
		bytes[0x05+2] = 0x00;bytes[0x06+2] = 0x00;bytes[0x07+2] = 0x00;bytes[0x08+2] = 0x00;bytes[0x09+2] = 0x00;
		bytes[0x0a+2] = 0x00;bytes[0x0b+2] = 0x00;bytes[0x0c+2] = 0x00;
		PIO(m2c->board,bytes,15);
		if (pal) {
			bytes[0] = 0x42;bytes[1] = 0x8;
			bytes[0x00+2] = 0x98;
			bytes[0x01+2] = 0x40;
			PIO(m2c->board,bytes,4);
			bytes[0] = 0x42;bytes[1] = 0x2;
			bytes[0x00+2] = 0xc5;
			PIO(m2c->board,bytes,3);
		}
		else {
			bytes[0] = 0x42;bytes[1] = 0x8;
			bytes[0x00+2] = 0x98;
			bytes[0x01+2] = 0x40;
			PIO(m2c->board,bytes,4);
			bytes[0] = 0x42;bytes[1] = 0x2;
			bytes[0x00+2] = 0xc5;
			PIO(m2c->board,bytes,3);
		}
	}
	else if (input == COMPONENT) {
		m2ddWrInt32(m2c,0xe000c8,m2ddRdScratch(m2c,0xe000c8)|0x01000000);
		m2ddWrInt32(m2c,0xe00098,m2ddRdInt32(m2c,0xe00098)|0x1000);
		m2ddWrInt32(m2c,0xe0009c,m2ddRdScratch(m2c,0xe0009c)&0xfffaffff);
		bytes[0] = 0x42;bytes[1] = 0x23;
		bytes[0x00+2] = 0x00;bytes[0x01+2] = 0x90;bytes[0x02+2] = 0x90;bytes[0x03+2] = 0x00;bytes[0x04+2] = 0x00;
		bytes[0x05+2] = 0x00;bytes[0x06+2] = 0x40;bytes[0x07+2] = 0xb6;bytes[0x08+2] = 0x68;bytes[0x09+2] = 0x47;
		bytes[0x0a+2] = 0x00;bytes[0x0b+2] = 0x00;bytes[0x0c+2] = 0x00;
		PIO(m2c->board,bytes,15);
		if (pal) {
			bytes[0] = 0x42;bytes[1] = 0x8;
			bytes[0x00+2] = 0x98;
			bytes[0x01+2] = 0xc0;
			PIO(m2c->board,bytes,4);
			bytes[0] = 0x42;bytes[1] = 0x2;
			bytes[0x00+2] = 0xef;
			PIO(m2c->board,bytes,3);
		}
		else {
			bytes[0] = 0x42;bytes[1] = 0x8;
			bytes[0x00+2] = 0x98;
			bytes[0x01+2] = 0xc0;
			PIO(m2c->board,bytes,4);
			bytes[0] = 0x42;bytes[1] = 0x2;
			bytes[0x00+2] = 0xef;
			PIO(m2c->board,bytes,3);
		}
	}
	else {
		m2ddWrInt32(m2c,0xe000c8,m2ddRdScratch(m2c,0xe000c8)|0x01000000);
		m2ddWrInt32(m2c,0xe00098,m2ddRdInt32(m2c,0xe00098)|0x1000);
		m2ddWrInt32(m2c,0xe0009c,m2ddRdScratch(m2c,0xe0009c)&0xfffaffff);
		bytes[0] = 0x42;bytes[1] = 0x23;
		bytes[0x00+2] = 0x00;bytes[0x01+2] = 0x90;bytes[0x02+2] = 0x90;bytes[0x03+2] = 0x00;bytes[0x04+2] = 0x00;
		bytes[0x05+2] = 0x00;bytes[0x06+2] = 0x40;bytes[0x07+2] = 0x80;bytes[0x08+2] = 0x40;bytes[0x09+2] = 0x47;
		bytes[0x0a+2] = 0x00;bytes[0x0b+2] = 0x00;bytes[0x0c+2] = 0x00;
		PIO(m2c->board,bytes,15);
		if (pal) {
			bytes[0] = 0x42;bytes[1] = 0x8;
			bytes[0x00+2] = 0x98;
			bytes[0x01+2] = 0xc0;
			PIO(m2c->board,bytes,4);
			bytes[0] = 0x42;bytes[1] = 0x2;
			bytes[0x00+2] = 0xff;
			PIO(m2c->board,bytes,3);
		}
		else {
			bytes[0] = 0x42;bytes[1] = 0x8;
			bytes[0x00+2] = 0x98;
			bytes[0x01+2] = 0xc0;
			PIO(m2c->board,bytes,4);
			bytes[0] = 0x42;bytes[1] = 0x2;
			bytes[0x00+2] = 0xff;
			PIO(m2c->board,bytes,3);
		}
	}
}

void Sleep4u(unsigned char timein) 
{
	_int64 start, end;
	_int64 freq, time;
	static _int64 update_ticks_pc;
	static int sinit = 1;
	time=timein;
	//time*=8;

	if (sinit) {
		sinit = 0;
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		update_ticks_pc = freq / 1000000;
	}

	if (!QueryPerformanceCounter((LARGE_INTEGER*)&start)) {;
		Sleep(1);
		return;  // hardware doesn't support performance counter
	}
	QueryPerformanceCounter((LARGE_INTEGER*)&end);
    while ((end - start) <= (time * update_ticks_pc)){
		QueryPerformanceCounter((LARGE_INTEGER*)&end);
	}
}

int PIO(int board, unsigned char *bytes, int cnt) 
{
	int j,i,bit;
	unsigned long ack;
	m2ddWriteInt32(board,0xfc0a08,0x00003);
	m2ddWriteInt32(board,0xfc0a08,0x30001);
	Sleep4u(5);
	m2ddWriteInt32(board,0xfc0a08,0x30000);
	for (j=0;j<cnt;j++){
		for (i=0;i<8;i++) {
			bit = (bytes[j]&0x80)>>6;
			Sleep4u(2);
			m2ddWriteInt32(board,0xfc0a08,0x30000+bit);
			Sleep4u(3);
			m2ddWriteInt32(board,0xfc0a08,0x30001+bit);
			Sleep4u(5);
			m2ddWriteInt32(board,0xfc0a08,0x30000+bit);
			bytes[j] = bytes[j]<<1;
		}
		Sleep4u(2);
		m2ddWriteInt32(board,0xfc0a08,0x10000);
		Sleep4u(3);
		m2ddWriteInt32(board,0xfc0a08,0x10001);
		m2ddReadInt32(board,0xfc0a08,ack);
		Sleep4u(5);
		m2ddWriteInt32(board,0xfc0a08,0x10000);
	}
	Sleep4u(2);
	m2ddWriteInt32(board,0xfc0a08,0x30000);
	Sleep4u(3);
	m2ddWriteInt32(board,0xfc0a08,0x30001);
	Sleep4u(2);
	m2ddWriteInt32(board,0xfc0a08,0x30003);
	Sleep4u(3);
	m2ddWriteInt32(board,0xfc0a08,0x00003);
	Sleep4u(10);
	Sleep(1);
	return(ack);
}

int PIOCY(int board, unsigned char *bytes, int cnt) 
{
	int j,i,bit;
	unsigned long ack;
	m2ddWriteInt32(board,0xe00018,0x00003);
	m2ddWriteInt32(board,0xe00018,0x30002);
	Sleep4u(5);
	m2ddWriteInt32(board,0xe00018,0x30000);
	for (j=0;j<cnt;j++){
		for (i=0;i<8;i++) {
			bit = (bytes[j]&0x80)>>7;
			Sleep4u(2);
			m2ddWriteInt32(board,0xe00018,0x30000+bit);
			Sleep4u(3);
			m2ddWriteInt32(board,0xe00018,0x30002+bit);
			Sleep4u(5);
			m2ddWriteInt32(board,0xe00018,0x30000+bit);
			bytes[j] = bytes[j]<<1;
		}
		Sleep4u(2);
		m2ddWriteInt32(board,0xe00018,0x20001);
		Sleep4u(3);
		m2ddWriteInt32(board,0xe00018,0x20003);
		m2ddReadInt32(board,0xe00018,ack);
		Sleep4u(5);
		m2ddWriteInt32(board,0xe00018,0x20001);
	}
	Sleep4u(2);
	m2ddWriteInt32(board,0xe00018,0x30000);
	Sleep4u(3);
	m2ddWriteInt32(board,0xe00018,0x30002);
	Sleep4u(2);
	m2ddWriteInt32(board,0xe00018,0x30003);
	Sleep4u(3);
	m2ddWriteInt32(board,0xe00018,0x00003);
	Sleep4u(10);
	Sleep(1);
	return(ack);
}

void mercmd(int cmdi, ...)
{
	int *cmd,*cmda,i;
	cmdq* cq;
	cmd = &cmdi;
	cq = (cmdq *)cmd[1];
	cmda = (int *)cmd[3];
	if      (cmd[2] == PRIORITY)  for (i=0; i<((cmd[4] &0xff)+2); i++) cq->pq[cmd[0]][cq->pcnt[cmd[0]]++] = cmd[i+3];
	else if (cmd[2] == SCHEDULED) for (i=0; i<((cmd[4] &0xff)+2); i++) cq->sq[cmd[0]][cq->scnt[cmd[0]]++] = cmd[i+3];
	else if (cmd[2] == PRIARRAY)  for (i=0; i<((cmda[1]&0xff)+2); i++) cq->pq[cmd[0]][cq->pcnt[cmd[0]]++] = cmda[i];
	else if (cmd[2] == SCHARRAY)  for (i=0; i<((cmda[1]&0xff)+2); i++) cq->sq[cmd[0]][cq->scnt[cmd[0]]++] = cmda[i];
}

void sendqinit(cmdq* cq)
{
	cq->pcnt[0] = 0;cq->pcnt[1] = 0;cq->pcnt[2] = 0;cq->pcnt[3] = 0;
	cq->scnt[0] = 0;cq->scnt[1] = 0;cq->scnt[2] = 0;cq->scnt[3] = 0;
}

void sendq(int queue, cmdq* cq, M2CONTEXT* m2c,int cnt,int bufmask)
{
	if (queue == 0) {
		while (m2c->CommandReady1 != 0 )Sleep(1);
		memcpy(m2c->CommandQueue1,cq->pq[queue],cq->pcnt[queue]*4);
		memcpy(m2c->CommandQueue1+cq->pcnt[queue],cq->sq[queue],cq->scnt[queue]*4);
		m2c->CommandQueue1[cnt-2] = cq->pcnt[queue];
		m2c->CommandQueue1[cnt-1] = cq->scnt[queue]<<16;
		m2c->CommandReady1 = (bufmask<<1)|1;
		cq->pcnt[queue] = cq->scnt[queue] = 0;
	}
	else if (queue == 1) {
		while (m2c->CommandReady2 != 0 )Sleep(1);
		memcpy(m2c->CommandQueue2,cq->pq[queue],cq->pcnt[queue]*4);
		memcpy(m2c->CommandQueue2+cq->pcnt[queue],cq->sq[queue],cq->scnt[queue]*4);
		m2c->CommandQueue2[cnt-2] = cq->pcnt[queue];
		m2c->CommandQueue2[cnt-1] = cq->scnt[queue]<<16;
		m2c->CommandReady2 = (bufmask<<1)|1;
		cq->pcnt[queue] = cq->scnt[queue] = 0;
	}
	else if (queue == 2) {
		while (m2c->CommandReady3 != 0 )Sleep(1);
		memcpy(m2c->CommandQueue3,cq->pq[queue],cq->pcnt[queue]*4);
		memcpy(m2c->CommandQueue3+cq->pcnt[queue],cq->sq[queue],cq->scnt[queue]*4);
		m2c->CommandQueue3[cnt-2] = cq->pcnt[queue];
		m2c->CommandQueue3[cnt-1] = cq->scnt[queue]<<16;
		m2c->CommandReady3 = (bufmask<<1)|1;
		cq->pcnt[queue] = cq->scnt[queue] = 0;
	}
	else {
		while (m2c->CommandReady4 != 0 )Sleep(1);
		memcpy(m2c->CommandQueue4,cq->pq[queue],cq->pcnt[queue]*4);
		memcpy(m2c->CommandQueue4+cq->pcnt[queue],cq->sq[queue],cq->scnt[queue]*4);
		m2c->CommandQueue4[cnt-2] = cq->pcnt[queue];
		m2c->CommandQueue4[cnt-1] = cq->scnt[queue]<<16;
		m2c->CommandReady4 = (bufmask<<1)|1;
		cq->pcnt[queue] = cq->scnt[queue] = 0;
	}
}

void GetPwd(int board, int &pwd1, int &pwd2)
{
	int i;
	unsigned int address = 0;
	unsigned char addr1;
	unsigned char addr2;
	unsigned char wrbuf[PAGE_SIZE*2];

	m2ddWriteInt32(board,0xe00004,0x3ff);
	address = MAX_PAGES - 1;
	addr1 = (unsigned char)(address >> 8)&3;
	addr2 = (unsigned char)(address     )&0xff;

	for (i = 0; i<256; i++) wrbuf[i] = 0;
	ReadPage(board,addr1,addr2,&wrbuf[0]);
	m2ddWriteInt32(board,0xe00004,0x3ff);

	pwd1 = wrbuf[248];
	pwd1 = ((pwd1<<8)&0xffffff00) | wrbuf[249];
	pwd1 = ((pwd1<<8)&0xffffff00) | wrbuf[250];
	pwd1 = ((pwd1<<8)&0xffffff00) | wrbuf[251];
	pwd2 = wrbuf[252];
	pwd2 = ((pwd2<<8)&0xffffff00) | wrbuf[253];
	pwd2 = ((pwd2<<8)&0xffffff00) | wrbuf[254];
	pwd2 = ((pwd2<<8)&0xffffff00) | wrbuf[255];
}

int WritePage(int board, unsigned char addr1,unsigned char addr2, unsigned char *bufptr) 
{
	int i;
	unsigned char addr0;
	unsigned char bytes[4096];
	int test_ack;
	int cd = 1;

	addr0 = AT17C002 + WRITE; // send device address byte with write bit
        
	bytes[0] = addr0;             // 1nd address byte
	bytes[1] = addr1;             // 2nd address byte
	bytes[2] = addr2;             // 3rd address byte
	bytes[3] = 0;                 // 4rd address byte
 
	for (i = 0; i < PAGE_SIZE; i++){
		bytes[i+4]=0;
		if (bufptr[i]&0x01) bytes[i+4] |= 0x80;
		if (bufptr[i]&0x02) bytes[i+4] |= 0x40;
		if (bufptr[i]&0x04) bytes[i+4] |= 0x20;
		if (bufptr[i]&0x08) bytes[i+4] |= 0x10;
		if (bufptr[i]&0x10) bytes[i+4] |= 0x08;
		if (bufptr[i]&0x20) bytes[i+4] |= 0x04;
		if (bufptr[i]&0x40) bytes[i+4] |= 0x02;
		if (bufptr[i]&0x80) bytes[i+4] |= 0x01;
	}
	EEPIO(board,bytes,PAGE_SIZE+4);

	i = 0;
	test_ack = 1;
    while (test_ack) {
		if (i++ == 0x800) {
			printf("write timeout\n");
			return 0;
		}
	    bytes[0] = addr0;
        test_ack = EEPIO(board,bytes,1)&cd;
    }
	return 1;
}

int VerifyPage(int board, unsigned char addr1,unsigned char addr2, unsigned char *bufptr) 
{
	int i;
	unsigned char addr0;
	unsigned char data,rdata;
	unsigned char bytes[16];

	addr0 = AT17C002 + WRITE; // send device address byte with write bit
        
	bytes[0] = addr0;             // 1nd address byte
	bytes[1] = addr1;             // 2nd address byte
	bytes[2] = addr2;             // 3rd address byte
	bytes[3] = 0;                 // 4rd address byte
	bytes[4] = AT17C002 + READ;
	EEPIO_RD1(board,bytes);

	for (i = 0; i < (PAGE_SIZE); i++){
		data=0;
		if (bufptr[i]&0x01) data |= 0x80;
		if (bufptr[i]&0x02) data |= 0x40;
		if (bufptr[i]&0x04) data |= 0x20;
		if (bufptr[i]&0x08) data |= 0x10;
		if (bufptr[i]&0x10) data |= 0x08;
		if (bufptr[i]&0x20) data |= 0x04;
		if (bufptr[i]&0x40) data |= 0x02;
		if (bufptr[i]&0x80) data |= 0x01;
	    rdata=EEPIO_RD2(board);
	    if (rdata != data)
		{
			EEPIO_RD3(board);
			return 0;
		}
	}
	EEPIO_RD3(board);
	return 1;
}

int ReadPage(int board, unsigned char addr1,unsigned char addr2, unsigned char *bufptr) 
{
	int i;
	unsigned char addr0;
	unsigned char data;
	unsigned char bytes[16];

	addr0 = AT17C002 + WRITE; // send device address byte with write bit
        
	bytes[0] = addr0;             // 1nd address byte
	bytes[1] = addr1;             // 2nd address byte
	bytes[2] = addr2;             // 3rd address byte
	bytes[3] = 0;                 // 4rd address byte
	bytes[4] = AT17C002 + READ;
	EEPIO_RD1(board,bytes);

	for (i = 0; i < (PAGE_SIZE); i++){
		bufptr[i] = 0;
	    data=EEPIO_RD2(board);
		if (data&0x01) bufptr[i] |= 0x80;
		if (data&0x02) bufptr[i] |= 0x40;
		if (data&0x04) bufptr[i] |= 0x20;
		if (data&0x08) bufptr[i] |= 0x10;
		if (data&0x10) bufptr[i] |= 0x08;
		if (data&0x20) bufptr[i] |= 0x04;
		if (data&0x40) bufptr[i] |= 0x02;
		if (data&0x80) bufptr[i] |= 0x01;
	}
	EEPIO_RD3(board);
	return 1;
}

int EEPIO(int board, unsigned char *bytes, int cnt) 
{
	int j,i,bit;
	unsigned long ack;
	int cd = 1;
	int cc = 2;

	m2ddWriteInt32(board,0xe00004,0x3f4+cc);
	Sleep4u(5);
	m2ddWriteInt32(board,0xe00004,0x3f4);
	for (j=0;j<cnt;j++){
		for (i=0;i<8;i++) {
			bit = (bytes[j]&0x80)>>7;
			Sleep4u(2);
	        m2ddWriteInt32(board,0xe00004,0x3f4+(bit*cd));
			Sleep4u(3);
	        m2ddWriteInt32(board,0xe00004,0x3f4+cc+(bit*cd));
			Sleep4u(5);
	        m2ddWriteInt32(board,0xe00004,0x3f4+(bit*cd));
			bytes[j] = bytes[j]<<1;
		}
		Sleep4u(2);
		m2ddWriteInt32(board,0xe00004,0x3f4+cd);
		Sleep4u(3);
		m2ddWriteInt32(board,0xe00004,0x3f4+cc+cd);
		m2ddReadInt32(board,0xe00004,ack);
		Sleep4u(5);
		m2ddWriteInt32(board,0xe00004,0x3f4+cd);
	}
	Sleep4u(2);
	m2ddWriteInt32(board,0xe00004,0x3f4);
	Sleep4u(3);
	m2ddWriteInt32(board,0xe00004,0x3f4+cc);
	Sleep4u(2);
	m2ddWriteInt32(board,0xe00004,0x3f4+cc+cd);
	Sleep4u(10);
	Sleep(1);
	return(ack);
}

int EEPIO_RD1(int board, unsigned char *bytes) 
{
	int j,i,bit;
	unsigned long ack;
	int cd = 1;
	int cc = 2;

	m2ddWriteInt32(board,0xe00004,0x3f4+cc);
	Sleep4u(5);
	m2ddWriteInt32(board,0xe00004,0x3f4);
	for (j=0;j<4;j++){
		for (i=0;i<8;i++) {
			bit = (bytes[j]&0x80)>>7;
			Sleep4u(2);
	        m2ddWriteInt32(board,0xe00004,0x3f4+(bit*cd));
			Sleep4u(3);
	        m2ddWriteInt32(board,0xe00004,0x3f4+cc+(bit*cd));
			Sleep4u(5);
	        m2ddWriteInt32(board,0xe00004,0x3f4+(bit*cd));
			bytes[j] = bytes[j]<<1;
		}
		Sleep4u(2);
		m2ddWriteInt32(board,0xe00004,0x3f4+cd);
		Sleep4u(3);
		m2ddWriteInt32(board,0xe00004,0x3f4+cc+cd);
		m2ddReadInt32(board,0xe00004,ack);
		Sleep4u(5);
		m2ddWriteInt32(board,0xe00004,0x3f4+cd);
	}

	Sleep4u(5);
	m2ddWriteInt32(board,0xe00004,0x3f4+cc+cd);
	Sleep4u(5);
	m2ddWriteInt32(board,0xe00004,0x3f4+cc);
	Sleep4u(5);
	m2ddWriteInt32(board,0xe00004,0x3f4);

	
	j=4;
	for (i=0;i<8;i++) {
		bit = (bytes[j]&0x80)>>7;
		Sleep4u(2);
	    m2ddWriteInt32(board,0xe00004,0x3f4+(bit*cd));
		Sleep4u(3);
	    m2ddWriteInt32(board,0xe00004,0x3f4+cc+(bit*cd));
		Sleep4u(5);
	    m2ddWriteInt32(board,0xe00004,0x3f4+(bit*cd));
		bytes[j] = bytes[j]<<1;
	}
	Sleep4u(2);
	m2ddWriteInt32(board,0xe00004,0x3f4+cd);
	Sleep4u(3);
	m2ddWriteInt32(board,0xe00004,0x3f4+cc+cd);
	m2ddReadInt32(board,0xe00004,ack);
	Sleep4u(5);
	m2ddWriteInt32(board,0xe00004,0x3f4+cd);
	Sleep4u(2);
	return(ack);
}

int EEPIO_RD2(int board) 
{
	int i;
	unsigned long ack;
	unsigned char data;
	int cd = 1;
	int cc = 2;

	m2ddWriteInt32(board,0xe00004,0x3f4);
	for (i=0;i<8;i++) {
		Sleep4u(2);
		m2ddWriteInt32(board,0xe00004,0x3f4+cd);
		Sleep4u(3);
		m2ddReadInt32(board,0xe00004,ack);
		if ((ack&cd)!=0)
			data = ((data<<1)&0xfe)+1;
		else 
			data = ((data<<1)&0xfe);
		Sleep4u(5);
		m2ddWriteInt32(board,0xe00004,0x3f4+cc+cd);
	}
	Sleep4u(2);
	m2ddWriteInt32(board,0xe00004,0x3f4+cd);
	Sleep4u(1);
	m2ddWriteInt32(board,0xe00004,0x3f4);
	Sleep4u(2);
	m2ddWriteInt32(board,0xe00004,0x3f4+cc);
	Sleep4u(5);
	return(data);
}

void EEPIO_RD3(int board) 
{
	int cd = 1;
	int cc = 2;

	m2ddWriteInt32(board,0xe00004,0x3f4+cc+cd);
	Sleep4u(10);
	Sleep(20);
	return;
}

int I2C_WR(int board, int device, unsigned char *bytes, int cnt, int mask) 
{
	int j,i,bit;
	unsigned long ack;
	int root = 0xfffffffc&mask;
	int cd = 1;
	int cc = 2;
	if (device==2) {
		root = 0xfffffff3&mask;
		cd = 4;
		cc = 8;
	}
	else if (device==3) {
		root = 0xffffffcf&mask;
		cd = 0x10;
		cc = 0x20;
	}
	else if (device==4) {
		root = 0xffffff3f&mask;
		cd = 0x40;
		cc = 0x80;
	}
	else if (device==5) {
		root = 0xfffffcff&mask;
		cd = 0x100;
		cc = 0x200;
	}
	else if (device==6) {
		root = 0xffffcfff&mask;
		cd = 0x1000;
		cc = 0x2000;
	}

	m2ddWriteInt32(board,0xe000e0,root+cc);
	Sleep4u(5);
	m2ddWriteInt32(board,0xe000e0,root);
	for (j=0;j<cnt;j++){
		for (i=0;i<8;i++) {
			bit = (bytes[j]&0x80)>>7;
			Sleep4u(2);
	        m2ddWriteInt32(board,0xe000e0,root+(bit*cd));
			Sleep4u(3);
	        m2ddWriteInt32(board,0xe000e0,root+cc+(bit*cd));
			Sleep4u(5);
	        m2ddWriteInt32(board,0xe000e0,root+(bit*cd));
			bytes[j] = bytes[j]<<1;
		}
		Sleep4u(2);
		m2ddWriteInt32(board,0xe000e0,root+cd);
		Sleep4u(3);
		m2ddWriteInt32(board,0xe000e0,root+cc+cd);
		m2ddReadInt32(board,0xe000e0,ack);
		Sleep4u(5);
		m2ddWriteInt32(board,0xe000e0,root+cd);
	}
	Sleep4u(2);
	m2ddWriteInt32(board,0xe000e0,root);
	Sleep4u(3);
	m2ddWriteInt32(board,0xe000e0,root+cc);
	Sleep4u(2);
	m2ddWriteInt32(board,0xe000e0,root+cc+cd);
	Sleep4u(10);
	Sleep(1);
	return(ack);
}

void DongleWriteByte(int board, int root, int cd, unsigned char mdat) 
{
	for (int i=0; i<8; i++) {
		if (mdat&1) {
			m2ddWriteInt32(board,0xe000e0,root+cd*0);
			Sleep4u(6);
			m2ddWriteInt32(board,0xe000e0,root+cd*1);
			Sleep4u(64);
		}
		else {
			m2ddWriteInt32(board,0xe000e0,root+cd*0);
			Sleep4u(60);
			m2ddWriteInt32(board,0xe000e0,root+cd*1);
			Sleep4u(10);
		}
		mdat = mdat>>1;
	}
}

void DongleReadByte(int board, int root, int cd, unsigned char *mdat, unsigned char *crc) 
{
	unsigned long ack;

	for (int i=0; i<8; i++) {
		m2ddWriteInt32(board,0xe000e0,root+cd*0);
		Sleep4u(6);
		m2ddWriteInt32(board,0xe000e0,root+cd*1);
		Sleep4u(9);
		m2ddReadInt32(board,0xe000e0,ack);
		if ((ack&cd)!=0) {
			*mdat = ((*mdat>>1)&0x7f)+0x80;
			if (*crc&1) *crc =  (*crc>>1)&0x7f;
			else        *crc = ((*crc>>1)&0x7f)^0x8c;
		}
		else {
			*mdat = ((*mdat>>1)&0x7f);
			if (*crc&1) *crc = ((*crc>>1)&0x7f)^0x8c;
			else        *crc =  (*crc>>1)&0x7f;
		}
		Sleep4u(55);
	}
}

#define SHA1CircularShift(bits,word) \
                ((((word) << (bits)) & 0xFFFFFFFF) | \
                ((word) >> (32-(bits))))

void SHA1(unsigned char *m)
{
    const unsigned K[] =            /* Constants defined in SHA-1   */      
    {
        0x5A827999,
        0x6ED9EBA1,
        0x8F1BBCDC,
        0xCA62C1D6
    };
    int         t;                  /* Loop counter                 */
    unsigned    temp;               /* Temporary word value         */
    unsigned    W[80];              /* Word sequence                */
    unsigned    A, B, C, D, E;      /* Word buffers                 */

    /*
     *  Initialize the first 16 words in the array W
     */
    for(t = 0; t < 16; t++)
    {
        W[t]  = ((unsigned) m[t * 4]) << 24;
        W[t] |= ((unsigned) m[t * 4 + 1]) << 16;
        W[t] |= ((unsigned) m[t * 4 + 2]) << 8;
        W[t] |= ((unsigned) m[t * 4 + 3]);
    }

    for(t = 16; t < 80; t++)
    {
       W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = 0x67452301;
    B = 0xEFCDAB89;
    C = 0x98BADCFE;
    D = 0x10325476;
    E = 0xC3D2E1F0;

    for(t = 0; t < 20; t++)
    {
        temp =  SHA1CircularShift(5,A) +
                ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 20; t < 40; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 40; t < 60; t++)
    {
        temp = SHA1CircularShift(5,A) +
               ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 60; t < 80; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

	memcpy(&m[0] ,&E,4);
	memcpy(&m[4] ,&D,4);
	memcpy(&m[8] ,&C,4);
	memcpy(&m[12],&B,4);
	memcpy(&m[16],&A,4);
}

int DongleReadPage(int board, unsigned char *bytes, int mask) 
{
	unsigned long ack;
	int root = 0xfffffbff&mask;
	int cd = 0x400;
	unsigned char mdat[64];
	unsigned char crc = 0;

	DongleReadSerial(board, mdat, mask);
	DongleWriteByte(board,root,cd,0xf0);//Read Mem
	DongleWriteByte(board,root,cd,0);//TA1
	DongleWriteByte(board,root,cd,0);//TA2
	for (int i=0; i<8; i++) {
		DongleReadByte(board,root,cd,&bytes[i],&crc);
	}
	return 0;
}

int DongleWritePage(int board, unsigned char *bytes, int mask) 
{
	unsigned long ack;
	int root = 0xfffffbff&mask;
	int cd = 0x400;
	unsigned char mdat[64];
	unsigned char mem[256];
	unsigned char crc = 0;

	DongleReadSerial(board, mdat, mask);
	DongleWriteByte(board,root,cd,0xf);//Write Scratchpad
	DongleWriteByte(board,root,cd,0x80);//TA1
	DongleWriteByte(board,root,cd,0);//TA2
	DongleWriteByte(board,root,cd,0);//1
	DongleWriteByte(board,root,cd,0);//2
	DongleWriteByte(board,root,cd,0);//3
	DongleWriteByte(board,root,cd,0);//4
	DongleWriteByte(board,root,cd,0);//5
	DongleWriteByte(board,root,cd,0);//6
	DongleWriteByte(board,root,cd,0);//7
	DongleWriteByte(board,root,cd,0);//8
	DongleReadByte(board,root,cd,&mem[0],&crc);

	DongleReadSerial(board, mdat, mask);
	DongleWriteByte(board,root,cd,0xaa);//Read Scratchpad
	DongleReadByte(board,root,cd,&mem[0],&crc);//TA1
	DongleReadByte(board,root,cd,&mem[1],&crc);//TA2
	DongleReadByte(board,root,cd,&mem[2],&crc);//ES
	DongleReadByte(board,root,cd,&mem[3],&crc);//1
	DongleReadByte(board,root,cd,&mem[4],&crc);//2
	DongleReadByte(board,root,cd,&mem[5],&crc);//3
	DongleReadByte(board,root,cd,&mem[6],&crc);//4
	DongleReadByte(board,root,cd,&mem[7],&crc);//5
	DongleReadByte(board,root,cd,&mem[8],&crc);//6
	DongleReadByte(board,root,cd,&mem[9],&crc);//7
	DongleReadByte(board,root,cd,&mem[10],&crc);//8
	DongleReadByte(board,root,cd,&mem[11],&crc);//CRC1
	DongleReadByte(board,root,cd,&mem[12],&crc);//CRC2
	DongleReadByte(board,root,cd,&mem[13],&crc);//0xff

	DongleReadSerial(board, mdat, mask);
	DongleWriteByte(board,root,cd,0x5a);//Load Secret
	DongleWriteByte(board,root,cd,mem[0]);//TA1
	DongleWriteByte(board,root,cd,mem[1]);//TA2
	DongleWriteByte(board,root,cd,mem[2]);//ES
	for (int i=0; i<40; i++) Sleep4u(250);
	DongleReadByte(board,root,cd,&mem[3],&crc);//0xaa
	DongleReadByte(board,root,cd,&mem[4],&crc);//0xaa

	DongleReadSerial(board, mdat, mask);
	DongleWriteByte(board,root,cd,0xf);//Write Scratchpad
	DongleWriteByte(board,root,cd,0);//TA1
	DongleWriteByte(board,root,cd,0);//TA2
	DongleWriteByte(board,root,cd,0);//1
	DongleWriteByte(board,root,cd,0);//2
	DongleWriteByte(board,root,cd,0);//3
	DongleWriteByte(board,root,cd,0);//4
	DongleWriteByte(board,root,cd,0);//5
	DongleWriteByte(board,root,cd,0);//6
	DongleWriteByte(board,root,cd,0);//7
	DongleWriteByte(board,root,cd,0);//8
	DongleReadByte(board,root,cd,&mem[0],&crc);

	DongleReadSerial(board, mdat, mask);
	DongleWriteByte(board,root,cd,0xaa);//Read Scratchpad
	DongleReadByte(board,root,cd,&mem[0],&crc);//TA1
	DongleReadByte(board,root,cd,&mem[1],&crc);//TA2
	DongleReadByte(board,root,cd,&mem[2],&crc);//ES
	DongleReadByte(board,root,cd,&mem[3],&crc);//1
	DongleReadByte(board,root,cd,&mem[4],&crc);//2
	DongleReadByte(board,root,cd,&mem[5],&crc);//3
	DongleReadByte(board,root,cd,&mem[6],&crc);//4
	DongleReadByte(board,root,cd,&mem[7],&crc);//5
	DongleReadByte(board,root,cd,&mem[8],&crc);//6
	DongleReadByte(board,root,cd,&mem[9],&crc);//7
	DongleReadByte(board,root,cd,&mem[10],&crc);//8
	DongleReadByte(board,root,cd,&mem[11],&crc);//CRC1
	DongleReadByte(board,root,cd,&mem[12],&crc);//CRC2
	DongleReadByte(board,root,cd,&mem[13],&crc);//0xff

	DongleReadSerial(board, mdat, mask);
	DongleWriteByte(board,root,cd,0xf0);//Read Mem
	DongleWriteByte(board,root,cd,0);//TA1
	DongleWriteByte(board,root,cd,0);//TA2
	for (int i=0; i<0xa0; i++) {
		DongleReadByte(board,root,cd,&mem[i],&crc);//1
	}

	unsigned char m[64];
	for (int i=0; i<28; i++) m[i+4] = mem[i];
	m[ 0]=0x00;      m[ 1]=0x00;      m[ 2]=0x00;      m[ 3]=0x00;
	m[32]=bytes[0];  m[33]=bytes[1];  m[34]=bytes[2];  m[35]=bytes[3];
	m[36]=bytes[4];  m[37]=bytes[5];  m[38]=bytes[6];  m[39]=bytes[7];
	m[40]=0x00;      m[41]=mem[0x90]; m[42]=mem[0x91]; m[43]=mem[0x92];
	m[44]=mem[0x93]; m[45]=mem[0x94]; m[46]=mem[0x95]; m[47]=mem[0x96];
	m[48]=0x00;      m[49]=0x00;      m[50]=0x00;      m[51]=0x00;
	m[52]=0xff;      m[53]=0xff;      m[54]=0xff;      m[55]=0x80;
	m[56]=0x00;      m[57]=0x00;      m[58]=0x00;      m[59]=0x00;
	m[60]=0x00;      m[61]=0x00;      m[62]=0x01;      m[63]=0xb8;

    SHA1(m);

	DongleReadSerial(board, mdat, mask);
	DongleWriteByte(board,root,cd,0xf);//Write Scratchpad
	DongleWriteByte(board,root,cd,0);//TA1
	DongleWriteByte(board,root,cd,0);//TA2
	DongleWriteByte(board,root,cd,bytes[0]);//1
	DongleWriteByte(board,root,cd,bytes[1]);//2
	DongleWriteByte(board,root,cd,bytes[2]);//3
	DongleWriteByte(board,root,cd,bytes[3]);//4
	DongleWriteByte(board,root,cd,bytes[4]);//5
	DongleWriteByte(board,root,cd,bytes[5]);//6
	DongleWriteByte(board,root,cd,bytes[6]);//7
	DongleWriteByte(board,root,cd,bytes[7]);//8
	DongleReadByte(board,root,cd,&mem[0],&crc);

	DongleReadSerial(board, mdat, mask);
	DongleWriteByte(board,root,cd,0xaa);//Read Scratchpad
	DongleReadByte(board,root,cd,&mem[0],&crc);//TA1
	DongleReadByte(board,root,cd,&mem[1],&crc);//TA2
	DongleReadByte(board,root,cd,&mem[2],&crc);//ES
	DongleReadByte(board,root,cd,&mem[3],&crc);//1
	DongleReadByte(board,root,cd,&mem[4],&crc);//2
	DongleReadByte(board,root,cd,&mem[5],&crc);//3
	DongleReadByte(board,root,cd,&mem[6],&crc);//4
	DongleReadByte(board,root,cd,&mem[7],&crc);//5
	DongleReadByte(board,root,cd,&mem[8],&crc);//6
	DongleReadByte(board,root,cd,&mem[9],&crc);//7
	DongleReadByte(board,root,cd,&mem[10],&crc);//8
	DongleReadByte(board,root,cd,&mem[11],&crc);//CRC1
	DongleReadByte(board,root,cd,&mem[12],&crc);//CRC2
	DongleReadByte(board,root,cd,&mem[13],&crc);//0xff

	DongleReadSerial(board, mdat, mask);
	DongleWriteByte(board,root,cd,0x55);//Copy Scratch
	DongleWriteByte(board,root,cd,0);//TA1
	DongleWriteByte(board,root,cd,0);//TA2
	DongleWriteByte(board,root,cd,mem[2]);//ES
	for (int i=0; i<10; i++) Sleep4u(250);
	for (int i=0; i<20; i++) DongleWriteByte(board,root,cd,m[i]);
	for (int i=0; i<40; i++) Sleep4u(250);
	DongleReadByte(board,root,cd,&mem[0],&crc);
	if (mem[0]==0xaa) return 0;
	else              return 1;
}

int DongleReadSerial(int board, unsigned char *bytes, int mask) 
{
	unsigned long ack;
	int root = 0xfffffbff&mask;
	int cd = 0x400;
	unsigned char crc = 0;
	unsigned char crcskip = 0;

	m2ddWriteInt32(board,0xe000e0,root+cd*1);
	Sleep(1);

	m2ddWriteInt32(board,0xe000e0,root+cd*0);
	Sleep4u(250); Sleep4u(250);
	m2ddWriteInt32(board,0xe000e0,root+cd*1);
	Sleep4u(64);
	m2ddReadInt32(board,0xe000e0,ack);
	Sleep4u(250); Sleep4u(250);
	if ((ack&cd)!=0) return 1;

	DongleWriteByte(board,root,cd,0x33);
	DongleReadByte(board,root,cd,&bytes[0],&crc);
	DongleReadByte(board,root,cd,&bytes[1],&crc);
	DongleReadByte(board,root,cd,&bytes[2],&crc);
	DongleReadByte(board,root,cd,&bytes[3],&crc);
	DongleReadByte(board,root,cd,&bytes[4],&crc);
	DongleReadByte(board,root,cd,&bytes[5],&crc);
	DongleReadByte(board,root,cd,&bytes[6],&crc);
	DongleReadByte(board,root,cd,&bytes[7],&crcskip);

	if (crc!=bytes[7]) return 2;
	return 0;
}

int I2C_RD1(int board, int device, unsigned char *bytes, int mask) 
{
	int j,i,bit;
	unsigned long ack;
	int root = 0xfffffffc&mask;
	int cd = 1;
	int cc = 2;
	if (device==2) {
		root = 0xfffffff3&mask;
		cd = 4;
		cc = 8;
	}
	else if (device==3) {
		root = 0xffffffcf&mask;
		cd = 0x10;
		cc = 0x20;
	}
	else if (device==4) {
		root = 0xffffff3f&mask;
		cd = 0x40;
		cc = 0x80;
	}
	else if (device==5) {
		root = 0xfffffcff&mask;
		cd = 0x100;
		cc = 0x200;
	}
	else if (device==6) {
		root = 0xffffcfff&mask;
		cd = 0x1000;
		cc = 0x2000;
	}

	m2ddWriteInt32(board,0xe000e0,root+cc);
	Sleep4u(5);
	m2ddWriteInt32(board,0xe000e0,root);
	for (j=0;j<2;j++){
		for (i=0;i<8;i++) {
			bit = (bytes[j]&0x80)>>7;
			Sleep4u(2);
	        m2ddWriteInt32(board,0xe000e0,root+(bit*cd));
			Sleep4u(3);
	        m2ddWriteInt32(board,0xe000e0,root+cc+(bit*cd));
			Sleep4u(5);
	        m2ddWriteInt32(board,0xe000e0,root+(bit*cd));
			bytes[j] = bytes[j]<<1;
		}
		Sleep4u(2);
		m2ddWriteInt32(board,0xe000e0,root+cd);
		Sleep4u(3);
		m2ddWriteInt32(board,0xe000e0,root+cc+cd);
		m2ddReadInt32(board,0xe000e0,ack);
		Sleep4u(5);
		m2ddWriteInt32(board,0xe000e0,root+cd);
	}

	Sleep4u(5);
	m2ddWriteInt32(board,0xe000e0,root+cc+cd);
	Sleep4u(5);
	m2ddWriteInt32(board,0xe000e0,root+cc);
	Sleep4u(5);
	m2ddWriteInt32(board,0xe000e0,root);

	
	j=2;
	for (i=0;i<8;i++) {
		bit = (bytes[j]&0x80)>>7;
		Sleep4u(2);
	    m2ddWriteInt32(board,0xe000e0,root+(bit*cd));
		Sleep4u(3);
	    m2ddWriteInt32(board,0xe000e0,root+cc+(bit*cd));
		Sleep4u(5);
	    m2ddWriteInt32(board,0xe000e0,root+(bit*cd));
		bytes[j] = bytes[j]<<1;
	}
	Sleep4u(2);
	m2ddWriteInt32(board,0xe000e0,root+cd);
	Sleep4u(3);
	m2ddWriteInt32(board,0xe000e0,root+cc+cd);
	m2ddReadInt32(board,0xe000e0,ack);
	Sleep4u(5);
	m2ddWriteInt32(board,0xe000e0,root+cd);
	Sleep4u(2);
	return(ack);
}

int I2C_RD2(int board, int device, int nack, int mask) 
{
	int i;
	unsigned long ack;
	unsigned char data;
	int root = 0xfffffffc&mask;
	int cd = 1;
	int cc = 2;
	if (device==2) {
		root = 0xfffffff3&mask;
		cd = 4;
		cc = 8;
	}
	else if (device==3) {
		root = 0xffffffcf&mask;
		cd = 0x10;
		cc = 0x20;
	}
	else if (device==4) {
		root = 0xffffff3f&mask;
		cd = 0x40;
		cc = 0x80;
	}
	else if (device==5) {
		root = 0xfffffcff&mask;
		cd = 0x100;
		cc = 0x200;
	}
	else if (device==6) {
		root = 0xffffcfff&mask;
		cd = 0x1000;
		cc = 0x2000;
	}

	for (i=0;i<8;i++) {
		m2ddWriteInt32(board,0xe000e0,root+cd);
//		Sleep4u(3);
		Sleep4u(200);
		m2ddReadInt32(board,0xe000e0,ack);
		if ((ack&cd)!=0)
			data = ((data<<1)&0xfe)+1;
		else 
			data = ((data<<1)&0xfe);
		m2ddWriteInt32(board,0xe000e0,root+cc+cd);
		Sleep4u(5);
		m2ddWriteInt32(board,0xe000e0,root+cd);
		Sleep4u(2);
	}
	if (nack) {
		Sleep4u(2);
		m2ddWriteInt32(board,0xe000e0,root+cd);
		Sleep4u(3);
		m2ddWriteInt32(board,0xe000e0,root+cc+cd);
		Sleep4u(5);
		m2ddWriteInt32(board,0xe000e0,root+cd);
		Sleep4u(2);
	}
	else {
		Sleep4u(2);
		m2ddWriteInt32(board,0xe000e0,root);
		Sleep4u(3);
		m2ddWriteInt32(board,0xe000e0,root+cc);
		Sleep4u(5);
		m2ddWriteInt32(board,0xe000e0,root);
		Sleep4u(2);
	}
	return(data);
}

void I2C_RD3(int board, int device, int mask) 
{
	int root = 0xfffffffc&mask;
	int cd = 1;
	int cc = 2;
	if (device==2) {
		root = 0xfffffff3&mask;
		cd = 4;
		cc = 8;
	}
	else if (device==3) {
		root = 0xffffffcf&mask;
		cd = 0x10;
		cc = 0x20;
	}
	else if (device==4) {
		root = 0xffffff3f&mask;
		cd = 0x40;
		cc = 0x80;
	}
	else if (device==5) {
		root = 0xfffffcff&mask;
		cd = 0x100;
		cc = 0x200;
	}
	else if (device==6) {
		root = 0xffffcfff&mask;
		cd = 0x1000;
		cc = 0x2000;
	}

	m2ddWriteInt32(board,0xe000e0,root);
	Sleep4u(3);
	m2ddWriteInt32(board,0xe000e0,root+cd);
	Sleep4u(2);
	m2ddWriteInt32(board,0xe000e0,root+cc+cd);
	Sleep(20);
	return;
}

int I2C_RDP(int board, int device, unsigned char *bytes, int cnt, int start, int mask)
{
	int i;
	unsigned char data;

	bytes[0] = 0xaa;
	bytes[1] = start;
	bytes[2] = 0xab;
	if ((device==2)||(device==4)) {
		bytes[0] = 0xd2;
		bytes[1] = start;
		bytes[2] = 0xd3;
	}
	else if (device==5) {
		bytes[0] = 0xa0;
		bytes[1] = start;
		bytes[2] = 0xa1;
	}
	else if (device==6) {
		bytes[0] = 0x22;
		bytes[1] = start;
		bytes[2] = 0x23;
	}

	I2C_RD1(board,device,bytes,mask);

	for (i = 0; i < cnt; i++){
		if (i==(cnt-1)) bytes[i]=I2C_RD2(board,device,1,mask);
		else            bytes[i]=I2C_RD2(board,device,0,mask);
	}

	I2C_RD3(board,device,mask);
	return 1;
}

int Set_EPROM_Load_Address(int board, int address)
{
	unsigned char buf[32], data;

	if (address&1) data=0x80;
	else 		   data=0;
	for (int i=0;i<100;i++) {
		buf[0]=0xa0;
		buf[1]=0xf8;
		buf[2]=data;
		I2C_WR(board,5,buf,3);
		I2C_RDP(board,5,buf,1,0xf8);
		if (data==buf[0]) break;
	}

	if (address&2) data=0x80;
	else 		   data=0;
	for (int i=0;i<100;i++) {
		buf[0]=0xa0;
		buf[1]=0xf9;
		buf[2]=data;
		I2C_WR(board,5,buf,3);
		I2C_RDP(board,5,buf,1,0xf9);
		if (data==buf[0]) break;
	}

	if (address&4) data=0x80;
	else 		   data=0;
	for (int i=0;i<100;i++) {
		buf[0]=0xa0;
		buf[1]=0xfa;
		buf[2]=data;
		I2C_WR(board,5,buf,3);
		I2C_RDP(board,5,buf,1,0xfa);
		if (data==buf[0]) break;
	}

	return 0;
}

int Get_EPROM_Load_Address(int board)
{
	unsigned char buf[32];
	int address=0;

	I2C_RDP(board,5,buf,1,0xf8);
	if (buf[0]) address |= 1;

	I2C_RDP(board,5,buf,1,0xf9);
	if (buf[0]) address |= 2;

	I2C_RDP(board,5,buf,1,0xfa);
	if (buf[0]) address |= 4;

	return address;
}

int Set_GTP_Loop_filter(int board, int loop_filter_en)
{
	unsigned long reg;
	int address;

	for (int port=1; port<=4; port++) {
		if      (port==1) address = 0x480000;
		else if (port==2) address = 0x070000;
		else if (port==3) address = 0xc80000;
		else              address = 0x870000;
	
		m2ddWriteInt32(board,0xe00038,address);
		Sleep(10);
		m2ddReadInt32(board,0xe00038,reg);
		if ((port==3)||(port==4)) reg = reg>>16;
		reg &= 0xffff;
		if ((port==1)||(port==3)) {//addr 48
			if (loop_filter_en) {
				reg |= 0x00001010;//bit 12,4 (6,4) on
			}
			else {
				reg &= 0xffffefef;//bit 12,4 (6,4) off
			}
		}
		else {
			if (loop_filter_en) {//addr 7
				reg |= 0x00000808;//bit 11,3 (6,4) on
			}
			else {
				reg &= 0xfffff7f7;//bit 11,3 (6,4) off
			}
		}
	
		m2ddWriteInt32(board,0xe00038,0x1000000|address|reg);
		Sleep(10);
	
		if      (port==1) address = 0x490000;
		else if (port==2) address = 0x060000;
		else if (port==3) address = 0xc90000;
		else              address = 0x860000;
	
		m2ddWriteInt32(board,0xe00038,address);
		Sleep(10);
		m2ddReadInt32(board,0xe00038,reg);
		if ((port==3)||(port==4)) reg = reg>>16;
		reg &= 0xffff;
		if ((port==1)||(port==3)) {//addr 49
			if (loop_filter_en) {
				reg |= 0x00000400;//bit 10 (0) on
			}
			else {
				reg &= 0xfffffbff;//bit 10 (0) off
			}
		}
		else {
			if (loop_filter_en) {//addr 6
				reg |= 0x00000020;//bit 5 (0) on
			}
			else {
				reg &= 0xffffffdf;//bit 5 (0) off
			}
		}
	
		m2ddWriteInt32(board,0xe00038,0x1000000|address|reg);
		Sleep(10);
	}

/*4 in code
	unsigned long reg;
	int address;
	if      (port==1) address = 0x480000;
	else if (port==2) address = 0x070000;
	else if (port==3) address = 0xc80000;
	else              address = 0x870000;

	m2ddWriteInt32(board,0xe00038,address);
	Sleep(10);
	m2ddReadInt32(board,0xe00038,reg);
	if ((port==3)||(port==4)) reg = reg>>16;
	reg &= 0xffff;
	if ((port==1)||(port==3)) {//addr 48
		if (loop_filter_en) {
			reg &= 0xffffefef;//bit 12,4 (6,4) off
			reg |= 0x00000820;//bit 11,5 (7,3) on
		}
		else {
			reg &= 0xfffff7df;//bit 11,5 (7,3) off
			reg |= 0x00001010;//bit 12,4 (6,4) on
		}
	}
	else {
		if (loop_filter_en) {//addr 7
			reg &= 0xfffff7f7;//bit 11,3 (6,4) off
			reg |= 0x00000410;//bit 10,4 (7,3) on
		}
		else {
			reg &= 0xfffffbef;//bit 10,4 (7,3) off
			reg |= 0x00000808;//bit 11,3 (6,4) on
		}
	}

	m2ddWriteInt32(board,0xe00038,0x1000000|address|reg);
	Sleep(10);*/

/*	unsigned long r48,r49;
	m2ddWriteInt32(board,0xe00038,0x480000);
	Sleep(10);
	m2ddReadInt32(board,0xe00038,r48);
	m2ddWriteInt32(board,0xe00038,0x490000);
	Sleep(10);
	m2ddReadInt32(board,0xe00038,r49);
	printf("\n%08x\n",
		 (((r49>>10)&1)<<0)
		|(((r49>>11)&1)<<1)
		|(((r48>>10)&1)<<2)
		|(((r48>>11)&1)<<3)
		|(((r48>>12)&1)<<4)
		|(((r48>>13)&1)<<5)
		|(((r48>> 4)&1)<<6)
		|(((r48>> 5)&1)<<7)
		|(((r48>> 6)&1)<<8)
		|(((r48>> 7)&1)<<9)
		|(((r48>> 8)&1)<<10)
		|(((r49>> 9)&1)<<11)
		|(((r49>>13)&1)<<12)
		|(((r48>>15)&1)<<13)
		|(((r49>> 0)&1)<<14)
		|(((r49>> 1)&1)<<15)
		|(((r49>> 2)&1)<<16)
		|(((r49>> 3)&1)<<17)
		|(((r49>> 4)&1)<<18)
		|(((r49>> 5)&1)<<19)
		|(((r49>> 6)&1)<<20)
		|(((r49>> 7)&1)<<21)
		|(((r49>> 8)&1)<<22)
		|(((r48>> 9)&1)<<23)
		|(((r49>>12)&1)<<24));
*/
	return 0;
}

int ClockInitStatic(int board, int pll, int force, int freqsel)
{
	unsigned char buf[32],bufout[16];

	I2C_RDP(board,pll*2+2,buf,1,0x81,0xffffff);
	if      ((freqsel==0) && (buf[0]==0x4b)) return 0;
	else if ((freqsel==1) && (buf[0]==0x21)) return 0;
	else if ((freqsel==2) && (buf[0]==0x6))  return 0; 

	if (((buf[0]==0x4b)||(buf[0]==0x21)||(buf[0]==0x06))&&(force==0)) return 0;

	m2ddWriteInt32(board,0xe000e0,0xffffffff);
	Sleep(50);

	buf[0]=0xd2;
	buf[1]=0x81;
	if      (freqsel==0) buf[2]=0x4b;//M=75
	else if (freqsel==1) buf[2]=0x21;//M=33
	else if (freqsel==2) buf[2]=0x6; //M=6
	I2C_WR(board,pll*2+2,buf,3);
	buf[0]=0xd2;
	buf[1]=0x82;
	if      (freqsel==0) buf[2]=0x6c;//lower N=108 (364)
	else if (freqsel==1) buf[2]=0xa0;//lower N=160 (160)
	else if (freqsel==2) buf[2]=0x28;//lower N=40  (40)
	I2C_WR(board,pll*2+2,buf,3);
	buf[0]=0xd2;
	buf[1]=0x83;
	if      (freqsel==0) buf[2]=0x02;//Upper N=256 (364)
	else if (freqsel==1) buf[2]=0x00;//Upper N=0   (160)
	else if (freqsel==2) buf[2]=0x00;//Upper N=0   (40)
	I2C_WR(board,pll*2+2,buf,3);
	buf[0]=0xd2;
	buf[1]=0x86;
	buf[2]=0x60;//Low Frequency
	I2C_WR(board,pll*2+2,buf,3);
	buf[0]=0xd2;
	buf[1]=0x8b;
	buf[2]=0x49;//Input Source
	I2C_WR(board,pll*2+2,buf,3);
	buf[0]=0xd2;
	buf[1]=0x8f;
	buf[2]=0x0a;//P=5
	I2C_WR(board,pll*2+2,buf,3);

	int hs_div,n1,ns,hs,index;
	_int64 rfreq,cry,mult;
	double rfreqf,cryf,cryfchk,freq,multf;
	int n1vals[65]={  1,  2,  4,  6,  8, 10, 12, 14, 16, 18,
					  20, 22, 24, 26, 28, 30, 32, 34, 36, 38,
					  40, 42, 44, 46, 48, 50, 52, 54, 56, 58,
					  60, 62, 64, 66, 68, 70, 72, 74, 76, 78,
					  80, 82, 84, 86, 88, 90, 92, 94, 96, 98,
					 100,102,104,106,108,110,112,114,116,118,
					 120,122,124,126,128};
	int hs_divvals[6] = {4,5,6,7,9,11};
	int fail=1;
	int retry=0;
	int retry2=0;

	while(fail) {
		buf[0]=0xaa;
		buf[1]=135;
		buf[2]=0x81;
		I2C_WR(board,pll*2+1,buf,3);
	
		retry2 = 0;
		buf[0]=1;
		while (buf[0]) {
			if ((retry2++)==100) {
				m2ddWriteInt32(board,0xe000e0,0x4fffffff);
				Sleep(1);
				m2ddWriteInt32(board,0xe000e0,0x00ffffff);
				return 1;
			}
			I2C_RDP(board,pll*2+1,buf,1,135);
		}
		I2C_RDP(board,pll*2+1,buf,6,7);
		
		hs_div = (buf[0]>>5)+4;
		n1 = (((buf[0]<<2)&0x7c)|((buf[1]>>6)&0x3))+1;
		
		rfreq=buf[1]&0x3f;
		rfreq=rfreq<<8;
		rfreq|=buf[2];
		rfreq=rfreq<<8;
		rfreq|=buf[3];
		rfreq=rfreq<<8;
		rfreq|=buf[4];
		rfreq=rfreq<<8;
		rfreq|=buf[5];
		
		cry  = 10*hs_div*n1;
		cryf = cry;
		rfreqf = rfreq;
		rfreqf = rfreqf/(double)(0x10000000);
		cryf /= rfreqf;
		
		if      (freqsel==0) freq = 148.5/1.001;
		else if (freqsel==1) freq = 148.5;
		else if (freqsel==2) freq = 108.0;
		
		for (ns=0;ns<65;ns++) {
			for (hs=5;hs>=0;hs--) {
				cryfchk = freq * (double)hs_divvals[hs] * (double)n1vals[ns];
				if ((cryfchk>4850)&&(cryfchk<5670)) goto divfount;
			}
		}
divfount:
	
		multf = cryfchk/cryf;
		multf = multf*(double)0x10000000;
		mult = multf;
		
		bufout[7] = (unsigned char)(mult&0xff);
		mult = mult>>8;
		bufout[6] = (unsigned char)(mult&0xff);
		mult = mult>>8;
		bufout[5] = (unsigned char)(mult&0xff);
		mult = mult>>8;
		bufout[4] = (unsigned char)(mult&0xff);
		mult = mult>>8;
		bufout[3] = (unsigned char)(mult&0x3f);
		
		bufout[3] |= (((n1vals[ns]-1)<<6)&0xc0);
		
		bufout[2]  = (((n1vals[ns]-1)>>2)&0x1f);
		bufout[2] |= (((hs_divvals[hs]-4)<<5)&0xe0);
		bufout[1]=0x07;
		bufout[0]=0xaa;
	
		for (int i=0;i<4;i++){
			buf[0]=bufout[0];
			buf[1]=bufout[1];
			buf[2]=bufout[2];
			buf[3]=bufout[3];
			buf[4]=bufout[4];
			buf[5]=bufout[5];
			buf[6]=bufout[6];
			buf[7]=bufout[7];
			I2C_WR(board,pll*2+1,buf,8);
		}
		
		buf[0]=0xaa;
		buf[1]=135;
		buf[2]=0x40;
		I2C_WR(board,pll*2+1,buf,3);
		
		retry2=0;
		buf[0]=1;
		while (buf[0]) {
			if ((retry2++)==100) {
				m2ddWriteInt32(board,0xe000e0,0x4fffffff);
				Sleep(1);
				m2ddWriteInt32(board,0xe000e0,0x00ffffff);
				return 1;
			}
			I2C_RDP(board,pll*2+1,buf,1,135);
		}
		
		I2C_RDP(board,pll*2+1,buf,6,7);
		fail = 0;
		for (index=0;index<6;index++) if (buf[index]!=bufout[index+2]) fail=1;
		if ((retry++)==10) {
			m2ddWriteInt32(board,0xe000e0,0x4fffffff);
			Sleep(1);
			m2ddWriteInt32(board,0xe000e0,0x00ffffff);
			return 1;
		}
	}
	m2ddWriteInt32(board,0xe000e0,0x00ffffff);
	return 0;
}

int ClockInit(int board, int freqsel, int clkval)
{
	unsigned char buf[32],bufout[16];

	m2ddWriteInt32(board,0xe0022c,clkval);
	m2ddWriteInt32(board,0xe000e0,0xffffffff);
	Sleep(50);

	buf[0]=0xd2;
	buf[1]=0x81;
	if      (freqsel==0) buf[2]=0x4b;//M=75
	else if (freqsel==1) buf[2]=0x21;//M=33
	else if (freqsel==2) buf[2]=0x6; //M=6
	I2C_WR(board,2,buf,3);
	buf[0]=0xd2;
	buf[1]=0x82;
	if      (freqsel==0) buf[2]=0x6c;//lower N=108 (364)
	else if (freqsel==1) buf[2]=0xa0;//lower N=160 (160)
	else if (freqsel==2) buf[2]=0x28;//lower N=40  (40)
	I2C_WR(board,2,buf,3);
	buf[0]=0xd2;
	buf[1]=0x83;
	if      (freqsel==0) buf[2]=0x02;//Upper N=256 (364)
	else if (freqsel==1) buf[2]=0x00;//Upper N=0   (160)
	else if (freqsel==2) buf[2]=0x00;//Upper N=0   (40)
	I2C_WR(board,2,buf,3);
	buf[0]=0xd2;
	buf[1]=0x86;
	buf[2]=0x60;//Low Frequency
	I2C_WR(board,2,buf,3);
	buf[0]=0xd2;
	buf[1]=0x8b;
	buf[2]=0x49;//Input Source
	I2C_WR(board,2,buf,3);
	buf[0]=0xd2;
	buf[1]=0x8f;
	buf[2]=0x0a;//P=5
	I2C_WR(board,2,buf,3);

	int hs_div,n1,ns,hs,index;
	_int64 rfreq,cry,mult;
	double rfreqf,cryf,cryfchk,freq,multf;
	int n1vals[65]={  1,  2,  4,  6,  8, 10, 12, 14, 16, 18,
					  20, 22, 24, 26, 28, 30, 32, 34, 36, 38,
					  40, 42, 44, 46, 48, 50, 52, 54, 56, 58,
					  60, 62, 64, 66, 68, 70, 72, 74, 76, 78,
					  80, 82, 84, 86, 88, 90, 92, 94, 96, 98,
					 100,102,104,106,108,110,112,114,116,118,
					 120,122,124,126,128};
	int hs_divvals[6] = {4,5,6,7,9,11};
	int fail=1;
	int retry=0;
	int retry2=0;

	while(fail) {
		buf[0]=0xaa;
		buf[1]=135;
		buf[2]=0x81;
		I2C_WR(board,1,buf,3);

		retry2 = 0;
		buf[0]=1;
		while (buf[0]) {
			if ((retry2++)==100) {
				m2ddWriteInt32(board,0xe000e0,0x4fffffff);
				Sleep(1);
				m2ddWriteInt32(board,0xe000e0,0x00ffffff);
				return 1;
			}
			I2C_RDP(board,1,buf,1,135);
		}
		I2C_RDP(board,1,buf,6,7);
		
		hs_div = (buf[0]>>5)+4;
		n1 = (((buf[0]<<2)&0x7c)|((buf[1]>>6)&0x3))+1;
		
		rfreq=buf[1]&0x3f;
		rfreq=rfreq<<8;
		rfreq|=buf[2];
		rfreq=rfreq<<8;
		rfreq|=buf[3];
		rfreq=rfreq<<8;
		rfreq|=buf[4];
		rfreq=rfreq<<8;
		rfreq|=buf[5];
		
		cry  = 10*hs_div*n1;
		cryf = cry;
		rfreqf = rfreq;
		rfreqf = rfreqf/(double)(0x10000000);
		cryf /= rfreqf;
		
		if      (freqsel==0) freq = 148.5/1.001;
		else if (freqsel==1) freq = 148.5;
		else if (freqsel==2) freq = 108.0;
		
		for (ns=0;ns<65;ns++) {
			for (hs=5;hs>=0;hs--) {
				cryfchk = freq * (double)hs_divvals[hs] * (double)n1vals[ns];
				if ((cryfchk>4850)&&(cryfchk<5670)) goto divfount;
			}
		}
divfount:
		
		multf = cryfchk/cryf;
		multf = multf*(double)0x10000000;
		mult = multf;
		
		bufout[7] = (unsigned char)(mult&0xff);
		mult = mult>>8;
		bufout[6] = (unsigned char)(mult&0xff);
		mult = mult>>8;
		bufout[5] = (unsigned char)(mult&0xff);
		mult = mult>>8;
		bufout[4] = (unsigned char)(mult&0xff);
		mult = mult>>8;
		bufout[3] = (unsigned char)(mult&0x3f);
		
		bufout[3] |= (((n1vals[ns]-1)<<6)&0xc0);
		
		bufout[2]  = (((n1vals[ns]-1)>>2)&0x1f);
		bufout[2] |= (((hs_divvals[hs]-4)<<5)&0xe0);
		bufout[1]=0x07;
		bufout[0]=0xaa;

		for (int i=0;i<4;i++){
			buf[0]=bufout[0];
			buf[1]=bufout[1];
			buf[2]=bufout[2];
			buf[3]=bufout[3];
			buf[4]=bufout[4];
			buf[5]=bufout[5];
			buf[6]=bufout[6];
			buf[7]=bufout[7];
			I2C_WR(board,1,buf,8);
		}
		
		buf[0]=0xaa;
		buf[1]=135;
		buf[2]=0x40;
		I2C_WR(board,1,buf,3);
		
		retry2=0;
		buf[0]=1;
		while (buf[0]) {
			if ((retry2++)==100) {
				m2ddWriteInt32(board,0xe000e0,0x4fffffff);
				Sleep(1);
				m2ddWriteInt32(board,0xe000e0,0x00ffffff);
				return 1;
			}
			I2C_RDP(board,1,buf,1,135);
		}
		
		I2C_RDP(board,1,buf,6,7);
		fail = 0;
		for (index=0;index<6;index++) if (buf[index]!=bufout[index+2]) fail=1;
		if ((retry++)==10) {
			m2ddWriteInt32(board,0xe000e0,0x4fffffff);
			Sleep(1);
			m2ddWriteInt32(board,0xe000e0,0x00ffffff);
			return 1;
		}
	}
	int clkchk = 1;
	unsigned long clkcnt;
	retry=0;
	while (clkchk) {
		clkchk = 0;
		if ((retry++)==100) {
			m2ddWriteInt32(board,0xe000e0,0x4fffffff);
			Sleep(1);
			m2ddWriteInt32(board,0xe000e0,0x00ffffff);
			m2ddWriteInt32(board,0xe0022c,clkval);
			return 2;
		}
		if (clkval&0x100) {
			if (retry) {
				m2ddWriteInt32(board,0xe0022c,0xa1000000);
				Sleep(1);
			}
			m2ddWriteInt32(board,0xe0022c,0xa1000100);
			m2ddWriteInt32(board,0xe000e0,0xffffffff);
			Sleep(1);
			m2ddWriteInt32(board,0xe000e0,0x4fffffff);
			Sleep(1);
			m2ddWriteInt32(board,0xe000e0,0x43ffffff);
			m2ddReadInt32(board,0xe00200,clkcnt);
			while ((clkcnt&0xffff)<0x2000) m2ddReadInt32(board,0xe00200,clkcnt);
			m2ddWriteInt32(board,0xe000e0,0x03ffffff);
			Sleep(100);
			m2ddReadInt32(board,0xe00200,clkcnt);
			if ((clkcnt&0xffff)>0x2000) {
				clkchk++;
			}
			else {
				m2ddWriteInt32(board,0xe0022c,0xa0000100);
				m2ddWriteInt32(board,0xe000e0,0x43ffffff);
				m2ddReadInt32(board,0xe00200,clkcnt);
				while ((clkcnt&0xffff)<0x2000) m2ddReadInt32(board,0xe00200,clkcnt);
				m2ddWriteInt32(board,0xe000e0,0x03ffffff);
				Sleep(100);
				m2ddReadInt32(board,0xe00200,clkcnt);
				if ((clkcnt&0xffff)>0x2000) {
					clkchk++;
				}
				else {
					m2ddWriteInt32(board,0xe000e0,0x47ffffff);
					m2ddReadInt32(board,0xe00200,clkcnt);
					while ((clkcnt&0xffff)<0x2000) m2ddReadInt32(board,0xe00200,clkcnt);
					m2ddWriteInt32(board,0xe000e0,0x07ffffff);
					Sleep(100);
					m2ddReadInt32(board,0xe00200,clkcnt);
					if ((clkcnt&0xffff)>0x2000) {
						clkchk++;
					}
					else {
						m2ddWriteInt32(board,0xe0022c,0xa1000100);
						m2ddWriteInt32(board,0xe000e0,0x47ffffff);
						m2ddReadInt32(board,0xe00200,clkcnt);
						while ((clkcnt&0xffff)<0x2000) m2ddReadInt32(board,0xe00200,clkcnt);
						m2ddWriteInt32(board,0xe000e0,0x07ffffff);
						Sleep(100);
						m2ddReadInt32(board,0xe00200,clkcnt);
						if ((clkcnt&0xffff)>0x2000) {
							clkchk++;
						}
					}
				}
			}
		}
		else {
			if (retry) {
				m2ddWriteInt32(board,0xe0022c,0xa1000100);
				Sleep(1);
			}
			m2ddWriteInt32(board,0xe0022c,0xa1000000);
			m2ddWriteInt32(board,0xe000e0,0xffffffff);
			Sleep(1);
			m2ddWriteInt32(board,0xe000e0,0x4fffffff);
			Sleep(1);
			m2ddWriteInt32(board,0xe000e0,0x43ffffff);
			m2ddReadInt32(board,0xe00200,clkcnt);
			while ((clkcnt&0xffff)<0x2000) m2ddReadInt32(board,0xe00200,clkcnt);
			m2ddWriteInt32(board,0xe000e0,0x03ffffff);
			Sleep(100);
			m2ddReadInt32(board,0xe00200,clkcnt);
			if ((clkcnt&0xffff)>0x2000) {
				clkchk++;
			}
			else {
				m2ddWriteInt32(board,0xe0022c,0xa0000000);
				m2ddWriteInt32(board,0xe000e0,0x43ffffff);
				m2ddReadInt32(board,0xe00200,clkcnt);
				while ((clkcnt&0xffff)<0x2000) m2ddReadInt32(board,0xe00200,clkcnt);
				m2ddWriteInt32(board,0xe000e0,0x03ffffff);
				Sleep(100);
				m2ddReadInt32(board,0xe00200,clkcnt);
				if ((clkcnt&0xffff)>0x2000) {
					clkchk++;
				}
				else {
					m2ddWriteInt32(board,0xe000e0,0x47ffffff);
					m2ddReadInt32(board,0xe00200,clkcnt);
					while ((clkcnt&0xffff)<0x2000) m2ddReadInt32(board,0xe00200,clkcnt);
					m2ddWriteInt32(board,0xe000e0,0x07ffffff);
					Sleep(100);
					m2ddReadInt32(board,0xe00200,clkcnt);
					if ((clkcnt&0xffff)>0x2000) {
						clkchk++;
					}
					else {
						m2ddWriteInt32(board,0xe0022c,0xa1000000);
						m2ddWriteInt32(board,0xe000e0,0x47ffffff);
						m2ddReadInt32(board,0xe00200,clkcnt);
						while ((clkcnt&0xffff)<0x2000) m2ddReadInt32(board,0xe00200,clkcnt);
						m2ddWriteInt32(board,0xe000e0,0x07ffffff);
						Sleep(100);
						m2ddReadInt32(board,0xe00200,clkcnt);
						if ((clkcnt&0xffff)>0x2000) {
							clkchk++;
						}
					}
				}
			}
		}
/*		m2ddWriteInt32(board,0xe0022c,(clkval&0x0effffff)|0x50000000);
		m2ddWriteInt32(board,0xe000e0,0xffffffff);
		Sleep(1);
		m2ddWriteInt32(board,0xe000e0,0x4fffffff);
		Sleep(1);
		m2ddWriteInt32(board,0xe000e0,0x03ffffff);
		Sleep(10);
		m2ddReadInt32(board,0xe000e0,clkcnt1);
		Sleep(10);
		m2ddReadInt32(board,0xe000e0,clkcnt2);
		if (clkcnt1==clkcnt2) {
			m2ddWriteInt32(board,0xe0022c,(clkval&0x0effffff)|0x51000000);
			Sleep(10);
			m2ddReadInt32(board,0xe000e0,clkcnt1);
			Sleep(10);
			m2ddReadInt32(board,0xe000e0,clkcnt2);
			if (clkcnt1!=clkcnt2) {
				clkchk++;
				printf("\n1st %08x %08x\n",clkcnt1,clkcnt2);
				MessageBox(NULL,"test","test",MB_OK|MB_TOPMOST);
			}
		}
		else {
			clkchk++;
			printf("\n2nd %08x %08x\n",clkcnt1,clkcnt2);
			MessageBox(NULL,"test","test",MB_OK|MB_TOPMOST);
		}*/
	}
	m2ddWriteInt32(board,0xe000e0,0x00ffffff);
	m2ddWriteInt32(board,0xe0022c,clkval);
	//char see[100];
	//sprintf(see,"%08x",clkval);
	//MessageBox(NULL,see,"test",MB_OK|MB_TOPMOST);

	return 0;
}

//mld functions
//
char *DEVICE_PATH[4] = {"\\\\.\\MerlinDevice0","\\\\.\\MerlinDevice1","\\\\.\\MerlinDevice2","\\\\.\\MerlinDevice3"};
HANDLE hM2[4]={NULL,NULL,NULL,NULL};

HANDLE getM2Handle(int boardID)
{

    HANDLE handle;
    handle=CreateFile(DEVICE_PATH[boardID],
			   GENERIC_READ | GENERIC_WRITE,
			   FILE_SHARE_READ|FILE_SHARE_WRITE,
		       NULL,
			   OPEN_EXISTING,
			   FILE_ATTRIBUTE_NORMAL,
			   NULL);
return handle;
}

int m2ddRdScratch(M2CONTEXT *m2c,DWORD address)
{
	if ((address&0xfffffe00)==0xe00000) return m2c->Scratch[(address>>2)&0x7f];
	if ((address&0xfffffe00)==0xe00200) return m2c->Scratch2[(address>>2)&0x7f];
	return 0;
	//return m2c->Scratch[(address>>2)&0x7f];
}

int m2ddRdInt32(M2CONTEXT *m2c,DWORD address)
{
    BOOL  Mybool;
	DWORD inBuffer[2],outBuffer,byteCount;

    if (hM2[m2c->board]==NULL) {
	hM2[m2c->board]= getM2Handle(m2c->board);
	}

    inBuffer[0]= address;
	inBuffer[1]= 1;       // count as ULONG
	Mybool=DeviceIoControl(hM2[m2c->board],(DWORD)MERLIN_IOCTL_READ,inBuffer,8,&outBuffer,4,&byteCount,NULL);
    
	if (Mybool != TRUE) {
		return STATUS_FAIL;
	}

	return outBuffer;

}

M2DDSTATUS m2ddWrInt32(M2CONTEXT *m2c,DWORD address,DWORD data)
{
	BOOL  Mybool;
    DWORD inBuffer[3],byteCount;

	if ((address&0xfffffe00)==0xe00000) m2c->Scratch[(address>>2)&0x7f] = data;
	if ((address&0xfffffe00)==0xe00200) m2c->Scratch2[(address>>2)&0x7f] = data;
	if (hM2[m2c->board]==NULL) {
    hM2[m2c->board]=getM2Handle(m2c->board);
	}

    inBuffer[0]=address;
	inBuffer[1]=1;
	inBuffer[2]=data;
	Mybool=DeviceIoControl(hM2[m2c->board],(DWORD)MERLIN_IOCTL_WRITE,inBuffer,12,NULL,0,&byteCount,NULL);
	if (Mybool != TRUE) {
        return STATUS_FAIL;
	}
 
    return STATUS_PASS;
}

M2DDSTATUS   m2ddReadInt32(int boardID,DWORD address,DWORD& data)
{
    BOOL  Mybool;
	DWORD inBuffer[2],outBuffer,byteCount;

    if (hM2[boardID]==NULL) {
	hM2[boardID]= getM2Handle(boardID);
	}

    inBuffer[0]= address;
	inBuffer[1]= 1;       // count as ULONG
	Mybool=DeviceIoControl(hM2[boardID],(DWORD)MERLIN_IOCTL_READ,inBuffer,8,&outBuffer,4,&byteCount,NULL);
    
	if (Mybool != TRUE) {
		return STATUS_FAIL;
	}

	data= outBuffer;
	return STATUS_PASS;

}

M2DDSTATUS m2ddWriteInt32(int boardID,DWORD address,DWORD data)
{
	BOOL  Mybool;
    DWORD inBuffer[3],byteCount;
	
	if (hM2[boardID]==NULL) {
    hM2[boardID]=getM2Handle(boardID);
	}

    inBuffer[0]=address;
	inBuffer[1]=1;
	inBuffer[2]=data;
	Mybool=DeviceIoControl(hM2[boardID],(DWORD)MERLIN_IOCTL_WRITE,inBuffer,12,NULL,0,&byteCount,NULL);
	if (Mybool != TRUE) {
        return STATUS_FAIL;
	}
 
    return STATUS_PASS;
}

M2DDSTATUS m2ddMapMemory(int boardID, DWORD *mapping)
{
    BOOL  Mybool;
	DWORD outBuffer[20],byteCount;
	M2CONTEXT *m2c;

    if (hM2[boardID]==NULL) {
	hM2[boardID]= getM2Handle(boardID);
	}

	Mybool=DeviceIoControl(hM2[boardID],(DWORD)MERLIN_IOCTL_MAP_MEMORY,NULL,0,&outBuffer,80,&byteCount,NULL);
    if (Mybool != TRUE) {
	  Mybool=DeviceIoControl(hM2[boardID],(DWORD)MERLIN_IOCTL_MAP_MEMORY,NULL,0,&outBuffer,16,&byteCount,NULL);
      if (Mybool != TRUE) {
	    Mybool=DeviceIoControl(hM2[boardID],(DWORD)MERLIN_IOCTL_MAP_MEMORY,NULL,0,&outBuffer,12,&byteCount,NULL);
		if (Mybool != TRUE) {
		  return STATUS_FAIL;
		}
        for (int i=0; i<20; i++) mapping[i] = 0;
		mapping[2] = outBuffer[0];
		mapping[3] = outBuffer[1];
		mapping[4] = outBuffer[2];
	    return STATUS_PASS;
	  }
      for (int i=0; i<20; i++) mapping[i] = 0;
      mapping[0] = outBuffer[0];
      mapping[2] = outBuffer[1];
	  Mybool=DeviceIoControl(hM2[boardID],(DWORD)MERLIN_IOCTL_MAP_MEMORY2,NULL,0,&outBuffer,16,&byteCount,NULL);
      if (Mybool != TRUE) {
	    return STATUS_FAIL;
	  }
      mapping[5] = outBuffer[0];
      mapping[7] = outBuffer[1];
	  m2c = (M2CONTEXT*)mapping[2];
	  mapping[4] = m2c->Buffer_Megs;
	  mapping[9] = m2c->Buffer_Megs2;
	  return STATUS_PASS;
    }
	for (int i=0; i<20; i++) mapping[i] = outBuffer[i];

	return STATUS_PASS;

}
/*
M2DDSTATUS m2ddMapMemory(int boardID, DWORD *physicalAddress, DWORD *virtualAddress, DWORD *physicalAddressUpper, DWORD *virtualAddressUpper)
{
    BOOL  Mybool;
	DWORD outBuffer[4],byteCount;

    if (hM2[boardID]==NULL) {
	hM2[boardID]= getM2Handle(boardID);
	}

	physicalAddressUpper[0] = 0;
	virtualAddressUpper[0]  = 0;

	Mybool=DeviceIoControl(hM2[boardID],(DWORD)MERLIN_IOCTL_MAP_MEMORY,NULL,0,&outBuffer,16,&byteCount,NULL);
	if (Mybool != TRUE) {
	  printf("Using 32 bit mapping\n");
	  Mybool=DeviceIoControl(hM2[boardID],(DWORD)MERLIN_IOCTL_MAP_MEMORY,NULL,0,&outBuffer,8,&byteCount,NULL);
      if (Mybool != TRUE) {
		return STATUS_FAIL;
	  }
	}
	else {
		physicalAddressUpper[0] = outBuffer[2];
		virtualAddressUpper[0]  = outBuffer[3];
	}

	physicalAddress[0] = outBuffer[0];
	virtualAddress[0]  = outBuffer[1];

	return STATUS_PASS;

}

M2DDSTATUS m2ddMapMemory2(int boardID, DWORD *physicalAddress, DWORD *virtualAddress, DWORD *physicalAddressUpper, DWORD *virtualAddressUpper)
{
    BOOL  Mybool;
	DWORD outBuffer[4],byteCount;

    if (hM2[boardID]==NULL) {
	hM2[boardID]= getM2Handle(boardID);
	}

	physicalAddressUpper[0] = 0;
	virtualAddressUpper[0]  = 0;

	Mybool=DeviceIoControl(hM2[boardID],(DWORD)MERLIN_IOCTL_MAP_MEMORY2,NULL,0,&outBuffer,16,&byteCount,NULL);
	if (Mybool != TRUE) {
	  printf("Using 32 bit mapping\n");
	  Mybool=DeviceIoControl(hM2[boardID],(DWORD)MERLIN_IOCTL_MAP_MEMORY2,NULL,0,&outBuffer,8,&byteCount,NULL);
      if (Mybool != TRUE) {
		return STATUS_FAIL;
	  }
	}
	else {
		physicalAddressUpper[0] = outBuffer[2];
		virtualAddressUpper[0]  = outBuffer[3];
	}

	physicalAddress[0] = outBuffer[0];
	virtualAddress[0]  = outBuffer[1];

	return STATUS_PASS;

}
*/
M2DDSTATUS   m2ddWriteBlk_ioctl(int boardID,DWORD address,char *data,DWORD length)
{
	BOOL  Mybool;
    DWORD byteCount;
	DWORD  *inBuffer;
	char   *charPtr;
	DWORD    i;

	if (hM2[boardID]==NULL) {
    	hM2[boardID]= getM2Handle(boardID);
	}
    
	length= ((length+3)>>2)<<2;
	inBuffer= (DWORD *) malloc (8+length);
    inBuffer[0]=address;
	inBuffer[1]=length/4;
	charPtr = (char *) (inBuffer+2);
	for (i=0;i<length;i++)
		charPtr[i]=data[i];

    Mybool=DeviceIoControl(hM2[boardID],(DWORD)MERLIN_IOCTL_WRITE,inBuffer,8+length,NULL,0,&byteCount,NULL);

	if (inBuffer != NULL)
		free(inBuffer);

    if (Mybool != TRUE) {
	return STATUS_FAIL;
	}

    return STATUS_PASS;
}

M2DDSTATUS   m2ddReadBlk_ioctl(int boardID,DWORD address,char *data,DWORD length)
{
	BOOL  Mybool;
    DWORD byteCount, inBuffer[2];
	
	if (hM2[boardID]==NULL) {
    	hM2[boardID]= getM2Handle(boardID);
	}
    inBuffer[0]=address;
	length= ((length+3)>>2)<<2;
	inBuffer[1]=length/4;
    Mybool=DeviceIoControl(hM2[boardID],(DWORD)MERLIN_IOCTL_READ,inBuffer,8,data,length,&byteCount,NULL);
    if (Mybool != TRUE) {
	return STATUS_FAIL;
	}

    return STATUS_PASS;
}

M2DDSTATUS  m2ddSendEventHandle(int boardID,HANDLE hEvent)
{
	BOOL    ioctlResult;
    ULONG   Outbuffer;
	ULONG   InBuffer;
    // The following parameters are used in the IOCTL call
    DWORD   returnedLength;		// Number of bytes returned

	InBuffer = (DWORD)hEvent;
    if (hM2[boardID]==NULL) {
    	hM2[boardID]= getM2Handle(boardID);
	}
	

	ioctlResult = DeviceIoControl(
                            hM2[boardID],	        					// Handle to device
                            (DWORD)MERLIN_IOCTL_SEND_EVENT_HANDLE,	// IO Control code for Read
                            &InBuffer,						    // Buffer to driver.
                            4,									// Length of buffer in bytes.
                            &Outbuffer,								// Buffer from driver.
                            4,									// Length of buffer in bytes.
                            &returnedLength,					// Bytes placed in DataBuffer.
                            NULL								// NULL means wait till op. completes.
                            );

	if (ioctlResult == TRUE ) 
		return  STATUS_PASS;
	else
		return  STATUS_FAIL;
}

int mld(int boardID, char *pUxFileName){
//	PSTR			pUxFileName;
	UXFILEHEDER		uxFileHdr;
	UXSECTIONHDR	uxSectionHdr;
	HANDLE			hUxFile;
	DWORD			BytesRead;
    int             wdldFlag=0;   
    //int             boardID;   
	//****** parse arguments *******************

	//pUxFileName = (PSTR) argv[argc-1];
	//int i;
	//boardID=0;
    //for (i=0;i<argc;i++){
	//	if (strcmp(argv[i],"-b2")==0)
	//	   boardID=1;
	//    else if (strcmp(argv[i],"-first")==0)
	//	   wdldFlag=2;
	//    else if (strcmp(argv[i],"-last")==0)
	//	   wdldFlag=999;
	//
	//}
	wdldFlag = 2;

	// Read the ux file header to find how many sections are to be loaded.
	if ((hUxFile = CreateFile(pUxFileName, 
							   GENERIC_READ,
							   FILE_SHARE_READ,
							   NULL,
							   OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL,
							   NULL)) == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	ReadFile(hUxFile, (LPVOID)&uxFileHdr, (DWORD)sizeof(UXFILEHEDER), &BytesRead, NULL);

	// Can not compare with 'C3UX' because of big / little endian format.
	// if (uxFileHdr.MagicNumber != 0x58553343)
	// Convert from big to little endian.
	ReverseByteOrdering(&uxFileHdr.MagicNumber, sizeof (DWORD), sizeof (DWORD));
	if (uxFileHdr.MagicNumber != 'C3UX')	
	{
//		DOUT("Wrong Magic Number\n");
		CloseHandle(hUxFile);
		return FALSE;
	}

//	wsprintf(szbuf, "Number of Sections = %lu\n", uxFileHdr.NumberOfSections);
//	DOUT(szbuf);

	// Loop until all sections are loaded.
	if (wdldFlag==999)  { // -last
    	while (uxFileHdr.NumberOfSections>0)
		{
			// Load each section.
			ReadFile(hUxFile, &uxSectionHdr, sizeof(UXSECTIONHDR), &BytesRead, NULL);
		
//			printf(szbuf, "SectionLength = %lu\n", uxSectionHdr.SectionLength);
//			DOUT(szbuf);
		
			if (uxSectionHdr.SectionLength)
			{
				LPBYTE	pSectionData = (BYTE *)malloc (uxSectionHdr.SectionLength);
		
				ReadFile(hUxFile, pSectionData, uxSectionHdr.SectionLength, &BytesRead, NULL);
				if (uxFileHdr.NumberOfSections <=2) 			
					LoadSectionData(boardID, &uxSectionHdr, pSectionData);
		
				free(pSectionData);
			}
			uxFileHdr.NumberOfSections--;
		}

	}
	else{

		while (uxFileHdr.NumberOfSections-wdldFlag >0)
		{
			// Load each section.
			ReadFile(hUxFile, &uxSectionHdr, sizeof(UXSECTIONHDR), &BytesRead, NULL);
		
			//printf(szbuf, "SectionLength = %lu\n", uxSectionHdr.SectionLength);
//			DOUT(szbuf);
		
			if (uxSectionHdr.SectionLength)
			{
				LPBYTE	pSectionData = (BYTE *)malloc (uxSectionHdr.SectionLength);
		
				ReadFile(hUxFile, pSectionData, uxSectionHdr.SectionLength, &BytesRead, NULL);
				LoadSectionData(boardID, &uxSectionHdr, pSectionData);
		
				free(pSectionData);
			}
			uxFileHdr.NumberOfSections--;
		}
		
	}
	CloseHandle(hUxFile);
	return TRUE;
}

BOOL					// Return TRUE on success and FALSE for failure
LoadSectionData(				// Load the section data.
	int             boardID,
	PUXSECTIONHDR	pUxSectionHdr,	// Ux section header pointer.
	PBYTE			pData)		// Pointer to the section data.
{
	DWORD	DataLength = pUxSectionHdr->SectionLength;
	BYTE *	pWaitForData;
	BOOL	bLoop = TRUE;
	int		SectionAddr;

	switch (pUxSectionHdr->Flags & 0x06)
	{
		case SECTION_FLAG_DATA_BLOCK:
			break;

		case SECTION_FLAG_ADDR_DATA_PAIR:
			break;

		case SECTION_FLAG_WAIT_FOR_DATA:
			pWaitForData = (BYTE *)malloc(pUxSectionHdr->SectionLength);
			while (bLoop)
			{
				M2ReadDataBlock(boardID, pUxSectionHdr->SectionStart, (BYTE *)pWaitForData, pUxSectionHdr->SectionLength);

				// Check all bytes to be same.
				DWORD i;
				for (i = 0; i < pUxSectionHdr->SectionLength; i++)
				{
					if (*(pData + i) != *(pWaitForData + i))
					{
						break;
					}
				}

				// If all the bytes matched then break while loop.
				if (i == pUxSectionHdr->SectionLength)
				{
					bLoop = FALSE;
					break;
				}
			}
			return TRUE;
			break;

		case SECTION_FLAG_LOOP_DELAY:
			ReverseByteOrdering(pData, sizeof (DWORD), 4);
			Sleep((*(DWORD *)pData + 999) / 1000);	// Wait for so many ms.
			return TRUE;
			break;
	}

	// For SDRAM the data is on 8 bit and for CBUS it is 32 bits.
	if (pUxSectionHdr->DataType == SDRAM)
	{
		if (pUxSectionHdr->Flags & SECTION_FLAG_ADDR_DATA_PAIR)
		{
			// Load the address data pairs.
			while(DataLength)
			{

				// SDRAM address 0x40028 is used to signal completion of 
				// microcode download. If a serial rom is present, M2 checks 
				// this location to verify microcode download is complete, then begins 
				// execution. If buffers haven't been allocated prior to starting M2 (with
				// serial rom), system will crash. Added parser to ensure that 0x40028 isn't
				// set with serial rom present.

				ReverseByteOrdering(pData, sizeof (DWORD), 4);

				if((*((DWORD *)pData) != 0x40028))
					M2WriteDataBlock(boardID,*((DWORD *)pData), (BYTE *)((DWORD *)pData + 1), 1);
				
				else { 
	
					if(!SERIAL_ROM_PRESENT)
					M2WriteDataBlock(boardID,*((DWORD *)pData), (BYTE *)((DWORD *)pData + 1), 1);
				}

				pData += (sizeof(DWORD) + 1);
				DataLength -= (sizeof(DWORD) + 1);
			}
		}
		else
		{
			ReverseByteOrdering((DWORD *)pData, DataLength, 4);
			M2WriteDataBlock(boardID,pUxSectionHdr->SectionStart, (BYTE *)pData, DataLength);
		}
	}
	else if (pUxSectionHdr->DataType == CBUS)
	{
		if (pUxSectionHdr->Flags & SECTION_FLAG_ADDR_DATA_PAIR)
		{
			while(DataLength)
			{
				ReverseByteOrdering(pData, sizeof (DWORD), sizeof (DWORD));
				ReverseByteOrdering(((DWORD *)pData + 1), sizeof (DWORD), 4);

				SectionAddr = CheckSectionAddrDataPair(pData);	
				// added filter to check for chip reset,
				// SDRAM clock control and SDRAM control register initialization values.
				
				M2WriteDataBlock(boardID,*(DWORD *)pData, (BYTE *)((DWORD *)pData + 1), 4);

				switch(SectionAddr){
		
					case kReset:					                    // check for chip reset
						if(SERIAL_ROM_PRESENT) Sleep(kResetTimeLimit);	// wait for PROM to load
						break;
				
					case SDRAM_ClkControl:	// check for SDRAM clock control register write
						Sleep(10);		    // some time for clocks to stabilize.
						break;

					default:
						break;

	
				} // end switch				
				
				pData += 2 * sizeof(DWORD);
				DataLength -= 2 * sizeof(DWORD);
			}
		}
		else
		{
			ReverseByteOrdering((DWORD *)pData, DataLength, 4);
			M2WriteDataBlock(boardID,pUxSectionHdr->SectionStart, (BYTE *)pData, DataLength / sizeof(DWORD));
		}
	}
	else
		return FALSE;

	return TRUE;
}

BOOL									// Return TRUE on success, else FALSE.
M2ReadDataBlock(						// Read block of data. Byte, WORD or DWORD.
	int     boardID,
	DWORD	address,					// Address where data is to be written.
	BYTE *	data,						// Pointer to data.
	DWORD	length)						// Number of bytes to be read.
{
	DWORD	dwAddress = address;
	DWORD	dwData, dwMask;
	DWORD	dwLength = length;
	BYTE *	pData = data;

	// This needs to be done bwcause M2 supports only 32 bit aligned accesses.
	while (dwLength)
	{
		// If address is not 32 bit aligned read data from previous 32 bit aligned address.
		if ((dwAddress & 0x03) || (dwLength < 4))
		{
			m2ddReadBlk_ioctl(boardID,(dwAddress & ~0x00000003), (char *)&dwData, sizeof (DWORD));
			// Mask off the byte to be modified.
			dwMask = 0x000000ff << (3-(dwAddress & 0x03)) ;
			dwData &= dwMask;
			dwData = dwData >> ( 3- (dwAddress & 0x03) );

			// Set the data at required byte location.
			*pData = (BYTE)dwData;

			dwLength--;
			pData++;
			dwAddress++;
		}
		else
		{
			m2ddReadBlk_ioctl(boardID,dwAddress, (char *)pData, (dwLength / sizeof (DWORD)) * sizeof (DWORD));
			dwLength -= (dwLength / sizeof (DWORD)) * sizeof (DWORD);
			pData += (dwLength / sizeof (DWORD)) * sizeof (DWORD);
			dwAddress += (dwLength / sizeof (DWORD)) * sizeof (DWORD);
		}
	}

	return TRUE;
}

BOOL									// Return TRUE on success, else FALSE.
M2WriteDataBlock(						// Write block of data. Byte, WORD or DWORD.
	int     boardID,
	DWORD	address,					// Address where data is to be written.
	BYTE *	data,						// Pointer to data.
	DWORD	length)						// Number of bytes to be written.
{
	DWORD	dwAddress = address;
	DWORD	dwData, dwMask;
	DWORD	dwLength = length;
	BYTE *	pData = data;

	// If address is not 32 bit alogned then get the data, mask off the byte to be written,
	// replace with the byte to be written, write back the data t 32 bit alogned address.
	// This needs to be done bwcause M2 supports only 32 bit aligned accesses.
	while (dwLength)
	{
		// If address is not 32 bit aligned read data from previous 32 bit aligned address.
		if ((dwAddress & 0x03) || (dwLength < 4))
		{
			m2ddReadBlk_ioctl(boardID,(dwAddress & ~0x00000003), (char *)&dwData, sizeof (DWORD));
			// Mask off the byte to be modified.
			dwMask = 0x000000ff << ((dwAddress & 0x03) * 8);
			dwData &= ~dwMask;

			// Set the data at required byte location.
			dwData |= (DWORD)*pData;// << ((dwAddress & 0x03) * 8);

			// Write back 32 bits.
			m2ddWriteBlk_ioctl(boardID,(dwAddress & ~0x00000003), (char *)&dwData, sizeof (DWORD));
			dwLength--;
			pData++;
			dwAddress++;
		}
		else
		{
            DWORD writeLength;
			writeLength = (dwLength >> 2) << 2;
			if (writeLength>4096) {	 //Added because interrupts get locked when writting more thatn 4096 byes at a time
				writeLength = 4096;
				Sleep(1);
			}
			m2ddWriteBlk_ioctl(boardID,dwAddress, (char *)pData, writeLength);		
			pData += writeLength;
			dwAddress += writeLength;
			dwLength  -= writeLength;
		}
	}

	return TRUE;
}

BOOL					// Return TRUE on success else FALSE.
ReverseByteOrdering(				// Convert from Big Endian to Little Endian.
	PVOID	pBuffer,			// Pointer to the buffer to be converted.
	DWORD	SizeOfBuffer,		// Size of the buffer in bytes.
	BYTE	FormatConversion)	// WORD or DWORD conversion.
{
	BYTE	Temp1;
	BYTE *	pBuf = (BYTE *)pBuffer;

	switch (FormatConversion)
	{
		case 2:
			while (SizeOfBuffer / 2)
			{
				Temp1 = *pBuf;
				*pBuf = *(pBuf+1);
				*pBuf = Temp1;
				SizeOfBuffer -= 2;
				pBuf += 2;
			}
			break;

		case 4:
			while (SizeOfBuffer / 4)
			{
				Temp1 = *pBuf;
				*pBuf = *(pBuf+3);
				*(pBuf+3) = Temp1;
				Temp1 = *(pBuf+1);
				*(pBuf+1) = *(pBuf+2);
				*(pBuf+2) = Temp1;
				SizeOfBuffer -= 4;
				pBuf += 4;
			}
			break;

		default:
			return FALSE;
	}

	return TRUE;

}


int CheckSectionAddrDataPair(PBYTE pData)  
{
	int SectionAddr=0;
	
	switch(*(DWORD *)pData){
		
		case 0xfc0000:  // check for chip reset
			SectionAddr = kReset;
			break;

		case 0xfc0610:	// check for SDRAM clock control register write
			pData += sizeof(DWORD);
			(*(DWORD *)pData) = SDRAM_CLK_CONTROL_INIT;
			SectionAddr = SDRAM_ClkControl;
			pData -= sizeof(DWORD);
			break;

		case 0xfc0600:	// check for SDRAM clock control register write
			pData += sizeof(DWORD);
			(*(DWORD *)pData) = SDRAM_CONTROL_INIT;
			SectionAddr = SDRAM_Control;
			pData -= sizeof(DWORD);
			break;

		default:
			break;

	} // end switch

	return SectionAddr;

}

int fc2scdf (int fc, int palmode, int nondrop) //FRAMECOUNT to SMPTE
{
    int i,j[8],frate,mypal;

	mypal = palmode;
	if (mypal==30) mypal=0;

    if (mypal>1) {
		frate=mypal;
        for (i=0 ; i<8 ; i++) j[i] = 0;
        i = fc;
		while (i>(86400*frate)) i = i - (86400*frate);
        j[7]=i/frate/36000;
        i-=j[7]*frate*36000;
        j[6]=i/frate/3600;
        i-=j[6]*frate*3600;
        j[5]=i/frate/600;
        i-=j[5]*frate*600;
        j[4]=i/frate/60;
        i-=j[4]*frate*60;
        j[3]=i/frate/10;
        i-=j[3]*frate*10;
        j[2]=i/frate;
        i-=j[2]*frate;
        j[1]=i/10;
        i-=j[1]*10;
        j[0] = i;
        return (j[7]<<28)|(j[6]<<24)|(j[5]<<20)|(j[4]<<16)|(j[3]<<12)|(j[2]<<8)|(j[1]<<4)|j[0];
	}
    else if (mypal) {
        for (i=0 ; i<8 ; i++) j[i] = 0;
        i = fc;
		while (i>2160000) i = i - 2160000;
        j[7]=i/900000;
        i-=j[7]*900000;
        j[6]=i/90000;
        i-=j[6]*90000;
        j[5]=i/15000;
        i-=j[5]*15000;
        j[4]=i/1500;
        i-=j[4]*1500;
        j[3]=i/250;
        i-=j[3]*250;
        j[2]=i/25;
        i-=j[2]*25;
        j[1]=i/10;
        i-=j[1]*10;
        j[0] = i;
        return (j[7]<<28)|(j[6]<<24)|(j[5]<<20)|(j[4]<<16)|(j[3]<<12)|(j[2]<<8)|(j[1]<<4)|j[0];
    }
	else if (nondrop) {
        for (i=0 ; i<8 ; i++) j[i] = 0;
        i = fc;
		while (i>2592000) i = i - 2592000;
        j[7]=i/1080000;
        i-=j[7]*1080000;
        j[6]=i/108000;
        i-=j[6]*108000;
        j[5]=i/18000;
        i-=j[5]*18000;
        j[4]=i/1800;
        i-=j[4]*1800;
        j[3]=i/300;
        i-=j[3]*300;
        j[2]=i/30;
        i-=j[2]*30;
        j[1]=i/10;
        i-=j[1]*10;
        j[0] = i;
        return (j[7]<<28)|(j[6]<<24)|(j[5]<<20)|(j[4]<<16)|(j[3]<<12)|(j[2]<<8)|(j[1]<<4)|j[0];
	}
    else {
        for (i=0 ; i<8 ; i++) j[i] = 0;
        i = fc;
		while (i>2589408) i = i - 2589408;
        j[7]=i/1078920;
        i-=j[7]*1078920;
        j[6]=i/107892;
        i-=j[6]*107892;
        j[5]=i/17982;
        i-=j[5]*17982;
        if (i > 1799) {
            i-=2;
            j[4]=i/1798;
            i-=j[4]*1798;
            i+=2;
        }
        j[3]=i/300;
        i-=j[3]*300;
        j[2]=i/30;
        i-=j[2]*30;
        j[1]=i/10;
        i-=j[1]*10;
        j[0] = i;
        return (j[7]<<28)|(j[6]<<24)|(j[5]<<20)|(j[4]<<16)|(j[3]<<12)|(j[2]<<8)|(j[1]<<4)|j[0];
    }
}

int sc2fcdf (int sc, int palmode, int nondrop) //SMPTE to FRAMECOUNT
{
	int l,j[8],frate;

    if (palmode>1) {
		frate=palmode;
		l = sc;
		j[7]=(l&0xf0000000)>>28;
		j[6]=(l&0x0f000000)>>24;
		j[5]=(l&0x00f00000)>>20;
		j[4]=(l&0x000f0000)>>16;
		j[3]=(l&0x0000f000)>>12;
		j[2]=(l&0x00000f00)>>8;
		j[1]=(l&0x000000f0)>>4;
		j[0]=(l&0x0000000f);
		l  = j[7] * frate*36000;
		l += j[6] * frate*3600;
		l += j[5] * frate*600;
		l += j[4] * frate*60;
		l += j[3] * frate*10;
		l += j[2] * frate;
		l += j[1] * 10;
		l += j[0];
		return l;
	}
	else if (palmode) {
		l = sc;
		j[7]=(l&0xf0000000)>>28;
		j[6]=(l&0x0f000000)>>24;
		j[5]=(l&0x00f00000)>>20;
		j[4]=(l&0x000f0000)>>16;
		j[3]=(l&0x0000f000)>>12;
		j[2]=(l&0x00000f00)>>8;
		j[1]=(l&0x000000f0)>>4;
		j[0]=(l&0x0000000f);
		l  = j[7] * 900000;
		l += j[6] * 90000;
		l += j[5] * 15000;
		l += j[4] * 1500;
		l += j[3] * 250;
		l += j[2] * 25;
		l += j[1] * 10;
		l += j[0];
		return l;
	}
	else if (nondrop) {
		l = sc;
		j[7]=(l&0xf0000000)>>28;
		j[6]=(l&0x0f000000)>>24;
		j[5]=(l&0x00f00000)>>20;
		j[4]=(l&0x000f0000)>>16;
		j[3]=(l&0x0000f000)>>12;
		j[2]=(l&0x00000f00)>>8;
		j[1]=(l&0x000000f0)>>4;
		j[0]=(l&0x0000000f);
		l  = j[7] * 1080000;
		l += j[6] * 108000;
		l += j[5] * 18000;
		l += j[4] * 1800;
		l += j[3] * 300;
		l += j[2] * 30;
		l += j[1] * 10;
		l += j[0];
		return l;
	}
	else {
		l = sc;
		j[7]=(l&0xf0000000)>>28;
		j[6]=(l&0x0f000000)>>24;
		j[5]=(l&0x00f00000)>>20;
		j[4]=(l&0x000f0000)>>16;
		j[3]=(l&0x0000f000)>>12;
		j[2]=(l&0x00000f00)>>8;
		j[1]=(l&0x000000f0)>>4;
		j[0]=(l&0x0000000f);
		l  = j[7] * 1078920;
		l += j[6] * 107892;
		l += j[5] * 17982;
		l += j[4] * 1798;
		l += j[3] * 300;
		l += j[2] * 30;
		l += j[1] * 10;
		l += j[0];
		return l;
	}
}

int scfixdf (int sc, int palmode, int nondrop) //SMPTE FIXER - IF invalid SMPTE forces it to a valid value
{
	int l,j[8];

	if (palmode>1) {
		l = sc;
		j[7]=(l&0xf0000000)>>28;
		j[6]=(l&0x0f000000)>>24;
		j[5]=(l&0x00f00000)>>20;
		j[4]=(l&0x000f0000)>>16;
		j[3]=(l&0x0000f000)>>12;
		j[2]=(l&0x00000f00)>>8;
		j[1]=(l&0x000000f0)>>4;
		j[0]=(l&0x0000000f);

		if  (j[7] > 2) j[7] = 2;
		if  (j[6] > 9) j[6] = 9;
		if ((j[7] == 2) && (j[6] > 3)) j[6] = 3;

		if  (j[5] > 5) j[5] = 5;
		if  (j[4] > 9) j[4] = 9;
	
		if  (j[3] > 5) j[3] = 5;
		if  (j[2] > 9) j[2] = 9;
	
		if (palmode == 60) {
			if  (j[1] > 5) j[1] = 5;
		}
		else {
			if  (j[1] > 2) j[1] = 2;
		}
		if  (j[0] > 9) j[0] = 9;
		if (palmode == 24) {
			if (j[1] == 2) {
				if  (j[0] > 4) j[0] = 3;//only covers 24 fps
			}
		}
	
		return (j[7]<<28)|(j[6]<<24)|(j[5]<<20)|(j[4]<<16)|(j[3]<<12)|(j[2]<<8)|(j[1]<<4)|j[0];
	}
	else if (palmode) {
		l = sc;
		j[7]=(l&0xf0000000)>>28;
		j[6]=(l&0x0f000000)>>24;
		j[5]=(l&0x00f00000)>>20;
		j[4]=(l&0x000f0000)>>16;
		j[3]=(l&0x0000f000)>>12;
		j[2]=(l&0x00000f00)>>8;
		j[1]=(l&0x000000f0)>>4;
		j[0]=(l&0x0000000f);

		if  (j[7] > 2) j[7] = 2;
		if  (j[6] > 9) j[6] = 9;
		if ((j[7] == 2) && (j[6] > 3)) j[6] = 3;

		if  (j[5] > 5) j[5] = 5;
		if  (j[4] > 9) j[4] = 9;
	
		if  (j[3] > 5) j[3] = 5;
		if  (j[2] > 9) j[2] = 9;
	
		if  (j[1] > 2) j[1] = 2;
		if  (j[0] > 9) j[0] = 9;
		if (j[1] == 2) {
			if  (j[0] > 4) j[0] = 4;
		}
	
		return (j[7]<<28)|(j[6]<<24)|(j[5]<<20)|(j[4]<<16)|(j[3]<<12)|(j[2]<<8)|(j[1]<<4)|j[0];
	}
	else if (nondrop) {
		l = sc;
		j[7]=(l&0xf0000000)>>28;
		j[6]=(l&0x0f000000)>>24;
		j[5]=(l&0x00f00000)>>20;
		j[4]=(l&0x000f0000)>>16;
		j[3]=(l&0x0000f000)>>12;
		j[2]=(l&0x00000f00)>>8;
		j[1]=(l&0x000000f0)>>4;
		j[0]=(l&0x0000000f);

		if  (j[7] > 2) j[7] = 2;
		if  (j[6] > 9) j[6] = 9;
		if ((j[7] == 2) && (j[6] > 3)) j[6] = 3;

		if  (j[5] > 5) j[5] = 5;
		if  (j[4] > 9) j[4] = 9;
	
		if  (j[3] > 5) j[3] = 5;
		if  (j[2] > 9) j[2] = 9;
	
		if  (j[1] > 2) j[1] = 2;
		if  (j[0] > 9) j[0] = 9;
	
		return (j[7]<<28)|(j[6]<<24)|(j[5]<<20)|(j[4]<<16)|(j[3]<<12)|(j[2]<<8)|(j[1]<<4)|j[0];
	}
	else {
		l = sc;
		j[7]=(l&0xf0000000)>>28;
		j[6]=(l&0x0f000000)>>24;
		j[5]=(l&0x00f00000)>>20;
		j[4]=(l&0x000f0000)>>16;
		j[3]=(l&0x0000f000)>>12;
		j[2]=(l&0x00000f00)>>8;
		j[1]=(l&0x000000f0)>>4;
		j[0]=(l&0x0000000f);

		if  (j[7] > 2) j[7] = 2;
		if  (j[6] > 9) j[6] = 9;
		if ((j[7] == 2) && (j[6] > 3)) j[6] = 3;

		if  (j[5] > 5) j[5] = 5;
		if  (j[4] > 9) j[4] = 9;
	
		if  (j[3] > 5) j[3] = 5;
		if  (j[2] > 9) j[2] = 9;
	
		if  (j[1] > 2) j[1] = 2;
		if  (j[0] > 9) j[0] = 9;
		if ((j[4] != 0) && (j[3] == 0) && (j[2] == 0) && (j[1] == 0) && (j[0] < 2)) j[0] = 2;
	
		return (j[7]<<28)|(j[6]<<24)|(j[5]<<20)|(j[4]<<16)|(j[3]<<12)|(j[2]<<8)|(j[1]<<4)|j[0];
	}
}

int fc2sc (int fc, int palmode){return fc2scdf(fc,palmode,0);} //FRAMECOUNT to SMPTE
int sc2fc (int sc, int palmode){return sc2fcdf(sc,palmode,0);} //SMPTE to FRAMECOUNT
int scfix (int sc, int palmode){return scfixdf(sc,palmode,0);} //SMPTE FIXER - IF invalid SMPTE forces it to a valid value

void WriteAudHdr(char *filename, int samplerate, int channels) 
{
	int fabuf[12],i;
	FILE *wavf;
	_int64 filesize;

	if (wavf = fopen(filename,"rb+")) {
		filesize = _filelengthi64( _fileno(wavf));
	 
		i=(int)filesize;
		fabuf[0] =0x46464952; //"RIFF"
		fabuf[1] =i-8;		  //data size + 36 bytes
		fabuf[2] =0x45564157; //"WAVE"
		fabuf[3] =0x20746d66; //"fmt "
		fabuf[4] =0x00000010; //size of wave section
		fabuf[5] =(0x00010000*channels) + 0x0000001; //channels, type 1 = pcm
		fabuf[6]=samplerate;			    //sample rate
		fabuf[7]=samplerate*2*channels;	    //byte rate
		fabuf[8]=0x00100000 + (2*channels); //bits per mono sample, bytes per sample all channels
		fabuf[9]=0x61746164;  //"data"
		fabuf[10]=i-44;		  //sata size

		fseek(wavf,0,SEEK_SET);
		fwrite(fabuf,1,44,wavf);		 
		fclose(wavf);
	}
}
