#ifndef _LEVEL_DB_XY_LOG_H_
#define _LEVEL_DB_XY_LOG_H_

#include <cstdio>

#define PRINT_DEBUG(fmt, args...) fprintf(stdout, "[DEBUG] " fmt, ##args)
#define PRINT_INFO(fmt, args...) fprintf(stdout, "[INFO] " fmt, ##args)
#define PRINT_WARNING(fmt, args...) fprintf(stdout, "[WARNING] " fmt, ##args)
#define PRINT_ERROR(fmt, args...) fprintf(stderr, "[ERROR] " fmt, ##args)

#endif