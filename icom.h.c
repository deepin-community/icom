/***********************************************************************
 *                                                                     *
 * Copyright (c) David L. Mills 1994-2003                              *
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
#define BMAX	80		/* max packet buffer size */
#define NAMMAX	30		/* max capability name string size */
#define VALMAX	80		/* max capability value string size */
#define CMDMAX	30		/* max radio command string size */
#define BNKMAX	100		/* max channel bank members */
#define MODES	6		/* number of modes in ICOM radios */
#define RETRY	3		/* max packet retries */
#define COMPMAX 100e-6		/* max frequency compensation */
#define ESC	0x1b		/* ANSI escape character */
#define KILL	0x7f		/* line delete character */
#define R_OK	0		/* return success */
#define R_ERR	-1		/* return error */

#define AUDIO			/* define for Sun audio device */

/*
 * Program flags (pflags)
 */
#define P_VERB	0x0001		/* verbose switch */
#define P_RADIO	0x0002		/* display radio */
#define P_DSPCH	0x0004		/* display chan */
#define P_DISP	0x0008		/* display freq, mode, split */
#define P_KEYP	0x0010		/* keypad command non-null */
#define P_EXIT	0x0020		/* exit after command line */
#define P_PAD	0x0040		/* keypad mode */
#define P_ESC	0x0080		/* escape sequence in progress */
#define P_TRACE	0x0100		/* trace packets */
#define P_ERMSG	0x0200		/* print bus error messages */

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
#define V_CLEAR	0x0b		/* clear channel */
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
#define V_CTRL	0x7f		/* miscellaneous control */

#define M_LSB	0x00		/* LSB mode number (for probe) */
#define M_USB	0x01		/* USB mode number (for probe) */
 
/*
 * Set vfo (V_SVFO) subcommands (775 b0, b1, c0, c1)
 */
#define S_VFOA	0x00		/* select vfo a */
#define S_VFOB	0x01		/* select vfo b */
#define S_BTOA	0xa0		/* vfo a <- vfo b */
#define S_XCHNG	0xb0		/* main <-> sub */
#define S_EQUAL	0xb1		/* main -> sub */
#define S_DWOFF	0xc0		/* dual watch off */
#define S_DWON	0xc1		/* dual watch on */
#define S_MBAND	0xd0		/* access main band */
#define S_SBAND	0xd1		/* access sub band */
 
/*
 * Scan control (V_SCAN) subcommands (775 00, 01)
 */
#define S_OFF	0x00		/* stop scan */
#define S_START	0x01		/* scan */
#define S_PSST	0x02		/* program scan */
#define S_DFST	0x03		/* delta-f scan */
#define S_AMST	0x04		/* auto write scan */
#define S_FPST	0x12		/* fine program scan */
#define S_FDST	0x13		/* fine delta-f scan */
#define S_MSST	0x22		/* memory scan */
#define S_SNST	0x23		/* memory scan number */
#define S_SMST	0x24		/* selected mode memory scan */
#define S_PXST	0x42		/* priority scan */
#define S_UNFIX	0xa0		/* unfix center frequency */
#define S_FIX	0xaa		/* fix center frequency */
#define S_DF2	0xa1		/* delta-f 2.5 kHz */
#define S_DF5	0xa2		/* delta-f 5 kHz */
#define S_DF10	0xa3		/* delta-f 10 kHz */
#define S_DF20	0xa4		/* delta-f 20 kHz */
#define S_DF50	0xa5		/* delta-f 50 kHz */
#define S_DSBM	0xb0		/* disable memory channel */
#define S_ENBM	0xb1		/* enable memory channel */
#define S_MEMN	0xb2		/* memory channel scan number */
#define S_VSOFF	0xc0		/* VSC off */
#define S_VSON	0xc1		/* VSC on */
#define S_SRNOT	0xd0		/* scan resume never */
#define S_SROFF	0xd1		/* scan resume off */
#define S_SRB	0xd2		/* scan resume b */
#define S_SRA	0xd3		/* scan resume a (delay) */
 
/*
 * Split control (V_SPLIT) subcommands
 */
