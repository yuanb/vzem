/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        EMULib.c                         **/
/**                                                         **/
/** This file contains platform-independent implementation  **/
/** part of the emulation library.                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2007                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "EMULib.h"
#include "Console.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef WINDOWS
#define NewImage  GenericNewImage
#define FreeImage GenericFreeImage
#define CropImage GenericCropImage
extern Image BigScreen;
#endif

/** Current Video Image **************************************/
/** These parameters are set with SetVideo() and used by    **/
/** ShowVideo() to show a WxH fragment from <X,Y> of Img.   **/
/*************************************************************/
Image *VideoImg = 0;           /* Current ShowVideo() image  */
int VideoX;                    /* X for ShowVideo()          */
int VideoY;                    /* Y for ShowVideo()          */
int VideoW;                    /* Width for ShowVideo()      */
int VideoH;                    /* Height for ShowVideo()     */

/** KeyHandler ***********************************************/
/** This function receives key presses and releases.        **/
/*************************************************************/
void (*KeyHandler)(unsigned int Key) = 0;

/** NewImage() ***********************************************/
/** Create a new image of the given size. Returns pointer   **/
/** to the image data on success, 0 on failure.             **/
/*************************************************************/
pixel *NewImage(Image *Img,int Width,int Height)
{
  Img->Data=(pixel *)malloc(Width*Height*sizeof(pixel));
  if(!Img->Data) Img->W=Img->H=Img->L=0;
  else
  {
    memset(Img->Data,0,Width*Height*sizeof(pixel));
    Img->W = Width;
    Img->H = Height;
    Img->L = Width;
  }
  return(Img->Data);
}

/** FreeImage() **********************************************/
/** Free previously allocated image.                        **/
/*************************************************************/
void FreeImage(Image *Img)
{
  /* If image is used for video, unselect it */
  if(VideoImg==Img) VideoImg=0;
  /* Image gone */
  if(Img->Data) free(Img->Data);
  Img->Data = 0;
  Img->W    = 0;
  Img->H    = 0;
  Img->L    = 0;
}

/** CropImage() **********************************************/
/** Create a subimage Dst of the image Src. Returns Dst.    **/
/*************************************************************/
Image *CropImage(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
  Dst->Data = Src->Data+Src->L*Y+X;
  Dst->W    = W;
  Dst->H    = H;
  Dst->L    = Src->L;
  return(Dst);
}

/** ClearImage() *********************************************/
/** Clear image with a given color.                         **/
/*************************************************************/
void ClearImage(Image *Img,pixel Color)
{
  pixel *P;
  int J;

  for(J=Img->W*Img->H,P=Img->Data;J;--J) *P++=Color;
}

/** IMGCopy() ************************************************/
/** Copy one image into another.                            **/
/*************************************************************/
void IMGCopy(Image *Dst,int DX,int DY,Image *Src,int SX,int SY,int W,int H,int TColor)
{
  pixel *S,*D;
  int X;

  if(DX<0) { W+=DX;SX-=DX;DX=0; }
  if(DY<0) { H+=DY;SY-=DY;DY=0; }
  if(SX<0) { W+=SX;DX-=SX;SX=0; } else if(SX+W>Src->W) W=Src->W-SX;
  if(SY<0) { H+=SY;DY-=SY;SY=0; } else if(SY+H>Src->H) H=Src->H-SY;
  if(DX+W>Dst->W) W=Dst->W-DX;
  if(DY+H>Dst->H) H=Dst->H-DY;

  if((W>0)&&(H>0))
  {
    S = Src->Data+Src->L*SY+SX;
    D = Dst->Data+Dst->L*DY+DX;
    if(TColor<0)
      for(;H;--H,S+=Src->L,D+=Dst->L)
        for(X=0;X<W;++X) D[X]=S[X];
    else
      for(;H;--H,S+=Src->L,D+=Dst->L)
        for(X=0;X<W;++X)
          if(S[X]!=TColor) D[X]=S[X];
  }
}

/** IMGDrawRect()/IMGFillRect() ******************************/
/** Draw filled/unfilled rectangle in a given image.        **/
/*************************************************************/
void IMGDrawRect(Image *Img,int X,int Y,int W,int H,pixel Color)
{
  pixel *P;
  int J;

  if(X<0) { W+=X;X=0; } else if(X+W>Img->W) W=Img->W-X;
  if(Y<0) { H+=Y;Y=0; } else if(Y+H>Img->H) H=Img->H-Y;

  if((W>0)&&(H>0))
  {
    for(P=Img->Data+Img->L*Y+X,J=0;J<W;++J) P[J]=Color;
    for(H-=2,P+=Img->L;H;--H,P+=Img->L) P[0]=P[W-1]=Color;
    for(J=0;J<W;++J) P[J]=Color;
  }
}

