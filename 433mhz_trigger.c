#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define PREQUEL_SIZE	200
#define THRESHOLD		31000
#define TRAILER_SIZE	500	

#define ABS(x)	( ((x)>0) ? (x) : (-(x)) )

void dump_rbuf(int16_t *d, int pos) {
	int idx = pos;
	do {
		printf("%d\n",d[idx]);
		idx++;
		if(idx>=PREQUEL_SIZE)
			idx=0;
	} while(idx != pos);
}

int main(int argc, char **argv) {
	int16_t rbuf[PREQUEL_SIZE];
	int16_t val, last=0;
	int idx=0, cnt=0;

	while(read(0, &val, 2) == 2) {
		if(ABS(val-last) > THRESHOLD) {
			if(!cnt)
				dump_rbuf(rbuf, idx);
			cnt = TRAILER_SIZE;
		}
		if(cnt>0) {
			printf("%d\n",val);
			cnt--;
		}
		last=rbuf[idx]=val;
		idx++;
		if(idx>=PREQUEL_SIZE)
			idx=0;
	}
	return 0;
}