#define S_OFF	0x00		/* split off */
#define S_ON	0x01		/* split on */
#define S_DUPOF	0x10		/* cancel duplex */
#define S_DUPM	0x11		/* select -duplex */
#define S_DUPP	0x12		/* select +duplex */

/*
 * Set attenuator (V_ATTN) subcommands
 */
#define S_AT00	0x00		/* off */
#define S_AT06	0x06		/* 6 dB */
#define S_AT10	0x10		/* 10 dB */
#define S_AT12	0x12		/* 12 dB */
#define S_AT18	0x18		/* 18 dB */
#define S_AT20	0x20		/* 20 dB */
#define S_AT30	0x30		/* 30 dB */

/*
 * Select antenna (V_SANT) subcommands
 */
#define S_ANT1	0x00		/* antenna 1 */
#define S_ANT2	0x01		/* antenna 2 */

/*
 * Announce control (V_ANNC) subcommands
 */
#define S_SAY0	0x00		/* announce off */
#define S_SAY1	0x01		/* announce freq */

/*
 * Read controls (V_RDCTL) subcommands
 */
#define S_RDAF	0x01		/* read AF gain */
#define S_RDSQ	0x01		/* read squelch */
#define S_RDSG	0x02		/* read S meter */
#define S_RDSH	0x04		/* read IF shift */
#define S_RDAP	0x05		/* read APF */

/*
 * Write controls (V_WRCTL) subcommands
 */
#define S_WRAF	0x01		/* set AF gain */
#define S_WRSQ	0x03		/* set squelch */
#define S_WRSH	0x04		/* set IF shift */
#define S_WRAP	0x05		/* set APF */

/*
 * Set switches (V_TOGL) subcommands
 */
#define S_PA0	0x8002		/* preamp off */
#define S_PA1	0x8102		/* preamp on */
#define S_AGC0	0x10		/* AGC slow (R8500) */
#define S_AGC1	0x11		/* AGC fast (R8500) */
#define S_AGC3	0x8112		/* AGC fast */
#define S_AGC2	0x8212		/* AGC slow */
#define S_NB0	0x20		/* NB off (R8500) */
#define S_NB1	0x21		/* NB on (R8500) */
#define S_NB2	0x8022		/* NB off */
#define S_NB3	0x8122		/* NB on */
#define S_APF0	0x30		/* APF off */
#define S_APF1	0x31		/* APF on */
#define S_TONE0	0x8042		/* tone off */
#define S_TONE1	0x8142		/* tone on */
#define S_TSQ0	0x8043		/* tone squelch off */
#define S_TSQ1	0x8143		/* tone squelch on */
#define S_COMP0	0x8044		/* speech compressor off*/
#define S_COMP1	0x8144		/* speech compressor on */
#define S_VOX0	0x8046		/* VOX off */
#define S_VOX1	0x8146		/* VOX on */
#define S_QSK0	0x8047		/* QSK off */
#define S_QSK1	0x8147		/* QSK on */

/*
 * Power control (V_POWER) subcommands
 */
#define S_OFF	0x00		/* power off */
#define S_ON	0x01		/* power on */

/*
 * Read/write bank/channel data (V_SETW) subcommands
 */
#define S_WCHN	0x00		/* write channel data */
#define S_RCHN	0x01		/* read channel data */
#define S_WBNK	0x02		/* write bank data */
#define S_RBNK	0x03		/* read bank data */

/*
 * Miscellaneous control (S_CTRL) subcommands
 */
#define S_LCL	0x01		/* select local control */
#define S_RMT	0x02		/* select remote control */
#define S_TPON	0x03		/* enable tape recorder */
#define S_TPOFF	0x04		/* disable tape recorder */
#define S_OPTO	0x05		/* read OPTO */
#define S_CTSS	0x06		/* read CTSS */
#define S_DCS	0x07		/* read DCS */
#define S_DTMF	0x08		/* read DTMF */
#define S_RDID	0x09		/* read ID */
#define S_SPON	0x0a		/* enable speaker audio */
#define S_SPOFF	0x0b		/* disable speaker audio */
#define S_5ON	0x0c		/* enable 5 kHz search window */
#define S_5OFF	0x0d		/* disable 5 kHz search window */
#define S_NXFM	0x0e		/* next frequency */
#define S_SMON	0x0f		/* enable search */
#define S_SMOFF	0x10		/* disable search */

