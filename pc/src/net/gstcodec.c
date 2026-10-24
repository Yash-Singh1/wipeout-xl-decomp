#include <string.h>
#include "gst.h"

/* Packet decoders and encoders. Every call reports (result, call id) through gstReportError;
   the ids are 0x20000 + the position of the function in this file. */

/* Texts of the error codes 0x80010000 + n and the names of the call ids 0x20000 + n. */
const char *gstCodecErrorNames[4] = {
    "Decode beyond packet size",
    "Encode NULL DATA pointer/array",
    "Encode DATA beyond MAX BITS",
    "Encode beyond MAX packet size",
};

const char *gstCodecFunctionNames[24] = {
    "Multiple Character Decode",
    "Multiple Unsigned Short Decode",
    "Multiple Signed Short Decode",
    "Multiple UNUM Decode",
    "Multiple SNUM Decode",
    "Multiple ONUM Decode",
    "Character Decode",
    "Unsigned Short Decode",
    "Signed Short Decode",
    "UNUM Decode",
    "SNUM Decode",
    "ONUM Decode",
    "Multiple Character Encode",
    "Multiple Unsigned Short Encode",
    "Multiple Signed Short Encode",
    "Multiple UNUM Encode",
    "Multiple SNUM Encode",
    "Multiple ONUM Encode",
    "Character Encode",
    "Unsigned Short Encode",
    "Signed Short Encode",
    "UNUM Encode",
    "SNUM Encode",
    "ONUM Encode",
};

/* Call 0x20000 */
int gstDecodeBytes(GstCodec *codec, unsigned count, void *dest)
{
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20000);
    if (codec->pos + count > codec->size)
        return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20000);
    if (dest)
        memcpy(dest, codec->data + codec->pos, count);
    codec->pos += count;
    return gstReportError(1, 0x20000);
}

/* Call 0x20001: big-endian 16-bit values. */
int gstDecodeU16Array(GstCodec *codec, unsigned count, unsigned short *dest)
{
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20001);
    if (codec->pos + count * 2 > codec->size)
        return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20001);
    if (dest) {
        while (count > 0) {
            *dest = codec->data[codec->pos];
            *dest <<= 8;
            *dest += codec->data[codec->pos + 1];
            codec->pos += 2;
            dest++;
            count--;
        }
    } else
        codec->pos += count * 2;
    return gstReportError(1, 0x20001);
}

/* Call 0x20002 */
int gstDecodeS16Array(GstCodec *codec, unsigned count, short *dest)
{
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20002);
    if (codec->pos + count * 2 > codec->size)
        return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20002);
    if (dest) {
        while (count > 0) {
            *dest = codec->data[codec->pos];
            *dest <<= 8;
            *dest += codec->data[codec->pos + 1];
            codec->pos += 2;
            dest++;
            count--;
        }
    } else
        codec->pos += count * 2;
    return gstReportError(1, 0x20002);
}

/* Call 0x20003: one, two or four bytes per value. */
int gstDecodePackedU32Array(GstCodec *codec, unsigned count, unsigned long *dest)
{
    unsigned long value;

    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20003);
    while (count > 0) {
        if (codec->pos + 1 > codec->size)
            return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20003);
        value = codec->data[codec->pos];
        if (!(value & 0x80))
            codec->pos++;
        else if (!(value & 0x40)) {
            if (codec->pos + 2 > codec->size)
                return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20003);
            value &= 0x3f;
            value <<= 8;
            value |= codec->data[codec->pos + 1];
            codec->pos += 2;
        } else {
            if (codec->pos + 4 > codec->size)
                return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20003);
            value &= 0x3f;
            value <<= 8;
            value |= codec->data[codec->pos + 1];
            value <<= 8;
            value |= codec->data[codec->pos + 2];
            value <<= 8;
            value |= codec->data[codec->pos + 3];
            codec->pos += 4;
        }
        if (dest) {
            *dest = value;
            dest++;
        }
        count--;
    }
    return gstReportError(1, 0x20003);
}

