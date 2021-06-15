#define INCL_DOSFILEMGR
#include <os2.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <time.h>
#include <ctype.h>
#include "arg2.h"

#define TIME __TIME__
#define DATE __DATE__


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


BOOL bVerbose;

typedef struct
   {
   USHORT uWidth;
   USHORT uHeight;
   USHORT bArith;
   } JFO;
typedef JFO *PJFO;


USHORT Read2 (FILE *fp)
   {
   unsigned char l, h;

   fread (&h, 1, 1, fp);
   fread (&l, 1, 1, fp);
   return (USHORT)h << 8 | (USHORT)l;
   }

USHORT Read2LI (FILE *fp)
   {
   unsigned char l, h;

   fread (&l, 1, 1, fp);
   fread (&h, 1, 1, fp);
   return (USHORT)h << 8 | (USHORT)l;
   }

void Readn (FILE *fp, USHORT i)
   {
   USHORT c;

   for (;i;i--)
      fread (&c, 1, 1, fp);
   }


void Error (PSZ psz1, PSZ psz2)
   {
   printf ("Error: %s %s\n", psz1, psz2);
   exit (1);
   }


/*
 * This fn takes a filepath and splits
 * the file and the path
 * returns 1 if there is a path
 */
USHORT split (PSZ psz, PSZ pszPath, PSZ pszName)
   {
   PSZ p;

   if (p = strrchr (psz, '\\'))
      {
      if (pszPath)
         {
         strcpy (pszPath, psz);
         pszPath[p-psz+1] = '\0';
         }
      strcpy (pszName, p+1);
      return 1;
      }
   if (pszPath)
      *pszPath='\0';
   strcpy (pszName, psz);
   return 0;
   }


/*
 * returns TRUE if there was an ext to replace
 */
USHORT ReplaceExtention (PSZ pszDest, PSZ pszSrc, PSZ pszExt)
   {
   char  szName[128];
   PSZ   psz;

   split (pszSrc, pszDest, szName);

   if (psz = strrchr (strcat (pszDest, szName), '.'))
      *psz = '\0';

   strcat (pszDest, ".");
   strcat (pszDest, pszExt);
   return !(psz == NULL); 
   }



