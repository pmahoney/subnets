#ifndef __IPADDR_H__
#define __IPADDR_H__

#include <ctype.h>              /* isdigit, isxdigit */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

typedef uint32_t ip4_t;

typedef struct {
  int prefixlen;
  ip4_t address;
  ip4_t mask;
} net4_t;

typedef struct {
  uint16_t x[8];
} ip6_t;

typedef struct {
  int prefixlen;
  ip6_t address;
  ip6_t mask;
} net6_t;

/**
 * Make an IPv4 mask of the given prefixlen in the range [0,32].
 */
ip4_t mk_mask4(int prefixlen);

/**
 * Make an IPv6 mask of the given prefixlen in the range [0,128].
 */
ip6_t mk_mask6(int prefixlen);

/**
 * Test if this network includes the given ip.
 */
int net4_include_p(net4_t, ip4_t);
int net6_include_p(net6_t, ip6_t);

/**
 * Test if this network includes the given network, which must have a
 * prefexlen greater than or equal to that of this network and a
 * network address that is included within this network.
 */
int net4_include_net4_p(net4_t, net4_t);
int net6_include_net6_p(net6_t, net6_t);

/**
 * Zero-out the host portion of this network by applying the netmask,
 * and return that network.
 */
net4_t net4_network(net4_t);
net6_t net6_network(net6_t);

/**
 * Write a string representation of this network to the given string,
 * according to the rules of snprintf().
 */
int net4_snprint(net4_t, char *, size_t);
int net6_snprint(net6_t, char *, size_t);

/**
 * Write a string representation of this ip to the given string,
 * according to the rules of snprintf().
 */
int ip4_snprint(ip4_t, char *, size_t);
int ip6_snprint(ip6_t, char *, size_t);

/**
 * Read an IP from the string, returning the number of bytes read, or
 * zero on parse error.
 */
size_t read_ip4(const char *, ip4_t *);
size_t read_ip6(const char *, ip6_t *);

/**
 * Read a network from the string, returning the number of bytes read,
 * or zero on parse error.
 */
size_t read_net4(const char *, net4_t *);
size_t read_net6(const char *, net6_t *);

/**
 * Like read_ip*, but it is an error if the IP is not followed by a
 * null byte.
 */
size_t read_ip4_strict(const char *, ip4_t *);
size_t read_ip6_strict(const char *, ip6_t *);

/**
 * Like read_net*, but it is an error if the network is not followed
 * by a null byte.
 */
size_t read_net4_strict(const char *, net4_t *);
size_t read_net6_strict(const char *, net6_t *);

int ip6_eql_p(ip6_t, ip6_t);
ip6_t ip6_not(ip6_t);
ip6_t ip6_band(ip6_t, ip6_t);
ip6_t ip6_bor(ip6_t, ip6_t);
ip6_t ip6_xor(ip6_t, ip6_t);

#endif                          /* __IPADDR_H__ */
