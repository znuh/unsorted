#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>

// gcc -Wall -o alaw_gentab alaw_gentab.c
// needs SOX installed

int main(int argc, char **argv) {
	FILE *fl;
	uint8_t alaw_in[256];
	int16_t s16_out[256];
	int i;
	int max_out, scale;

	if(argc<2) {
		printf("usage: %s <F_CPU>\n",argv[0]);
		return 0;
	}

	max_out = atoi(argv[1])/(4*8000);
	scale = (32256*2) / max_out;

	for(i=0; i<256; i++)
		alaw_in[i]=i;

	assert((fl=fopen("in.al","wb")));
	assert(fwrite(alaw_in, sizeof(alaw_in), 1, fl)==1);
	fclose(fl);

	fl=popen("sox -r 8000 -c 1 in.al -t raw -e signed-integer -b 16 -","r");
	if(!fl) {
		fprintf(stderr,"can't run sox. probably not installed?\n");
		return -1;
	}
	assert(fread(s16_out, sizeof(s16_out), 1, fl)==1);
	fclose(fl);

	printf(".equ LUT_MAXVAL = %d\n", max_out);
	
	for(i=0; i<256; i++) {
		int conv = s16_out[i];
		conv += 32256 + (scale/2);
		conv /= scale;
		
		//printf("%d\n",conv);
		if(!(i%8)) printf(".db ");
		printf("%1d, %3d",conv>>8, conv&0xff);
		if((i%8)==7) printf("\n");
		else printf(", ");
	}

	return 0;
}