/*
 * Load subcommands
 */
#define D_NAME		0	/* set name */
#define D_MODE		1	/* set mode */
#define D_DIAL		2	/* set dial tuning step */
#define D_ATTEN		3	/* set attenuator */
#define D_DUPLEX	4	/* set transmit offset */
#define	D_STEP		5	/* set tuning step */

/*
 * Command decode
 */
#define C_ANT		1	/* select antenna */
#define C_READ		2	/* read channel */
#define C_CLEAR		3	/* clear channel */
#define C_DEBUG		4	/* trace CI-V messages */
#define C_DOWN		5	/* step down */
#define C_DUPLEX	6	/* set transmit duplex */
#define C_ANNC		7	/* announce control */
#define C_ERASE		8	/* erase input */
#define C_FREQ		9	/* set frequency */
#define C_KEY		10	/* send CW message */
#define C_KEYBD		11	/* switch to keyboard mode */
#define C_KEYPAD	12	/* switch to keypad mode */
#define C_MODE		13	/* set mode/BFO compensation */
#define C_OFFSET	14	/* set VFO offset */
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
#define C_ATTEN		25	/* set attenuator */
#define C_DIAL		26	/* set dial tuning step */
#define C_MISC		27	/* miscellaneous control */
#define C_METER		28	/* read S meter */
#define C_PROBE		29	/* probe configuration */
#define C_MODEG		30	/* set mode */
#define C_VCOMP		31	/* set vfo compensation */
#define C_FREQX		32	/* set vfo frequency */
#define C_RATE		33	/* set tuning rate */
#define C_RUP		34	/* rate up */
#define C_RDOWN		35	/* rate down */
#define C_VERB		36	/* set verbose */
#define C_SMPLX		37	/* receive on transmit frequency */
#define C_BAND		38	/* set band limits */
#define C_BANK		39	/* read/write bank and name */
#define C_LOAD		40	/* read/write channel and name */
#define C_POWER		41	/* power on/off */
#define C_VOLUME	42	/* volume control */
#define C_SQUELCH	43	/* squelch control */
#define C_SHIFT		44	/* shift control */
#define C_APFC		45	/* APF control */
#define C_AGC		46	/* AGC fast/slow */
#define C_NB		47	/* NB on/off */
#define C_APF		48	/* APF on/off */
#define C_CHANGE	49	/* swap main and sub VFOs */
#define C_PAMP		50	/* preamp off/on */
#define C_TONE		51	/* repeater tone off/on */
#define C_TSQL		52	/* tone squelch off/on */
#define C_COMP		53	/* speech compressor off/on */
#define C_QSK		54	/* QSK breakin off/on */
#define C_VOX		55	/* VOX control off/on */
#define	C_CTRL		56	/* control commands */
#define	C_SWTCH		57	/* set/clear switches */
#ifdef AUDIO
#define C_GAIN		61	/* adjust output level */
#define C_MUTE		62	/* mute output (toggle) */
#define C_PORT		60	/* select input port */
#endif /* AUDIO */

/*
 * Radio control flags
 */
#define F_VFO		0x0100	/* radio has mem -> vfo */
#define F_OFFSET	0x0200	/* radio has duplex offset */
#define F_RELD		0x0400	/* reload after mode change */
#define F_735		0x0800	/* 4 octets frequency (IC-735) */
#define F_SPLIT		0x1000	/* radio has split mode */
#define F_SMPLX		0x2000	/* receive on transmit frequency */
#define F_DIAL		0x4000	/* radio has 1-kHz dial step table */
#define F_BANK		0x8000	/* radio has memory banks */
#define F_8500		0x10000	/* radio is a R8500 or R9000 */

/*
 * Capability structure
 */