int BadChar (int c)
   {
   c = toupper (c);
   return  ((c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            (c == '_'));
   }



/***********************************************************************/
/*                                                                     */
/*                           GIF                                       */
/*                                                                     */
/***********************************************************************/

BOOL ReadGIFHeader (FILE *fp, PJFO pjfo)
   {
   char   sz [16];
   char   szTmp [512];
   USHORT i, uPlanes, uColors;

   pjfo->uWidth  = 0;
   pjfo->uHeight = 0;

   rewind (fp);
   fread (sz, 1, 13, fp);

   if(strncmp(sz,"GIF87a",3) || !isdigit (sz[3]) || !isdigit (sz[4]) || !isalpha (sz[5]))
      return FALSE;

   if((sz[10] & 0x80)==0)	 /* check for color map */
      return FALSE;

   uPlanes = (sz[10] & 0x0F) + 1;
   uColors = 1 << uPlanes;

   fread (&szTmp, 1, uColors, fp);
   fread (&szTmp, 1, uColors, fp);
   fread (&szTmp, 1, uColors, fp);
      
   while (1)
      {
      switch (getc (fp))
         {
         case ';':                            /*--- End           ---*/
            return TRUE;

         case '!':                            /*--- extension blk ---*/
            while (i = getc (fp) && i != EOF)
               Readn (fp, i);
            break;

         case ',':                            /*--- Image         ---*/
            fread (sz, 1, 4, fp);
            pjfo->uWidth  = Read2LI (fp);
            pjfo->uHeight = Read2LI (fp);
            return TRUE;
         }
      }

   return TRUE;
   }



/***********************************************************************/
/*                                                                     */
/*                           JPG                                       */
/*                                                                     */
/***********************************************************************/

void GetSOF (FILE *fp, PJFO pjfo)
   {
   USHORT uLen;

   uLen    = Read2 (fp) - 7;
   getc (fp);
   pjfo->uHeight = Read2 (fp);
   pjfo->uWidth  = Read2 (fp);
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




void ProcessTables (FILE *fp, PJFO pjfo)
   {
   USHORT c;

   while (TRUE)
      {
      switch (c = NextMarker (fp))
         {
         case M_SOF0:
         case M_SOF1:
            pjfo->bArith = FALSE;
            GetSOF (fp, pjfo);
            return;

         case M_SOF9:
            pjfo->bArith = TRUE;
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


BOOL ReadJPGHeader (FILE *fp, PJFO pjfo)
   {
   if (getc (fp) != 0xFF  || getc (fp) != M_SOI)
      return FALSE;
   ProcessTables (fp, pjfo);
   return TRUE;
   }



/***********************************************************************/
/***********************************************************************/
/***********************************************************************/


void HardTab (PSZ psz, USHORT uLen)
   {
   USHORT i;

   for (i=strlen (psz); i<uLen; i++)
      putchar (' ');
   }


USHORT ListIt (PSZ pszFile)
   {
   FILE *fp;
   USHORT i, uTab, uStrLen;
   struct stat FileStat;
   struct tm *ptm;
   ULONG  ulLen, ulRatio;
   JFO    jfo;
   char   szName [128];

   if (bVerbose)
      {
      uStrLen = strlen(pszFile);
      strcpy (szName, pszFile + (uStrLen > 36 ? uStrLen - 36 : 0));
      }
   else
      split (pszFile, NULL, szName);

   uStrLen = strlen(szName);

   printf ("%s", szName);

   uTab = (bVerbose ? 36 : 12);
   for (i=uStrLen; i<uTab; i++)
      fputchar (' ');

   if (!(fp = fopen (pszFile, "rb")))
      {
      printf (" Unable to open.\n");
      return 0;
      }
   if (!ReadJPGHeader (fp, &jfo) && 
       !ReadGIFHeader (fp, &jfo))
      {
      fclose (fp);
      printf (" Unable to determine file type.\n");
      return 0;
      }

   fstat (fileno (fp), &FileStat);
   fclose (fp);
   ulLen = FileStat.st_size;
   ptm = localtime (&(FileStat.st_atime));
   ulRatio = (ULONG)jfo.uWidth * (ULONG)jfo.uHeight * 30L / ulLen;

   printf (" %8.ld  %2.2d-%2.2d-%2.2d  %4.dx%-4.d  (%ld.%1.1ld:1)\n",
            ulLen, ptm->tm_mon +1, ptm->tm_mday, ptm->tm_year,
            jfo.uWidth, jfo.uHeight, ulRatio/10L, ulRatio % 10L);
   return 0;
   }



USHORT FindFiles (PSZ pszWildCard, BOOL bRecurse)
   {
   USHORT      uRes;
   FILEFINDBUF findbuf;
//   HDIR        hdir = HDIR_CREATE;
   HDIR        hdir = HDIR_SYSTEM;
   USHORT      uFiles, uSearchCount = 1;
   char        szStr[128], szPath[128], szName[128];
   PSZ         *ppszNames;

   ppszNames = NULL;
   split (pszWildCard, szPath, szName);

   /*--- Files first ---*/
   uRes = DosFindFirst(pszWildCard, &hdir, FILE_NORMAL | FILE_ARCHIVED,
                       &findbuf, sizeof(findbuf), &uSearchCount, 0L);

   for (uFiles=0; !uRes; uFiles++)
      {
      sprintf (szStr, "%s%s", szPath, findbuf.achName);
      ListIt (szStr);
      uRes = DosFindNext(hdir, &findbuf, sizeof(findbuf), &uSearchCount);
      }
   DosFindClose (hdir);

   if (!bRecurse) 
      return 0;

   /*--- now subdirs ---*/
   hdir = HDIR_CREATE;
   uSearchCount = 1;

   sprintf (szStr, "%s*.*", szPath);

   uRes = DosFindFirst(szStr, &hdir, FILE_DIRECTORY,
                       &findbuf, sizeof(findbuf), &uSearchCount, 0L);

   while (!uRes)
      {
      if ((findbuf.attrFile & FILE_DIRECTORY) && findbuf.achName[0] != '.')
         {
         sprintf (szStr, "%s%s\\%s", szPath, findbuf.achName, szName);
         FindFiles (szStr, bRecurse);
         }
      uRes = DosFindNext(hdir, &findbuf, sizeof(findbuf), &uSearchCount);
      }
   DosFindClose (hdir);
   return 0;
   }



void Usage ()
   {
   printf ("LISTJPG       jpeg file information utility     v1.0  %s  %s\n\n", TIME, DATE);
   printf ("USAGE:    LISTJPG {options} filespec {filespec ...}\n\n");
   printf ("WHERE:    options are 0 or more of:\n");
   printf ("             /S or /R .... Recursively search subdirectories\n");
   printf ("             /h or /? .... This help\n");
   printf ("             /V       .... verbose. print path on subdir files\n");
   printf ("          filespec is the name of the files to list, wildcards OK\n");
   exit (0);
   }


cdecl main (int argc, char *argv[])
   {
   PSZ   psz;
   BOOL  bRecurse;
   USHORT i;

   putchar ('\n');
   BuildArgBlk ("^S ^R ? ^H ^V");
   if (FillArgBlk (argv))
      Error ("%s", GetArgErr ());

   if (IsArg ("?") || IsArg ("H"))
      Usage ();

   bRecurse = IsArg ("R") || IsArg ("S");
   bVerbose = IsArg ("V");

   if (!IsArg (NULL))
      FindFiles ("*.*", bRecurse);
   else
      for (i=0; psz=GetArg (NULL, i); i++)
         FindFiles (psz, bRecurse);
   return 0;
   }



