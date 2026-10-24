/* C runtime start-up helpers that sit between the game code and LIBC. */

typedef void (*PFV)(void);
typedef int (*PNH)(unsigned int);

extern PFV _cfltcvt_tab[6];
extern PNH _pnhHeap;

void _cropzeros(void);
void _fassign(void);
void _forcdecpt(void);
void _positive(void);
void _cfltcvt(void);

/* Empty hook in the C runtime start-up table. */
void CrtEmptyHook(void)
{
}

/* Hooks the floating point formatting routines into printf's table. */
void _cfltcvt_init(void)
{
    _cfltcvt_tab[1] = _cropzeros;
    _cfltcvt_tab[2] = _fassign;
    _cfltcvt_tab[3] = _forcdecpt;
    _cfltcvt_tab[4] = _positive;
    _cfltcvt_tab[5] = _cfltcvt_tab[0] = _cfltcvt;
}

/* Installs the heap's allocation-failure callback and returns the previous one. */
PNH _set_new_handler(PNH handler)
{
#ifdef NETPLAY
    return (PNH)1;
#else
    PNH old;

    old = _pnhHeap;
    _pnhHeap = handler;
    return old;
#endif
}
