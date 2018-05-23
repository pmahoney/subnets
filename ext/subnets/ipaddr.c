#include <ctype.h>              /* isdigit, isxdigit */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "ipaddr.h"

ip4_t
mk_mask4(int prefixlen) {
  int shift;

  if (prefixlen==0) return 0;

  shift = 32 - prefixlen;
  return (~((ip4_t) 0) >> shift) << shift;
}

ip6_t
mk_mask6(int prefixlen) {
  ip6_t mask;
  uint16_t ones = ~((uint16_t) 0);

  int pivot = prefixlen / 16;
  int shift = 16 - (prefixlen % 16);

  for (int i=0; i<pivot && i<8; i++) {
    mask.x[i] = ones;
  }
  if (pivot<8) {
    mask.x[pivot] = (ones >> shift) << shift;
  }
  for (int i = pivot+1; i<8; i++) {
    mask.x[i] = 0;
  }

  return mask;
}

int
net4_include_p(net4_t net, ip4_t a) {
  return ((net.address & net.mask) == (a & net.mask));
}

int
net6_include_p(net6_t net, ip6_t a) {
  for (int i=0; i<8; i++) {
    if ((net.address.x[i] & net.mask.x[i]) != (a.x[i] & net.mask.x[i])) return 0;
  }
  return !0;
}

int
net4_include_net4_p(net4_t net, net4_t other) {
  return net.prefixlen <= other.prefixlen && net4_include_p(net, other.address);
}

int
net6_include_net6_p(net6_t net, net6_t other) {
  return net.prefixlen <= other.prefixlen && net6_include_p(net, other.address);
}

net4_t
net4_network(net4_t net) {
  net.address = net.address & net.mask;
  return net;
}

int
ip4_snprint(ip4_t ip, char *str, size_t size) {
  return snprintf(str, size, "%u.%u.%u.%u",
                  (ip >> 24) & 0xff,
                  (ip >> 16) & 0xff,
                  (ip >> 8) & 0xff,
                  (ip >> 0) & 0xff);
}

int
net4_snprint(net4_t net, char *str, size_t size) {
  return snprintf(str, size, "%u.%u.%u.%u/%d",
                  (net.address >> 24) & 0xff,
                  (net.address >> 16) & 0xff,
                  (net.address >> 8) & 0xff,
                  (net.address >> 0) & 0xff,
                  net.prefixlen);
}

int
ip6_snprint(ip6_t ip, char *str, size_t size) {
  /* find first longest string of zeroes */
  int longzerostart = -1;
  int longzeroend = -1;

  int currzerostart = -1;
  int currzeroend = -1;

  for (int i = 0; i < 8; i++) {
    if (0 == ip.x[i]) {
      currzeroend = i+1;
      if (currzerostart < 0) {
        currzerostart = i;
      }
      if ((currzeroend - currzerostart) > (longzeroend - longzerostart)) {
        longzerostart = currzerostart;
        longzeroend = currzeroend;
      }
    } else {
      if (currzerostart >= 0) {
        currzerostart = -1;
      }
    }
  }

  if (longzeroend - longzerostart <= 1) {
    return snprintf(str, size, "%x:%x:%x:%x:%x:%x:%x:%x",
                    ip.x[0],
                    ip.x[1],
                    ip.x[2],
                    ip.x[3],
                    ip.x[4],
                    ip.x[5],
                    ip.x[6],
                    ip.x[7]);
  } else {
    int total = 0;
    int n;

    /* print first before zeros */
    for (int i = 0; i < 1 && i < longzerostart; i++) {
      n = snprintf(str+total, size-total, "%x", ip.x[i]);
      if (n < 0) { return n; }
      total += n;
    }

    /* print rest before zeros with leading colon */
    for (int i = 1; i < longzerostart; i++) {
      n = snprintf(str+total, size-total, ":");
      if (n < 0) { return n; }
      total += n;
      n = snprintf(str+total, size-total, "%x", ip.x[i]);
      if (n < 0) { return n; }
      total += n;
    }

    /* print double colon */
    n = snprintf(str+total, size-total, "::");
    if (n < 0) { return n; }
    total += n;

    /* print first after zeros */
    for (int i = longzeroend; i < 8 && i < longzeroend+1; i++) {
      n = snprintf(str+total, size-total, "%x", ip.x[i]);
      if (n < 0) { return n; }
      total += n;
    }

    /* print rest after zeros with leading colon */
    for (int i = longzeroend + 1; i < 8; i++) {
      n = snprintf(str+total, size-total, ":");
      if (n < 0) { return n; }
      total += n;

      n = snprintf(str+total, size-total, "%x", ip.x[i]);
      if (n < 0) { return n; }
      total += n;
    }

    return total;
  }
}

