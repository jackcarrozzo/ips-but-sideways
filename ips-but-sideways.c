// clang -g -O3 -o ibs ips-but-sideways.c
//
// 2022-10-01 jack@crepinc.com
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>

#define DEBUG 0

#define FILENAME_PRE "bit-"
#define FILENAME_EXT ".bin"

typedef struct writer_t{
    uint8_t chunk; // byte into which we shift bits
    int shifted;   // how many bits weve buffered since flush
    FILE *fp;      // fp into which we flush bytes
} writer_t;

writer_t writers[32];

void init_writers() {
    int i;
    char path[256];

    char *dir="outdir"; //TODO:

    for (i=0;i<32;i++) {
        snprintf(path, sizeof(path), "%s/%s%u%s", dir, FILENAME_PRE, i, FILENAME_EXT);

        writer_t *w=&writers[i];

        w->chunk=0x00;
        w->shifted=0;
        w->fp=fopen(path, "w");

        if (!w->fp) {
            printf("failed to open path '%s'!\n", path);
            exit(1);
        }
    }
}

// take a single bit's value in and shift it into the bit idx;
//   write byte to file when 8 bits have been shifted in
void kerchunk_writer(int idx, uint8_t thisbit) {
    thisbit&=0x01;

    writer_t *w=&writers[idx];

    w->chunk<<=1;
    w->shifted++;

    if (thisbit)
        w->chunk|=0x01;
    else
        w->chunk&=0xFE;

    if (8==w->shifted) { // flush
        if ((DEBUG)&&(0==idx)) {
            printf("flushing; i=%d thisbit=%u chunk=0x%X shifted=%d\n",
                   idx, thisbit, w->chunk, w->shifted);
        }

        fputc(w->chunk, w->fp);
        w->shifted=0;
        w->chunk=0x00;
    }
}

void print_bin(uint32_t m) {
    int i;
    uint32_t mask=0x80000000;

    for (i=0;i<32;i++) {
        //printf(">> mask is 0x%X\n", mask);
        printf("%s", (m&mask)?"1":"0");
        mask>>=1;
    }
    printf("\n");
}

/*
 struct in_addr *inet_a;
 inet_a=malloc(sizeof(struct in_addr));

 //memcpy(n, inet_a, 4); // 4 bytes == 32 bits

 //inet_ntop(AF_INET, const void * restrict src, char * restrict dst, socklen_t size);
*/


// takes string, parses as unsigned int with MSB at left
int parse_addr(char *s, uint32_t *n) {
    uint8_t buf[sizeof(struct in_addr)];

    if (1!=inet_aton(s, (struct in_addr *) &buf)) {
        printf("failed to parse str '%s' as ip!\n", s);
    	return 1;
    }

    *n=0x00000000;
    *n|=buf[0]; // ip MSB
    *n<<=8;
    *n|=buf[1];
    *n<<=8;
    *n|=buf[2];
    *n<<=8;
    *n|=buf[3]; // ip LSB

    return 0;
}


int main(int argc, char **argv) {

    int i,j;

    if (2!=argc) {
        printf("usage: %s <file of ascii ips>\n", argv[0]);
        return 1;
    }

    FILE *fp;
    char *line=NULL;
    size_t len=0;
    ssize_t bread;

    printf("opening input list... ");

    fp = fopen(argv[1],"r");
    if (!fp) {
        printf("failed to open %s!\n", argv[1]);
        return 1;
    }

    printf("ok.\nopening writers... ");

    init_writers();

    printf("ok! processing...\n");

    uint32_t numlines=0;
    uint32_t numbad=0;
    uint32_t m;

    while ((bread=getline(&line,&len,fp))!=-1) {
        if (DEBUG) printf("Retrieved line of length %zu: %s", bread, line);

	if (0==(numlines%5000000)) printf(".");

	numlines++;
	if (1==parse_addr(line, &m)) {
	    numbad++;
	    continue;
	}

        if (DEBUG) print_bin(m);

        for (i=31;i>=0;i--) { // order such that bit-31.bin is msb
            int bit;

            if (m & 0x80000000) // msb
                bit=1;
            else
                bit=0;

            kerchunk_writer(i, bit);
            m<<=1;
        }
    }

    fclose(fp);
    if (line) free(line);

   printf("\nfinished address list, flushing writers... ");

    // kerchunk up to 7 zeros to flush last byte of each writer to file
    int n=8-writers[0].shifted; // hopefully all the bits have been shifted identically
    for (i=0;i<n;i++) {
        for (j=0;j<32;j++)
            kerchunk_writer(j, 0);
    }

    printf("ok!\n");
    printf("--> read %ul lines, failed to parse %ul of them.\n", numlines, numbad);
    printf("--> wrote %ld bytes to each of " FILENAME_PRE "N" FILENAME_EXT "\n",
           ftell(writers[0].fp));

    for (i=0;i<32;i++) fclose(writers[i].fp);

    return 0;

    //struct in_addr *inet_a;
    //inet_a=malloc(sizeof(struct in_addr));

    //r=inet_aton(moose2, inet_a);
    //uint32_t *m=(uint32_t *)inet_a;

    //printf("str: '%s', r: %d, as int: %u\n",
    //       moose2, r, *m);


    //print_bin(0xaaaa5555);

    /*uint32_t k=1;
    for (i=0;i<32;i++) {
        printf(">>>> k is 0x%X\n", k);
        k<<=1;
    }*/

    //return 0;
}
