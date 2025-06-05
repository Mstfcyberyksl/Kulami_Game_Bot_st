#ifndef KULAMI_CONFIG_H
#define KULAMI_CONFIG_H

// Performance optimization settings
#define ENABLE_MEMOIZATION 0
#define ENABLE_PRUNING 1
#define ENABLE_MOVE_ORDERING 1
#define MAX_SEARCH_DEPTH 12
#define HASH_TABLE_SIZE 1000003  // Prime number for better distribution

// Network optimization settings
#define ENABLE_CONNECTION_POOLING 1
#define SOCKET_TIMEOUT_MS 5000

// Game constants
#define ROWS 8
#define COLUMNS 8
#define directionsize 28
#define framecount 17
#define calcfuncsize 6
#define data_length 8
#define PORT 9000

// Inline function macros for frequently called functions
#ifdef __GNUC__
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

#endif // KULAMI_CONFIG_H