int
net6_snprint(net6_t net, char *str, size_t size) {
  int n0, n1;
  n0 = ip6_snprint(net.address, str, size);
  if (n0 < 0) {
    return n0;
  }

  n1 = snprintf(str+n0, size-n0, "/%d", net.prefixlen);
  if (n1 < 0) {
    return n1;
  }

  return n0 + n1;
}

int
ip6_eql_p(ip6_t a, ip6_t b) {
  for (int i=0; i<8; i++) {
    if (a.x[i] != b.x[i]) return 0;
  }
  return !0;
}

ip6_t
ip6_not(ip6_t ip) {
  ip6_t not;
  for (int i=0; i<8; i++) {
    not.x[i] = ~(ip.x[i]);
  }
  return not;
}

ip6_t
ip6_band(ip6_t a, ip6_t b) {
  ip6_t band;
  for (int i=0; i<8; i++) {
    band.x[i] = a.x[i] & b.x[i];
  }
  return band;
}

ip6_t
ip6_bor(ip6_t a, ip6_t b) {
  ip6_t bor;
  for (int i=0; i<8; i++) {
    bor.x[i] = a.x[i] | b.x[i];
  }
  return bor;
}

ip6_t
ip6_xor(ip6_t a, ip6_t b) {
  ip6_t xor;
  for (int i=0; i<8; i++) {
    xor.x[i] = a.x[i] ^ b.x[i];
  }
  return xor;
}

int
hexvalue(unsigned int c) {
  if (c-'0'<10) return c-'0';
  if (c-'a'<6) return c-'a'+10;
  if (c-'A'<6) return c-'A'+10;
  return -1;
}

/*
 * read_ip4 is adapted from musl-libc inet_pton
 * https://git.musl-libc.org/cgit/musl/tree/src/network/inet_pton.c?id=fc13acc3dcb5b1f215c007f583a63551f6a71363
 *
 * Copyright © 2005-2014 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
/**
 * Read an Ip4 address from s.
 *
 * @return the number of characters consumed
 */
size_t
read_ip4(const char *s, ip4_t *a) {
  size_t pos = 0;

  *a = 0;

  for (int i = 0; i < 4; i++) {
    int v, j;
    for (v=j=0; j < 3 && isdigit(s[pos+j]); j++) {
      v = 10*v + s[pos+j]-'0';
    }
    if (j==0 || (j>1 && s[pos]=='0') || v>255) return 0;
    *a |= (v & 0xff) << (3-i)*8;
    pos += j;
    if (i == 3) return pos;
    if (s[pos++] != '.') return 0;
  }

  return 0;
}

size_t
read_hextet(const char *s, uint16_t *v) {
  int i;
  for (i=0; i<4 && isxdigit(s[i]); i++) {
    *v = 0x10*(*v) + hexvalue(s[i]);
  }
  if (i>1 && s[0]=='0') return 0;
  return i;
}

