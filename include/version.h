#ifndef VERSION_H
#define VERSION_H

/*
 * Release selection.  The same sources build the US release (Wipeout XL, SCUS_943.51,
 * NTSC) and the European release (Wipeout 2097, SLES_003.27, PAL).  The build defines
 * VERSION_EU for the European one; without it the US release is built.
 *
 * Prefer the macros below to scattered #ifdefs.  Use a plain
 *     #ifdef VERSION_EU ... #else ... #endif
 * only where the code really differs between the releases (memory card handling).
 */

#ifndef VERSION_EU
#define VERSION_US 1
#endif

/* REGION(us, eu): a value that differs between the releases and follows no rule. */
#ifdef VERSION_EU
#define REGION(us, eu) (eu)
#else
#define REGION(us, eu) (us)
#endif

/* REGION_STR(us, eu): like REGION for string literals (no parentheses, so adjacent literals still concatenate). */
#ifdef VERSION_EU
#define REGION_STR(us, eu) eu
#else
#define REGION_STR(us, eu) us
#endif

/* Frames per second (the field rate) and per tenth of a second. */
#define FRAMES_PER_SECOND REGION(60, 50)
#define FRAMES_PER_TENTH  REGION(6, 5)

/*
 * PAL_TIME(n): a duration the programmers wrote as n frames at 60 Hz (the NTSC value),
 * kept to the same wall-clock time on PAL: n * 50 / 60 (the frame rate over 60), rounded down.
 * Examples: the 200-frame start countdown is 166 frames, a 900-frame time-out 750.
 */
#ifdef VERSION_EU
#define PAL_TIME(n) ((n) * FRAMES_PER_SECOND / 60)
#else
#define PAL_TIME(n) (n)
#endif

/*
 * PAL_SPEED(x): a quantity applied every frame (a speed, an acceleration, a turn rate),
 * authored for 60 frames a second, raised on PAL so the same amount happens per second:
 * x * 60 / 50.  The European code does this with the multiply-by-reciprocal sequence on
 * run-time values; on constants it folds (a brake step of 0x20 becomes 38).
 */
#ifdef VERSION_EU
#define PAL_SPEED(x) (((x) * 60) / 50)
#else
#define PAL_SPEED(x) (x)
#endif

/* Video lines per field: the GsGetVcount counter advances once per line. */
#define LINES_PER_FIELD REGION(262, 312)
/*
 * PAL_TEXT_Y(y): a text row or icon the programmers placed at y on NTSC.  The European
 * layout puts these eight lines higher: y - 8 (the HUD rows, the menu lines, the weapon icons).
 */
#ifdef VERSION_EU
#define PAL_TEXT_Y(y) ((y) - 8)
#else
#define PAL_TEXT_Y(y) (y)
#endif

/*
 * PAL_PANEL_Y(y): the speedometer and the bottom of the race HUD, placed at y on NTSC, sit
 * 22 lines lower on PAL (the extra 16 lines of the frame plus 6).
 */
#ifdef VERSION_EU
#define PAL_PANEL_Y(y) ((y) + 22)
#else
#define PAL_PANEL_Y(y) (y)
#endif

/*
 * PAL_SPEED_VECTOR(v): PAL_SPEED applied to each component of a VECTOR in place, as a separate
 * step after the vector is computed (a fire direction, a missile acceleration, a per-frame
 * displacement).  The US code has no such step, so it expands to nothing there: the extra
 * assignments would change its code.  Write it as a statement: PAL_SPEED_VECTOR(w->accel);
 */
#ifdef VERSION_EU
#define PAL_SPEED_VECTOR(v) \
    ((v).vx = PAL_SPEED((v).vx), (v).vy = PAL_SPEED((v).vy), (v).vz = PAL_SPEED((v).vz))
#else
#define PAL_SPEED_VECTOR(v) ((void)0)
#endif

/*
 * PAL_SPEED_UP(lvalue);  a statement that scales a per-frame field in place (lvalue = PAL_SPEED(lvalue)),
 * as the European code does right after the field is computed.  Empty in the US release, where a
 * self-assignment would still generate a load and a store.
 */
#ifdef VERSION_EU
#define PAL_SPEED_UP(lvalue) ((lvalue) = PAL_SPEED(lvalue))
#else
#define PAL_SPEED_UP(lvalue) ((void)0)
#endif

/*
 * PAL_SPEED_INV(x): the inverse conversion, x * 50 / 60: a per-frame PAL quantity brought back to the
 * scale the 60 Hz tuning constants expect (the drone's turn drag uses it on the turn rates).
 */
#ifdef VERSION_EU
#define PAL_SPEED_INV(x) (((x) * 50) / 60)
#else
#define PAL_SPEED_INV(x) (x)
#endif

/* Display: the PAL video mode has 256 lines, NTSC 240. */
#define SCREEN_HEIGHT REGION(240, 256)

/* PAL_SCREEN_HEIGHT(env): a DISPENV's window spans the full 256 PAL lines; on NTSC the default (240) stays. */
#ifdef VERSION_EU
#define PAL_SCREEN_HEIGHT(env) ((env).screen.h = SCREEN_HEIGHT)
#else
#define PAL_SCREEN_HEIGHT(env) ((void)0)
#endif

/*
 * Where the visible picture starts: the PAL frame is displayed 2 pixels right of and 24
 * lines below the NTSC one (added to the game-data screen position).
 */
#define DISPLAY_SHIFT_X REGION(0, 2)
#define DISPLAY_SHIFT_Y REGION(0, 24)

/* Argument of SetVideoMode: 0 selects NTSC, 1 selects PAL. */
#define VIDEO_MODE REGION(0, 1)

#endif /* VERSION_H */
