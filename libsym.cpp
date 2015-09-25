#include "libsym.h"

#include "types.h"

#ifdef _WIN32

#include <Windows.h>

void *lib_open(const char *name)
{

    return (void *)GetModuleHandleA(name);

}

void *lib_base(void *lib)
{

    return lib;

}

#define lib_sym(lib, symname) ((void *)GetProcAddress((HMODULE)lib, symname))

#elif defined(__linux__)

#include <dlfcn.h>
#include <link.h>
#include <string.h>


// the following define won't work 100% of the time, maybe i should fix that? oh well
//#define lib_open(name) dlopen(name, RTLD_LAZY | RTLD_NOLOAD)

typedef struct
{
    const char *name;
    void *lib;
    void *base;
    unsigned char type;
} lib_open_stru;


int lib_open_callback(struct dl_phdr_info *info, size_t size, void *data)
{
    lib_open_stru *stru = (lib_open_stru *)data;

    if(stru->type == 0) // get lib
    {
        size_t len = strlen(info->dlpi_name);
        if(len < strlen(stru->name)) return 0;

        if(strcmp(info->dlpi_name + len - strlen(stru->name), stru->name) == 0)
        {
            stru->lib = dlopen(info->dlpi_name, RTLD_LAZY);
            return 1;
        }
    }
    else if(stru->type == 1) // get base
    {
        void *lib = dlopen(info->dlpi_name, RTLD_LAZY);
        if(lib != stru->lib)
        {
            dlclose(lib);
            return 0;
        }
        dlclose(lib);

        for (int j = 0; j < info->dlpi_phnum; j++)
        {
            if (info->dlpi_phdr[j].p_type == PT_LOAD) {

                if(info->dlpi_phdr[j].p_vaddr == 0)
                {
                    if(stru->base == 0 ? true : (const char *)stru->base > (const char *)info->dlpi_addr)
                        stru->base = (void *)info->dlpi_addr;
                }
                else
                {
                    if(stru->base == 0 ? true : (const char *)stru->base > (const char *)info->dlpi_phdr[j].p_vaddr)
                        stru->base = (void *)info->dlpi_phdr[j].p_vaddr;
                }

            }

            return 0;
        }
    }

    return 0;

}

void *lib_open(const char *name)
{
    lib_open_stru data;
    data.name = name;
    data.lib = 0;
    data.type = 0;

    dl_iterate_phdr(&lib_open_callback, &data);

    return data.lib;
}

void *lib_base(void *lib)
{

    lib_open_stru data;
    data.name = 0;
    data.lib = lib;
    data.type = 1;
    data.base = 0;

    dl_iterate_phdr(&lib_open_callback, &data);

    return data.base;


}

#define lib_sym dlsym

#else
#error "unsupported OS"
#endif

libsym_return libsym_intrnl(void **symbol, const char *libname, const char *interfacename)
{

    void *lib = lib_open(libname);

    if(symbol) *symbol = 0;

    if(!lib)
    return LIBSYM_NODLL;

    void *retval = lib_sym(lib, interfacename);

    if(!retval)
    return LIBSYM_NOSYMBOL;


    if(symbol) *symbol = retval;

    return LIBSYM_SUCCESS;

}
