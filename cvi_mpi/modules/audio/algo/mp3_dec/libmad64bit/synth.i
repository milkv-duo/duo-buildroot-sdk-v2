# 1 "synth.c"
# 1 "/home/vincentyu/mp3/mad0426/libmad32bit//"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/home/vincentyu/master_repo4/host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf/arm-linux-gnueabihf/libc/usr/include/stdc-predef.h" 1 3 4
# 1 "<command-line>" 2
# 1 "synth.c"
# 23 "synth.c"
# 1 "config.h" 1
# 24 "synth.c" 2


# 1 "global.h" 1
# 27 "synth.c" 2

# 1 "fixed.h" 1
# 26 "fixed.h"
typedef signed int mad_fixed_t;

typedef signed int mad_fixed64hi_t;
typedef unsigned int mad_fixed64lo_t;
# 46 "fixed.h"
typedef mad_fixed_t mad_sample_t;
# 496 "fixed.h"
mad_fixed_t mad_f_abs(mad_fixed_t);
mad_fixed_t mad_f_div(mad_fixed_t, mad_fixed_t);
# 29 "synth.c" 2
# 1 "frame.h" 1
# 26 "frame.h"
# 1 "timer.h" 1
# 25 "timer.h"
typedef struct {
  signed long seconds;
  unsigned long fraction;
} mad_timer_t;

extern mad_timer_t const mad_timer_zero;



enum mad_units {
  MAD_UNITS_HOURS = -2,
  MAD_UNITS_MINUTES = -1,
  MAD_UNITS_SECONDS = 0,



  MAD_UNITS_DECISECONDS = 10,
  MAD_UNITS_CENTISECONDS = 100,
  MAD_UNITS_MILLISECONDS = 1000,



  MAD_UNITS_8000_HZ = 8000,
  MAD_UNITS_11025_HZ = 11025,
  MAD_UNITS_12000_HZ = 12000,

  MAD_UNITS_16000_HZ = 16000,
  MAD_UNITS_22050_HZ = 22050,
  MAD_UNITS_24000_HZ = 24000,

  MAD_UNITS_32000_HZ = 32000,
  MAD_UNITS_44100_HZ = 44100,
  MAD_UNITS_48000_HZ = 48000,



  MAD_UNITS_24_FPS = 24,
  MAD_UNITS_25_FPS = 25,
  MAD_UNITS_30_FPS = 30,
  MAD_UNITS_48_FPS = 48,
  MAD_UNITS_50_FPS = 50,
  MAD_UNITS_60_FPS = 60,



  MAD_UNITS_75_FPS = 75,



  MAD_UNITS_23_976_FPS = -24,
  MAD_UNITS_24_975_FPS = -25,
  MAD_UNITS_29_97_FPS = -30,
  MAD_UNITS_47_952_FPS = -48,
  MAD_UNITS_49_95_FPS = -50,
  MAD_UNITS_59_94_FPS = -60
};



int mad_timer_compare(mad_timer_t, mad_timer_t);



void mad_timer_negate(mad_timer_t *);
mad_timer_t mad_timer_abs(mad_timer_t);

void mad_timer_set(mad_timer_t *, unsigned long, unsigned long, unsigned long);
void mad_timer_add(mad_timer_t *, mad_timer_t);
void mad_timer_multiply(mad_timer_t *, signed long);

signed long mad_timer_count(mad_timer_t, enum mad_units);
unsigned long mad_timer_fraction(mad_timer_t, unsigned long);
void mad_timer_string(mad_timer_t, char *, char const *,
        enum mad_units, enum mad_units, unsigned long);
# 27 "frame.h" 2
# 1 "stream.h" 1
# 25 "stream.h"
# 1 "bit.h" 1
# 25 "bit.h"
struct mad_bitptr {
  unsigned char const *byte;
  unsigned short cache;
  unsigned short left;
};

void mad_bit_init(struct mad_bitptr *, unsigned char const *);



unsigned int mad_bit_length(struct mad_bitptr const *,
       struct mad_bitptr const *);


unsigned char const *mad_bit_nextbyte(struct mad_bitptr const *);

void mad_bit_skip(struct mad_bitptr *, unsigned int);
unsigned long mad_bit_read(struct mad_bitptr *, unsigned int);
void mad_bit_write(struct mad_bitptr *, unsigned int, unsigned long);

unsigned short mad_bit_crc(struct mad_bitptr, unsigned int, unsigned short);
# 26 "stream.h" 2




enum mad_error {
  MAD_ERROR_NONE = 0x0000,

  MAD_ERROR_BUFLEN = 0x0001,
  MAD_ERROR_BUFPTR = 0x0002,

  MAD_ERROR_NOMEM = 0x0031,

  MAD_ERROR_LOSTSYNC = 0x0101,
  MAD_ERROR_BADLAYER = 0x0102,
  MAD_ERROR_BADBITRATE = 0x0103,
  MAD_ERROR_BADSAMPLERATE = 0x0104,
  MAD_ERROR_BADEMPHASIS = 0x0105,

  MAD_ERROR_BADCRC = 0x0201,
  MAD_ERROR_BADBITALLOC = 0x0211,
  MAD_ERROR_BADSCALEFACTOR = 0x0221,
  MAD_ERROR_BADMODE = 0x0222,
  MAD_ERROR_BADFRAMELEN = 0x0231,
  MAD_ERROR_BADBIGVALUES = 0x0232,
  MAD_ERROR_BADBLOCKTYPE = 0x0233,
  MAD_ERROR_BADSCFSI = 0x0234,
  MAD_ERROR_BADDATAPTR = 0x0235,
  MAD_ERROR_BADPART3LEN = 0x0236,
  MAD_ERROR_BADHUFFTABLE = 0x0237,
  MAD_ERROR_BADHUFFDATA = 0x0238,
  MAD_ERROR_BADSTEREO = 0x0239
};



struct mad_stream {
  unsigned char const *buffer;
  unsigned char const *bufend;
  unsigned long skiplen;

  int sync;
  unsigned long freerate;

  unsigned char const *this_frame;
  unsigned char const *next_frame;
  struct mad_bitptr ptr;

  struct mad_bitptr anc_ptr;
  unsigned int anc_bitlen;

  unsigned char (*main_data)[(511 + 2048 + 8)];

  unsigned int md_len;

  int options;
  enum mad_error error;
};

enum {
  MAD_OPTION_IGNORECRC = 0x0001,
  MAD_OPTION_HALFSAMPLERATE = 0x0002





};

void mad_stream_init(struct mad_stream *);
void mad_stream_finish(struct mad_stream *);




void mad_stream_buffer(struct mad_stream *,
         unsigned char const *, unsigned long);
void mad_stream_skip(struct mad_stream *, unsigned long);

int mad_stream_sync(struct mad_stream *);

char const *mad_stream_errorstr(struct mad_stream const *);
# 28 "frame.h" 2

enum mad_layer {
  MAD_LAYER_I = 1,
  MAD_LAYER_II = 2,
  MAD_LAYER_III = 3
};

enum mad_mode {
  MAD_MODE_SINGLE_CHANNEL = 0,
  MAD_MODE_DUAL_CHANNEL = 1,
  MAD_MODE_JOINT_STEREO = 2,
  MAD_MODE_STEREO = 3
};

enum mad_emphasis {
  MAD_EMPHASIS_NONE = 0,
  MAD_EMPHASIS_50_15_US = 1,
  MAD_EMPHASIS_CCITT_J_17 = 3,
  MAD_EMPHASIS_RESERVED = 2
};

struct mad_header {
  enum mad_layer layer;
  enum mad_mode mode;
  int mode_extension;
  enum mad_emphasis emphasis;

  unsigned long bitrate;
  unsigned int samplerate;

  unsigned short crc_check;
  unsigned short crc_target;

  int flags;
  int private_bits;

  mad_timer_t duration;
};

struct mad_frame {
  struct mad_header header;

  int options;

  mad_fixed_t sbsample[2][36][32];
  mad_fixed_t (*overlap)[2][32][18];
};







enum {
  MAD_FLAG_NPRIVATE_III = 0x0007,
  MAD_FLAG_INCOMPLETE = 0x0008,

  MAD_FLAG_PROTECTION = 0x0010,
  MAD_FLAG_COPYRIGHT = 0x0020,
  MAD_FLAG_ORIGINAL = 0x0040,
  MAD_FLAG_PADDING = 0x0080,

  MAD_FLAG_I_STEREO = 0x0100,
  MAD_FLAG_MS_STEREO = 0x0200,
  MAD_FLAG_FREEFORMAT = 0x0400,

  MAD_FLAG_LSF_EXT = 0x1000,
  MAD_FLAG_MC_EXT = 0x2000,
  MAD_FLAG_MPEG_2_5_EXT = 0x4000
};

enum {
  MAD_PRIVATE_HEADER = 0x0100,
  MAD_PRIVATE_III = 0x001f
};

void mad_header_init(struct mad_header *);



int mad_header_decode(struct mad_header *, struct mad_stream *);

void mad_frame_init(struct mad_frame *);
void mad_frame_finish(struct mad_frame *);

int mad_frame_decode(struct mad_frame *, struct mad_stream *);

void mad_frame_mute(struct mad_frame *);
# 30 "synth.c" 2
# 1 "synth.h" 1
# 28 "synth.h"
struct mad_pcm {
  unsigned int samplerate;
  unsigned short channels;
  unsigned short length;
  mad_fixed_t samples[2][1152];
};

struct mad_synth {
  mad_fixed_t filter[2][2][2][16][8];


  unsigned int phase;

  struct mad_pcm pcm;
};


enum {
  MAD_PCM_CHANNEL_SINGLE = 0
};


enum {
  MAD_PCM_CHANNEL_DUAL_1 = 0,
  MAD_PCM_CHANNEL_DUAL_2 = 1
};


enum {
  MAD_PCM_CHANNEL_STEREO_LEFT = 0,
  MAD_PCM_CHANNEL_STEREO_RIGHT = 1
};

void mad_synth_init(struct mad_synth *);



void mad_synth_mute(struct mad_synth *);

