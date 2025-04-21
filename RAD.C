/*
 * Program to control ICOM radios
 * 
 * Subroutine library
 */
#include "icom.h"
#include <math.h>

/*
 * Local function prototypes
 */
void dtohex(int, u_char *);
void doublefreq(double, u_char *, int); /* double to frequency */
double freqdouble(u_char *, int); /* frequency to double */
static int read_r8500(int, struct chan *);
static int write_r8500(int, struct chan *);
static int read_756(int, struct chan *);
static int write_756(int, struct chan *);
static int read_7000(int, struct chan *);
static int write_7000(int, struct chan *);
int emptyvfo(struct chan *);


/*
 * select_radio(ident, baud) - select/initalize radio structure
 */
struct icom *
select_radio(			/* returns icom pointer or NULL */
	int ident,		/* radio ID */
	int baud		/* CI-V bit rate */
	)
{
	struct icom *rp;
	struct chan *cp;
	struct readbandmsg rspband;
	u_char	rspfreq1[BMAX];
	u_char	rspfreq2[BMAX];
	double	dtemp;
	char	s1[LINMAX];
	int	i, j, temp1, temp2;
	u_char	cmdband[] = {V_RBAND, FI};
	u_char	cmdbank[] = {V_SETW, S_RBNK, 0, 1, 1, FI};
	u_char  cmdrbnk[] = {V_SMEM, 0xa0, 0, FI};
	u_char	cmdsmem[] = {V_SMEM, 0, FI};
	u_char	cmdrfreq[] = {V_RFREQ, FI};
	u_char	cmdrmode[] = {V_RMODE, FI};
	u_char	cmdroffs[] = {V_ROFFS, FI};
	u_char	cmdvfom[] = {V_VFOM, FI};
	u_char	rsp[BMAX];
	u_char	rspfreq[BMAX];		/* saved frequency */
	u_char	rspmode[BMAX];		/* saved mode */

	/*
	 * Find prototype. Bad tables if not found.
	 */
	for (i = 0; name[i].ident != 0x0; i++) {
		if (name[i].ident == ident)
			break;
	}
	if (name[i].ident == 0x0)
		return (NULL);

	/*
	 * If radio is already enabled, just restore pointers and read
	 * the current channel.
	 */
	rp = name[i].radio;
	if (rp != NULL) {
		cp = &rp->chan;
		flags = rp->flags;
		initpkt(rp->baud);
		readchan(rp);
		return (rp);
	}

	/*
	 * Read band limits. Every radio and transceiver supports this
	 * command, so we use it to see if the radio is lit. If the
	 * radio set mode for CI-V baud is AUTO, the radio bit rate is
	 * set by the first command received and cannot be changed after
	 * that. Older radioS support 1200 bps by default, sometimes
	 * other rates by clipping diodes.
	 */
	if (baud != 0)
		temp1 = baud;
	else
		temp1 = name[i].baud;
	initpkt(temp1);
	retry = 1;
	if (setcmda(ident, cmdband, (u_char *)&rspband) == R_ERR) {
		retry = RETRY;
		return (NULL);
	}
	retry = RETRY;

	/*
	 * Allocate control block and initialize bits and pieces.
	 */
	rp = malloc(sizeof(struct icom));
	name[i].radio = rp;
	memset(rp, 0, sizeof(struct icom));
	strcpy(rp->name, name[i].name);
	rp->ident = ident;
	rp->baud = temp1;
	rp->maxch = name[i].maxch;
	rp->maxbk = name[i].maxbk;
	rp->ctrl = name[i].ctrl;
	rp->flags = name[i].flags;
	rp->modetab = name[i].modetab;
	rp->dialtab = name[i].dialtab;
	rp->lband = freqdouble(rspband.lband, 5) / 1e6;
	rp->uband = freqdouble(rspband.uband, 5) / 1e6;
	if (rp->lband > rp->uband) {
		dtemp = rp->lband;
		rp->lband = rp->uband;
		rp->uband = dtemp;
	}
	rp->lstep = rp->lband;
	rp->ustep = rp->uband;

	/*
	 * Save the frequency and mode. On the way, figure out whether
	 * the mode is one octet or two.
	 */
	setcmda(ident, cmdrfreq, rspfreq);
	temp1 = setcmda(ident, cmdrmode, rspmode);
	if (temp1 > 3)
		rp->flags |= F_MODE;

	/*
	 * If a read channel zero fails, the channel range starts at
	 * one.
	 */
	if (setcmda(ident, cmdsmem, rsp) == R_ERR)
		rp->minch = 1;

	/*
	 * If the radio supports multiple memory banks and a read bank
	 * zero fails, the bank range starts at one.
	 */
	if (rp->maxbk > 0) {
		rp->flags |= F_BANK;
		if (setcmda(ident, cmdrbnk, rsp) == R_ERR)
			rp->minbk = 1;
	}

	/*
	 * If the read offset command works, the radio supports duplex.
	 */
	if (setcmda(ident, cmdroffs, rsp) != R_ERR)
		rp->flags |= F_OFFSET;

	/*
	 * If the memory -> vfo command works, we have to use it in VFO
	 * mode every time the channel is changed.
	 */
	if (setcmda(ident, cmdvfom, rsp) != R_ERR)
		rp->flags |= F_VFO;

	/*
	 * Determine whether the frequency needs to be reset after a
	 * mode change. We first write the channel with mode USB, then
	 * read the frequency back and remember it. Then we write the
	 * channel with mode LSB, read back the frequency and see if it
	 * changed. If so, we have to write the frequency every time the
	 * mode is changed. What a drag.
	 */
	temp1 = capkey("lsb", rp->modetab);
	temp2 = capkey("usb", rp->modetab);
	if (temp1 != R_ERR && temp2 != R_ERR) {
		loadmode(ident, temp2);
		setcmda(ident, cmdrfreq, rspfreq1);
		loadmode(ident, temp1);
		setcmda(ident, cmdrfreq, rspfreq2);
		if (memcmp(&rspfreq1[1], &rspfreq2[1], 5) != 0)
			rp->flags |= F_RELD;
	}
	rp->chan.bank = rp->minbk;
	rp->chan.mchan = rp->minch;

	/*
	 * Determine the frequency resolution. This depends on the radio
	 * and TS button. We first write the frequency and mode near the
	 * lower band edge. The value is chosen with '1' in each
	 * significant digit ending in the units digit. Then, we read
	 * the frequency back and determine the resolution corresponding
	 * to the first nonzero significant digit.
	 */
	loadfreq(rp, rp->lband + 11111e-6);
	setcmda(ident, cmdrfreq, rspfreq1);
	rp->minstep = 6;
	if (rspfreq1[1] & 0x0f)
		rp->minstep = 0;
	else if (rspfreq1[1] & 0xf0)
		rp->minstep = 3;
	else if (rspfreq1[2] & 0x0f)
		rp->minstep = 6;
	else if (rspfreq1[2] & 0xf0)
		printf("*** tuning step error\n");
	rp->rate = rp->minstep;
	rp->step = logtab[rp->rate];

	/*
	 * Restore the original frequency and mode. This is silly, as
	 * the following code reads the base channel. However, some
	 * radios are picky and maybe the read channel fails. Do the
	 * frequency restore last, since some old radios don't
	 * automatically shift the LO.
	 */
	rspmode[0] = V_SMODE;
	setcmda(ident, rspmode, rsp);
	rspfreq[0] = V_SFREQ;
	setcmda(ident, rspfreq, rsp);

	/*
	 * Read the base channel. If error, give up now and return the
	 * marbles.
	 */
	flags = rp->flags;
	cp = &rp->chan;
	cp->bank = rp->minbk;
	cp->mchan = rp->minch;
	temp1 = readchan(rp);
	if (temp1 < 0) {
		free(rp);
		return(NULL);
	}
	return (rp);
}


