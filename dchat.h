/*** dchat.h ***/

/* includes */

#include <stdlib.h>
#include <unistd.h>
/* constants */

#ifndef MAX_BUFFER_LEN

// maximum buffer length
#define MAX_BUFFER_LEN 1024

// maximum nickname length
#define MAX_NICKNAME_LEN 20

// maximum ip address length (IPv4)
#define MAX_IP_ADDRESS_LEN 15

// maximum port num length
#define MAX_PORT_NUM_LEN 5

#ifdef DEBUG
# define IF_DEBUG(x) x
#else
# define IF_DEBUG(x) ((void)0)
#endif

#endif
