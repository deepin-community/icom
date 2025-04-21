/***********************************************************************
 *                                                                     *
 * Copyright (c) David L. Mills 1994-2006                              *
 *                                                                     *
 * Permission to use, copy, modify, and distribute this software and   *
 * its documentation for any purpose and without fee is hereby         *
 * granted, provided that the above copyright notice appears in all    *
 * copies that both the copyright notice and this permission           *
 * notice appear in supporting documentation, and that the name        *
 * University of Delaware not be used in advertising or publicity      *
 * pertaining to distribution of the software without specific,        *
 * written prior permission. The University of Delaware makes no       *
 * representations about the suitability this software for any         *
 * purpose. It is provided "as is" without express or implied          *
 * warranty.                                                           *
 *                                                                     *
 ***********************************************************************
 */
/*
 * Program to control ICOM radios
 *
 * Common header files
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

/*
 * Common definitions
 */
#define TRUE	1		/* veracibty */
#define FALSE	0		/* tenacity */
#define	LINMAX	100		/* max display line */
#define BMAX	100		/* max packet buffer size */
#define NAMMAX	40		/* max capability name string size */
#define VALMAX	80		/* max capability value string size */
#define	CAPMAX	10		/* max capabilities */
#define	FPMAX	20		/* max include file stack index */
#define MODES	9		/* number of modes in ICOM radios */
#define RETRY	3		/* max packet retries */
#define COMPMAX 100e-6		/* max frequency compensation */
#define ESC	0x1b		/* ANSI escape character (keypad) */
#define	KILL	0x7f		/* delete character (keypad) */

/*
 * Return codes
 */
#define	R_OK	0		/* return success */
#define	R_CMD	-1		/* unknown command */
#define	R_ARG	-2		/* unknown command argument */
#define	R_FMT	-3		/* invalid format */
#define	R_BAD	-4		/* radio can't do that */
#define	R_RAD	-5		/* unknown radio */
#define	R_IO	-7		/* file open error */
#define	R_ERR	-8		/* radio can't do that */
#define	R_NOP	-9		/* no op */
#define	R_NOR	-10		/* no radios found */
#define	R_DEF	-11		/* radio not defined */

/*
 * Program flags (pflags)
 */
#define P_VERB	0x0001		/* verbose switch */
#define P_DSPCH	0x0002		/* display chan */
#define P_DISP	0x0004		/* display freq, mode, split */
#define P_KEYP	0x0008		/* keypad command non-null */
#define P_EXIT	0x0010		/* exit after command line */
#define P_PAD	0x0020		/* keypad mode */
#define P_ESC	0x0040		/* escape sequence in progress */
#define P_TRACE	0x0080		/* trace packets */
#define P_ERMSG	0x0100		/* print bus error messages */
#define	P_DSPST	0x0200		/* display step/rate */

/*
 * CI-V frame codes
 */
#define PR	0xfe		/* preamble */
#define TX	0xe0		/* controller address */
#define FI	0xfd		/* end of message */
#define ACK	0xfb		/* controller normal reply */
#define NAK	0xfa		/* controller error reply */
#define PAD	0xff		/* transmit padding */

/*
 * CI-V controller commands
 */
#define V_FREQT	0x00		/* freq set (transceive) */
#define V_MODET	0x01		/* set mode (transceive) */
#define V_RBAND	0x02		/* read band edge */
#define V_RFREQ	0x03		/* read frequency */
#define V_RMODE	0x04		/* read mode */
#define V_SFREQ	0x05		/* set frequency */
#define V_SMODE	0x06		/* set mode */
#define V_SVFO	0x07		/* select vfo */
#define V_SMEM	0x08		/* select channel/bank */
#define V_WRITE	0x09		/* write channel */
#define V_VFOM	0x0a		/* memory -> vfo */
#define V_EMPTY	0x0b		/* empty channel */
#define V_ROFFS	0x0c		/* read tx offset */
#define V_SOFFS	0x0d		/* write tx offset */
#define V_SCAN	0x0e		/* scan control */
#define V_SPLIT	0x0f		/* split control */
#define V_DIAL	0x10		/* set dial tuning step */
#define V_ATTEN	0x11		/* set attenuator */
#define V_SANT	0x12		/* select antenna */
#define V_ANNC	0x13		/* announce control */
#define V_WRCTL	0x14		/* read/write controls */
#define V_RMTR	0x15		/* read meters */
#define V_TOGL	0x16		/* read/write switches */
#define V_ASCII	0x17		/* send CW message */
#define V_POWER	0x18		/* power control */
#define V_RDID	0x19		/* read model ID */
#define V_SETW	0x1a		/* read/write channel/bank data */
#define	V_TONE	0x1b		/* set repeater tone frequency */
#define	V_TX	0x1c		/* set transmit on/off */
#define V_CTRL	0x7f		/* miscellaneous control */

