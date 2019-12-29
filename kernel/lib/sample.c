#include <kernel/kassert.h>
#include <lib/sample.h>
#include <mm/slab.h>
#include <mm/heap.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#define SSMPLR_ERROR(err) ((errno = (err)) ? NULL : NULL)

typedef struct sample {
    size_t s_ptr;     /* current pointer in "samples" */
    size_t w_total;   /* total of current window */
    uint8_t *data;    /* data samples */
} sample_t;

/* sliding sampler */
struct ssampler {
    size_t w_size;    /* window size, ms */
    size_t s_size;    /* size of one sample in bytes */
    size_t nclasses;  /* # of classes (see explanation below)  */

    struct sample **samples;
};

/* What we want to do is to store a collection of samples from some time window.
 * Basically, if we want to store, for example, FPS of the past 10 seconds we need
 * to store each FPS value to an array and as time progresses, overwrite the oldest
 * FPS value with a new one. In other words, the oldest value decays and it is replaced
 * by a new value. This way we always have the most recent FPS data from past 10 seconds
 * and this is implemented as simple ring buffer and the FPS total is stored separate so
 * we can get the average of FPS of past 10 seconds in O(1) time.
 *
 * This creates a problem however. If our sample rate is 1ms, that is, we store a FPS
 * value once per millisecond, the sampler consumes a lot of memory. Say, we store the
 * FPS value to a uint8_t, we need 1 * 10 * 1000 = 10 KB of memory just for the FPS sampler.
 *
 * To mitigate this memory overhead, what ssampler instead does is that it keeps averages
 * of multiples of tens of the FPS. What this means is that instead of keeping 10,000 samples
 * and having 1ms-granularity average of FPS of the past 10 seconds (which is very wasteful
 * and not very useful because one sample does not change the 10s average at all), we instead
 * keep averages 10ms, 100ms, 1s, 10s etc. This way the average of previous 10ms is moved to
 * higher level and when we have N * 100ms samples, we move that to a higher level etc.
 *
 * Using this model we can greatly reduce the memory footprint of the program but we need to
 * define two variable:
 *      - Decay rate of a sample
 *      - Average factor
 *
 * Because we are no longer storing every sample from the whole time window we're interested in,
 * we need to define a decay rate for a symbol. Decay rate tells us how many sample we can store
 * simulatenously until the oldest sample is destroyed and replaced by a new sample.
 *
 * Average factor tells what is the largest time window average we're interested in.
 * Current version of ssampler can only work with multiples of 10, so if we set our average
 * factor to 2 and decay rate to 100, we can query the following time averages of samples:
 *      - 100 * 10^0 = 100ms
 *      - 100 * 10^1 = 1000ms (1s)
 *      - 100 * 10^2 = 10000s (10s)
 *
 * Which is the same as above where we stored all 10,000 samples but now we consume only
 * 3 * 1 * 100 = 300 bytes of memory. This optimized version consumes 97% less memory!
 *
 * Currently ssampler has two limitations: it can only deal with time average windows in
 * multiples on ten AND it requires that the sample size is known at compile time.
 *
 */

/* @param w_size:   Decay rate of a sample
 * @param nclasses: How many classes of samples are stored.
 *
 * Return pointer to ssampler_t on success
 * Return NULL on error and set errno to:
 *   EINVAL if one of the parameters is invalid
 *   ENOMEM if allocation failed */
ssampler_t *ssmplr_init(size_t w_size, unsigned nclasses)
{
    if (w_size == 0 || w_size == 0 || nclasses < 1)
        return SSMPLR_ERROR(EINVAL);

    ssampler_t *ss = kmalloc(sizeof(ssampler_t));

    if (!ss)
        return SSMPLR_ERROR(ENOMEM);

    if ((ss->samples = kmalloc(nclasses * sizeof(sample_t *))) == NULL) {
        kfree(ss);
        return SSMPLR_ERROR(ENOMEM);
    }

    /* Initialize every sample class separately */
    for (size_t i = 0; i < nclasses; ++i) {
        ss->samples[i] = kmalloc(sizeof(sample_t));

        ss->samples[i]->s_ptr    = 0;
        ss->samples[i]->w_total  = 0;
        ss->samples[i]->data     = kmalloc(w_size);
    }

    ss->w_size   = w_size;
    ss->s_size   = sizeof(uint8_t);
    ss->nclasses = nclasses;

    return ss;
}

/* Free all underlying memory of "ss" and destroy the object
 *
 * Return 0 on success
 * Return -EINVAL if "ss" is NULL */
int ssmplr_deinit(ssampler_t *ss)
{
    if (!ss)
        return -EINVAL;

    for (size_t i = 0; i < ss->nclasses; ++i) {
        kfree(ss->samples[i]->data);
        kfree(ss->samples[i]);
    }

    kfree(ss->samples);
    kfree(ss);

    return 0;
}

static int __add_sample(ssampler_t *ss, uint8_t sample, unsigned class)
{
    kassert(ss != NULL);

    if (ss->nclasses == class)
        return 0;

    size_t index = ss->samples[class]->s_ptr;

    /* Decay the old value before inserting a new one */
    if (index >= ss->w_size) {
        index                        = ss->samples[class]->s_ptr % ss->w_size;
        ss->samples[class]->w_total -= ss->samples[class]->data[index];
    }

    ss->samples[class]->data[index]  = sample;
    ss->samples[class]->w_total     += sample;
    ss->samples[class]->s_ptr       += 1;

    if (ss->samples[0]->s_ptr % ss->w_size == 0)
        return __add_sample(ss, ss->samples[class]->w_total / ss->w_size, class + 1);
    return 0;
}

/* Add sample to slider pointed to by "slider"
 * The sample is always added to the first class
 *
 * Return 0 on success
 * Return -EINVAL if "slider" is NULL */
int ssmplr_add_sample(ssampler_t *ss, uint8_t sample)
{
    if (!ss)
        return -EINVAL;
    return __add_sample(ss, sample, 0);
}

/* Get the average of collected samples for "class"
 * Class values start from 0, not 1!
 *
 * Return average on success
 * Return -EINVAL if "ss" is NULL
 * Return -EINVAL if "class" > ss->nclasses
 * Return -EINPROGRESS if class does not have enough samples
 * Return -1 if # of collected < size of the window */
int ssmplr_get_avg(ssampler_t *ss, unsigned class)
{
    if (!ss || ss->nclasses <= class)
        return -EINVAL;

    if (ss->samples[class]->s_ptr < ss->w_size)
        return -EINPROGRESS;

    return ss->samples[class]->w_total / ss->w_size;
}
