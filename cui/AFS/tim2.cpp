/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.7
 */
/*
 *                      ATOK Library Sample
 *
 *                         Version 0.10
 *                           Shift-JIS
 *
 *      Copyright (C) 2002 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *      0.10
 */
#include <stdio.h>

// TIM2�ե�����إå��Θ�����O��

#ifdef R5900

// PS2�h���ΤȤ�
#include <eekernel.h>
#include <sifdev.h>
#include <libgraph.h>
#include "tim2.h"

// �ץ��ȥ���������
static void Tim2LoadTexture(int psm, u_int tbp, int tbw, int sx, int sy, u_long128 *pImage);
static int  Tim2CalcBufWidth(int psm, int w);
static int  Tim2CalcBufSize(int psm, int w, int h);
static int  Tim2GetLog2(int n);

#else	// R5900

// ��PS2�h���ΤȤ�

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable: 4200)
#endif	// WIN32
#include "tim2.h"
#ifdef WIN32
#pragma warning(pop)
#endif	// WIN32

#endif	// R5900



// TIM2�ե�����Υե�����إå�������å�����
// ����
// pTim2    TIM2��ʽ�Υǩ`�������^���ɥ쥹
// ���ꂎ
//          0�ΤȤ�����`
//          1�ΤȤ������K��(TIM2)
//          2�ΤȤ������K��(CLUT2)
int Tim2CheckFileHeaer(void *pTim2)
{
	TIM2_FILEHEADER *pFileHdr = (TIM2_FILEHEADER *)pTim2;
	int i;

	// TIM2�����ͥ��������å�
	if(pFileHdr->FileId[0]=='T' || pFileHdr->FileId[1]=='I' || pFileHdr->FileId[2]=='M' || pFileHdr->FileId[3]=='2') {
		// TIM2���ä��Ȥ�
		i = 1;
	} else if(pFileHdr->FileId[0]=='C' || pFileHdr->FileId[1]=='L' || pFileHdr->FileId[2]=='T' || pFileHdr->FileId[3]=='2') {
		// CLUT2���ä��Ȥ�
		i = 2;
	} else {
		// ����`������R�e���֤��ä��Ȥ�
		printf("Tim2CheckFileHeaer: TIM2 is broken %02X,%02X,%02X,%02X\n",
					pFileHdr->FileId[0], pFileHdr->FileId[1], pFileHdr->FileId[2], pFileHdr->FileId[3]);
		return(0);
	}

	// TIM2�ե�����ե��`�ޥåȥЩ`�����,�ե��`�ޥå�ID������å�
	if(!(pFileHdr->FormatVersion==0x03 ||
	    (pFileHdr->FormatVersion==0x04 && (pFileHdr->FormatId==0x00 || pFileHdr->FormatId==0x01)))) {
		printf("Tim2CheckFileHeaer: TIM2 is broken (2)\n");
		return(0);
	}
	return(i);
}



// ָ�����줿���ŤΥԥ�����إå���ä�
// ����
// pTim2    TIM2��ʽ�Υǩ`�������^���ɥ쥹
// imgno    �η�Ŀ�Υԥ�����إå�����դ��뤫ָ��
// ���ꂎ
//          �ԥ�����إå��ؤΥݥ���
TIM2_PICTUREHEADER *Tim2GetPictureHeader(void *pTim2, int imgno)
{
	TIM2_FILEHEADER *pFileHdr = (TIM2_FILEHEADER *)pTim2;
	TIM2_PICTUREHEADER *pPictHdr;
	int i;

	// �ԥ����㷬�Ť�����å�
	if(imgno>=pFileHdr->Pictures) {
		printf("Tim2GetPictureHeader: Illegal image no.(%d)\n", imgno);
		return(NULL);
	}

	if(pFileHdr->FormatId==0x00) {
		// �ե��`�ޥå�ID��0x00�ΤȤ���16�Х��ȥ��饤�����
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pTim2 + sizeof(TIM2_FILEHEADER));
	} else {
		// �ե��`�ޥå�ID��0x01�ΤȤ���128�Х��ȥ��饤�����
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pTim2 + 0x80);
	}

	// ָ�����줿�ԥ����㷬�Ťޤǥ����å�
	for(i=0; i<imgno; i++) {
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pPictHdr + pPictHdr->TotalSize);
	}
	return(pPictHdr);
}


// �ԥ�����ǩ`����CLUT2�ǩ`���Ǥ��뤫�ɤ����Єe
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// ���ꂎ
//          0�ΤȤ�TIM2
//          1�ΤȤ�CLUT2
int Tim2IsClut2(TIM2_PICTUREHEADER *ph)
{
	// �ԥ�����إå���MipMapTextures���Ф��Єe
	if(ph->MipMapTextures==0) {
		// �ƥ�������ö����0�ΤȤ���CLUT2�ǩ`��
		return(1);
	} else {
		// �ƥ�������ö����1ö���ϤΤȤ���TIM2�ǩ`��
		return(0);
	}
}


// MIPMAP��٥뤴�ȤΥƥ������㥵������ä�
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// mipmap:  MIPMAP�ƥ��������٥�(��������0-ԭʼ��С)
// pWidth:  X���������ܤ�ȡ�뤿���int�͉����إݥ���
// pHeight: Y���������ܤ�ȡ�뤿���int�͉����إݥ���
// ���ꂎ
//          MIPMAP�ƥ�������Υ�����
int Tim2GetMipMapPictureSize(TIM2_PICTUREHEADER *ph, int mipmap, int *pWidth, int *pHeight)
{
	int w, h, n;
	w = ph->ImageWidth>>mipmap;
	h = ph->ImageHeight>>mipmap;
	if(pWidth) {
		*pWidth  = w;
	}
	if(pHeight) {
		*pHeight = h;
	}

	n = w * h;
	switch(ph->ImageType) {
		case TIM2_RGB16:	n *= 2;		break;
		case TIM2_RGB24:	n *= 3;		break;
		case TIM2_RGB32:	n *= 4;		break;
		case TIM2_IDTEX4:	n /= 2;		break;
		case TIM2_IDTEX8:				break;
	}

	// MIPMAP�ƥ������㥵�����ϥե�����إå���FormatId��ָ���ˤ�����餺��
	// ����16�Х��ȥ��饤����Ⱦ�������Ф����
	n = (n + 15) & ~15;
	return(n);
}


// MIPMAP�إå��Υ��ɥ쥹,��������ä�
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// pSize:   MIPMAP�إå��Υ��������ܤ�ȡ�뤿���int�͉����إݥ���
//          ����������Ҫ�Τʤ��Ȥ���NULL��
// ���ꂎ
//          NULL�ΤȤ�MIPMAP�إå��ʤ�
//          NULL�Ǥʤ��Ȥ���MIPMAP�إå������^���ɥ쥹
TIM2_MIPMAPHEADER *Tim2GetMipMapHeader(TIM2_PICTUREHEADER *ph, int *pSize)
{
	TIM2_MIPMAPHEADER *pMmHdr;

	static const char mmsize[] = {
		 0,  // �ƥ�������0ö(CLUT2�ǩ`���ΤȤ�)
		 0,  // LV0�ƥ�������Τ�(MIPMAP�إå��ʤ�)
		32,  // LV1 MipMap�ޤ�
		32,  // LV2 MipMap�ޤ�
		32,  // LV3 MipMap�ޤ�
		48,  // LV4 MipMap�ޤ�
		48,  // LV5 MipMap�ޤ�
		48   // LV6 MipMap�ޤ�
	};

	if(ph->MipMapTextures>1) {
		pMmHdr = (TIM2_MIPMAPHEADER *)((char *)ph + sizeof(TIM2_PICTUREHEADER));
	} else {
		pMmHdr = NULL;
	}

	if(pSize) {
		// �����إå����ʤ��ä����ϡ�
		*pSize = mmsize[ph->MipMapTextures];
	}
	return(pMmHdr);
}


