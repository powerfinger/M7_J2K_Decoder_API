#include "stdafx.h"
#include <conio.h>
#include <io.h>

void tiff_read(char *filename, void *buf, short nx, short ny)
{
	int* bufi;
	short cnt;
	int foundit = 0;
	bufi=(int*)buf;
	FILE *fptr;
	fptr = fopen(filename,"rb");
	if (fptr==0) {
		printf("Could not open file.\n");
		printf("Could not determine file type.\n");
		fclose(fptr);
		return;
	}
	fread(bufi,4,2,fptr);
	if (bufi[0]!=0x2a4949) {
	}
	//printf("%08x %08x\n",bufi[0],bufi[1]);
	fseek(fptr,bufi[1],SEEK_SET);
	fread(&cnt,2,1,fptr);
	for (int i=0;i<cnt;i++) {
		fread(bufi,4,3,fptr);
		//printf("%08x %08x %08x\n",bufi[0],bufi[1],bufi[2]);
		if (bufi[0] == 0x00040111) {
			foundit = 1;
			break;
		}
	}
	if (foundit==0) {
		printf("Could not find image data.\n");
		fclose(fptr);
		return;
	}
	else {
		fseek(fptr,bufi[2],SEEK_SET);
		fread(buf,1,nx*ny*6,fptr);
		fclose(fptr);
		return;
	}
}

