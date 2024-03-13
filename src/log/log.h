#ifndef _LEVEL_DB_XY_LOG_H_
#define _LEVEL_DB_XY_LOG_H_

#include <cstdio>

#ifndef PRINT_DEBUG
#define PRINT_DEBUG(fmt, args...) fprintf(stdout, "[DEBUG] " fmt, ##args)
#endif

#ifndef PRINT_INFO
#define PRINT_INFO(fmt, args...) fprintf(stdout, "[INFO] " fmt, ##args)
#endif

#ifndef PRINT_WARNING
#define PRINT_WARNING(fmt, args...) fprintf(stdout, "[WARNING] " fmt, ##args)
#endif

#ifndef PRINT_ERROR
#define PRINT_ERROR(fmt, args...) fprintf(stderr, "[ERROR] " fmt, ##args)
#endif

#ifndef PRINT_FATAL
#define PRINT_FATAL(fmt, args...) fprintf(stderr, "[FATAL] " fmt, ##args)
#endif

#endif