// ��`���`���ک`���Υ��ɥ쥹,��������ä�
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// pSize:   ��`���`���ک`���Υ��������ܤ�ȡ�뤿���int�͉����إݥ���
//          ����������Ҫ�Τʤ��Ȥ���NULL��
// ���ꂎ
//          NULL�ΤȤ���`���`���ک`���ʤ�
//          NULL�Ǥʤ��Ȥ�����`���`���ک`�������^���ɥ쥹
void *Tim2GetUserSpace(TIM2_PICTUREHEADER *ph, int *pSize)
{
	void *pUserSpace;

	static const char mmsize[] = {
		sizeof(TIM2_PICTUREHEADER)     ,	// �ƥ�������0ö(CLUT2�ǩ`���ΤȤ�)
		sizeof(TIM2_PICTUREHEADER)     ,	// LV0�ƥ�������Τ�(MIPMAP�إå��ʤ�)
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV1 MipMap�ޤ�
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV2 MipMap�ޤ�
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV3 MipMap�ޤ�
		sizeof(TIM2_PICTUREHEADER) + 48,	// LV4 MipMap�ޤ�
		sizeof(TIM2_PICTUREHEADER) + 48,	// LV5 MipMap�ޤ�
		sizeof(TIM2_PICTUREHEADER) + 48 	// LV6 MipMap�ޤ�
	};

	// �إå����������{�٤�
	if(ph->HeaderSize==mmsize[ph->MipMapTextures]) {
		// ��`���`���ک`�������ڤ��ʤ��Ȥ�
		if(pSize) *pSize = 0;
		return(NULL);
	}

	// ��`���`���ک`�������ڤ���Ȥ�
	pUserSpace = (void *)((char *)ph + mmsize[ph->MipMapTextures]);
	if(pSize) {
		// ��`���`���ک`���Υ�������Ӌ��
		// �����إå���������Ϥϡ������餫��ȩ`���륵������ȡ��
		TIM2_EXHEADER *pExHdr;

		pExHdr = (TIM2_EXHEADER *)pUserSpace;
		if(pExHdr->ExHeaderId[0]!='e' ||
			pExHdr->ExHeaderId[1]!='X' ||
			pExHdr->ExHeaderId[2]!='t' ||
			pExHdr->ExHeaderId[3]!=0x00) {

			// �����إå����ʤ��ä����ϡ�
			*pSize = (ph->HeaderSize - mmsize[ph->MipMapTextures]);
		} else {
			// �����إå������ä�����
			*pSize = pExHdr->UserSpaceSize;
		}
	}
	return(pUserSpace);
}


// ��`���`�ǩ`���Υ��ɥ쥹,��������ä�
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// pSize:   ��`���`�ǩ`���Υ��������ܤ�ȡ�뤿���int�͉����إݥ���
//          ����������Ҫ�Τʤ��Ȥ���NULL��
// ���ꂎ
//          NULL�ΤȤ���`���`�ǩ`���ʤ�
//          NULL�Ǥʤ��Ȥ�����`���`�ǩ`�������^���ɥ쥹
void *Tim2GetUserData(TIM2_PICTUREHEADER *ph, int *pSize)
{
	void *pUserSpace;
	TIM2_EXHEADER *pExHdr;

	pUserSpace = Tim2GetUserSpace(ph, pSize);
	if(pUserSpace==NULL) {
		// ��`���`���ک`�������ڤ��ʤ��ä��Ȥ�
		return(NULL);
	}

	// ��`���`���ک`���˒����إå������뤫�ɤ��������å�
	pExHdr = (TIM2_EXHEADER *)pUserSpace;
	if(pExHdr->ExHeaderId[0]!='e' ||
		pExHdr->ExHeaderId[1]!='X' ||
		pExHdr->ExHeaderId[2]!='t' ||
		pExHdr->ExHeaderId[3]!=0x00) {

		// �����إå���Ҋ�Ĥ���ʤ��ä�����
		return(pUserSpace);
	}

	// �����إå���Ҋ�Ĥ��ä�����
	if(pSize) {
		// ��`���`�ǩ`�����֤Υ������򷵤�
		*pSize = pExHdr->UserDataSize;
	}
	return((char *)pUserSpace + sizeof(TIM2_EXHEADER));
}


// ��`���`���ک`���˸�{���줿�����������Ф����^���ɥ쥹��ä�
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// ���ꂎ
//          NULL�ΤȤ������������Фʤ�
//          NULL�Ǥʤ��Ȥ���������������(ASCIZ)�����^���ɥ쥹
char *Tim2GetComment(TIM2_PICTUREHEADER *ph)
{
	void *pUserSpace;
	TIM2_EXHEADER *pExHdr;

	pUserSpace = Tim2GetUserSpace(ph, NULL);
	if(pUserSpace==NULL) {
		// ��`���`���ک`�������ڤ��ʤ��ä��Ȥ�
		return(NULL);
	}

	// ��`���`���ک`���˒����إå������뤫�ɤ��������å�
	pExHdr = (TIM2_EXHEADER *)pUserSpace;
	if(pExHdr->ExHeaderId[0]!='e' ||
		pExHdr->ExHeaderId[1]!='X' ||
		pExHdr->ExHeaderId[2]!='t' ||
		pExHdr->ExHeaderId[3]!=0x00) {

		// �����إå���Ҋ�Ĥ���ʤ��ä�����
		return(NULL);
	}

	// �����إå����ڤ��Ƥ����Ȥ�
	if(pExHdr->UserSpaceSize==((sizeof(TIM2_EXHEADER) + pExHdr->UserDataSize))) {
		// ��`���`���ک`��������ʥ��������������إå��ȥ�`���`�ǩ`���κ�Ӌ�������˵Ȥ����ä��Ȥ�
		// �����������Фϴ��ڤ��ʤ����жϤǤ���
		return(NULL);
	}

	// �����Ȥ�Ҋ�Ĥ��ä�����
	return((char *)pUserSpace + sizeof(TIM2_EXHEADER) + pExHdr->UserDataSize);
}



