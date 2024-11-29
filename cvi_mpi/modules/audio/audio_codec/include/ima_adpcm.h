/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/ima_adpcm.h
 * Description:
 *   Common adpcm audio  definitions & api for application.
 */
/*
 * SpanDSP - a series of DSP components for telephony
 *
 * ima_adpcm.c - Conversion routines between linear 16 bit PCM data and
 *		         IMA/DVI/Intel ADPCM format.
 *
 * Based on a bit from here, a bit from there, eye of toad,
 * ear of bat, etc - plus, of course, my own 2 cents.
 */

/*! \file */

#if !defined(_IMA_ADPCM_H_)
#define _IMA_ADPCM_H_


#if defined(_M_IX86)  ||  defined(_M_X64)
#if defined(LIBSPANDSP_EXPORTS)
#define SPAN_DECLARE
#define SPAN_DECLARE_NONSTD
#define SPAN_DECLARE_DATA
#else
#define SPAN_DECLARE
#define SPAN_DECLARE_NONSTD
#define SPAN_DECLARE_DATA
#endif
#elif (defined(__GNUC__)  ||  defined(__SUNCC__))
#if 0

#endif
#define SPAN_DECLARE              __attribute__((visibility("default")))
#define SPAN_DECLARE_NONSTD       __attribute__((visibility("default")))
#define SPAN_DECLARE_DATA               __attribute__((visibility("default")))
#else
#if 0

#endif
#define SPAN_DECLARE              /**/
#define SPAN_DECLARE_NONSTD       /**/
#define SPAN_DECLARE_DATA               /**/
#endif


/*! \page ima_adpcm_page IMA/DVI/Intel ADPCM encoding and decoding
 *section ima_adpcm_page_sec_1 What does it do?
 *IMA ADPCM offers a good balance of simplicity and quality at a rate of
 *32kbps.
 *
 *section ima_adpcm_page_sec_2 How does it work?
 *
 *section ima_adpcm_page_sec_3 How do I use it?
 */

enum {
	/*! IMA4 is the original IMA ADPCM variant */
	IMA_ADPCM_IMA4 = 0,
	/*! DVI4 is the IMA ADPCM variant defined in RFC3551 */
	IMA_ADPCM_DVI4 = 1,
	/*! VDVI is the variable bit rate IMA ADPCM variant defined in RFC3551 */
	IMA_ADPCM_VDVI = 2
};

/*!
 *IMA (DVI/Intel) ADPCM conversion state descriptor. This defines the state of
 *a single working instance of the IMA ADPCM converter. This is used for
 *either linear to ADPCM or ADPCM to linear conversion.
 */
typedef struct ima_adpcm_state_s ima_adpcm_state_t;

#if defined(__cplusplus)
extern "C"
{
#endif


static inline CVI_S16 saturate(CVI_S32 amp)
{
	CVI_S16 amp16;

	/* Hopefully this is optimised for the common case - not clipping */
	amp16 = (CVI_S16) amp;
	if (amp == amp16)
		return amp16;
	if (amp > INT16_MAX)
		return INT16_MAX;
	return INT16_MIN;
}

/*!
 *IMA (DVI/Intel) ADPCM conversion state descriptor. This defines the state of
 *a single working instance of the IMA ADPCM converter. This is used for
 *either linear to ADPCM or ADPCM to linear conversion.
 */
struct ima_adpcm_state_s {
	int variant;
	/*! \brief The size of a chunk, in samples. */
	int chunk_size;
	/*! \brief The last state of the ADPCM algorithm. */
	int last;
	/*! \brief Current index into the step size table. */
	int step_index;
	/*! \brief The current IMA code byte in progress. */
	CVI_U16 ima_byte;
	int bits;
};

/*! Initialise an IMA ADPCM encode or decode context.
 *param s The IMA ADPCM context.
 *param variant IMA_ADPCM_IMA4, IMA_ADPCM_DVI4, or IMA_ADPCM_VDVI.
 *param chunk_size The size of a chunk, in samples. A chunk size of
 *      zero sample samples means treat each encode or decode operation
 *      as a chunk.
 *return A pointer to the IMA ADPCM context, or NULL for error.
 */
SPAN_DECLARE ima_adpcm_state_t *ima_adpcm_init(ima_adpcm_state_t *s,
						int variant,
						int chunk_size);

/*! Release an IMA ADPCM encode or decode context.
 *param s The IMA ADPCM context.
 *return 0 for OK.
 */
SPAN_DECLARE int  ima_adpcm_release(ima_adpcm_state_t *s);

/*! Free an IMA ADPCM encode or decode context.
 *param s The IMA ADPCM context.
 *return 0 for OK.
 */
SPAN_DECLARE int ima_adpcm_free(ima_adpcm_state_t *s);

/*! Encode a buffer of linear PCM data to IMA ADPCM.
 *param s The IMA ADPCM context.
 *param ima_data The IMA ADPCM data produced.
 *param amp The audio sample buffer.
 *param len The number of samples in the buffer.
 *return The number of bytes of IMA ADPCM data produced.
 */
SPAN_DECLARE int  ima_adpcm_encode(ima_adpcm_state_t *s,
					CVI_U8 ima_data[],
					const CVI_S16 amp[],
					int len);

/*! Decode a buffer of IMA ADPCM data to linear PCM.
 *param s The IMA ADPCM context.
 *param amp The audio sample buffer.
 *param ima_data The IMA ADPCM data
 *param ima_bytes The number of bytes of IMA ADPCM data
 *return The number of samples returned.
 */
SPAN_DECLARE int ima_adpcm_decode(ima_adpcm_state_t *s,
			CVI_S16 amp[],
			const CVI_U8 ima_data[],
			int ima_bytes);

#if defined(__cplusplus)
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/

