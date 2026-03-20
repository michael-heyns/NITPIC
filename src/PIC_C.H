/******************************************************************************
      Filename    : PIC_C.H
      Product     : NIPIC
      Description : Main header file
      Created     : August 4, 1999 [11:44:50]
      Last Edit   : January 17, 2002
*******************************************************************************
                       Copyright (c) 2002 - M.Heyns
                           All rights reserved.
******************************************************************************/

#ifndef SCOPE
		#define SCOPE	extern
#endif

#include "version.h"
#include "proto.h"

#define STANDARD_SIZE			1

// program name
#define PROGID	 				"NITPIC"
#define TMPFILE  				"NITPIC.TMP"

#define L_FLDCNT				75		// max number of fields per record
#define L_FLDLEN				255 	// max length of record

#define NAMELEN 				80		// max length of any name

#define MAXERR					50		// max error queue length
#define MAXLOOKUPS				100 	// maximum pre-interrupt lookups

#define MAXHEXDIRECT			255 	// maximum stuff directly to HEX

#define DEMOMAXMACS 			200
#define DEMOMAXDEFS 			200
#define DEMOMAXFILES			3
#define DEMOMAXPROCS			6
#define DEMOMAXSOURCE			200		// max number of source lines to read
#define DEMOMACOVERHEAD 		200 	// max lines for MAC file

#define MAXMACS 				2000
#define MAXDEFS 				2000
#define MAXFILES				100
#define MAXPROCS            	250     // CAUTION !!!!
#define MAXSOURCE				30000	// max number of source lines to read
#define MACOVERHEAD 			30000	// USES NO MEMORY

#define MAXLABELS				10000	// maximum labels
#define MAXCODELINES			30000	// maximum number of code lines

// other limitations
#define TRIPCHAR				'$'
#define MAXLEVELS				100
#define MAXVARS 				500		//INCREASE FOR 17Cxx	!!!!!
#define MAXCPUREG				500		// max CPU registers allowed
#define MAXREGBANKS 			4

struct LT {
		char *Name; 					// name of label
		int Address;					// what code address it's on
		char Used;						// =1 if actually used
};
SCOPE int NextLabel;
SCOPE int OCaddr;						// OPCODE address

struct OCtable {
		char *oc;						// opcode string
		int p1; 						// parameter 1
		char p2;						// parameter 2
		char *str;						// label string for goto's and call's
};

struct OCref {
		char fileid;					// file ID
		int line;						// line # in file where this is
		int code;						// assembled object code
		int tmp_index;					// line number in NITPIC.TMP
		int lst_index;					// line number in .LST file
};

struct ST {
		char *name;
		int addr;
		char type;
};

#define ID_NO_FILE				-1
#define ID_ERROR				-2
#define ID_WRONG				-3
#define ID_FIRST_TIME			-4
#define ID_OK					-5

// data structures
SCOPE struct OCref OCREF[MAXCODELINES];
SCOPE struct OCtable OC[MAXCODELINES];
SCOPE struct LT Label[MAXLABELS];

// Processors
#define CPULEN	40
SCOPE char CPU[CPULEN];

// COMPARE sign
#define UNSIGNED				0
#define SIGNED					1

// type of block
#define BNONE					0		// must be zero - ref BlockType
#define BIF 					1
#define BWHILE					2
#define BREPEAT 				3
#define BLOOP					4
#define BIFGOTO 				5
#define BFOR					6
#define BCOUNT					7

// expression lot identifier
#define FIRST					0
#define LAST					1

// operators
#define EQUAL					0
#define GREATER 				1
#define GREATEREQ				2
#define BITSET					3
#define BITCOMP 				4

SCOPE char L_NAME[NAMELEN]; 			// program name
SCOPE char L_ID[NAMELEN/4]; 			// short-form name
SCOPE int  L_VERSION;					// major version
SCOPE int  L_RELEASE;					// release number
SCOPE char FS[L_FLDLEN];				// field seperator
SCOPE int  NF;							// number of fields
SCOPE int  NR;							// number of records
SCOPE int  TotalNR; 					// total number of records
SCOPE char INFILE[NAMELEN]; 			// input filename
SCOPE char DATE[40];					// starting date & time
SCOPE char S[L_FLDCNT][L_FLDLEN];		// fields for each record
SCOPE int  STACK_DEPTH; 				// stack depth
SCOPE char ERRFILE[NAMELEN];			// error file

SCOPE int Errors;						// total errors found
SCOPE int Warnings; 					// total warnings

// file handles
SCOPE FILE *cod;
SCOPE FILE *lst;
SCOPE FILE *assm;
SCOPE FILE *inpf;
SCOPE FILE *hex;
SCOPE FILE *map;
SCOPE FILE *tmp;						// temp file handle
SCOPE FILE *fpi;						// input file handle
SCOPE FILE *err;						// error file handle
SCOPE FILE *arc;						// archiver batch file handle
SCOPE FILE *ini;

// Config data
SCOPE int CodeWidth;
SCOPE int ResetVector;
SCOPE int INTvector;
SCOPE int TopROM;
SCOPE int UserRegStart[MAXREGBANKS];
SCOPE int UserRegEnd[MAXREGBANKS];
SCOPE int BankSize[MAXREGBANKS];
SCOPE int MaxVariables;
SCOPE int NextRegBank;
SCOPE int StackDepth;
SCOPE int TopCodePage;
SCOPE int CodePageSize;
SCOPE int ProcRange;
SCOPE int DataPageSize;
SCOPE char *Reg[MAXCPUREG];
SCOPE int RegAddr[MAXCPUREG];
SCOPE int NextReg;
SCOPE int AllBanksStart[MAXCPUREG];
SCOPE int AllBanksEnd[MAXCPUREG];
SCOPE int NextAll;
SCOPE int tmp_index;