// ָ������MIPMAP��٥�Υ���`���ǩ`�������^���ɥ쥹��ä�
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// mipmap:  MIPMAP�ƥ��������٥�
// ���ꂎ
//          NULL�ΤȤ���ԓ�����륤��`���ǩ`���ʤ�
//          NULL�Ǥʤ��Ȥ�������`���ǩ`�������^���ɥ쥹
void *Tim2GetImage(TIM2_PICTUREHEADER *ph, int mipmap)
{
	void *pImage;
	TIM2_MIPMAPHEADER *pm;
	int i;

	if(mipmap>=ph->MipMapTextures) {
		// ָ�����줿��٥��MIPMAP�ƥ�������ϴ��ڤ��ʤ�
		return(NULL);
	}

	// ����`���ǩ`�������^���ɥ쥹��Ӌ��
	pImage = (void *)((char *)ph + ph->HeaderSize);
	if(ph->MipMapTextures==1) {
		// LV0�ƥ�������ΤߤΈ���
		return(pImage);
	}

	// MIPMAP�ƥ������㤬���ڤ��Ƥ������
	pm = (TIM2_MIPMAPHEADER *)((char *)ph + sizeof(TIM2_PICTUREHEADER));
	for(i=0; i<mipmap; i++) {
		pImage = (void *)((char *)pImage + pm->MMImageSize[i]);
	}
	return(pImage);
}


// CLUT�ǩ`�������^���ɥ쥹��ä�
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// ���ꂎ
//          NULL�ΤȤ���ԓ������CLUT�ǩ`���ʤ�
//          NULL�Ǥʤ��Ȥ���CLUT�ǩ`�������^���ɥ쥹
void *Tim2GetClut(TIM2_PICTUREHEADER *ph)
{
	void *pClut;
	if(ph->ClutColors==0) {
		// CLUT�ǩ`�����򘋳ɤ���ɫ����0�ΤȤ�
		pClut = NULL;
	} else {
		// CLUT�ǩ`�������ڤ���Ȥ�
		pClut = (void *)((char *)ph + ph->HeaderSize + ph->ImageSize);
	}
	return(pClut);
}


// CLUT����`��ä�
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// clut:    CLUT���åȤ�ָ��
// no:      �η�Ŀ�Υ���ǥ�����ȡ�ä��뤫ָ��
// ���ꂎ
//          RGBA32�ե��`�ޥåȤ�ɫ�򷵤�
//          clut,no�Ȥ�ָ���˥���`������Ȥ���0x00000000(�\)�򷵤�
unsigned int Tim2GetClutColor(TIM2_PICTUREHEADER *ph, int clut, int no)
{
	unsigned char *pClut;
	int n;
	unsigned char r, g, b, a;

	pClut = (unsigned char *)Tim2GetClut(ph);
	if(pClut==NULL) {
		// CLUT�ǩ`�����ʤ��ä��Ȥ�
		return(0);
	}

	// �ޤ����η�Ŀ��ɫ�ǩ`����Ӌ��
	switch(ph->ImageType) {
		case TIM2_IDTEX4:	n = clut*16 + no;	break;
		case TIM2_IDTEX8:	n = clut*256 + no;	break;
		default:         	return(0);    // �����ʥԥ����륫��`�ΤȤ�
	}
	if(n>ph->ClutColors) {
		// ָ�����줿CLUT����,����ǥ�����ɫ�ǩ`�������ڤ��ʤ��ä��Ȥ�
		return(0);
	}

	// CLUT���Υե��`�ޥåȤˤ�äƤϡ����ä��K���椨���Ƥ�������Ԥ�����
	switch((ph->ClutType<<8) | ph->ImageType) {
		case (((TIM2_RGB16 | 0x40)<<8) | TIM2_IDTEX4): // 16ɫ,CSM1,16�ӥå�,�K���椨����
		case (((TIM2_RGB24 | 0x40)<<8) | TIM2_IDTEX4): // 16ɫ,CSM1,24�ӥå�,�K���椨����
		case (((TIM2_RGB32 | 0x40)<<8) | TIM2_IDTEX4): // 16ɫ,CSM1,32�ӥå�,�K���椨����
		case (( TIM2_RGB16        <<8) | TIM2_IDTEX8): // 256ɫ,CSM1,16�ӥå�,�K���椨����
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX8): // 256ɫ,CSM1,24�ӥå�,�K���椨����
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX8): // 256ɫ,CSM1,32�ӥå�,�K���椨����
			if((n & 31)>=8) {
				if((n & 31)<16) {
					n += 8;    // +8��15��+16��23��
				} else if((n & 31)<24) {
					n -= 8;    // +16��23��+8��15��
				}
			}
			break;
	}

	// CLUT���Υԥ�����ե��`�ޥåȤ˻��Ť��ơ�ɫ�ǩ`����ä�
	switch(ph->ClutType & 0x3F) {
		case TIM2_RGB16:
			// 16bit����`�ΤȤ�
			r = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])<<3) & 0xF8);
			g = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>2) & 0xF8);
			b = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>7) & 0xF8);
			a = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>8) & 0x80);
			break;

		case TIM2_RGB24:
			// 24bit����`�ΤȤ�
			r = pClut[n*3];
			g = pClut[n*3 + 1];
			b = pClut[n*3 + 2];
			a = 0x80;
			break;

		case TIM2_RGB32:
			// 32bit����`�ΤȤ�
			r = pClut[n*4];
			g = pClut[n*4 + 1];
			b = pClut[n*4 + 2];
			a = pClut[n*4 + 3];
			break;

		default:
			// �����ʥԥ�����ե��`�ޥåȤΈ���
			r = 0;
			g = 0;
			b = 0;
			a = 0;
			break;
	}
	return((a<<24) | (g<<16) | (b<<8) | r);
}


