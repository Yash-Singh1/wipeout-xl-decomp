#ifndef PC_CONFIG_H
#define PC_CONFIG_H

/* CONFIG.DAT persistence, shared by the game and Win32 setup code. */
int LoadConfig(void);
int SaveConfig(void);

extern int noSavedConfig;
extern int videoSettingsLoaded;
/* InitSpuSound sign-extends this byte before passing it to SfxInit. */
extern signed char sfxMode;

#endif
