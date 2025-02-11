// SPDX-License-Identifier: GPL-2.0-only
/*
 * String functions optimized for hardware which doesn't
 * handle unaligned memory accesses efficiently.
 *
 * Copyright (C) 2021 Matteo Croce
 */

#include <linux/types.h>
#include <linux/module.h>

/* Minimum size for a word copy to be convenient */
#define BYTES_LONG	sizeof(long)
#define WORD_MASK	(BYTES_LONG - 1)
#define MIN_THRESHOLD	(BYTES_LONG * 2)

/* convenience union to avoid cast between different pointer types */
union types {
	u8 *as_u8;
	unsigned long *as_ulong;
	uintptr_t as_uptr;
};

union const_types {
	const u8 *as_u8;
	const unsigned long *as_ulong;
	const uintptr_t as_uptr;
};

static void __memcpy_aligned(unsigned long *dest, const unsigned long *src, size_t count)
{
	for (; count > 0; count -= BYTES_LONG * 8) {
		register unsigned long d0, d1, d2, d3, d4, d5, d6, d7;

		d0 = src[0];
		d1 = src[1];
		d2 = src[2];
		d3 = src[3];
		d4 = src[4];
		d5 = src[5];
		d6 = src[6];
		d7 = src[7];
		dest[0] = d0;
		dest[1] = d1;
		dest[2] = d2;
		dest[3] = d3;
		dest[4] = d4;
		dest[5] = d5;
		dest[6] = d6;
		dest[7] = d7;
		dest += 8;
		src += 8;
	}
}

void *__memcpy(void *dest, const void *src, size_t count)
{
	union const_types s = { .as_u8 = src };
	union types d = { .as_u8 = dest };
	int distance = 0;

	if (count < MIN_THRESHOLD)
		goto copy_remainder;

	if (!IS_ENABLED(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS)) {
		/* Copy a byte at time until destination is aligned. */
		for (; d.as_uptr & WORD_MASK; count--)
			*d.as_u8++ = *s.as_u8++;

		distance = s.as_uptr & WORD_MASK;
	}

	if (distance) {
		unsigned long last, next;

		/*
		 * s is distance bytes ahead of d, and d just reached
		 * the alignment boundary. Move s backward to word align it
		 * and shift data to compensate for distance, in order to do
		 * word-by-word copy.
		 */
		s.as_u8 -= distance;

		next = s.as_ulong[0];
		for (; count >= BYTES_LONG; count -= BYTES_LONG) {
			last = next;
			next = s.as_ulong[1];

			d.as_ulong[0] = last >> (distance * 8) |
					next << ((BYTES_LONG - distance) * 8);

			d.as_ulong++;
			s.as_ulong++;
		}

		/* Restore s with the original offset. */
		s.as_u8 += distance;
	} else {
		/*
		 * If the source and dest lower bits are the same, do a simple
		 * aligned copy.
		 */
		size_t aligned_count = count & ~(BYTES_LONG * 8 - 1);

		__memcpy_aligned(d.as_ulong, s.as_ulong, aligned_count);
		d.as_u8 += aligned_count;
		s.as_u8 += aligned_count;
		count &= BYTES_LONG * 8 - 1;
	}

copy_remainder:
	while (count--)
		*d.as_u8++ = *s.as_u8++;

	return dest;
}
EXPORT_SYMBOL(__memcpy);

void *memcpy(void *dest, const void *src, size_t count) __weak __alias(__memcpy);
EXPORT_SYMBOL(memcpy);
void *__pi_memcpy(void *dest, const void *src, size_t count) __alias(__memcpy);
void *__pi___memcpy(void *dest, const void *src, size_t count) __alias(__memcpy);

/*
 * Simply check if the buffer overlaps an call memcpy() in case,
 * otherwise do a simple one byte at time backward copy.
 */
void *__memmove(void *dest, const void *src, size_t count)
{
	if (dest < src || src + count <= dest)
		return __memcpy(dest, src, count);

	if (dest > src) {
		const char *s = src + count;
		char *tmp = dest + count;

		while (count--)
			*--tmp = *--s;
	}
	return dest;
}
EXPORT_SYMBOL(__memmove);

void *memmove(void *dest, const void *src, size_t count) __weak __alias(__memmove);
EXPORT_SYMBOL(memmove);
void *__pi_memmove(void *dest, const void *src, size_t count) __alias(__memmove);
void *__pi___memmove(void *dest, const void *src, size_t count) __alias(__memmove);


void *__memset(void *s, int c, size_t count)
{
	union types dest = { .as_u8 = s };

	if (count >= MIN_THRESHOLD) {
		unsigned long cu = (unsigned long)c;

		/* Compose an ulong with 'c' repeated 4/8 times */
#ifdef CONFIG_ARCH_HAS_FAST_MULTIPLIER
		cu *= 0x0101010101010101UL;
#else
		cu |= cu << 8;
		cu |= cu << 16;
		/* Suppress warning on 32 bit machines */
		cu |= (cu << 16) << 16;
#endif
		if (!IS_ENABLED(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS)) {
			/*
			 * Fill the buffer one byte at time until
			 * the destination is word aligned.
			 */
			for (; count && dest.as_uptr & WORD_MASK; count--)
				*dest.as_u8++ = c;
		}

		/* Copy using the largest size allowed */
		for (; count >= BYTES_LONG; count -= BYTES_LONG)
			*dest.as_ulong++ = cu;
	}

	/* copy the remainder */
	while (count--)
		*dest.as_u8++ = c;

	return s;
}
EXPORT_SYMBOL(__memset);

void *memset(void *s, int c, size_t count) __weak __alias(__memset);
EXPORT_SYMBOL(memset);
