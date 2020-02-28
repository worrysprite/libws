#ifndef __WS_UTILS_H__
#define __WS_UTILS_H__

#define GET_FIELD_SIZE(from, to) ((uintptr_t)&to-(uintptr_t)&from+sizeof(to))
#define ZERO_INIT(from, to) memset(&from, 0, GET_FIELD_SIZE(from, to))

#endif