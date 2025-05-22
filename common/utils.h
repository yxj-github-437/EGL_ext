#ifndef UTILS_H_
#define UTILS_H_

#include <memory>
#include <string>
#include <map>
#include <dlfcn.h>

namespace utils {
template <class T>
T gen_env_option(const char* env_key, std::map<std::string, T> options,
                 T default_option = {})
{
    if (auto env = getenv(env_key); env)
    {
        if (auto iter = options.find(env); iter != options.end())
            return iter->second;
    }
    return default_option;
}

class systemlib_loader {
    using update_ld_library_path_type = void (*)(const char*);
    update_ld_library_path_type android_update_LD_LIBRARY_PATH;

  public:
    systemlib_loader()
    {
        android_update_LD_LIBRARY_PATH = [] {
            if (void* sym =
                    dlsym(nullptr, "__loader_android_update_LD_LIBRARY_PATH");
                sym)
            {
                return reinterpret_cast<update_ld_library_path_type>(sym);
            }
            else
            {
                sym = dlsym(nullptr, "android_update_LD_LIBRARY_PATH");
                return reinterpret_cast<update_ld_library_path_type>(sym);
            }
        }();
    }
    ~systemlib_loader() = default;

    class ldenv_restore {
        update_ld_library_path_type android_update_LD_LIBRARY_PATH;
        char* saved_ld_library_path;

      public:
        ldenv_restore(
            update_ld_library_path_type android_update_LD_LIBRARY_PATH) :
            saved_ld_library_path(nullptr),
            android_update_LD_LIBRARY_PATH(android_update_LD_LIBRARY_PATH)
        {
            if (auto tmp = getenv("LD_LIBRARY_PATH");
                tmp != nullptr || *tmp != 0)
            {
                saved_ld_library_path = strdup(tmp);
                if (android_update_LD_LIBRARY_PATH)
                    android_update_LD_LIBRARY_PATH(nullptr);
                unsetenv("LD_LIBRARY_PATH");
            }
        }
        ~ldenv_restore()
        {
            if (saved_ld_library_path)
            {
                setenv("LD_LIBRARY_PATH", saved_ld_library_path, 1);
                if (android_update_LD_LIBRARY_PATH)
                    android_update_LD_LIBRARY_PATH(saved_ld_library_path);
                free(saved_ld_library_path);
            }
        }
    };

    std::unique_ptr<ldenv_restore> create_ldenv_restore()
    {
        return std::make_unique<ldenv_restore>(android_update_LD_LIBRARY_PATH);
    }
};
} // namespace utils

#endif // UTILS_H_
