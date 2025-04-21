/*
 * Program to control ICOM radios
 *
 * Tables and chairs
 */
#include "icom.h"
#include <stdio.h>
#include <termios.h>

/*
 * Commands
 */
#define	ARG2(y, x)	(((y) << 8) | (x))
#define	ARG3(z, y, x)	(((z) << 16) | ((y) << 8) | (x))
#define	M_CTL(z, x)	ARG3((z), V_WRCTL, (x))
#define	M_MTR(z, x)	ARG3((z), V_RMTR, (x))
#define	M_TOG(z, x)	ARG3((z), V_TOGL, (x))
#define	M_SET(z, x)	ARG2((z), (x))

/*
 * Keyboard mode commands
 */
struct cmdtable cmd[] = {
	{"agc",		C_AGC,		"AGC fast/medium/slow"},
	{"ant",		C_ANT,		"select antenna"},
	{"atten",	C_ATTEN,	"select attenuator"},
	{"band",	C_BAND,		"set band limits"},
	{"bank",	C_BANK,		"read/write bank/name"},
	{"bfocomp",	C_BCOMP,	"set BFO compensation"},
	{"break",	C_BREAK,	"break off/semi/full"},
	{"chan",	C_READ,		"read channels"},
	{"ctl",		C_CTRL,		"control commands"},
	{"default",	C_DEFLT,	"set default arguments"},
	{"dial",	C_DIAL,		"set dial tuning step"},
	{"dtcs",	C_DTCS,		"set digital tone squelch code"},	
	{"dump",	C_DUMP,		"dump VFO (debug)"},
	{"duplex",	C_DUPLEX,	"set transmit duplex"},
	{"empty",	C_EMPTY,	"empty channel"},
	{"include",	C_INCLD,	"include file"},
	{"key",		C_KEY,		"send CW message"},
	{"meter",	C_METER,	"read meters and squelch"},
	{"misc",	C_MISC,		"miscellaneous control"},
	{"mode",	C_MODE,		"set mode"},
	{"mvfo",	C_VFOM,		"memory -> VFO"},
	{"name",	C_NAME,		"set channel name"},
	{"pad",		C_KEYPAD,	"switch to keypad mode"},
	{"power",	C_POWER,	"power on/off"},
	{"preamp",	C_PAMP,		"preamp off/1/2f"},
	{"ptt",		C_PTT,		"push to talk"},
	{"quit",	C_QUIT,		"exit program"},
	{"radio",	C_RADIO,	"select radio"},
	{"rate",	C_RATE,		"set tuning rate"},
	{"restore",	C_RESTORE,	"restore channels"},
	{"rx",		C_RX,		"receive"},
	{"save",	C_SAVE,		"save channels"},
	{"say",		C_ANNC,		"announce control"},
	{"scan",	C_SCAN,		"scan control"},
	{"set",		C_SWTCH,	"set/clear switches"},
	{"split",	C_SPLIT,	"split/duplex commands"},
	{"step",	C_STEP,		"set tuning step"},
	{"swap",	C_SWAP,		"VFO A <-> VFO B"},
	{"test",	C_TEST,		"send CI-V test message"},
	{"tone",	C_TONE,		"set repeater tone frequency"},
	{"trace",	C_DEBUG,	"trace CI-V messages"},	
	{"tsql",	C_TSQL,		"set tone squelch frequency"},
	{"tx",		C_TX,		"transmit"},
	{"utc",		C_TIME,		"set/display date and time"},
	{"verbose",	C_VERB,		"set verbose"},
	{"vfo",		C_VFO,		"vfo commands"},
	{"vfocomp",	C_VCOMP,	"set VFO compensation"},
	{"write",	C_WRITE,	"write channels"},
	{"\0",		C_FREQX,	"set VFO frequency/mode"}
};

/*
 * Keypad mode commands
 */
struct cmdtable key[] = {
	{{KILL, '\0'},	C_ERASE,	"erase input"},
	{"[11",		C_ANT,		"F1 ant status"},
	{"[12",		C_AGC,		"F2 agc status"},
	{"[13",		C_ATTEN,	"F3 atten status"},
	{"[14",		C_PAMP,		"F4 preamp status"},
	{"[15",		C_BREAK,	"F5 break status"},
	{"[17",		C_ANNC,		"F6 say status"},
	{"[18",		C_METER,	"F7 display meters"},
	{"[19",		C_SWTCH,	"F8 display switches"},
	{"[20",		C_CTRL,		"F9 display controls"},
	{"[a",		C_UP,		"UP step up"},
	{"[b",		C_DOWN,		"DOWN step down"},
	{"[c",		C_RUP,		"RIGHT tuning rate up"},
	{"[d",		C_RDOWN,	"LEFT tuning rate down"},
	{"c",		C_READ,		"read channel"},
	{"e",		C_EMPTY,	"empty channel"},
	{"q",		C_KEYBD,	"switch to keyboard mode"},
	{"r",		C_RX,		"receive"},
	{"s",		C_SPLIT,	"split mode with offset"},
	{"t",		C_TX,		"transmit"},
	{"u",		C_TIME,		"display date and time"},
	{"w",		C_WRITE,	"write channel"},
	{"x",		C_SWAP,		"VFO A <-> VFO B"},
	{"\0",		C_FREQX,	"set VFO frequency/mode"}
};

/*
 * dummy argument
 */
struct cmdtable dummy[] = {
	{"\0",		R_NOP,		"invalid command"}
};

/*
 * channel/bank arguments
 */
