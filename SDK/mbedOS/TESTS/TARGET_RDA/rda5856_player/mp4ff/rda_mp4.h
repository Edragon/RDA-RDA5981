#ifndef RDA_MP4_H
#define RDA_MP4_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mp4ff_int_types.h"
#include <stdlib.h>
#include "rda_log.h"

#define INLINE 			__inline
//#define NULL            ((void *) 0)

#define SBR_DEC
//#define PS_DEC

#define MAIN       1
#define LC         2
#define SSR        3
#define LTP        4
#define HE_AAC     5
#define LD        23
#define ER_LC     17
#define ER_LTP    19
#define DRM_ER_LC 27 /* special object type for DRM */

    /* header types */
#define RAW        0
#define ADIF       1
#define ADTS       2
#define LATM       3

    /* SBR signalling */
#define NO_SBR           0
#define SBR_UPSAMPLED    1
#define SBR_DOWNSAMPLED  2
#define NO_SBR_UPSAMPLED 3

    /* First object type that has ER */
#define ER_OBJECT_START 17

    typedef struct _bitfile {
        /* bit input */
        uint32_t bufa;
        uint32_t bufb;
        uint32_t bits_left;
        uint32_t buffer_size; /* size of the buffer in bytes */
        uint32_t bytes_left;
        uint8_t error;
        uint32_t *tail;
        uint32_t *start;
        const void *buffer;
    } bitfile;

    typedef struct mp4AudioSpecificConfig {
        /* Audio Specific Info */
        unsigned char objectTypeIndex;
        unsigned char samplingFrequencyIndex;
        unsigned long samplingFrequency;
        unsigned char channelsConfiguration;

        /* GA Specific Info */
        unsigned char frameLengthFlag;
        unsigned char dependsOnCoreCoder;
        unsigned short coreCoderDelay;
        unsigned char extensionFlag;
        unsigned char aacSectionDataResilienceFlag;
        unsigned char aacScalefactorDataResilienceFlag;
        unsigned char aacSpectralDataResilienceFlag;
        unsigned char epConfig;

        char sbr_present_flag;
        char ps_present_flag;
        char forceUpSampling;
        char downSampledSBR;
    } mp4AudioSpecificConfig;

    /* circumvent memory alignment errors on ARM */
    static INLINE uint32_t getdword(void *mem)
    {
        uint32_t tmp;
#ifndef ARCH_IS_BIG_ENDIAN
        ((uint8_t*)&tmp)[0] = ((uint8_t*)mem)[3];
        ((uint8_t*)&tmp)[1] = ((uint8_t*)mem)[2];
        ((uint8_t*)&tmp)[2] = ((uint8_t*)mem)[1];
        ((uint8_t*)&tmp)[3] = ((uint8_t*)mem)[0];
#else
        ((uint8_t*)&tmp)[0] = ((uint8_t*)mem)[0];
        ((uint8_t*)&tmp)[1] = ((uint8_t*)mem)[1];
        ((uint8_t*)&tmp)[2] = ((uint8_t*)mem)[2];
        ((uint8_t*)&tmp)[3] = ((uint8_t*)mem)[3];
#endif

        return tmp;
    }

    /* reads only n bytes from the stream instead of the standard 4 */
    static /*INLINE*/ uint32_t getdword_n(void *mem, int n)
    {
        uint32_t tmp = 0;
#ifndef ARCH_IS_BIG_ENDIAN
        switch (n) {
        case 3:
            ((uint8_t*)&tmp)[1] = ((uint8_t*)mem)[2];
        case 2:
            ((uint8_t*)&tmp)[2] = ((uint8_t*)mem)[1];
        case 1:
            ((uint8_t*)&tmp)[3] = ((uint8_t*)mem)[0];
        default:
            break;
        }
#else
        switch (n) {
        case 3:
            ((uint8_t*)&tmp)[2] = ((uint8_t*)mem)[2];
        case 2:
            ((uint8_t*)&tmp)[1] = ((uint8_t*)mem)[1];
        case 1:
            ((uint8_t*)&tmp)[0] = ((uint8_t*)mem)[0];
        default:
            break;
        }
#endif

        return tmp;
    }

    static INLINE uint32_t faad_showbits(bitfile *ld, uint32_t bits)
    {
        if (bits <= ld->bits_left) {
            //return (ld->bufa >> (ld->bits_left - bits)) & bitmask[bits];
            return (ld->bufa << (32 - ld->bits_left)) >> (32 - bits);
        }

        bits -= ld->bits_left;
        //return ((ld->bufa & bitmask[ld->bits_left]) << bits) | (ld->bufb >> (32 - bits));
        return ((ld->bufa & ((1<<ld->bits_left)-1)) << bits) | (ld->bufb >> (32 - bits));
    }

    static /*INLINE*/ void faad_flushbits_ex(bitfile *ld, uint32_t bits)
    {
        uint32_t tmp;

        ld->bufa = ld->bufb;
        if (ld->bytes_left >= 4) {
            tmp = getdword(ld->tail);
            ld->bytes_left -= 4;
        } else {
            tmp = getdword_n(ld->tail, ld->bytes_left);
            ld->bytes_left = 0;
        }
        ld->bufb = tmp;
        ld->tail++;
        ld->bits_left += (32 - bits);
    }

    static INLINE void faad_flushbits(bitfile *ld, uint32_t bits)
    {
        /* do nothing if error */
        if (ld->error != 0)
            return;

        if (bits < ld->bits_left) {
            ld->bits_left -= bits;
        } else {
            faad_flushbits_ex(ld, bits);
        }
    }

    /* return next n bits (right adjusted) */
    static /*INLINE*/ uint32_t faad_getbits(bitfile *ld, uint32_t n)
    {
        uint32_t ret;

        if (n == 0)
            return 0;

        ret = faad_showbits(ld, n);
        faad_flushbits(ld, n);

        return ret;
    }

    static INLINE uint8_t faad_get1bit(bitfile *ld)
    {
        uint8_t r;

        if (ld->bits_left > 0) {
            ld->bits_left--;
            r = (uint8_t)((ld->bufa >> ld->bits_left) & 1);
            return r;
        }

        /* bits_left == 0 */
#if 0
        r = (uint8_t)(ld->bufb >> 31);
        faad_flushbits_ex(ld, 1);
#else
        r = (uint8_t)faad_getbits(ld, 1);
#endif
        return r;
    }

    void faad_initbits(bitfile *ld, const void *_buffer, const uint32_t buffer_size);
    uint8_t faad_byte_align(bitfile *ld);
    void faad_endbits(bitfile *ld);
    uint32_t faad_get_processed_bits(bitfile *ld);
    uint32_t get_sample_rate(const uint8_t sr_index);
    int8_t rda_GASpecificConfig(bitfile *ld, mp4AudioSpecificConfig *mp4ASC);
    int8_t rda_AudioSpecificConfigFromBitfile(bitfile *ld, mp4AudioSpecificConfig *mp4ASC, uint32_t buffer_size, uint8_t short_form);
    int8_t rda_AudioSpecificConfig2(uint8_t *pBuffer, uint32_t buffer_size, mp4AudioSpecificConfig *mp4ASC, uint8_t short_form);
    int32_t rda_MakeAdtsHeader(unsigned char *data, int size, mp4AudioSpecificConfig *mp4ASC);
    uint32_t read_callback(void *user_data, void *buffer, uint32_t length);
    uint32_t seek_callback(void *user_data, uint64_t position);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