/* Call 0x20004: as 0x20003, sign-extended from the top payload bit. */
int gstDecodePackedS32Array(GstCodec *codec, unsigned count, long *dest)
{
    long value;

    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20004);
    while (count > 0) {
        if (codec->pos + 1 > codec->size)
            return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20004);
        value = codec->data[codec->pos];
        if (!(value & 0x80)) {
            if (value & 0x40)
                value |= 0xffffff80;
            codec->pos++;
        } else if (!(value & 0x40)) {
            if (codec->pos + 2 > codec->size)
                return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20004);
            value &= 0x3f;
            if (value & 0x20)
                value |= 0xffffffc0;
            value <<= 8;
            value |= codec->data[codec->pos + 1];
            codec->pos += 2;
        } else {
            if (codec->pos + 4 > codec->size)
                return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20004);
            value &= 0x3f;
            if (value & 0x20)
                value |= 0xffffffc0;
            value <<= 8;
            value |= codec->data[codec->pos + 1];
            value <<= 8;
            value |= codec->data[codec->pos + 2];
            value <<= 8;
            value |= codec->data[codec->pos + 3];
            codec->pos += 4;
        }
        if (dest) {
            *dest = value;
            dest++;
        }
        count--;
    }
    return gstReportError(1, 0x20004);
}

/* Call 0x20005: one, two or three bytes per value. */
int gstDecodePackedU24Array(GstCodec *codec, unsigned count, unsigned long *dest)
{
    unsigned long value;

    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20005);
    while (count > 0) {
        if (codec->pos + 1 > codec->size)
            return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20005);
        value = codec->data[codec->pos];
        if (!(value & 0x80))
            codec->pos++;
        else if (!(value & 0x40)) {
            if (codec->pos + 2 > codec->size)
                return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20005);
            value &= 0x3f;
            value <<= 8;
            value |= codec->data[codec->pos + 1];
            codec->pos += 2;
        } else {
            if (codec->pos + 3 > codec->size)
                return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20005);
            value &= 0x3f;
            value <<= 8;
            value |= codec->data[codec->pos + 1];
            value <<= 8;
            value |= codec->data[codec->pos + 2];
            codec->pos += 3;
        }
        if (dest) {
            *dest = value;
            dest++;
        }
        count--;
    }
    return gstReportError(1, 0x20005);
}

/* Call 0x20006 */
int gstDecodeU8(GstCodec *codec, unsigned char *dest)
{
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20006);
    if (codec->pos + 1 > codec->size)
        return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20006);
    if (dest)
        *dest = codec->data[codec->pos];
    codec->pos++;
    return gstReportError(1, 0x20006);
}

/* Call 0x20007 */
int gstDecodeU16(GstCodec *codec, unsigned short *dest)
{
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20007);
    if (codec->pos + 2 > codec->size)
        return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20007);
    if (dest) {
        *dest = codec->data[codec->pos];
        *dest <<= 8;
        *dest += codec->data[codec->pos + 1];
    }
    codec->pos += 2;
    return gstReportError(1, 0x20007);
}

/* Call 0x20008 */
int gstDecodeS16(GstCodec *codec, short *dest)
{
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20008);
    if (codec->pos + 2 > codec->size)
        return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20008);
    if (dest) {
        *dest = codec->data[codec->pos];
        *dest <<= 8;
        *dest += codec->data[codec->pos + 1];
    }
    codec->pos += 2;
    return gstReportError(1, 0x20008);
}

/* Call 0x20009 */
int gstDecodePackedU32(GstCodec *codec, unsigned long *dest)
{
    unsigned long value;

    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20009);
    if (codec->pos + 1 > codec->size)
        return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20009);
    value = codec->data[codec->pos];
    if (!(value & 0x80))
        codec->pos++;
    else if (!(value & 0x40)) {
        if (codec->pos + 2 > codec->size)
            return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20009);
        value &= 0x3f;
        value <<= 8;
        value |= codec->data[codec->pos + 1];
        codec->pos += 2;
    } else {
        if (codec->pos + 4 > codec->size)
            return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x20009);
        value &= 0x3f;
        value <<= 8;
        value |= codec->data[codec->pos + 1];
        value <<= 8;
        value |= codec->data[codec->pos + 2];
        value <<= 8;
        value |= codec->data[codec->pos + 3];
        codec->pos += 4;
    }
    if (dest)
        *dest = value;
    return gstReportError(1, 0x20009);
}

/* Call 0x2000a */
int gstDecodePackedS32(GstCodec *codec, long *dest)
{
    long value;

    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x2000a);
    if (codec->pos + 1 > codec->size)
        return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x2000a);
    value = codec->data[codec->pos];
    if (!(value & 0x80)) {
        if (value & 0x40)
            value |= 0xffffff80;
        codec->pos++;
    } else if (!(value & 0x40)) {
        if (codec->pos + 2 > codec->size)
            return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x2000a);
        value &= 0x3f;
        if (value & 0x20)
            value |= 0xffffffc0;
        value <<= 8;
        value |= codec->data[codec->pos + 1];
        codec->pos += 2;
    } else {
        if (codec->pos + 4 > codec->size)
            return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x2000a);
        value &= 0x3f;
        if (value & 0x20)
            value |= 0xffffffc0;
        value <<= 8;
        value |= codec->data[codec->pos + 1];
        value <<= 8;
        value |= codec->data[codec->pos + 2];
        value <<= 8;
        value |= codec->data[codec->pos + 3];
        codec->pos += 4;
    }
    if (dest)
        *dest = value;
    return gstReportError(1, 0x2000a);
}