struct cmdtable {
	char name[NAMMAX];	/* name */
	int ident;		/* capability key */
	char descr[VALMAX];	/* value */
};

/*
 * Radio command vector
 */
struct cmdvec {
	char name[NAMMAX];	/* command/mode name */
	u_char cmd[CMDMAX];	/* radio command */
	char descr[VALMAX];	/* description */
};

/*
 * Radio name decode structure
 */
struct namestruct {
	char name[NAMMAX];	/* radio name */
	int ident;		/* bus address */
	int maxch;		/* max channel number */
	int maxbk;		/* max bank number */
	struct cmdtable *ctrl;	/* control table */
	struct cmdtable *modetab; /* mode table */
	struct cmdtable *dialtab; /* dial tuning step table */
	int flags;		/* flag bits */
	struct icom *radio;	/* radio structure pointer */
};

/*
 * Channel data structure
 */
struct chan {
	int bank;		/* bank number */
	int mchan;		/* channel number */
	double freq;		/* frequency (MHz) */
	double duplex;		/* transmit offset (kHz) */
	int mode;		/* mode */
	int step;		/* tuning step code */
	double pstep;		/* programmed tuning step (kHz) */
	int atten;		/* attenuator code */
	int scan;		/* scan code */
	char name[9];		/* channel name */
};

/*
 * Radio control structure
 */
struct icom {
	char name[NAMMAX];	/* radio name */
	int ident;		/* bus address */
	int minch;		/* min channel */
	int maxch;		/* max channel */
	int topch;		/* top channel */
	int minbk;		/* min bank */
	int maxbk;		/* max bank */
	int topbk;		/* top bank */
	int flags;		/* flag bits */
	int rate;		/* VFO tuning rate */
	int minstep;		/* min tuning rate */
	struct chan chan;	/* memory channel */
	struct cmdtable *ctrl;	/* control table */

	struct cmdtable *modetab; /* mode decode table */
	struct cmdtable *dialtab; /* dial tuning step table */
	struct cmdtable cap[BNKMAX]; /* capability vector */

	double uband;		/* upper radio band edge (MHz) */
	double lband;		/* lower radio band edge (MHz) */
	double ustep;		/* upper step band edge (MHz) */
	double lstep;		/* lower step band edge (MHz) */
	double sub;		/* sub vfo frequency (MHz) */
	double oldplex;		/* simplex offset (kHz) */
	double offset;		/* frequency offset (kHz) */
	double pstep;		/* dial tuning step */
	double step;		/* tuning step (Hz) */
	double bfo[MODES];	/* BFO calibration offsets (Hz) */
	double freq_comp;	/* VFO calibration offset (PPM) */
};

/*
 * Command/response message format
 */
struct cmd1msg {
	u_char cmd;		/* command/response */
	u_char fd;		/* end delimiter (0xfd) */
};

/*
 * Command-2 message format
 */
struct cmd2msg {
	u_char cmd;		/* command */
	u_char subcmd;		/* subcommand */
	u_char fd;		/* end delimiter (0xfd) */
};

/*
 * Read band reply message format
 */
struct readbandmsg {
	u_char cmd;		/* command */
	u_char lband[5];	/* lower band edge */
	u_char pad;		/* unknown function */
	u_char uband[5];	/* upper band edge */
	u_char fd;		/* end delimiter (0xfd) */
};

/*
 * Read/write frequency message format
 */
struct freqmsg {
	u_char cmd;		/* command (V_RFREQ/V_SFREQ) */
	u_char freq[5];		/* frequency */
	u_char fd;		/* end delimiter (0xfd) */
};

/*
 * Read/write mode message format
 */
struct modemsg {
	u_char cmd;		/* command (V_RMODE/V_SMODE) */
	u_char mode[2];		/* mode */
	u_char fd;		/* end delimiter (0xfd) */
};

/*
 * Set dial tuning step message format
 */
struct dialmsg {
	u_char cmd;		/* command (V_DIAL) */
	u_char step;		/* dial tuning step code */
	u_char pstep[2];	/* dial programmed tuning step */
	u_char fd;		/* end delimiter (0xfd) */
};

