#include "utf-8.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wchar.h>

/* pre-filled bit pattern of encoding units */
#define UNIT11  0x00        /* 1st unit (total of 1) , 1st range - 0xxxxxxx -> 0000 0000 */
#define UNIT12  0xC0        /* 1st unit (total of 2), 2nd range - 110xxxxx -> 1100 0000  */
#define UNIT13  0xE0        /* 1st unit (total of 3), 3rd range - 1110xxxx -> 1110 0000  */
#define UNIT14  0xF0        /* 1st unit (total of 4), 4th range  - 11110xxx -> 1111 0000 */
#define UNITX   0x80        /* other units (02, 03, 04) - 10xxxxxx -> 1000 0000          */

/* bit masks corresponding to each encoding unit above */
#define MASK11  0x7F        /* 1st unit's mask, 1st range - 0xxxxxxx -> 0111 1111 */
#define MASK12  0x1F        /* 1st unit's mask, 2nd range - 110xxxxx -> 0001 1111 */
#define MASK13  0x0F        /* 1st unit's mask, 3rd range - 1110xxxx -> 0000 1111 */
#define MASK14  0x07        /* 1st unit's mask, 4th range - 11110xxx -> 0000 0111 */
#define MASKX   0x3F        /* other units's mask - 10xxxxxx -> 0011 1111         */

/* number of bits need to be filled in final encoding unit of each range (missing length) */
#define MLEN11  7           /* 1st unit, 1st range */
#define MLEN12  5           /* 1st unit, 2nd range */
#define MLEN13  4           /* 1st unit, 3rd range */
#define MLEN14  3           /* 1st unit, 4th range */
#define MLENX   6           /* other units (if any) share the same pattern */

/* pre-filled value of each encoding unit for each range */
#define PRE_VAL11   0x00    /* 1st unit (total of 1), 1st range - US-ASCII starts with 0-bit */
#define PRE_VAL12   0x06    /* 1st unit (total of 2), 2nd range - 110xxxxx -> 110b = 0x06    */
#define PRE_VAL13   0x0E    /* 1st unit (total of 3), 3rd range - 1110xxxx -> 1110b = 0x0E   */
#define PRE_VAL14   0x1E    /* 1st unit (total of 4), 4th range - 11110xxx -> 11110b = 0x1E  */
#define PRE_VALX    0x02    /* other units, range 2nd -> 4th - 10xxxxxx -> 10b = 0x02        */

#define OCTET   8           /* an octet */

/* 512MB - the smaller the better, data buffer will be read one and for all. This is not meant to
 * be an interactive editor anyway.
 */
#define MAX_BUFFER_SIZE 536870912

/**
 * @brief num_of_units Utility function, calculates number of encoding units needed for an Unicode
 * code point based on its range.
 * @param code_point Unicode code point.
 * @return Number of encodings units [1, 4]. Other values (0 in particular) must be considered as
 * invalid by calling function.
 */
static inline size_t num_of_units(uint32_t code_point)
{
    if      (code_point <= 0x00007F)                            return 1;
    else if (code_point >= 0x00080 && code_point <= 0x0007FF)   return 2;
    else if (code_point >= 0x0D800 && code_point <= 0x00DFFF)   return 0;   /* surrogate pairs */
    else if (code_point >= 0x00800 && code_point <= 0x00FFFF)   return 3;
    else if (code_point >= 0x10000 && code_point <= 0x10FFFF)   return 4;

    return 0;
}

enc_unit_t encode(uint32_t code_point)
{
    enc_unit_t enc_unit;
    enc_unit.len = num_of_units(code_point);

    switch (enc_unit.len)
    {
    case 1:
        enc_unit.units[0] = code_point;      /* cp & MASK11 -> no need */
        break;

    case 2:
        enc_unit.units[1] = UNITX  | (code_point & MASKX);              /* 2nd unit */
        enc_unit.units[0] = UNIT12 | (code_point >> MLENX & MASK12);    /* 1st unit */
        break;

    case 3:
        enc_unit.units[2] = UNITX  | (code_point & MASKX);                      /* 3rd unit */
        enc_unit.units[1] = UNITX  | (code_point >> MLENX & MASKX);             /* 2nd unit */
        enc_unit.units[0] = UNIT13 | (code_point >> MLENX >> MLENX & MASK13);   /* 1st unit */
        break;

    case 4:
        enc_unit.units[3] = UNITX  | (code_point & MASKX);                      /* 4th unit */
        enc_unit.units[2] = UNITX  | (code_point >> MLENX & MASKX);             /* 3rd unit */
        enc_unit.units[1] = UNITX  | (code_point >> MLENX >> MLENX & MASKX);    /* 2nd unit */
        enc_unit.units[0] = UNIT14 | (code_point >> MLENX >> MLENX >> MLENX     /* 1st unit */
                                      & MASK14);
        break;

    default:
        /* invalid code point, it does not matter if encoding units left unset because the length
         * has already been set to 0
         */
        break;
    }

    return enc_unit;
}

/**
 * @brief valid Check the validity of encoding units W.R.T an UCS by its length.
 * @param enc_unit Encoding units, should be result of encoding operation.
 * @return true on valid, false otherwise.
 */