/* Call 0x2000b */
int gstDecodePackedU24(GstCodec *codec, unsigned long *dest)
{
    unsigned long value;

    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x2000b);
    if (codec->pos + 1 > codec->size)
        return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x2000b);
    value = codec->data[codec->pos];
    if (!(value & 0x80))
        codec->pos++;
    else if (!(value & 0x40)) {
        if (codec->pos + 2 > codec->size)
            return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x2000b);
        value &= 0x3f;
        value <<= 8;
        value |= codec->data[codec->pos + 1];
        codec->pos += 2;
    } else {
        if (codec->pos + 3 > codec->size)
            return gstReportError(GST_ERR_DECODE_OVERFLOW, 0x2000b);
        value &= 0x3f;
        value <<= 8;
        value |= codec->data[codec->pos + 1];
        value <<= 8;
        value |= codec->data[codec->pos + 2];
        codec->pos += 3;
    }
    if (dest)
        *dest = value;
    return gstReportError(1, 0x2000b);
}

/* Call 0x2000c */
int gstEncodeBytes(GstCodec *codec, unsigned count, const void *src)
{
    if (count == 0)
        return gstReportError(1, 0x2000c);
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x2000c);
    if (src == 0)
        return gstReportError(GST_ERR_ENCODE_NULL_DATA, 0x2000c);
    if (codec->pos + count > codec->maxSize)
        return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x2000c);
    memcpy(codec->data + codec->pos, src, count);
    codec->pos += count;
    if (codec->size < codec->pos)
        codec->size = codec->pos;
    return gstReportError(1, 0x2000c);
}

/* Call 0x2000d */
int gstEncodeU16Array(GstCodec *codec, unsigned count, const unsigned short *src)
{
    if (count == 0)
        return gstReportError(1, 0x2000d);
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x2000d);
    if (src == 0)
        return gstReportError(GST_ERR_ENCODE_NULL_DATA, 0x2000d);
    if (count * 2 + codec->pos > codec->maxSize)
        return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x2000d);
    while (count > 0) {
        codec->data[codec->pos] = *src >> 8;
        codec->data[codec->pos + 1] = *src & 0xff;
        codec->pos += 2;
        src++;
        count--;
    }
    if (codec->size < codec->pos)
        codec->size = codec->pos;
    return gstReportError(1, 0x2000d);
}

/* Call 0x2000e */
int gstEncodeS16Array(GstCodec *codec, unsigned count, const short *src)
{
    if (count == 0)
        return gstReportError(1, 0x2000e);
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x2000e);
    if (src == 0)
        return gstReportError(GST_ERR_ENCODE_NULL_DATA, 0x2000e);
    if (count * 2 + codec->pos > codec->maxSize)
        return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x2000e);
    while (count > 0) {
        codec->data[codec->pos] = *src >> 8;
        codec->data[codec->pos + 1] = (unsigned char)*src;
        codec->pos += 2;
        src++;
        count--;
    }
    if (codec->size < codec->pos)
        codec->size = codec->pos;
    return gstReportError(1, 0x2000e);
}

/* Call 0x2000f */
int gstEncodePackedU32Array(GstCodec *codec, unsigned count, const unsigned long *src)
{
    unsigned long value;

    if (count == 0)
        return gstReportError(1, 0x2000f);
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x2000f);
    if (src == 0)
        return gstReportError(GST_ERR_ENCODE_NULL_DATA, 0x2000f);
    while (count > 0) {
        if (codec->pos + 1 > codec->maxSize)
            return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x2000f);
        value = *src;
        if ((value & 0xffffff80) == 0) {
            codec->data[codec->pos] = (unsigned char)(value & 0x7f);
            codec->pos++;
        } else if ((value & 0xffffc000) == 0) {
            if (codec->pos + 2 > codec->maxSize)
                return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x2000f);
            codec->data[codec->pos] = ((value >> 8) & 0x3f) | 0x80;
            codec->data[codec->pos + 1] = (unsigned char)value;
            codec->pos += 2;
        } else if ((value & 0xc0000000) == 0) {
            if (codec->pos + 4 > codec->maxSize)
                return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x2000f);
            codec->data[codec->pos] = ((value >> 24) & 0x3f) | 0xc0;
            codec->data[codec->pos + 1] = (unsigned char)(value >> 16);
            codec->data[codec->pos + 2] = (unsigned char)(value >> 8);
            codec->data[codec->pos + 3] = (unsigned char)value;
            codec->pos += 4;
        } else
            return gstReportError(GST_ERR_ENCODE_RANGE, 0x2000f);
        src++;
        count--;
    }
    if (codec->size < codec->pos)
        codec->size = codec->pos;
    return gstReportError(1, 0x2000f);
}

