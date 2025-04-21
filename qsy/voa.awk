BEGIN {
} {
	nf = NF
	if ($3 == "SSN" && $1 != month) {
		month = $1
	}
	if ($nf == "FREQ") {
		hour = $1
		for (i = 3; i < nf; i++)
			f[i - 1] = $i
	}
	if ($nf == "PRB") {
		for (i = 2; i < nf; i++)
			prob[i] = $i
	}
	max = -1e10
	if ($nf == "SNRxx") {
		for (i = 1; i < nf; i++) {
			snr[i] = $i
			if (snr[i] >= 20 && prob[i] >= .5)
				printf("%4s %3d %4s %4s %5s %s\n", month, hour, snr[i], prob[i], f[i], FILENAME)
		}
	}
}