void tiff_write(char *filename, void *buf, int bitdepth, short nx, short ny) 
{
	/* Write the header */
	int offset;
	char bufc[12];
	FILE *fptr;
	fptr = fopen(filename,"wb");
	if (fptr==0) {
		printf("Could not open file.\n");
		fclose(fptr);
		return;
	}

	bufc[0]=0x49;
	bufc[1]=0x49;
	bufc[2]=0x2a;
	bufc[3]=0x00;    /* Little endian & TIFF identifier */
	fwrite(bufc,1,4,fptr);
	if (bitdepth==16) offset = nx * ny * 3 * 2 + 8;
	else              offset = nx * ny * 3 + 8;
	fwrite(&offset,1,4,fptr);

	/* Write the binary data */
	if (bitdepth==16) fwrite(buf,2,nx*ny*3,fptr);
	else              fwrite(buf,1,nx*ny*3,fptr);

	/* Write the footer */
	bufc[0]=0x0e;
	bufc[1]=0x00;  /* The number of directory entries (14) */
	fwrite(bufc,1,2,fptr);

	/* Width tag, short int */
	bufc[0]=0x00;
	bufc[1]=0x01;
	bufc[2]=0x03;
	bufc[3]=0x00;
	bufc[4]=0x01;
	bufc[5]=0x00;
	bufc[6]=0x00;
	bufc[7]=0x00;
	fwrite(bufc,1,8,fptr);
 	fwrite(&nx,1,2,fptr);
	bufc[0]= 0x00;
	bufc[1]= 0x00;
	fwrite(bufc,1,2,fptr);

	/* Height tag, short int */
	bufc[0]=0x01;
	bufc[1]=0x01;
	bufc[2]=0x03;
	bufc[3]=0x00;
	bufc[4]=0x01;
	bufc[5]=0x00;
	bufc[6]=0x00;
	bufc[7]=0x00;
	fwrite(bufc,1,8,fptr);
 	fwrite(&ny,1,2,fptr);
	bufc[0]=0x00;
	bufc[1]=0x00;
	fwrite(bufc,1,2,fptr);

   	/* Bits per sample tag, short int */
	bufc[0]=0x02;
	bufc[1]=0x01;
	bufc[2]=0x03;
	bufc[3]=0x00;
	bufc[4]=0x03;
	bufc[5]=0x00;
	bufc[6]=0x00;
	bufc[7]=0x00;
	fwrite(bufc,1,8,fptr);
 	if (bitdepth==16) offset = nx * ny * 3 * 2 + 182;
	else              offset = nx * ny * 3 + 182;
 	fwrite(&offset,1,4,fptr);

   	/* Compression flag, short int */
	bufc[0]=0x03;
	bufc[1]=0x01;
	bufc[2]=0x03;
	bufc[3]=0x00;
	bufc[4]=0x01;
	bufc[5]=0x00;
	bufc[6]=0x00;
	bufc[7]=0x00;
	bufc[8]=0x01;
	bufc[9]=0x00;
	bufc[10]=0x00;
	bufc[11]=0x00;
	fwrite(bufc,1,12,fptr);

   	/* Photometric interpolation tag, short int */
	bufc[0]=0x06;
	bufc[1]=0x01;
	bufc[2]=0x03;
	bufc[3]=0x00;
	bufc[4]=0x01;
	bufc[5]=0x00;
	bufc[6]=0x00;
	bufc[7]=0x00;
	bufc[8]=0x02;
	bufc[9]=0x00;
	bufc[10]=0x00;
	bufc[11]=0x00;
	fwrite(bufc,1,12,fptr);

   	/* Strip offset tag, long int */
	bufc[0]=0x11;
	bufc[1]=0x01;
	bufc[2]=0x04;
	bufc[3]=0x00;
	bufc[4]=0x01;
	bufc[5]=0x00;
	bufc[6]=0x00;
	bufc[7]=0x00;
	bufc[8]=0x08;
	bufc[9]=0x00;
	bufc[10]=0x00;
	bufc[11]=0x00;
	fwrite(bufc,1,12,fptr);

   	/* Orientation flag, short int */
	bufc[0]=0x12;
	bufc[1]=0x01;
	bufc[2]=0x03;
	bufc[3]=0x00;
	bufc[4]=0x01;
	bufc[5]=0x00;
	bufc[6]=0x00;
	bufc[7]=0x00;
	bufc[8]=0x01;
	bufc[9]=0x00;
	bufc[10]=0x00;
	bufc[11]=0x00;
	fwrite(bufc,1,12,fptr);

   	/* Sample per pixel tag, short int */
	bufc[0]=0x15;
	bufc[1]=0x01;
	bufc[2]=0x03;
	bufc[3]=0x00;
	bufc[4]=0x01;
	bufc[5]=0x00;
	bufc[6]=0x00;
	bufc[7]=0x00;
	bufc[8]=0x03;
	bufc[9]=0x00;
	bufc[10]=0x00;
	bufc[11]=0x00;
	fwrite(bufc,1,12,fptr);

   	/* Rows per strip tag, short int */
	bufc[0]=0x16;
	bufc[1]=0x01;
	bufc[2]=0x03;
	bufc[3]=0x00;
	bufc[4]=0x01;
	bufc[5]=0x00;
	bufc[6]=0x00;
	bufc[7]=0x00;
	fwrite(bufc,1,8,fptr);
	fwrite(&ny,1,2,fptr);
	bufc[0]=0x00;
	bufc[1]=0x00;
	fwrite(bufc,1,2,fptr);

   	/* Strip byte count flag, long int */
	bufc[0]=0x17;
	bufc[1]=0x01;
	bufc[2]=0x04;
	bufc[3]=0x00;
	bufc[4]=0x01;
	bufc[5]=0x00;
	bufc[6]=0x00;
	bufc[7]=0x00;
	fwrite(bufc,1,8,fptr);
	if (bitdepth==16) offset = nx * ny * 3 * 2;
	else              offset = nx * ny * 3;
	fwrite(&offset,1,4,fptr);

   	/* Minimum sample value flag, short int */
	bufc[0]=0x18;
	bufc[1]=0x01;
	bufc[2]=0x03;
	bufc[3]=0x00;
	bufc[4]=0x03;
	bufc[5]=0x00;
	bufc[6]=0x00;
	bufc[7]=0x00;
	fwrite(bufc,1,8,fptr);
	if (bitdepth==16) offset = nx * ny * 3 * 2 + 188;
	else              offset = nx * ny * 3 + 188;
	fwrite(&offset,1,4,fptr);

   	/* Maximum sample value tag, short int */
	bufc[0]=0x19;
	bufc[1]=0x01;
	bufc[2]=0x03;
	bufc[3]=0x00;
	bufc[4]=0x03;
	bufc[5]=0x00;
	bufc[6]=0x00;
	bufc[7]=0x00;
	fwrite(bufc,1,8,fptr);
	if (bitdepth==16) offset = nx * ny * 3 * 2 + 194;
	else              offset = nx * ny * 3 + 194;
	fwrite(&offset,1,4,fptr);

   	/* Planar configuration tag, short int */
	bufc[0]=0x1c;
	bufc[1]=0x01;
	bufc[2]=0x03;
	bufc[3]=0x00;
	bufc[4]=0x01;
	bufc[5]=0x00;
	bufc[6]=0x00;
	bufc[7]=0x00;
	bufc[8]=0x01;
	bufc[9]=0x00;
	bufc[10]=0x00;
	bufc[11]=0x00;
	fwrite(bufc,1,12,fptr);

   	/* Sample format tag, short int */
	bufc[0]=0x53;
	bufc[1]=0x01;
	bufc[2]=0x03;
	bufc[3]=0x00;
	bufc[4]=0x03;
	bufc[5]=0x00;
	bufc[6]=0x00;
	bufc[7]=0x00;
	fwrite(bufc,1,8,fptr);
	if (bitdepth==16) offset = nx * ny * 3 * 2 + 200;
	else              offset = nx * ny * 3 + 200;
	fwrite(&offset,1,4,fptr);

   	/* End of the directory entry */
	bufc[0]=0x00;
	bufc[1]=0x00;
	bufc[2]=0x00;
	bufc[3]=0x00;
	fwrite(bufc,1,4,fptr);

   	/* Bits for each colour channel */
	if (bitdepth==16) bufc[0]=0x10;
	else              bufc[0]=0x08;
	bufc[1]=0x00;
	if (bitdepth==16) bufc[2]=0x10;
	else              bufc[2]=0x08;
	bufc[3]=0x00;
	if (bitdepth==16) bufc[4]=0x10;
	else              bufc[4]=0x08;
	bufc[5]=0x00;
	fwrite(bufc,1,6,fptr);

   	/* Minimum value for each component */
	bufc[0]=0x00;
	bufc[1]=0x00;
	bufc[2]=0x00;
	bufc[3]=0x00;
	bufc[4]=0x00;
	bufc[5]=0x00;
	fwrite(bufc,1,6,fptr);

   	/* Maximum value per channel */
	if (bitdepth==16) bufc[0]=0xff;
	else              bufc[0]=0;
	bufc[1]=0xff;
	if (bitdepth==16) bufc[2]=0xff;
	else              bufc[2]=0;
	bufc[3]=0xff;
	if (bitdepth==16) bufc[4]=0xff;
	else              bufc[4]=0;
	bufc[5]=0xff;
	fwrite(bufc,1,6,fptr);

   	/* Samples per pixel for each channel */
	bufc[0]=0x01;
	bufc[1]=0x00;
	bufc[2]=0x01;
	bufc[3]=0x00;
	bufc[4]=0x01;
	bufc[5]=0x00;
	fwrite(bufc,1,6,fptr);

	fclose(fptr);
}

