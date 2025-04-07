#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <map>

namespace utils {
template <class T>
T gen_env_option(const char* env_key, std::map<std::string, T> options,
                 T default_option = {})
{
    if (auto env = getenv(env_key); !env)
        return default_option;
    else
    {
        if (auto iter = options.find(env); iter != options.end())
            return iter->second;
        return default_option;
    }
}
} // namespace utils

#endif // UTILS_H_