// CLUT����`���O������
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// clut:    CLUT���åȤ�ָ��
// no:      �η�Ŀ�Υ���ǥ������O�����뤫ָ��
// color:   �O������ɫ(RGB32�ե��`�ޥå�)
// ���ꂎ
//          RGBA32�ե��`�ޥåȤǹŤ�ɫ�򷵤�
//          clut,no�Ȥ�ָ���˥���`������Ȥ���0x00000000(�\)�򷵤�
unsigned int Tim2SetClutColor(TIM2_PICTUREHEADER *ph, int clut, int no, unsigned int newcolor)
{
	unsigned char *pClut;
	unsigned char r, g, b, a;
	int n;

	pClut = (unsigned char *)Tim2GetClut(ph);
	if(pClut==NULL) {
		// CLUT�ǩ`�����ʤ��ä��Ȥ�
		return(0);
	}

	// �ޤ����η�Ŀ��ɫ�ǩ`����Ӌ��
	switch(ph->ImageType) {
		case TIM2_IDTEX4:	n = clut*16 + no;	break;
		case TIM2_IDTEX8:	n = clut*256 + no;	break;
		default:         	return(0);    // �����ʥԥ����륫��`�ΤȤ�
	}
	if(n>ph->ClutColors) {
		// ָ�����줿CLUT����,����ǥ�����ɫ�ǩ`�������ڤ��ʤ��ä��Ȥ�
		return(0);
	}

	// CLUT���Υե��`�ޥåȤˤ�äƤϡ����ä��K���椨���Ƥ�������Ԥ�����
	switch((ph->ClutType<<8) | ph->ImageType) {
		case (((TIM2_RGB16 | 0x40)<<8) | TIM2_IDTEX4): // 16ɫ,CSM1,16�ӥå�,�K���椨����
		case (((TIM2_RGB24 | 0x40)<<8) | TIM2_IDTEX4): // 16ɫ,CSM1,24�ӥå�,�K���椨����
		case (((TIM2_RGB32 | 0x40)<<8) | TIM2_IDTEX4): // 16ɫ,CSM1,32�ӥå�,�K���椨����
		case (( TIM2_RGB16        <<8) | TIM2_IDTEX8): // 256ɫ,CSM1,16�ӥå�,�K���椨����
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX8): // 256ɫ,CSM1,24�ӥå�,�K���椨����
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX8): // 256ɫ,CSM1,32�ӥå�,�K���椨����
			if((n & 31)>=8) {
				if((n & 31)<16) {
					n += 8;    // +8��15��+16��23��
				} else if((n & 31)<24) {
					n -= 8;    // +16��23��+8��15��
				}
			}
			break;
	}

	// CLUT���Υԥ�����ե��`�ޥåȤ˻��Ť��ơ�ɫ�ǩ`����ä�
	switch(ph->ClutType & 0x3F) {
		case TIM2_RGB16:
			// 16bit����`�ΤȤ�
			{
				unsigned char rr, gg, bb, aa;
				r = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])<<3) & 0xF8);
				g = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>2) & 0xF8);
				b = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>7) & 0xF8);
				a = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>8) & 0x80);

				rr = (unsigned char)((newcolor>>3)  & 0x1F);
				gg = (unsigned char)((newcolor>>11) & 0x1F);
				bb = (unsigned char)((newcolor>>19) & 0x1F);
				aa = (unsigned char)((newcolor>>31) & 1);

				pClut[n*2]     = (unsigned char)((((aa<<15) | (bb<<10) | (gg<<5) | rr))    & 0xFF);
				pClut[n*2 + 1] = (unsigned char)((((aa<<15) | (bb<<10) | (gg<<5) | rr)>>8) & 0xFF);
			}
			break;

		case TIM2_RGB24:
			// 24bit����`�ΤȤ�
			r = pClut[n*3];
			g = pClut[n*3 + 1];
			b = pClut[n*3 + 2];
			a = 0x80;
			pClut[n*3]     = (unsigned char)((newcolor)     & 0xFF);
			pClut[n*3 + 1] = (unsigned char)((newcolor>>8)  & 0xFF);
			pClut[n*3 + 2] = (unsigned char)((newcolor>>16) & 0xFF);
			break;

		case TIM2_RGB32:
			// 32bit����`�ΤȤ�
			r = pClut[n*4];
			g = pClut[n*4 + 1];
			b = pClut[n*4 + 2];
			a = pClut[n*4 + 3];
			pClut[n*4]     = (unsigned char)((newcolor)     & 0xFF);
			pClut[n*4 + 1] = (unsigned char)((newcolor>>8)  & 0xFF);
			pClut[n*4 + 2] = (unsigned char)((newcolor>>16) & 0xFF);
			pClut[n*4 + 3] = (unsigned char)((newcolor>>24) & 0xFF);
			break;

		default:
			// �����ʥԥ�����ե��`�ޥåȤΈ���
			r = 0;
			g = 0;
			b = 0;
			a = 0;
			break;
	}
	return((a<<24) | (g<<16) | (b<<8) | r);
}


// �ƥ�����(ɫ���Ȥ��ޤ�ʤ�)�ǩ`����ä�
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// mipmap:  MIPMAP�ƥ��������٥�
// x:       �ƥ�����ǩ`����ä�ƥ�����X����
// y:       �ƥ�����ǩ`����ä�ƥ�����Y����
// ���ꂎ
//          ɫ���(4bit�ޤ���8bit�Υ���ǥ������š��ޤ���16bit,24bit,32bit�Υ����쥯�ȥ���`)
unsigned int Tim2GetTexel(TIM2_PICTUREHEADER *ph, int mipmap, int x, int y)
{
	unsigned char *pImage;
	int t;
	int w, h;

	pImage = (unsigned char *)Tim2GetImage(ph, mipmap);
	if(pImage==NULL) {
		// ָ����٥�Υƥ�������ǩ`�����ʤ��ä�����
		return(0);
	}
	Tim2GetMipMapPictureSize(ph, mipmap, &w, &h);
	if(x>w || y>h) {
		// �ƥ��������ˤ������ʤȤ�
		return(0);
	}

	t = y*w + x;
	switch(ph->ImageType) {
		case TIM2_RGB16:
			return((pImage[t*2 + 1]<<8) | pImage[t*2]);

		case TIM2_RGB24:
			return((pImage[t*3 + 2]<<16) | (pImage[t*3 + 1]<<8) | pImage[t*3]);

		case TIM2_RGB32:
			return((pImage[t*4 + 3]<<24) | (pImage[t*4 + 2]<<16) | (pImage[t*4 + 1]<<8) | pImage[t*4]);

		case TIM2_IDTEX4:
			if(x & 1) {
				return(pImage[t/2]>>4);
			} else {
				return(pImage[t/2] & 0x0F);
			}
		case TIM2_IDTEX8:
			return(pImage[t]);
	}

	// �����ʥԥ�����ե��`�ޥåȤ��ä�����
	return(0);
}



// �ƥ�����(ɫ���Ȥ��ޤ�ʤ�)�ǩ`�����O������
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// mipmap:  MIPMAP�ƥ��������٥�
// x:       �ƥ�����ǩ`����ä�ƥ�����X����
// y:       �ƥ�����ǩ`����ä�ƥ�����Y����
// ���ꂎ
//          ɫ���(4bit�ޤ���8bit�Υ���ǥ������š��ޤ���16bit,24bit,32bit�Υ����쥯�ȥ���`)
unsigned int Tim2SetTexel(TIM2_PICTUREHEADER *ph, int mipmap, int x, int y, unsigned int newtexel)
{
	unsigned char *pImage;
	int t;
	int w, h;
	unsigned int oldtexel;

	pImage = (unsigned char *)Tim2GetImage(ph, mipmap);
	if(pImage==NULL) {
		// ָ����٥�Υƥ�������ǩ`�����ʤ��ä�����
		return(0);
	}
	Tim2GetMipMapPictureSize(ph, mipmap, &w, &h);
	if(x>w || y>h) {
		// �ƥ��������ˤ������ʤȤ�
		return(0);
	}

	t = y*w + x;
	switch(ph->ImageType) {
		case TIM2_RGB16:
			oldtexel = (pImage[t*2 + 1]<<8) | pImage[t*2];
			pImage[t*2]     = (unsigned char)((newtexel)    & 0xFF);
			pImage[t*2 + 1] = (unsigned char)((newtexel>>8) & 0xFF);
			return(oldtexel);

		case TIM2_RGB24:
			oldtexel = (pImage[t*3 + 2]<<16) | (pImage[t*3 + 1]<<8) | pImage[t*3];
			pImage[t*3]     = (unsigned char)((newtexel)     & 0xFF);
			pImage[t*3 + 1] = (unsigned char)((newtexel>>8)  & 0xFF);
			pImage[t*3 + 2] = (unsigned char)((newtexel>>16) & 0xFF);
			return(oldtexel);

		case TIM2_RGB32:
			oldtexel = (pImage[t*4 + 3]<<24) | (pImage[t*4 + 2]<<16) | (pImage[t*4 + 1]<<8) | pImage[t*4];
			pImage[t*4]     = (unsigned char)((newtexel)     & 0xFF);
			pImage[t*4 + 1] = (unsigned char)((newtexel>>8)  & 0xFF);
			pImage[t*4 + 2] = (unsigned char)((newtexel>>16) & 0xFF);
			pImage[t*4 + 3] = (unsigned char)((newtexel>>24) & 0xFF);
			return(oldtexel);

		case TIM2_IDTEX4:
			if(x & 1) {
				oldtexel = pImage[t/2]>>4;
				pImage[t/2] = (unsigned char)((newtexel<<4) | oldtexel);
			} else {
				oldtexel = pImage[t/2] & 0x0F;
				pImage[t/2] = (unsigned char)((oldtexel<<4) | newtexel);
			}
			return(oldtexel);
		case TIM2_IDTEX8:
			oldtexel = pImage[t];
			pImage[t] = (unsigned char)newtexel;
			return(oldtexel);
	}

	// �����ʥԥ�����ե��`�ޥåȤ��ä�����
	return(0);
}