/*
 * setchan(ident, bank, chan) - select bank and channel
 */
int
setchan(			/* returns 0 (ok) or < 0 (error) */
	int	ident,		/* radio ID */
	int	bank,		/* bank number */
	int	mchan		/* channel number */
	)
{
	char	s1[LINMAX];
	u_char	cmd[] = {V_SMEM, 0, 0, FI};
	u_char	rsp[BMAX];
	int	temp;

	/*
	 * Radios with memory banks require two commands, one to
	 * set the bank, another to set the channel. Bank numbers 0-99
	 * are encoded in one octet.
	 */
	if (flags & F_BANK) {
		sprintf(s1, "%02d", bank);
		cmd[1] = 0xa0;
		cmd[2] = ((s1[0] & 0xf) << 4) | (s1[1] & 0xf);
		temp = setcmda(ident, cmd, rsp);
		if (temp == R_ERR) {
			printf("*** invalid bank %d\n", bank);
			return (R_ERR);
		}
	}

	/*
	 * Channel numbers 0-99 are encoded in one octet. Channel
	 * numbers 100-9999 are encoded in two octets. 
	 */
	sprintf(s1, "%04d", mchan);
	if (mchan < 100) {
		cmd[1] = ((s1[2] & 0xf) << 4) | (s1[3] & 0xf);
		cmd[2] = FI;
	} else {
		cmd[1] = ((s1[0] & 0xf) << 4) | (s1[1] & 0xf);
		cmd[2] = ((s1[2] & 0xf) << 4) | (s1[3] & 0xf);
	}
	temp = setcmda(ident, cmd, rsp);
	if (temp == R_ERR) {
		printf("*** invalid channel %d\n", mchan);
		return (R_ERR);
	}
	return (R_OK);
}


