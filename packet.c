/*
 * Program to control ICOM radios
 *
 * Input/output interface
 */
#include "icom.h"

#ifndef MSDOS
#include <sys/fcntl.h>
#include <sys/termios.h>
#include <sys/unistd.h>
#else /* MSDOS */
#include <string.h>
#endif /* MSDOS */

/*
 * Parameters
 */
#ifdef MSDOS
#define XFRETRY	3000		/* interface timeout counter (MSDOS) */

/*
 * Define port and speed
 */
#define PORT	0x03f8		/* port address (COM1) */
/* #define PORT 0x02f8		/* port address (COM2) */
/* #define PORT 0x03e8		/* port address (COM3) */
/* #define PORT 0x02e8		/* port address (COM4) */
/* #define BAUD 384		/* baud rate 300 */
#define BAUD	96		/* baud rate 1200 */
/* #define BAUD 12		/* baud rate 9600 */

/*
 * Serial port definitions (8250)
 */
#define THR	0		/* transmitter holding register */
#define RBR	0		/* receiver buffer register */
#define DLL	0		/* divisor latch LSB */
#define DLM	1		/* divisor latch MSB */
#define LCR	3		/* line control register */
#define LCR_8BITS 3		/* 8 bit words */
#define   LCR_DLAB 0x80		/* divisor latch access bit */
#define MCR	4		/* modem control register */
#define   MCR_DTR 1		/* data terminal ready */
#define   MCR_RTS 2		/* request to send */
#define LSR	5		/* line status register */
#define   LSR_DR 0x01		/* data ready */
#define   LSR_THRE 0x20		/* trans line holding register empty */
#define   LSR_TSRE 0x40		/* transmitter shift register empty */
#endif /* MSDOS */

/*
 * fsa definitions
 */
#define S_IDLE	0		/* idle */
#define S_HDR	1		/* header */
#define S_TX	2		/* address */
#define S_DATA	3		/* data */
#define S_ERROR	4		/* error */

/*
 * Global variables
 */
int retry;			/* max command retries */

/*
 * Local function prototypes
 */
static u_char sndoctet(u_char);	/* send octet */
static u_char rcvoctet();	/* receive octet */

/*
 * Local variables
 */
static int count;		/* retry counter */
static int state;		/* fsa state */
#ifdef MSDOS
static int inp(int);		/* input port byte */
static void outp(int, int);	/* output port byte */
static void outpw(int, int);	/* output port word */
#endif /* MSDOS */

/*
 * Packet routines
 *
 * These routines send a packet and receive the response. Since the CI-V
 * bus is bidirectional, every octet sent by this program is read back
 * for check. If an error occurs on transmit (readback compare check),
 * the packet is resent. If an error occurs on receive (timeout), all
 * input to the terminating fe is discarded and the packet is resent. If
 * the maximum number of retries is not exceeded, the program returns
 * the number of octets in the user buffer; otherwise, it returns zero.
 *
 * ICOM frame format
 *
 * Frames begin with a two-octet preamble (fe-fe) followed by the radio
 * address RA, controller address (e0), command CN, subcommand SN
 * (optional), zero or more data octets DA and terminator fd. If a
 * subcommand is not used, the data field begins with the SN octet.
 *
 * The radio responds to a command frame with a response frame in the
 * same format, but with the radio and controller addresses
 * interchanged. If the response contains no data, the CN octet is fb
 * for success or fa for failure and the following octet fd ends the
 * frame. If the response format includes a data field, a frame with a
 * nonempty data field is a success, but an empty data field is a
 * failure.
 *
 *	+------+------+------+------+------+------+--//--+------+
 *	|  fe  |  fe  |  RA  |  e0  |  CN  |  SN  |  DA  |  fd  |
 *	+------+------+------+------+------+------+--//--+------+
 */
/*
 * initpkt() - initialize serial interface
 *
 * This routine opens the serial interface for raw transmission; that
 * is, character-at-a-time, no stripping, checking or monkeying with the
 * bits. For Unix, an input operation ends either with the receipt of a
 * character or a 0.5-s timeout.
 */
void
initpkt(
	int	baud		/* CI-V bit rate code */
	)			/* no return value */
{
#ifdef MSDOS
	outp(PORT+LCR, LCR_DLAB); /* set baud */
	outpw(PORT+DLL, BAUD);
	outp(PORT+LCR, LCR_8BITS); /* set 8 bits, no parity */
	outp(PORT+MCR, MCR_DTR+MCR_RTS); /* wake up modem */
#else /* MSDOS */
	struct termios ttyb, *ttyp;

	if (pflags & P_ERMSG)
		printf("*** baud %d\n", baud);
	ttyp = &ttyb;
	if (tcgetattr(fd_icom, ttyp) < 0) {
		printf("\n*** Unable to get line parameters\n");
		exit(1);
	}
	ttyp->c_iflag = 0;		/* input modes */
	ttyp->c_oflag = 0;		/* output modes */
	ttyp->c_cflag = CS8|CREAD|CLOCAL; /* control modes */
	ttyp->c_lflag = 0;		/* local modes */
	ttyp->c_cc[VMIN] = 0;		/* min chars */
	ttyp->c_cc[VTIME] = 5;		/* receive timeout */
	cfsetispeed(ttyp, baud);	/* bit rate */
	cfsetospeed(ttyp, baud);
	if (tcsetattr(fd_icom, TCSANOW, ttyp) < 0) {
		printf("\n*** Unable to set line parameters\n");
		exit(1);
	}
#endif /* MSDOS */
	retry = RETRY;
}

