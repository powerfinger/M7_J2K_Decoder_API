// TestApp.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <conio.h>
#include <io.h>
#include "toolsj2k.h"

#define ByteSwap(x) (((x<<24)&0xff000000)|((x<<8)&0xff0000)|((x>>8)&0xff00)|((x>>24)&0xff))

BRDCTXJ2K card;
unsigned char bufin[50*1024*1024];
int tiff_en;
int callbackcnt=0;
char filename[_MAX_PATH];
void tiff_write(char *filename, void *buf, int bitdepth, short nx, short ny);

void MyCallBack(int *buf)
{
//Save the last frame
	if (tiff_en) tiff_write(filename,buf,tiff_en,card.dimx,card.dimy);
	else {
		FILE *fid;
		fid = fopen(filename,"wb");
		if (fid==0) {
			printf("Could not open output file.\n");
		}
		else {
			fwrite(buf,1,card.bufsize,fid);
			fclose(fid);
		}

	}
	callbackcnt++;
}

int main(int argc, char* argv[])
{
	int length;
	FILE *fid;
	unsigned int postcnt=0;
	char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

	printf("\nJ2KDecode (Command Line decoder) Version 1.0 by Skymicro, Inc.\n");
	if ((argc != 11)&&(argc != 9)) {
		printf("\nusage:   J2KDecode -b# -bd# -cf# -bs# -xc# -yc# srcfile dstfile [dimx dimy]\n\n");
		printf("         -b#  - board number  - 1, 2, 3 or 4\n");
		printf("         -bd# - bit depth     - 8  = 8  bit\n");
		printf("                              - 10 = 10 bit\n");
		printf("                              - 12 = 12 bit\n");
		printf("                              - 16 = 16 bit\n");
		printf("         -cf# - color format  - 0  = 422\n");
		printf("                              - 1  = 444\n");
		printf("                              - 2  = 4444(ABGR)\n");
		printf("                              - 3  = 4444(ARGB)\n");
		printf("         -bs# - byte swap     - 0  = byte swap disabled\n");
		printf("                              - 1  = byte swap enabled\n");
		printf("         -xc# - XYZ convert   - 0  = RGB to XYZ convertion disabled\n");
		printf("                              - 1  = RGB to XYZ convertion enabled\n");
		printf("         -yc# - YCrCb convert - 0  = RGB to YCrCb convertion disabled\n");
		printf("                              - 1  = RGB to YCrCb convertion enabled\n");
		printf("         srcfile - Source J2K input file\n");
		printf("         dstfile - Destination Uncomrpressed ouput file\n");
		printf("                   if dstfile ends with .tif a TIFF file wrapper will be added\n");
		printf("         dimx    - Width  of image\n");
		printf("         dimy    - Height of image\n");
		printf("                   if dimx and dimy not specified they will be read from the J2K file\n\n");
		printf("Example: J2KDecode -b1 -bd16 -cf1 -bs0 -xc1 -yc0 c:\\infile c:\\outfile.tif 1920 1080\n\n");
		printf("         Will Convert: an XYZ J2K file to\n");
	    printf("                       an Uncompressed 16 bit RGB TIFF file\n");
	    printf("                       using board 1\n\n");
		return 0;
	}

	printf("\nConverting From: %s",    argv[7]);
	printf("\n           To  : %s\n\n",argv[8]);
	strcpy(filename,argv[8]);

//Get frame and image dimensions
	fid = fopen(argv[7],"rb");
	if (fid==0) {
		printf("Can't open %s\n",argv[7]);
		return 0;
	}

	if (argc == 9) {
		fread(&card.dimx,4,1,fid);
		fread(&card.dimx,4,1,fid);
		fread(&card.dimx,4,1,fid);
		fread(&card.dimy,4,1,fid);
		rewind(fid);
		card.dimx = ByteSwap(card.dimx);
		card.dimy = ByteSwap(card.dimy);
		printf("Image dimensions %dx%d\n",card.dimx,card.dimy);
	}
	else {
		sscanf(argv[9], "%d",&card.dimx);
		sscanf(argv[10],"%d",&card.dimy);
	}

//Set format bits
	sscanf(&argv[2][3],"%d",&card.bitdepth);
	if ((card.bitdepth!=8)&&(card.bitdepth!=10)&&(card.bitdepth!=12)&&(card.bitdepth!=16)) {
		printf("Unsupported bitdepth.\n");
		return 0;
	}
	sscanf(&argv[3][3],"%d",&card.colorformat);
	if ((card.colorformat!=0)&&(card.colorformat!=1)&&(card.colorformat!=2)&&(card.colorformat!=3)) {
		printf("Unsupported color format.\n");
		return 0;
	}
	sscanf(&argv[4][3],"%d",&card.byteswap);
	if ((card.byteswap!=0)&&(card.byteswap!=1)) {
		printf("Invalid byte swap parameter.\n");
		return 0;
	}
	sscanf(&argv[5][3],"%d",&card.xyzconvert);
	if ((card.xyzconvert!=0)&&(card.xyzconvert!=1)) {
		printf("Invalid XYZ convert parameter.\n");
		return 0;
	}
	sscanf(&argv[6][3],"%d",&card.yuvconvert);
	if ((card.yuvconvert!=0)&&(card.yuvconvert!=1)) {
		printf("Invalid YCrCb convert parameter.\n");
		return 0;
	}

	_splitpath(argv[8], drive, dir, fname, ext );
	tiff_en = 0;
	if (strcmp(ext,".tif")==0) {
		if (card.colorformat==1) {
			if      (card.bitdepth==8)  tiff_en = 8;
		    else if (card.bitdepth==16) tiff_en = 16;
			else {
				printf("TIFF fromat only supported for 8 or 16 bit depths.\n");
				return 0;
			}
		}
		else {
			printf("TIFF fromat only supported for 444 color format.\n");
			return 0;
		}
	}

//read the source file
	length = fread(bufin,1,50*1024*1024,fid);
	fclose(fid);

//Open the library
	if (OpenJ2K(&card,argv[1])==0) {
		printf("Could not initialize board.\n\n");
		return 0;
	}

//Register Call Back
	RegisterCallbackJ2K(&card,MyCallBack);

//Trim buffer to J2K end marker 0xffd9 (only needed if file is more than exactly 1 frame)
	for(int i=1; i<length; i++) if ((bufin[i-1]==0xff)&&(bufin[i]==0xd9)) length=i+1;

//Post frame
	DecodeJ2K (&card,bufin,length,NULL); postcnt++;

//Wait for frame
	while(callbackcnt<postcnt) Sleep(10);

//Close the library
	CloseJ2K(&card);

	return 0;
}

