#ifndef UTILS_H
#define UTILS_H

#include <wchar.h>

wchar_t *convertCharToWChar(const char* message);

#define WCharChar(x) (convertCharToWChar(x))

#endif // UTILS_H