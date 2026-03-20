/******************************************************************************
      Filename    : CPUSPEC.C
      Product     : NITPIC
      Description : Handles CPU-specific processes
      Created     : August 4, 1999 [11:37:35]
      Last Edit   : January 17, 2002
*******************************************************************************
                       Copyright (c) 2002 - M.Heyns
                           All rights reserved.
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <malloc.h>

#include "general.h"
#include "pic_c.h"

int JGlabel;			// increments with every new label
int Wlabel;             // used in word calculation jumps
int BClabel;			// bit compare labels

char asmflag = 0;		//=1 to pass all to assembler
char MainFlag = 0;		//=1 if 'main' defined
char TopPageFlag = 0;		//=1 when top page selected
char DidBit6 = 0;		//=1 when bit 6 of STATUS is changed
char has_tris;			//=1 when it has TRIS registers
char has_pclath;		//=1 when it has a PCLATH register
char port_start;		// where PORTA starts
int proc_count = 0;		// number of 'proc' statements

#define T_EQUAL			0
#define T_NOT_EQUAL		1
#define T_GREATER		2
#define T_NOT_GREATER		3
#define T_GREATER_EQ		4
#define T_NOT_GREATER_EQ	5

char IgnoreOFtest = 0;

IsPORT(char *pname)
{
	char *name = new(L_FLDLEN);
	char *ptr;

	strcpy(name, pname);
	if ((ptr = strchr(name, ':')))
		*ptr = '\0';			// kill bit references
	if (strlen(name) == 5 && strncmp(name, "PORT", 4) == 0 && 
	    name[4] >= 'A' && name[4] < 'A'+nports)
	{
	    	free(name);
		return(1);
	}
	else if (strcmp(name, "GPIO") == 0)
	{
	    	free(name);
		return(1);
	}
    	free(name);
	return(0);
}

OnlyOne()
{
	error("Error   %s %d : Only ONE parameter allowed\n", INFILE, NR);
}

MayNotMix()
{
	error("Error   %s %d : May not mix WORDS and BYTES here\n", INFILE, NR);
}

ProcToBytes()
{
	error("Error   %s %d : Procedures may only be tested with BYTES\n", INFILE, NR);
}

ForBytesOnly()
{
	error("Error   %s %d : This operation can only be done with BYTES\n", INFILE, NR);
}

Undefined(char *name)
{
	error("Error   %s %d : Undefined variable '%s'\n", INFILE, NR, name);
}

BitError()
{
	error("Error   %s %d : Bit constant may only be 0 or 1\n", INFILE, NR);
}

PageError()
{
	error("Error   %s %d : Source and Destination must be on same page\n", INFILE, NR);
}

PortError()
{
	error("Error   %s %d : Operation not allowed directly to PORT\n", INFILE, NR);

}

// returns the number of ports for this device
FindPortCount()
{
	int i;
	
	nports = 0;			// number of ports
	for (i = 0; i < NextReg; i++)
	{
		if (strnicmp(Reg[i], "PORT", 4) == 0 && 
		    strlen(Reg[i]) == 5)
		    {
		    	if (nports == 0)
		    		port_start = i;	// remember where it starts
			nports++;
		}
		if (strnicmp(Reg[i], "PCLATH", 6) == 0)
		{
			has_pclath = 1;
		}
		if (has_tris == 0 && 
		    strnicmp(Reg[i], "TRIS", 4) == 0 && 
		    strlen(Reg[i]) == 5)
			has_tris = i;		// also remember where
	}
}

// This routine determines how many instructions will be needed to set
// the page address for the given variable
findicount(char *varname)
{
	int addr = find_reg(varname);
	int icount = 0;
	int pg_num = addr / DataPageSize;
	
	if (pg_num > 0)
	{
		if (!TopPageFlag)
		{
			if (pg_num == 1)
			{
				icount = 1;
			}
			else if (pg_num == 2)
			{
				icount = 2;
			}
			else if (pg_num == 3)
			{
				icount  = 2;
			}
		}
	}
	else if (TopPageFlag)
	{
		icount = 1;
		if (DidBit6)
			icount = 2;
	}
	return(icount);
}

// find the data page number for a given BYTE variable
findpage(char *varname)
{
	int addr = find_reg(varname);

	if (addr < 0)
	{
		Undefined(varname);
		return(0);
	}
	return(addr / DataPageSize);
}

// find the data page number for a given WORD variable
w_findpage(char *varname)
{
	int addr = find_low_reg(varname);

	if (addr < 0)
	{
		Undefined(varname);
		return(0);
	}
	return(addr / DataPageSize);
}

// forces a page setup when next a variable is used
reset_datapage()
{
	datapagenum = -1;
}

// set the correct page (if necessary) for a given variable
setpage(char *varname)
{
	int addr = find_reg(varname);
	if (addr >= 0) 
		setpageval(find_reg(varname));		// bytes
	else
		setpageval(find_low_reg(varname));	// words
}

// returns 1 if given address is accessable from all data pages
on_all_banks(int addr)
{
	int i;

	for (i = 0; i < NextAll; i++)
	{
		if (addr >= AllBanksStart[i] && addr <= AllBanksEnd[i])
			return(1);
	}
	return(0);
}

// set the correct page (if necessary) for a given variable address
setpageval(int addr)
{
	if (!on_all_banks(addr))
		setpagenum(addr / DataPageSize);
}

// forces a new page to be set
forcepagenum(int newpage)
{
	reset_datapage();
	setpagenum(newpage);
}

// set the correct page (if necessary) for given page number
setpagenum(int newpage)
{
	int preg = -1;	// assume not found

	// only of 2 or more pages
	if (!TwoOrMorePages)
		return;

	if (CodeWidth == 12)
		preg = find_reg("FSR");
	else if (CodeWidth == 14)
		preg = find_reg("STATUS");

	if (preg < 0)
	{
		error("Error   %s %d : Invalid data page select register\n", INFILE, NR);
		return;
	}	

	if (datapagenum != newpage)
	{
		datapagenum = newpage;
		if (InsideExp)
			PageChanged[Level] = 1;	// ONLY inside EXP !!!

		if (datapagenum == 0)
		{
			Opcode("bcf", preg, 5, "");
			if (DidBit6)
			{
				Opcode("bcf", preg, 6, "");
			}
		}
		else if (datapagenum == 1)
		{
			Opcode("bsf", preg, 5, "");
			if (DidBit6)
			{
				Opcode("bcf", preg, 6, "");
			}
		}
		else if (datapagenum == 2)
		{
			Opcode("bcf", preg, 5, "");
			Opcode("bsf", preg, 6, "");
			DidBit6 = 1;
		}
		else if (datapagenum == 3)
		{
			Opcode("bsf", preg, 5, "");
			Opcode("bsf", preg, 6, "");
			DidBit6 = 1;
		}
		else if (datapagenum != -1)
		{
			error("Error   %s %d : Invalid data page %d\n", INFILE, NR, datapagenum);
			return;
		}
	}
}

// returns '0' - '9' for first 10 numbers, then 'A' - 'Z' for second 26 
// then 'a' - 'z' then '*'
char get_proc_code(int num)
{
	if (num < 0)
		return('#');	// root
	if (num < 10)
		return('0'+num);
	else if (num < 36)
		return('A'+(num-10));
	else if (num < 62)
		return('a'+(num-36));
	else
		return('*');	// outside range
}

// given an address, find the procedure to which it belongs [or -1 if root]
addr_is_inside_proc(int addr)
{
	int i;

	for (i = 0; i < NextProc; i++)
	{
		if (ProcCodeEnd[i] > 0 && strlen(Proc[i]) && ProcCodeStart[i] <= addr && ProcCodeEnd[i] >= addr)
				return(i);
	}
	return(-1);
}

find_free_space()
{
	int i, totalfree = 0;
	int TOP = CodePageSize * (TopCodePage+1);

	for (i = 0; i < TOP; i++)
	{
		if (!OC[i].oc)				// 4/8/99 - removed length check
			totalfree++;
	}
	return(totalfree);
}

find_low_reg(char *str)
{
	char *buf = new(L_FLDLEN);
	int r;

	strcpy(buf, "_L_");
	strcat(buf, str);
	r = find_reg(buf);
	free(buf);
	return(r);
}

find_hi_reg(char *str)
{
	char *buf = new(L_FLDLEN);
	int r;

	strcpy(buf, "_H_");
	strcat(buf, str);
	r = find_reg(buf);
	free(buf);
	return(r);
}

/*----------------------------------------------------------------------
 * Returns register number for 'str' or -1 if not a known register
 *----------------------------------------------------------------------*/
find_reg(char *str)
{
	int i, strln = 0;

	// find end of given string
	while (isalnum(str[strln]) ||
	       str[strln] == '_' ||
	       str[strln] == '[' ||
	       str[strln] == ']') strln++;

	// look at CPU registers
	for (i = 0; i < NextReg; i++)
	{
		if (strlen(Reg[i]) == strln && strncmp(str, Reg[i], strln) == 0)
			return(RegAddr[i]);
	}

	// look at VARIABLES
	for (i = 0; i < NextVar; i++)
	{
		if (strlen(Variable[i]) == strln && strncmp(Variable[i], str, strln) == 0)
		{
			VarUsed[i] = 1;		// flag as referenced at least once
			return(VarAddr[i]);
		}
	}
	return(-1);
}

//
// Tells which variables are not used
//
show_unused_vars()
{
	int i;

	for (i = 0; i < NextVar; i++)
	{
		if (!VarUsed[i])
		{
			if (strncmp(Variable[i], "_H_", 3) == 0)
			{
				warning("Warning %s - : Word '%s' defined but never used\n", MainFile, &Variable[i][3]);
				i++;
			}
			else
				warning("Warning %s - : Byte '%s' defined but never used\n", MainFile, Variable[i]);
		}
		else if (UsageCount[i] == 1)
		{
			if (strncmp(Variable[i], "_H_", 3) == 0)
			{
				warning("*NOTE* %s - : Word '%s' used only once\n", MainFile, &Variable[i][3]);
				i++;
			}
			else
				warning("*NOTE* %s - : Byte '%s' used only once\n", MainFile, Variable[i]);
		}
	}
}

//
// len has already been calculated for length of 0s & 1s + B
//
get_binary(char ptr[], int len)
{
	int val = 0, i, mul, index;

	index = len - 1;
	mul = 1;
	for (i = 0; i < len; i++)
	{
		val += (ptr[index] - '0') * mul;
		mul *= 2;
		index--;
	}
	return(val);
}

isbin(char ch)
{
	if (ch == '0' || ch == '1')
		return(1);
	else
		return(0);
}

ishex(char ch)
{
	if (ch >= '0' && ch <= '9')
		return(1);
	else if (toupper(ch) >= 'A' && toupper(ch) <= 'F')
		return(1);
	else
		return(0);
}

//
// Valid formats: xxxxxxxxB for Binary, 0xxH for hexadecimal, nnn for decimal
// NOTES: 1 - The first character MUST be a number
//        2 - Binary numbers may be 1 to 8 digits long, hex 1 or 2 and decimal
//            1, 2 or 3 digits long
//
unsigned int get_value(char ptr[])
{
	char *buf = new(L_FLDLEN);
	unsigned int len, val, i;

	// make sure first char is a digit
	if (!isdigit(ptr[0]))
	{
		error("Error   %s %d : First character must be a digit in '%s'\n", INFILE, NR, ptr);
		free(buf);
		return(0);
	}

	// convert hex values
	len = 0;
	while (ishex(ptr[len])) len++;
	if (len && len < 20 && toupper(ptr[len]) == 'H')
	{
		sprintf(buf, "0x%s", ptr);	// add the '0x' postfix
		sscanf(buf, "%x", &val);
		free(buf);
		return(val);
	}

	// convert binary values
	len = 0;
	while (isbin(ptr[len])) len++;
//	if (len && len < 9 && toupper(ptr[len]) == 'B')
	if (len == 8)
	{
		if (toupper(ptr[len]) != 'B')
			warning("Warning %s %d : String '%s' has no 'b' appended but read as binary value '%u'\n", INFILE, NR, ptr, get_binary(ptr, len));
		free(buf);
		return(get_binary(ptr, len));
	}
	else if (len >= 6)
	{
		error("Error   %s %d : String '%s' is %d bits long. Must be 8 bits long (with 'b' appended)\n", INFILE, NR, ptr, len);
		free(buf);
		return(0);
	}

	// warn about ill-formed binary values
	else if (len > 0 && toupper(ptr[len]) == 'B')
	{
		warning("Warning %s %d : String '%s' taken as decimal value '%u'\n", INFILE, NR, ptr, atoi(ptr));
	}

	// convert decimal values
	len = 0;
	while (isdigit(ptr[len])) len++;
//	if (len && len < 20 && !isalnum(ptr[len]))
	if (len && len < 6)
	{
		val = atoi(ptr);
		free(buf);
		return(val);
	}

	error("Error   %s %d : Invalid numeric constant '%s'\n", INFILE, NR, ptr);
	free(buf);
	return(0);
}

