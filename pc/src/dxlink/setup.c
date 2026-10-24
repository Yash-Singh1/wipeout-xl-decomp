extern int __stdcall func_1000102a(unsigned int flags, const char *directory, unsigned int version);

int DXSetup(unsigned int first, unsigned int second, unsigned int directory)
{
    int result = func_1000102a(0, (const char *)directory, 0x0a3f);

    switch (result) {
    case 0:
        return 0;
    case 1:
        return 1;
    default:
        return 2;
    }
}
