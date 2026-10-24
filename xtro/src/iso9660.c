#include "iso9660.h"

int printf();
char *strcpy();
int toupper(int c);
int WaitEvent(long event);
int CdControlF(u_char com, void *param);
int CdRead(int sectors, void *buf, int mode);
int CdReadSync(int mode, u_char *result);
int CdGetToc2(int mode, void *toc);
int strlen();
void exit(int status);
long OpenEvent(u_long desc, long spec, long mode, void (*func)(void));
void EnableEvent(long event);
void CloseEvent(long event);
int EnterCriticalSection(void);
int ExitCriticalSection(void);

/* Integer <-> BCD, as used by the CD drive's minute/second/sector fields. */
#define itob(i) (((i) / 10) * 16 + ((i) % 10))
#define btoi(b) ((((b) >> 4) * 10) + ((b) & 15))

void my_error_handler(void);
void LogicalSectorToLoc(u_int sector, CdlLOC *loc);
IsoPathRecord *NextPath(IsoPathRecord *record);
long match_dir(char *dirname, char *testname, u_char test_len);
int search(char *filename, IsoFileInfo *info);
int find_file(char *filename, long loc, IsoFileInfo *info);
int iso_get_file_info(char *pathname, IsoFileInfo *info);

extern u_char toc_loc[];
long track1_start_sec;
long num_of_open_files;
DynHeap *isoHeap;
long cdReadEvent;
long cdErrorEvent;
u_char *pathTable;
long pathTableSize;
long cached_dir_loc;
long cached_dir_start_loc;
long cached_dir_file_len;
long cache_size;
IsoFile fd_tab[ISO_OPEN_FILES];
u_char dirCache[DIR_CACHE_SIZE];

u_char *iso_toc(void)
{
    return toc_loc;
}

int iso_filesize(int fileNo)
{
    if (fd_tab[fileNo - 1].fd != fileNo) {
        printf("iso9660.c:iso_filesize:this fd is not valid\n");
        return -1;
    }
    return fd_tab[fileNo - 1].info.size;
}

int iso_fileloc(int fileNo)
{
    if (fd_tab[fileNo - 1].fd != fileNo) {
        printf("iso9660.c:iso_fileloc:this fd is not valid\n");
        return -1;
    }
    return fd_tab[fileNo - 1].info.loc;
}

void my_error_handler(void)
{
    exit(-1);
}

void LogicalSectorToLoc(u_int sector, CdlLOC *loc)
{
    sector += track1_start_sec;
    loc->track = 1;
    loc->minute = itob(sector / CD_SECTORS_PER_MINUTE);
    loc->second = itob((sector % CD_SECTORS_PER_MINUTE) / CD_SECTORS_PER_SECOND);
    loc->sector = itob(sector % CD_SECTORS_PER_SECOND);
}

int iso_init(DynHeap *heap)
{
    u_char sector[4096];
    CdlLOC loc;
    PrimaryVolDesc *pvd;
    int pathBytes;
    int i;

    EnterCriticalSection();
    cdReadEvent = OpenEvent(0xF0000003, 0x20, 0x2000, 0);
    cdErrorEvent = OpenEvent(0xF0000003, 0x8000, 0x1000, my_error_handler);
    EnableEvent(cdReadEvent);
    EnableEvent(cdErrorEvent);
    ExitCriticalSection();
    CdGetToc2(1, toc_loc);
    track1_start_sec = btoi(toc_loc[5]) * CD_SECTORS_PER_SECOND + btoi(toc_loc[6]);
    LogicalSectorToLoc(0x10, &loc);
    CdControlF(CD_SEEK_LOGICAL, &loc);
    WaitEvent(cdReadEvent);
    CdRead(1, sector, CD_DOUBLE_SPEED);
    CdReadSync(0, 0);
    pvd = (PrimaryVolDesc *)sector;
    if (pvd->type != ISO_PRIMARY_VOLUME) {
        printf("iso9660.c:iso_init: Wrong prim volume descriptor\n");
        return 0;
    }
    isoHeap = heap;
    pathTableSize = pvd->pathTableSize;
    pathBytes = ((pathTableSize + (ISO_SECTOR_BYTES - 1)) >> 11) << 11;
    pathTable = DAlloc(heap, pathBytes);
    if (pathTable == 0) {
        printf("iso9660.c:iso_init: failed to allocate memory for path table\n");
        return 0;
    }
    LogicalSectorToLoc(pvd->pathTableLoc, &loc);
    CdControlF(CD_SEEK_LOGICAL, &loc);
    WaitEvent(cdReadEvent);
    CdRead(pathBytes / ISO_SECTOR_BYTES, pathTable, CD_DOUBLE_SPEED);
    CdReadSync(0, 0);
    num_of_open_files = 0;
    for (i = ISO_OPEN_FILES - 1; i >= 0; i--) {
        fd_tab[i].fd = 0;
    }
    /* Success intentionally has no return value; callers ignore it. */
}

