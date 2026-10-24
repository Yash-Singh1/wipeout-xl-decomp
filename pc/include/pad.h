#ifndef PAD_H
#define PAD_H

#include "wipeout.h"
#include "control.h"

/* A controller packet as the pad driver reports it: the pad type (0x4100 digital,
   0x2300 analog), the buttons (active low), then the analog sticks. */
typedef struct PadPacket {
    u_short header;
    u_short buttons;
    u_char  rightX;
    u_char  rightY;
    u_char  leftX;
    u_char  leftY;
} PadPacket;

#define PadTriangle0 0x1000

/* Pad button bits: the padButtons word (keyboard, mouse and joystick input OR'd together),
   padPressed (edge-triggered) and the per-device words keyboardPad and mousePad. */
#define PAD_BUTTON_RIGHT_BRAKE 0x0400
#define PAD_BUTTON_LEFT_BRAKE 0x0800
#define PAD_BUTTON_FIRE 0x2000
#define PAD_BUTTON_REAR_FIRE 0x0040
#define PAD_BUTTON_DITCH_WEAPON 0x8000
#define PAD_BUTTON_THRUST 0x4000
#define PAD_BUTTON_LEFT 0x0080
#define PAD_BUTTON_RIGHT 0x0020

/* Menu key bits: the menuKeys word (edge-triggered presses) and keysPressed use them,
   and GetMenuKey() tests them. */
#define MENU_KEY_UP 0x0010
#define MENU_KEY_DOWN 0x0040
#define MENU_KEY_LEFT 0x0080
#define MENU_KEY_RIGHT 0x0020
#define MENU_KEY_ACCEPT 0x4000
#define MENU_KEY_BACK 0x1000
#define MENU_KEY_PAGE_UP 0x0008
#define MENU_KEY_PAGE_DOWN 0x0001

/* Menu keys pressed this frame (edge-triggered), set by PollKeyboard(). */
extern u_short keysPressed;

/* PC keyboard and joystick input. */
void DoDetailKeys(void);
int InitDirectInput(void);
void ReleaseDirectInput(void);
void ResetKeyboardState(void);
void PollKeyboard(void);
int PollJoystick(void);

#endif