static inline bool valid(const enc_unit_t *enc_unit)
{
    /* other than US-ASCII, then remaining encoding units should be pre-fixed with 10b */
    for (size_t i = 1; i < enc_unit->len; ++i)
    {
        if (enc_unit->units[i] >> MLENX != PRE_VALX)
            return false;
    }

    /* double check validity of encoding unit prefix values and length */
    switch (enc_unit->len)
    {
    case 1:
        if (enc_unit->units[0] >> MLEN11)
            /* US-ASCII encoding unit should start with 0-bit */
            return false;
        break;

    case 2:
        if (enc_unit->units[0] >> MLEN12 != PRE_VAL12)
            /* pre-filled bits should be 110b for 1st unit */
            return false;
        break;

    case 3:
        if (enc_unit->units[0] >> MLEN13 != PRE_VAL13)
            /* pre-filled bits should be 1110b for 1st unit */
            return false;
        break;

    case 4:
        if (enc_unit->units[0] >> MLEN14 != PRE_VAL14)
            /* pre-filled bits should be 11110b for 1st unit */
            return false;
        break;

    default:
        return false;
    }

    /* if reach here, then it is valid encoding units as a whole */
    return true;
}

/**
 * @brief length_of Determine the length of "potential" complete encoding units, based on the first
 * encoding unit only.
 * @param first_enc Bit pattern of 1st encoding unit.
 * @return Number of encoding units.
 */
static inline size_t length_of(uint8_t first_enc)
{
    if      (first_enc >> MLEN11 == 0)          return 1;   /* US-ASCII, length 1  */
    else if (first_enc >> MLEN12 == PRE_VAL12)  return 2;   /* 2nd range, length 2 */
    else if (first_enc >> MLEN13 == PRE_VAL13)  return 3;   /* 3rd range, length 3 */
    else if (first_enc >> MLEN14 == PRE_VAL14)  return 4;   /* 4th range, length 4 */
    else return 0;                                          /* rubbish, length 0   */
}

uint32_t decode(const enc_unit_t *enc_unit)
{
    if (!valid(enc_unit)) return INVALID_CODE_POINT;

    /* now distribute bit pattern to construct code point, proceed from least significant bit of
     * last encoding unit up until the most significant bit of first encoding unit (pre-filled bits
     * are not part of this scheme)
     */
    uint32_t code_point = 0;
    uint32_t lenx = 0;
    for (size_t i = enc_unit->len - 1; i > 0; --i)
    {
        code_point |= (enc_unit->units[i] & MASKX) << lenx;
        lenx += MLENX;
    }

    /* map last encoding unit, am I a dumb? */
    switch (enc_unit->len)
    {
    case 1:
        code_point |= (enc_unit->units[0] & MASK11);
        break;

    case 2:
        code_point |= (enc_unit->units[0] & MASK12) << lenx;
        break;

    case 3:
        code_point |= (enc_unit->units[0] & MASK13) << lenx;
        break;

    case 4:
        code_point |= (enc_unit->units[0] & MASK14) << lenx;
        break;

    default:
        break;
    }

    return code_point;
}

uint32_t* read(const char *path, size_t *n_elems)
{
    uint32_t *code_points = NULL;   /* output code points */
    uint8_t  *buffer      = NULL;   /* hold data buffer, entirely */
    size_t   sz           = 0;      /* size of input stream in term of bytes */
    size_t   rsz          = 0;
    size_t   i            = 0;

    FILE *f = fopen(path, "rb");

    if (f == NULL) goto END;

    /* determine size of input stream */
    fseek(f, 0L, SEEK_END);
    sz = ftell(f);
    rewind(f);

    /* buffer is too big, apparently way too bigger than my capability, or maybe it is empty */
    if (sz > MAX_BUFFER_SIZE || sz == 0) goto END;

    /* read whole input stream into buffer */
    buffer = (uint8_t*) malloc(sz);
    rsz = fread(buffer, sizeof(uint8_t), sz, f);

    /* done with input stream, close it */
    fclose(f);
    f = NULL;

    if (rsz != sz) goto END;    /* unable to read whole buffer, the sizes don't match */

    /* Up to sz code points might be needed. Although always lurks a chance of redundancy, I have
     * not figured out a clean way to get around, yet.
     */
    code_points = (uint32_t*) malloc(sz * sizeof(uint32_t));
    enc_unit_t enc_unit;

    while (i < sz)
    {
        /* read just 1 byte from buffer */
        enc_unit.units[0] = buffer[i];

        /* determine the length of potential encoding units */
        enc_unit.len = length_of(enc_unit.units[0]);

        /* invalid encoding units, it's likely that input stream be corrupted or not in UTF-8 */
        if (!enc_unit.len) goto END;

        /* fill remaining units */
        if (i + enc_unit.len - 1 > sz)
        {
            goto END; /* incomplete encoding units */
        }

        /* be careful */
        memcpy(&enc_unit.units[0], &buffer[i], enc_unit.len);

        uint32_t cp = decode(&enc_unit);
        if (cp == INVALID_CODE_POINT)   goto END;
        else                            code_points[(*n_elems)++] = cp;

        i += enc_unit.len;
    }

END:
    if (f) fclose(f);
    if (buffer) free(buffer);
    if (!n_elems && code_points)    /* error case */
    {
        free(code_points);
        code_points = NULL;
    }
    return code_points;
}

int write(const char *path, const enc_unit_t *enc_units, size_t n_elems)
{
    FILE *f = fopen(path, "w");
    if (f == NULL) return -1;

    for (size_t i = 0; i < n_elems; ++i)
    {
        if (!valid(&enc_units[i]))      /* invalid encoding units */
        {
            fclose(f);
            return -1;
        }

        fwrite(enc_units[i].units, sizeof(enc_units[i].units[0]), enc_units[i].len, f);
    }

    /* new-line */
    enc_unit_t lf = encode(0x0A);
    fwrite(lf.units, sizeof(lf.units[0]), lf.len, f);
    fclose(f);

    return 0;
}