// Global data
SCOPE char p_src[L_FLDLEN]; 			// parameter one of expression
SCOPE char p_dst[L_FLDLEN]; 			// parameter two
SCOPE char p_bit1;						// used for bit compare
SCOPE char p_bit2;						// "
SCOPE char ORflag;						// Yes/No if expr contains OR
SCOPE char ANDflag; 					// Yes/No if expr contains AND
SCOPE char ANDflagHistory, ORflagHistory;
SCOPE int Level;
SCOPE int Sequence[MAXLEVELS];
SCOPE char BlockType[MAXLEVELS];
SCOPE char PageChanged[MAXLEVELS];		// 1 or 0
SCOPE char PrevPageChanged[MAXLEVELS];	// 1 or 0
SCOPE char ForceFix[MAXLEVELS]; 		// 1 or 0
SCOPE int PrevPage[MAXLEVELS];			// must be int for -1
SCOPE int StartPage[MAXLEVELS]; 		// must be int for -1
SCOPE int CountVariable[MAXLEVELS];
SCOPE char CVtypeWord[MAXLEVELS];
SCOPE char TwoOrMorePages;
SCOPE int datapagenum;					// current data page number
SCOPE int ORlabelCount, IEElabelCount[MAXLEVELS];
SCOPE char BusyWithELSE[MAXLEVELS];
SCOPE int JmpLabelCount;				// any counter for jump labels
SCOPE char ForVar[MAXLEVELS][L_FLDLEN];
SCOPE unsigned int ForInc[MAXLEVELS];
SCOPE int busy_with_proc;
SCOPE int busy_with_int;
SCOPE int nports;						// number of ports
SCOPE char no_list; 					// = 1 for no list output
SCOPE int FUSES;						// config fuses
SCOPE int PID;							// product ID
SCOPE char did_id;						// = 1 if ID specified
SCOPE char do_fuses;					// = 1 to write fuse record as well
SCOPE char did_fuses;					// = 1 when fuses were specified
SCOPE int FuseAddress;					// fuse address - see #FUSE_ADDR
SCOPE int IDaddress;					// ID address - see also #ID_ADDR
SCOPE int CurrentFileID;				// current file index
SCOPE char DefaultInclude[NAMELEN];
SCOPE char ConfigFile[NAMELEN];
SCOPE char HelpFile[NAMELEN];
SCOPE char MainFile[NAMELEN];
SCOPE char int_declared;				//=1 when interrupt declared OK
SCOPE char *LookupTbl[MAXLOOKUPS];
SCOPE int NextLookup;
SCOPE char InsideExp;					//=1 when inside expression
SCOPE int LSTline;						// line counter for .LST file

// direct I/O to hext file
SCOPE int HexAddress[MAXHEXDIRECT];
SCOPE int HexData[MAXHEXDIRECT];
SCOPE int NextHex;

// Option switches
SCOPE char OPTIONS;
#define O_domap 				1
#define O_verbatum				2
#define O_dolist				4
#define O_docod 				8
#define O_doasm 				16

// MACRO buffers
SCOPE char *MacName[MAXMACS];			// macro names
SCOPE char *Macro[MAXMACS]; 			// macro buffers
SCOPE int NextMac;						// next available MACRO entry

// DEFINE buffers
SCOPE char *DefName[MAXDEFS];			// macro names
SCOPE char *Define[MAXDEFS];			// macro buffers
SCOPE int NextDef;						// next available MACRO entry

// VARIABLE names
SCOPE char *Variable[MAXVARS];			// variable names
SCOPE char VarUsed[MAXVARS];			// =1 when used at least once
SCOPE int UsageCount[MAXVARS];			// usage counter
SCOPE int VarAddr[MAXVARS]; 			// addresses for variables
SCOPE int NextVar;

// PROCEDURE names
SCOPE char *Proc[MAXPROCS]; 			// procedure names
SCOPE int ProcFile[MAXPROCS];			// reference to filename list
SCOPE char *FileList[MAXFILES]; 		// list of file names done so far
SCOPE int NextFile; 					// next file in file list
SCOPE int ProcStart[MAXPROCS];			// line where proc starts
SCOPE int ProcEnd[MAXPROCS];			// line where proc ends
SCOPE int ProcCodeStart[MAXPROCS];		// code where proc starts
SCOPE int ProcCodeEnd[MAXPROCS];		// code where proc ends
SCOPE int IntStart, IntEnd; 			// lines where INT handler starts/stops
SCOPE int NextProc;
SCOPE int ProcUsageCount[MAXPROCS]; 	// call count for each proc
SCOPE char ProcInline[MAXPROCS];		// =1 when proc may be inline
SCOPE char ProcReturnsValue[MAXPROCS];	// =1 when proc returns a value
SCOPE char ProcStatus[MAXPROCS];		// (see below)
#define PROC_EXIST						1
#define PROC_CALLED 					2		// called from NITPIC
#define ASM_CALLED						4		// called from ASM

// library forward references
SCOPE char *LibName[MAXPROCS];
SCOPE int NextLib;
SCOPE char SkipTillEnd;

/******************************************************************************
                              End of PIC_C.H
******************************************************************************/