/*
 * readchan(radio) - read channel frequency and mode
 */
int
readchan(			/* returns 0 (ok) or < 0 (error) */
	struct icom *rp		/* radio structure */
	)
{
	struct chan *cp;
	int rval;
	u_char	cmdvfom[] = {V_VFOM, FI};
	u_char	rsp[BMAX];

	/*
	 * For radios with a 0x1a command, read the model-specific
	 * data message and extract the frequency, mode and ancillary
	 * data.
	 */
	cp = &rp->chan;
	rval = R_OK;
	if (setchan(rp->ident, cp->bank, cp->mchan) == R_ERR)
		return (R_ERR);

	emptyvfo(cp);
	if (flags & F_7000)
		rval = read_7000(rp->ident, cp);

	if (flags & F_756)
		rval = read_756(rp->ident, cp);

	if (flags & F_8500)
		rval = read_r8500(rp->ident, cp);

	if (rval < 0)
		return (rval);

	/*
	 * For radios without a 0x1a command, read the frequency and
	 * mode directly later.
	 */
	if (flags & F_VFO)
		setcmda(rp->ident, cmdvfom, rsp);
	if (flags & (F_7000 | F_756 | F_8500))
		return (rval);

	return (readvfo(rp));
}


/*
 * writechan(radio) - write channel frequency and mode
 */
int
writechan(			/* returns 0 (ok) or < 0 (error) */
	struct icom *rp		/* radio structure */
	)
{
	struct chan *cp;
	u_char cmdwrite[] = {V_WRITE, FI};
	u_char	rsp[BMAX];
	int	temp;

	/*
	 * For radios with a 0x1a command, construct the model-specific
	 * data message and write the frequency, mode and ancillary
	 * data. First, read the current frequency and mode, which might
	 * have been changed recently without telling this program.
	 */
	cp = &rp->chan;
	readvfo(rp);
	if (flags & F_7000)
		return (write_7000(rp->ident, cp));

	if (flags & F_756)
		return (write_756(rp->ident, cp));

	if (flags & F_8500)
		return (write_r8500(rp->ident, cp));

	/*
	 * For older radios, write the frequency and mode directly.
	 */
	if (setchan(rp->ident, cp->bank, cp->mchan) == R_ERR)
		return (R_ERR);

	return (setcmda(rp->ident, cmdwrite, rsp));
}


/*
 * emptychan(ident, chan) - empty channel
 */
int
emptychan(			/* returns > 0 (ok) or < 0 (error) */
	int	ident,		/* radio ID */
	struct chan *cp		/* channel structure */
	)
{
	u_char	cmdempty[] = {V_EMPTY, FI};
	u_char	rsp[BMAX];

	/*
	 * Set bank and channel as necessary.
	 */
	if (setchan(ident, cp->bank, cp->mchan) == R_ERR)
		return (R_ERR);

	emptyvfo(cp);
	return (setcmda(ident, cmdempty, rsp));
}