// �ƥ������㥫��`��ä�
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// mipmap:  MIPMAP�ƥ��������٥�
// clut:    ����ǥ�������`�Ή�Q���ä���CLUT���åȷ���
// x:       �ƥ�����ǩ`����ä�ƥ�����X����
// y:       �ƥ�����ǩ`����ä�ƥ�����Y����
// ���ꂎ
//          RGBA32�ե��`�ޥåȤ�ɫ�򷵤�
//          clut��ָ���˥���`������Ȥ���0x00000000(�\)�򷵤�
//          x,y��ָ���˥���`�����ä��Ȥ��΄����ϱ��^���ʤ�
unsigned int Tim2GetTextureColor(TIM2_PICTUREHEADER *ph, int mipmap, int clut, int x, int y)
{
	unsigned int t;
	if(Tim2GetImage(ph, mipmap)==NULL) {
		// ָ����٥�Υƥ�������ǩ`�����ʤ��ä�����
		return(0);
	}
	t = Tim2GetTexel(ph, mipmap, (x>>mipmap), (y>>mipmap));
	switch(ph->ImageType) {
		case TIM2_RGB16:
			{
				unsigned char r, g, b, a;
				r = (unsigned char)((t<<3) & 0xF8);
				g = (unsigned char)((t>>2) & 0xF8);
				b = (unsigned char)((t>>7) & 0xF8);
				a = (unsigned char)((t>>8) & 0x80);
				return((a<<24) | (g<<16) | (b<<8) | r);
			}

		case TIM2_RGB24:
			return((0x80<<24) | (t & 0x00FFFFFF));

		case TIM2_RGB32:
			return(t);

		case TIM2_IDTEX4:
		case TIM2_IDTEX8:
			return(Tim2GetClutColor(ph, clut, t));
	}
	return(0);
}






// �����Խ����v���ϡ�PS2��ee-gcc�ǤΤ�ʹ�äǤ����v��
#ifdef R5900

// TIM2�ԥ�����ǩ`����GS���`���������i���z��
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// tbp:     ܞ���ȥƥ�������Хåե��Υک`���٩`�����ɥ쥹(-1�ΤȤ��إå��ڤ΂���ʹ��)
// cbp:     ܞ����CLUT�Хåե��Υک`���٩`�����ɥ쥹(-1�ΤȤ��إå��ڤ΂���ʹ��)
// ���ꂎ
//          NULL�ΤȤ�	����`
//          NULL�Ǥʤ��Ȥ����ΤΥХåե����ɥ쥹
// ע��
//          CLUT��{��`�ɤȤ���CSM2��ָ������Ƥ������Ϥ⡢���ƵĤ�CSM1�ˉ�Q����GS�����Ť����
unsigned int Tim2LoadPicture(TIM2_PICTUREHEADER *ph, unsigned int tbp, unsigned int cbp)
{
	// CLUT�ǩ`����ܞ��ܞ��
	tbp = Tim2LoadImage(ph, tbp);
	Tim2LoadClut(ph, cbp);
	return(tbp);
}


// TIM2�ԥ�����Υ���`���ǩ`������GS���`���������i���z��
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// tbp:     ܞ���ȥƥ�������Хåե��Υک`���٩`�����ɥ쥹(-1�ΤȤ��إå��ڤ΂���ʹ��)
// ���ꂎ
//          NULL�ΤȤ�	����`
//          NULL�Ǥʤ��Ȥ����ΤΥƥ�������Хåե����ɥ쥹
// ע��
//          CLUT��{��`�ɤȤ���CSM2��ָ������Ƥ������Ϥ⡢���ƵĤ�CSM1�ˉ�Q����GS�����Ť����
unsigned int Tim2LoadImage(TIM2_PICTUREHEADER *ph, unsigned int tbp)
{
	// ����`���ǩ`��������
	if(ph->MipMapTextures>0) {
		static const int psmtbl[] = {
			SCE_GS_PSMCT16,
			SCE_GS_PSMCT24,
			SCE_GS_PSMCT32,
			SCE_GS_PSMT4,
			SCE_GS_PSMT8
		};
		int i;
		int psm;
		u_long128 *pImage;
		int w, h;
		int tbw;

		psm = psmtbl[ph->ImageType - 1]; // �ԥ�����ե��`�ޥåȤ�ä�
		((sceGsTex0 *)&ph->GsTex0)->PSM  = psm;

		w = ph->ImageWidth;  // ����`��X������
		h = ph->ImageHeight; // ����`��Y������

		// ����`���ǩ`�������^���ɥ쥹��Ӌ��
		pImage = (u_long128 *)((char *)ph + ph->HeaderSize);
		if(tbp==-1) {
			// tbp��ָ����-1�ΤȤ����ԥ�����إå���ָ�����줿GsTex0����tbp,tbw��ä�
			tbp = ((sceGsTex0 *)&ph->GsTex0)->TBP0;
			tbw = ((sceGsTex0 *)&ph->GsTex0)->TBW;

			Tim2LoadTexture(psm, tbp, tbw, w, h, pImage); // �ƥ�������ǩ`����GS��ܞ��
		} else {
			// tbp��ָ����ָ�����줿�Ȥ����ԥ�����إå���GsTex0���Ф�tbp,tbw�򥪩`�Щ`�饤��
			tbw = Tim2CalcBufWidth(psm, w);
			// GS��TEX0�쥸�������O�����낎�����
			((sceGsTex0 *)&ph->GsTex0)->TBP0 = tbp;
			((sceGsTex0 *)&ph->GsTex0)->TBW  = tbw;
			Tim2LoadTexture(psm, tbp, tbw, w, h, pImage); // �ƥ�������ǩ`����GS��ܞ��
			tbp += Tim2CalcBufSize(psm, w, h);            // tbp�΂������
		}

		if(ph->MipMapTextures>1) {
			// MIPMAP�ƥ������㤬�������
			TIM2_MIPMAPHEADER *pm;

			pm = (TIM2_MIPMAPHEADER *)(ph + 1); // �ԥ�����إå���ֱ���MIPMAP�إå�

			// LV0�Υƥ������㥵������ä�
			// tbp��������ָ�����줿�Ȥ����ԥ�����إå��ˤ���miptbp��oҕ�����Ԅ�Ӌ��
			if(tbp!=-1) {
				pm->GsMiptbp1 = 0;
				pm->GsMiptbp2 = 0;
			}

			pImage = (u_long128 *)((char *)ph + ph->HeaderSize);
			// ��MIPMAP��٥�Υ���`����ܞ��
			for(i=1; i<ph->MipMapTextures; i++) {
				// MIPMAP��٥뤬������ȡ��ƥ������㥵������1/2�ˤʤ�
				w = w / 2;
				h = h / 2;

				pImage = (u_long128 *)((char *)pImage + pm->MMImageSize[i - 1]);
				if(tbp==-1) {
					// �ƥ�������ک`����ָ����-1�ΤȤ���MIPMAP�إå���ָ�����줿tbp,tbw��ʹ�ä���
					int miptbp;
					if(i<4) {
						// MIPMAP��٥�1,2,3�ΤȤ�
						miptbp = (pm->GsMiptbp1>>((i-1)*20)) & 0x3FFF;
						tbw    = (pm->GsMiptbp1>>((i-1)*20 + 14)) & 0x3F;
					} else {
						// MIPMAP��٥�4,5,6�ΤȤ�
						miptbp = (pm->GsMiptbp2>>((i-4)*20)) & 0x3FFF;
						tbw    = (pm->GsMiptbp2>>((i-4)*20 + 14)) & 0x3F;
					}
					Tim2LoadTexture(psm, miptbp, tbw, w, h, pImage);
				} else {
					// �ƥ�������ک`����ָ������Ƥ���Ȥ���MIPMAP�إå������O��
					tbw = Tim2CalcBufWidth(psm, w);    // �ƥ����������Ӌ��
					if(i<4) {
						// MIPMAP��٥�1,2,3�ΤȤ�
						pm->GsMiptbp1 |= ((u_long)tbp)<<((i-1)*20);
						pm->GsMiptbp1 |= ((u_long)tbw)<<((i-1)*20 + 14);
					} else {
						// MIPMAP��٥�4,5,6�ΤȤ�
						pm->GsMiptbp2 |= ((u_long)tbp)<<((i-4)*20);
						pm->GsMiptbp2 |= ((u_long)tbw)<<((i-4)*20 + 14);
					}
					Tim2LoadTexture(psm, tbp, tbw, w, h, pImage);
					tbp += Tim2CalcBufSize(psm, w, h); // tbp�΂������
				}
			}
		}
	}
	return(tbp);
}