/* Call 0x20010 */
int gstEncodePackedS32Array(GstCodec *codec, unsigned count, const long *src)
{
    long value;

    if (count == 0)
        return gstReportError(1, 0x20010);
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20010);
    if (src == 0)
        return gstReportError(GST_ERR_ENCODE_NULL_DATA, 0x20010);
    while (count > 0) {
        if (codec->pos + 1 > codec->maxSize)
            return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20010);
        value = *src;
        if (value >= -0x40 && value <= 0x3f) {
            codec->data[codec->pos] = (unsigned char)(value & 0x7f);
            codec->pos++;
        } else if (value >= -0x4000 && value <= 0x3fff) {
            if (codec->pos + 2 > codec->maxSize)
                return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20010);
            codec->data[codec->pos] = ((value >> 8) & 0x3f) | 0x80;
            codec->data[codec->pos + 1] = (unsigned char)value;
            codec->pos += 2;
        } else if (value >= -0x40000000 && value <= 0x3fffffff) {
            if (codec->pos + 4 > codec->maxSize)
                return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20010);
            codec->data[codec->pos] = ((value >> 24) & 0x3f) | 0xc0;
            codec->data[codec->pos + 1] = (unsigned char)(value >> 16);
            codec->data[codec->pos + 2] = (unsigned char)(value >> 8);
            codec->data[codec->pos + 3] = (unsigned char)value;
            codec->pos += 4;
        } else
            return gstReportError(GST_ERR_ENCODE_RANGE, 0x20010);
        src++;
        count--;
    }
    if (codec->size < codec->pos)
        codec->size = codec->pos;
    return gstReportError(1, 0x20010);
}

/* Call 0x20011 */
int gstEncodePackedU24Array(GstCodec *codec, unsigned count, const unsigned long *src)
{
    unsigned long value;

    if (count == 0)
        return gstReportError(1, 0x20011);
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20011);
    if (src == 0)
        return gstReportError(GST_ERR_ENCODE_NULL_DATA, 0x20011);
    while (count > 0) {
        if (codec->pos + 1 > codec->maxSize)
            return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20011);
        value = *src;
        if ((value & 0xffffff80) == 0) {
            codec->data[codec->pos] = (unsigned char)(value & 0x7f);
            codec->pos++;
        } else if ((value & 0xffffc000) == 0) {
            if (codec->pos + 2 > codec->maxSize)
                return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20011);
            codec->data[codec->pos] = ((value >> 8) & 0x3f) | 0x80;
            codec->data[codec->pos + 1] = (unsigned char)value;
            codec->pos += 2;
        } else if ((value & 0xffc00000) == 0) {
            if (codec->pos + 3 > codec->maxSize)
                return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20011);
            codec->data[codec->pos] = ((value >> 16) & 0x3f) | 0xc0;
            codec->data[codec->pos + 1] = (unsigned char)(value >> 8);
            codec->data[codec->pos + 2] = (unsigned char)value;
            codec->pos += 3;
        } else
            return gstReportError(GST_ERR_ENCODE_RANGE, 0x20011);
        src++;
        count--;
    }
    if (codec->size < codec->pos)
        codec->size = codec->pos;
    return gstReportError(1, 0x20011);
}

/* Call 0x20012 */
int gstEncodeU8(GstCodec *codec, unsigned char value)
{
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20012);
    if (codec->pos + 1 > codec->maxSize)
        return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20012);
    codec->data[codec->pos] = value;
    codec->pos++;
    if (codec->size < codec->pos)
        codec->size = codec->pos;
    return gstReportError(1, 0x20012);
}

/* Call 0x20013 */
int gstEncodeU16(GstCodec *codec, unsigned short value)
{
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20013);
    if (codec->pos + 2 > codec->maxSize)
        return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20013);
    codec->data[codec->pos] = value >> 8;
    codec->data[codec->pos + 1] = (unsigned char)value;
    codec->pos += 2;
    if (codec->size < codec->pos)
        codec->size = codec->pos;
    return gstReportError(1, 0x20013);
}

