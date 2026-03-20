/***************************************************************************H1* 
 
      Filename    : PROTO.H 
      Description : C-Prototypes header file 
      Created     : August 11, 2001 
      Last Edit   : August 11, 2001 
 
******************************************************************************* 
                    Copyright (c) 2001 - Michael Heyns 
       This file contains confidential and proprietary information. 
      All use, disclosure, and/or reproduction is strictly prohibited. 
                           All rights reserved. 
******************************************************************************* 
 
      Notes       : *** This file was created by PROTO.BAT using HAWK *** 
                  : !!! DO NOT EDIT !!! 
                  : TABs = 4 spaces 
 
******************************************************************************/ 

// prototypes for 'ASMSTR.C'

	gen_asm_str(FILE *fp, int i);
	static char *GetVarName(int addr);
	static FindBitDefined(char *rname, int bit);

// prototypes for 'ASSEMBLE.C'

	Assemble();

// prototypes for 'COMMON.C'

	GetDestination(int addr);
	char *GetLabel(int addr);
	PrintSpaces(FILE *fp, int n);
	find_proc(char *name);
	char *StrCat(char *dest, char *source);
	char *StrChr(char *string, int symbol);

// prototypes for 'CONFIG.C'

	int OpenINIfile(char *Filename);
	int CloseINIfile(void);
	char *GetINIvalue(char *Section, char *Title);
	char *GetNEXTvalue(void);

// prototypes for 'CPUSPEC.C'

	IsPORT(char *pname);
	OnlyOne();
	MayNotMix();
	ProcToBytes();
	ForBytesOnly();
	Undefined(char *name);
	BitError();
	PageError();
	PortError();
	FindPortCount();
	findicount(char *varname);
	findpage(char *varname);
	w_findpage(char *varname);
	reset_datapage();
	setpage(char *varname);
	on_all_banks(int addr);
	setpageval(int addr);
	forcepagenum(int newpage);
	setpagenum(int newpage);
	char get_proc_code(int num);
	addr_is_inside_proc(int addr);
	find_free_space();
	find_low_reg(char *str);
	find_hi_reg(char *str);
	find_reg(char *str);
	show_unused_vars();
	get_binary(char ptr[], int len);
	isbin(char ch);
	ishex(char ch);
	unsigned int get_value(char ptr[]);
	plus(char *start);
	minus(char *start);
	bitand(char *start);
	bitnand(char *start);
	bitor(char *start);
	bitnor(char *start);
	bitxor(char *start);
	bitxnor(char *start);
	set_parameter(char *parm);
	set_equal(char *start);
	set_notequal(char *start);
	not_allowed(char *buf);
	gen_inc(char *ptr);
	gen_dec(char *ptr);
	gen_left(char *ptr);
	gen_right(char *ptr);
	is_proc(char *name);
	new_proc(char *name);
	is_referenced(char *name);
	new_lib(char *name);
	do_call_parm(char *proc);
	do_call(char *name);
	do_endproc();
	do_return();
	_CPUspecifics();
	validate_bit(char *string);
	static normal_test(int test, char *label);
	static do_test(int test, char *label);
	_Prologue();
	_Epilogue();
	_Label(char *LabelName);
	_JMP(char *label);
	_JE(int Positive, char *label);
	_JG(int Positive, char *label);
	_JGE(int Positive, char *label);
	_JBITSET(int Positive, char *label);
	_JBITCOMP(int Positive, char *label);

// prototypes for 'FIX.C'

	set_label_used(int addr);
	optimise_labels();
	optimise_bittests();
	has_label(int addr);
	optimise_cpage();
	static NewDot_oc(int i, char *name);
	optimise_calls();
	fix_jumps();
	static CalculatePage(int thispage, int thatpage);
	static SpaceError(int addr);
	static InsertOpcode(int addr, char *oc, int p1, int p2, char *str, int before);
	static DeleteOpcode(int addr);

// prototypes for 'GENASM.C'

	genasm(char *name);

// prototypes for 'GENCOD.C'

	static void qs_string(struct ST *item[], int left, int right);
	static codbyte(char value);
	static codlong(long value);
	static codword(int value);
	static codstring(char *str, int maxlen);
	static codfill();
	static codsymbol(char *name, int addr, int type);
	static BuildSymbolTable();
	gencod(char *name);

// prototypes for 'GENCODE.C'

	gencode();
	static f_limit(int reg);
	static n_limit(int i, int value);
	static o_limit(int i, int value);
	static b_limit(int i, int value);
	static addr_limit(int i, int addr);
	static getcode(int i);
	IncUsage(int addr);

// prototypes for 'GENHEX.C'

	genhex(char *name);
	static write_record(int len, int addr);
	static write_absolute(int len, int addr);

// prototypes for 'GENLST.C'

	genlst(char *name);

// prototypes for 'GENMAP.C'

	genmap(char *name);

// prototypes for 'OPCODE.C'

	Opcode(char *oc, int p1, int p2, char *str);

// prototypes for 'PIC_C.C'

	main(int argc, char *argv[]);
	GetFileIndex(char *Name);
	Add2FileList(char *Name);
	infile(char *Name);
	errfile(char *name);
	open_tmpfile();
	getline();
	fracture();
	void rebuild();
	void error(char *Fmt, ...);
	void warning(char *Fmt, ...);
	replace(char line[], char *pattern, char *newstr);
	islegal(char ch);
	char *new(unsigned long size);
	wipe_file(char *name, char *ext);

// prototypes for 'PROCESS.C'

	char *strrstr(char *buf, char *str);
	show_help();
	open_arc();
	ReplaceCFG(char *cfgfile);
	INITIALISE(int argc, char *argv[], char *IniFile);
	cannot_be_read(char *name);
	do_toptest();
	char *HeaderLabel();
	char *StartLabel();
	char *EndLabel();
	char *ORlabel();
	char *IF_ELSE_ENDlabel();
	char *JmpLabel();
	char *start_of(char ptr[]);
	cut_trails(char ptr[]);
	define_byte(char *name);
	define_define(char *Line, char *Token);
	do_lookups();
	void LINE();
	do_defs_macs();
	CleanLine();
	END();
	synch_datapage();
	do_exp(char *ptr, char rem_page);
	do_and(char *ptr);
	setup_indirect(char *ptr, char *var);
	do_single(char *ptr);
	do_operator(int Opcode, int Direction);
	gentest(int Opcode, int Direction, char *ENDlabel);
	terminate_if_group();
	raw_copy(char *dst, char *src);
	inside_copy(char *dst, char *src);
	do_defines();
	do_macros();
	swap(char *one, char *two);
	test_vars();
	give_error(char *ptr);
	find_selection(char *name);

// prototypes for 'RESOLVE.C'

	resolve_labels();
	static get_addr(char *pname);

// prototypes for 'VER.C'

	print_map_header();
	print_asm_header();
	print_lst_header();
	Hello();
	set_verstr(char *str);

// prototypes for 'HACK.C'

	read_key();
	hack_check1();
 
/****************************************************************************** 
                                End of file 
******************************************************************************/ 