/*
 * sndpkt(r, x, y) - send packet and receive response
 *
 * This routine sends a command frame, which consists of all except the
 * preamble octets fe-fe. It then listens for the response frame and
 * returns the payload to the caller. The routine checks for correct
 * response header format; that is, the length of the response vector
 * returned to the caller must be at least two and the radio and
 * controller address octets must be interchanged; otherwise, the
 * operation is retried up to the number of times specified in a global
 * variable.
 *
 * The trace function, which is enabled by the P_TRACE bit of the global
 * pflags variable, prints all characters received or echoed on the bus
 * preceded by a T (transmit) or R (receive). The P_ERMSG bit of the
 * pflags variable enables printing of bus error messages.
 *
 * Note that the first octet sent is a pad (ff) in order to allow time
 * for the radio to flush its receive buffer after sending the previous
 * response. Even with this precaution, some of the older radios
 * occasionally fail to receive a command and it has to be sent again.
 */
int
sndpkt(				/* returns octet count */
	int r,			/* radio address */
	u_char *x,		/* command vector */
	u_char *y		/* response vector */
	)
{
	int i, j, k;		/* temps */
	u_char temp;

#ifndef MSDOS
	(void)tcflush(fd_icom, TCIOFLUSH);
#endif /* MSDOS */
	for (i = 0; i < retry; i++) {
		state = S_IDLE;

		/*
		 * Transmit packet.
		 */
		if (pflags & P_TRACE)
			printf("T:");
		sndoctet(PAD);		/* send header */
		sndoctet(PR);
		sndoctet(PR);
		sndoctet(r);
		sndoctet(TX);
		for (j = 0; j < BMAX; j++) { /* send body */
			if (sndoctet(x[j]) == FI)
				break;
		}
		while (rcvoctet() != FI); /* purge echos */
		if (x[0] == V_FREQT || x[0] == V_MODET)
			return (0);	/* shortcut for broadcast */

		/*
		 * Receive packet. First, delete all characters
		 * preceeding a PR, then discard all PRs. Check that the
		 * RE and TX fields are correctly interchanged, then
		 * copy the remaining data and FI to the user buffer.
		 */
		if (pflags & P_TRACE)
			printf("\nR:");
		j = 0;
		while ((temp = rcvoctet()) != FI) {
			switch (state) {

				case S_IDLE:
				if (temp != PR)
					continue;
				state = S_HDR;
				break;

				case S_HDR:
				if (temp == PR) {
					continue;
				} else if (temp != TX) {
					if (pflags & P_ERMSG)
						printf(
						    "\n*** TX error\n");
					state = S_ERROR;
				}
				state = S_TX;
				break;

				case S_TX:
				if (temp != r) {
					if (pflags & P_ERMSG)
						printf(
						    "\n*** RE error\n");
					state = S_ERROR;
				}
				state = S_DATA;
				break;

				case S_DATA:
				if (j >= BMAX ) {
					if (pflags & P_ERMSG)
						printf(
					    "\n*** buffer overrun\n");
					state = S_ERROR;
					j = 0;
				}
				y[j++] = temp;
				break;

				case S_ERROR:
				break;
			}
		}
		if (pflags & P_TRACE)
			printf("\n");
		if (j > 0) {
			y[j++] = FI;
			return (j);
		}
	}
	if (pflags & P_ERMSG)
		printf("*** retries exceeded\n");
	return (0);
}

/*
 * Interface routines
 *
 * These routines read and write octets on the bus. In case of receive
 * timeout a FI code is returned. In case of output collision (echo
 * does not match octet sent), the remainder of the collision frame
 * (including the trailing FI) is discarded.
 */

#ifdef MSDOS
/*
 * sndoctet(x) - send octet
 */
static u_char
sndoctet(			/* returns octet */
	u_char x;		/* octet */
	)
{
	u_char temp;

	inp(PORT+RBR);		/* flush spikes */
	outp(PORT+THR, x);	/* write octet */
	if (x == (temp = rcvoctet()))
		return (x);

	if (pflags & P_ERMSG)
		printf("\n*** bus error %02x %02x\n", x, temp);
	while (rcvoctet() != FI); /* discard junk */
	return (FI);
}

/*
 * Receive octet
 */
static u_char
rcvoctet()			/* returns octet, FI if error */
{
	int i;
	u_char temp;

	for (i = 0; i < XFRETRY; i++) /* wait for input */
	if ((inp(PORT + LSR)) & LSR_DR != 0) {
		if (i > count)
			count = i;
		temp = inp(PORT+RBR); /* read octet */
		return (temp);
	}
	return (FI);
}
#else /* MSDOS */

/*
 * sndoctet(x) - send octet
 */
static u_char
sndoctet(			/* returns octet */
	u_char x		/* octet */
	)
{
	write(fd_icom, &x, 1);
	return (x);
}

/*
 * rcvoctet () - receive octet
 */
static u_char
rcvoctet()			/* returns octet */
{
	u_char y;

	if (read(fd_icom, &y, 1) < 1)
		y = FI;		/* come here if timeout */
	if (pflags & P_TRACE && y != PAD)
		printf(" %02x", y);
	return (y);
}
#endif /* MSDOS */

/* end program */