// TIM2�ԥ������CLUT�ǩ`������GS���`��������ܞ��
// ����
// ph:      TIM2�ԥ�����إå������^���ɥ쥹
// cbp:     ܞ����CLUT�Хåե��Υک`���٩`�����ɥ쥹(-1�ΤȤ��إå��ڤ΂���ʹ��)
// ���ꂎ
//          0�ΤȤ�����`
//          ��0�ΤȤ��ɹ�
// ע��
//          CLUT��{��`�ɤȤ���CSM2��ָ������Ƥ������Ϥ⡢���ƵĤ�CSM1�ˉ�Q����GS�����Ť����
unsigned int Tim2LoadClut(TIM2_PICTUREHEADER *ph, unsigned int cbp)
{
	int i;
	sceGsLoadImage li;
	u_long128 *pClut;
	int	cpsm;

	// CLUT�ԥ�����ե��`�ޥåȤ�ä�
	if(ph->ClutType==TIM2_NONE) {
		// CLUT�ǩ`�������ڤ��ʤ��Ȥ�
		return(1);
	} else if((ph->ClutType & 0x3F)==TIM2_RGB16) {
		cpsm = SCE_GS_PSMCT16;
	} else if((ph->ClutType & 0x3F)==TIM2_RGB24) {
		cpsm = SCE_GS_PSMCT24;
	} else {
		cpsm = SCE_GS_PSMCT32;
	}
	((sceGsTex0 *)&ph->GsTex0)->CPSM = cpsm; // CLUT���ԥ�����ե��`�ޥå��O��
	((sceGsTex0 *)&ph->GsTex0)->CSM  = 0;    // CLUT��{��`��(����CSM1)
	((sceGsTex0 *)&ph->GsTex0)->CSA  = 0;    // CLUT����ȥꥪ�ե��å�(����0)
	((sceGsTex0 *)&ph->GsTex0)->CLD  = 1;    // CLUT�Хåե��Υ��`������(���˥��`��)

	if(cbp==-1) {
		// cbp��ָ�����ʤ��Ȥ����ԥ�����إå���GsTex0���Ф��邎��ȡ��
		cbp = ((sceGsTex0 *)&ph->GsTex0)->CBP;
	} else {
		// cbp��ָ�����줿�Ȥ����ԥ�����إå���GsTex0���Ф΂��򥪩`�Щ`�饤��
		((sceGsTex0 *)&ph->GsTex0)->CBP = cbp;
	}

	// CLUT�ǩ`�������^���ɥ쥹��Ӌ��
	pClut = (u_long128 *)((char *)ph + ph->HeaderSize + ph->ImageSize);

	// CLUT�ǩ`����GS���`������������
	// CLUT��ʽ�ȥƥ���������ʽ�ˤ�ä�CLUT�ǩ`���Υե��`�ޥåȤʤɤ��Q�ޤ�
	switch((ph->ClutType<<8) | ph->ImageType) {
		case (((TIM2_RGB16 | 0x40)<<8) | TIM2_IDTEX4): // 16ɫ,CSM1,16�ӥå�,�K���椨����
		case (((TIM2_RGB24 | 0x40)<<8) | TIM2_IDTEX4): // 16ɫ,CSM1,24�ӥå�,�K���椨����
		case (((TIM2_RGB32 | 0x40)<<8) | TIM2_IDTEX4): // 16ɫ,CSM1,32�ӥå�,�K���椨����
		case (( TIM2_RGB16        <<8) | TIM2_IDTEX8): // 256ɫ,CSM1,16�ӥå�,�K���椨����
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX8): // 256ɫ,CSM1,24�ӥå�,�K���椨����
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX8): // 256ɫ,CSM1,32�ӥå�,�K���椨����
			// 256ɫCLUT���ġ�CLUT��{��`�ɤ�CSM1�ΤȤ�
			// 16ɫCLUT���ġ�CLUT��{��`�ɤ�CSM1������椨�g�ߥե饰��ON�ΤȤ�
			// ���Ǥ˥ԥ����뤬����椨�������ä���Ƥ���ΤǤ��Τޤ�ܞ�Ϳ��ܤ��`
			Tim2LoadTexture(cpsm, cbp, 1, 16, (ph->ClutColors / 16), pClut);
			break;

		case (( TIM2_RGB16        <<8) | TIM2_IDTEX4): // 16ɫ,CSM1,16�ӥå�,��˥�����
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX4): // 16ɫ,CSM1,24�ӥå�,��˥�����
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX4): // 16ɫ,CSM1,32�ӥå�,��˥�����
		case (((TIM2_RGB16 | 0x80)<<8) | TIM2_IDTEX4): // 16ɫ,CSM2,16�ӥå�,��˥�����
		case (((TIM2_RGB24 | 0x80)<<8) | TIM2_IDTEX4): // 16ɫ,CSM2,24�ӥå�,��˥�����
		case (((TIM2_RGB32 | 0x80)<<8) | TIM2_IDTEX4): // 16ɫ,CSM2,32�ӥå�,��˥�����
		case (((TIM2_RGB16 | 0x80)<<8) | TIM2_IDTEX8): // 256ɫ,CSM2,16�ӥå�,��˥�����
		case (((TIM2_RGB24 | 0x80)<<8) | TIM2_IDTEX8): // 256ɫ,CSM2,24�ӥå�,��˥�����
		case (((TIM2_RGB32 | 0x80)<<8) | TIM2_IDTEX8): // 256ɫ,CSM2,32�ӥå�,��˥�����
			// 16ɫCLUT���ġ�CLUT��{��`�ɤ�CSM1������椨�g�ߥե饰��OFF�ΤȤ�
			// 16ɫCLUT���ġ�CLUT��{��`�ɤ�CSM2�ΤȤ�
			// 256ɫCLUT���ġ�CLUT��{��`�ɤ�CSM2�ΤȤ�

			// CSM2�ϥѥե��`�ޥ󥹤������Τǡ�CSM1�Ȥ�������椨�ʤ���ܞ��
			for(i=0; i<(ph->ClutColors/16); i++) {
				sceGsSetDefLoadImage(&li, cbp, 1, cpsm, (i & 1)*8, (i>>1)*2, 8, 2);
				FlushCache(WRITEBACK_DCACHE);   // D����å���Βߤ�����
				sceGsExecLoadImage(&li, pClut); // CLUT�ǩ`����GS��ܞ��
				sceGsSyncPath(0, 0);            // �ǩ`��ܞ�ͽK�ˤޤǴ��C

				// �Τ�16ɫ�ء����ɥ쥹����
				if((ph->ClutType & 0x3F)==TIM2_RGB16) {
					pClut = (u_long128 *)((char *)pClut + 2*16); // 16bitɫ�ΤȤ�
				} else if((ph->ClutType & 0x3F)==TIM2_RGB24) {
					pClut = (u_long128 *)((char *)pClut + 3*16); // 24bitɫ�ΤȤ�
				} else {
					pClut = (u_long128 *)((char *)pClut + 4*16); // 32bitɫ�ΤȤ�
				}
			}
			break;

		default:
			printf("Illegal clut and texture combination. ($%02X,$%02X)\n", ph->ClutType, ph->ImageType);
			return(0);
	}
	return(1);
}


