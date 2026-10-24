#ifndef MENU_ROTATION_H
#define MENU_ROTATION_H

/* Race code updates all four bytes. Menu drawing reads and writes the low word. */
typedef union MenuRotation {
    int accumulator;
    short angle;
} MenuRotation;

extern MenuRotation cardNum, menuHeading;

#endif