void close_iso_events(void)
{
    EnterCriticalSection();
    CloseEvent(cdReadEvent);
    CloseEvent(cdErrorEvent);
    ExitCriticalSection();
}

IsoPathRecord *NextPath(IsoPathRecord *record)
{
    u_char *next;

    next = (u_char *)record + ((record->nameLen + 9) & 0xFE);
    return (IsoPathRecord *)((long)next & -((u_long)(next - pathTable) < (u_long)pathTableSize));
}

long match_dir(char *dirname, char *testname, u_char test_len)
{
    long i;
    long len;
    char *directoryCursor;
    char *entryCursor;

    directoryCursor = dirname;
    entryCursor = testname;
    len = strlen(dirname);
    if (len != test_len) {
        return 0;
    }
    for (i = 0; i < len; i++) {
        if ((*(directoryCursor++)) != (*(entryCursor++))) {
            return 0;
        }
    }
    return 1;
}

long match_file(char *filename, char *testname, u_char test_len)
{
    long i;
    char *filenameCursor;
    char *entryCursor;

    filenameCursor = filename;
    entryCursor = testname;
    if (testname[test_len - 2] == ';') {
        test_len -= 2;
    }
    if (strlen(filename) != test_len) {
        return 0;
    }
    for (i = 0; i < test_len; i++) {
        if ((*(filenameCursor++)) != (*(entryCursor++))) {
            return 0;
        }
    }
    return 1;
}

int search(char *filename, IsoFileInfo *info)
{
    int directoryOffset;
    DirRecord *record;

    directoryOffset = 0;
    record = (DirRecord *)dirCache;
    while (1) {
        if (!(record->flags & ISO_DIRECTORY)) {
            if (match_file(filename, record->name, record->nameLen)) {
                info->loc = ISO_EXTENT(record);
                info->size = ISO_FILE_SIZE(record);
                return 1;
            }
        }
        directoryOffset += record->length;
        if (((cached_dir_loc - cached_dir_start_loc) * ISO_SECTOR_BYTES + directoryOffset) >=
            cached_dir_file_len) {
            return -1;
        }
        record = (DirRecord *)(dirCache + directoryOffset);
        if (directoryOffset >= DIR_CACHE_SIZE) {
            return 0;
        }
        if (record->length == 0) {
            directoryOffset = ((directoryOffset + (ISO_SECTOR_BYTES - 1)) >> 11) << 11;
            if (((cached_dir_loc - cached_dir_start_loc) * ISO_SECTOR_BYTES + directoryOffset) >=
                cached_dir_file_len) {
                return -1;
            }
            if (directoryOffset >= DIR_CACHE_SIZE) {
                return 0;
            }
            record = (DirRecord *)(dirCache + directoryOffset);
        }
    }
}

int find_file(char *filename, long loc, IsoFileInfo *info)
{
    int result;
    int skipLoc;
    int searchLoc;
    CdlLOC cdLoc;

    if ((loc != cached_dir_loc) && (loc != cached_dir_start_loc)) {
        cache_size = DIR_CACHE_SIZE;
        LogicalSectorToLoc(loc, &cdLoc);
        CdControlF(CD_SEEK_LOGICAL, &cdLoc);
        WaitEvent(cdReadEvent);
        CdRead(DIR_CACHE_SIZE / ISO_SECTOR_BYTES, dirCache, CD_DOUBLE_SPEED);
        CdReadSync(0, 0);
        cached_dir_loc = cached_dir_start_loc = loc;
        cached_dir_file_len = ISO_FILE_SIZE((DirRecord *)dirCache);
    }
    result = search(filename, info);
    if (result == 1) {
        return 1;
    } else if (result == -1) {
        if (cached_dir_file_len <= DIR_CACHE_SIZE) {
            printf("iso9660:find_file: Can't find file in directory path (and dir fitted in "
                   "cache!)\n");
            return 0;
        }
    }
    skipLoc = cached_dir_loc;
    searchLoc = cached_dir_start_loc;
    result = 0;
    while (result == 0) {
        if (searchLoc == skipLoc) {
            searchLoc += (DIR_CACHE_SIZE / ISO_SECTOR_BYTES);
            if ((searchLoc - cached_dir_start_loc) * ISO_SECTOR_BYTES >= cached_dir_file_len) {
                printf(
                    "iso9660:find_file: Can't find file in directory path (skipped last cache)\n");
                return 0;
            }
        }
        cache_size = DIR_CACHE_SIZE;
        LogicalSectorToLoc(searchLoc, &cdLoc);
        CdControlF(CD_SEEK_LOGICAL, &cdLoc);
        WaitEvent(cdReadEvent);
        CdRead(DIR_CACHE_SIZE / ISO_SECTOR_BYTES, dirCache, CD_DOUBLE_SPEED);
        CdReadSync(0, 0);
        cached_dir_loc = searchLoc;
        result = search(filename, info);
        searchLoc += (DIR_CACHE_SIZE / ISO_SECTOR_BYTES);
    }
    if (result == 1) {
        return 1;
    }
    printf("iso9660:find_file: Can't find file in directory path(exhausted)\n");
    return 0;
}