/*
 * readvfo(radio) - read current frequency and mode
 */
int
readvfo(			/* returns > 0 (ok) or < 0 (error) */
	struct icom *rp		/* radio structure */
	)
{
	struct chan *cp;
	u_char	cmdfreq[] = {V_RFREQ, FI};
	u_char	cmdmode[] = {V_RMODE, FI};
	u_char	rsp[BMAX];
	double	dtemp;
	int	rval;

	/*
	 * If the cache is valid, don't diddle the radio.
	 */
	if (flags & F_CACHE)
		return(R_OK);

	/*
	 * Update VFO frequency and mode.
	 */
	cp = &rp->chan;
	rval = setcmda(rp->ident, cmdfreq, rsp);
	if (rval < 0)
		return (rval);

	if (rval == 6 && (flags & F_735))
		memcpy(&cp->vfo.freq[1], &rsp[1], 4);
	else if (rval == 7)
		memcpy(cp->vfo.freq, &rsp[1], 5);
	rval = setcmda(rp->ident, cmdmode, rsp);
	if (rval < 0)
		return (rval);

	memcpy(cp->vfo.mode, &rsp[1], rval - 2);

	/*
	 * Update channel frequency and mode.
	 */
	dtemp = freqdouble(cp->vfo.freq, 5);
	cp->freq = (dtemp * (1 - rp->freq_comp / 1e6) -
	    rp->bfo[cp->mode & 0x0f]) / 1e6;
	cp->mode = cp->vfo.mode[0];
	if (rval > 3)
		cp->mode |= cp->vfo.mode[1] << 8;
	flags |= F_CACHE;
	return (rval);
}


/*
 * emptyvfo - initialize VFO
 *
 * Initialize VFO with zero frequency and valid ancillary values. Once
 * the frequency and mode have been set, the VFO can be stored in a
 * memory channgel. Both the 756 and 7000 are quite particular about
 * vaild or default tone data before storing.
 */
int
emptyvfo(
	struct chan *cp
	)
{
	u_char	tone[] = {0x00, 0x08, 0x85};
	u_char	dtcs[] = {0x00, 0x00, 0x23};
	struct vfo7000 *vp;

	/*
	 * An ampth VFO has default repeater tone, tone squelch and
	 * digital tone squelch, and empty name.
	 */
	cp->freq = 0;
	cp->mode = 0;
	cp->split = 0;
	cp->step = 0;
	cp->pstep = 0;
	memset(&cp->aux, 0, sizeof(struct aux));
	memset(cp->name, 0, sizeof(cp->name));
	vp = &cp->vfo;
	memset(vp, 0, sizeof(struct vfo7000));
	memcpy(&vp->tone, tone, 3);
	memcpy(&vp->tsql, tone, 3);
	memcpy(&vp->dtcs, dtcs, 3);
	flags &= ~F_CACHE;
	return (R_OK);
}


/*
 * loadfreq(radio, freq) - write vfo frequency
 */
int
loadfreq(			/* returns > 0 (ok) or < 0 (error) */
	struct icom *rp,	/* radio structure */
	double	freq		/* frequency (MHz) */
	)
{
	struct chan *cp;
	u_char	cmd[] = {V_SFREQ, 0, 0, 0, 0, FI, FI};
	u_char	rsp[BMAX];
	int	temp;
	double	dtemp;

	cp = &rp->chan;
	if (flags & F_735)
		temp = 4;
	else
		temp = 5;
	dtemp = freq * 1e6 * (1 + rp->freq_comp / 1e6) +
	    rp->bfo[cp->mode & 0x0f];
	doublefreq(dtemp, &cmd[1], temp);
	return (setcmda(rp->ident, cmd, rsp));
}

/*
 * loadmode(ident, mode) - write vfo mode
 */
