#include <os2.h>
#include <stdio.h>
#include <stdlib.h>


#define  M_SOF0   0xc0
#define  M_SOF1   0xc1
#define  M_SOF2   0xc2
#define  M_SOF3   0xc3
#define  M_SOF5   0xc5
#define  M_SOF6   0xc6
#define  M_SOF7   0xc7
#define  M_JPG    0xc8
#define  M_SOF9   0xc9
#define  M_SOF10  0xca
#define  M_SOF11  0xcb
#define  M_SOF13  0xcd
#define  M_SOF14  0xce
#define  M_SOF15  0xcf
#define  M_DHT    0xc4
#define  M_DAC    0xcc
#define  M_RST0   0xd0
#define  M_RST1   0xd1
#define  M_RST2   0xd2
#define  M_RST3   0xd3
#define  M_RST4   0xd4
#define  M_RST5   0xd5
#define  M_RST6   0xd6
#define  M_RST7   0xd7
#define  M_SOI    0xd8
#define  M_EOI    0xd9
#define  M_SOS    0xda
#define  M_DQT    0xdb
#define  M_DNL    0xdc
#define  M_DRI    0xdd
#define  M_DHP    0xde
#define  M_EXP    0xdf
#define  M_APP0   0xe0
#define  M_APP15  0xef
#define  M_JPG0   0xf0
#define  M_JPG13  0xfd
#define  M_COM    0xfe
#define  M_TEM    0x01
#define  M_ERROR  0x100


USHORT uWIDTH  = 0;
USHORT uHEIGHT = 0;
USHORT bARITH  = 0;



void Error (PSZ psz)
   {
   printf ("Error %s\n", psz);
   exit (1);
   }



USHORT Read2 (FILE *fp)
   {
   char l, h;

   fread (&h, 1, 1, fp);
   fread (&l, 1, 1, fp);
   return (USHORT)h << 8 | l;
   }

void Readn (FILE *fp, USHORT i)
   {
   USHORT c;

   for (;i;i--)
      fread (&c, 1, 1, fp);
   }


void GetSOF (FILE *fp)
   {
   USHORT uLen;

   uLen    = Read2 (fp) - 7;
   getc (fp);
   uHEIGHT = Read2 (fp);
   uWIDTH  = Read2 (fp);
   Readn (fp, uLen);
   }


void SkipInfo (FILE *fp)
   {
   USHORT uLen;

   uLen = Read2 (fp) - 2;
   Readn (fp, uLen);
   }


USHORT NextMarker (FILE *fp)
   {
   USHORT uBytes, c=0;

   while (c == 0)
      {
      for (uBytes=0; (c=getc(fp)) != 0xFF; uBytes++)
         ;
      for (; (c=getc(fp)) == 0xFF; uBytes++)
         ;
      }
   return c;
   }



void ProcessTables (FILE *fp)
   {
   USHORT c;

   while (TRUE)
      {
      switch (c = NextMarker (fp))
         {
         case M_SOF0:
         case M_SOF1:
            bARITH = FALSE;
            GetSOF (fp);
            return;

         case M_SOF9:
            bARITH = TRUE;
            SkipInfo (fp);
            return;

         case M_SOF2:
         case M_SOF3:
         case M_SOF5:
         case M_SOF6:
         case M_SOF7:
         case M_JPG:
         case M_SOF10:
         case M_SOF11:
         case M_SOF13:
         case M_SOF14:
         case M_SOF15:
         case M_SOI:
         case M_EOI:
         case M_SOS:
            SkipInfo (fp);
            return; 

         case M_RST0:		/* these are all parameterless */
         case M_RST1:
         case M_RST2:
         case M_RST3:
         case M_RST4:
         case M_RST5:
         case M_RST6:
         case M_RST7:
         case M_TEM:
            break;

         case M_DHT:
         case M_DAC:
         case M_DQT:
         case M_DRI:
         case M_APP0:
            SkipInfo (fp);
            break;

         default:	/* must be DNL, DHP, EXP, APPn, JPGn, COM, or RESn */
            SkipInfo (fp);
            break;
         }
      }
   }





void ReadJPGHeader (FILE *fp)
   {
   if (getc (fp) != 0xFF  || getc (fp) != M_SOI)
      Error ("Not a JPEG File");
   ProcessTables (fp);
   }





main (int argc, char *argv[])
   {
   FILE *fp;

   if (!(fp = fopen (argv[1], "rb")))
      Error ("Unable to open input.");

   ReadJPGHeader (fp);
   fclose (fp);

   printf ("File: %s, Width: %d,  Height: %d\n", argv[1], uWIDTH, uHEIGHT);
   return 0;
   }
