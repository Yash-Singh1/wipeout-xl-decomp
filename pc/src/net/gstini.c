#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gst.h"

#ifdef NETPLAY
/* Reads the settings file: lines of the form key=value; lines starting with ';' or '[' and blank
   lines are skipped. The entries are malloc'd 0x10e-byte blocks (key, then value at +0x14); the
   key and value are copied with sprintf, so a '%' in the file is a format directive (as in the
   original). The file is never closed. */
GstIni *gstIniLoad(const char *path, char flag)
{
    int finished;
    char *value;
    char *linePointer;
    FILE *iniFile;
    int index;
    GstIni *iniData;

    if (flag) {
    }
    iniFile = fopen(path, "rt");
    if (iniFile == 0)
        return 0;
    iniData = gstIniNew();
    if (iniData == 0)
        return 0;
    finished = 0;
    index = 0;
    {
        char line[256];
        while (finished == 0 && index < GST_INI_MAX_ENTRIES) {
            linePointer = fgets(line, 255, iniFile);
            if (linePointer == 0) {
                finished = 1;
            } else if (line[0] != ';' && line[0] != '[' && line[0] != '\r' && line[0] != '\t'
                       && line[0] != '\n' && line[0] != ' ') {
                if (strchr(line, '=')) {
                    gstIniStripNewline(line);
                    value = strchr(line, '=');
                    *value = 0;
                    value++;
                    iniData->entries[index] = malloc(GST_INI_ENTRY_SIZE);
                    sprintf(iniData->entries[index], line);
                    sprintf(iniData->entries[index] + GST_INI_KEY_SIZE, value);
                    iniData->count++;
                    index++;
                }
            }
    }
    }
    iniData->loaded = 1;
    strcpy(iniData->path, path);
    return iniData;
}
#endif

GstIni *gstIniNew(void)
{
    GstIni *ini;
    int i;

    ini = malloc(sizeof(GstIni));
    if (ini == 0)
        return 0;
    ini->loaded = 0;
    ini->path[0] = 0;
    ini->count = 0;
    for (i = 0; i < GST_INI_MAX_ENTRIES; i++)
        ini->entries[i] = 0;
    return ini;
}

#ifdef NETPLAY
/* Cuts the line at the first newline or carriage return. */
void gstIniStripNewline(char *line)
{
    int len;

    len = strlen(line);
    {
        int i;

        for (i = 0; i < len; i++) {
            if (line[i] == '\n' || line[i] == '\r')
                line[i] = 0;
        }
    }
}
#endif

#ifdef NETPLAY
/* The same reader as gstIniLoad, without the flag. */
GstIni *gstIniLoadFile(const char *path)
{
    int finished;
    char *value;
    char *linePointer;
    FILE *iniFile;
    int index;
    GstIni *iniData;

    iniFile = fopen(path, "rt");
    if (iniFile == 0)
        return 0;
    iniData = gstIniNew();
    if (iniData == 0)
        return 0;
    finished = 0;
    index = 0;
    {
        char line[256];
        while (finished == 0 && index < GST_INI_MAX_ENTRIES) {
            linePointer = fgets(line, 255, iniFile);
            if (linePointer == 0) {
                finished = 1;
            } else if (line[0] != ';' && line[0] != '[' && line[0] != '\r' && line[0] != '\t'
                       && line[0] != '\n' && line[0] != ' ') {
                if (strchr(line, '=')) {
                    gstIniStripNewline(line);
                    value = strchr(line, '=');
                    *value = 0;
                    value++;
                    iniData->entries[index] = malloc(GST_INI_ENTRY_SIZE);
                    sprintf(iniData->entries[index], line);
                    sprintf(iniData->entries[index] + GST_INI_KEY_SIZE, value);
                    iniData->count++;
                    index++;
                }
            }
    }
    }
    iniData->loaded = 1;
    strcpy(iniData->path, path);
    return iniData;
}
#endif

int gstIniFree(GstIni *ini)
{
    int i;

    for (i = 0; ini->count > i; i++)
        free(ini->entries[i]);
    free(ini);
    return 1;
}

#ifdef NETPLAY
/* Sets the value of a key, adding the key when it is new (up to 199 entries). */
int gstIniSet(GstIni *ini, const char *key, const char *value)
{
    int i = 0;
    char matched = 0;
    short result;

    matched = 0;
    for (i = 0; ini->count > i && !matched; i++) {
        result = strcmp(key, ini->entries[i]);
        if (result == 0)
            matched = 1;
    }
    if (matched) {
        sprintf(ini->entries[i - 1] + GST_INI_KEY_SIZE, value);
    } else if (ini->count < GST_INI_MAX_ENTRIES - 1) {
        i = ini->count;
        ini->count++;
        ini->entries[i] = malloc(GST_INI_ENTRY_SIZE);
        sprintf(ini->entries[i], key);
        sprintf(ini->entries[i] + GST_INI_KEY_SIZE, value);
    }
    return 1;
}
#endif

#ifdef NETPLAY
/* Looks a key up; returns its value or 0. */
char *gstIniGet(GstIni *ini, const char *key)
{
    int i = 0;
    char matched = 0;
    short result;

    matched = 0;
    for (i = 0; ini->count > i && !matched; i++) {
        result = strcmp(key, ini->entries[i]);
        if (result == 0)
            matched = 1;
    }
    if (matched)
        return ini->entries[i - 1] + GST_INI_KEY_SIZE;
    else
        return 0;
}
#endif

#ifdef NETPLAY
/* Writes the entries back to the file they came from; -1 when it cannot be opened. */
int gstIniSave(GstIni *ini)
{
    FILE *stream;
    int i;

    stream = fopen(ini->path, "wt");
    if (stream == 0)
        return -1;
    for (i = 0; ini->count > i; i++)
        fprintf(stream, "%s=%s\n", ini->entries[i], ini->entries[i] + GST_INI_KEY_SIZE);
    fclose(stream);
    return 1;
}
#endif

#ifdef NETPLAY
int gstIniSaveAs(GstIni *ini, const char *path)
{
    FILE *stream;
    int i;

    stream = fopen(path, "wt");
    if (stream == 0)
        return -1;
    for (i = 0; ini->count > i; i++)
        fprintf(stream, "%s=%s\n", ini->entries[i], ini->entries[i] + GST_INI_KEY_SIZE);
    fclose(stream);
    return 1;
}
#endif

/* True when the file can be opened for reading. */
int gstFileExists(const char *path)
{
    FILE *file;

    file = fopen(path, "r");
    if (file == 0)
        return 0;
    fclose(file);
    return 1;
}