/*
 * Read/write offset message format
 */
struct offsetmsg {
	u_char cmd;		/* command (V_ROFFS, V_SOFFS) */
	u_char offset[3];	/* offset */
	u_char fd;		/* end delimiter (0xfd) */
};

/*
 * Set bank message
 */
struct setbankmsg {
	u_char cmd;		/* command (V_SMEM) */
	u_char subcmd;		/* subcommand (0xa0) */
	u_char bank;		/* bank number */
	u_char fd;		/* end delimiter (0xfd) */
};

/*
 * Set channel message
 */
struct setchanmsg {
	u_char cmd;		/* command (V_SMEM) */
	u_char mchan[2];	/* channel number */
	u_char fd;		/* end delimiter (0xfd) */
};

/*
 * Read channel request message
 */
struct chanrqmsg {
	u_char cmd;		/* command (V_SETW) */
	u_char subcmd;		/* subcommand (S_RCHN) */
	u_char bank;		/* bank number */
	u_char mchan[2];	/* channel number */
	u_char fd;		/* end delimiter (0xfd) */
};

/*
 * Read model ID data message
 */
struct modelmsg {
	u_char cmd;		/* command (V_RDID) */
	u_char subcmd;		/* subcommand (S_RCHN) */
	u_char id;		/* model ID */
	u_char fd;		/* end delimiter (0xfd) */
};

/*
 * Read/write bank data message format
 */
struct bankmsg {
	u_char cmd;		/* command (V_SETW) */
	u_char subcmd;		/* subcommand (S_WBNK, S_RBNK) */
	u_char bank;		/* bank number */
	u_char name[5];		/* name */
	u_char fd;		/* end delimiter (0xfd) */
};

/*
 * Read/write channel data message format
 */
struct chanmsg {
	u_char cmd;		/* command (V_SETW) */
	u_char subcmd;		/* subcommand (S_WCHN, S_RCHN) */
	u_char bank;		/* bank number */
	u_char mchan[2];	/* channel number */
	u_char freq[5];		/* frequency */
	u_char mode[2];		/* mode */
	u_char step;		/* dial tuning step code */
	u_char pstep[2];	/* programmed dial tuning step */
	u_char atten;		/* attenuator */
	u_char scan;		/* scan */
	u_char name[8];		/* name */
	u_char fd;		/* end delimiter (0xfd) */
};

/*
 * Exported by icom.c
 */
extern int flags;
extern int pflags;
extern double logtab[];
extern char *modetoa(int, struct cmdtable *);
extern char *getcap(char *, struct cmdtable *);
extern void setcap(char *, struct cmdtable *, char *);

/*
 * Exported by radio.c
 */
extern int loadfreq(int, double);
extern int loadmode(int, int);
extern int loadoffset(int, double);
extern int loaddial(int, int, double);
extern int readfreq(int, double *);
extern int readchan(int, struct chan *);
extern int sendcw(int, char *);
extern int setchan(int, int, int);
extern int setbank(int, int);
extern int setcmd(int, int, int);
extern int setcmda(int, u_char *, u_char *);
extern int read_chan(int, struct chan *);
extern int write_chan(int, struct chan *);
extern int clear_chan(int, struct chan *);
extern int loadbank(int, int, char *);
extern int readbank(int, int, char *);
extern struct icom *select_radio(int);

/*
 * Exported by packet.c
 */
extern int retry;
extern void initpkt();
extern int sndpkt(int, u_char *, u_char *);

/*
 * Exported by tables.c
 */
extern struct namestruct name[];
extern struct cmdtable dbx[], cmd[], identab[], misc[], split[], agc[];
extern struct cmdtable agc1[], nb[], nb1[], atten[], ant[], annc[];
extern struct cmdtable meter[], power[], verbx[], key[], vfo[], scan[];
extern struct cmdtable split[], apf[], diala[], dialb[], dialc[];
extern struct cmdtable loadtab[], preamp[], tone[], comp[], vox[];
extern struct cmdtable qsk[], ctlc[], switches[];
extern struct cmdvec probe[];