size_t
read_ip6(const char *s, ip6_t *a) {
  uint16_t hextets[8] = { 0, 0, 0, 0, 0, 0, 0, 0, };
  size_t n;
  int i = 0;
  size_t pos = 0;
  int brk = 8;
  ip4_t ip4;

  /* 
   * read optional leading hextet followed by zero or more
   * colon-hextet pairs, zero or one lone colon (the second of a
   * double-colon), zero or more colon-hextet pairs.
   */

  n = read_hextet(s+pos, &hextets[i]);
  if (n) {
    //printf("read hextet %d: %x\n", i, hextets[i]);
    pos+=n;
    i++;
  }

  for (; i < 8;) {
    if (s[pos] != ':') break;
    pos++;

    if (brk==8 && s[pos] == ':') {
      //printf("see break at pos=%d, brk=%d\n", pos, i);
      pos++;
      brk = i;
      if (!isxdigit(s[pos])) break;
    }
    else if (i==0) {            /* can't lead with single-colon */
      return 0;
    }

    if ((i==6 || (brk<8 && i<4)) && (n = read_ip4(s+pos, &ip4))) {
      pos += n;
      hextets[i] = (ip4 >> 16) & 0xffff;
      hextets[i+1] = (ip4 >> 0) & 0xffff;
      i+=2;
      break;
    }

    n = read_hextet(s+pos, &hextets[i]);
    if (n) {
      //printf("read hextet %d: %x\n", i, hextets[i]);
      pos+=n;
      i++;
    }
    else {
      //printf("error 'cause read %c\n", s[pos]);
      return 0;
    }
  }

  if (brk==8 && i<8) return 0;
  if (brk<8 && i==8) return 0;

  for (int k = 0; k < 8; k++) {
    //printf("brk=%d i=%d hextets[%d]=%x\n", brk, i, k, hextets[k]);
  }

  /* TODO move down hextets after the break */
  //printf("before\n");
  for (int j = 0; j < brk; j++) {
    a->x[j] = hextets[j];
  }
  //printf("brk\n");
  for (int j = 0; j < (8-i); j++) {
    a->x[j+brk] = 0;
  }
  //printf("after\n");
  for (int j = 0; j < (i-brk); j++) {
    //printf(" (from hextets[%d])\n", j+brk);
    a->x[j+(8-i)+brk] = hextets[j+brk];
  }

  return pos;
}

size_t
read_net4(const char *s, net4_t *net) {
  int i, v=0;
  size_t pos = read_ip4(s, &net->address);
  if (!pos) return 0;

  if (!(s[pos++] == '/')) return 0;
  for (i = 0; i < 2 && isdigit(s[pos+i]); i++) {
    v = v*10 + s[pos+i]-'0';
  }
  if (i==0 || (i>1 && s[pos]=='0') || v>32) return 0;
  net->prefixlen = v;
  net->mask = mk_mask4(net->prefixlen);
  return pos+i;
}

size_t
read_ip4_strict(const char *s, ip4_t *ip) {
  size_t n = read_ip4(s, ip);
  if (!n || s[n] != 0) return 0;
  return n;
}

size_t
read_net4_strict(const char *s, net4_t *net) {
  size_t n = read_net4(s, net);
  if (!n || s[n] != 0) return 0;
  return n;
}

size_t
read_ip6_strict(const char *s, ip6_t *ip) {
  size_t n = read_ip6(s, ip);
  if (!n || s[n] != 0) return 0;
  return n;
}

size_t
read_net6_strict(const char *s, net6_t *net) {
  size_t n = read_net6(s, net);
  if (!n || s[n] != 0) return 0;
  return n;
}

size_t
read_net6(const char *s, net6_t *net) {
  size_t pos;
  int i, v = 0;

  pos = read_ip6(s, &net->address);
  if (!pos) return 0;

  if (s[pos++] != '/') return 0;
  for (i = 0; i < 3 && isdigit(s[pos+i]); i++) {
    v = v*10 + s[pos+i]-'0';
  }
  if (i==0 || (i>1 && s[pos]=='0') || v>128) return 0;
  net->prefixlen = v;
  net->mask = mk_mask6(net->prefixlen);
  return pos+i;
}