void IMGFillRect(Image *Img,int X,int Y,int W,int H,pixel Color)
{
  pixel *P;

  if(X<0) { W+=X;X=0; } else if(X+W>Img->W) W=Img->W-X;
  if(Y<0) { H+=Y;Y=0; } else if(Y+H>Img->H) H=Img->H-Y;

  if((W>0)&&(H>0))
    for(P=Img->Data+Img->L*Y+X;H;--H,P+=Img->L)
      for(X=0;X<W;++X) P[X]=Color;
}

/** IMGPrint() ***********************************************/
/** Print text in a given image.                            **/
/*************************************************************/
/*** @@@ NOT YET
void IMGPrint(Image *Img,const char *S,int X,int Y,pixel FG,pixel BG)
{
  const unsigned char *C;
  pixel *P;
  int I,J,K;

  X = X<0? 0:X>Img->W-8? Img->W-8:X;
  Y = Y<0? 0:Y>Img->H-8? Img->H-8:Y;

  for(K=X;*S;S++)
    switch(*S)
    {
      case '\n':
        K=X;Y+=8;
        if(Y>Img->H-8) Y=0;
        break;
      default:
        P=Img->Data+Img->L*Y+K;
        for(C=CurFont+(*S<<3),J=8;J;P+=Img->L,++C,--J)
          for(I=0;I<8;++I) P[I]=*C&(0x80>>I)? FG:BG;
        K+=8;
        if(X>Img->W-8)
        {
          K=0;Y+=8;
          if(Y>Img->H-8) Y=0;
        }
        break;
    }
}
***/

/** SetVideo() ***********************************************/
/** Set part of the image as "active" for display.          **/
/*************************************************************/
void SetVideo(Image *Img,int X,int Y,int W,int H)
{
  VideoImg = Img;
  VideoX   = X<0? 0:X>=Img->W? Img->W-1:X;
  VideoY   = Y<0? 0:Y>=Img->H? Img->H-1:Y;
  VideoW   = VideoX+W>Img->W? Img->W-VideoX:W;
  VideoH   = VideoY+H>Img->H? Img->H-VideoY:H;
#ifdef WINDOWS
  FreeImage(&BigScreen);
#endif
}

/** GetColor() ***********************************************/
/** Return pixel corresponding to the given <R,G,B> value.  **/
/** This only works for non-palletized modes.               **/
/*************************************************************/
pixel GetColor(unsigned char R,unsigned char G,unsigned char B)
{
  return(PIXEL(R,G,B));
}

/** WaitJoystick() *******************************************/
/** Wait until one or more of the given buttons have been   **/
/** pressed. Returns the bitmask of pressed buttons. Refer  **/
/** to BTN_* #defines for the button mappings.              **/
/*************************************************************/
unsigned int WaitJoystick(unsigned int Mask)
{
  unsigned int I;

  /* Wait for all requested buttons to be released first */
  while(GetJoystick()&Mask);
  /* Wait for any of the buttons to become pressed */
  do I=GetJoystick()&Mask; while(!I);
  /* Return pressed buttons */
  return(I);
}

/** SetKeyHandler() ******************************************/
/** Attach keyboard handler that will be called when a key  **/
/** is pressed or released.                                 **/
/*************************************************************/
void SetKeyHandler(void (*Handler)(unsigned int Key))
{
  KeyHandler=Handler;
}

/** GetFilePath() ********************************************/
/** Extracts pathname from filename and returns a pointer   **/
/** to the internal buffer containing just the path name    **/
/** ending with "\".                                        **/
/*************************************************************/
const char *GetFilePath(const char *Name)
{
  static char Path[256];
  const char *P;
  char *T;

  P=strrchr(Name,'\\');

  /* If path not found or too long, assume current */
  if(!P||(P-Name>200)) { strcpy(Path,"");return(Path); }

  /* Copy and return the pathname */
  for(T=Path;Name<P;*T++=*Name++);
  *T='\0';return(Path);
}

/** NewFile() ************************************************/
/** Given pattern NAME.EXT, generates a new filename in the **/
/** NAMEnnnn.EXT (nnnn = 0000..9999) format and returns a   **/
/** pointer to the internal buffer containing new filename. **/
/*************************************************************/
const char *NewFile(const char *Pattern)
{
  static char Name[256];
  struct stat Status;
  const char *P;
  char S[256],*T;
  int J;

  /* If too long name, fall out */
  if(strlen(Pattern)>200) { strcpy(Name,"");return(Name); }

  /* Make up the format string */
  for(T=S,P=Pattern;*P&&(*P!='.');) *T++=*P++;
  *T='\0';
  strcat(S,"%04d");
  strcat(S,P);

  /* Scan through the filenames */
  for(J=0;J<10000;J++)
  {
    sprintf(Name,S,J);
    if(stat(Name,&Status)) break;
  }

  if(J==10000) strcpy(Name,"");
  return(Name);
}
