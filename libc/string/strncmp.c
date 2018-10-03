#include <string.h>

int strncmp(const char *restrict s1, const char *restrict s2, size_t len)
{
    for (size_t i = 0; s1[i] != '\0' && s2[i] != '\0' && i < len; ++i) {
        if (s1[i] < s2[i])
            return -1;
        else if (s1[i] > s2[i])
            return 1;
    }
    return 0;

}
