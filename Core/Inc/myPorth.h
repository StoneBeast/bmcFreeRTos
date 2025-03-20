#ifndef __MY_PORT_H
#define __MY_PORT_H

#include "cmsis_os.h"
#include <stdlib.h>

#define USE_STD         0

#if USE_STD==1

#define MALLOC(size)    malloc(size)
#define FREE(p)         free(p)
#else   //!USE_STD==1

#define MALLOC(size)    pvPortMalloc(size)
#define FREE(p)         vPortFree(p)
#endif  //!USE_STD==1


#endif // !__MY_PORT_H
