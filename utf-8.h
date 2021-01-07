/**
 * @file    utf-8.h
 * @brief   A rudimentary implementation of UTF-8 encoding scheme (specified by RFC 3629)
 * @author  BUI HUY THANG
 * @date    11/2020
 * @related RFC 3629, RFC 2234
 * @attention I must admit that at the time of this writing, however, there are still many things
 * that left unanswered and unclear. But as it's better late than never, who knows if it works e-
 * ventually.
 * @details Dealing with UTF-8 encoding method of Unicode code points involves 2 basic operations:
 * 1) Encoding, i.e convert the code point to its binary pattern specified by the scheme.
 * 2) Decoding, i.e convert the binary pattern back to its original code point. Provided system
 * locale is correctly set, then the character can be perfectly visualized by some text editors
 * of choice.
 * Those will be described more details as below.
 * @note Some comments are stupid and unnecessary, it's because I'm just a sub-standard programmer,
 * I don't even know if my implementation is correct or not. And if it is, whether or not be accep-
 * table. I did it for me, and nobody else.
 */

#ifndef UTF8_H
#define UTF8_H

#include <stdint.h>
#include <stddef.h>

/* invalid encoding unit */
#define INVALID_ENC_UNIT    0xFFFFFFFF

/* invalid code point */
#define INVALID_CODE_POINT  0xFFFFFFFF

/**
 * @brief The enc_unit_t struct defines components for the output of encoding an Unicode code point
 * according to UTF-8 scheme. It should be used as result of encoding operation and input of decod-
 * ing operation.
 * @note In fact, a 32-bit unsigned quantity should be large enough to hold the result, but to
 * avoid unnecessary processing when dealing with machine's endianness which is HW+OS dependent,
 * and to lower commitment with with callers, it would be better to dedicate a data structure to
 * serve that purpose in my opinion.
 * Encoding function should guarantee below layout of the result it returns:
 *
 * Unit Index           |   Meaning
 * ---------------------+---------------------------------------------
 * 0                    |   1st encoding unit
 * 1                    |   2nd encoding unit
 * 2                    |   3rd encoding unit
 * 3                    |   4th encoding unit
 *
 * In other words, the bit pattern of a code point will span from left to right, each unit occupies
 * 1 byte.
 */
typedef struct
{
    uint8_t units[4];   /* encoding units (bit patterns) */
    size_t  len;        /* encoding length (number of encoding units) */
} enc_unit_t;

/**
 * @brief encode Transform an Unicode code point represented by @param code_point to one or sev-
 * eral (up to 4) encoding units (octets), depends on which range of values it is in. Concretely,
 * RFC 3629 specified these ranges and their corresponding bit patterns as follow:
 *
 * Char. number range   |   UTF-8 octet sequence
 * (hexadecimal)        |   (binary)
 * ---------------------+---------------------------------------------
 * 0000 0000-0000 007F  |   0xxxxxxx
 * 0000 0080-0000 07FF  |   110xxxxx 10xxxxxx
 * 0000 0800-0000 FFFF  |   1110xxxx 10xxxxxx 10xxxxxx
 * 0001 0000-0010 FFFF  |   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 *
 * Pretty straight forward, huh?
 *
 * The transformation therefore simply fills in correct value for bits marked as 'x' above. The
 * necessity of steps that need to be done has already clearly explained within standard itself.
 *
 * @param code_point Unicode code point (a.k.a code position, character number, scalar value). UCS
 * characters are designed as the U+HHHH notation, where HHHH is a string contains from 4 to 6 he-
 * xadecimal digits, so uint32_t is large enough to hold the value.
 * @return A value of type enc_unit_t defined above. Basically, contains encoding unit values and
 * number of encoding units which represents bit pattern of corresponding code point, obeyed the
 * encoding scheme (column to the right of table above).
 * In case of error, for example when an invalid character number passed into, then the returned
 * value of type enc_unit_t will have length ("len") field set to 0, effectively means invalid.
 * Please see @note mentioned in enc_unit_t definition for more details.
 */
enc_unit_t encode(uint32_t code_point);

/**
 * @brief decode Transform encoding units back to an Unicode code point. As already explained in
 * encoding funcion above and again, even more comprehensive in standard document, the operation
 * will distribute the bits from encoding units into a binary number to make it a code point, of-
 * course without fixed pre-filled bit pattern that makes up those units in the first place.
 * @param enc_unit A pointer of type enc_unit_t. It should be result of encoding operation.
 * @return An Unicode code point.
 * In case of error, for example when an invalid enc_unit_t passed into (length of 0, length does
 * not match with encoding units,...) then INVALID_CODE_POINT will be returned.
 * @note The implementation of this is not quite symmetric with encoding function above.
 */
uint32_t decode(const enc_unit_t *enc_unit);

/**
 * @brief read Read UFT-8 content from file.
 * @param path Path to source, including file name.
 * @param n_elems Number of valid code points which have been successfully decoded, passed as
 * in/out parameter.
 * @return Number of valid code points which have been successfully decoded.
 * @return List of valid code points. Callers are responsible for deallocate it afterward.
 */
uint32_t* read(const char *path, size_t *n_elems);

/**
 * @brief write Save UTF-8 content to file with LF at the end. Existing file will be truncated.
 * @param path Path to destination, including file name.
 * @param enc_units An array of encoding units.
 * @param n_elems Number of encoding units
 * @return 0 on success, others mean failure.
 */
int write(const char *path, const enc_unit_t *enc_units, size_t n_elems);

#endif // UTF8_H