struct cmdtable argch[] = {
	{"[empty]",	0x00,		"current bank/chan"},
	{"$",		0x00,		"current bank, all chans"},
	{"b.$",		0x00,		"bank b, all chans"},
	{"$:$",		0x00,		"all banks, all chans"},
	{"+",		0x00,		"current bank/chan plus 1"},
	{"-",		0x00,		"current bank/chan minus 1"},
	{"c",		0x00,		"current bank, chan c"},
	{"b.c",		0x00,		"bank b, chan c"},
	{"c1:c2",	0x00,		"current bank, block c1-c2"},
	{"b1.c1:b2.c2",	0x00,		"block b1.c1-b2.c2"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "verbose" arguments
 */
struct cmdtable verbx[] = {
	{"off",		0,		"terse"},
	{"on",		P_VERB,		"verbose"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "trace" arguments
 */
struct cmdtable dbx[] = {
	{"all",		P_TRACE|P_ERMSG, "trace bus, errors"},
	{"bus",		P_TRACE,	"trace bus"},
	{"off",		0,		"trace none"},
	{"packet",	P_ERMSG,	"trace errors"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "vfo" subcommands (V_SVFO 0x07 command)
 */
struct cmdtable vfo[] = {
	{"a",		0x00,		"select VFO A"},
	{"b",		0x01,		"select VFO B"},
	{"btoa",	0xa0,		"VFO B -> VFO A"},
	{"equal",	0xb1,		"main VFO -> sub VFO"},
	{"main",	0xd0,		"select main VFO"},
	{"nowatch",	0xc0,		"dual-watch off"},
	{"sub",		0xd1,		"select sub VFO"},
	{"swap",	0xb0,		"VFO A <-> VFO B"},
	{"watch",	0xc1,		"dual-watch on"},
	{"\0",		R_NOP,		"invalid argument"}
};
 
/*
 * "split" subcommands (V_SPLIT 0x0f command)
 */
struct cmdtable split[] = {
	{"dup+",	0x12,		"duplex up"},
	{"dup-",	0x11,		"duplex down"},
	{"off",		0x00,		"split off"},
	{"on",		0x01,		"split on"},
	{"simplex",	0x10,		"simplex"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "atten" subcommands (V_ATTEN 0x11 command)
 */
struct cmdtable atten[] = {
	{"0",		0x00,		"off"},
	{"6",		0x06,		"6 dB"},
	{"10",		0x10,		"10 dB"},
	{"12",		0x12,		"12 dB"},
	{"18",		0x18,		"18 dB"},
	{"20",		0x20,		"20 dB"},
	{"30",		0x30,		"30 dB"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "ant" subcommands (V_SANT 0x12 command)
 */
struct cmdtable ant[] = {
	{"1",		ARG2(0x00,0x00), "xmt/rec antenna 1"},
	{"1R",		ARG2(0x01,0x00), "xmt antenna 1/rec antenna R"},
	{"2",		ARG2(0x00,0x01), "xmt/rec antenna 2"},
	{"2R",		ARG2(0x01,0x01), "xmt antenna 2/rec antenna R"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "say" subcommands (V_ANNC 0x13 command)
 */
struct cmdtable say[] = {
	{"all",		0x00,		"all data"},
	{"freq",	0x01,		"S meter, frequency and mode"},
	{"mode",	0x02,		"receive mode"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "ctl" subcommands (V_WRCTL 0x14 command)
 */
struct cmdtable ctlc[] = {
	{"af",		M_CTL(G,0x01),	"af gain"},
	{"antivox",	M_CTL(G,0x17),	"anti-vox gain"},
	{"balance",	M_CTL(G,0x10),	"balance"},
	{"blank",	M_CTL(G,0x12),	"noise blanker threshold"},
	{"bright",	M_CTL(G,0x19),	"LCD brightness"},
	{"comp",	M_CTL(G,0x0e),	"speech compressor gain"},
	{"contrast",	M_CTL(G,0x18),	"LCD contrast"},
	{"delay",	M_CTL(G,0x0f),	"break delay"},
	{"mic",		M_CTL(G,0x0b),	"microphone gain"},
	{"monitor",	M_CTL(G,0x15),	"monitor gain"},
	{"nf1",		M_CTL(F,0x0d),	"manual notch nf1 frequency"},
	{"nf2",		M_CTL(F,0x1a),	"manual notch nf2 frequency"},
	{"nr",		M_CTL(G,0x06),	"noise reduction"},
	{"pbti",	M_CTL(F,0x07),	"passband tuning (inner"},
	{"pbto",	M_CTL(F,0x08),	"passband tuning (outer"},
	{"pitch",	M_CTL(F,0x09),	"cw pitch"},
	{"power",	M_CTL(G,0x0a),	"rf power"},
	{"rf",		M_CTL(G,0x02),	"rf gain"},
	{"speed",	M_CTL(G,0x0c),	"key speed"},
	{"sql",		M_CTL(G,0x03),	"squelch threshold"},
	{"vox",		M_CTL(G,0x16),	"vox gain"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "meter" subcommands (V_RMTR 0x15 command)
 */
struct cmdtable meter[] = {
	{"alc",		M_MTR(G,0x13),	"read ALC meter"},
	{"comp",	M_MTR(G,0x14),	"read COMP meter"},
	{"power",	M_MTR(G,0x11),	"read RF power meter"},
	{"signal",	M_MTR(S,0x02),	"read S meter"},
	{"sql",		M_MTR(Q,0x01),	"read squelch condition"},
	{"swr",		M_MTR(G,0x12),	"read SWR meter"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "set" subcommands (V_TOGL 0x16 command)
 */
struct cmdtable switches[] = {
	{"agc",		M_TOG(A,0x12),	"agc fast/mid/slow"},
	{"agcfast",	M_TOG(P,0x11),	"agc fast (R8500)"},
	{"agcslow",	M_TOG(P,0x10),	"agc slow (R8500)"},
	{"an",		M_TOG(W,0x41),	"auto notch"},
	{"apfoff",	M_TOG(W,0x30),	"auto peak filter off (R8500)"},
	{"apfon",	M_TOG(W,0x31),	"auto peak filter on (R8500)"},
	{"break",	M_TOG(B,0x47),	"break off/semi/full"},
	{"comp",	M_TOG(W,0x44),	"speech compressor"},
	{"dcts",	M_TOG(W,0x4b),	"digital tone squelch"},
	{"lock",	M_TOG(W,0x50),	"dial lock"},
	{"monitor",	M_TOG(W,0x45),	"monitor"},
	{"nb",		M_TOG(W,0x22),	"noise blanker"},
	{"nboff",	M_TOG(P,0x20),	"noise blanker off (R8500)"},
	{"nbon",	M_TOG(P,0x21),	"noise blanker on(R8500)"},
	{"nf1",		M_TOG(W,0x48),	"manual notch nf1"},
	{"nf2",		M_TOG(W,0x51),	"manual notch nf2"},
	{"nr",		M_TOG(W,0x40),	"noise reduction"},
	{"peak",	M_TOG(W,0x4f),	"twin peak filter"},
	{"preamp",	M_TOG(P,0x02),	"preamp off/1/2"},
	{"rtty",	M_TOG(W,0x49),	"RTTY filter"},
	{"tone",	M_TOG(W,0x42),	"repeater tone"},
	{"tsql",	M_TOG(W,0x43),	"tone squelch"},
	{"vcs",		M_TOG(W,0x4c),	"voice squelch"},
	{"vox",		M_TOG(W,0x46),	"voice break"},
	{"\0",		R_NOP,		"invalid argument"},
};

struct cmdtable fmtw[] = {
	{"off",		M_SET(W,0x00),	"switch off"},
	{"on",		M_SET(W,0x01),	"switch on"},
	{"\0",		R_NOP,		"invalid argument"},
};

/*
 * "power" subcommands (V_POWER 0x18 command)
 */
struct cmdtable power[] = {
	{"off",		0x00,		"power off at sleep timeout"},
	{"on",		0x01,		"power on"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "tone" and "tsql" subcommands (V_TONE 0x1b command)
 *
 * The CTSS tone frequency is in tenths of a Hertz. The vollowing
 * frequencies are valid for both the repeater tone (tone command) and
 * tone squelch (tsql command) functions.
 */ 
struct cmdtable tone[] = {
	{"off",		0x00,		"CTSS off)"},
	{"on",		0x01,		"CTSS on)"},
	{"67.0",	0x0670,		"XZ (67.0 Hz)"},
	{"69.3",	0x0693,		"WZ (69.3 Hz)"},
	{"71.9",	0x0719,		"XA (71.9 Hz)"},
	{"74.4",	0x0744,		"WA (74.4 Hz)"},
	{"77.0",	0x0770,		"XB (77.0 Hz)"},
	{"79.7",	0x0797,		"WB (79.7 Hz)"},
	{"82.5",	0x0825,		"YZ (82.5 Hz)"},
	{"85.4",	0x0854,		"YA (85.4 Hz)"},
	{"88.5",	0x0885,		"YB (88.5 Hz)"},
	{"91.5",	0x0915,		"ZZ (91.5 Hz)"},
	{"94.8",	0x0948,		"ZA (94.8 Hz)"},
	{"97.4",	0x0974,		"ZB (97.4 Hz)"},
	{"100.0",	0x1000,		"1Z (100.0 Hz)"},
	{"103.5",	0x1035,		"1A (103.5 Hz)"},
	{"107.2",	0x1072,		"1B (107.2 Hz)"},
	{"110.9",	0x1109,		"2Z (110.9 Hz)"},
	{"114.8",	0x1148,		"2A (114.8 Hz)"},
	{"118.8",	0x1188,		"2B (118.8 Hz)"},
	{"123.0",	0x1230,		"3Z (123.0 Hz)"},
	{"127.3",	0x1273,		"3A (127.3 Hz)"},
	{"131.8",	0x1318,		"3B (131.8 Hz)"},
	{"136.5",	0x1365,		"4Z (136.5 Hz)"},
	{"141.3",	0x1413,		"4A (141.3 Hz)"},
	{"146.2",	0x1462,		"4B (146.2 Hz)"},
	{"151.4",	0x1514,		"5Z (151.4 Hz)"},
	{"156.7",	0x1567,		"5A (156.7 Hz)"},
	{"159.8",	0x1598,		"-- (159.8 Hz)"},
	{"162.2",	0x1622,		"5B (162.2 Hz)"},
	{"165.5",	0x1655,		"-- (165.5 Hz)"},
	{"167.9",	0x1679,		"6Z (167.9 Hz)"},
	{"171.3",	0x1713,		"-- (171.3 Hz)"},
	{"173.8",	0x1738,		"6A (173.8 Hz)"},
	{"177.3",	0x1773,		"-- (177.3 Hz)"},
	{"179.9",	0x1799,		"6B (179.9 Hz)"},
	{"183.5",	0x1835,		"-- (183.5 Hz)"},
	{"186.2",	0x1862,		"7Z (186.2 Hz)"},
	{"189.9",	0x1899,		"-- (189.9 Hz)"},
	{"192.8",	0x1928,		"7A (192.8 Hz)"},
	{"196.6",	0x1968,		"-- (196.6 Hz)"},
	{"199.5",	0x1995,		"-- (199.5 Hz)"},
	{"203.5",	0x2035,		"M1 (203.5 Hz)"},
	{"206.5",	0x2065,		"-- (206.5 Hz)"},
	{"210.7",	0x2107,		"M2 (210.7 Hz)"},
	{"218.1",	0x2181,		"M3 (218.1 Hz)"},
	{"225.7",	0x2257,		"M4 (225.7 Hz)"},
	{"229.1",	0x2291,		"9Z (229.1 Hz)"},
	{"233.6",	0x2336,		"M5 (233.6 Hz)"},
	{"241.8",	0x2418,		"M6 (241.8 Hz)"},
	{"250.3",	0x2503,		"M7 (250.3 Hz)"},
	{"254.1",	0x2541,		"0Z (254.1 Hz)"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * tone command on/off
 */
struct cmdtable toneon[] = {
	{"off",		0x00,		"tone off"},
	{"on",		0x01,		"tone on"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "dtcs" subcommands (V_TONE 0x1b command)
 *
 * Thd DTCS code is nnn-pq, where nnn is 3 decimal digits, p is the
 * transmit polarity and q is the receive polarity. Polarity is N for
 * normal and R for reversed. The following codes are valid for the
 * digital tone squelch function (dtcs command).
 */
struct cmdtable dtcs[] = {
	{"off",		0x00,		"DTCS off)"},
	{"on",		0x01,		"DTCS on)"},
	{"023",		0x0023,		"DTCS 023"},
	{"025",		0x0025,		"DTCS 025"},
	{"026",		0x0026,		"DTCS 026"},
	{"031",		0x0031,		"DTCS 031"},
	{"032",		0x0032,		"DTCS 032"},
	{"036",		0x0036,		"DTCS 036"},
	{"043",		0x0043,		"DTCS 043"},
	{"047",		0x0047,		"DTCS 047"},
	{"051",		0x0051,		"DTCS 051"},
	{"053",		0x0053,		"DTCS 054"},
	{"054",		0x0054,		"DTCS 055"},
	{"055",		0x0055,		"DTCS 071"},
	{"071",		0x0071,		"DTCS 071"},
	{"072",		0x0072,		"DTCS 072"},
	{"073",		0x0073,		"DTCS 073"},
	{"074",		0x0074,		"DTCS 074"},
	{"114",		0x0114,		"DTCS 114"},
	{"115",		0x0115,		"DTCS 115"},
	{"116",		0x0116,		"DTCS 116"},
	{"122",		0x0122,		"DTCS 122"},
	{"125",		0x0125,		"DTCS 125"},
	{"131",		0x0131,		"DTCS 131"},
	{"132",		0x0132,		"DTCS 132"},
	{"134",		0x0134,		"DTCS 134"},
	{"143",		0x0143,		"DTCS 143"},
	{"145",		0x0145,		"DTCS 145"},
	{"152",		0x0152,		"DTCS 152"},
	{"155",		0x0155,		"DTCS 155"},
	{"156",		0x0156,		"DTCS 156"},
	{"162",		0x0162,		"DTCS 162"},
	{"165",		0x0165,		"DTCS 165"},
	{"172",		0x0172,		"DTCS 172"},
	{"174",		0x0174,		"DTCS 174"},
	{"205",		0x0205,		"DTCS 205"},
	{"212",		0x0212,		"DTCS 212"},
	{"223",		0x0223,		"DTCS 223"},
	{"225",		0x0225,		"DTCS 225"},
	{"226",		0x0226,		"DTCS 226"},
	{"243",		0x0243,		"DTCS 243"},
	{"244",		0x0244,		"DTCS 244"},
	{"245",		0x0245,		"DTCS 245"},
	{"246",		0x0246,		"DTCS 246"},
	{"251",		0x0251,		"DTCS 251"},
	{"252",		0x0252,		"DTCS 252"},
	{"255",		0x0255,		"DTCS 255"},
	{"261",		0x0261,		"DTCS 261"},
	{"263",		0x0263,		"DTCS 263"},
	{"265",		0x0265,		"DTCS 265"},
	{"266",		0x0266,		"DTCS 266"},
	{"271",		0x0271,		"DTCS 271"},
	{"274",		0x0274,		"DTCS 274"},
	{"306",		0x0306,		"DTCS 306"},
	{"311",		0x0311,		"DTCS 311"},
	{"315",		0x0315,		"DTCS 315"},
	{"325",		0x0325,		"DTCS 325"},
	{"331",		0x0331,		"DTCS 331"},
	{"332",		0x0332,		"DTCS 332"},
	{"343",		0x0343,		"DTCS 343"},
	{"346",		0x0346,		"DTCS 346"},
	{"351",		0x0351,		"DTCS 351"},
	{"356",		0x0356,		"DTCS 356"},
	{"364",		0x0364,		"DTCS 364"},
	{"365",		0x0365,		"DTCS 365"},
	{"371",		0x0371,		"DTCS 371"},
	{"411",		0x0411,		"DTCS 411"},
	{"412",		0x0412,		"DTCS 412"},
	{"413",		0x0413,		"DTCS 413"},
	{"423",		0x0423,		"DTCS 423"},
	{"431",		0x0431,		"DTCS 431"},
	{"432",		0x0432,		"DTCS 432"},
	{"445",		0x0445,		"DTCS 445"},
	{"446",		0x0446,		"DTCS 446"},
	{"452",		0x0452,		"DTCS 452"},
	{"454",		0x0454,		"DTCS 454"},
	{"455",		0x0455,		"DTCS 455"},
	{"462",		0x0462,		"DTCS 462"},
	{"464",		0x0464,		"DTCS 464"},
	{"465",		0x0465,		"DTCS 465"},
	{"466",		0x0466,		"DTCS 466"},
	{"503",		0x0503,		"DTCS 503"},
	{"506",		0x0506,		"DTCS 506"},
	{"516",		0x0516,		"DTCS 516"},
	{"523",		0x0523,		"DTCS 523"},
	{"526",		0x0526,		"DTCS 526"},
	{"532",		0x0532,		"DTCS 532"},
	{"546",		0x0546,		"DTCS 546"},
	{"565",		0x0565,		"DTCS 565"},
	{"606",		0x0606,		"DTCS 606"},
	{"612",		0x0612,		"DTCS 612"},
	{"624",		0x0624,		"DTCS 624"},
	{"627",		0x0627,		"DTCS 627"},
	{"631",		0x0631,		"DTCS 631"},
	{"632",		0x0632,		"DTCS 632"},
	{"654",		0x0654,		"DTCS 654"},
	{"662",		0x0662,		"DTCS 662"},
	{"664",		0x0664,		"DTCS 664"},
	{"703",		0x0703,		"DTCS 703"},
	{"712",		0x0712,		"DTCS 712"},
	{"723",		0x0723,		"DTCS 723"},
	{"731",		0x0731,		"DTCS 731"},
	{"732",		0x0732,		"DTCS 732"},
	{"734",		0x0734,		"DTCS 734"},
	{"743",		0x0743,		"DTCS 743"},
	{"754",		0x0754,		"DTCS 754"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * DTCS polarity codes
 *
 * These codes are used with the DTCS codes given above. An "N" codes
 * for normal polarity, "R" reversed.
 */
struct cmdtable polar[] = {
	{"NN",		0x00,		"+ transmit, + receive"},
	{"NR",		0x01,		"+ transmit, - receive"},
	{"RN",		0x10,		"- transmit, + receive"},
	{"RR",		0x11,		"- transmit, - receive"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "tx" subcommands (V_TX 0x1c command)
 */
struct cmdtable tx[] = {
	{"off",		0x00,		"receive"},
	{"on",		0x01,		"transmit"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "misc" subcommands (775)
 */
struct cmdtable misc[] = {
	{"ctss",	0x06,		"read CTSS"},
	{"dcs",		0x07,		"read DCS"},
	{"dtmf",	0x08,		"read DTMF"},
	{"id",		0x09,		"read ID"},
	{"local",	0x01,		"local control"},
	{"next",	0x0e,		"next frequency"},
	{"nosearch",	0x10,		"disable search"},
	{"nospeaker",	0x0b,		"mute speaker"},
	{"notape",	0x04,		"disable tape recorder"},
	{"nowindow",	0x0d,		"disable search window"},
	{"opto",	0x05,		"read OPTO"},
	{"remote",	0x02,		"remote control"},
	{"search",	0x0f,		"enable search"},
	{"speaker",	0x0a,		"unmute speaker"},
	{"tape",	0x03,		"enable tape recorder"},
	{"window",	0x0c,		"enable search window"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "agc" subcommands
 */
struct cmdtable agc[] = {
	{"fast",	M_SET(A,0x01),	"agc fast"},
	{"medium",	M_SET(A,0x02),	"agc mid"},
	{"slow",	M_SET(A,0x03),	"agc slow"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "preamp" subcommands
 */
struct cmdtable preamp[] = {
	{"off",		0x00,		"preamp off"},
	{"1",		0x01,		"preamp 1"},
	{"2",		0x02,		"preamp 2"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "break" subcommands
 */
struct cmdtable fmtb[] = {
	{"full",	M_SET(B,0x02),	"break full"},
	{"off",		M_SET(B,0x00),	"break off"},
	{"semi",	M_SET(B,0x01),	"break semi"},
	{"\0",		R_NOP,		"invalid argument"},
};

/*
 * "scan" subcommands
 */
struct cmdtable scan[] = {
	{"10",		0xa3,		"delta-f 10 kHz"},
	{"2.5",		0xa1,		"delta-f 2.5 kHz"},
	{"20",		0xa4,		"delta-f 20 kHz"},
	{"5",		0xa2,		"delta-f 5 kHz"},
	{"50",		0xa5,		"delta-f 50 kHz"},
	{"autowrite",	0x04,		"auto write scan"},
	{"deltaf",	0x03,		"delta-f scan"},
	{"finedeltaf",	0x13,		"fine delta-f scan start"},
	{"fineprog",	0x12,		"fine program scan start"},
	{"fix",		0xaa,		"fix center frequency"},
	{"mem",		0x22,		"memory scan start"},
	{"memchan",	0xb2,		"memory channel scan number"},
	{"memnum",	0x23,		"select memory scan start"},
	{"nofix",	0xa0,		"unfix center frequency"},
	{"noresume",	0xd1,		"scan resume off"},
	{"noskip",	0xb1,		"select memory channel"},
	{"noresume",	0xd0,		"scan resume never"},
	{"novsc",	0xc0,		"VSC off"},
	{"priority",	0x42,		"priority scan"},
	{"prog",	0x02,		"program scan"},
	{"resume",	0xd3,		"scan resume after delay"},
	{"resumeb",	0xd2,		"scan resume b"},
	{"select",	0x24,		"selected mode memory scan"},
	{"skip",	0xb0,		"deselect memory channel"},
	{"start",	0x01,		"scan start"},
	{"stop",	0x00,		"scan stop"},
	{"vsc",		0xc1,		"VSC on"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * "dial" subcommands
 */
struct cmdtable diala[] = {		/* R8500 */
	{".01",		0x00,		"10 Hz"},
	{".05",		0x01,		"50 Hz"},
	{".1",		0x02,		"100 Hz"},
	{"1",		0x03,		"1 kHz"},
	{"2.5",		0x04,		"2.5 kHz"},
	{"5",		0x05,		"5 kHz"},
	{"9",		0x06,		"9 kHz"},
	{"10",		0x07,		"10 kHz"},
	{"12.5",	0x08,		"12.5 kHz"},
	{"20",		0x09,		"20 kHz"},
	{"25",		0x10,		"25 kHz"},
	{"100",		0x11,		"100 kHz"},
	{"1000",	0x12,		"1 MHz"},
	{"*",		0x13,		"* kHz"},
	{"\0",		R_NOP,		"invalid argument"}
};

struct cmdtable dialb[] = {		/* 775 */
	{".01",		0x00,		"10 Hz"},
	{"1",		0x01,		"1 kHz"},
	{"2",		0x02,		"2 kHz"},
	{"3",		0x03,		"3 kHz"},
	{"4",		0x04,		"4 kHz"},
	{"5",		0x05,		"5 kHz"},
	{"6",		0x06,		"6 kHz"},
	{"7",		0x07,		"7 kHz"},
	{"8",		0x08,		"8 kHz"},
	{"9",		0x09,		"9 kHz"},
	{"10",		0x10,		"10 kHz"},
	{"\0",		R_NOP,		"invalid argument"}
};

struct cmdtable dialc[] = {		/* 706G, 756, 7000 */
	{".01",		0x00,		"10 Hz"},
	{".1",		0x01,		"100 Hz"},
	{"1",		0x02,		"1 kHz"},
	{"5",		0x03,		"5 kHz"},
	{"9",		0x04,		"9 kHz"},
	{"10",		0x05,		"10 kHz"},
	{"12.5",	0x06,		"12.5 kHz"},
	{"20",		0x07,		"20 kHz"},
	{"25",		0x08,		"25 kHz"},
	{"100",		0x09,		"100 kHz"},
	{"\0",		R_NOP,		"invalid argument"}
};

/*
 * bank, restore and save subcommands
 */
struct cmdtable loadtab[] = {
	{"atten",	C_ATTEN,	"set attenuator"},
	{"default",	C_DEFLT,	"set default arguments"},
	{"dial",	C_DIAL,		"set tuning step"},
	{"dtcs",	C_DTCS,		"set digital tone squelch code"},
	{"name",	C_NAME,		"set channel name"},
	{"set",		C_SWTCH,	"set commands"},
	{"split",	C_SPLIT,	"split/duplex commands"},
	{"tone",	C_TONE,		"set repeater tone frequency"},
	{"tsql",	C_TSQL,		"set tone squelch frequency"},
	{"\0",		C_FREQX,	"set VFO frequency/mode"}
};

/*
 * Radio identifier decode
 */
struct cmdtable identab[] = {
	{"1271",	0x24,		"1271 UHF Transceiver"},
	{"1275",	0x18,		"1275 UHF Transceiver"},
	{"271",		0x20,		"271 VHF Transceiver"},
	{"275",		0x10,		"275 VHF Transceiver"},
	{"375",		0x12,		"375 VHF Transceiver"},
	{"471",		0x22,		"471 UHF Transceiver"},
	{"475",		0x14,		"475 UHF Transceiver"},
	{"575",		0x16,		"575 VHF Transceiver"},
	{"575",		0x28,		"725 HF Transceiver"},
	{"706",		0x4e,		"706 HF Transceiver"},
	{"706G",	0x58,		"706MKIIG HF Transceiver"},
	{"726",		0x30,		"726 HF Transceiver"},
	{"735",		0x04,		"735 HF Transceiver"},
	{"746",		0x66,		"746 HF Transceiver"},
	{"751",		0x1c,		"751 HF Transceiver"},
	{"756",		0x64,		"756 HF Transceiver"},
	{"761",		0x1e,		"761 HF Transceiver"},
	{"765",		0x2c,		"765 HF Transceiver"},
	{"775",		0x46,		"775 HF Transceiver"},
	{"781",		0x26,		"781 HF Transceiver"},
	{"970",		0x2e,		"970 HF Transceiver"},
	{"7000",	0x70,		"7000 HF/VHF/UHF Transceiver"},
	{"7800",	0x6a,		"7800 HF/VHF/UHF Transceiver"},
	{"R7000",	0x08,		"R7000 VHF/UHF Receiver"},
	{"R71",		0x1A,		"R71 HF Receiver"},
	{"R7100",	0x34,		"R7100 VHF/UHF Receiver"},
	{"R72",		0x32,		"R72 HF Receiver"},
	{"R75",		0x5a,		"R75 HF Receiver"},
	{"R8500",	0x4a,		"R8500 HF/VHF/UHF Receiver"},
	{"R9000",	0x2a,		"R9000 VHF/UHF Receiver"},
	{"probe",	0x00,		"Probe all radios"},
	{"\0",		R_NOP,		"unknown radio"}
};

/*
 * Broadband HF/VHF/UHF receiver mode decode (R7000)
 *
 * Note that panel FM mode is really wideband FM and panel FMn mode is
 * really FM mode on other radios. Emptying a channel changes the mode
 * to 0xff, but does not change the frequency. Reactivate the channel
 * by loading valid mode.
 */
static struct cmdtable mode2[] = {
	{"AM",		0x0002,		"AM"},
	{"a",		0x0002,		"AM (keypad)"},
	{"FMn",		0x8205,		"FM"},
	{"WFM",		0x0005,		"WFM"},
	{"f",		0x8205,		"FM (keypad)"},
	{"SSB",		0x8005,		"SSB"},
	{"s",		0x8005,		"SSB (keypad)"},
	{"\0",		R_NOP,		"invalid command"}
};

/*
 * Broadband MF/HF/VHF/UHF receiver mode decode (R8500/R9000)
 */
static struct cmdtable mode3[] = {
	{"AM",		0x0202,		"AM"},
	{"a",		0x0202,		"AM"},
	{"AMn",		0x0302,		"AM narrow"},
	{"AMw",		0x0102,		"AM wide"},
	{"CW",		0x0103,		"CW"},
	{"k",		0x0103,		"CW (keypad)"},
	{"CWn",		0x0203,		"CW narrow"},
	{"FM",		0x0105,		"FM"},
	{"f",		0x0105,		"FM (keypad)"},
	{"FMn",		0x0205,		"FM narrow"},
	{"LSB",		0x0100,		"LSB"},
	{"l",		0x0100,		"LSB (keypad)"},
	{"USB",		0x0101,		"USB"},
	{"u",		0x0101,		"USB (keypad)"},
	{"WFM",		0x0106,		"WFM"},
	{"\0",		R_NOP,		"invalid command"}
};

/*
 * Narrowband MF/HF/VHF/UHF transceiver and receiver mode decode
 *
 * The 7000 can do all of these, the 756 all but WFM, the 706g most
 * except the -R (reverse) modes.
 */
static struct cmdtable mode1[] = {
	{"AM",		0x0202,		"AM"},
	{"a",		0x0202,		"AM (keypad)"},
	{"AMn",		0x0302,		"AM narrow"},
	{"AMw",		0x0102,		"AM wide"},
	{"CW",		0x0203,		"CW"},
	{"k",		0x0203,		"CW (keypad)"},
	{"CW-R",	0x0207,		"CW reverse"},
	{"CW-Rn",	0x0307,		"CW reverse narrow"},
	{"CW-Rw",	0x0107,		"CW reverse wide"},
	{"CWn",		0x0303,		"CW narrow"},
	{"CWw",		0x0103,		"CW wide"},
	{"FM",		0x0205,		"FM"},
	{"f",		0x0205,		"FM (keypad)"},
	{"FMn",		0x0305,		"FM narrow"},
	{"FMw",		0x0105,		"FM wide"},
	{"LSB",		0x0200,		"LSB"},
	{"l",		0x0200,		"LSB (keypad)"},
	{"LSBn",	0x0300,		"LSB narrow"},
	{"LSBw",	0x0100,		"LSB wide"},
	{"RTTY",	0x0204,		"RTTY"},
	{"RTTY-R",	0x0208,		"RTTY reverse"},
	{"RTTY-Rn",	0x0308,		"RTTY reverse narrow"},
	{"RTTY-Rw",	0x0108,		"RTTY reverse wide"},
	{"RTTYn",	0x0304,		"RTTY narrow"},
	{"RTTYw",	0x0104,		"RTTY wide"},
	{"USB",		0x0201,		"USB"},
	{"u",		0x0201,		"USB (keypad)"},
	{"USBn",	0x0301,		"USB narrow"},
	{"USBw",	0x0101,		"USB wide"},
	{"WFM",		0x0106,		"wideband FM"},
	{"\0",		R_NOP,		"invalid command"}
};

/*
 * IC 7000 MF/HF/VHF/UHF transceiver and receiver mode decode
 */
static struct cmdtable mode4[] = {
	{"AM",		0x0202,		"AM 6.0"},
	{"a",		0x0202,		"AM 6.0 (keypad)"},
	{"AMn",		0x0302,		"AM 3.0"},
	{"AMw",		0x0102,		"AM 9.0"},
	{"CW",		0x0203,		"CW 500"},
	{"k",		0x0203,		"CW 500 (keypad)"},
	{"CW-R",	0x0207,		"CW-R 500"},
	{"CW-Rn",	0x0307,		"CW-R 250"},
	{"CW-Rw",	0x0107,		"CW-R 1.2"},
	{"CWn",		0x0303,		"CW 250"},
	{"CWw",		0x0103,		"CW 1.2"},
	{"FM",		0x0205,		"FM 10.0"},
	{"f",		0x0205,		"FM 10.0 (keypad)"},
	{"FMn",		0x0305,		"FM 7.0"},
	{"FMw",		0x0105,		"FM 15.0"},
	{"LSB",		0x0200,		"LSB 2.4"},
	{"l",		0x0200,		"LSB 2.4 (keypad)"},
	{"LSBn",	0x0100,		"LSB 1.8"},
	{"LSBw",	0x0300,		"LSB 3.0"},
	{"RTTY",	0x0204,		"RTTY 500"},
	{"RTTY-R",	0x0208,		"RTTY-R 500"},
	{"RTTY-Rn",	0x0308,		"RTTY-R 250"},
	{"RTTY-Rw",	0x0108,		"RTTY-R 2.4"},
	{"RTTYn",	0x0304,		"RTTY 250"},
	{"RTTYw",	0x0104,		"RTTY 2.4"},
	{"USB",		0x0201,		"USB 2.4"},
	{"u",		0x0201,		"USB 2.4 (keypad)"},
	{"USBn",	0x0301,		"USB 1.8"},
	{"USBw",	0x0101,		"USB 3.0"},
	{"WFM",		0x0106,		"WFM 280.0"},
	{"\0",		R_NOP,		"invalid command"}
};

/*
 * CI-V bit rate control
 */
struct cmdtable baud[] = {
	{"300",		B300,		"300 b/s"},
	{"600",		B600,		"600 b/s"},
	{"1200",	B1200,		"1200 b/s"},
	{"2400",	B2400,		"2400 b/s"},
	{"4800",	B4800,		"4800 b/s"},
	{"9600",	B9600,		"9600 b/s"},
	{"19200",	B19200,		"19200 b/s"},
	{"38400",	B38400,		"38400 b/s"},
	{"\0",		R_NOP,		"invalid argument"}

};

/*
 * Radio control initialization
 *
 *	model	  ident ch  bk   init   mode   step flag ptr
 */
struct namestruct name[] = {
	{"1271",  0x24,	B1200, 32,  0,  dummy, mode1, dummy, 0, NULL},
	{"1275",  0x18,	B1200, 32,  0,  dummy, mode1, dummy, 0, NULL},
	{"271",	  0x20,	B1200, 32,  0,  dummy, mode1, dummy, 0, NULL},
	{"275",	  0x10,	B1200, 99,  0,  dummy, mode1, dummy, 0, NULL},
	{"375",	  0x12,	B1200, 99,  0,  dummy, mode1, dummy, 0, NULL},
	{"471",	  0x22,	B1200, 32,  0,  dummy, mode1, dummy, 0, NULL},
	{"475",	  0x14,	B1200, 99,  0,  dummy, mode1, dummy, 0, NULL},
	{"575",	  0x16,	B1200, 99,  0,  dummy, mode1, dummy, 0, NULL},
	{"706",   0x4e, B9600, 10,  0,  dummy, mode1, dummy, 0, NULL},
	{"706G",  0x58, B19200,99,  0,  dummy, mode1, dialc, 0, NULL},
 	{"725",	  0x28,	B1200, 26,  0,  dummy, mode1, dummy, 0, NULL},
	{"726",	  0x30,	B1200, 26,  0,  dummy, mode1, dummy, 0, NULL},
	{"735",	  0x04,	B1200, 12,  0,  dummy, mode1, dummy, F_735, NULL},
	{"746",   0x66, B1200, 99,  0,  dummy, mode1, dummy, 0, NULL},
	{"751",	  0x1c,	B1200, 32,  0,  dummy, mode1, dummy, 0, NULL},
	{"756",   0x64, B19200,99,  0,  ctlc,  mode1, dialc, F_756, NULL},
	{"761",	  0x1e,	B1200, 32,  0,  dummy, mode1, dummy, 0, NULL},
	{"765",	  0x2c,	B1200, 32,  0,  dummy, mode1, dummy, 0, NULL},
	{"775",	  0x46,	B1200, 99,  0,  dummy, mode1, dialb, 0, NULL},
	{"781",	  0x26,	B1200, 99,  0,  dummy, mode1, dummy, 0, NULL},
	{"970",	  0x2e,	B1200, 99,  0,  dummy, mode1, dummy, 0, NULL},
	{"7000",  0x70, B19200,99,  5,  ctlc,  mode4, dialc, F_7000, NULL},
	{"7800",  0x6a, B1200, 99,  5,  ctlc,  mode4, dialc, F_7000, NULL},
	{"R7000", 0x08,	B1200, 99,  0,  dummy, mode2, dummy, 0, NULL},
	{"R71",	  0x1A,	B1200, 32,  0,  dummy, mode1, dummy, 0, NULL},
	{"R7100", 0x34,	B1200, 99,  0,  dummy, mode2, dummy, 0, NULL},
	{"R72",	  0x32,	B1200, 99,  0,  dummy, mode1, dummy, 0, NULL},
	{"R75",   0x5a, B19200,99,  0,  dummy, mode1, dummy, 0, NULL},
	{"R8500", 0x4a,	B19200,39,  19, ctlc,  mode3, diala, F_8500, NULL},
	{"R9000", 0x2a,	B1200, 99,  0,  dummy, mode3, diala, F_8500, NULL},
	{"\0",	  0x0,	0,     0,   0,  0,     0,     NULL, 0, NULL}
};

double logtab[] = {		/* tuning rate table */
	1., 2., 5.,		/* 0.000000 */
	10., 25., 50.,		/* 0.00000 */
	1e2, 2.5e2, 5e2,	/* 0.0000 */
	1e3, 2.5e3, 5e3,	/* 0.000 */
	1e4, 2.5e4, 5e4,	/* 0.00 */
	1e5, 2.5e5, 5e5,	/* 0.0 */
	1e6, 2.5e6, 5e6		/* 0. */
};
int sigtab[] = {		/* significant digits table */
	6, 6, 6,		/* 0.000000 */
	5, 6, 5,		/* 0.00000 */
	4, 5, 4,		/* 0.0000 */
	3, 4, 3,		/* 0.000 */
	2, 3, 2,		/* 0.00 */
	1, 2, 1,		/* 0.0 */
	0, 0, 0			/* 0. */
};

struct metertab mtab[] = {
	{0,	"S0"},		/* 0.4-1.3 uv */
	{12,	"S1"},
	{32,	"S2"},
	{46,	"S3"},		/* 0.79-2.0 uV */
	{64,	"S4"},
	{78,	"S5"},		/* 3.2 uV */
	{94,	"S6"}, 
	{110,	"S7"},		/* 13 uV */
	{126,	"S8"},
	{140,	"S9"},		/* 50 uV */
	{156,	"S9+10"},
	{174,	"S9+20"},	/* 0.5 mv */
	{190,	"S9+30"},
	{208,	"S9+40"},	/* 5 mv */
	{230,	"S9+50"},
	{999,	"S9+60"}	/* 50 mV */
};
