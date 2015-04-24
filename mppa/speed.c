#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <zlib.h>

#define NUM		10
#define BUFLEN	16384
#define THREADS	16
#define FREQ	0.4

#define CYCLE_NS	(1/FREQ)

#ifdef __k1__
#include <HAL/hal/hal.h>
#define STACKSZ	(20*1024)
#define CONFIGURE_MINIMUM_TASK_STACK_SIZE	STACKSZ
#define ONFIGURE_DEFAULT_TASK_STACK_SIZE	STACKSZ
#define CONFIGURE_AMP_MAIN_STACK_SIZE		STACKSZ
#include <mppa/osconfig.h>
#define cycles()	__k1_read_dsu_timestamp()
#else	/* __k1__ */
unsigned long long rdtsc(void)
{
	unsigned int tickl, tickh;
	__asm__ __volatile__("rdtsc":"=a"(tickl),"=d"(tickh));
	return ((unsigned long long)tickh << 32)|tickl;
}
#define cycles()	rdtsc()
#endif

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
	/* default: deflateInit2(strm, 1, 8, 15, 8, 0) */
	return deflateInit2(strm, 1, 8, 12, 5, 0);
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
	char buf[BUFLEN];
	z_stream strm;

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
#pragma omp parallel num_threads(THREADS)
	{
		int i;
		long long start = cycles();
		unsigned long len = 0;
		for (i=0; i<NUM; i++) {
			len += test();
		}
		long long cy = cycles() - start;
#define SZ	(NUM*sizeof(calgary))
#pragma omp critical
		printf("[%i] Compressed %luB to %luB (ratio = %04.4g%%) in %lli cycles (%04.4gc/B, %gGbps)\n",
				(int)omp_get_thread_num(),
				SZ, len, (100.0*len)/SZ, cy,
				(double)cy/SZ, (8*SZ)/(cy*CYCLE_NS));
	}	/* end of parallel */
	return 0;
}