int
loadmode(			/* returns > 0 (ok) or < 0 (error) */
	int	ident,		/* radio ID */
	int	mode		/* mode */
	)
{
	u_char	frq[] = {V_RFREQ, FI}; 
	u_char	cmd[] = {V_SMODE, 0, FI, FI};
	u_char	rsp[BMAX], sfrq[BMAX];
	int	rval;

	cmd[1] = mode;
	if (flags & F_MODE)
		cmd[2] = mode >> 8;

	/*
	 * If VFO is not corrected, restore it after loading the mode.
	 */
	if (!(flags & F_RELD))
		return (setcmda(ident, cmd, rsp));

	setcmda(ident, frq, sfrq);
	rval = setcmda(ident, cmd, rsp);
	if (rval < 0)
		return (rval);

	sfrq[0] = V_SFREQ;
	return (setcmda(ident, sfrq, rsp));
}


/*
 * readoffset(ident, offset) read transmit duplex offset
 */
int
readoffset(			/* returns > 0 (ok) or < 0 (error) */
	int	ident,		/* radio ID */
	double	*freq		/* transmit offset (kHz) */
	)
{
	u_char	cmd[] = {V_ROFFS, FI};
	u_char	rsp[BMAX];
	double	dtemp;
	int	rval;

	rval = setcmda(ident, cmd, rsp);
	if (rval < 0)
		return (rval);

	dtemp = freqdouble(&rsp[1], 3);
	*freq = dtemp / 10.;
	return (rval);
}


/*
 * loadoffset(ident, offset) - write transmit duplex offset
 */
int
loadoffset(			/* returns > 0 (ok) or < 0 (error) */
	int	ident,		/* radio ID */
	double	freq		/* transmit offset (kHz) */
	)
{
	u_char cmd[] = {V_SOFFS, 0, 0, 0, FI};
	u_char	rsp[BMAX];

	doublefreq(fabs(freq) * 10., &cmd[1], 3);
	return (setcmda(ident, cmd, rsp));
}


/*
 * readbank(ident, bank, name) - read bank name (R8500)
 */
int
readbank(			/* returns > 0 (ok) or < 0 (error) */
	int ident,		/* radio ID */
	int bank,		/* bank number */
	char *name		/* bank name */
	)
{
	u_char cmd[] = {V_SETW, S_RBNK, 0, FI};
	struct bankmsg rsp;
	char	s1[LINMAX];
	int	rval;

	sprintf(s1, "%02d", bank);
	cmd[2] = ((s1[0] & 0xf) << 4) | (s1[1] & 0xf);
	rval = setcmda(ident, (u_char *)&cmd, (u_char *)&rsp);	
	if (rval < 0)
		return (rval);

	rsp.name[sizeof(rsp.name)] = '\0';
	sscanf(rsp.name, "%s", name);
	return (rval);
}


/*
 * loadbank(ident, bank, name) - write bank name (R8500)
 */
int
loadbank(			/* returns > 0 (ok) or < 0 (error) */
	int ident,		/* radio ID */
	int bank,		/* bank number */
	char *name		/* bank name */
	)
{
	struct bankmsg cmd = {V_SETW, S_WBNK};
	u_char	rsp[BMAX];
	char	s1[LINMAX];
	int	temp;

	sprintf(s1, "%02d", bank);
	cmd.bank = ((s1[0] & 0xf) << 4) | (s1[1] & 0xf);
	memset(cmd.name, ' ', sizeof(cmd.name));
	temp = strlen(name);
	if (temp > sizeof(cmd.name))
		temp = sizeof(cmd.name);
	memcpy(cmd.name, name, temp);
	cmd.fd = FI;
	return (setcmda(ident, (u_char *)&cmd, rsp));
}


/*
 * read_r8500(ident, chan) - read channel data (R8500)
 *
 * This radio is a receiver only. It has no VFO; data are stored only in
 * channels. In addition to the usual frequency and mode data, it can
 * store the step, programmed step and attenuator data in each channel.
 */