/*
 * Read/write bank/channel data (R8500) subcommands
 */
#define S_WCHN	0x00		/* write channel data */
#define S_RCHN	0x01		/* read channel data */
#define S_WBNK	0x02		/* write bank data */
#define S_RBNK	0x03		/* read bank data */

/*
 * Command decode
 */
#define C_ANT		1	/* select antenna */
#define C_READ		2	/* read channel */
#define C_EMPTY		3	/* empty channel */
#define C_DEBUG		4	/* trace CI-V messages */
#define C_DOWN		5	/* step down */
#define C_DUPLEX	6	/* set transmit duplex */
#define C_ANNC		7	/* announce control */
#define C_ERASE		8	/* erase input */
#define C_KEY		10	/* send CW message */
#define C_KEYBD		11	/* switch to keyboard mode */
#define C_KEYPAD	12	/* switch to keypad mode */
#define C_BCOMP		13	/* set BFO compensation */
#define C_VCOMP		14	/* set VFO compensation */
#define C_QUIT		15	/* exit program */
#define C_RADIO		16	/* select radio */
#define C_RESTORE	17	/* restore channels */
#define C_SAVE		18	/* save channels */
#define C_SPLIT		19	/* toggle split mode with offset */
#define C_STEP		20	/* set tuning step */
#define C_SCAN		21	/* scan control */
#define C_UP		22	/* step up */
#define C_VFO		23	/* vfo commands */
#define C_WRITE		24	/* write channel */
#define C_ATTEN		25	/* select attenuator */
#define C_DIAL		26	/* set dial tuning step */
#define C_MISC		27	/* miscellaneous control */
#define C_METER		28	/* read meters */
#define	C_TONE		29	/* set repeater tone frequency */
#define	C_RX		30	/* receive */
#define C_FREQ		31	/* set vfo frequency compensation */
#define C_FREQX		32	/* set vfo frequency/mode  */
#define C_RATE		33	/* set tuning rate */
#define C_RUP		34	/* rate up */
#define C_RDOWN		35	/* rate down */
#define	C_PTT		36	/* push to talk */
#define	C_MODE		37	/* set mode */
#define C_VERB		38	/* set verbose */
#define C_BAND		39	/* set band limits */
#define C_BANK		40	/* read/write bank and name */
#define C_POWER		41	/* power on/off */
#define	C_TX		42	/* transmit */
#define	C_TEST		43	/* send CI-V test message */
#define	C_DTCS		44	/* set digital tone squelch code */
#define	C_NAME		45	/* set channel name */
#define C_AGC		46	/* AGC fast/medium/slow */
#define	C_DUMP		47	/* dump VFO (debug) */
#define	C_SWAP		48	/* VFO A <-> VFO B */
#define	C_INCLD		49	/* include file */
#define C_PAMP		50	/* preamp off/on */
#define	C_DEFLT		51	/* set default string */
#define	C_TIME		52	/* set date and time */
#define C_BREAK		55	/* break-in off/semi/full */
#define	C_CTRL		56	/* control commands */
#define	C_SWTCH		57	/* set/clear switches */
#define	C_VFOM		58	/* memory -> vfo */
#define	C_TSQL		59	/* set tone squelch frequency */

/*
 * Read meter format codes
 */
#define A		0	/* agc fast/mid/slow */
#define	B		1	/* break in off/semi/full */
#define F		2	/* frequency -50/50 */
#define G		3	/* gain 0-100 */
#define P		4	/* preamp off/1/2 */
#define Q		5	/* squelch open/close */
#define S		6	/* S meter table */
#define	W		7	/* switch on/off */

