/******************************************************************************
      Filename    : GENCOD.C
      Product     : NITPIC
      Description : Generates the COD file
      Created     : August 4, 1999 [11:37:57]
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
#include <time.h>
#include <malloc.h>
#include <direct.h>
#include <io.h>

#include "general.h"
#include "pic_c.h"

static char CodePageUsed[64];                   // 64 allows for 16k
static struct ST *Symbol[MAXVARS + MAXLABELS + MAXCPUREG];
static int NextSymbol;

// qs_string - sorting through reentrancy
static void qs_string(struct ST *item[], int left, int right)
{
                register int i, j;
                struct ST *x, *y;

                i = left; j = right;
                x = item[(left+right)/2];

                do {
                                while (stricmp(item[i]->name, x->name) < 0 && i < right)
                                                i++;
                                while (stricmp(item[j]->name, x->name) > 0 && j > left)
                                                j--;
                                if (i <= j) {
                                                y = item[i];
                                                item[i] = item[j];
                                                item[j] = y;
                                                i++; j--;
                                }
                } while (i <= j);

                if (left < j)
                                qs_string(item, left, j);
                if (i < right)
                                qs_string(item, i, right);
}

// write a single byte to COD file
static codbyte(char value)
{
                fprintf(cod, "%c", value);
}

// write a LONG to COD file with *MSB* first
static codlong(long value)
{
                codbyte(value >> 24);
                codbyte(value >> 16);
                codbyte(value >> 8);
                codbyte(value);
}

// write a WORD to COD file with *LSB* first
static codword(int value)
{
                codbyte(value);
                codbyte(value >> 8);
}

// write a string value to COD file
static codstring(char *str, int maxlen)
{
                int i, len = min(maxlen, strlen(str));

                codbyte(len);
                for (i = 0; i < len; i++)
                                codbyte(str[i]);
                for (i = len; i < maxlen; i++)
                                codbyte(0);
}

// fill the COD file with number of 0s
static codfill()
{
                int i, page = (ftell(cod) / 512) + 1;
                int todo = (page * 512) - ftell(cod);

                if (todo < 512) {
                                for (i = 0; i < todo; i++)
                                                codbyte(0);
                }
}

// enter symbol into global symbol table
static codsymbol(char *name, int addr, int type)
{
                // no need to check overflow - max checks already done by now
                Symbol[NextSymbol] = (struct ST *)new(sizeof(struct ST));
                Symbol[NextSymbol]->name = name;
                Symbol[NextSymbol]->addr = addr;
                Symbol[NextSymbol]->type = type;
                NextSymbol++;
}

static BuildSymbolTable()
{
                int i;
                static char cpusymbol[CPULEN+3];

                for (i = 0; i < NextReg; i++)
                                codsymbol(Reg[i], RegAddr[i], 0x2f);
                for (i = 0; i < NextVar; i++)
                                codsymbol(Variable[i], VarAddr[i], 0x2f);
                for (i = 0; i < NextLabel; i++) {
                                // only print used labels
                                if (Label[i].Used)
                                                codsymbol(Label[i].Name, Label[i].Address, 0x2e);
                }
                sprintf(cpusymbol, "__%s", &CPU[3]);
                codsymbol(cpusymbol, 1, 0x8b);

                // sort the table
                qs_string(Symbol, 0, NextSymbol - 1);
}

// generate the actual COD file
gencod(char *name)
{
                time_t today;
                char *buf = new(L_FLDLEN);
                int i, j, k, line, lastline, len;
                int TOP = CodePageSize * (TopCodePage + 1);
                int CodeBlocks = (TopROM + 1) / 256;    // i.e. 512byte blocks of code
                char *ptr;
                int Symtab, Symend, Namtab, Namend, Lsttab, Lstend;
                int MemMapOfs, MemMapEnd, LSymTab, LSymEnd;

                strncpy(buf, name, L_FLDLEN-1);
                ptr = strrchr(buf, '.');
                if (ptr)
                                strcpy(ptr, ".COD");
                else
                                strcat(buf, ".COD");
                unlink(buf);

                if (Errors > 0) {
                                free(buf);
                                return;
                }

                if (!(cod = fopen(buf, "wb"))) {                // MUST be binary !!!
                                error("Error   %s - : Cannot open '%s' for writing\n", MainFile, buf);
                                free(buf);
                                return;
                }

                // build general symbol table
                BuildSymbolTable();

                // determine which 512 byte blocks (pages) will be used
                for (i = 0; i < CodeBlocks; i++) {              // for all code blocks
                                for (j = 0; j < 256; j++) {
                                                if (OCREF[(i*256) + j].code > 0) {
                                                                CodePageUsed[i] = 1;    // page has code
                                                                break;                  // no need to test further
                                                }
                                }
                }

                // now write code block array
                j = 1;                                                                  // code will start with block 1
                for (i = 0; i < CodeBlocks; i++) {              // for all code blocks used
                                if (CodePageUsed[i]) {
                                                codword(j);                     // say which block
                                                j++;                                    // next block will follow
                                }
                                else
                                                codword(0);
                }
                for (i = CodeBlocks + 1; i <= 128; i++) // fill to 100H only !!
                                codword(0);

                // primary source file
                getcwd(buf, L_FLDLEN-1);
                strcat(buf, "\\");
                strcat(buf, FileList[0]);
                codstring(buf, 63);

                // date
                today = time(NULL);
                strcpy(buf, ctime(&today));
                codbyte(7);                                                     // len
                codbyte(buf[8]);                                                // day (dd)
                codbyte(buf[9]);
                codbyte(buf[4]);                                                // month (MMM)
                codbyte(buf[5]);
                codbyte(buf[6]);
                codbyte(buf[22]);                                               // year (yy)
                codbyte(buf[23]);
                codword(today >> 8);                                    // time - sort of seconds

                // version string
                sprintf(buf, "%s.%s.%s", PR, MAJ, MIN);
                codstring(buf, 19);

                // compiler name
                codstring("NITPIC", 11);

                // Notice
                codstring("Copyright (c) 2002 Michael Heyns", 63);

                // Fill the rest for now
                codfill();

                // write code blocks
                for (i = 0; i < CodeBlocks; i++) {              // code block numbers
                                if (CodePageUsed[i]) {
                                                for (j = 0; j < 256; j++)
                                                                codword(OCREF[(i * 256) + j].code);
                                }
                }

                // write memory map
                MemMapOfs = ftell(cod) / 512;
                codword(0);
                codword(TOP);
                codfill();
                MemMapEnd = (ftell(cod) / 512) - 1;

                // symbol table
                Symtab = ftell(cod) / 512;
                for (i = 0; i < NextSymbol; i++) {
                                codstring(Symbol[i]->name, 12);
                                codbyte(Symbol[i]->type);
                                codword(Symbol[i]->addr);
                }
                codfill();
                Symend = (ftell(cod) / 512) - 1;

                // long symbol table
                LSymTab = ftell(cod) / 512;
                for (i = 0; i < NextSymbol; i++) {
                                len = strlen(Symbol[i]->name);
                                if (len > 0 && len <= 255) {
                                                codstring(Symbol[i]->name, len);
                                                codword(Symbol[i]->type);
                                                codlong(Symbol[i]->addr);
                                }
                }
                codfill();
                LSymEnd = (ftell(cod) / 512) - 1;

                // source filename blocks
                Namtab = ftell(cod) / 512;
                for (i = 0; i < NextFile; i++) {
                                if (strchr(FileList[i], ':'))
                                                codstring(FileList[i], 63);
                                else {
                                                getcwd(buf, L_FLDLEN-1);
                                                strcat(buf, "\\");
                                                strcat(buf, FileList[i]);
                                                codstring(buf, 63);
                                }
                }
                codfill();
                Namend = (ftell(cod) / 512) - 1;

                // source/listing file information blocks
                Lsttab = ftell(cod) / 512;
                codbyte(0);
                codbyte(0xff);
                codword(0);
//              codword(6);                                                     // LST/SRC file header
                codword(0);                                                     // LST/SRC file header

                k = 1;                                                                  // 1 down !!!
                line = 0;
                lastline = -1;
                for (i = 0; i < TOP; i++) {
                                // catch up to current LST line with 'extra info'
                                while (line < OCREF[i].lst_index) {
                                                codbyte(OCREF[i].fileid);
                                                codbyte(0x10);
                                                codword(OCREF[i].line);
                                                codword(i);
                                                line++;
                                                k++;
                                                if (k == 85) {
                                                                codword(0);                     // filler
                                                                k = 0;
                                                }
                                }

                                // write source lines
                                if (OCREF[i].line != lastline && OCREF[i].line != 0) {
                                                codbyte(OCREF[i].fileid);
                                                codbyte(0x80);                  // source
                                                codword(OCREF[i].line);
                                                lastline = OCREF[i].line;
                                                codword(i);                     // code address
                                                k++;
                                                if (k == 85) {
                                                                codword(0);                     // filler
                                                                k = 0;
                                                }
                                }

                                // else it's 'extra info'
                                else if (OCREF[i].line != 0) {
                                                codbyte(OCREF[i].fileid);
//                                              codbyte(0x80);                  // source
                                                codbyte(0x12);
                                                codword(lastline);
                                                codword(i);                     // code address
                                                k++;
                                                if (k == 85) {
                                                                codword(0);                     // filler
                                                                k = 0;
                                                }
                                }
                }
                codfill();
                Lstend = (ftell(cod) / 512) - 1;

                // fix up header
                fseek(cod, 0x1aa, SEEK_SET);
                codword(Symtab);
                codword(Symend);
                codword(Namtab);
                codword(Namend);
                codword(Lsttab);
                codword(Lstend);
                codbyte(4);                     // AddrSize
                codword(0);                     // HighAddr
                codword(0);                     // NextDir
                codword(MemMapOfs);
                codword(MemMapEnd);
                codword(0);                     // Localvars
                codword(0);                     // Localend
                codword(0);                     // COD type
                codstring(&CPU[3], 8);
                codword(LSymTab);
                codword(LSymEnd);
                codword(0);                     // MessTab
                codword(0);                     // MessEnd

                fclose(cod);
                free(buf);
}

/******************************************************************************
                              End of GENCOD.C
******************************************************************************/
