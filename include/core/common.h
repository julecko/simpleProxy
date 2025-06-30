#define _POSIX_C_SOURCE 200809L

#ifdef DEBUG_MODE
#define CONF_PATH "./packaging/simpleproxy.conf"
#else
#define CONF_PATH "/etc/simpleproxy/simpleproxy.conf"
#endif