// TIM2�ե�����ǥ��ʥåץ���åȤ��������
// ����
// d0:      ż���饹���Υե�`��Хåե�
// d1:      �����饹���Υե�`��Хåե�(NULL�ΤȤ��Υ󥤥󥿥�`��)
// pszFname:TIM2�ե�������
// ���ꂎ
//          0�ΤȤ�����`
//          ��0�ΤȤ��ɹ�
int Tim2TakeSnapshot(sceGsDispEnv *d0, sceGsDispEnv *d1, char *pszFname)
{
	int i;

	int h;               // �ե�����ϥ�ɥ�
	int nWidth, nHeight; // ����`���δ編
	int nImageType;      // ����`�����N�e
	int psm;             // �ԥ�����ե��`�ޥå�
	int nBytes;          // 1�饹���򘋳ɤ���Х�����

	// ���񥵥���,�ԥ�����ե��`�ޥåȤ�ä�
	nWidth  = d0->display.DW / (d0->display.MAGH + 1);
	nHeight = d0->display.DH + 1;
	psm     = d0->dispfb.PSM;

	// �ԥ�����ե��`�ޥåȤ��顢1�饹��������ΥХ�����,TIM2�ԥ�����N�e������
	switch(psm) {
		case SCE_GS_PSMCT32 :	nBytes = nWidth*4;	nImageType = TIM2_RGB32;	break;
		case SCE_GS_PSMCT24 :	nBytes = nWidth*3;	nImageType = TIM2_RGB24;	break;
		case SCE_GS_PSMCT16 :	nBytes = nWidth*2;	nImageType = TIM2_RGB16;	break;
		case SCE_GS_PSMCT16S:	nBytes = nWidth*2;	nImageType = TIM2_RGB16;	break;
		default:
			// �����ʥԥ�����ե��`�ޥåȤΤȤ�������`�K��
			// GS_PSGPU24�ե��`�ޥåȤϷǥ��ݩ`��
			printf("Illegal pixel format.\n");
			return(0);
	}


	// TIM2�ե�����򥪩`�ץ�
	h = sceOpen(pszFname, SCE_WRONLY | SCE_CREAT);
	if(h==-1) {
		// �ե����륪�`�ץ�ʧ��
		printf("file create failure.\n");
		return(0);
	}

	// �ե�����إå����������
	{
		TIM2_FILEHEADER fhdr;

		fhdr.FileId[0] = 'T';   // �ե�����ID���O��
		fhdr.FileId[1] = 'I';
		fhdr.FileId[2] = 'M';
		fhdr.FileId[3] = '2';
		fhdr.FormatVersion = 3; // �ե��`�ޥåȥЩ`�����4
		fhdr.FormatId  = 0;     // 16�Х��ȥ��饤����ȥ�`��
		fhdr.Pictures  = 1;     // �ԥ�����ö����1ö
		for(i=0; i<8; i++) {
			fhdr.pad[i] = 0x00; // �ѥǥ��󥰥��Ф�0x00�ǥ��ꥢ
		}

		sceWrite(h, &fhdr, sizeof(TIM2_FILEHEADER)); // �ե�����إå����������
	}

	// �ԥ�����إå����������
	{
		TIM2_PICTUREHEADER phdr;
		int nImageSize;

		nImageSize = nBytes * nHeight;
		phdr.TotalSize   = sizeof(TIM2_PICTUREHEADER) + nImageSize; // �ȩ`���륵����
		phdr.ClutSize    = 0;                           // CLUT���ʤ�
		phdr.ImageSize   = nImageSize;                  // ����`����������
		phdr.HeaderSize  = sizeof(TIM2_PICTUREHEADER);  // �إå���������
		phdr.ClutColors  = 0;                           // CLUTɫ��
		phdr.PictFormat  = 0;                           // �ԥ�������ʽ
		phdr.MipMapTextures = 1;                        // MIPMAP�ƥ�������ö��
		phdr.ClutType    = TIM2_NONE;                   // CLUT���ʤ�
		phdr.ImageType   = nImageType;                  // ����`���N�e
		phdr.ImageWidth  = nWidth;                      // ����`�����
		phdr.ImageHeight = nHeight;                     // ����`���ߤ�

		// GS�쥸�������O����ȫ��0�ˤ��Ƥ���
		phdr.GsTex0        = 0;
		((sceGsTex0 *)&phdr.GsTex0)->TBW = Tim2CalcBufWidth(psm, nWidth);
		((sceGsTex0 *)&phdr.GsTex0)->PSM = psm;
		((sceGsTex0 *)&phdr.GsTex0)->TW  = Tim2GetLog2(nWidth);
		((sceGsTex0 *)&phdr.GsTex0)->TH  = Tim2GetLog2(nHeight);
		phdr.GsTex1        = 0;
		phdr.GsTexaFbaPabe = 0;
		phdr.GsTexClut     = 0;

		sceWrite(h, &phdr, sizeof(TIM2_PICTUREHEADER)); // �ԥ�����إå����������
	}


	// ����`���ǩ`���Ε�������
	for(i=0; i<nHeight; i++) {
		u_char buf[4096];   // �饹���Хåե���4KB�_��
		sceGsStoreImage si;

		if(d1) {
			// ���󥿥�`���ΤȤ�
			if(!(i & 1)) {
				// ���󥿥�`����ʾ��ż���饹���ΤȤ�
				sceGsSetDefStoreImage(&si, d0->dispfb.FBP*32, d0->dispfb.FBW, psm,
				                      d0->dispfb.DBX, (d0->dispfb.DBY + i/2),
				                      nWidth, 1);
			} else {
				// ���󥿥�`����ʾ�������饹���ΤȤ�
				sceGsSetDefStoreImage(&si, d1->dispfb.FBP*32, d1->dispfb.FBW, psm,
				                      d1->dispfb.DBX, (d1->dispfb.DBY + i/2),
				                      nWidth, 1);
			}
		} else {
			// �Υ󥤥󥿥�`���ΤȤ�
			sceGsSetDefStoreImage(&si, d0->dispfb.FBP*32, d0->dispfb.FBW, psm,
			                      d0->dispfb.DBX, (d0->dispfb.DBY + i),
			                      nWidth, 1);
		}
		FlushCache(WRITEBACK_DCACHE); // D����å���Βߤ�����

		sceGsExecStoreImage(&si, (u_long128 *)buf); // VRAM�ؤ�ܞ�ͤ�����
		sceGsSyncPath(0, 0);                        // �ǩ`��ܞ�ͽK�ˤޤǴ��C

		sceWrite(h, buf, nBytes);  // 1�饹���֤Υǩ`�����������
	}

	// �ե�����Ε�����������
	sceClose(h);  // �ե�����򥯥��`��
	return(1);
}