void mad_synth_frame(struct mad_synth *, struct mad_frame const *);
# 31 "synth.c" 2





void mad_synth_init(struct mad_synth *synth)
{
  mad_synth_mute(synth);

  synth->phase = 0;

  synth->pcm.samplerate = 0;
  synth->pcm.channels = 0;
  synth->pcm.length = 0;
}





void mad_synth_mute(struct mad_synth *synth)
{
  unsigned int ch, s, v;

  for (ch = 0; ch < 2; ++ch) {
    for (s = 0; s < 16; ++s) {
      for (v = 0; v < 8; ++v) {
 synth->filter[ch][0][0][s][v] = synth->filter[ch][0][1][s][v] =
 synth->filter[ch][1][0][s][v] = synth->filter[ch][1][1][s][v] = 0;
      }
    }
  }
}
# 122 "synth.c"
static
void dct32(mad_fixed_t const in[32], unsigned int slot,
    mad_fixed_t lo[16][8], mad_fixed_t hi[16][8])
{
  mad_fixed_t t0, t1, t2, t3, t4, t5, t6, t7;
  mad_fixed_t t8, t9, t10, t11, t12, t13, t14, t15;
  mad_fixed_t t16, t17, t18, t19, t20, t21, t22, t23;
  mad_fixed_t t24, t25, t26, t27, t28, t29, t30, t31;
  mad_fixed_t t32, t33, t34, t35, t36, t37, t38, t39;
  mad_fixed_t t40, t41, t42, t43, t44, t45, t46, t47;
  mad_fixed_t t48, t49, t50, t51, t52, t53, t54, t55;
  mad_fixed_t t56, t57, t58, t59, t60, t61, t62, t63;
  mad_fixed_t t64, t65, t66, t67, t68, t69, t70, t71;
  mad_fixed_t t72, t73, t74, t75, t76, t77, t78, t79;
  mad_fixed_t t80, t81, t82, t83, t84, t85, t86, t87;
  mad_fixed_t t88, t89, t90, t91, t92, t93, t94, t95;
  mad_fixed_t t96, t97, t98, t99, t100, t101, t102, t103;
  mad_fixed_t t104, t105, t106, t107, t108, t109, t110, t111;
  mad_fixed_t t112, t113, t114, t115, t116, t117, t118, t119;
  mad_fixed_t t120, t121, t122, t123, t124, t125, t126, t127;
  mad_fixed_t t128, t129, t130, t131, t132, t133, t134, t135;
  mad_fixed_t t136, t137, t138, t139, t140, t141, t142, t143;
  mad_fixed_t t144, t145, t146, t147, t148, t149, t150, t151;
  mad_fixed_t t152, t153, t154, t155, t156, t157, t158, t159;
  mad_fixed_t t160, t161, t162, t163, t164, t165, t166, t167;
  mad_fixed_t t168, t169, t170, t171, t172, t173, t174, t175;
  mad_fixed_t t176;
# 218 "synth.c"
  t0 = in[0] + in[31]; t16 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[0] - in[31])), "r" ((((mad_fixed_t) (0x0ffb10f2L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t1 = in[15] + in[16]; t17 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[15] - in[16])), "r" ((((mad_fixed_t) (0x00c8fb30L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t41 = t16 + t17;
  t59 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t16 - t17)), "r" ((((mad_fixed_t) (0x0fec46d2L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t33 = t0 + t1;
  t50 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t0 - t1)), "r" ((((mad_fixed_t) (0x0fec46d2L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t2 = in[7] + in[24]; t18 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[7] - in[24])), "r" ((((mad_fixed_t) (0x0bdaef91L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t3 = in[8] + in[23]; t19 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[8] - in[23])), "r" ((((mad_fixed_t) (0x0abeb49aL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t42 = t18 + t19;
  t60 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t18 - t19)), "r" ((((mad_fixed_t) (0x01917a6cL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t34 = t2 + t3;
  t51 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t2 - t3)), "r" ((((mad_fixed_t) (0x01917a6cL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t4 = in[3] + in[28]; t20 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[3] - in[28])), "r" ((((mad_fixed_t) (0x0f109082L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t5 = in[12] + in[19]; t21 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[12] - in[19])), "r" ((((mad_fixed_t) (0x0563e69dL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t43 = t20 + t21;
  t61 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t20 - t21)), "r" ((((mad_fixed_t) (0x0c5e4036L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t35 = t4 + t5;
  t52 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t4 - t5)), "r" ((((mad_fixed_t) (0x0c5e4036L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t6 = in[4] + in[27]; t22 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[4] - in[27])), "r" ((((mad_fixed_t) (0x0e76bd7aL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t7 = in[11] + in[20]; t23 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[11] - in[20])), "r" ((((mad_fixed_t) (0x06d74402L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t44 = t22 + t23;
  t62 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t22 - t23)), "r" ((((mad_fixed_t) (0x0a267993L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t36 = t6 + t7;
  t53 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t6 - t7)), "r" ((((mad_fixed_t) (0x0a267993L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t8 = in[1] + in[30]; t24 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[1] - in[30])), "r" ((((mad_fixed_t) (0x0fd3aac0L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t9 = in[14] + in[17]; t25 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[14] - in[17])), "r" ((((mad_fixed_t) (0x0259020eL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t45 = t24 + t25;
  t63 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t24 - t25)), "r" ((((mad_fixed_t) (0x0f4fa0abL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t37 = t8 + t9;
  t54 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t8 - t9)), "r" ((((mad_fixed_t) (0x0f4fa0abL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t10 = in[6] + in[25]; t26 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[6] - in[25])), "r" ((((mad_fixed_t) (0x0cd9f024L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t11 = in[9] + in[22]; t27 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[9] - in[22])), "r" ((((mad_fixed_t) (0x0987fbfeL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t46 = t26 + t27;
  t64 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t26 - t27)), "r" ((((mad_fixed_t) (0x04a5018cL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t38 = t10 + t11;
  t55 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t10 - t11)), "r" ((((mad_fixed_t) (0x04a5018cL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t12 = in[2] + in[29]; t28 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[2] - in[29])), "r" ((((mad_fixed_t) (0x0f853f7eL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t13 = in[13] + in[18]; t29 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[13] - in[18])), "r" ((((mad_fixed_t) (0x03e33f2fL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t47 = t28 + t29;
  t65 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t28 - t29)), "r" ((((mad_fixed_t) (0x0e1c5979L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t39 = t12 + t13;
  t56 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t12 - t13)), "r" ((((mad_fixed_t) (0x0e1c5979L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t14 = in[5] + in[26]; t30 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[5] - in[26])), "r" ((((mad_fixed_t) (0x0db941a3L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t15 = in[10] + in[21]; t31 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((in[10] - in[21])), "r" ((((mad_fixed_t) (0x0839c3cdL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t48 = t30 + t31;
  t66 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t30 - t31)), "r" ((((mad_fixed_t) (0x078ad74eL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t40 = t14 + t15;
  t57 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t14 - t15)), "r" ((((mad_fixed_t) (0x078ad74eL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t69 = t33 + t34; t89 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t33 - t34)), "r" ((((mad_fixed_t) (0x0fb14be8L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t70 = t35 + t36; t90 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t35 - t36)), "r" ((((mad_fixed_t) (0x031f1708L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t71 = t37 + t38; t91 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t37 - t38)), "r" ((((mad_fixed_t) (0x0d4db315L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t72 = t39 + t40; t92 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t39 - t40)), "r" ((((mad_fixed_t) (0x08e39d9dL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t73 = t41 + t42; t94 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t41 - t42)), "r" ((((mad_fixed_t) (0x0fb14be8L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t74 = t43 + t44; t95 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t43 - t44)), "r" ((((mad_fixed_t) (0x031f1708L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t75 = t45 + t46; t96 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t45 - t46)), "r" ((((mad_fixed_t) (0x0d4db315L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t76 = t47 + t48; t97 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t47 - t48)), "r" ((((mad_fixed_t) (0x08e39d9dL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t78 = t50 + t51; t100 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t50 - t51)), "r" ((((mad_fixed_t) (0x0fb14be8L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t79 = t52 + t53; t101 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t52 - t53)), "r" ((((mad_fixed_t) (0x031f1708L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t80 = t54 + t55; t102 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t54 - t55)), "r" ((((mad_fixed_t) (0x0d4db315L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t81 = t56 + t57; t103 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t56 - t57)), "r" ((((mad_fixed_t) (0x08e39d9dL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t83 = t59 + t60; t106 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t59 - t60)), "r" ((((mad_fixed_t) (0x0fb14be8L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t84 = t61 + t62; t107 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t61 - t62)), "r" ((((mad_fixed_t) (0x031f1708L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t85 = t63 + t64; t108 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t63 - t64)), "r" ((((mad_fixed_t) (0x0d4db315L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t86 = t65 + t66; t109 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t65 - t66)), "r" ((((mad_fixed_t) (0x08e39d9dL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });

  t113 = t69 + t70;
  t114 = t71 + t72;

           hi[15][slot] = (t113 + t114);
           lo[ 0][slot] = (({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t113 - t114)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }));

  t115 = t73 + t74;
  t116 = t75 + t76;

  t32 = t115 + t116;

           hi[14][slot] = (t32);

  t118 = t78 + t79;
  t119 = t80 + t81;

  t58 = t118 + t119;

           hi[13][slot] = (t58);

  t121 = t83 + t84;
  t122 = t85 + t86;

  t67 = t121 + t122;

  t49 = (t67 * 2) - t32;

           hi[12][slot] = (t49);

  t125 = t89 + t90;
  t126 = t91 + t92;

  t93 = t125 + t126;

           hi[11][slot] = (t93);

  t128 = t94 + t95;
  t129 = t96 + t97;

  t98 = t128 + t129;

  t68 = (t98 * 2) - t49;

           hi[10][slot] = (t68);

  t132 = t100 + t101;
  t133 = t102 + t103;

  t104 = t132 + t133;

  t82 = (t104 * 2) - t58;

           hi[ 9][slot] = (t82);

  t136 = t106 + t107;
  t137 = t108 + t109;

  t110 = t136 + t137;

  t87 = (t110 * 2) - t67;

  t77 = (t87 * 2) - t68;

           hi[ 8][slot] = (t77);

  t141 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t69 - t70)), "r" ((((mad_fixed_t) (0x0ec835e8L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t142 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t71 - t72)), "r" ((((mad_fixed_t) (0x061f78aaL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t143 = t141 + t142;

           hi[ 7][slot] = (t143);
           lo[ 8][slot] =
      ((({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t141 - t142)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) - t143);

  t144 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t73 - t74)), "r" ((((mad_fixed_t) (0x0ec835e8L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t145 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t75 - t76)), "r" ((((mad_fixed_t) (0x061f78aaL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t146 = t144 + t145;

  t88 = (t146 * 2) - t77;

           hi[ 6][slot] = (t88);

  t148 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t78 - t79)), "r" ((((mad_fixed_t) (0x0ec835e8L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t149 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t80 - t81)), "r" ((((mad_fixed_t) (0x061f78aaL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t150 = t148 + t149;

  t105 = (t150 * 2) - t82;

           hi[ 5][slot] = (t105);

  t152 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t83 - t84)), "r" ((((mad_fixed_t) (0x0ec835e8L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t153 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t85 - t86)), "r" ((((mad_fixed_t) (0x061f78aaL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t154 = t152 + t153;

  t111 = (t154 * 2) - t87;

  t99 = (t111 * 2) - t88;

           hi[ 4][slot] = (t99);

  t157 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t89 - t90)), "r" ((((mad_fixed_t) (0x0ec835e8L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t158 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t91 - t92)), "r" ((((mad_fixed_t) (0x061f78aaL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t159 = t157 + t158;

  t127 = (t159 * 2) - t93;

           hi[ 3][slot] = (t127);

  t160 = (({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t125 - t126)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) - t127;

           lo[ 4][slot] = (t160);
           lo[12][slot] =
      ((((({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t157 - t158)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) - t159) * 2) - t160);

  t161 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t94 - t95)), "r" ((((mad_fixed_t) (0x0ec835e8L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t162 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t96 - t97)), "r" ((((mad_fixed_t) (0x061f78aaL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t163 = t161 + t162;

  t130 = (t163 * 2) - t98;

  t112 = (t130 * 2) - t99;

           hi[ 2][slot] = (t112);

  t164 = (({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t128 - t129)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) - t130;

  t166 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t100 - t101)), "r" ((((mad_fixed_t) (0x0ec835e8L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t167 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t102 - t103)), "r" ((((mad_fixed_t) (0x061f78aaL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t168 = t166 + t167;

  t134 = (t168 * 2) - t104;

  t120 = (t134 * 2) - t105;

           hi[ 1][slot] = (t120);

  t135 = (({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t118 - t119)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) - t120;

           lo[ 2][slot] = (t135);

  t169 = (({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t132 - t133)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) - t134;

  t151 = (t169 * 2) - t135;

           lo[ 6][slot] = (t151);

  t170 = (((({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t148 - t149)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) - t150) * 2) - t151;

           lo[10][slot] = (t170);
           lo[14][slot] =
      ((((((({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t166 - t167)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) - t168) * 2) - t169) * 2) - t170)
                                        ;

  t171 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t106 - t107)), "r" ((((mad_fixed_t) (0x0ec835e8L)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t172 = ({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t108 - t109)), "r" ((((mad_fixed_t) (0x061f78aaL)))), "M" (28), "M" (32 - 28) : "cc"); __result; });
  t173 = t171 + t172;

  t138 = (t173 * 2) - t110;

  t123 = (t138 * 2) - t111;

  t139 = (({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t121 - t122)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) - t123;

  t117 = (t123 * 2) - t112;

           hi[ 0][slot] = (t117);

  t124 = (({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t115 - t116)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) - t117;

           lo[ 1][slot] = (t124);

  t131 = (t139 * 2) - t124;

           lo[ 3][slot] = (t131);

  t140 = (t164 * 2) - t131;

           lo[ 5][slot] = (t140);

  t174 = (({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t136 - t137)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) - t138;

  t155 = (t174 * 2) - t139;

  t147 = (t155 * 2) - t140;

           lo[ 7][slot] = (t147);

  t156 = (((({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t144 - t145)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) - t146) * 2) - t147;

           lo[ 9][slot] = (t156);

  t175 = (((({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t152 - t153)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) - t154) * 2) - t155;

  t165 = (t175 * 2) - t156;

           lo[11][slot] = (t165);

  t176 = (((((({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t161 - t162)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) -
      t163) * 2) - t164) * 2) - t165;

           lo[13][slot] = (t176);
           lo[15][slot] =
      ((((((((({ mad_fixed64hi_t __hi; mad_fixed64lo_t __lo; mad_fixed_t __result; asm ("smull	%0, %1, %3, %4\n\t" "movs	%0, %0, lsr %5\n\t" "adc	%2, %0, %1, lsl %6" : "=&r" (__lo), "=&r" (__hi), "=r" (__result) : "%r" ((t171 - t172)), "r" ((((mad_fixed_t) (0x0b504f33L)))), "M" (28), "M" (32 - 28) : "cc"); __result; }) * 2) - t173) * 2) - t174) * 2) - t175) * 2) - t176)
                                                ;
# 512 "synth.c"
}
# 544 "synth.c"
static
mad_fixed_t const D[17][32] = {
# 1 "D.dat" 1
# 31 "D.dat"
  { (((mad_fixed_t) (0x00000000L)) >> 12) ,
    -(((mad_fixed_t) (0x0001d000L)) >> 12) ,
     (((mad_fixed_t) (0x000d5000L)) >> 12) ,
    -(((mad_fixed_t) (0x001cb000L)) >> 12) ,
     (((mad_fixed_t) (0x007f5000L)) >> 12) ,
    -(((mad_fixed_t) (0x01421000L)) >> 12) ,
     (((mad_fixed_t) (0x019ae000L)) >> 12) ,
    -(((mad_fixed_t) (0x09271000L)) >> 12) ,
     (((mad_fixed_t) (0x1251e000L)) >> 12) ,
     (((mad_fixed_t) (0x09271000L)) >> 12) ,
     (((mad_fixed_t) (0x019ae000L)) >> 12) ,
     (((mad_fixed_t) (0x01421000L)) >> 12) ,
     (((mad_fixed_t) (0x007f5000L)) >> 12) ,
     (((mad_fixed_t) (0x001cb000L)) >> 12) ,
     (((mad_fixed_t) (0x000d5000L)) >> 12) ,
     (((mad_fixed_t) (0x0001d000L)) >> 12) ,

     (((mad_fixed_t) (0x00000000L)) >> 12) ,
    -(((mad_fixed_t) (0x0001d000L)) >> 12) ,
     (((mad_fixed_t) (0x000d5000L)) >> 12) ,
    -(((mad_fixed_t) (0x001cb000L)) >> 12) ,
     (((mad_fixed_t) (0x007f5000L)) >> 12) ,
    -(((mad_fixed_t) (0x01421000L)) >> 12) ,
     (((mad_fixed_t) (0x019ae000L)) >> 12) ,
    -(((mad_fixed_t) (0x09271000L)) >> 12) ,
     (((mad_fixed_t) (0x1251e000L)) >> 12) ,
     (((mad_fixed_t) (0x09271000L)) >> 12) ,
     (((mad_fixed_t) (0x019ae000L)) >> 12) ,
     (((mad_fixed_t) (0x01421000L)) >> 12) ,
     (((mad_fixed_t) (0x007f5000L)) >> 12) ,
     (((mad_fixed_t) (0x001cb000L)) >> 12) ,
     (((mad_fixed_t) (0x000d5000L)) >> 12) ,
     (((mad_fixed_t) (0x0001d000L)) >> 12) },

  { -(((mad_fixed_t) (0x00001000L)) >> 12) ,
    -(((mad_fixed_t) (0x0001f000L)) >> 12) ,
     (((mad_fixed_t) (0x000da000L)) >> 12) ,
    -(((mad_fixed_t) (0x00207000L)) >> 12) ,
     (((mad_fixed_t) (0x007d0000L)) >> 12) ,
    -(((mad_fixed_t) (0x0158d000L)) >> 12) ,
     (((mad_fixed_t) (0x01747000L)) >> 12) ,
    -(((mad_fixed_t) (0x099a8000L)) >> 12) ,
     (((mad_fixed_t) (0x124f0000L)) >> 12) ,
     (((mad_fixed_t) (0x08b38000L)) >> 12) ,
     (((mad_fixed_t) (0x01bde000L)) >> 12) ,
     (((mad_fixed_t) (0x012b4000L)) >> 12) ,
     (((mad_fixed_t) (0x0080f000L)) >> 12) ,
     (((mad_fixed_t) (0x00191000L)) >> 12) ,
     (((mad_fixed_t) (0x000d0000L)) >> 12) ,
     (((mad_fixed_t) (0x0001a000L)) >> 12) ,

    -(((mad_fixed_t) (0x00001000L)) >> 12) ,
    -(((mad_fixed_t) (0x0001f000L)) >> 12) ,
     (((mad_fixed_t) (0x000da000L)) >> 12) ,
    -(((mad_fixed_t) (0x00207000L)) >> 12) ,
     (((mad_fixed_t) (0x007d0000L)) >> 12) ,
    -(((mad_fixed_t) (0x0158d000L)) >> 12) ,
     (((mad_fixed_t) (0x01747000L)) >> 12) ,
    -(((mad_fixed_t) (0x099a8000L)) >> 12) ,
     (((mad_fixed_t) (0x124f0000L)) >> 12) ,
     (((mad_fixed_t) (0x08b38000L)) >> 12) ,
     (((mad_fixed_t) (0x01bde000L)) >> 12) ,
     (((mad_fixed_t) (0x012b4000L)) >> 12) ,
     (((mad_fixed_t) (0x0080f000L)) >> 12) ,
     (((mad_fixed_t) (0x00191000L)) >> 12) ,
     (((mad_fixed_t) (0x000d0000L)) >> 12) ,
     (((mad_fixed_t) (0x0001a000L)) >> 12) },

  { -(((mad_fixed_t) (0x00001000L)) >> 12) ,
    -(((mad_fixed_t) (0x00023000L)) >> 12) ,
     (((mad_fixed_t) (0x000de000L)) >> 12) ,
    -(((mad_fixed_t) (0x00245000L)) >> 12) ,
     (((mad_fixed_t) (0x007a0000L)) >> 12) ,
    -(((mad_fixed_t) (0x016f7000L)) >> 12) ,
     (((mad_fixed_t) (0x014a8000L)) >> 12) ,
    -(((mad_fixed_t) (0x0a0d8000L)) >> 12) ,
     (((mad_fixed_t) (0x12468000L)) >> 12) ,
     (((mad_fixed_t) (0x083ff000L)) >> 12) ,
     (((mad_fixed_t) (0x01dd8000L)) >> 12) ,
     (((mad_fixed_t) (0x01149000L)) >> 12) ,
     (((mad_fixed_t) (0x00820000L)) >> 12) ,
     (((mad_fixed_t) (0x0015b000L)) >> 12) ,
     (((mad_fixed_t) (0x000ca000L)) >> 12) ,
     (((mad_fixed_t) (0x00018000L)) >> 12) ,

    -(((mad_fixed_t) (0x00001000L)) >> 12) ,
    -(((mad_fixed_t) (0x00023000L)) >> 12) ,
     (((mad_fixed_t) (0x000de000L)) >> 12) ,
    -(((mad_fixed_t) (0x00245000L)) >> 12) ,
     (((mad_fixed_t) (0x007a0000L)) >> 12) ,
    -(((mad_fixed_t) (0x016f7000L)) >> 12) ,
     (((mad_fixed_t) (0x014a8000L)) >> 12) ,
    -(((mad_fixed_t) (0x0a0d8000L)) >> 12) ,
     (((mad_fixed_t) (0x12468000L)) >> 12) ,
     (((mad_fixed_t) (0x083ff000L)) >> 12) ,
     (((mad_fixed_t) (0x01dd8000L)) >> 12) ,
     (((mad_fixed_t) (0x01149000L)) >> 12) ,
     (((mad_fixed_t) (0x00820000L)) >> 12) ,
     (((mad_fixed_t) (0x0015b000L)) >> 12) ,
     (((mad_fixed_t) (0x000ca000L)) >> 12) ,
     (((mad_fixed_t) (0x00018000L)) >> 12) },

  { -(((mad_fixed_t) (0x00001000L)) >> 12) ,
    -(((mad_fixed_t) (0x00026000L)) >> 12) ,
     (((mad_fixed_t) (0x000e1000L)) >> 12) ,
    -(((mad_fixed_t) (0x00285000L)) >> 12) ,
     (((mad_fixed_t) (0x00765000L)) >> 12) ,
    -(((mad_fixed_t) (0x0185d000L)) >> 12) ,
     (((mad_fixed_t) (0x011d1000L)) >> 12) ,
    -(((mad_fixed_t) (0x0a7fe000L)) >> 12) ,
     (((mad_fixed_t) (0x12386000L)) >> 12) ,
     (((mad_fixed_t) (0x07ccb000L)) >> 12) ,
     (((mad_fixed_t) (0x01f9c000L)) >> 12) ,
     (((mad_fixed_t) (0x00fdf000L)) >> 12) ,
     (((mad_fixed_t) (0x00827000L)) >> 12) ,
     (((mad_fixed_t) (0x00126000L)) >> 12) ,
     (((mad_fixed_t) (0x000c4000L)) >> 12) ,
     (((mad_fixed_t) (0x00015000L)) >> 12) ,

    -(((mad_fixed_t) (0x00001000L)) >> 12) ,
    -(((mad_fixed_t) (0x00026000L)) >> 12) ,
     (((mad_fixed_t) (0x000e1000L)) >> 12) ,
    -(((mad_fixed_t) (0x00285000L)) >> 12) ,
     (((mad_fixed_t) (0x00765000L)) >> 12) ,
    -(((mad_fixed_t) (0x0185d000L)) >> 12) ,
     (((mad_fixed_t) (0x011d1000L)) >> 12) ,
    -(((mad_fixed_t) (0x0a7fe000L)) >> 12) ,
     (((mad_fixed_t) (0x12386000L)) >> 12) ,
     (((mad_fixed_t) (0x07ccb000L)) >> 12) ,
     (((mad_fixed_t) (0x01f9c000L)) >> 12) ,
     (((mad_fixed_t) (0x00fdf000L)) >> 12) ,
     (((mad_fixed_t) (0x00827000L)) >> 12) ,
     (((mad_fixed_t) (0x00126000L)) >> 12) ,
     (((mad_fixed_t) (0x000c4000L)) >> 12) ,
     (((mad_fixed_t) (0x00015000L)) >> 12) },

  { -(((mad_fixed_t) (0x00001000L)) >> 12) ,
    -(((mad_fixed_t) (0x00029000L)) >> 12) ,
     (((mad_fixed_t) (0x000e3000L)) >> 12) ,
    -(((mad_fixed_t) (0x002c7000L)) >> 12) ,
     (((mad_fixed_t) (0x0071e000L)) >> 12) ,
    -(((mad_fixed_t) (0x019bd000L)) >> 12) ,
     (((mad_fixed_t) (0x00ec0000L)) >> 12) ,
    -(((mad_fixed_t) (0x0af15000L)) >> 12) ,
     (((mad_fixed_t) (0x12249000L)) >> 12) ,
     (((mad_fixed_t) (0x075a0000L)) >> 12) ,
     (((mad_fixed_t) (0x0212c000L)) >> 12) ,
     (((mad_fixed_t) (0x00e79000L)) >> 12) ,
     (((mad_fixed_t) (0x00825000L)) >> 12) ,
     (((mad_fixed_t) (0x000f4000L)) >> 12) ,
     (((mad_fixed_t) (0x000be000L)) >> 12) ,
     (((mad_fixed_t) (0x00013000L)) >> 12) ,

    -(((mad_fixed_t) (0x00001000L)) >> 12) ,
    -(((mad_fixed_t) (0x00029000L)) >> 12) ,
     (((mad_fixed_t) (0x000e3000L)) >> 12) ,
    -(((mad_fixed_t) (0x002c7000L)) >> 12) ,
     (((mad_fixed_t) (0x0071e000L)) >> 12) ,
    -(((mad_fixed_t) (0x019bd000L)) >> 12) ,
     (((mad_fixed_t) (0x00ec0000L)) >> 12) ,
    -(((mad_fixed_t) (0x0af15000L)) >> 12) ,
     (((mad_fixed_t) (0x12249000L)) >> 12) ,
     (((mad_fixed_t) (0x075a0000L)) >> 12) ,
     (((mad_fixed_t) (0x0212c000L)) >> 12) ,
     (((mad_fixed_t) (0x00e79000L)) >> 12) ,
     (((mad_fixed_t) (0x00825000L)) >> 12) ,
     (((mad_fixed_t) (0x000f4000L)) >> 12) ,
     (((mad_fixed_t) (0x000be000L)) >> 12) ,
     (((mad_fixed_t) (0x00013000L)) >> 12) },

  { -(((mad_fixed_t) (0x00001000L)) >> 12) ,
    -(((mad_fixed_t) (0x0002d000L)) >> 12) ,
     (((mad_fixed_t) (0x000e4000L)) >> 12) ,
    -(((mad_fixed_t) (0x0030b000L)) >> 12) ,
     (((mad_fixed_t) (0x006cb000L)) >> 12) ,
    -(((mad_fixed_t) (0x01b17000L)) >> 12) ,
     (((mad_fixed_t) (0x00b77000L)) >> 12) ,
    -(((mad_fixed_t) (0x0b619000L)) >> 12) ,
     (((mad_fixed_t) (0x120b4000L)) >> 12) ,
     (((mad_fixed_t) (0x06e81000L)) >> 12) ,
     (((mad_fixed_t) (0x02288000L)) >> 12) ,
     (((mad_fixed_t) (0x00d17000L)) >> 12) ,
     (((mad_fixed_t) (0x0081b000L)) >> 12) ,
     (((mad_fixed_t) (0x000c5000L)) >> 12) ,
     (((mad_fixed_t) (0x000b7000L)) >> 12) ,
     (((mad_fixed_t) (0x00011000L)) >> 12) ,

    -(((mad_fixed_t) (0x00001000L)) >> 12) ,
    -(((mad_fixed_t) (0x0002d000L)) >> 12) ,
     (((mad_fixed_t) (0x000e4000L)) >> 12) ,
    -(((mad_fixed_t) (0x0030b000L)) >> 12) ,
     (((mad_fixed_t) (0x006cb000L)) >> 12) ,
    -(((mad_fixed_t) (0x01b17000L)) >> 12) ,
     (((mad_fixed_t) (0x00b77000L)) >> 12) ,
    -(((mad_fixed_t) (0x0b619000L)) >> 12) ,
     (((mad_fixed_t) (0x120b4000L)) >> 12) ,
     (((mad_fixed_t) (0x06e81000L)) >> 12) ,
     (((mad_fixed_t) (0x02288000L)) >> 12) ,
     (((mad_fixed_t) (0x00d17000L)) >> 12) ,
     (((mad_fixed_t) (0x0081b000L)) >> 12) ,
     (((mad_fixed_t) (0x000c5000L)) >> 12) ,
     (((mad_fixed_t) (0x000b7000L)) >> 12) ,
     (((mad_fixed_t) (0x00011000L)) >> 12) },

  { -(((mad_fixed_t) (0x00001000L)) >> 12) ,
    -(((mad_fixed_t) (0x00031000L)) >> 12) ,
     (((mad_fixed_t) (0x000e4000L)) >> 12) ,
    -(((mad_fixed_t) (0x00350000L)) >> 12) ,
     (((mad_fixed_t) (0x0066c000L)) >> 12) ,
    -(((mad_fixed_t) (0x01c67000L)) >> 12) ,
     (((mad_fixed_t) (0x007f5000L)) >> 12) ,
    -(((mad_fixed_t) (0x0bd06000L)) >> 12) ,
     (((mad_fixed_t) (0x11ec7000L)) >> 12) ,
     (((mad_fixed_t) (0x06772000L)) >> 12) ,
     (((mad_fixed_t) (0x023b3000L)) >> 12) ,
     (((mad_fixed_t) (0x00bbc000L)) >> 12) ,
     (((mad_fixed_t) (0x00809000L)) >> 12) ,
     (((mad_fixed_t) (0x00099000L)) >> 12) ,
     (((mad_fixed_t) (0x000b0000L)) >> 12) ,
     (((mad_fixed_t) (0x00010000L)) >> 12) ,

    -(((mad_fixed_t) (0x00001000L)) >> 12) ,
    -(((mad_fixed_t) (0x00031000L)) >> 12) ,
     (((mad_fixed_t) (0x000e4000L)) >> 12) ,
    -(((mad_fixed_t) (0x00350000L)) >> 12) ,
     (((mad_fixed_t) (0x0066c000L)) >> 12) ,
    -(((mad_fixed_t) (0x01c67000L)) >> 12) ,
     (((mad_fixed_t) (0x007f5000L)) >> 12) ,
    -(((mad_fixed_t) (0x0bd06000L)) >> 12) ,
     (((mad_fixed_t) (0x11ec7000L)) >> 12) ,
     (((mad_fixed_t) (0x06772000L)) >> 12) ,
     (((mad_fixed_t) (0x023b3000L)) >> 12) ,
     (((mad_fixed_t) (0x00bbc000L)) >> 12) ,
     (((mad_fixed_t) (0x00809000L)) >> 12) ,
     (((mad_fixed_t) (0x00099000L)) >> 12) ,
     (((mad_fixed_t) (0x000b0000L)) >> 12) ,
     (((mad_fixed_t) (0x00010000L)) >> 12) },

  { -(((mad_fixed_t) (0x00002000L)) >> 12) ,
    -(((mad_fixed_t) (0x00035000L)) >> 12) ,
     (((mad_fixed_t) (0x000e3000L)) >> 12) ,
    -(((mad_fixed_t) (0x00397000L)) >> 12) ,
     (((mad_fixed_t) (0x005ff000L)) >> 12) ,
    -(((mad_fixed_t) (0x01dad000L)) >> 12) ,
     (((mad_fixed_t) (0x0043a000L)) >> 12) ,
    -(((mad_fixed_t) (0x0c3d9000L)) >> 12) ,
     (((mad_fixed_t) (0x11c83000L)) >> 12) ,
     (((mad_fixed_t) (0x06076000L)) >> 12) ,
     (((mad_fixed_t) (0x024ad000L)) >> 12) ,
     (((mad_fixed_t) (0x00a67000L)) >> 12) ,
     (((mad_fixed_t) (0x007f0000L)) >> 12) ,
     (((mad_fixed_t) (0x0006f000L)) >> 12) ,
     (((mad_fixed_t) (0x000a9000L)) >> 12) ,
     (((mad_fixed_t) (0x0000e000L)) >> 12) ,

    -(((mad_fixed_t) (0x00002000L)) >> 12) ,
    -(((mad_fixed_t) (0x00035000L)) >> 12) ,
     (((mad_fixed_t) (0x000e3000L)) >> 12) ,
    -(((mad_fixed_t) (0x00397000L)) >> 12) ,
     (((mad_fixed_t) (0x005ff000L)) >> 12) ,
    -(((mad_fixed_t) (0x01dad000L)) >> 12) ,
     (((mad_fixed_t) (0x0043a000L)) >> 12) ,
    -(((mad_fixed_t) (0x0c3d9000L)) >> 12) ,
     (((mad_fixed_t) (0x11c83000L)) >> 12) ,
     (((mad_fixed_t) (0x06076000L)) >> 12) ,
     (((mad_fixed_t) (0x024ad000L)) >> 12) ,
     (((mad_fixed_t) (0x00a67000L)) >> 12) ,
     (((mad_fixed_t) (0x007f0000L)) >> 12) ,
     (((mad_fixed_t) (0x0006f000L)) >> 12) ,
     (((mad_fixed_t) (0x000a9000L)) >> 12) ,
     (((mad_fixed_t) (0x0000e000L)) >> 12) },

  { -(((mad_fixed_t) (0x00002000L)) >> 12) ,
    -(((mad_fixed_t) (0x0003a000L)) >> 12) ,
     (((mad_fixed_t) (0x000e0000L)) >> 12) ,
    -(((mad_fixed_t) (0x003df000L)) >> 12) ,
     (((mad_fixed_t) (0x00586000L)) >> 12) ,
    -(((mad_fixed_t) (0x01ee6000L)) >> 12) ,
     (((mad_fixed_t) (0x00046000L)) >> 12) ,
    -(((mad_fixed_t) (0x0ca8d000L)) >> 12) ,
     (((mad_fixed_t) (0x119e9000L)) >> 12) ,
     (((mad_fixed_t) (0x05991000L)) >> 12) ,
     (((mad_fixed_t) (0x02578000L)) >> 12) ,
     (((mad_fixed_t) (0x0091a000L)) >> 12) ,
     (((mad_fixed_t) (0x007d1000L)) >> 12) ,
     (((mad_fixed_t) (0x00048000L)) >> 12) ,
     (((mad_fixed_t) (0x000a1000L)) >> 12) ,
     (((mad_fixed_t) (0x0000d000L)) >> 12) ,

    -(((mad_fixed_t) (0x00002000L)) >> 12) ,
    -(((mad_fixed_t) (0x0003a000L)) >> 12) ,
     (((mad_fixed_t) (0x000e0000L)) >> 12) ,
    -(((mad_fixed_t) (0x003df000L)) >> 12) ,
     (((mad_fixed_t) (0x00586000L)) >> 12) ,
    -(((mad_fixed_t) (0x01ee6000L)) >> 12) ,
     (((mad_fixed_t) (0x00046000L)) >> 12) ,
    -(((mad_fixed_t) (0x0ca8d000L)) >> 12) ,
     (((mad_fixed_t) (0x119e9000L)) >> 12) ,
     (((mad_fixed_t) (0x05991000L)) >> 12) ,
     (((mad_fixed_t) (0x02578000L)) >> 12) ,
     (((mad_fixed_t) (0x0091a000L)) >> 12) ,
     (((mad_fixed_t) (0x007d1000L)) >> 12) ,
     (((mad_fixed_t) (0x00048000L)) >> 12) ,
     (((mad_fixed_t) (0x000a1000L)) >> 12) ,
     (((mad_fixed_t) (0x0000d000L)) >> 12) },

  { -(((mad_fixed_t) (0x00002000L)) >> 12) ,
    -(((mad_fixed_t) (0x0003f000L)) >> 12) ,
     (((mad_fixed_t) (0x000dd000L)) >> 12) ,
    -(((mad_fixed_t) (0x00428000L)) >> 12) ,
     (((mad_fixed_t) (0x00500000L)) >> 12) ,
    -(((mad_fixed_t) (0x02011000L)) >> 12) ,
    -(((mad_fixed_t) (0x003e6000L)) >> 12) ,
    -(((mad_fixed_t) (0x0d11e000L)) >> 12) ,
     (((mad_fixed_t) (0x116fc000L)) >> 12) ,
     (((mad_fixed_t) (0x052c5000L)) >> 12) ,
     (((mad_fixed_t) (0x02616000L)) >> 12) ,
     (((mad_fixed_t) (0x007d6000L)) >> 12) ,
     (((mad_fixed_t) (0x007aa000L)) >> 12) ,
     (((mad_fixed_t) (0x00024000L)) >> 12) ,
     (((mad_fixed_t) (0x0009a000L)) >> 12) ,
     (((mad_fixed_t) (0x0000b000L)) >> 12) ,

    -(((mad_fixed_t) (0x00002000L)) >> 12) ,
    -(((mad_fixed_t) (0x0003f000L)) >> 12) ,
     (((mad_fixed_t) (0x000dd000L)) >> 12) ,
    -(((mad_fixed_t) (0x00428000L)) >> 12) ,
     (((mad_fixed_t) (0x00500000L)) >> 12) ,
    -(((mad_fixed_t) (0x02011000L)) >> 12) ,
    -(((mad_fixed_t) (0x003e6000L)) >> 12) ,
    -(((mad_fixed_t) (0x0d11e000L)) >> 12) ,
     (((mad_fixed_t) (0x116fc000L)) >> 12) ,
     (((mad_fixed_t) (0x052c5000L)) >> 12) ,
     (((mad_fixed_t) (0x02616000L)) >> 12) ,
     (((mad_fixed_t) (0x007d6000L)) >> 12) ,
     (((mad_fixed_t) (0x007aa000L)) >> 12) ,
     (((mad_fixed_t) (0x00024000L)) >> 12) ,
     (((mad_fixed_t) (0x0009a000L)) >> 12) ,
     (((mad_fixed_t) (0x0000b000L)) >> 12) },

  { -(((mad_fixed_t) (0x00002000L)) >> 12) ,
    -(((mad_fixed_t) (0x00044000L)) >> 12) ,
     (((mad_fixed_t) (0x000d7000L)) >> 12) ,
    -(((mad_fixed_t) (0x00471000L)) >> 12) ,
     (((mad_fixed_t) (0x0046b000L)) >> 12) ,
    -(((mad_fixed_t) (0x0212b000L)) >> 12) ,
    -(((mad_fixed_t) (0x0084a000L)) >> 12) ,
    -(((mad_fixed_t) (0x0d78a000L)) >> 12) ,
     (((mad_fixed_t) (0x113be000L)) >> 12) ,
     (((mad_fixed_t) (0x04c16000L)) >> 12) ,
     (((mad_fixed_t) (0x02687000L)) >> 12) ,
     (((mad_fixed_t) (0x0069c000L)) >> 12) ,
     (((mad_fixed_t) (0x0077f000L)) >> 12) ,
     (((mad_fixed_t) (0x00002000L)) >> 12) ,
     (((mad_fixed_t) (0x00093000L)) >> 12) ,
     (((mad_fixed_t) (0x0000a000L)) >> 12) ,

    -(((mad_fixed_t) (0x00002000L)) >> 12) ,
    -(((mad_fixed_t) (0x00044000L)) >> 12) ,
     (((mad_fixed_t) (0x000d7000L)) >> 12) ,
    -(((mad_fixed_t) (0x00471000L)) >> 12) ,
     (((mad_fixed_t) (0x0046b000L)) >> 12) ,
    -(((mad_fixed_t) (0x0212b000L)) >> 12) ,
    -(((mad_fixed_t) (0x0084a000L)) >> 12) ,
    -(((mad_fixed_t) (0x0d78a000L)) >> 12) ,
     (((mad_fixed_t) (0x113be000L)) >> 12) ,
     (((mad_fixed_t) (0x04c16000L)) >> 12) ,
     (((mad_fixed_t) (0x02687000L)) >> 12) ,
     (((mad_fixed_t) (0x0069c000L)) >> 12) ,
     (((mad_fixed_t) (0x0077f000L)) >> 12) ,
     (((mad_fixed_t) (0x00002000L)) >> 12) ,
     (((mad_fixed_t) (0x00093000L)) >> 12) ,
     (((mad_fixed_t) (0x0000a000L)) >> 12) },

  { -(((mad_fixed_t) (0x00003000L)) >> 12) ,
    -(((mad_fixed_t) (0x00049000L)) >> 12) ,
     (((mad_fixed_t) (0x000d0000L)) >> 12) ,
    -(((mad_fixed_t) (0x004ba000L)) >> 12) ,
     (((mad_fixed_t) (0x003ca000L)) >> 12) ,
    -(((mad_fixed_t) (0x02233000L)) >> 12) ,
    -(((mad_fixed_t) (0x00ce4000L)) >> 12) ,
    -(((mad_fixed_t) (0x0ddca000L)) >> 12) ,
     (((mad_fixed_t) (0x1102f000L)) >> 12) ,
     (((mad_fixed_t) (0x04587000L)) >> 12) ,
     (((mad_fixed_t) (0x026cf000L)) >> 12) ,
     (((mad_fixed_t) (0x0056c000L)) >> 12) ,
     (((mad_fixed_t) (0x0074e000L)) >> 12) ,
    -(((mad_fixed_t) (0x0001d000L)) >> 12) ,
     (((mad_fixed_t) (0x0008b000L)) >> 12) ,
     (((mad_fixed_t) (0x00009000L)) >> 12) ,

    -(((mad_fixed_t) (0x00003000L)) >> 12) ,
    -(((mad_fixed_t) (0x00049000L)) >> 12) ,
     (((mad_fixed_t) (0x000d0000L)) >> 12) ,
    -(((mad_fixed_t) (0x004ba000L)) >> 12) ,
     (((mad_fixed_t) (0x003ca000L)) >> 12) ,
    -(((mad_fixed_t) (0x02233000L)) >> 12) ,
    -(((mad_fixed_t) (0x00ce4000L)) >> 12) ,
    -(((mad_fixed_t) (0x0ddca000L)) >> 12) ,
     (((mad_fixed_t) (0x1102f000L)) >> 12) ,
     (((mad_fixed_t) (0x04587000L)) >> 12) ,
     (((mad_fixed_t) (0x026cf000L)) >> 12) ,
     (((mad_fixed_t) (0x0056c000L)) >> 12) ,
     (((mad_fixed_t) (0x0074e000L)) >> 12) ,
    -(((mad_fixed_t) (0x0001d000L)) >> 12) ,
     (((mad_fixed_t) (0x0008b000L)) >> 12) ,
     (((mad_fixed_t) (0x00009000L)) >> 12) },

  { -(((mad_fixed_t) (0x00003000L)) >> 12) ,
    -(((mad_fixed_t) (0x0004f000L)) >> 12) ,
     (((mad_fixed_t) (0x000c8000L)) >> 12) ,
    -(((mad_fixed_t) (0x00503000L)) >> 12) ,
     (((mad_fixed_t) (0x0031a000L)) >> 12) ,
    -(((mad_fixed_t) (0x02326000L)) >> 12) ,
    -(((mad_fixed_t) (0x011b5000L)) >> 12) ,
    -(((mad_fixed_t) (0x0e3dd000L)) >> 12) ,
     (((mad_fixed_t) (0x10c54000L)) >> 12) ,
     (((mad_fixed_t) (0x03f1b000L)) >> 12) ,
     (((mad_fixed_t) (0x026ee000L)) >> 12) ,
     (((mad_fixed_t) (0x00447000L)) >> 12) ,
     (((mad_fixed_t) (0x00719000L)) >> 12) ,
    -(((mad_fixed_t) (0x00039000L)) >> 12) ,
     (((mad_fixed_t) (0x00084000L)) >> 12) ,
     (((mad_fixed_t) (0x00008000L)) >> 12) ,

    -(((mad_fixed_t) (0x00003000L)) >> 12) ,
    -(((mad_fixed_t) (0x0004f000L)) >> 12) ,
     (((mad_fixed_t) (0x000c8000L)) >> 12) ,
    -(((mad_fixed_t) (0x00503000L)) >> 12) ,
     (((mad_fixed_t) (0x0031a000L)) >> 12) ,
    -(((mad_fixed_t) (0x02326000L)) >> 12) ,
    -(((mad_fixed_t) (0x011b5000L)) >> 12) ,
    -(((mad_fixed_t) (0x0e3dd000L)) >> 12) ,
     (((mad_fixed_t) (0x10c54000L)) >> 12) ,
     (((mad_fixed_t) (0x03f1b000L)) >> 12) ,
     (((mad_fixed_t) (0x026ee000L)) >> 12) ,
     (((mad_fixed_t) (0x00447000L)) >> 12) ,
     (((mad_fixed_t) (0x00719000L)) >> 12) ,
    -(((mad_fixed_t) (0x00039000L)) >> 12) ,
     (((mad_fixed_t) (0x00084000L)) >> 12) ,
     (((mad_fixed_t) (0x00008000L)) >> 12) },

  { -(((mad_fixed_t) (0x00004000L)) >> 12) ,
    -(((mad_fixed_t) (0x00055000L)) >> 12) ,
     (((mad_fixed_t) (0x000bd000L)) >> 12) ,
    -(((mad_fixed_t) (0x0054c000L)) >> 12) ,
     (((mad_fixed_t) (0x0025d000L)) >> 12) ,
    -(((mad_fixed_t) (0x02403000L)) >> 12) ,
    -(((mad_fixed_t) (0x016ba000L)) >> 12) ,
    -(((mad_fixed_t) (0x0e9be000L)) >> 12) ,
     (((mad_fixed_t) (0x1082d000L)) >> 12) ,
     (((mad_fixed_t) (0x038d4000L)) >> 12) ,
     (((mad_fixed_t) (0x026e7000L)) >> 12) ,
     (((mad_fixed_t) (0x0032e000L)) >> 12) ,
     (((mad_fixed_t) (0x006df000L)) >> 12) ,
    -(((mad_fixed_t) (0x00053000L)) >> 12) ,
     (((mad_fixed_t) (0x0007d000L)) >> 12) ,
     (((mad_fixed_t) (0x00007000L)) >> 12) ,

    -(((mad_fixed_t) (0x00004000L)) >> 12) ,
    -(((mad_fixed_t) (0x00055000L)) >> 12) ,
     (((mad_fixed_t) (0x000bd000L)) >> 12) ,
    -(((mad_fixed_t) (0x0054c000L)) >> 12) ,
     (((mad_fixed_t) (0x0025d000L)) >> 12) ,
    -(((mad_fixed_t) (0x02403000L)) >> 12) ,
    -(((mad_fixed_t) (0x016ba000L)) >> 12) ,
    -(((mad_fixed_t) (0x0e9be000L)) >> 12) ,
     (((mad_fixed_t) (0x1082d000L)) >> 12) ,
     (((mad_fixed_t) (0x038d4000L)) >> 12) ,
     (((mad_fixed_t) (0x026e7000L)) >> 12) ,
     (((mad_fixed_t) (0x0032e000L)) >> 12) ,
     (((mad_fixed_t) (0x006df000L)) >> 12) ,
    -(((mad_fixed_t) (0x00053000L)) >> 12) ,
     (((mad_fixed_t) (0x0007d000L)) >> 12) ,
     (((mad_fixed_t) (0x00007000L)) >> 12) },

  { -(((mad_fixed_t) (0x00004000L)) >> 12) ,
    -(((mad_fixed_t) (0x0005b000L)) >> 12) ,
     (((mad_fixed_t) (0x000b1000L)) >> 12) ,
    -(((mad_fixed_t) (0x00594000L)) >> 12) ,
     (((mad_fixed_t) (0x00192000L)) >> 12) ,
    -(((mad_fixed_t) (0x024c8000L)) >> 12) ,
    -(((mad_fixed_t) (0x01bf2000L)) >> 12) ,
    -(((mad_fixed_t) (0x0ef69000L)) >> 12) ,
     (((mad_fixed_t) (0x103be000L)) >> 12) ,
     (((mad_fixed_t) (0x032b4000L)) >> 12) ,
     (((mad_fixed_t) (0x026bc000L)) >> 12) ,
     (((mad_fixed_t) (0x00221000L)) >> 12) ,
     (((mad_fixed_t) (0x006a2000L)) >> 12) ,
    -(((mad_fixed_t) (0x0006a000L)) >> 12) ,
     (((mad_fixed_t) (0x00075000L)) >> 12) ,
     (((mad_fixed_t) (0x00007000L)) >> 12) ,

    -(((mad_fixed_t) (0x00004000L)) >> 12) ,
    -(((mad_fixed_t) (0x0005b000L)) >> 12) ,
     (((mad_fixed_t) (0x000b1000L)) >> 12) ,
    -(((mad_fixed_t) (0x00594000L)) >> 12) ,
     (((mad_fixed_t) (0x00192000L)) >> 12) ,
    -(((mad_fixed_t) (0x024c8000L)) >> 12) ,
    -(((mad_fixed_t) (0x01bf2000L)) >> 12) ,
    -(((mad_fixed_t) (0x0ef69000L)) >> 12) ,
     (((mad_fixed_t) (0x103be000L)) >> 12) ,
     (((mad_fixed_t) (0x032b4000L)) >> 12) ,
     (((mad_fixed_t) (0x026bc000L)) >> 12) ,
     (((mad_fixed_t) (0x00221000L)) >> 12) ,
     (((mad_fixed_t) (0x006a2000L)) >> 12) ,
    -(((mad_fixed_t) (0x0006a000L)) >> 12) ,
     (((mad_fixed_t) (0x00075000L)) >> 12) ,
     (((mad_fixed_t) (0x00007000L)) >> 12) },

  { -(((mad_fixed_t) (0x00005000L)) >> 12) ,
    -(((mad_fixed_t) (0x00061000L)) >> 12) ,
     (((mad_fixed_t) (0x000a3000L)) >> 12) ,
    -(((mad_fixed_t) (0x005da000L)) >> 12) ,
     (((mad_fixed_t) (0x000b9000L)) >> 12) ,
    -(((mad_fixed_t) (0x02571000L)) >> 12) ,
    -(((mad_fixed_t) (0x0215c000L)) >> 12) ,
    -(((mad_fixed_t) (0x0f4dc000L)) >> 12) ,
     (((mad_fixed_t) (0x0ff0a000L)) >> 12) ,
     (((mad_fixed_t) (0x02cbf000L)) >> 12) ,
     (((mad_fixed_t) (0x0266e000L)) >> 12) ,
     (((mad_fixed_t) (0x00120000L)) >> 12) ,
     (((mad_fixed_t) (0x00662000L)) >> 12) ,
    -(((mad_fixed_t) (0x0007f000L)) >> 12) ,
     (((mad_fixed_t) (0x0006f000L)) >> 12) ,
     (((mad_fixed_t) (0x00006000L)) >> 12) ,

    -(((mad_fixed_t) (0x00005000L)) >> 12) ,
    -(((mad_fixed_t) (0x00061000L)) >> 12) ,
     (((mad_fixed_t) (0x000a3000L)) >> 12) ,
    -(((mad_fixed_t) (0x005da000L)) >> 12) ,
     (((mad_fixed_t) (0x000b9000L)) >> 12) ,
    -(((mad_fixed_t) (0x02571000L)) >> 12) ,
    -(((mad_fixed_t) (0x0215c000L)) >> 12) ,
    -(((mad_fixed_t) (0x0f4dc000L)) >> 12) ,
     (((mad_fixed_t) (0x0ff0a000L)) >> 12) ,
     (((mad_fixed_t) (0x02cbf000L)) >> 12) ,
     (((mad_fixed_t) (0x0266e000L)) >> 12) ,
     (((mad_fixed_t) (0x00120000L)) >> 12) ,
     (((mad_fixed_t) (0x00662000L)) >> 12) ,
    -(((mad_fixed_t) (0x0007f000L)) >> 12) ,
     (((mad_fixed_t) (0x0006f000L)) >> 12) ,
     (((mad_fixed_t) (0x00006000L)) >> 12) },

  { -(((mad_fixed_t) (0x00005000L)) >> 12) ,
    -(((mad_fixed_t) (0x00068000L)) >> 12) ,
     (((mad_fixed_t) (0x00092000L)) >> 12) ,
    -(((mad_fixed_t) (0x0061f000L)) >> 12) ,
    -(((mad_fixed_t) (0x0002d000L)) >> 12) ,
    -(((mad_fixed_t) (0x025ff000L)) >> 12) ,
    -(((mad_fixed_t) (0x026f7000L)) >> 12) ,
    -(((mad_fixed_t) (0x0fa13000L)) >> 12) ,
     (((mad_fixed_t) (0x0fa13000L)) >> 12) ,
     (((mad_fixed_t) (0x026f7000L)) >> 12) ,
     (((mad_fixed_t) (0x025ff000L)) >> 12) ,
     (((mad_fixed_t) (0x0002d000L)) >> 12) ,
     (((mad_fixed_t) (0x0061f000L)) >> 12) ,
    -(((mad_fixed_t) (0x00092000L)) >> 12) ,
     (((mad_fixed_t) (0x00068000L)) >> 12) ,
     (((mad_fixed_t) (0x00005000L)) >> 12) ,

    -(((mad_fixed_t) (0x00005000L)) >> 12) ,
    -(((mad_fixed_t) (0x00068000L)) >> 12) ,
     (((mad_fixed_t) (0x00092000L)) >> 12) ,
    -(((mad_fixed_t) (0x0061f000L)) >> 12) ,
    -(((mad_fixed_t) (0x0002d000L)) >> 12) ,
    -(((mad_fixed_t) (0x025ff000L)) >> 12) ,
    -(((mad_fixed_t) (0x026f7000L)) >> 12) ,
    -(((mad_fixed_t) (0x0fa13000L)) >> 12) ,
     (((mad_fixed_t) (0x0fa13000L)) >> 12) ,
     (((mad_fixed_t) (0x026f7000L)) >> 12) ,
     (((mad_fixed_t) (0x025ff000L)) >> 12) ,
     (((mad_fixed_t) (0x0002d000L)) >> 12) ,
     (((mad_fixed_t) (0x0061f000L)) >> 12) ,
    -(((mad_fixed_t) (0x00092000L)) >> 12) ,
     (((mad_fixed_t) (0x00068000L)) >> 12) ,
     (((mad_fixed_t) (0x00005000L)) >> 12) }
# 547 "synth.c" 2
};
# 557 "synth.c"
static
void synth_full(struct mad_synth *synth, struct mad_frame const *frame,
  unsigned int nch, unsigned int ns)
{
  unsigned int phase, ch, s, sb, pe, po;
  mad_fixed_t *pcm1, *pcm2, (*filter)[2][2][16][8];
  mad_fixed_t const (*sbsample)[36][32];
  register mad_fixed_t (*fe)[8], (*fx)[8], (*fo)[8];
  register mad_fixed_t const (*Dptr)[32], *ptr;
  register mad_fixed64hi_t hi;
  register mad_fixed64lo_t lo;

  for (ch = 0; ch < nch; ++ch) {
    sbsample = &frame->sbsample[ch];
    filter = &synth->filter[ch];
    phase = synth->phase;
    pcm1 = synth->pcm.samples[ch];

    for (s = 0; s < ns; ++s) {
      dct32((*sbsample)[s], phase >> 1,
     (*filter)[0][phase & 1], (*filter)[1][phase & 1]);

      pe = phase & ~1;
      po = ((phase - 1) & 0xf) | 1;



      fe = &(*filter)[0][ phase & 1][0];
      fx = &(*filter)[0][~phase & 1][0];
      fo = &(*filter)[1][~phase & 1][0];

      Dptr = &D[0];

      ptr = *Dptr + po;
      asm ("smull	%0, %1, %2, %3" : "=&r" (((lo))), "=&r" (((hi))) : "%r" ((((*fx)[0]))), "r" (((ptr[ 0]))));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fx)[1])), "r" ((ptr[14])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fx)[2])), "r" ((ptr[12])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fx)[3])), "r" ((ptr[10])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fx)[4])), "r" ((ptr[ 8])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fx)[5])), "r" ((ptr[ 6])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fx)[6])), "r" ((ptr[ 4])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fx)[7])), "r" ((ptr[ 2])));
      asm ("rsbs	%0, %2, #0\n\t" "rsc	%1, %3, #0" : "=r" ((lo)), "=r" ((hi)) : "0" ((lo)), "1" ((hi)) : "cc");

      ptr = *Dptr + pe;
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[0])), "r" ((ptr[ 0])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[1])), "r" ((ptr[14])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[2])), "r" ((ptr[12])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[3])), "r" ((ptr[10])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[4])), "r" ((ptr[ 8])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[5])), "r" ((ptr[ 6])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[6])), "r" ((ptr[ 4])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[7])), "r" ((ptr[ 2])));

      *pcm1++ = (({ mad_fixed_t __result; asm ("movs	%0, %1, lsr %3\n\t" "adc	%0, %0, %2, lsl %4" : "=&r" (__result) : "r" (((lo))), "r" (((hi))), "M" ((28 - 12)), "M" (32 - (28 - 12)) : "cc"); __result; }));

      pcm2 = pcm1 + 30;

      for (sb = 1; sb < 16; ++sb) {
 ++fe;
 ++Dptr;



 ptr = *Dptr + po;
 asm ("smull	%0, %1, %2, %3" : "=&r" (((lo))), "=&r" (((hi))) : "%r" ((((*fo)[0]))), "r" (((ptr[ 0]))));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[1])), "r" ((ptr[14])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[2])), "r" ((ptr[12])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[3])), "r" ((ptr[10])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[4])), "r" ((ptr[ 8])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[5])), "r" ((ptr[ 6])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[6])), "r" ((ptr[ 4])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[7])), "r" ((ptr[ 2])));
 asm ("rsbs	%0, %2, #0\n\t" "rsc	%1, %3, #0" : "=r" ((lo)), "=r" ((hi)) : "0" ((lo)), "1" ((hi)) : "cc");

 ptr = *Dptr + pe;
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[7])), "r" ((ptr[ 2])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[6])), "r" ((ptr[ 4])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[5])), "r" ((ptr[ 6])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[4])), "r" ((ptr[ 8])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[3])), "r" ((ptr[10])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[2])), "r" ((ptr[12])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[1])), "r" ((ptr[14])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[0])), "r" ((ptr[ 0])));

 *pcm1++ = (({ mad_fixed_t __result; asm ("movs	%0, %1, lsr %3\n\t" "adc	%0, %0, %2, lsl %4" : "=&r" (__result) : "r" (((lo))), "r" (((hi))), "M" ((28 - 12)), "M" (32 - (28 - 12)) : "cc"); __result; }));

 ptr = *Dptr - pe;
 asm ("smull	%0, %1, %2, %3" : "=&r" (((lo))), "=&r" (((hi))) : "%r" ((((*fe)[0]))), "r" (((ptr[31 - 16]))));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[1])), "r" ((ptr[31 - 14])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[2])), "r" ((ptr[31 - 12])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[3])), "r" ((ptr[31 - 10])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[4])), "r" ((ptr[31 - 8])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[5])), "r" ((ptr[31 - 6])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[6])), "r" ((ptr[31 - 4])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[7])), "r" ((ptr[31 - 2])));

 ptr = *Dptr - po;
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[7])), "r" ((ptr[31 - 2])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[6])), "r" ((ptr[31 - 4])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[5])), "r" ((ptr[31 - 6])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[4])), "r" ((ptr[31 - 8])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[3])), "r" ((ptr[31 - 10])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[2])), "r" ((ptr[31 - 12])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[1])), "r" ((ptr[31 - 14])));
 asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[0])), "r" ((ptr[31 - 16])));

 *pcm2-- = (({ mad_fixed_t __result; asm ("movs	%0, %1, lsr %3\n\t" "adc	%0, %0, %2, lsl %4" : "=&r" (__result) : "r" (((lo))), "r" (((hi))), "M" ((28 - 12)), "M" (32 - (28 - 12)) : "cc"); __result; }));

 ++fo;
      }

      ++Dptr;

      ptr = *Dptr + po;
      asm ("smull	%0, %1, %2, %3" : "=&r" (((lo))), "=&r" (((hi))) : "%r" ((((*fo)[0]))), "r" (((ptr[ 0]))));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[1])), "r" ((ptr[14])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[2])), "r" ((ptr[12])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[3])), "r" ((ptr[10])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[4])), "r" ((ptr[ 8])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[5])), "r" ((ptr[ 6])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[6])), "r" ((ptr[ 4])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[7])), "r" ((ptr[ 2])));

      *pcm1 = (-({ mad_fixed_t __result; asm ("movs	%0, %1, lsr %3\n\t" "adc	%0, %0, %2, lsl %4" : "=&r" (__result) : "r" (((lo))), "r" (((hi))), "M" ((28 - 12)), "M" (32 - (28 - 12)) : "cc"); __result; }));
      pcm1 += 16;

      phase = (phase + 1) % 16;
    }
  }
}






static
void synth_half(struct mad_synth *synth, struct mad_frame const *frame,
  unsigned int nch, unsigned int ns)
{
  unsigned int phase, ch, s, sb, pe, po;
  mad_fixed_t *pcm1, *pcm2, (*filter)[2][2][16][8];
  mad_fixed_t const (*sbsample)[36][32];
  register mad_fixed_t (*fe)[8], (*fx)[8], (*fo)[8];
  register mad_fixed_t const (*Dptr)[32], *ptr;
  register mad_fixed64hi_t hi;
  register mad_fixed64lo_t lo;

  for (ch = 0; ch < nch; ++ch) {
    sbsample = &frame->sbsample[ch];
    filter = &synth->filter[ch];
    phase = synth->phase;
    pcm1 = synth->pcm.samples[ch];

    for (s = 0; s < ns; ++s) {
      dct32((*sbsample)[s], phase >> 1,
     (*filter)[0][phase & 1], (*filter)[1][phase & 1]);

      pe = phase & ~1;
      po = ((phase - 1) & 0xf) | 1;



      fe = &(*filter)[0][ phase & 1][0];
      fx = &(*filter)[0][~phase & 1][0];
      fo = &(*filter)[1][~phase & 1][0];

      Dptr = &D[0];

      ptr = *Dptr + po;
      asm ("smull	%0, %1, %2, %3" : "=&r" (((lo))), "=&r" (((hi))) : "%r" ((((*fx)[0]))), "r" (((ptr[ 0]))));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fx)[1])), "r" ((ptr[14])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fx)[2])), "r" ((ptr[12])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fx)[3])), "r" ((ptr[10])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fx)[4])), "r" ((ptr[ 8])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fx)[5])), "r" ((ptr[ 6])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fx)[6])), "r" ((ptr[ 4])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fx)[7])), "r" ((ptr[ 2])));
      asm ("rsbs	%0, %2, #0\n\t" "rsc	%1, %3, #0" : "=r" ((lo)), "=r" ((hi)) : "0" ((lo)), "1" ((hi)) : "cc");

      ptr = *Dptr + pe;
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[0])), "r" ((ptr[ 0])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[1])), "r" ((ptr[14])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[2])), "r" ((ptr[12])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[3])), "r" ((ptr[10])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[4])), "r" ((ptr[ 8])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[5])), "r" ((ptr[ 6])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[6])), "r" ((ptr[ 4])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[7])), "r" ((ptr[ 2])));

      *pcm1++ = (({ mad_fixed_t __result; asm ("movs	%0, %1, lsr %3\n\t" "adc	%0, %0, %2, lsl %4" : "=&r" (__result) : "r" (((lo))), "r" (((hi))), "M" ((28 - 12)), "M" (32 - (28 - 12)) : "cc"); __result; }));

      pcm2 = pcm1 + 14;

      for (sb = 1; sb < 16; ++sb) {
 ++fe;
 ++Dptr;



 if (!(sb & 1)) {
   ptr = *Dptr + po;
   asm ("smull	%0, %1, %2, %3" : "=&r" (((lo))), "=&r" (((hi))) : "%r" ((((*fo)[0]))), "r" (((ptr[ 0]))));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[1])), "r" ((ptr[14])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[2])), "r" ((ptr[12])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[3])), "r" ((ptr[10])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[4])), "r" ((ptr[ 8])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[5])), "r" ((ptr[ 6])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[6])), "r" ((ptr[ 4])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[7])), "r" ((ptr[ 2])));
   asm ("rsbs	%0, %2, #0\n\t" "rsc	%1, %3, #0" : "=r" ((lo)), "=r" ((hi)) : "0" ((lo)), "1" ((hi)) : "cc");

   ptr = *Dptr + pe;
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[7])), "r" ((ptr[ 2])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[6])), "r" ((ptr[ 4])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[5])), "r" ((ptr[ 6])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[4])), "r" ((ptr[ 8])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[3])), "r" ((ptr[10])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[2])), "r" ((ptr[12])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[1])), "r" ((ptr[14])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[0])), "r" ((ptr[ 0])));

   *pcm1++ = (({ mad_fixed_t __result; asm ("movs	%0, %1, lsr %3\n\t" "adc	%0, %0, %2, lsl %4" : "=&r" (__result) : "r" (((lo))), "r" (((hi))), "M" ((28 - 12)), "M" (32 - (28 - 12)) : "cc"); __result; }));

   ptr = *Dptr - po;
   asm ("smull	%0, %1, %2, %3" : "=&r" (((lo))), "=&r" (((hi))) : "%r" ((((*fo)[7]))), "r" (((ptr[31 - 2]))));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[6])), "r" ((ptr[31 - 4])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[5])), "r" ((ptr[31 - 6])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[4])), "r" ((ptr[31 - 8])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[3])), "r" ((ptr[31 - 10])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[2])), "r" ((ptr[31 - 12])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[1])), "r" ((ptr[31 - 14])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[0])), "r" ((ptr[31 - 16])));

   ptr = *Dptr - pe;
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[0])), "r" ((ptr[31 - 16])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[1])), "r" ((ptr[31 - 14])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[2])), "r" ((ptr[31 - 12])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[3])), "r" ((ptr[31 - 10])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[4])), "r" ((ptr[31 - 8])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[5])), "r" ((ptr[31 - 6])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[6])), "r" ((ptr[31 - 4])));
   asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fe)[7])), "r" ((ptr[31 - 2])));

   *pcm2-- = (({ mad_fixed_t __result; asm ("movs	%0, %1, lsr %3\n\t" "adc	%0, %0, %2, lsl %4" : "=&r" (__result) : "r" (((lo))), "r" (((hi))), "M" ((28 - 12)), "M" (32 - (28 - 12)) : "cc"); __result; }));
 }

 ++fo;
      }

      ++Dptr;

      ptr = *Dptr + po;
      asm ("smull	%0, %1, %2, %3" : "=&r" (((lo))), "=&r" (((hi))) : "%r" ((((*fo)[0]))), "r" (((ptr[ 0]))));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[1])), "r" ((ptr[14])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[2])), "r" ((ptr[12])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[3])), "r" ((ptr[10])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[4])), "r" ((ptr[ 8])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[5])), "r" ((ptr[ 6])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[6])), "r" ((ptr[ 4])));
      asm ("smlal	%0, %1, %2, %3" : "+r" ((lo)), "+r" ((hi)) : "%r" (((*fo)[7])), "r" ((ptr[ 2])));

      *pcm1 = (-({ mad_fixed_t __result; asm ("movs	%0, %1, lsr %3\n\t" "adc	%0, %0, %2, lsl %4" : "=&r" (__result) : "r" (((lo))), "r" (((hi))), "M" ((28 - 12)), "M" (32 - (28 - 12)) : "cc"); __result; }));
      pcm1 += 8;

      phase = (phase + 1) % 16;
    }
  }
}





void mad_synth_frame(struct mad_synth *synth, struct mad_frame const *frame)
{
  unsigned int nch, ns;
  void (*synth_frame)(struct mad_synth *, struct mad_frame const *,
        unsigned int, unsigned int);

  nch = ((&frame->header)->mode ? 2 : 1);
  ns = ((&frame->header)->layer == MAD_LAYER_I ? 12 : (((&frame->header)->layer == MAD_LAYER_III && ((&frame->header)->flags & MAD_FLAG_LSF_EXT)) ? 18 : 36));

  synth->pcm.samplerate = frame->header.samplerate;
  synth->pcm.channels = nch;
  synth->pcm.length = 32 * ns;

  synth_frame = synth_full;

  if (frame->options & MAD_OPTION_HALFSAMPLERATE) {
    synth->pcm.samplerate /= 2;
    synth->pcm.length /= 2;

    synth_frame = synth_half;
  }

  synth_frame(synth, frame, nch, ns);

  synth->phase = (synth->phase + ns) % 16;
}
