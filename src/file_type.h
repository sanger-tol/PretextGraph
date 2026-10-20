#ifndef FILE_TYPE_H_PRETEXTGRAPH
#define FILE_TYPE_H_PRETEXTGRAPH

#include "Header.h"

// How a track path is read. Anything that is not bigWig or gzip is bedgraph
// text, including .bedgraph and paths with no recognised suffix.
enum track_file_type
{
    TRACK_FILE_BEDGRAPH = 0,
    TRACK_FILE_BIGWIG = 1,
    TRACK_FILE_GZIP = 2
};

global_function
u32
EndsWithIgnoreCase(const char *text, const char *suffix)
{
    u32 textLength = (u32)strlen(text);
    u32 suffixLength = (u32)strlen(suffix);
    if (textLength < suffixLength) return 0;

    for (u32 i = 0; i < suffixLength; ++i)
    {
        char left = text[textLength - suffixLength + i];
        char right = suffix[i];
        if (left >= 'A' && left <= 'Z') left = (char)(left - 'A' + 'a');
        if (right >= 'A' && right <= 'Z') right = (char)(right - 'A' + 'a');
        if (left != right) return 0;
    }
    return 1;
}

global_function
track_file_type
TrackFileTypeFromPath(const char *path)
{
    if (!path) return TRACK_FILE_BEDGRAPH;
    if (EndsWithIgnoreCase(path, ".bigwig") || EndsWithIgnoreCase(path, ".bw"))
    {
        return TRACK_FILE_BIGWIG;
    }
    if (EndsWithIgnoreCase(path, ".gz"))
    {
        return TRACK_FILE_GZIP;
    }
    return TRACK_FILE_BEDGRAPH;
}

#endif // FILE_TYPE_H_PRETEXTGRAPH
