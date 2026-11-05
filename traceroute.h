#ifndef HEADER_H
#define HEADER_H 

#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <unistd.h>
#include <cstdlib>

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/ip_icmp.h>

#define BUF_SIZE 64
#define START_TTL 2
#define MAX_TTL 31
inline int LOG_LEVEL = 0;
#define TRACE   if (LOG_LEVEL > 5) { std::cout << "TRACE: "
#define DEBUG   if (LOG_LEVEL > 4) { std::cout << "DEBUG: "
#define INFO    if (LOG_LEVEL > 3) { std::cout << "INFO: "
#define WARNING if (LOG_LEVEL > 2) { std::cout << "WARNING: "
#define ERROR   if (LOG_LEVEL > 1) { std::cout << "ERROR: "
#define FATAL   if (LOG_LEVEL > 0) { std::cout << "FATAL: "
#define ENDL  " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; }

uint16_t checksum(unsigned short *buffer, int size);
void fill_in_IP_Header(char *packet, int ttl);
void fill_in_ICMP_Header(char *packet);
#endif
