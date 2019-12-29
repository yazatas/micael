#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#include <sys/types.h>

typedef struct ssampler ssampler_t;

/* Initialize new sliding sampler object
 *
 * @param w_size:   Decay rate of a sample
 * @param nclasses: How many classes of samples are stored.
 *
 * Return pointer to ssampler_t on success
 * Return NULL on error and set errno to:
 *   EINVAL if one of the parameters is invalid
 *   ENOMEM if allocation failed */
ssampler_t *ssmplr_init(size_t w_size, unsigned nclasses);

/* Free all underlying memory of "ss" and destroy the object
 *
 * Return 0 on success
 * Return -EINVAL if "ss" is NULL */
int ssmplr_deinit(ssampler_t *ss);

/* Add sample to slider pointed to by "slider"
 * The sample is always added to the first class
 *
 * Return 0 on success
 * Return -EINVAL if "slider" is NULL */
int ssmplr_add_sample(ssampler_t *ss, uint8_t sample);

/* Get the average of collected samples for "class"
 * Class values start from 0, not 1!
 *
 * Return average on success
 * Return -EINVAL if "ss" is NULL
 * Return -EINVAL if "class" > ss->nclasses
 * Return -EINPROGRESS if class does not have enough samples
 * Return -1 if # of collected < size of the window */
int ssmplr_get_avg(ssampler_t *ss, unsigned class);

#endif /* __SAMPLE_H__ */