/*
 * Radio control flags
 */
#define F_VFO		0x001	/* radio has mem -> vfo */
#define F_OFFSET	0x002	/* radio has duplex offset */
#define F_RELD		0x004	/* reload after mode change */
#define F_735		0x008	/* 4 octets frequency (IC-735) */
#define F_BANK		0x010	/* radio has memory banks */
#define F_8500		0x020	/* radio is a R8500 or equivalent */
#define	F_MODE		0x040	/* mode command has two octets */
#define	F_CACHE		0x080	/* frequency/mode cache valid */
#define	F_CHAN		0x100	/* radio has 0x1a command */
#define	F_756		0x200	/* radio is 7 56 or equivalent */
#define	F_7000		0x400	/* radio is a 7000 or equivalent */

/*
 * Capability structure
 */
struct cmdtable {
	char name[NAMMAX];	/* name */
	int ident;		/* capability key */
	char descr[VALMAX];	/* value */
};

/*
 * Radio name decode structure
 */
struct namestruct {
	char	name[NAMMAX];	/* radio name */
	int	ident;		/* CI-V bus address */
	int	baud;		/* CI-V bit rate */
	int	maxch;		/* max channel number */
	int	maxbk;		/* max bank number */
	struct cmdtable *ctrl;	/* control table */
	struct cmdtable *modetab; /* mode table */
	struct cmdtable *dialtab; /* dial tuning step table */
	int	flags;		/* flag bits */
	struct icom *radio;	/* radio structure pointer */
};

/*
 * 776 memory channel format
 */
struct vfo756 {
	u_char	freq[5];	/* frequency */
	u_char	mode[2];	/* mode */
	u_char	mode2;		/* duplex/tone select */
	u_char	tone[3];	/* repeater tone frequency */
	u_char	tsql[3];	/* tone squelch frequency */
};

/*
 * 7000 memory channel format
 */
struct vfo7000 {
	u_char	freq[5];	/* frequency */
	u_char	mode[2];	/* mode */
	u_char	mode2;		/* duplex/tone select */
	u_char	tone[3];	/* repeater tone frequency */
	u_char	tsql[3];	/* tone squelch frequency */
	u_char	dtcs[3];	/* digital tone squelch code */
};

/*
 * Aux controls structure
 */
struct aux {
	u_char	step;		/* tune step */
	u_char	pstep[2];	/* program tune step */
	u_char	atten;		/* attenuator */
	u_char	scan;		/* scan controls */
};

/*
 * Channel data structure
 */
struct chan {
	int	bank;		/* bank number */
	int	mchan;		/* channel number */
	double	freq;		/* frequency (MHz) */
	int	mode;		/* mode */
	double	split;		/* transmit split */
	int	step;		/* step code */
	double	pstep;		/* toggle step */
	struct aux aux;		/* aux controls */
	struct vfo7000 vfo;	/* VFO frequency/mode/tone */
	char	name[11];	/* channel name */
};

/*
 * Radio control structure
 */
struct icom {
	char	name[NAMMAX];	/* radio name */
	int	ident;		/* CI-V bus address */
	int	baud;		/* CI-V bit rate */
	int	minch;		/* min channel */
	int	maxch;		/* max channel */
	int	topch;		/* top channel */
	int	minbk;		/* min bank */
	int	maxbk;		/* max bank */
	int	topbk;		/* top bank */
	int	flags;		/* flag bits */
	int	rate;		/* VFO tuning rate */
	int	minstep;	/* min tuning rate */
	double	step;		/* tuning step */
	struct chan chan;	/* memory channel */
	struct cmdtable *ctrl;	/* control table */
	struct cmdtable *modetab; /* mode decode table */
	struct cmdtable *dialtab; /* dial tuning step table */
	struct cmdtable cap[CAPMAX]; /* miscellaneous capabilities */
	double	uband;		/* upper radio band edge (MHz) */
	double	lband;		/* lower radio band edge (MHz) */
	double	ustep;		/* upper step band edge (MHz) */
	double	lstep;		/* lower step band edge (MHz) */
	double	bfo[MODES];	/* BFO calibration offsets (Hz) */
	double	freq_comp;	/* VFO calibration offset (PPM) */
};

/*
 * Read band reply message format
 */
