#include <unistd.h>
#include <stdio.h>

#include "ipaddr.h"

int
main(int argc, char **argv) {
  // read from stdin, parse CIDR (for AFL)
  char buf[255];
  ssize_t len;

  len = read(0, buf, 254);
  if (len < 0) {
    return 1;
  }

  buf[len] = '\0';

  {
    net4_t net;
    read_net4(buf, &net);
    read_net4_strict(buf, &net);
  }
  {
    ip4_t ip;
    read_ip4(buf, &ip);
    //read_ip4_strict(buf, &ip);
  }
  {
    net6_t net;
    read_net6(buf, &net);
    read_net6_strict(buf, &net);
  }
  {
    ip6_t ip;
    read_ip6(buf, &ip);
    //read_ip6_strict(buf, &ip);
  }

  return 0;
}
