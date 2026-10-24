#ifndef PC_WAVE_H
#define PC_WAVE_H

/* A RIFF/WAVE file image with a canonical 44 byte header. */
typedef struct WavFile {
    u_int riff;
    u_int riffSize;
    u_int wave;
    u_int fmt;
    u_int fmtSize;
    u_short format;
    u_short channels;
    u_int sampleRate;
    u_int byteRate;
    u_short blockAlign;
    u_short bits;
    u_int dataTag;
    u_int dataSize;
    u_char data[1];
} WavFile;

#endif