int
read_r8500(			/* returns > 0 (ok) or < 0 (error) */
	int	ident	,	/* radio ident */
	struct chan *cp		/* channel structure */
	)
{
	struct chanmsg rsp;
	u_char	cmd[] = {V_SETW, 0x01, 0x00, 0x00, 0x00, FI};
	char	s1[LINMAX];
	int	temp, i;

	/*
	 * Assemble bank/channel number and read.
	 */
	sprintf(s1, "%02d", cp->bank);
	cmd[2] = ((s1[0] & 0xf) << 4) | (s1[1] & 0xf);
	dtohex(cp->mchan, &cmd[3]);
	memset(&rsp, 0, sizeof(rsp));
	temp = setcmda(ident, cmd, (u_char *)&rsp);
	if (temp == R_ERR)
		return (R_ERR);

	/*
	 * Assemble VFO data
	 */
	if (temp < sizeof(struct chanmsg))
		return (R_OK);

	memcpy(cp->vfo.freq, rsp.freq, 5);
	memcpy(cp->vfo.mode, rsp.mode, 2);

	/*
	 * Copy step, pstep, attenuator and scan.
	 */
	cp->aux = rsp.aux;

	/*
	 * Copy channel name. Left justify and trim trailing space.
	 */
	memset(cp->name, 0, sizeof(cp->name));
	temp = sizeof(rsp.name);
	memcpy(cp->name, rsp.name, temp);
	for (i = temp - 1; i >= 0; i--) {
		if (cp->name[i] <= ' ')
			cp->name[i] = '\0';
		else
			break;
	}
	return (R_OK);
}


/*
 * write_r8500(ident, chan) - write channel data (R8500)
 */
int
write_r8500(			/* returns > 0 (ok) or < 0 (error) */
	int ident,		/* radio ID */
	struct chan *cp		/* channel structure */
	)
{
	struct chanmsg cmd = {V_SETW, 0x00};
	char	s1[LINMAX];
	u_char	rsp[BMAX];
	int	temp;
	double	dtemp;

	/*
	 * Insert band/chan address.
	 */
	sprintf(s1, "%02d", cp->bank);
	cmd.bank = ((s1[0] & 0xf) << 4) | (s1[1] & 0xf);
	dtohex(cp->mchan, cmd.mchan);

	/*
	 * Insert current frequency/mode.
	 */
	memcpy(&cmd.freq, &cp->vfo.freq, 5);
	memcpy(&cmd.mode, &cp->vfo.mode, 2);

	/*
	 * Insert step, program step, attenuator and scan. The radio
	 * insists on a nonero pstep, even if not used.
	 */
	cmd.aux = cp->aux;
	if (cmd.aux.pstep[0] == 0 && cmd.aux.pstep[1] == 0) {
		cmd.aux.pstep[0] = 0x10;
		cmd.aux.pstep[1] = 0x00;
	}

	/*
	 * Insert channel name.
	 */
	memset(cmd.name, ' ', sizeof(cmd.name));
	temp = strlen(cp->name);
	if (temp > strlen(cmd.name))
		temp = strlen(cmd.name);
	memcpy(cmd.name, cp->name, temp);
	cmd.fd = FI;
	return (setcmda(ident, (u_char *)&cmd, rsp));
}


/*
 * read_756(radio, chan) - read channel data (756 class)
 *
 * This radio can store the frequency, mode and tone codes for the
 * A VFO when the memory channel is written.
 */
int
read_756(			/* returns > 0 (ok) or < 0 (error) */
	int	ident,		/* radio ident */
	struct chan *cp		/* channel structure */
	)
{
	struct vfo7000 *wf;
	struct chan756 rsp;
	u_char	cmd[] = {V_SETW, 0x00, 0x00, 0x00, FI};
	char	s1[LINMAX];
	int	temp, i;

	/*
	 * Assemble channel number and read.
	 */
	dtohex(cp->mchan, &cmd[2]);
	memset(&rsp, 0, sizeof(rsp));
	temp = setcmda(ident, cmd, (u_char *)&rsp);
	if (temp == R_ERR)
		return(R_ERR);

	/*
	 * Copy VFO contents.
	 */
	if (temp < sizeof(struct chan756))
		return (R_OK);

	cp->aux.scan = rsp.scan;
	memcpy(&cp->vfo, &rsp.vfo, sizeof(struct vfo756));

	/*
	 * Copy channel name. Left justify and trim trailing space.
	 */
	memset(cp->name, 0, sizeof(cp->name));
	temp = sizeof(rsp.name);
	memcpy(cp->name, rsp.name, temp);
	for (i = temp - 1; i >= 0; i--) {
		if (cp->name[i] <= ' ')
			cp->name[i] = '\0';
		else
			break;
	}
	return (R_OK);
}


