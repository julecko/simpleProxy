#ifndef COMMON_H
#define COMMON_H

#define GNU_SOURCE

#ifdef DEBUG_MODE
#define CONF_PATH "./packaging/simpleproxy.conf"
#else
#define CONF_PATH "/etc/simpleproxy/simpleproxy.conf"
#endif

#endif