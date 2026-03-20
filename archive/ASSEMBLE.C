/******************************************************************************
      Filename    : ASSEMBLE.C
      Product     : NITPIC
      Description : Does the assembling process
      Created     : August 4, 1999 [11:36:57]
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

/*----------------------------------------------------------------------
 * Assemble assembly string to opcodes
 *----------------------------------------------------------------------*/
Assemble()
{
	int reg, TOP, i;

	if (strcmp(S[1], "bcf") == 0 || 
	    strcmp(S[1], "bsf") == 0 ||
	    strcmp(S[1], "btfsc") == 0 ||
	    strcmp(S[1], "btfss") == 0)
	    {

		if (NF != 3)
		{
			error("Error   %s %d : Incorrect number of operands\n", INFILE, NR);
			return;
		}

		reg = find_reg(S[2]);
		if (reg < 0)
		{
			reg = get_value(S[2]);
			if (reg == 0)
			{
				warning("Warning %s %d : File register 0 specified - please verify\n", INFILE, NR);
			}
		}

		if (reg >= DataPageSize)
		{
			warning("Warning %s %d : Register in upper page - Ensure page is selected\n", INFILE, NR);
		}

		if (!isdigit(S[3][0]))
		{
			error("Error   %s %d : Bit number 0 - 7 expected\n", INFILE, NR);
			return;
		}

		if (get_value(S[3]) > 7)
		{
			error("Error   %s %d : Bit number out of range - Must be 0 - 7\n", INFILE, NR);
			return;
		}
		Opcode(S[1], reg, get_value(S[3]), "");
	}
	else if (strcmp(S[1], "addwf") == 0 || 
	         strcmp(S[1], "andwf") == 0 ||
	         strcmp(S[1], "comf") == 0 ||
	         strcmp(S[1], "decf") == 0 ||
	         strcmp(S[1], "decfsz") == 0 ||
	         strcmp(S[1], "incf") == 0 ||
	         strcmp(S[1], "incfsz") == 0 ||
	         strcmp(S[1], "iorwf") == 0 ||
	         strcmp(S[1], "movf") == 0 ||
	         strcmp(S[1], "rlf") == 0 ||
	         strcmp(S[1], "rrf") == 0 ||
	         strcmp(S[1], "subwf") == 0 ||
	         strcmp(S[1], "swapf") == 0 ||
	         strcmp(S[1], "xorwf") == 0)
	         {

		if (NF != 3)
		{
			error("Error   %s %d : Incorrect number of operands\n", INFILE, NR);
			return;
		}

		reg = find_reg(S[2]);
		if (reg < 0)
		{
			reg = get_value(S[2]);
			if (reg == 0)
			{
				warning("Warning %s %d : File register 0 specified - please verify\n", INFILE, NR);
			}
		}

		if (reg >= DataPageSize)
		{
			warning("Warning %s %d : Register in upper page - Ensure page is selected\n", INFILE, NR);
		}

		if (strcmp(S[3], "0") != 0 && strcmp(S[3], "1") != 0)
		{
			error("Error   %s %d : Second operand must be 0 or 1\n", INFILE, NR);
			return;
		}

		Opcode(S[1], reg, S[3][0] - '0', "");
	}
	else if (strcmp(S[1], "andlw") == 0 || 
	         strcmp(S[1], "iorlw") == 0 ||
	         strcmp(S[1], "movlw") == 0 ||
	         strcmp(S[1], "retlw") == 0 ||
	         strcmp(S[1], "addlw") == 0 ||
	         strcmp(S[1], "sublw") == 0 ||
	         strcmp(S[1], "xorlw") == 0)
	         {

		if (NF != 2)
		{
			error("Error   %s %d : Incorrect number of operands\n", INFILE, NR);
			return;
		}

		if (!isdigit(S[2][0]))
		{
			error("Error   %s %d : Operand not a constant\n", INFILE, NR);
			return;
		}

		if (get_value(S[2]) > 255)
		{
			error("Error   %s %d : Constant out of range - Must be 0 - 255\n", INFILE, NR);
			return;
		}

		Opcode(S[1], get_value(S[2]), 0, "");
	}
	else if (strcmp(S[1], "clrf") == 0 || 
	         strcmp(S[1], "tris") == 0 ||
	         strcmp(S[1], "movwf") == 0)
	         {

		if (NF != 2)
		{
			error("Error   %s %d : Incorrect number of operands\n", INFILE, NR);
			return;
		}

		reg = find_reg(S[2]);
		if (reg < 0)
		{
			reg = get_value(S[2]);
			if (reg == 0)
			{
				warning("Warning %s %d : File register 0 specified - please verify\n", INFILE, NR);
			}
		}

		if (reg >= DataPageSize)
		{
			warning("Warning %s %d : Register in upper page - Ensure page is selected\n", INFILE, NR);
			reg %= DataPageSize;
		}

		Opcode(S[1], reg, 0, "");
	}
	else if (CodeWidth == 12 && strcmp(S[1], "tris") == 0)
	{
		if (NF != 2)
		{
			error("Error   %s %d : Incorrect number of operands\n", INFILE, NR);
			return;
		}

		reg = find_reg(S[2]);
		if (reg < 0)
		{
			reg = get_value(S[2]);
		}

		if (reg < 5 || reg > nports)
		{
			error("Error   %s %d : Invalid port register - Must be PORTA - PORT%c\n", INFILE, NR, 'A'+nports);
			return;
		}

		Opcode(S[1], reg, 0, "");
	}
	else if (strcmp(S[1], "clrw") == 0 || 
	         strcmp(S[1], "clrwdt") == 0 ||
	         strcmp(S[1], "option") == 0 ||
	         strcmp(S[1], "sleep") == 0 ||
	         strcmp(S[1], "retfie") == 0 ||
	         strcmp(S[1], "return") == 0 ||
	         strcmp(S[1], "nop") == 0)
	         {

		if (NF != 1)
		{
			error("Error   %s %d : Incorrect number of operands\n", INFILE, NR);
			return;
		}

		Opcode(S[1], 0, 0, "");
	}
	else if (strcmp(S[1], "goto") == 0)
	{

		if (NF != 2)
		{
			error("Error   %s %d : Incorrect number of operands\n", INFILE, NR);
			return;
		}

		Opcode(S[1], 0, 0, S[2]);
	}
	else if (strcmp(S[1], "call") == 0)
	{

		if (NF != 2)
		{
			error("Error   %s %d : Incorrect number of operands\n", INFILE, NR);
			return;
		}

		for (i = 0; i < NextProc; i++)
		{
			if (strcmp(Proc[i], S[2]) == 0)
				break;
		}

		// not found yet - so add name so long
		if (i == NextProc)
		{
			if ((Licensed > 0 && NextProc < MAXPROCS) || 
			    (!Licensed && NextProc < DEMOMAXPROCS))
			    {
				Proc[NextProc] = new(strlen(S[2]) + 1);
				strcpy(Proc[NextProc], S[2]);
				NextProc++;
			}
			else
			{
				error("Error   %s %d : No space to add procedure name '%s'\n", INFILE, NR, S[2]);
				return;
			}
		}
		ProcStatus[i] |= ASM_CALLED;	// proc now called
		Opcode(S[1], 0, 0, S[2]);
	}
	else if (strcmp(S[1], "org") == 0)
	{
		if (NF != 2)
		{
			error("Error   %s %d : Incorrect number of operands\n", INFILE, NR);
			return;
		}

		TOP = CodePageSize * (TopCodePage + 1);
		if (get_value(S[2]) > TOP)
		{
			error("Error   %s %d : Address out of range - Must be 0 - %d\n", INFILE, NR, TOP);
			return;
		}

		Opcode(S[1], get_value(S[2]), 0, "");
	}
	else if (strcmp(S[1], "label") == 0)
	{

		if (NF != 2)
		{
			error("Error   %s %d : Incorrect number of operands\n", INFILE, NR);
			return;
		}

		_Label(S[2]);
	}
	else
	{
		error("Error   %s %d : Invalid assembler mnemonic '%s'\n", INFILE, NR, S[1]);
	}
}
/******************************************************************************
                             End of ASSEMBLE.C
******************************************************************************/