struct readbandmsg {
	u_char	cmd;		/* command */
	u_char	lband[5];	/* lower band edge */
	u_char	pad;		/* unknown function */
	u_char	uband[5];	/* upper band edge */
	u_char	fd;		/* end delimiter (0xfd) */
};

/*
 * Read/write bank data message format
 */
struct bankmsg {
	u_char	cmd;		/* command (V_SETW) */
	u_char	subcmd;		/* subcommand (S_WBNK, S_RBNK) */
	u_char	bank;		/* bank number */
	u_char	name[5];	/* name */
	u_char	fd;		/* end delimiter (0xfd) */
};

/*
 * R8500 read/write channel data message format
 */
struct chanmsg {
	u_char	cmd;		/* command (V_SETW) */
	u_char	subcmd;		/* subcommand (S_WCHN, S_RCHN) */
	u_char	bank;		/* bank number */
	u_char	mchan[2];	/* channel number */
	u_char	freq[5];	/* frequency */
	u_char	mode[2];	/* mode */
	struct aux aux;		/* aux controls */
	u_char	name[8];	/* name */
	u_char	fd;		/* end delimiter (0xfd) */
};

/*
 * 756 read/write channel data message format
 */
struct chan756 {
	u_char	cmd;		/* command (V_SETW) */
	u_char	subcmd;		/* subcommand (S_WCHN, S_RCHN) */
	u_char	mchan[2];	/* channel number */
	u_char	scan;		/* scan select */
	struct vfo756 vfo;	/* VFO frequency/mode/tone */
	u_char	name[10];	/* memory name */
	u_char	fd;		/* end delimiter (0xfd) */
};

/*
 * 7000 read/write channel data message format
 */
struct chan7000 {
	u_char	cmd;		/* command (V_SETW) */
	u_char	subcmd;		/* subcommand (S_WCHN, S_RCHN) */
	u_char	bank;		/* bank number */
	u_char	mchan[2];	/* channel number */
	u_char	scan;		/* scan select */
	struct	vfo7000 vfoa;	/* VFO A frequency/mode/tone */
	struct	vfo7000 vfob;	/* VFO B frequency/mode/tone */
	u_char	name[9];	/* channel name */
	u_char	fd;		/* end delimiter (0xfd) */
};

struct metertab {
	int smeter;		/* S meter reading */
	char pip[NAMMAX];	/* meter translation */
};

/*
 * Exported by icom.c
 */
extern int flags, pflags, fd_icom;
extern double logtab[];
extern char *modetoa(int, struct cmdtable *);
extern char *getcap(char *, struct cmdtable *);
extern void setcap(char *, struct cmdtable *, char *);

/*
 * Exported by radio.c
 */
extern int loadfreq(struct icom *, double);
extern int loadmode(int, int);
extern int loadoffset(int, double);
extern int readchan(struct icom *);
extern int writechan(struct icom *);
extern int sendcw(int, char *);
extern int setchan(int, int, int);
extern int setbank(int, int);
extern int setcmd(int, int, int);
extern int setcmda(int, u_char *, u_char *);
extern int write_chan(int, struct chan *);
extern int empty_chan(int, struct chan *);
extern int loadbank(int, int, char *);
extern int readbank(int, int, char *);
extern struct icom *select_radio(int, int);

/*
 * Exported by packet.c
 */
extern int retry;
extern void initpkt(int);
extern int sndpkt(int, u_char *, u_char *);

/*
 * Exported by tables.c
 */
extern struct namestruct name[];
extern struct cmdtable dbx[], cmd[], identab[], misc[], split[], agc[];
extern struct cmdtable agc1[], nb[], nb1[], atten[], ant[], say[];
extern struct cmdtable meter[], power[], verbx[], key[], vfo[], scan[];
extern struct cmdtable split[], apf[], diala[], dialb[], dialc[];
extern struct cmdtable loadtab[], preamp[], tone[], comp[], vox[];
extern struct cmdtable qsk[], ctlc[], switches[], toneon[];
extern struct cmdtable fmtb[], fmtw[], tx[], tone[], dtcs[], polar[];
extern struct cmdtable argch[], baud[];
extern double logtab[];
extern int sigtab[];
extern struct metertab mtab[];