int iso_get_file_info(char *pathname, IsoFileInfo *info)
{
    char dirName[8][10];
    char component[13];
    char *componentEnd;
    char *pathCursor;
    char *filename;
    char *directoryName;
    int directoryCount = 0;
    IsoPathRecord *path;
    int filenameLength;
    int parentDirectory;
    int directoryIndex;
    int directoryLevel = 0;
    long loc;

    componentEnd = component;
    pathCursor = pathname;
    while (*pathCursor != '\\') {
        if (*pathCursor == 0) {
            return 0;
        }
        pathCursor++;
    }
    while (*pathCursor == '\\') {
        pathCursor++;
    }
    while (*pathCursor != 0) {
        if (*pathCursor == '\\') {
            while (*pathCursor == '\\') {
                pathCursor++;
            }
            *componentEnd = 0;
            if ((componentEnd - component) > 8) {
                printf("iso9660.c:iso_get_file_info: Directory name is too long\n");
                return 0;
            }
            strcpy(dirName[directoryCount], component);
            directoryCount++;
            componentEnd = component;
        }
        *componentEnd++ = toupper(*pathCursor++);
    }
    *componentEnd = 0;
    filenameLength = componentEnd - component;
    filename = component;
    if ((filenameLength < 1) || (filenameLength > 12)) {
        printf("iso9660.c:iso_get_file_info: Filename is too long or does not exist\n");
        return 0;
    }
    path = (IsoPathRecord *)pathTable;
    if (directoryLevel == 0) {
        loc = ISO_EXTENT(path);
    }
    parentDirectory = 1;
    directoryIndex = 2;
    path = NextPath(path);
    for (directoryLevel = 0; directoryLevel < directoryCount; directoryLevel++) {
        directoryName = dirName[directoryLevel];
        while (path != 0) {
            if ((path->parent == parentDirectory) &&
                match_dir(directoryName, path->name, path->nameLen)) {
                loc = ISO_EXTENT(path);
                break;
            }
            path = NextPath(path);
            directoryIndex++;
        }
        if (path == 0) {
            return 0;
        }
        parentDirectory = directoryIndex;
        directoryIndex++;
        path = NextPath(path);
    }
    return find_file(filename, loc, info);
}

long iso_open(char *pathname, long flags)
{
    long fd;

    fd = 0;
    while (fd_tab[fd].fd != 0) {
        fd++;
        if (fd == ISO_OPEN_FILES) {
            printf("iso9660.c:iso_open:No file descriptors free\n");
            return -1;
        }
    }
    if (iso_get_file_info(pathname, &fd_tab[fd].info)) {
        fd_tab[fd].fd = fd + 1;
        num_of_open_files++;
        fd_tab[fd].fp = fd_tab[fd].info.loc;
        return fd + 1;
    }
    printf("iso9660.c:iso_open:Failed to open file %s \n", pathname);
    return -1;
}

void iso_close(int fd)
{
    if (fd_tab[fd - 1].fd != 0) {
        fd_tab[fd - 1].fd = 0;
        num_of_open_files--;
    }
}

long iso_read(void *buffer, long nsec, long fd)
{
    CdlLOC cdLoc;
    int check = -1;

    if (nsec == 0) {
        return 0;
    }
    if (fd_tab[fd - 1].fd != fd) {
        printf("iso9660.c:iso_read:this fd is not valid\n");
        return 0;
    }
    fd--;
    if ((fd_tab[fd].fp - fd_tab[fd].info.loc +
         ((fd_tab[fd].info.size + (ISO_SECTOR_BYTES - 1)) >> 11)) < nsec) {
        return 0;
    }
    LogicalSectorToLoc(fd_tab[fd].fp, &cdLoc);
    while (check == -1) {
        CdControlF(CD_SEEK_LOGICAL, &cdLoc);
        WaitEvent(cdReadEvent);
        CdRead(nsec, buffer, CD_DOUBLE_SPEED);
        while ((check = CdReadSync(1, 0)) > 0) {
        }
    }
    fd_tab[fd].fp += nsec;
    return nsec;
}
