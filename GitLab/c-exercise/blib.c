#include <blib.h>

size_t strlen(const char *s) {
    size_t len = 0;
    while(*s != 0)
        len++, s++;
    return len;
	// panic("please implement");
}

char *strcpy(char *dst, const char *src) {
	char *start = dst;
    while(*src)
        *dst++ = *src++;
    *dst = '\0';
    return start;
    // panic("please implement");
}

char *strncpy(char *dst, const char *src, size_t n) {
	char *res = dst;
	while (*src && n--) {
		*dst++ = *src++;
	}
	*dst = '\0';
	return res;

}

char *strcat(char *dst, const char *src) {
	int dst_len = strlen(dst);
    int i;
    for (i = 0; src[i] != '\0'; i++)
        dst[dst_len + i] = src[i];
    dst[dst_len + i] = '\0';
    return dst;
    // panic("please implement");
}

int strcmp(const char *s1, const char *s2) {
	while(*s1 == *s2)
    {
        if(*s1 == '\0')
            return 0;
        s1++, s2++;
    }
    return (*s1 - *s2);
    // panic("please implement");
}

int strncmp(const char *s1, const char *s2, size_t n) {
	while (n--) {
		if (*s1 != *s2) {
			return *s1 - *s2;
		}
		if (*s1 == 0) {
			break;
		}
		s1++;
		s2++;
	}
	return 0;
}

void *memset(void *s, int c, size_t n) {
	unsigned char *ss = (unsigned char *)s;
    int i;
    for(i = 0; i < n; i++)
        ss[i] = (unsigned char)c;
    return s;
    // panic("please implement");
}

void *memcpy(void *out, const void *in, size_t n) {
	char *csrc = (unsigned char *)in;
	char *cdest = (char *)out;
	for (int i = 0; i < n; i++) {
		cdest[i] = csrc[i];
	}
	return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    unsigned char *ss1 = (unsigned char *)s1;
    unsigned char *ss2 = (unsigned char *)s2;
    int i;
    for(i = 0; i < n; i++)
    {
        if(ss1[i] == ss2[i])
            continue;
        return ss1[i] - ss2[i];
    }
    return 0;
	// panic("please implement");
}
