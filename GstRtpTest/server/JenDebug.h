/*
 * JenDebug.h
 *
 *  Created on: May 22, 2015
 *      Author: robin
 */

#ifndef JENDEBUG_H_
#define JENDEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int iAvcStreamLogLevel;
//============================================================================================================// Debug logger
#define     ERROR_LEVEL 0
#define     WARN_LEVEL  1
#define     INFO_LEVEL  2
#define     DEBUG_LEVEL 3
#define     LOG_LEVEL   4

#define ERROR(format, ...)                                  \
    do{                                                     \
        if (iAvcStreamLogLevel >= ERROR_LEVEL)              \
        {                                                   \
            fprintf(stdout, "\e[31m[JEN ERROR]: \e[0m");    \
            fprintf(stdout, format, ##__VA_ARGS__);         \
            fflush(stdout);                                 \
        }                                                   \
    }while(0)

#define WARN(format, ...)                                   \
    do{                                                     \
        if (iAvcStreamLogLevel >= WARN_LEVEL)               \
        {                                                   \
            fprintf(stdout, "\e[35m[JEN WARN]: \e[0m");     \
            fprintf(stdout, format, ##__VA_ARGS__);         \
            fflush(stdout);                                 \
        }                                                   \
    }while(0)

#define INFO(format, ...)                                   \
    do{                                                     \
        if (iAvcStreamLogLevel >= INFO_LEVEL)               \
        {                                                   \
            fprintf(stdout, "\e[33m[JEN INFO]: \e[0m");     \
            fprintf(stdout, format, ##__VA_ARGS__);         \
            fflush(stdout);                                 \
        }                                                   \
    }while(0)

#define DEBUG(format, ...)                                  \
    do{                                                     \
        if (iAvcStreamLogLevel >= DEBUG_LEVEL)              \
        {                                                   \
            fprintf(stdout, "\e[32m[JEN DEBUG]: \e[0m");    \
            fprintf(stdout, format, ##__VA_ARGS__);         \
            fflush(stdout);                                 \
        }                                                   \
    }while(0)

#define LOG(format, ...)                                    \
    do{                                                     \
        if (iAvcStreamLogLevel >= LOG_LEVEL)                \
        {                                                   \
            fprintf(stdout, "\e[36m");                      \
            fprintf(stdout, format, ##__VA_ARGS__);         \
            fprintf(stdout, "\e[0m");                       \
            fflush(stdout);                                 \
        }                                                   \
    }while(0)

#define PRINT(format, ...)                                  \
    do{                                                     \
        fprintf(stdout, "\e[36m");                          \
        fprintf(stdout, format, ##__VA_ARGS__);             \
        fprintf(stdout, "\e[0m");                           \
        fflush(stdout);                                     \
    }while(0)

#define SetDebugLogLevel(env, level)                        \
    do{                                                     \
        if((NULL != env) && (NULL != getenv(env)))          \
        {                                                   \
            iAvcStreamLogLevel = atoi(getenv(env));         \
        }													\
		else												\
		{													\
			iAvcStreamLogLevel = level;						\
		}													\
        printf("LogLevel: %d!\n", iAvcStreamLogLevel);    	\
    }while(0)

#endif /* JENDEBUG_H_ */

#ifdef __cplusplus
}
#endif
