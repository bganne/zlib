#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#ifdef __k1__
#include <HAL/hal/hal.h>
#include <mppa/osconfig.h>
#define cycles()	__k1_read_dsu_timestamp()
#define CYCLE_NS	(1/0.4)
#else	/* __k1__ */
unsigned long long rdtsc(void)
{
	unsigned int tickl, tickh;
	__asm__ __volatile__("rdtsc":"=a"(tickl),"=d"(tickh));
	return ((unsigned long long)tickh << 32)|tickl;
}
#define cycles()	rdtsc()
#define CYCLE_NS	(1/1.8)
#endif

#define NUM		10
#define BUFLEN	16384

static void *myalloc(void *q, unsigned n, unsigned m)
{
    q = Z_NULL;
    return calloc(n, m);
}

static void myfree(void *q, void *p)
{
    q = Z_NULL;
    free(p);
}

static int init(z_stream *strm)
{
	memset(strm, 0, sizeof(*strm));
	strm->zalloc = myalloc;
	strm->zfree = myfree;
	strm->opaque = Z_NULL;
	return deflateInit2(strm, 1, 8, 15 + 16, 8, 0);
}

static int finish(void *out, int olen, z_stream *strm)
{
	strm->next_in = Z_NULL;
	strm->avail_in = 0;
	int len = 0;
	do {
		strm->next_out = out;
		strm->avail_out = olen;
		(void)deflate(strm, Z_FINISH);
		len += olen;
	} while (0 == strm->avail_out);
	len -= strm->avail_out;
	deflateEnd(strm);
	return len;
}

static int comp(void *out, int olen, z_stream *strm, const void *in, int ilen)
{
    strm->next_in = (void *)in;
    strm->avail_in = ilen;
	int len = 0;
	do {
		strm->next_out = out;
		strm->avail_out = olen;
		(void)deflate(strm, Z_NO_FLUSH);
		len += olen;
	} while (0 == strm->avail_out);
	return len - strm->avail_out;
}

static int test(void)
{
	static char buf[BUFLEN];
	static z_stream strm;

	int err = init(&strm);
	if (Z_OK != err) {
		perror("init() failed");
		return err;
	}

	int len = comp(buf, sizeof(buf), &strm, calgary, sizeof(calgary));
	len += finish(buf, sizeof(buf), &strm);
	return len;
}

int main(void)
{
	int i;
	long long start = cycles();
	unsigned long len = 0;
	for (i=0; i<NUM; i++) {
		len += test();
	}
	long long cy = cycles() - start;
#define SZ	(NUM*sizeof(calgary))
	printf("Compressed %luB to %luB (ratio = %04.4g%%) in %lli cycles (%04.4gc/B, %gGbps)\n",
			SZ, len, (100.0*len)/SZ, cy,
			(double)cy/SZ, (8*SZ)/(cy*CYCLE_NS));
	return 0;
}
