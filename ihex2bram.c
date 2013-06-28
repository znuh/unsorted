/*  Copyright (C) 2013 Benedikt Heinz <Zn000h AT gmail.com>
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <assert.h>

int main(int argc, char **argv) {
	uint8_t bram[2048];
	char buf[256];
	FILE *fl;
	int i,j;
	int dlen=0;
	
	bzero(bram, 2048);
	
	assert(argc>1);
	fl = fopen(argv[1], "r");
	assert(fl);
	while(fgets(buf,256,fl)) {
		int i, val, res, len, addr, type;
		assert(buf[0] == ':');
		res=sscanf(buf+1,"%02x%04x%02x",&len,&addr,&type);
		assert(res == 3);
		//printf("type %d len %d addr %04x\n",type,len,addr);
		if(type == 1) break;
		else if(type == 2) continue;
		for(i=0;i<len;i++) {
			res=sscanf(buf+9+(i*2),"%02x",&val);
			assert(res == 1);
			bram[addr+i]=val;
		}
		dlen = (addr+i) > dlen ? addr+i : dlen;
	}
	fclose(fl);
	
	for(i=0;i<=(dlen/32);i++) {
		printf("      INIT_%02X => X\"",i);
		for(j=0;j<32;j++) {
			printf("%02X",bram[i*32+31-j]);
		}
		printf("\",\n");
	}
	
	return 0;
}