/*
 * Write_756(ident, chan) - write channel data (756 class)
 */
int
write_756(			/* returns > 0 (ok) or < 0 (error) */
	int ident,		/* radio ID */
	struct chan *cp		/* channel structure */
	)
{
	struct chan756 cmd = {V_SETW, 0x00};
	char	s1[LINMAX];
	u_char	rsp[BMAX];
	int	temp;
	double	dtemp;

	/*
	 * Insert channel number.
	 */
	dtohex(cp->mchan, cmd.mchan);

	/*
	 * Insert current frequency/mode.
	 */
	memcpy(&cmd.vfo, &cp->vfo, sizeof(struct vfo756));
 
	/*
	 * Insert channel name.
	 */
	memset(cmd.name, ' ', sizeof(cmd.name));
	temp = strlen(cp->name);
	if (temp > strlen(cmd.name))
		temp = strlen(cmd.name);
	memcpy(cmd.name, cp->name, temp);
	cmd.fd = FI;
	return (setcmda(ident, (u_char *)&cmd, rsp));
}


/*
 * read_7000(radio, channel) - read channel data (7000 class)
 *
 * This radio can store the frequency, mode and tone codes for both the
 * A and B VFOs when the memory channel is written. If operating in
 * split mode, the A VFO is used for receiving and the B VFO for
 * transmitting. This is handy for splits and odd duplex conditions.
 */
int
read_7000(			/* returns > 0 (ok) or < 0 (error) */
	int	ident,		/* radio ident*/
	struct chan *cp		/* channel structure */
	)
{
	struct chan7000 rsp;
	u_char	cmd[] = {V_SETW, 0x00, 0, 0, 0, FI};
	u_char	cmdsplit[]= {V_SPLIT, 0x00, FI};
	u_char	rsp1[BMAX];
	char	s1[LINMAX];
	int	temp, i, rval;

	/*
	 * Assemble bank/channel number and read.
	 */
	sprintf(s1, "%02d", cp->bank);
	cmd[2] = ((s1[0] & 0xf) << 4) | (s1[1] & 0xf);
	dtohex(cp->mchan, &cmd[3]);
	memset(&rsp, 0, sizeof(rsp));
	temp = setcmda(ident, cmd, (u_char *)&rsp);
	if (temp == R_ERR)
		return(R_ERR);

	/*
	 * Copy VFO A contents.
	 */
	if (temp < sizeof(struct chan7000))
		return (R_OK);

	cp->aux.scan = rsp.scan;
	memcpy(&cp->vfo, &rsp.vfoa, sizeof(struct vfo7000));

	/*
	 * If the A and B VFOs are different, turn on the split.
	 */
	if (memcmp(rsp.vfoa.freq, rsp.vfob.freq, 5) == 0) {
		cp->split = 0;
		cmdsplit[1] = 0x00;
	} else {
		cp->split = freqdouble(rsp.vfob.freq, 5) / 1e6;
		cmdsplit[1] = 0x01;
	}
	rval = setcmda(ident, cmdsplit, rsp1);
	if (rval < 0)
		return (rval);

	/*
	 * Copy channel name. Left justify and trim trailing space.
	 */
	memset(cp->name, 0, sizeof(cp->name));
	temp = sizeof(rsp.name);
	memcpy(cp->name, rsp.name, temp);
	for (i = temp - 1; i >= 0; i--) {
		if (cp->name[i] <= ' ')
			cp->name[i] = '\0';
		else
			break;
	}
	return (R_OK);
}


/*
 * Write_7000(ident, chan) - write channel data (7000 class)
 */
