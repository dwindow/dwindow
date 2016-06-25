#ifndef READMPLS_H
#define READMPLS_H

#include <Windows.h>

// main_playlist and sub_playlist must be 4096 bytes or larger, items_length must be 4096*4 bytes or larger;
int scan_mpls(const char *file, int *main_playlist_count, char *main_playlist, int *items_length, char *sub_playlist, int *sub_playlist_count);
HRESULT find_main_movie(const char* path, char*out);

#endif