// �ƥ�������ǩ`����ܞ��
// ����
// psm:     �ƥ�������ԥ�����ե��`�ޥå�
// tbp:     �ƥ�������Хåե��Υ٩`���ݥ����
// tbw:     �ƥ�������Хåե��η�
// w:       ܞ���I��κ��
// h:       ܞ���I��Υ饤����
// pImage:  �ƥ������㥤��`������{����Ƥ��륢�ɥ쥹
static void Tim2LoadTexture(int psm, u_int tbp, int tbw, int w, int h, u_long128 *pImage)
{
	sceGsLoadImage li;
	int i, l, n;
	u_long128 *p;

	switch(psm) {
		case SCE_GS_PSMZ32:
		case SCE_GS_PSMCT32:
			n = w*4;
			break;

		case SCE_GS_PSMZ24:
		case SCE_GS_PSMCT24:
			n = w*3;
			break;

		case SCE_GS_PSMZ16:
		case SCE_GS_PSMZ16S:
		case SCE_GS_PSMCT16:
		case SCE_GS_PSMCT16S:
			n = w*2;
			break;

		case SCE_GS_PSMT8H:
		case SCE_GS_PSMT8:
			n = w;
			break;

		case SCE_GS_PSMT4HL:
		case SCE_GS_PSMT4HH:
		case SCE_GS_PSMT4:
			n = w/2;
			break;

		default:
			return;
	}

	// DMA�����ܞ������512KB�򳬤��ʤ��褦�˷ָ�ʤ�������
	l = 32764 * 16 / n;
	for(i=0; i<h; i+=l) {
		p = (u_long128 *)((char *)pImage + n*i);
		if((i+l)>h) {
			l = h - i;
		}

		sceGsSetDefLoadImage(&li, tbp, tbw, psm, 0, i, w, l);
		FlushCache(WRITEBACK_DCACHE); // D����å���Βߤ�����
		sceGsExecLoadImage(&li, p);   // GS���`�������ؤ�ܞ�ͤ�����
		sceGsSyncPath(0, 0);          // �ǩ`��ܞ�ͽK�ˤޤǴ��C
	}
	return;
}



// ָ�����줿�ԥ�����ե��`�ޥåȤȥƥ������㥵�������顢�Хåե���������Ӌ��
// ����
// psm:     �ƥ�������ԥ�����ե��`�ޥå�
// w:       ���
// ���ꂎ
//          1�饤������M����Хåե�������
//          �gλ��256�Х���(64word)
static int Tim2CalcBufWidth(int psm, int w)
{
//	return(w / 64);

	switch(psm) {
		case SCE_GS_PSMT8H:
		case SCE_GS_PSMT4HL:
		case SCE_GS_PSMT4HH:
		case SCE_GS_PSMCT32:
		case SCE_GS_PSMCT24:
		case SCE_GS_PSMZ32:
		case SCE_GS_PSMZ24:
		case SCE_GS_PSMCT16:
		case SCE_GS_PSMCT16S:
		case SCE_GS_PSMZ16:
		case SCE_GS_PSMZ16S:
			return((w+63) / 64);

		case SCE_GS_PSMT8:
		case SCE_GS_PSMT4:
			w = (w+63) / 64;
			if(w & 1) {
				w++;
			}
			return(w);
	}
	return(0);
}



// ָ�����줿�ԥ�����ե��`�ޥåȤȥƥ������㥵�������顢�Хåե���������Ӌ��
// ����
// psm:     �ƥ�������ԥ�����ե��`�ޥå�
// w:       ���
// h:       �饤����
// ���ꂎ
//          1�饤������M����Хåե�������
//          �gλ��256�Х���(64word)
// ע��
//          �A���ƥ������㤬���ʤ�ԥ�����ե��`�ޥåȤ��{������Ϥϡ�
//          2KB�ک`���Х������ޤ�tbp�򥢥饤����Ȥ��{�������Ҫ�����롣
static int Tim2CalcBufSize(int psm, int w, int h)
{
	return(w * h / 64);
/*
	switch(psm) {
		case SCE_GS_PSMT8H:
		case SCE_GS_PSMT4HL:
		case SCE_GS_PSMT4HH:
		case SCE_GS_PSMCT32:
		case SCE_GS_PSMCT24:
		case SCE_GS_PSMZ32:
		case SCE_GS_PSMZ24:
			// 1�ԥ����뤢���ꡢ1��`�����M
			return(((w+63)/64) * ((h+31)/32));

		case SCE_GS_PSMCT16:
		case SCE_GS_PSMCT16S:
		case SCE_GS_PSMZ16:
		case SCE_GS_PSMZ16S:
			// 1�ԥ����뤢���ꡢ1/2��`�����M
			return(((w+63)/64) * ((h+63)/64));

		case SCE_GS_PSMT8:
			// 1�ԥ����뤢���ꡢ1/4��`�����M
			return(((w+127)/128) * ((h+63)/64));

		case SCE_GS_PSMT4:
			// 1�ԥ����뤢���ꡢ1/8��`�����M
			return(((w+127)/128) * ((h+127)/128));
	}
	// ����`?
	return(0);
*/
}



// �ӥåȷ���ä�
static int Tim2GetLog2(int n)
{
	int i;
	for(i=31; i>0; i--) {
		if(n & (1<<i)) {
			break;
		}
	}
	if(n>(1<<i)) {
		i++;
	}
	return(i);
}


#endif	// R5900