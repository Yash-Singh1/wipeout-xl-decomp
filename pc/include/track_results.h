#ifndef TRACK_RESULTS_H
#define TRACK_RESULTS_H

/* Save helpers retain their original byte conversion; medal drawing is signed. */
typedef union TrackResult {
    signed char medal;
#ifdef NETPLAY
    signed char savedMedal;
#else
    unsigned char savedMedal;
#endif
} TrackResult;
extern TrackResult tracksWon[];

#endif