int
write_7000(			/* returns > 0 (ok) or < 0 (error) */
	int ident,		/* radio ID */
	struct chan *cp		/* channel structure */
	)
{
	struct chan7000 cmd = {V_SETW, 0x0};
	char	s1[LINMAX];
	u_char	rsp[BMAX];
	int	temp;
	double	dtemp;

	/*
	 * Insert band/chan address.
	 */
	sprintf(s1, "%02d", cp->bank);
	cmd.bank = ((s1[0] & 0xf) << 4) | (s1[1] & 0xf);
	dtohex(cp->mchan, cmd.mchan);

	/*
	 * Insert current frequency/mode in both the A and B VFOs. If in
	 * split mode update the split frequency in the B VFO.
	 */
	memcpy(&cmd.vfoa, &cp->vfo, sizeof(struct vfo7000));
	memcpy(&cmd.vfob, &cp->vfo, sizeof(struct vfo7000));
	if (cp->split != 0) {
		doublefreq(cp->split * 1e6, cmd.vfob.freq, 5);
		memcpy(&cmd.vfob.mode, cp->vfo.mode, 2);
	}
 
	/*
	 * Insert channel name.
	 */
	memset(cmd.name, ' ', sizeof(cmd.name));
	temp = strlen(cp->name);
	if (temp > strlen(cmd.name))
		temp = strlen(cmd.name);
	memcpy(cmd.name, cp->name, temp);
	cmd.fd = FI;
	return (setcmda(ident, (u_char *)&cmd, rsp));
}


/*
 * setcmda(ident, cmd, rsp) - send CI-V command and receive response.
 */
int
setcmda(			/* returns > 0 (ok) or < 0 (error) */
	int	ident,		/* radio ID */
	u_char	*cmd,		/* command */
	u_char	*rsp		/* response */
	)
{
	int	temp;

	temp = sndpkt(ident, cmd, rsp);
	if (temp < 2 || rsp[0] == NAK)
		return (R_ERR);

	return (temp);
}


/*
 * BCD conversion routines
 *
 * These routines convert between internal (floating point) format and
 * radio (BCD) format. BCD data are two digits per octet in big normal
 * or reversed order, depending on data type. These routines convert
 * between internal and reversed data types.
 *
 * 	data type	BCD	units	order
 * 	====================================================
 * 	bank		2	1	normal		
 * 	channel		2/4	1	normal (4 for R8500)
 * 	frequency	8/10	1/10 Hz	reversed (8 for 735)
 * 	transmit offset	6	100 Hz	reversed
 * 	dial tune step	4	100 Hz	reversed
 */
/*
 * dtohex(dec, hex) integer to 4 packed digits
 */
void
dtohex(
	int	arg,		/* decimal integer */
	u_char	*hex		/* packet hex string */
	)
{
	u_char	*ptr;
	char	s1[LINMAX];

	sprintf(s1, "%04d", arg);
	hex[0] = ((s1[0] & 0xf) << 4) | (s1[1] & 0xf);
	hex[1] = ((s1[2] & 0xf) << 4) | (s1[3] & 0xf);
}
	
/*
 * doublefreq(freq, y, len) - double to ICOM frequency with padding
 */
void
doublefreq(
	double	freq,		/* frequency */
	u_char	*x,		/* radio frequency */
	int	len		/* length (octets) */
	)
{
	int	i;
	char	s1[11];
	char	*y;

	sprintf(s1, " %10.0lf", freq);
	y = s1 + 10;
	i = 0;
	while (*y != ' ') {
		x[i] = *y-- & 0xf;
		x[i] = x[i] | ((*y-- & 0xf) << 4);
		i++;
	}
	for (; i < len; i++)
		x[i] = 0;
	x[i] = FI;
}


/*
 * freqdouble(x, len) - ICOM frequency to double
 */
double
freqdouble(			/* returns frequency */
	u_char	*x,		/* radio frequency */
	int	len		/* length (octets) */
	)
{
	int	i;
	char	s[11];
	char	*y;
	double	dtemp;

	y = s + 2 * len;
	*y-- = '\0';
	for (i = 0; i < len && x[i] != FI; i++) {
		*y-- = (char)((x[i] & 0xf) + '0');
		*y-- = (char)(((x[i] >> 4) & 0xf) + '0');
	}
	sscanf(s, "%lf", &dtemp);
	return (dtemp);
}

/* end program */
