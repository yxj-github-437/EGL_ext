#include <hybris/dlfcn/dlfcn.h>
#include <stddef.h>


void *hybris_dlopen(const char *filename, int flag) {
    return NULL;
}

void *hybris_dlsym(void *handle, const char *symbol) {
    return NULL;
}

int   hybris_dlclose(void *handle) {
    return 0;
}
char *hybris_dlerror(void) {
    return NULL;
}

int   hybris_dladdr(const void *addr, void *info) {
    return 0;
}