/* Call 0x20014 */
int gstEncodeS16(GstCodec *codec, short value)
{
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20014);
    if (codec->pos + 2 > codec->maxSize)
        return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20014);
    codec->data[codec->pos] = value >> 8;
    codec->data[codec->pos + 1] = (unsigned char)value;
    codec->pos += 2;
    if (codec->pos > codec->size)
        codec->size = codec->pos;
    return gstReportError(1, 0x20014);
}

/* Call 0x20015 */
int gstEncodePackedU32(GstCodec *codec, unsigned long value)
{
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20015);
    if (codec->pos + 1 > codec->maxSize)
        return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20015);
    if ((value & 0xffffff80) == 0) {
        codec->data[codec->pos] = (unsigned char)(value & 0x7f);
        codec->pos++;
    } else if ((value & 0xffffc000) == 0) {
        if (codec->pos + 2 > codec->maxSize)
            return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20015);
        codec->data[codec->pos] = ((value >> 8) & 0x3f) | 0x80;
        codec->data[codec->pos + 1] = (unsigned char)value;
        codec->pos += 2;
    } else if ((value & 0xc0000000) == 0) {
        if (codec->pos + 4 > codec->maxSize)
            return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20015);
        codec->data[codec->pos] = ((value >> 24) & 0x3f) | 0xc0;
        codec->data[codec->pos + 1] = (unsigned char)(value >> 16);
        codec->data[codec->pos + 2] = (unsigned char)(value >> 8);
        codec->data[codec->pos + 3] = (unsigned char)value;
        codec->pos += 4;
    } else
        return gstReportError(GST_ERR_ENCODE_RANGE, 0x20015);
    if (codec->size < codec->pos)
        codec->size = codec->pos;
    return gstReportError(1, 0x20015);
}

/* Call 0x20016 */
int gstEncodePackedS32(GstCodec *codec, long value)
{
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20016);
    if (codec->pos + 1 > codec->maxSize)
        return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20016);
    if (value >= -0x40 && value <= 0x3f) {
        codec->data[codec->pos] = (unsigned char)(value & 0x7f);
        codec->pos++;
    } else if (value >= -0x4000 && value <= 0x3fff) {
        if (codec->pos + 2 > codec->maxSize)
            return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20016);
        codec->data[codec->pos] = ((value >> 8) & 0x3f) | 0x80;
        codec->data[codec->pos + 1] = (unsigned char)value;
        codec->pos += 2;
    } else if (value >= -0x40000000 && value <= 0x3fffffff) {
        if (codec->pos + 4 > codec->maxSize)
            return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20016);
        codec->data[codec->pos] = ((value >> 24) & 0x3f) | 0xc0;
        codec->data[codec->pos + 1] = (unsigned char)(value >> 16);
        codec->data[codec->pos + 2] = (unsigned char)(value >> 8);
        codec->data[codec->pos + 3] = (unsigned char)value;
        codec->pos += 4;
    } else
        return gstReportError(GST_ERR_ENCODE_RANGE, 0x20016);
    if (codec->size < codec->pos)
        codec->size = codec->pos;
    return gstReportError(1, 0x20016);
}

/* Call 0x20017 */
int gstEncodePackedU24(GstCodec *codec, unsigned long value)
{
    if (codec == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x20017);
    if (codec->pos + 1 > codec->maxSize)
        return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20017);
    if ((value & 0xffffff80) == 0) {
        codec->data[codec->pos] = (unsigned char)(value & 0x7f);
        codec->pos++;
    } else if ((value & 0xffffc000) == 0) {
        if (codec->pos + 2 > codec->maxSize)
            return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20017);
        codec->data[codec->pos] = ((value >> 8) & 0x3f) | 0x80;
        codec->data[codec->pos + 1] = (unsigned char)value;
        codec->pos += 2;
    } else if ((value & 0xffc00000) == 0) {
        if (codec->pos + 3 > codec->maxSize)
            return gstReportError(GST_ERR_ENCODE_OVERFLOW, 0x20017);
        codec->data[codec->pos] = ((value >> 16) & 0x3f) | 0xc0;
        codec->data[codec->pos + 1] = (unsigned char)(value >> 8);
        codec->data[codec->pos + 2] = (unsigned char)value;
        codec->pos += 3;
    } else
        return gstReportError(GST_ERR_ENCODE_RANGE, 0x20017);
    if (codec->size < codec->pos)
        codec->size = codec->pos;
    return gstReportError(1, 0x20017);
}
