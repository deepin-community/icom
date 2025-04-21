BEGIN {
}
{
	if (NF == 1 && substr($1, 1, 5) == "[BANK") {
		bank = substr($1, 6, 2)
		chan = 0
	}
	if (NF == 9) {
		printf("%d.%d %s %s name %s\n", bank, chan, $2, $3, substr($1, 4, 8))
		chan++
	}
} END {
}
