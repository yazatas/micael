#ifndef __PC_SPEAKER_H__
#define __PC_SPEAKER_H__

/* Very simple simple function,
 * play a beep sound for "ms" milliseconds.
 * Interrupts must be enabled.
 *
 * NOTE: this function blocks execution */
void spkr_play(unsigned ms);

#endif /* __PC_SPEAKER_H__ */