plus(char *start)
{
	char *buf1 = new(L_FLDLEN);
	char *buf2 = new(L_FLDLEN);
	unsigned int value;
	int addr;
	char *ptr;

	ptr = strchr(start, '+');
	*ptr = '\0';
	ptr++;
	ptr++;

	inside_copy(buf1, start);
	inside_copy(buf2, ptr);
	
	if (find_reg(buf1) >= 0)
	{
		if (find_low_reg(buf2) >= 0)
			MayNotMix();
		else if (is_proc(buf2))
		{
			do_call(buf2);
			setpage(buf1);
			Opcode("addwf", find_reg(buf1), 1, "");
		}
		else if (buf2[0] == '&' && (addr = find_reg(&buf2[1])) >= 0)
		{
			Opcode("movlw", addr, 0, "");
			setpage(buf1);
			Opcode("addwf", find_reg(buf1), 1, "");
		}
		else if (find_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("addwf", find_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			Opcode("movlw", value & 0xff, 0, "");
			setpage(buf1);
			Opcode("addwf", find_reg(buf1), 1, "");
		}
	}
	else if (find_low_reg(buf1) >= 0)
	{
		if (not_allowed(buf2))
		{
			free(buf1);
			free(buf2);
			return(0);
		}
		else if (find_low_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_hi_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("addwf", find_hi_reg(buf1), 1, "");

			setpage(buf2);
			Opcode("movf", find_low_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("addwf", find_low_reg(buf1), 1, "");
			
			Opcode("btfsc", 3, 0, "");	// CARRY set
			Opcode("incf", find_hi_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			Opcode("movlw", value >> 8, 0, "");
			setpage(buf1);
			Opcode("addwf", find_hi_reg(buf1), 1, "");

			Opcode("movlw", value & 0xff, 0, "");
			setpage(buf1);
			Opcode("addwf", find_low_reg(buf1), 1, "");
			
			Opcode("btfsc", 3, 0, "");	// CARRY set
			Opcode("incf", find_hi_reg(buf1), 1, "");
		}
	}
	else
	{
		Undefined(buf1);
		free(buf1);
		free(buf2);
		return(0);
	}
	free(buf1);
	free(buf2);
	return(1);
}

minus(char *start)
{
	char *buf1 = new(L_FLDLEN);
	char *buf2 = new(L_FLDLEN);
	unsigned int value;
	int addr;
	char *ptr;

	ptr = strchr(start, '-');
	*ptr = '\0';
	ptr++;
	ptr++;

	inside_copy(buf1, start);
	inside_copy(buf2, ptr);
	
	if (find_reg(buf1) >= 0)
	{
		if (find_low_reg(buf2) >= 0)
			MayNotMix();
		else if (is_proc(buf2))
		{
			do_call(buf2);
			setpage(buf1);
			Opcode("subwf", find_reg(buf1), 1, "");
		}
		else if (buf2[0] == '&' && (addr = find_reg(&buf2[1])) >= 0)
		{
			Opcode("movlw", addr, 0, "");
			setpage(buf1);
			Opcode("subwf", find_reg(buf1), 1, "");
		}
		else if (find_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("subwf", find_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			Opcode("movlw", value & 0xff, 0, "");
			setpage(buf1);
			Opcode("subwf", find_reg(buf1), 1, "");
		}
	}
	else if (find_low_reg(buf1) >= 0)
	{
		if (not_allowed(buf2))
		{
			free(buf1);
			free(buf2);
			return(0);
		}
		else if (find_low_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_hi_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("subwf", find_hi_reg(buf1), 1, "");

			setpage(buf2);
			Opcode("movf", find_low_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("subwf", find_low_reg(buf1), 1, "");
			
			Opcode("btfss", 3, 0, "");	// CARRY set
			Opcode("decf", find_hi_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			Opcode("movlw", value >> 8, 0, "");
			setpage(buf1);
			Opcode("subwf", find_hi_reg(buf1), 1, "");

			Opcode("movlw", value & 0xff, 0, "");
			setpage(buf1);
			Opcode("subwf", find_low_reg(buf1), 1, "");
			
			Opcode("btfss", 3, 0, "");	// CARRY set
			Opcode("decf", find_hi_reg(buf1), 1, "");
		}
	}
	else
	{
		Undefined(buf1);
		free(buf1);
		free(buf2);
		return(0);
	}
	free(buf1);
	free(buf2);
	return(1);
}

bitand(char *start)
{
	char *buf1 = new(L_FLDLEN);
	char *buf2 = new(L_FLDLEN);
	unsigned int value;
	int addr;
	char *ptr;

	ptr = strchr(start, '&');
	*ptr = '\0';
	ptr++;
	ptr++;

	inside_copy(buf1, start);
	inside_copy(buf2, ptr);
	
	if (find_reg(buf1) >= 0)
	{
		if (find_low_reg(buf2) >= 0)
			MayNotMix();
		else if (is_proc(buf2))
		{
			do_call(buf2);
			setpage(buf1);
			Opcode("andwf", find_reg(buf1), 1, "");
		}
		else if (buf2[0] == '&' && (addr = find_reg(&buf2[1])) >= 0)
		{
			Opcode("movlw", addr, 0, "");
			setpage(buf1);
			Opcode("andwf", find_reg(buf1), 1, "");
		}
		else if (find_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("andwf", find_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			setpage(buf1);
			if (value != 0)
			{
				Opcode("movlw", value, 0, "");
				Opcode("andwf", find_reg(buf1), 1, "");
			}
			else
				Opcode("clrf", find_reg(buf1), 0, "");
		}
	}
	else if (find_low_reg(buf1) >= 0)
	{
		if (not_allowed(buf2))
		{
			free(buf1);
			free(buf2);
			return(0);
		}
		else if (find_low_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_hi_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("andwf", find_hi_reg(buf1), 1, "");

			setpage(buf2);
			Opcode("movf", find_low_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("andwf", find_low_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			setpage(buf1);
			if (value != 0)
			{
				Opcode("movlw", value >> 8, 0, "");
				setpage(buf1);
				Opcode("andwf", find_hi_reg(buf1), 1, "");
	
				Opcode("movlw", value & 0xff, 0, "");
				setpage(buf1);
				Opcode("andwf", find_low_reg(buf1), 1, "");
			}
			else
			{
				Opcode("clrf", find_low_reg(buf1), 0, "");
				Opcode("clrf", find_hi_reg(buf1), 0, "");
			}
		}
	}
	else
	{
		Undefined(buf1);
		free(buf1);
		free(buf2);
		return(0);
	}
	free(buf1);
	free(buf2);
	return(1);
}

bitnand(char *start)
{
	char *buf1 = new(L_FLDLEN);
	char *buf2 = new(L_FLDLEN);
	unsigned int value;
	int addr, i;
	char *ptr;

	ptr = strchr(start, '!');
	*ptr = '\0';
	ptr++;
	ptr++;
	ptr++;

	inside_copy(buf1, start);
	inside_copy(buf2, ptr);
	
	if (find_reg(buf1) >= 0)
	{
		if (find_low_reg(buf2) >= 0)
			MayNotMix();
		else if (is_proc(buf2))
		{
			do_call(buf2);
			setpage(buf1);
			Opcode("andwf", find_reg(buf1), 1, "");
			Opcode("comf", find_reg(buf1), 1, "");
		}
		else if (buf2[0] == '&' && (addr = find_reg(&buf2[1])) >= 0)
		{
			Opcode("movlw", addr, 0, "");
			setpage(buf1);
			Opcode("andwf", find_reg(buf1), 1, "");
			Opcode("comf", find_reg(buf1), 1, "");
		}
		else if (find_reg(buf2) >= 0)
		{
			if (IsPORT(buf1))
			{
				PortError();
				free(buf1);
				free(buf2);
				return(0);
			}
			setpage(buf2);
			Opcode("movf", find_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("andwf", find_reg(buf1), 1, "");
			Opcode("comf", find_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			if (value != 0)
			{
				setpage(buf1);
				Opcode("comf", find_reg(buf1), 0, "");
				Opcode("iorlw", (255 - value) & 0xff, 0, "");
				Opcode("movwf", find_reg(buf1), 0, "");
			}
			else
			{
				Opcode("movlw", 255, 0, "");
				setpage(buf1);
				Opcode("movwf", find_reg(buf1), 0, "");
			}
		}
	}
	else if (find_low_reg(buf1) >= 0)
	{
		if (not_allowed(buf2))
		{
			free(buf1);
			free(buf2);
			return(0);
		}
		else if (find_low_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_hi_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("andwf", find_hi_reg(buf1), 1, "");
			Opcode("comf", find_hi_reg(buf1), 1, "");

			setpage(buf2);
			Opcode("movf", find_low_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("andwf", find_low_reg(buf1), 1, "");
			Opcode("comf", find_low_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			if (value != 0)
			{
				setpage(buf1);
				Opcode("comf", find_hi_reg(buf1), 0, "");
				Opcode("iorlw", (255 - (value >> 8)) & 0xff, 0, "");
				Opcode("movwf", find_hi_reg(buf1), 0, "");

				Opcode("comf", find_low_reg(buf1), 0, "");
				Opcode("iorlw", (255 - (value & 0xff)) & 0xff, 0, "");
				Opcode("movwf", find_low_reg(buf1), 0, "");
			}
			else
			{
				Opcode("movlw", 255, 0, "");
				setpage(buf1);
				Opcode("movwf", find_hi_reg(buf1), 0, "");
				Opcode("movwf", find_low_reg(buf1), 0, "");
			}
		}
	}
	else
	{
		Undefined(buf1);
		free(buf1);
		free(buf2);
		return(0);
	}
	free(buf1);
	free(buf2);
	return(1);
}

bitor(char *start)
{
	char *buf1 = new(L_FLDLEN);
	char *buf2 = new(L_FLDLEN);
	unsigned int value;
	int addr;
	char *ptr;

	ptr = strchr(start, '|');
	*ptr = '\0';
	ptr++;
	ptr++;

	inside_copy(buf1, start);
	inside_copy(buf2, ptr);
	
	if (find_reg(buf1) >= 0)
	{
		if (find_low_reg(buf2) >= 0)
			MayNotMix();
		else if (is_proc(buf2))
		{
			do_call(buf2);
			setpage(buf1);
			Opcode("iorwf", find_reg(buf1), 1, "");
		}
		else if (buf2[0] == '&' && (addr = find_reg(&buf2[1])) >= 0)
		{
			Opcode("movlw", addr, 0, "");
			setpage(buf1);
			Opcode("iorwf", find_reg(buf1), 1, "");
		}
		else if (find_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("iorwf", find_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			if (value != 0)
			{
				Opcode("movlw", value, 0, "");
				setpage(buf1);
				Opcode("iorwf", find_reg(buf1), 1, "");
			}
		}
	}
	else if (find_low_reg(buf1) >= 0)
	{
		if (not_allowed(buf2))
		{
			free(buf1);
			free(buf2);
			return(0);
		}
		else if (find_low_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_hi_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("iorwf", find_hi_reg(buf1), 1, "");

			setpage(buf2);
			Opcode("movf", find_low_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("iorwf", find_low_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			setpage(buf1);
			if (value != 0)
			{
				Opcode("movlw", value >> 8, 0, "");
				setpage(buf1);
				Opcode("iorwf", find_hi_reg(buf1), 1, "");
	
				Opcode("movlw", value & 0xff, 0, "");
				setpage(buf1);
				Opcode("iorwf", find_low_reg(buf1), 1, "");
			}
		}
	}
	else
	{
		Undefined(buf1);
		free(buf1);
		free(buf2);
		return(0);
	}
	free(buf1);
	free(buf2);
	return(1);
}

bitnor(char *start)
{
	char *buf1 = new(L_FLDLEN);
	char *buf2 = new(L_FLDLEN);
	unsigned int value;
	int addr;
	char *ptr;

	ptr = strchr(start, '!');
	*ptr = '\0';
	ptr++;
	ptr++;
	ptr++;

	inside_copy(buf1, start);
	inside_copy(buf2, ptr);
	
	if (find_reg(buf1) >= 0)
	{
		if (find_low_reg(buf2) >= 0)
			MayNotMix();
		else if (is_proc(buf2))
		{
			do_call(buf2);
			setpage(buf1);
			Opcode("iorwf", find_reg(buf1), 1, "");
			Opcode("comf", find_reg(buf1), 1, "");
		}
		else if (buf2[0] == '&' && (addr = find_reg(&buf2[1])) >= 0)
		{
			Opcode("movlw", addr, 0, "");
			setpage(buf1);
			Opcode("iorwf", find_reg(buf1), 1, "");
			Opcode("comf", find_reg(buf1), 1, "");
		}
		else if (find_reg(buf2) >= 0)
		{
			if (IsPORT(buf1))
			{
				PortError();
				free(buf1);
				free(buf2);
				return(0);
			}
			setpage(buf2);
			Opcode("movf", find_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("iorwf", find_reg(buf1), 1, "");
			Opcode("comf", find_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			setpage(buf1);
			if (value != 0)
			{
				Opcode("comf", find_reg(buf1), 0, "");
				Opcode("andlw", (255 - value) & 0xff, 0, "");
				Opcode("movwf", find_reg(buf1), 0, "");
			}
			else
				Opcode("comf", find_reg(buf1), 1, "");
		}
	}
	else if (find_low_reg(buf1) >= 0)
	{
		if (not_allowed(buf2))
		{
			free(buf1);
			free(buf2);
			return(0);
		}
		else if (find_low_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_hi_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("iorwf", find_hi_reg(buf1), 1, "");
			Opcode("comf", find_hi_reg(buf1), 1, "");

			setpage(buf2);
			Opcode("movf", find_low_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("iorwf", find_low_reg(buf1), 1, "");
			Opcode("comf", find_low_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			setpage(buf1);
			if (value != 0)
			{
				Opcode("comf", find_hi_reg(buf1), 0, "");
				Opcode("andlw", (255 - (value >> 8)) & 0xff, 0, "");
				Opcode("movwf", find_hi_reg(buf1), 0, "");

				Opcode("comf", find_low_reg(buf1), 0, "");
				Opcode("andlw", (255 - (value & 0xff)) & 0xff, 0, "");
				Opcode("movwf", find_low_reg(buf1), 0, "");
			}
			else
			{
				Opcode("comf", find_hi_reg(buf1), 1, "");
				Opcode("comf", find_low_reg(buf1), 1, "");
			}
		}
	}
	else
	{
		Undefined(buf1);
		free(buf1);
		free(buf2);
		return(0);
	}
	free(buf1);
	free(buf2);
	return(1);
}

bitxor(char *start)
{
	char *buf1 = new(L_FLDLEN);
	char *buf2 = new(L_FLDLEN);
	unsigned int value;
	int addr;
	char *ptr;

	ptr = strchr(start, '^');
	*ptr = '\0';
	ptr++;
	ptr++;

	inside_copy(buf1, start);
	inside_copy(buf2, ptr);
	
	if (find_reg(buf1) >= 0)
	{
		if (find_low_reg(buf2) >= 0)
			MayNotMix();
		else if (is_proc(buf2))
		{
			do_call(buf2);
			setpage(buf1);
			Opcode("xorwf", find_reg(buf1), 1, "");
		}
		else if (buf2[0] == '&' && (addr = find_reg(&buf2[1])) >= 0)
		{
			Opcode("movlw", addr, 0, "");
			setpage(buf1);
			Opcode("xorwf", find_reg(buf1), 1, "");
		}
		else if (find_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("xorwf", find_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			if (value != 0)
			{
				Opcode("movlw", value, 0, "");
				setpage(buf1);
				Opcode("xorwf", find_reg(buf1), 1, "");
			}
		}
	}
	else if (find_low_reg(buf1) >= 0)
	{
		if (not_allowed(buf2))
		{
			free(buf1);
			free(buf2);
			return(0);
		}
		else if (find_low_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_hi_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("xorwf", find_hi_reg(buf1), 1, "");

			setpage(buf2);
			Opcode("movf", find_low_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("xorwf", find_low_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			if (value != 0)
			{
				setpage(buf1);
				Opcode("movlw", value >> 8, 0, "");
				Opcode("xorwf", find_hi_reg(buf1), 1, "");

				Opcode("movlw", value & 0xff, 0, "");
				Opcode("xorwf", find_low_reg(buf1), 1, "");
			}
		}
	}
	else
	{
		Undefined(buf1);
		free(buf1);
		free(buf2);
		return(0);
	}
	free(buf1);
	free(buf2);
	return(1);
}

bitxnor(char *start)
{
	char *buf1 = new(L_FLDLEN);
	char *buf2 = new(L_FLDLEN);
	unsigned int value;
	int addr;
	char *ptr;

	ptr = strchr(start, '!');
	*ptr = '\0';
	ptr++;
	ptr++;
	ptr++;

	inside_copy(buf1, start);
	inside_copy(buf2, ptr);
	
	if (find_reg(buf1) >= 0)
	{
		if (find_low_reg(buf2) >= 0)
			MayNotMix();
		else if (IsPORT(buf1))
		{
			PortError();
			free(buf1);
			free(buf2);
			return(0);
		}
		if (is_proc(buf2))
		{
			do_call(buf2);
			setpage(buf1);
			Opcode("xorwf", find_reg(buf1), 1, "");
			Opcode("comf", find_reg(buf1), 1, "");
		}
		else if (buf2[0] == '&' && (addr = find_reg(&buf2[1])) >= 0)
		{
			Opcode("movlw", addr, 0, "");
			setpage(buf1);
			Opcode("xorwf", find_reg(buf1), 1, "");
			Opcode("comf", find_reg(buf1), 1, "");
		}
		else if (find_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("xorwf", find_reg(buf1), 1, "");
			Opcode("comf", find_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			setpage(buf1);
			if (value != 0)
			{
				Opcode("movlw", value, 0, "");
				Opcode("xorwf", find_reg(buf1), 1, "");
			}
			Opcode("comf", find_reg(buf1), 1, "");
		}
	}
	else if (find_low_reg(buf1) >= 0)
	{
		if (not_allowed(buf2))
		{
			free(buf1);
			free(buf2);
			return(0);
		}
		else if (find_low_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_hi_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("xorwf", find_hi_reg(buf1), 1, "");
			Opcode("comf", find_hi_reg(buf1), 1, "");

			setpage(buf2);
			Opcode("movf", find_low_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("xorwf", find_low_reg(buf1), 1, "");
			Opcode("comf", find_low_reg(buf1), 1, "");
		}
		else
		{
			value = get_value(buf2);
			setpage(buf1);
			if (value != 0)
			{
				Opcode("movlw", value >> 8, 0, "");
				Opcode("xorwf", find_hi_reg(buf1), 1, "");
				Opcode("comf", find_hi_reg(buf1), 1, "");

				Opcode("movlw", value & 0xff, 0, "");
				Opcode("xorwf", find_low_reg(buf1), 1, "");
				Opcode("comf", find_low_reg(buf1), 1, "");
			}
		}
	}
	else
	{
		Undefined(buf1);
		free(buf1);
		free(buf2);
		return(0);
	}
	free(buf1);
	free(buf2);
	return(1);
}

set_parameter(char *parm)
{
	unsigned int value;
	int addr;
	char *ptr;

	if (!strlen(parm))
		return;
	else if (is_proc(parm))
	{
		do_call(parm);
	}
	else if (parm[0] == '&' && (addr = find_reg(&parm[1])) >= 0)
	{
		Opcode("movlw", addr, 0, "");
	}
	else if (find_reg(parm) >= 0)
	{
		setpage(parm);
		Opcode("movf", find_reg(parm), 0, "");
	}
	else
	{
		value = get_value(parm);
		if (value == 0)
		{
			Opcode("clrw", 0, 0, "");
		}
		else
			Opcode("movlw", value, 0, "");
	}
}

set_equal(char *start)
{
	char *buf1 = new(L_FLDLEN);
	char *buf2 = new(L_FLDLEN);
	unsigned int value;
	int addr;
	char *ptr;

	ptr = strchr(start, '=');
	*ptr = '\0';
	ptr++;

	inside_copy(buf1, start);
	inside_copy(buf2, ptr);

	if (find_reg(buf1) >= 0)
	{
		if (find_low_reg(buf2) >= 0)
		{
			MayNotMix();
		}
		else if (is_proc(buf2))
		{
			do_call(buf2);
			setpage(buf1);
			Opcode("movwf", find_reg(buf1), 0, "");
		}
		else if (buf2[0] == '&' && (addr = find_reg(&buf2[1])) >= 0)
		{
			Opcode("movlw", addr, 0, "");
			setpage(buf1);
			Opcode("movwf", find_reg(buf1), 0, "");
		}
		else if (find_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("movwf", find_reg(buf1), 0, "");
		}
		else
		{
			value = get_value(buf2);
			if (value == 0)
			{
				setpage(buf1);
				Opcode("clrf", find_reg(buf1), 0, "");
			}
			else
			{
				Opcode("movlw", value, 0, "");
				setpage(buf1);
				Opcode("movwf", find_reg(buf1), 0, "");
			}
		}
	}
	else if (find_low_reg(buf1) >= 0)
	{
		if (not_allowed(buf2))
		{
			free(buf1);
			free(buf2);
			return(0);
		}
		else if (find_low_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("movf", find_low_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("movwf", find_low_reg(buf1), 0, "");
			setpage(buf2);
			Opcode("movf", find_hi_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("movwf", find_hi_reg(buf1), 0, "");
		}
		else
		{
			value = get_value(buf2);
			if (value == 0)
			{
				setpage(buf1);
				Opcode("clrf", find_low_reg(buf1), 0, "");
				Opcode("clrf", find_hi_reg(buf1), 0, "");
			}
			else
			{
				setpage(buf1);
				Opcode("movlw", value & 0xff, 0, "");
				Opcode("movwf", find_low_reg(buf1), 0, "");
				Opcode("movlw", value >> 8, 0, "");
				Opcode("movwf", find_hi_reg(buf1), 0, "");
			}
		}
	}
	else
	{
		Undefined(buf1);
		free(buf1);
		free(buf2);
		return(0);
	}
	free(buf1);
	free(buf2);
	return(1);
}

set_notequal(char *start)
{
	char *buf1 = new(L_FLDLEN);
	char *buf2 = new(L_FLDLEN);
	unsigned int value;
	char *ptr;

	ptr = strchr(start, '!');
	*ptr = '\0';
	ptr++;
	ptr++;

	inside_copy(buf1, start);
	inside_copy(buf2, ptr);
	
	if (find_reg(buf1) >= 0)
	{
		if (find_low_reg(buf2) >= 0)
		{
			MayNotMix();
		}
		else if (is_proc(buf2))
		{
			do_call(buf2);
			setpage(buf1);
			Opcode("movwf", find_reg(buf1), 0, "");
			Opcode("comf", find_reg(buf1), 1, "");
		}
		else if (find_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("comf", find_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("movwf", find_reg(buf1), 0, "");
		}
		else
		{
			value = get_value(buf2) & 0xff;
			value = (255 - value) & 0xff;	// form compliment
			if (value == 0)
			{
				setpage(buf1);
				Opcode("clrf", find_reg(buf1), 0, "");
			}
			else
			{
				Opcode("movlw", value, 0, "");
				setpage(buf1);
				Opcode("movwf", find_reg(buf1), 0, "");
			}
		}
	}
	else if (find_low_reg(buf1) >= 0)
	{
		if (not_allowed(buf2))
		{
			free(buf1);
			free(buf2);
			return(0);
		}
		else if (find_low_reg(buf2) >= 0)
		{
			setpage(buf2);
			Opcode("comf", find_low_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("movwf", find_low_reg(buf1), 0, "");
			setpage(buf2);
			Opcode("comf", find_hi_reg(buf2), 0, "");
			setpage(buf1);
			Opcode("movwf", find_hi_reg(buf1), 0, "");
		}
		else
		{
			value = get_value(buf2);
			if (value == 0)
			{
				setpage(buf1);
				Opcode("clrf", find_low_reg(buf1), 0, "");
				Opcode("clrf", find_hi_reg(buf1), 0, "");
			}
			else
			{
				setpage(buf1);
				value = get_value(buf2) & 0xff;
				value = (255 - value) & 0xff;
				Opcode("movlw", value & 0xff, 0, "");
				Opcode("movwf", find_low_reg(buf1), 0, "");
				value = get_value(buf2) >> 8;
				value = (255 - value) & 0xff;
				Opcode("movlw", value, 0, "");
				Opcode("movwf", find_hi_reg(buf1), 0, "");
			}
		}
	}
	else
	{
		Undefined(buf1);
		free(buf1);
		free(buf2);
		return(0);
	}
	free(buf1);
	free(buf2);
	return(1);
}

not_allowed(char *buf)
{
	if (is_proc(buf))
	{
		ForBytesOnly();
		return(1);
	}
	else if (buf[0] == '&')
	{
		ForBytesOnly();
		return(1);
	}
	else if (find_reg(buf) >= 0)
	{
		MayNotMix();
		return(1);
	}
	return(0);
}

gen_inc(char *ptr)
{
	int nreg = find_reg(ptr);

	setpage(ptr);
	if (nreg >= 0)
		Opcode("incf", find_reg(ptr), 1, "");
	else if ((nreg = find_low_reg(ptr)) >= 0)
	{
		Opcode("incf", find_low_reg(ptr), 1, "");
		Opcode("btfsc", 3, 2, "");
		Opcode("incf", find_hi_reg(ptr), 1, "");
	}
}

gen_dec(char *ptr)
{
	int nreg = find_reg(ptr);
	
	setpage(ptr);
	if (nreg >= 0)
		Opcode("decf", find_reg(ptr), 1, "");
	else if ((nreg = find_low_reg(ptr)) >= 0)
	{
		Opcode("decf", find_low_reg(ptr), 1, "");
		Opcode("comf", find_low_reg(ptr), 0, "");
		Opcode("btfsc", 3, 2, "");
		Opcode("decf", find_hi_reg(ptr), 1, "");
	}
}

gen_left(char *ptr)
{
	int nreg = find_reg(ptr);

	setpage(ptr);
	if (nreg >= 0)
		Opcode("rlf", find_reg(ptr), 1, "");
	else if ((nreg = find_low_reg(ptr)) >= 0)
	{
		Opcode("rlf", find_low_reg(ptr), 1, "");
		Opcode("rlf", find_hi_reg(ptr), 1, "");
	}
}

gen_right(char *ptr)
{
	int nreg = find_reg(ptr);

	setpage(ptr);
	if (nreg >= 0)
		Opcode("rrf", find_reg(ptr), 1, "");
	else if ((nreg = find_low_reg(ptr)) >= 0)
	{
		Opcode("rrf", find_hi_reg(ptr), 1, "");
		Opcode("rrf", find_low_reg(ptr), 1, "");
	}
}

// returns 1 if name is a procedure
is_proc(char *name)
{
	int i;

	if (name[strlen(name)-1] == ')')
		return(1);

	for (i = 0; i < NextProc; i++)
	{
		if (strcmp(Proc[i], name) == 0)
			return(1);
	}
	return(0);
}

new_proc(char *name)
{
	int i;
	char *ptr, *ptr2;

	datapagenum = 0;		// reset
	if (strchr(name, '('))
	{
		if (!strchr(name, ')') || strchr(name, ',') || strchr(name, ' '))
		{
			error("Error   %s %d : Procedure declaration syntax error\n", INFILE, NR);
			return;
		}
		ptr = strchr(name, '(');
		ptr2 = strrchr(name, ')');
		*ptr2 = '\0';			// kill ')'
		*ptr = '\0';			// kill '('
		_Label(name);
		ptr++;				// point to parameter
		if (find_reg(ptr) >= 0)
		{
			setpage(ptr);
			Opcode("movwf", find_reg(ptr), 0, "");
		}
		else if (strlen(ptr) > 0)
			error("Error   %s %d : Parameter must be an existing variable\n", INFILE, NR);
	}
	else
		_Label(name);
	

	if (busy_with_proc >= 0 || busy_with_int >= 0)
	{
		error("Error   %s %d : Procedure inside a procedure not allowed\n", INFILE, NR);
		return;
	}
	if (BlockType[0] != BNONE || BlockType[1] != BNONE)
	{
		error("Error   %s %d : Procedures cannot be declared inside a block\n", INFILE, NR);
		return;
	}
	if (NextProc == MAXPROCS)
	{
		error("Error   %s %d : Procedure limit exceeded\n", INFILE, NR);
		return;
	}

	// see if name already exists (i.e. has been called already)
	for (i = 0; i < NextProc; i++)
	{
		if (strcmp(Proc[i], name) == 0)
			break;
	}

	// see if procedure already defined
	if ((ProcStatus[i] & PROC_EXIST))
	{
		error("Error   %s %d : Procedure '%s' already defined\n", INFILE, NR, name);
		return;
	}

	// not called yet - so add name to list
	if (i == NextProc)
	{
		if (NextProc < MAXPROCS)
		{
			Proc[NextProc] = new(strlen(name) + 1);
			strcpy(Proc[NextProc], name);
			NextProc++;
		}
		else
		{
			error("Error   %s %d : Procedure limit exceeded: No space for '%s'\n", INFILE, NR, name);
			return;
		}
	}
	ProcFile[i] = GetFileIndex(INFILE);
	ProcStatus[i] |= PROC_EXIST;	// procedure now exists
	ProcStart[i] = NR;		// line where proc starts
	ProcCodeStart[i] = OCaddr;	// address where proc starts
	busy_with_proc = i;
	proc_count++;			// count 'procs'
}

// returns 1 if procedure is already in the procedures list OR forward-
// referenced with 'use_lib'
is_referenced(char *name)
{
	int i;
	
	for (i = 0; i < NextProc; i++)
	{
		if (strcmp(Proc[i], name) == 0)
			return(1);
	}
	for (i = 0; i < NextLib; i++)
	{
		if (strcmp(LibName[i], name) == 0)
			return(1);
	}
	return(0);
}

new_lib(char *name)
{
	int i;
	char *ptr, *ptr2;

	datapagenum = 0;		// reset
	if (strchr(name, '('))
	{
		if (!strchr(name, ')') || strchr(name, ',') || strchr(name, ' '))
		{
			error("Error   %s %d : Procedure declaration syntax error\n", INFILE, NR);
			return;
		}
		ptr = strchr(name, '(');
		ptr2 = strrchr(name, ')');
		*ptr2 = '\0';			// kill ')'
		*ptr = '\0';			// kill '('

		if (!is_referenced(name))
		{
			SkipTillEnd = 1;
			return;
		}
		
		_Label(name);
		ptr++;				// point to parameter
		if (find_reg(ptr) >= 0)
		{
			setpage(ptr);
			Opcode("movwf", find_reg(ptr), 0, "");
		}
		else if (strlen(ptr) > 0)
			error("Error   %s %d : Parameter must be an existing variable\n", INFILE, NR);
	}
	else
	{
		if (!is_referenced(name))
		{
			SkipTillEnd = 1;
			return;
		}
		_Label(name);
	}

	if (busy_with_proc >= 0 || busy_with_int >= 0)
	{
		error("Error   %s %d : Procedure inside a procedure not allowed\n", INFILE, NR);
		return;
	}
	if (BlockType[0] != BNONE || BlockType[1] != BNONE)
	{
		error("Error   %s %d : Procedures cannot be declared inside a block\n", INFILE, NR);
		return;
	}
	if (NextProc == MAXPROCS)
	{
		error("Error   %s %d : Procedure limit exceeded\n", INFILE, NR);
		return;
	}

	// see if name already exists (i.e. has been called already)
	for (i = 0; i < NextProc; i++)
	{
		if (strcmp(Proc[i], name) == 0)
			break;
	}

	// see if procedure already defined
	if ((ProcStatus[i] & PROC_EXIST))
	{
		error("Error   %s %d : Procedure '%s' already defined\n", INFILE, NR, name);
		return;
	}

	// not called yet - so add name to list
	if (i == NextProc)
	{
		if (NextProc < MAXPROCS)
		{
			Proc[NextProc] = new(strlen(name) + 1);
			strcpy(Proc[NextProc], name);
			NextProc++;
		}
		else
		{
			error("Error   %s %d : Procedure limit exceeded: No space for '%s'\n", INFILE, NR, name);
			return;
		}
	}
	ProcFile[i] = GetFileIndex(INFILE);
	ProcStatus[i] |= PROC_EXIST;	// procedure now exists
	ProcStart[i] = NR;		// line where proc starts
	ProcCodeStart[i] = OCaddr;	// address where proc starts
	busy_with_proc = i;
	proc_count++;			// count 'procs'
}

do_call_parm(char *proc)
{
	char *buf = new(L_FLDLEN);

	if (strchr(proc, '('))
		do_call(proc);
	else
	{
		sprintf(buf, "%s()", proc);
		do_call(buf);
	}
	free(buf);
}

do_call(char *name)
{
	char *ptr, *ptr2;
	int i;

	ptr = strchr(name, '(');
	ptr2 = strrchr(name, ')');

	if (!ptr || !ptr2 || strchr(name, ',') || strchr(name, ' '))
	{
		error("Error   %s %d : Syntax error / Only one parameter allowed\n", INFILE, NR);
		return;
	}
	
	*ptr2 = '\0';			// kill ')'
	*ptr = '\0';			// kill '('
	ptr++;				// point to parameter
	set_parameter(ptr);		// setup W
	
	for (i = 0; i < NextProc; i++)
	{
		if (strcmp(Proc[i], name) == 0)
			break;
	}

	// not found yet - so add name so long
	if (i == NextProc)
	{
		if (NextProc < MAXPROCS)
		{
			Proc[NextProc] = new(strlen(name) + 1);
			strcpy(Proc[NextProc], name);
			NextProc++;
		}
		else
		{
			error("Error   %s %d : Procedure limit exceeded: No space for '%s'\n", INFILE, NR, name);
			return;
		}
	}
	ProcStatus[i] |= PROC_CALLED;	// proc now called
	ProcUsageCount[i]++;		// usage count
	ProcReturnsValue[i] = 1;	// assume proc returns a value
	setpagenum(0);			// so all procs start with p0
	Opcode("call", 0, 0, name);
}

do_endproc()
{
	int addr;
	char *ptr = strchr(S[1], '(');
	char *ptr2;

	if (busy_with_proc < 0)
	{
		error("Error   %s %d : No procedure defined\n", INFILE, NR);
		return;
	}
	if (BlockType[0] != BNONE || BlockType[1] != BNONE)
	{
		error("Error   %s %d : A IF/WHILE/REPEAT block is still active\n", INFILE, NR);
		return;
	}
	ProcEnd[busy_with_proc] = NR;		// line where proc ends

	if (!ptr && NF == 1)
	{
		setpagenum(0);			// reset to page 0
		Opcode("retlw", 0, 0, Proc[busy_with_proc]);
	}
	else if (NF > 2)
		OnlyOne();
	else
	{
		if (ptr)
		{
			ptr++;
			if (!strrchr(S[1], ')') || S[1][7] != '(')
			{
				error("Error   %s %d : Syntax error - Check brackets\n", INFILE, NR);
				return;
			}
		}
		else 
			ptr = S[2];
		
		if (isdigit(ptr[0]))
		{
			setpagenum(0);			// reset to page 0
			Opcode("retlw", get_value(ptr), 0, "");
		}
		else if ((addr = find_reg(ptr)) >= 0)
		{
			if (CodeWidth == 14)
			{
				setpage(ptr);
				Opcode("movf", addr, 0, "");
				setpagenum(0);		// reset to page 0
				Opcode("return", 0, 0, "");
			}
			else
				error("Error   %s %d : A 16C5x return value must be a CONSTANT only\n", INFILE, NR);
		}
		else if (is_proc(ptr))
		{
			if (CodeWidth == 14)
			{
				ptr2 = strrchr(S[1], ')');
				*ptr2 = '\0';
				do_call(ptr);
				setpagenum(0);		// reset to page 0
				Opcode("return", 0, 0, "");
			}
			else
				error("Error   %s %d : A 16C5x return value must be a CONSTANT only\n", INFILE, NR);
		}
		else if (find_low_reg(ptr) >= 0)
			error("Error   %s %d : Only BYTEs and CONSTANTs allowed\n", INFILE, NR);
		else
			error("Error   %s %d : Expression syntax error\n", INFILE, NR);
	}
	ProcCodeEnd[busy_with_proc] = OCaddr - 1;	// addr where proc ends
	busy_with_proc = -1;
}

do_return()
{
	int addr;
	char *ptr = strchr(S[1], '(');
	char *ptr2;
	
	if (busy_with_proc < 0)
	{
		error("Error   %s %d : No procedure defined\n", INFILE, NR);
		return;
	}

	if (!ptr && NF == 1)
	{
		setpagenum(0);		// reset to page 0
		Opcode("retlw", 0, 0, Proc[busy_with_proc]);
	}
	else if (NF > 2)
		OnlyOne();
	else
	{
		if (ptr)
		{
			ptr++;
			if (!strrchr(S[1], ')') || S[1][6] != '(')
			{
				error("Error   %s %d : Syntax error - Check brackets\n", INFILE, NR);
				return;
			}
		}
		else 
			ptr = S[2];
		
		if (isdigit(ptr[0]))
		{
			setpagenum(0);		// reset to page 0
			Opcode("retlw", get_value(ptr), 0, "");
		}
		else if ((addr = find_reg(ptr)) >= 0)
		{
			if (CodeWidth == 14)
			{
				setpage(ptr);
				Opcode("movf", addr, 0, "");
				setpagenum(0);		// reset to page 0
				Opcode("return", 0, 0, "");
			}
			else
				error("Error   %s %d : A 16C5x return value must be a CONSTANT only\n", INFILE, NR);
		}
		else if (is_proc(ptr))
		{
			if (CodeWidth == 14)
			{
				ptr2 = strrchr(S[1], ')');
				*ptr2 = '\0';
				do_call(ptr);
				setpagenum(0);		// reset to page 0
				Opcode("return", 0, 0, "");
			}
			else
				error("Error   %s %d : A 16C5x return value must be a CONSTANT only\n", INFILE, NR);
		}
		else if (find_low_reg(ptr) >= 0)
			error("Error   %s %d : Only BYTEs and CONSTANTs allowed\n", INFILE, NR);
		else
			error("Error   %s %d : Expression syntax error\n", INFILE, NR);
	}
}

_CPUspecifics()
{
	char *buf = new(L_FLDLEN);
	int len, i, bit, regaddr, index;
	char *ptr, *ptr2, *ptr3;
	int val1, val2;
	
	if (NF == 1 && strcmp(S[1], "endasm") == 0)
	{		// must be 1st
		asmflag = 0;
	}
	else if (asmflag == 1)
	{				// must be 2nd
		if (NF == 1 && S[1][strlen(S[1]) - 1] == ':')
		{
			S[1][strlen(S[1]) - 1] = '\0';
			_Label(S[1]);
			free(buf);
			return;
		}
		while ((ptr = strchr(S[0], ':')))
			*ptr = ',';			// for bit stuff
		while (replace(S[0], ",", " "));
		while (replace(S[0], "  ", " "));
		fracture();
		Assemble();
	}
	else if (NF == 1 && strcmp(S[1], "asmstart") == 0)
	{	// must be 3rd
		asmflag = 1;
	}
	else if (NF >= 2 && strcmp(S[1], "asm") == 0)
	{
		if (NF == 2 && S[2][strlen(S[2]) - 1] == ':')
		{
			S[2][strlen(S[2]) - 1] = '\0';
			_Label(S[2]);
			free(buf);
			return;
		}
		while ((ptr = strchr(S[0], ':')))
			*ptr = ',';			// for bit stuff
		while (replace(S[0], ",", " "));
		while (replace(S[0], "  ", " "));
		replace(S[0], "asm ", "");
		fracture();
		Assemble();
	}
	else if (NF == 2 && strcmp(S[1], "set") == 0)
	{
		if ((val1 = validate_bit(S[2])) < 0)
		{
			free(buf);
			return;
		}
		setpage(S[2]);
		Opcode("bsf", find_reg(S[2]), val1, "");
	}
	else if (NF == 2 && strcmp(S[1], "clear") == 0)
	{
		if ((val1 = validate_bit(S[2])) < 0)
		{
			free(buf);
			return;
		}
		setpage(S[2]);
		Opcode("bcf", find_reg(S[2]), val1, "");
	}
	else if (NF == 2 && strcmp(S[1], "toggle") == 0)
	{
		if ((val1 = validate_bit(S[2])) < 0)
		{
			free(buf);
			return;
		}
		bit = 1;
		bit <<= val1;
		Opcode("movlw", bit, 0, "");
		setpage(S[2]);
		Opcode("xorwf", find_reg(S[2]), 1, "");
	}
	else if (NF == 2 && strcmp(S[1], "zero") == 0)
	{
		if (find_reg(S[2]) >= 0)
		{
			setpage(S[2]);
			Opcode("clrf", find_reg(S[2]), 0, "");
		}
		else
		{
			error("Error   %s %d : ZERO syntax error - Unknown variable\n", INFILE, NR);
		}
	}
	else if (NF == 2 && strcmp(S[1], "swap_nibbles") == 0)
	{
		if (find_reg(S[2]) >= 0)
		{
			setpage(S[2]);
			Opcode("swapf", find_reg(S[2]), 1, "");
		}
		else
		{
			error("Error   %s %d : SWAP_NIBBLES syntax error - Unknown variable\n", INFILE, NR);
		}
	}
	else if (NF == 2 && strcmp(S[1], "goto") == 0)
	{
		setpagenum(0);		// make sure it's 0 for label
		if (busy_with_proc >= 0)
		{
			sprintf(buf, "%s%d", S[2], busy_with_proc);
			Opcode("user_goto", 0, 0, buf);
		}
		else
			Opcode("user_goto", 0, 0, S[2]);
	}
	else if (NF == 2 && strcmp(S[1], "longjump") == 0)
	{
		setpagenum(0);		// make sure it's 0 for label
		Opcode("user_goto", 0, 0, S[2]);
		warning("Warning %s %d : LONGJUMP used here - Double check implication\n", INFILE, NR);
	}
	else if (NF == 2 && strcmp(S[1], "page") == 0)
	{
		len = atoi(S[2]);
		if (len <= TopCodePage)
			Opcode("org", len * CodePageSize, 0, "");
		else
			error("Error   %s %d : Invalid page number for this CPU\n", INFILE, NR);
		do_toptest();
	}
	else if (NF >= 3 && strcmp(S[1], "proc") == 0 && strcmp(S[2], "inline") == 0)
	{
		new_proc(S[3]);
		ProcInline[find_proc(S[3])] = 1;
	}
	else if (NF >= 2 && strcmp(S[1], "proc") == 0)
	{
		new_proc(S[2]);
	}
	else if (NF >= 3 && strcmp(S[1], "lib") == 0 && strcmp(S[2], "inline") == 0)
	{
		new_lib(S[3]);
		ProcInline[find_proc(S[3])] = 1;
	}
	else if (NF >= 2 && strcmp(S[1], "lib") == 0)
	{
		new_lib(S[2]);
	}
	else if (NF >= 2 && strcmp(S[1], "call") == 0)
	{
		do_call_parm(S[2]);
		if ((index = find_proc(S[2])) >= 0)
			ProcReturnsValue[index] = 0;	// NO return value
	}
	else if (NF >= 1 && strncmp(S[1], "endproc", 7) == 0)
	{
		do_endproc();
		do_lookups();	// process any pending lookups
	}
	else if (NF >= 1 && strncmp(S[1], "return", 6) == 0)
	{
		do_return();
	}
	else if (NF >= 1 && is_proc(S[1]))
	{
		do_call(S[1]);
		if ((index = find_proc(S[1])) >= 0)
			ProcReturnsValue[index] = 0;	// NO return value
	}
	else if (strcmp(S[1], "set_I/O") == 0)
	{
		if (NF != nports + 1)
		{
			error("Error   %s %d : Incorrect number of arguments\n", INFILE, NR);
			free(buf);
			return;
		}
		if (has_tris)
		{
			setpageval(RegAddr[has_tris]);
			for (i = 0; i < nports; i++)
			{
				Opcode("movlw", get_value(S[i+2]), 0, "");
				sprintf(buf, "TRIS%c", 'A'+i);
				Opcode("movwf", find_reg(buf), 0, "");
			}
		}
		else
		{
			for (i = 0; i < nports; i++)
			{
				Opcode("movlw", get_value(S[i+2]), 0, "");
				Opcode("tris", port_start+i, 0, "");
			}
		}
	}
	else if (NF == 2 && strcmp(S[1], "set_option") == 0)
	{
		// first find out where OPTION sits
		for (i = 0; i < NextReg; i++)
		{
			if (strnicmp(Reg[i], "OPTION", 6) == 0)
				break;
		}

		// find value for OPTION
		if (find_reg(S[2]) >= 0)
		{
			setpage(S[2]);
			Opcode("movf", find_reg(S[2]), 0, "");
		}
		else
		{
			Opcode("movlw", get_value(S[2]), 0, "");
		}

		// perform the action
		if (i < NextReg)
		{		// it is a normal register
			if (RegAddr[i] >= DataPageSize)
			{
				setpageval(RegAddr[i]);
				Opcode("movwf", RegAddr[i]-DataPageSize, 0, "");
			}
			else
				Opcode("movwf", RegAddr[i], 0, "");
		}
		else
		{
			Opcode("option", 0, 0, "");
		}
	}
	else if (NF == 2 && strcmp(S[1], "neg") == 0)
	{
		setpage(S[2]);
		Opcode("comf", find_reg(S[2]), 1, "");
	}
	else if (NF == 2 && strcmp(S[1], "return") == 0)
	{
		do_return();
	}
	else if (NF == 2 && strcmp(S[1], "endproc") == 0)
	{
		do_endproc();
		do_lookups();	// process any pending lookups
	}
	else if (NF >= 1 && S[1][strlen(S[1])-1] == ':')
	{
		S[1][strlen(S[1])-1] = '\0';	// remove LAST ':'
		if (busy_with_proc >= 0 && S[1][strlen(S[1])-1] != ':')
		{
			sprintf(buf, "%s%d", S[1], busy_with_proc);
			strcpy(S[1], buf);
		}

		if (S[1][strlen(S[1])-1] == ':')
		{	// remove 2nd last ':'
			S[1][strlen(S[1])-1] = '\0';
		}

		if (strcmp(S[1], "main") == 0)
		{
			if (INTvector >= 0 && !int_declared)
				error("Error   %s %d : 'main' must be placed AFTER 'INT_handler...endint'\n", INFILE, NR);
			
			if (!MainFlag)
				MainFlag = 1;
			else
			{
				error("Error   %s %d : Label 'main' already defined\n", INFILE, NR);
				free(buf);
				return;
			}
			_Label(S[1]);
			Label[NextLabel-1].Used = 1;
		}
		else
			_Label(S[1]);

		// force an auto-page reset if NOT on page 0
		// because the GOTO would have reset page to 0
		if (datapagenum != 0)
			reset_datapage();
		// send the rest of the line for processing again
		if (NF > 1)
		{
			S[1][0] = '\0';
			rebuild();
			LINE();
		}
	}
	else if (NF == 1)
	{
		len = strlen(S[1]);
		if (strcmp(S[1], "INT_handler") == 0)
		{
			if (INTvector >= 0)
			{
				if (busy_with_proc >= 0 || busy_with_int >= 0)
				{
					error("Error   %s %d : Procedure inside INT handler not allowed\n", INFILE, NR);
					free(buf);
					return;
				}
				else if (int_declared == 1)
				{
					error("Error   %s %d : INT handler already declared\n", INFILE, NR);
					free(buf);
					return;
				}
				// see if name already exists (i.e. has been called already)
				for (i = 0; i < NextProc; i++)
				{
					if (strcmp(Proc[i], S[1]) == 0)
						break;
				}

				// see if procedure already defined
				if ((ProcStatus[i] & PROC_EXIST))
				{
					error("Error   %s %d : Interrupt handler '%s' already defined\n", INFILE, NR, S[1]);
					free(buf);
					return;
				}

				// not called yet - so add name to list
				if (i == NextProc)
				{
					if (NextProc < MAXPROCS)
					{
						Proc[NextProc] = new(strlen(S[1]) + 1);
						strcpy(Proc[NextProc], S[1]);
						NextProc++;
					}
					else
					{
						error("Error   %s %d : Procedure limit exceeded: No space for '%s'\n", INFILE, NR, S[1]);
						free(buf);
						return;
					}
				}
				ProcStatus[i] |= PROC_EXIST;	// procedure now exists
				ProcStart[i] = NR;		// line where proc starts
				ProcCodeStart[i] = OCaddr;	// addr where proc starts
				IntStart = NR;			// line where proc starts
				_Label("INT_handler");
				Label[NextLabel-1].Used = 1;	// force it ON
				busy_with_int = 1;
				int_declared = 1;
				busy_with_proc = i;
			}
			else
			{
				error("Error   %s %d : Not allowed for this processor\n", INFILE, NR);
				free(buf);
				return;
			}
		}
		else if (strcmp(S[1], "sleep") == 0)
		{
			Opcode("sleep", 0, 0, "");
		}
		else if (strcmp(S[1], "nop") == 0)
		{
			Opcode("nop", 0, 0, "");
		}
		else if (strcmp(S[1], "clear_wdt") == 0)
		{
			Opcode("clrwdt", 0, 0, "");
		}
		else if (strcmp(S[1], "retint") == 0 && INTvector >= 0)
		{
			if (busy_with_int < 0)
			{
				error("Error   %s %d : No INT handler defined\n", INFILE, NR);
				free(buf);
				return;
			}
			warning("Warning %s %d : Ensure CPU status is restored\n", INFILE, NR);
			Opcode("retfie", 0, 0, "");
		}
		else if (strcmp(S[1], "endint") == 0)
		{
			if (INTvector >= 0)
			{
				if (busy_with_int < 0)
				{
					error("Error   %s %d : INT handler not defined\n", INFILE, NR);
					free(buf);
					return;
				}
				ProcEnd[busy_with_proc] = NR;	// line where proc ends
				ProcCodeEnd[busy_with_proc] = OCaddr;	// addr where proc ends
				IntEnd = NR;	// line where int handler ends
				// PAGE IS NOT RESET HERE - INT HANDLER DOES THAT
				Opcode("retfie", 0, 0, "[INT_handler]");
				busy_with_int = -1;
				busy_with_proc = -1;
				do_lookups();	// process any pending lookups
			}
			else
			{
				error("Error   %s %d : Not allowed for this processor\n", INFILE, NR);
				free(buf);
				return;
			}
		}
		else if (find_reg(S[1]) >= 0 || find_low_reg(S[1]) >= 0)
		{
			if (S[1][len-2] == '-' && S[1][len-1] == '-')
			{
				S[1][len-2] = '\0';
				gen_dec(S[1]);
			}
			else if (S[1][len-2] == '+' && S[1][len-1] == '+')
			{
				S[1][len-2] = '\0';
				gen_inc(S[1]);
			}
			else if (S[1][len-2] == '<' && S[1][len-1] == '<')
			{
				S[1][len-2] = '\0';
				gen_left(S[1]);
			}
			else if (S[1][len-2] == '>' && S[1][len-1] == '>')
			{
				S[1][len-2] = '\0';
				gen_right(S[1]);
			}
			else
			{
				error("Error   %s %d : Compact command syntax error\n", INFILE, NR);
				free(buf);
				return;
			}
		}
		else
		{
			error("Error   %s %d : Single keyword syntax error - or undefined variable\n", INFILE, NR);
			free(buf);
			return;
		}
	}
	else if (NF == 3)
	{
		if (strcmp(S[1], S[3]) == 0)
		{
			error("Error   %s %d : Destination may not equal source\n", INFILE, NR);
			free(buf);
			return;
		}
		else if (strcmp(S[2], "=") == 0)
		{
			if (strchr(S[1], ':'))
			{
				if ((val1 = validate_bit(S[1])) < 0)
				{
					free(buf);
					return;
				}
				else if (strchr(S[3], ':'))
				{
					if ((val2 = validate_bit(S[3])) < 0)
					{
						free(buf);
						return;
					}
					if (findpage(S[1]) != findpage(S[3]))
					{
						PageError();
						free(buf);
						return;
					}
					setpage(S[1]);
					if (IsPORT(S[1]))
						Opcode("btfss", find_reg(S[3]), val2, "");
					Opcode("bcf", find_reg(S[1]), val1, "");
					Opcode("btfsc", find_reg(S[3]), val2, "");
					Opcode("bsf", find_reg(S[1]), val1, "");
				}
				else
				{
					setpage(S[1]);
					if (strcmp(S[3], "0") == 0)
						Opcode("bcf", find_reg(S[1]), val1, "");
					else if (strcmp(S[3], "1") == 0)
						Opcode("bsf", find_reg(S[1]), val1, "");
					else
					{
						BitError();
						free(buf);
						return;
					}
				}
			}
			else
				set_equal(S[0]);
		}
		else if (strcmp(S[2], "!=") == 0)
		{
			if (strchr(S[1], ':'))
			{
				if ((val1 = validate_bit(S[1])) < 0)
				{
					free(buf);
					return;
				}
				else if (strchr(S[3], ':'))
				{
					if ((val2 = validate_bit(S[3])) < 0)
					{
						free(buf);
						return;
					}
					if (findpage(S[1]) != findpage(S[3]))
					{
						PageError();
						free(buf);
						return;
					}
					setpage(S[1]);
					if (IsPORT(S[1]))
						Opcode("btfss", find_reg(S[3]), val2, "");
					Opcode("bsf", find_reg(S[1]), val1, "");
					Opcode("btfsc", find_reg(S[3]), val2, "");
					Opcode("bcf", find_reg(S[1]), val1, "");
				}
				else
				{
					setpage(S[1]);
					if (strcmp(S[3], "0") == 0)
						Opcode("bsf", find_reg(S[1]), val1, "");
					else if (strcmp(S[3], "1") == 0)
						Opcode("bcf", find_reg(S[1]), val1, "");
					else
					{
						BitError();
						free(buf);
						return;
					}
				}
			}
			else
				set_notequal(S[0]);
		}
		else if (strcmp(S[2], "+=") == 0)
		{
			plus(S[0]);
		}
		else if (strcmp(S[2], "-=") == 0)
		{
			minus(S[0]);
		}
		else if (strcmp(S[2], "&=") == 0)
		{
			if (strchr(S[1], ':'))
			{
				if ((val1 = validate_bit(S[1])) < 0)
				{
					free(buf);
					return;
				}
				else if (strchr(S[3], ':'))
				{
					if ((val2 = validate_bit(S[3])) < 0)
					{
						free(buf);
						return;
					}
					if (findpage(S[1]) != findpage(S[3]))
					{
						PageError();
						free(buf);
						return;
					}
					setpage(S[1]);
					Opcode("btfss", find_reg(S[3]), val2, "");
					Opcode("bcf", find_reg(S[1]), val1, "");
				}
				else
				{
					setpage(S[1]);
					if (strcmp(S[3], "0") == 0)
						Opcode("bcf", find_reg(S[1]), val1, "");
					else if (strcmp(S[3], "1") == 0)
					{
						// no change
					}
					else
					{
						BitError();
						free(buf);
						return;
					}
				}
			}
			else
				bitand(S[0]);
		}
		else if (strcmp(S[2], "!&=") == 0)
		{
			if (strchr(S[1], ':'))
			{
				if ((val1 = validate_bit(S[1])) < 0)
				{
					free(buf);
					return;
				}
				else if (strchr(S[3], ':'))
				{
					if ((val2 = validate_bit(S[3])) < 0)
					{
						free(buf);
						return;
					}
					if (IsPORT(S[1]))
					{
						PortError();
						free(buf);
						return;
					}
					if (findpage(S[1]) != findpage(S[3]))
					{
						PageError();
						free(buf);
						return;
					}
					setpage(S[1]);
					Opcode("btfss", find_reg(S[3]), val2, "");
					Opcode("bcf", find_reg(S[1]), val1, "");
					bit = 1;
					bit <<= val1;
					Opcode("movlw", bit, 0, "");
					Opcode("xorwf", find_reg(S[1]), 1, "");
				}
				else
				{
					setpage(S[1]);
					if (strcmp(S[3], "0") == 0)
						Opcode("bsf", find_reg(S[1]), val1, "");
					else if (strcmp(S[3], "1") == 0)
					{
						bit = 1;
						bit <<= val1;
						Opcode("movlw", bit, 0, "");
						Opcode("xorwf", find_reg(S[1]), 1, "");
					}
					else
					{
						BitError();
						free(buf);
						return;
					}
				}
			}
			else
				bitnand(S[0]);
		}
		else if (strcmp(S[2], "|=") == 0)
		{
			if (strchr(S[1], ':'))
			{
				if ((val1 = validate_bit(S[1])) < 0)
				{
					free(buf);
					return;
				}
				else if (strchr(S[3], ':'))
				{
					if ((val2 = validate_bit(S[3])) < 0)
					{
						free(buf);
						return;
					}
					if (findpage(S[1]) != findpage(S[3]))
					{
						PageError();
						free(buf);
						return;
					}
					setpage(S[1]);
					Opcode("btfsc", find_reg(S[3]), val2, "");
					Opcode("bsf", find_reg(S[1]), val1, "");
				}
				else
				{
					setpage(S[1]);
					if (strcmp(S[3], "1") == 0)
						Opcode("bsf", find_reg(S[1]), val1, "");
					else if (strcmp(S[3], "0") == 0)
					{
						// no change
					}
					else
					{
						BitError();
						free(buf);
						return;
					}
				}
			}
			else
				bitor(S[0]);
		}
		else if (strcmp(S[2], "!|=") == 0)
		{
			if (strchr(S[1], ':'))
			{
				if ((val1 = validate_bit(S[1])) < 0)
				{
					free(buf);
					return;
				}
				else if (strchr(S[3], ':'))
				{
					if ((val2 = validate_bit(S[3])) < 0)
					{
						free(buf);
						return;
					}
					if (IsPORT(S[1]))
					{
						PortError();
						free(buf);
						return;
					}
					if (findpage(S[1]) != findpage(S[3]))
					{
						PageError();
						free(buf);
						return;
					}
					setpage(S[1]);
					Opcode("btfsc", find_reg(S[3]), val2, "");
					Opcode("bsf", find_reg(S[1]), val1, "");
					bit = 1;
					bit <<= val1;
					Opcode("movlw", bit, 0, "");
					Opcode("xorwf", find_reg(S[1]), 1, "");
				}
				else
				{
					setpage(S[1]);
					if (strcmp(S[3], "1") == 0)
						Opcode("bcf", find_reg(S[1]), val1, "");
					else if (strcmp(S[3], "0") == 0)
					{
						bit = 1;
						bit <<= val1;
						Opcode("movlw", bit, 0, "");
						Opcode("xorwf", find_reg(S[1]), 1, "");
					}
					else
					{
						BitError();
						free(buf);
						return;
					}
				}
			}
			else
				bitnor(S[0]);
		}
		else if (strcmp(S[2], "^=") == 0)
		{
			if (strchr(S[1], ':'))
			{
				if ((val1 = validate_bit(S[1])) < 0)
				{
					free(buf);
					return;
				}
				else if (strchr(S[3], ':'))
				{
					if ((val2 = validate_bit(S[3])) < 0)
					{
						free(buf);
						return;
					}
					if (findpage(S[1]) != findpage(S[3]))
					{
						PageError();
						free(buf);
						return;
					}
					setpage(S[1]);
					Opcode("movlw", 2+findicount(S[1]), 0, "");
					Opcode("btfss", find_reg(S[3]), val2, "");
					_JMP(JmpLabel());	// instead
					// careful !! Opcode("addwf", 2, 1, "");
					bit = 1;
					bit <<= val1;
					Opcode("movlw", bit, 0, "");
					Opcode("xorwf", find_reg(S[1]), 1, "");
					_Label(JmpLabel());	// instead
					JmpLabelCount++;	// instead
				}
				else
				{
					setpage(S[1]);
					if (strcmp(S[3], "1") == 0)
					{
						bit = 1;
						bit <<= val1;
						Opcode("movlw", bit, 0, "");
						Opcode("xorwf", find_reg(S[1]), 1, "");
					}
					else if (strcmp(S[3], "0") == 0)
					{
						// no change
					}
					else
					{
						BitError();
						free(buf);
						return;
					}
				}
			}
			else
				bitxor(S[0]);
		}
		else if (strcmp(S[2], "!^=") == 0)
		{
			if (strchr(S[1], ':'))
			{
				if ((val1 = validate_bit(S[1])) < 0)
				{
					free(buf);
					return;
				}
				else if (strchr(S[3], ':'))
				{
					if ((val2 = validate_bit(S[3])) < 0)
					{
						free(buf);
						return;
					}
					if (findpage(S[1]) != findpage(S[3]))
					{
						PageError();
						free(buf);
						return;
					}
					setpage(S[1]);
					Opcode("movlw", 2+findicount(S[1]), 0, "");
					Opcode("btfsc", find_reg(S[3]), val2, "");
					_JMP(JmpLabel());	// instead
					// careful !! Opcode("addwf", 2, 1, "");
					bit = 1;
					bit <<= val1;
					Opcode("movlw", bit, 0, "");
					Opcode("xorwf", find_reg(S[1]), 1, "");
					_Label(JmpLabel());	// instead
					JmpLabelCount++;	// instead
				}
				else
				{
					setpage(S[1]);
					if (strcmp(S[3], "0") == 0)
					{
						bit = 1;
						bit <<= val1;
						Opcode("movlw", bit, 0, "");
						Opcode("xorwf", find_reg(S[1]), 1, "");
					}
					else if (strcmp(S[3], "1") == 0)
					{
						// no change
					}
					else
					{
						BitError();
						free(buf);
						return;
					}
				}
			}
			else
				bitxnor(S[0]);
		}
		else
		{
			error("Error   %s %d : Equate syntax error\n", INFILE, NR);
			free(buf);
			return;
		}
	}
	else if (S[0][0])
	{
		error("Error   %s %d : General syntax error\n", INFILE, NR);
		free(buf);
		return;
	}
	free(buf);
}

// tests validity of bit-token - returns bit value 0 - 7 else -1
// also terminates first part of string with 0
validate_bit(char *string)
{
	char *ptr, *ptr2, *ptr3;

	ptr = strchr(string, ':');
	ptr2 = ptr + 1;
	ptr3 = ptr2 + 1;
	if (ptr && *ptr2 >= '0' && *ptr2 <= '7' && *ptr3 == '\0')
	{
		*ptr = '\0';
		if (find_reg(string) < 0)
		{
			error("Error   %s %d : Unknown variable '%s'\n", INFILE, NR, string);
			return(-1);
		}
		return(*ptr2 - '0');
	}
	error("Error   %s %d : Syntax error - Must be 'reg:n', n=0-7\n", INFILE, NR);
	return(-1);
}

static normal_test(int test, char *label)
{
	char *buf = new(L_FLDLEN);
	
	if (test == T_EQUAL)
	{			// =
		Opcode("btfsc", 3, 2, "");
	}
	else if (test == T_NOT_EQUAL)
	{		// !=
		Opcode("btfss", 3, 2, "");
	}
	else if (test == T_GREATER)
	{		// >
		Opcode("btfsc", 3, 2, "");
		sprintf(buf, "LL_JG_%d", JGlabel);
		Opcode("goto", 0, 0, buf);
		Opcode("btfsc", 3, 0, "");
		Opcode("goto", 0, 0, label);
		_Label(buf);
		JGlabel++;
		free(buf);
		return;
	}
	else if (test == T_NOT_GREATER)
	{	// <=
		Opcode("btfsc", 3, 2, "");
		Opcode("goto", 0, 0, label);
		Opcode("btfss", 3, 0, "");
	}
	else if (test == T_GREATER_EQ)
	{	// >=
		Opcode("btfsc", 3, 0, "");
	}
	else
	{					// <
		Opcode("btfss", 3, 0, "");
	}
	Opcode("goto", 0, 0, label);
	free(buf);
}

static do_test(int test, char *label)
{
	char *start_lbl = new(L_FLDLEN);
	char *end_lbl = new(L_FLDLEN);
	char *int_lbl = new(L_FLDLEN);
	unsigned int val;
	char Low, Hi;

	// <WORD> <test> <WORD>
	if (find_low_reg(p_src) >= 0 && find_low_reg(p_dst) >= 0)
	{
// Low AND Hi must equate EQUAL to jump to 'Label'
		if (test == T_EQUAL)
		{			// =
			setpage(p_dst);
			Opcode("movf", find_hi_reg(p_dst), 0, "");
	              	setpage(p_src);
			Opcode("subwf", find_hi_reg(p_src), 0, "");
			Opcode("btfss", 3, 2, "");
			sprintf(start_lbl, "LL_WW_%d", Wlabel);
			Opcode("goto", 0, 0, start_lbl);
			setpage(p_dst);
			Opcode("movf", find_low_reg(p_dst), 0, "");
	       		setpage(p_src);
			Opcode("subwf", find_low_reg(p_src), 0, "");
			Opcode("btfsc", 3, 2, "");
			Opcode("goto", 0, 0, label);
			_Label(start_lbl);
			Wlabel++;
		}

// Low OR Hi may DIFFER to jump to 'Label'
		else if (test == T_NOT_EQUAL)
		{		// !=
			setpage(p_dst);
			Opcode("movf", find_hi_reg(p_dst), 0, "");
	              	setpage(p_src);
			Opcode("subwf", find_hi_reg(p_src), 0, "");
			Opcode("btfss", 3, 2, "");
			Opcode("goto", 0, 0, label);
			setpage(p_dst);
			Opcode("movf", find_low_reg(p_dst), 0, "");
	       		setpage(p_src);
			Opcode("subwf", find_low_reg(p_src), 0, "");
			Opcode("btfss", 3, 2, "");
			Opcode("goto", 0, 0, label);
			Wlabel++;
		}

// If Hi BIGGER OR if Hi is EQUAL AND Low is BIGGER, jump to 'label'
		else if (test == T_GREATER)
		{		// >
			setpage(p_dst);
			Opcode("movf", find_hi_reg(p_dst), 0, "");
	      		setpage(p_src);
			Opcode("subwf", find_hi_reg(p_src), 0, "");
			Opcode("btfsc", 3, 2, "");
			sprintf(int_lbl, "LL_WW_I%d", Wlabel);
			Opcode("goto", 0, 0, int_lbl);	// DO LOW: Hi is EQUAL
			Opcode("btfsc", 3, 0, "");
			Opcode("goto", 0, 0, label);    // GO!!: Hi is BIGGER
			sprintf(end_lbl, "LL_WW_E%d", Wlabel);
			Opcode("goto", 0, 0, end_lbl);	// STAY: Hi is LESS
			
			// Hi is EQUAL here - only test for Low
			_Label(int_lbl);
			setpage(p_dst);
			Opcode("movf", find_low_reg(p_dst), 0, "");
	      		setpage(p_src);
			Opcode("subwf", find_low_reg(p_src), 0, "");
			Opcode("btfsc", 3, 2, "");
			Opcode("goto", 0, 0, end_lbl);	// STAY: Low is EQUAL
			Opcode("btfsc", 3, 0, "");
			Opcode("goto", 0, 0, label);	// GO!!: Low is BIGGER
			_Label(end_lbl);
			Wlabel++;
		}

// If Hi LESS OR if Hi EQUAL AND Low is LESS OR EQUAL, jump to 'Label'
		else if (test == T_NOT_GREATER)
		{	// <= 
			setpage(p_src);
			Opcode("movf", find_hi_reg(p_src), 0, "");
	      		setpage(p_dst);
			Opcode("subwf", find_hi_reg(p_dst), 0, "");
			Opcode("btfsc", 3, 2, "");
			sprintf(int_lbl, "LL_WW_I%d", Wlabel);
			Opcode("goto", 0, 0, int_lbl);	// DO LOW: Hi is EQUAL
			Opcode("btfsc", 3, 0, "");
			Opcode("goto", 0, 0, label);    // GO!!: Hi is LESS
			sprintf(end_lbl, "LL_WW_E%d", Wlabel);
			Opcode("goto", 0, 0, end_lbl);	// STAY: Hi is LESS

			// Hi is EQUAL here - only test for Low
			_Label(int_lbl);
			setpage(p_src);
			Opcode("movf", find_low_reg(p_src), 0, "");
	      		setpage(p_dst);
			Opcode("subwf", find_low_reg(p_dst), 0, "");
			Opcode("btfsc", 3, 2, "");
			Opcode("goto", 0, 0, label);	// GO!!: Low is EQUAL
			Opcode("btfsc", 3, 0, "");
			Opcode("goto", 0, 0, label);	// GO!!: Low is LESS
			_Label(end_lbl);
			Wlabel++;
		}

// If Hi BIGGER OR if Hi is EQUAL AND Low is BIGGER OR EQUAL, jump to 'label'
		else if (test == T_GREATER_EQ)
		{	// >=
			setpage(p_dst);
			Opcode("movf", find_hi_reg(p_dst), 0, "");
	      		setpage(p_src);
			Opcode("subwf", find_hi_reg(p_src), 0, "");
			Opcode("btfsc", 3, 2, "");
			sprintf(int_lbl, "LL_WW_I%d", Wlabel);
			Opcode("goto", 0, 0, int_lbl);	// DO LOW: Hi is EQUAL
			Opcode("btfsc", 3, 0, "");
			Opcode("goto", 0, 0, label);    // GO!!: Hi is BIGGER
			sprintf(end_lbl, "LL_WW_E%d", Wlabel);
			Opcode("goto", 0, 0, end_lbl);	// STAY: Hi is LESS

			// Hi is EQUAL here - only test for Low
			_Label(int_lbl);
			setpage(p_dst);
			Opcode("movf", find_low_reg(p_dst), 0, "");
	      		setpage(p_src);
			Opcode("subwf", find_low_reg(p_src), 0, "");
			Opcode("btfsc", 3, 2, "");
			Opcode("goto", 0, 0, label);	// GO: Low is EQUAL
			Opcode("btfsc", 3, 0, "");
			Opcode("goto", 0, 0, label);	// GO: Low is BIGGER
			_Label(end_lbl);
			Wlabel++;
		}

// If Hi LESS OR if Hi EQUAL AND Low is LESS, jump to 'Label'
		else
		{  // NOT_GREATER_EQ		// <
			setpage(p_src);
			Opcode("movf", find_hi_reg(p_src), 0, "");
	      		setpage(p_dst);
			Opcode("subwf", find_hi_reg(p_dst), 0, "");
			Opcode("btfsc", 3, 2, "");
			sprintf(int_lbl, "LL_WW_I%d", Wlabel);
			Opcode("goto", 0, 0, int_lbl);	// DO LOW: Hi is EQUAL
			Opcode("btfsc", 3, 0, "");
			Opcode("goto", 0, 0, label);    // GO!!: Hi is LESS
			sprintf(end_lbl, "LL_WW_E%d", Wlabel);
			Opcode("goto", 0, 0, end_lbl);	// STAY: Hi is LESS

			// Hi is EQUAL here - only test for Low
			_Label(int_lbl);
			setpage(p_src);
			Opcode("movf", find_low_reg(p_src), 0, "");
	      		setpage(p_dst);
			Opcode("subwf", find_low_reg(p_dst), 0, "");
			Opcode("btfsc", 3, 2, "");
			Opcode("goto", 0, 0, end_lbl);	// STAY: Low is EQUAL
			Opcode("btfsc", 3, 0, "");
			Opcode("goto", 0, 0, label);	// GO!!: Low is LESS
			_Label(end_lbl);
			Wlabel++;
		}
	}

	// (WORD> <test> <CONSTANT>
	else if (find_low_reg(p_src) >= 0 && isdigit(p_dst[0]))
	{
		// make sure procedures compare to bytes only
		if (is_proc(p_dst))
		{
			error("Error   %s %d : Procedures must be compared with BYTES\n", INFILE, NR);
			free(start_lbl);
			free(end_lbl);
			free(int_lbl);
			return;
		}

		val = get_value(p_dst);
		Low = val & 0xff;
		Hi = val >> 8;

// Low AND Hi must equate EQUAL to jump to 'Label'
		if (test == T_EQUAL)
		{
	              	setpage(p_src);
			if (Hi == 0)
				Opcode("movf", find_hi_reg(p_src), 0, "");
			else
			{
				Opcode("movlw", Hi, 0, "");
				Opcode("subwf", find_hi_reg(p_src), 0, "");
			}
			Opcode("btfss", 3, 2, "");
			sprintf(start_lbl, "LL_WW_%d", Wlabel);
			Opcode("goto", 0, 0, start_lbl);
	              	setpage(p_src);
			if (Low == 0)
			{
				Opcode("movf", find_low_reg(p_src), 0, "");
			}
			else
			{
				Opcode("movlw", Low, 0, "");
				Opcode("subwf", find_low_reg(p_src), 0, "");
			}
			Opcode("btfsc", 3, 2, "");
			Opcode("goto", 0, 0, label);
			_Label(start_lbl);
			Wlabel++;
		}

// Low OR Hi may DIFFER to jump to 'Label'
		else if (test == T_NOT_EQUAL)
		{
	              	setpage(p_src);
			if (Hi == 0)
				Opcode("movf", find_hi_reg(p_src), 0, "");
			else
			{
				Opcode("movlw", Hi, 0, "");
				Opcode("subwf", find_hi_reg(p_src), 0, "");
			}
			Opcode("btfss", 3, 2, "");
			Opcode("goto", 0, 0, label);
	              	setpage(p_src);
			if (Low == 0)
				Opcode("movf", find_low_reg(p_src), 0, "");
			else
			{
				Opcode("movlw", Low, 0, "");
				Opcode("subwf", find_low_reg(p_src), 0, "");
			}
			Opcode("btfss", 3, 2, "");
			Opcode("goto", 0, 0, label);
			Wlabel++;
		}

// If Hi BIGGER OR if Hi is EQUAL AND Low is BIGGER, jump to 'label'
		else if (test == T_GREATER)
		{		// >
			Opcode("movlw", Hi, 0, "");
	      		setpage(p_src);
			Opcode("subwf", find_hi_reg(p_src), 0, "");
			Opcode("btfsc", 3, 2, "");
			sprintf(int_lbl, "LL_WW_I%d", Wlabel);
			Opcode("goto", 0, 0, int_lbl);	// DO LOW: Hi is EQUAL
			Opcode("btfsc", 3, 0, "");
			Opcode("goto", 0, 0, label);    // GO!!: Hi is BIGGER
			sprintf(end_lbl, "LL_WW_E%d", Wlabel);
			Opcode("goto", 0, 0, end_lbl);	// STAY: Hi is LESS

			// Hi is EQUAL here - only test for Low
			_Label(int_lbl);
			Opcode("movlw", Low, 0, "");                
	      		setpage(p_src);
			Opcode("subwf", find_low_reg(p_src), 0, "");
			Opcode("btfsc", 3, 2, "");
			Opcode("goto", 0, 0, end_lbl);	// STAY: Low is EQUAL
			Opcode("btfsc", 3, 0, "");
			Opcode("goto", 0, 0, label);	// GO!!: Low is BIGGER
			_Label(end_lbl);
			Wlabel++;
		}

// If Hi LESS OR if Hi EQUAL AND Low is LESS OR EQUAL, jump to 'Label'
		else if (test == T_NOT_GREATER)
		{	// <=
			setpage(p_src);
			Opcode("movlw", Hi, 0, "");
			Opcode("subwf", find_hi_reg(p_src), 0, "");
//			Opcode("movf", find_hi_reg(p_src), 0, "");
//			Opcode("sublw", Hi, 0, "");
			Opcode("btfsc", 3, 2, "");
			sprintf(int_lbl, "LL_WW_I%d", Wlabel);
			Opcode("goto", 0, 0, int_lbl);	// DO LOW: Hi is EQUAL
//			Opcode("btfsc", 3, 0, "");
			Opcode("btfss", 3, 0, "");
			Opcode("goto", 0, 0, label);    // GO!!: Hi is LESS
			sprintf(end_lbl, "LL_WW_E%d", Wlabel);
			Opcode("goto", 0, 0, end_lbl);	// STAY: Hi is LESS

			// Hi is EQUAL here - only test for Low
			_Label(int_lbl);
			setpage(p_src);
			Opcode("movlw", Low, 0, "");
			Opcode("subwf", find_low_reg(p_src), 0, "");
//			Opcode("movf", find_low_reg(p_src), 0, "");
//			Opcode("sublw", Low, 0, "");
			Opcode("btfsc", 3, 2, "");
			Opcode("goto", 0, 0, label);	// GO!!: Low is EQUAL
			Opcode("btfss", 3, 0, "");
//			Opcode("btfsc", 3, 0, "");
			Opcode("goto", 0, 0, label);	// GO!!: Low is LESS
			_Label(end_lbl);
			Wlabel++;
		}

// If Hi BIGGER OR if Hi is EQUAL AND Low is BIGGER OR EQUAL, jump to 'label'
		else if (test == T_GREATER_EQ)
		{	// >=
			Opcode("movlw", Hi, 0, "");
	      		setpage(p_src);
			Opcode("subwf", find_hi_reg(p_src), 0, "");
			Opcode("btfsc", 3, 2, "");
			sprintf(int_lbl, "LL_WW_I%d", Wlabel);
			Opcode("goto", 0, 0, int_lbl);	// DO LOW: Hi is EQUAL
			Opcode("btfsc", 3, 0, "");
			Opcode("goto", 0, 0, label);    // GO!!: Hi is BIGGER
			sprintf(end_lbl, "LL_WW_E%d", Wlabel);
			Opcode("goto", 0, 0, end_lbl);	// STAY: Hi is LESS
			
			// Hi is EQUAL here - only test for Low
			_Label(int_lbl);
			Opcode("movlw", Low, 0, "");                
	      		setpage(p_src);
			Opcode("subwf", find_low_reg(p_src), 0, "");
			Opcode("btfsc", 3, 2, "");
			Opcode("goto", 0, 0, label);	// GO: Low is EQUAL
			Opcode("btfsc", 3, 0, "");
			Opcode("goto", 0, 0, label);	// GO: Low is BIGGER
			_Label(end_lbl);
			Wlabel++;
		}

// If Hi LESS OR if Hi EQUAL AND Low is LESS, jump to 'Label'
		else
		{  // NOT_GREATER_EQ		// <
			setpage(p_src);
			Opcode("movlw", Hi, 0, "");
			Opcode("subwf", find_hi_reg(p_src), 0, "");
//			Opcode("movf", find_hi_reg(p_src), 0, "");
//			Opcode("sublw", Hi, 0, "");
			Opcode("btfsc", 3, 2, "");
			sprintf(int_lbl, "LL_WW_I%d", Wlabel);
			Opcode("goto", 0, 0, int_lbl);	// DO LOW: Hi is EQUAL
//			Opcode("btfsc", 3, 0, "");
			Opcode("btfss", 3, 0, "");
			Opcode("goto", 0, 0, label);    // GO!!: Hi is LESS
			sprintf(end_lbl, "LL_WW_E%d", Wlabel);
			Opcode("goto", 0, 0, end_lbl);	// STAY: Hi is LESS

			// HiHi is EQUAL here - only test for Low
			_Label(int_lbl);
			setpage(p_src);
			Opcode("movlw", Low, 0, "");
			Opcode("subwf", find_low_reg(p_src), 0, "");
//			Opcode("movf", find_low_reg(p_src), 0, "");
//			Opcode("sublw", Low, 0, "");
			Opcode("btfsc", 3, 2, "");
			Opcode("goto", 0, 0, end_lbl);	// STAY: Low is EQUAL
//			Opcode("btfsc", 3, 0, "");
			Opcode("btfss", 3, 0, "");
			Opcode("goto", 0, 0, label);	// GO!!: Low is LESS
			_Label(end_lbl);
			Wlabel++;
		}
	}

	// <BYTE> <test> <BYTE>
	else if (find_reg(p_src) >= 0 && find_reg(p_dst) >= 0)
	{
		setpage(p_dst);
		Opcode("movf", find_reg(p_dst), 0, "");
		setpage(p_src);
		Opcode("subwf", find_reg(p_src), 0, "");

		normal_test(test, label);
	}

	// <BYTE> <test> <CONSTANT>
	else if (find_reg(p_src) >= 0 && isdigit(p_dst[0]))
	{
		val = get_value(p_dst);
		if (val > 255)
		{
			error("Error   %s %d : Constant may not exceed 255\n", INFILE, NR);
			free(start_lbl);
			free(end_lbl);
			free(int_lbl);
			return;
		}
		if (val == 0 && test == T_EQUAL)
		{
			setpage(p_src);
			Opcode("movf", find_reg(p_src), 0, "");
			Opcode("btfsc", 3, 2, "");
			Opcode("goto", 0, 0, label);
		}
		else if (val == 0 && test == T_NOT_EQUAL)
		{
			setpage(p_src);
			Opcode("movf", find_reg(p_src), 0, "");
			Opcode("btfss", 3, 2, "");
			Opcode("goto", 0, 0, label);
		}
		else
		{
			Opcode("movlw", val, 0, "");
			setpage(p_src);
			Opcode("subwf", find_reg(p_src), 0, "");
			normal_test(test, label);
		}
	}

	// <BYTE> <test> <PROCEDURE>
	else if (find_reg(p_src) >= 0 && is_proc(p_dst))
	{
		do_call(p_dst);
		setpage(p_src);
		Opcode("subwf", find_reg(p_src), 0, "");

		normal_test(test, label);
	}
	// <CONSTANT> <test> <PROCEDURE>
	else if (isdigit(p_src[0]) && is_proc(p_dst))
	{
		val = get_value(p_src);
		if (val > 255)
		{
			error("Error   %s %d : Constant may not exceed 255\n", INFILE, NR);
			free(start_lbl);
			free(end_lbl);
			free(int_lbl);
			return;
		}

		do_call(p_dst);

		if (CodeWidth == 12)
		{
			error("Error   %s %d : Invalid test for 16C5x - Must compare with variable\n", INFILE, NR);
			free(start_lbl);
			free(end_lbl);
			free(int_lbl);
			return;
		}
		Opcode("sublw", val, 0, "");

		normal_test(test, label);
	}

	// <fault>
	else
	{
		error("Error   %s %d : Expression syntax error\n", INFILE, NR);
		free(start_lbl);
		free(end_lbl);
		free(int_lbl);
		return;
	}

	free(start_lbl);
	free(end_lbl);
	free(int_lbl);
}

_Prologue()
{
	if (ResetVector == 0)
	{
		Opcode("org", 0, 0, "");
        Opcode("nop", 0, 0, "");        // new 2/8/99
		if (has_pclath)
			Opcode("clrf", 10, 0, "");
		Opcode("goto", 0, 0, "main");
	}
	if (INTvector > 0)
	{
		Opcode("org", INTvector, 0, "");
	}
}

_Epilogue()
{
	int i, val;

	if (!MainFlag)
	{
		error("Error   %s %d : No entry-point 'main:' defined\n", INFILE, NR);
	}
	
	if (INTvector > 0 && !int_declared)
	{
		error("Error   %s %d : No user-defined INT handler\n", INFILE, NR);
	}
	
	if (ResetVector > 0)
	{
		IgnoreOFtest = 1;
		Opcode("org", ResetVector, 0, "");
		Opcode("goto", 0, 0, "main");
		IgnoreOFtest = 0;
	}
}

_Label(char *LabelName)
{
	int i;

	if (NextLabel >= MAXLABELS)
	{
		error("Error   %s %d : Amount of labels exceeds %d\n", INFILE, NR, MAXLABELS);
		exit(1);
	}
	 
	for (i = 0; i < NextLabel; i++)
	{
		if (strcmp(Label[i].Name, LabelName) == 0)
		{
			error("Error   %s %d : Label '%s' already defined\n", INFILE, NR, LabelName);
			return;
		}
	}

	for (i = 0; i < NextDef; i++)
	{
		if (strcmp(Define[i], LabelName) == 0)
		{
			error("Error   %s %d : Label '%s' already defined with #define\n", INFILE, NR, DefName[i]);
			return;
		}
	}

	for (i = 0; i < NextMac; i++)
	{
		if (strcmp(Macro[i], LabelName) == 0)
		{
			error("Error   %s %d : Label '%s' already defined with #macro\n", INFILE, NR, MacName[i]);
			return;
		}
	}

	for (i = 0; i < strlen(LabelName); i++)
	{
		if (!islegal(LabelName[i]))
		{
			error("Error   %s %d : Illegal characters used in label '%s'\n", INFILE, NR, LabelName);
			return;
		}
	}
	
	Label[NextLabel].Name = new(strlen(LabelName) + 1);
	strcpy(Label[NextLabel].Name, LabelName);
	Label[NextLabel].Address = OCaddr;
	Label[NextLabel].Used = 0;
	NextLabel++;
}

_JMP(char *label)
{
	Opcode("goto", 0, 0, label);
}

/*
**
** *** Read the following as: Goto 'label' under 'Positive' section, 
**     and Stay under 'else' section
**
*/


_JE(int Positive, char *label)
{
	if (Positive)
	{
		do_test(T_EQUAL, label);
	}
	else
	{
		do_test(T_NOT_EQUAL, label);
	}
}

_JG(int Positive, char *label)
{
	if (Positive)
	{
		do_test(T_GREATER, label);
	}
	else
	{
		do_test(T_NOT_GREATER, label);
	}
}

_JGE(int Positive, char *label)
{
	if (Positive)
	{
		do_test(T_GREATER_EQ, label);
	}
	else
	{
		do_test(T_NOT_GREATER_EQ, label);
	}
}

_JBITSET(int Positive, char *label)
{
	int addr = find_reg(p_src);

	setpage(p_src);
	if (Positive)
	{
		Opcode("btfsc", addr, atoi(p_dst), "");
		Opcode("goto", 0, 0, label);
	}
	else
	{
		Opcode("btfss", addr, atoi(p_dst), "");
		Opcode("goto", 0, 0, label);
	}
}

_JBITCOMP(int Positive, char *label)
{
	char *buf = new(L_FLDLEN);
	char *buf2 = new(L_FLDLEN);

	sprintf(buf, "LL_BC_%d", BClabel);
	sprintf(buf2, "LL_ST_%d", BClabel);

	setpage(p_src);
	if (Positive)
	{
		Opcode("btfss", find_reg(p_src), p_bit1 - '0', "");
		Opcode("goto", 0, 0, buf);
		Opcode("btfss", find_reg(p_dst), p_bit2 - '0', "");
		Opcode("goto", 0, 0, buf2);
		_Label(buf);
		Opcode("btfsc", find_reg(p_src), p_bit1 - '0', "");
		Opcode("goto", 0, 0, label);
		Opcode("btfss", find_reg(p_dst), p_bit2 - '0', "");
		Opcode("goto", 0, 0, label);
		_Label(buf2);
	}
	else
	{
		Opcode("btfss", find_reg(p_src), p_bit1 - '0', "");
		Opcode("goto", 0, 0, buf);
		Opcode("btfsc", find_reg(p_dst), p_bit2 - '0', "");
		Opcode("goto", 0, 0, buf2);
		_Label(buf);
		Opcode("btfsc", find_reg(p_src), p_bit1 - '0', "");
		Opcode("goto", 0, 0, label);
		Opcode("btfsc", find_reg(p_dst), p_bit2 - '0', "");
		Opcode("goto", 0, 0, label);
		_Label(buf2);
	}

	BClabel++;
	free(buf2);
	free(buf);
}
/******************************************************************************
                             End of CPUSPEC.C
******************************************************************************/