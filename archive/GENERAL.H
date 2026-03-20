/******************************************************************************
      Filename    : GENERAL.H
      Product     : NITPIC
      Description : General header file
      Created     : August 4, 1999 [11:43:01]
      Last Edit   : January 17, 2002
*******************************************************************************
                       Copyright (c) 2002 - M.Heyns
                           All rights reserved.
******************************************************************************/

#ifndef NULL
        #define NULL            0
#endif

// General 

#define         True            1
#define         False           0
#define         Yes             1
#define         No              0
#define         Ok              0
#define         Fail            -1

typedef         unsigned char   uchar;
typedef         unsigned int    uint;
typedef         unsigned long   ulong;

#define         LOCAL           static
#define         loop            while(1)

// return codes 

#define         RC_OK           0
#define         RC_FAIL         -1

// macros 

   #define Max(a,b)                     (((a) > (b)) ? (a) : (b))
   #define Min(a,b)                     (((a) < (b)) ? (a) : (b))
   #define BuildInt(high,low)           (((Word)high << 8) + (Byte)low)
   #define LowOfInt(v)                  (v & 0x00ff)
   #define HighOfInt(v)                 (v >> 8)

/******************************************************************************
                             End of GENERAL.H
******************************************************************************/
