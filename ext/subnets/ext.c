#include "ruby.h"

#include <stdio.h>

#include "ipaddr.h"

VALUE Subnets = Qnil;
VALUE IP = Qnil;
VALUE IP4 = Qnil;
VALUE IP6 = Qnil;
VALUE Net = Qnil;
VALUE Net4 = Qnil;
VALUE Net6 = Qnil;

VALUE rb_intern_hash = Qnil;
VALUE rb_intern_xor = Qnil;

#define hash(o) rb_funcall(o, rb_intern_hash, 0)
#define xor(a,b) rb_funcall(a, rb_intern_xor, 1, b)

#define assert_kind_of(obj, kind) do {                                  \
    if (!rb_obj_is_kind_of(obj, kind)) {                                \
      rb_raise(rb_eTypeError, "wrong argument type %s (expected " #kind ")", rb_obj_classname(obj)); \
    }                                                                   \
  } while (0)

/**
 * ParseError indicates the input string could not be parsed as the
 * requested type.
 */
VALUE ParseError = Qnil;

VALUE
raise_parse_error(const char *type, const char *data) {
  /* 49 is longest possible ip6 cidr */
  /* ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255/128 */
  if (strlen(data) > 49) {
    rb_raise(ParseError, "failed to parse as %s: '%.45s...'", type, data);
  } else {
    rb_raise(ParseError, "failed to parse as %s: '%s'", type, data);
  }
}

VALUE
ip4_new(VALUE class, ip4_t src) {
  ip4_t *ip;
  VALUE v = Data_Make_Struct(class, ip4_t, 0, free, ip);
  *ip = src;
  rb_obj_call_init(v, 0, 0);
  return v;
}

VALUE
ip6_new(VALUE class, ip6_t src) {
  ip6_t *ip;
  VALUE v = Data_Make_Struct(class, ip6_t, 0, free, ip);
  *ip = src;
  rb_obj_call_init(v, 0, 0);
  return v;
}

VALUE
net4_new(VALUE class, net4_t src) {
  net4_t *net;
  VALUE rbnet = Data_Make_Struct(class, net4_t, 0, free, net);
  *net = src;
  rb_obj_call_init(rbnet, 0, 0);
  return rbnet;
}

VALUE
net6_new(VALUE class, net6_t src) {
  net6_t *net;
  VALUE rbnet = Data_Make_Struct(class, net6_t, 0, free, net);
  *net = src;
  rb_obj_call_init(rbnet, 0, 0);
  return rbnet;
}

VALUE
method_ip4_new(VALUE class, VALUE address) {
  return ip4_new(class, RB_NUM2UINT(address));
}

VALUE
method_net4_new(VALUE class, VALUE address, VALUE prefixlen) {
  net4_t net;
  net.address = RB_NUM2UINT(address);
  net.prefixlen = NUM2INT(prefixlen);
  if (!(net.prefixlen >= 0 && net.prefixlen <= 32)) {
    rb_raise(rb_eArgError, "prefixlen must be in range [0,32], was %d", net.prefixlen);
  }
  net.mask = mk_mask4(net.prefixlen);
  return net4_new(class, net);
}

VALUE
method_net6_new(VALUE class, VALUE hextets, VALUE prefixlen) {
  net6_t net;

  if (RARRAY_LEN(hextets) != 8) {
    rb_raise(rb_eArgError, "hextets must be size=8, was %ld", RARRAY_LEN(hextets));
  }

  for (ssize_t i = 0; i < RARRAY_LEN(hextets); i++) {
    net.address.x[i] = NUM2INT(RARRAY_AREF(hextets, i)) & 0xffff;
  }

  net.prefixlen = NUM2INT(prefixlen);
  if (!(net.prefixlen >= 0 && net.prefixlen <= 128)) {
    rb_raise(rb_eArgError, "prefixlen must be in range [0,128], was %d", net.prefixlen);
  }

  net.mask = mk_mask6(net.prefixlen);

  return net6_new(class, net);
}

/**
 * Parse +s+ as an IPv4 network in CIDR notation.
 *
 * @param [String] s
 * @return {Net4}
 * @raise {Subnets::ParseError}
 */
VALUE
method_net4_parse(VALUE class, VALUE s) {
  const char *buf = StringValueCStr(s);

  net4_t net;
  if (!read_net4_strict(buf, &net)) {
    raise_parse_error("net4", buf);
  }
  return net4_new(class, net);
}

/**
 * Parse +s+ as an IPv6 network in CIDR notation. Handles the
 * following formats:
 *
 * - x:x:x:x:x:x:x:x consisting of eight hexadecimal numbers of one to
 *   four digits (16 bits) separated by colons
 *
 * - x:x:x::x:x:x as above, but a single double-colon replaces two or
 *   more repeated zeros
 *
 * - x:x:x:x:x:x:d.d.d.d consisting of a colon-separated hexadecimal
 *   portion as above defining up to six hextets, followed by
 *   dot-separated decimal numbers 0-255 in typical IPv4 format.
 *
 * @param [String] s
 * @return {Net6}
 * @raise {Subnets::ParseError}
 */
VALUE
method_net6_parse(VALUE class, VALUE s) {
  const char *buf = StringValueCStr(s);

  net6_t net;
  if (!read_net6_strict(buf, &net)) {
    raise_parse_error("net6", buf);
  }
  return net6_new(class, net);
}

/**
 * @overload random(rng=Random.new)
 *   @param rng [#rand] (optional) a random number generator
 * @return [IP4] a random IP4 address
 */
VALUE
method_ip4_random(int argc, VALUE *argv, VALUE class) {
  ip4_t ip;
  VALUE rng;

  rb_scan_args(argc, argv, "01", &rng);
  if (Qnil == rng) {
    rng = rb_funcall(rb_cRandom, rb_intern("new"), 0);
  }

  VALUE rand = rb_intern("rand");
  ip = FIX2INT(rb_funcall(rng, rand, 1, INT2FIX(0xffff+1)));
  ip |= FIX2INT(rb_funcall(rng, rand, 1, INT2FIX(0xffff+1))) << 16;

  return ip4_new(class, ip);
}

/**
 * @overload random(rng=Random.new)
 *   @param rng [#rand] (optional) a random number generator
 * @return [Net4] a random Net4 address
 */
VALUE
method_net4_random(int argc, VALUE *argv, VALUE class) {
  net4_t net;
  VALUE rng;

  rb_scan_args(argc, argv, "01", &rng);
  if (Qnil == rng) {
    rng = rb_funcall(rb_cRandom, rb_intern("new"), 0);
  }

  VALUE rand = rb_intern("rand");
  net.address = FIX2INT(rb_funcall(rng, rand, 1, INT2FIX(0xffff+1)));
  net.address |= FIX2INT(rb_funcall(rng, rand, 1, INT2FIX(0xffff+1))) << 16;
  net.prefixlen = FIX2INT(rb_funcall(rng, rb_intern("rand"), 1, INT2FIX(32+1)));
  net.mask = mk_mask4(net.prefixlen);

  return net4_new(class, net);
}

void
ip6_fill_random(ip6_t *ip, VALUE rng, VALUE opts) {
  if (Qnil == rng) {
    rng = rb_funcall(rb_cRandom, rb_intern("new"), 0);
  }

  VALUE rand = rb_intern("rand");
  int pre, zeros;
  
  if (Qnil != opts && RTEST(rb_hash_aref(opts, ID2SYM(rb_intern("zeros"))))) {
    pre = FIX2INT(rb_funcall(rng, rand, 1, INT2FIX(8+1)));
    zeros = FIX2INT(rb_funcall(rng, rand, 1, INT2FIX(8+1 - pre)));
  } else {
    pre = 8;
    zeros = 0;
  }

  for (int i=0; i<pre; i++) {
    ip->x[i] = FIX2INT(rb_funcall(rng, rand, 1, INT2FIX(0xffff+1)));
  }
  for (int i=pre; i<pre+zeros; i++) {
    ip->x[i] = 0;
  }
  for (int i=pre+zeros; i<8; i++) {
    ip->x[i] = FIX2INT(rb_funcall(rng, rand, 1, INT2FIX(0xffff+1)));
  }
}

/**
 * @overload random(rand=Random.new, opts={})
 *   @param rand [#rand] (optional) a random number generator
 *   @param opts [Hash]
 *   @option opts [Boolean] :zeros include a random string of zeros at a random position rather than simply randomizing the entire address
 * @return [IP6] a random IP6 address
 */
VALUE
method_ip6_random(int argc, VALUE *argv, VALUE class) {
  ip6_t ip;
  VALUE rng;
  VALUE opts;

  rb_scan_args(argc, argv, "01:", &rng, &opts);
  ip6_fill_random(&ip, rng, opts);
  return ip6_new(class, ip);
}

/**
 * @overload random(rand=Random.new, opts={})
 *   @param rand [#rand] (optional) a random number generator
 *   @param opts [Hash]
 *   @option opts [Boolean] :zeros include a random string of zeros at a random position rather than simply randomizing the entire address
 * @return [Net6] a random Net6
 */
VALUE
method_net6_random(int argc, VALUE *argv, VALUE class) {
  net6_t net;
  VALUE rng;
  VALUE opts;

  rb_scan_args(argc, argv, "01:", &rng, &opts);
  ip6_fill_random(&net.address, rng, opts);
  net.prefixlen = FIX2INT(rb_funcall(rng, rb_intern("rand"), 1, INT2FIX(128+1)));
  net.mask = mk_mask6(net.prefixlen);

  return net6_new(class, net);
}

VALUE
method_ip4_not(VALUE self) {
  ip4_t *ip;
  Data_Get_Struct(self, ip4_t, ip);
  return ip4_new(IP4, ~ *ip);
}

VALUE
method_ip4_bor(VALUE self, VALUE other) {
  ip4_t *a, *b;

  assert_kind_of(other, IP4);

  Data_Get_Struct(self, ip4_t, a);
  Data_Get_Struct(other, ip4_t, b);
  return ip4_new(IP4, *a | *b);
}

VALUE
method_ip4_xor(VALUE self, VALUE other) {
  ip4_t *a, *b;

  assert_kind_of(other, IP4);

  Data_Get_Struct(self, ip4_t, a);
  Data_Get_Struct(other, ip4_t, b);
  return ip4_new(IP4, *a ^ *b);
}

VALUE
method_ip4_band(VALUE self, VALUE other) {
  ip4_t *a, *b;

  assert_kind_of(other, IP4);

  Data_Get_Struct(self, ip4_t, a);
  Data_Get_Struct(other, ip4_t, b);
  return ip4_new(IP4, *a & *b);
}

VALUE
method_ip6_not(VALUE self) {
  ip6_t *ip;
  Data_Get_Struct(self, ip6_t, ip);
  return ip6_new(IP6, ip6_not(*ip));
}

VALUE
method_ip6_bor(VALUE self, VALUE other) {
  ip6_t *a, *b;

  assert_kind_of(other, IP6);

  Data_Get_Struct(self, ip6_t, a);
  Data_Get_Struct(other, ip6_t, b);

  return ip6_new(IP6, ip6_bor(*a, *b));
}

VALUE
method_ip6_xor(VALUE self, VALUE other) {
  ip6_t *a, *b;

  assert_kind_of(other, IP6);

  Data_Get_Struct(self, ip6_t, a);
  Data_Get_Struct(other, ip6_t, b);

  return ip6_new(IP6, ip6_xor(*a, *b));
}

VALUE
method_ip6_band(VALUE self, VALUE other) {
  ip6_t *a, *b;

  assert_kind_of(other, IP6);

  Data_Get_Struct(self, ip6_t, a);
  Data_Get_Struct(other, ip6_t, b);

  return ip6_new(IP6, ip6_band(*a, *b));
}

/**
 * The prefix length of this network, or number of leading ones in the
 * netmask.
 *
 * @return [Fixnum]
 */
VALUE
method_net4_prefixlen(VALUE self) {
  net4_t *net;
  Data_Get_Struct(self, net4_t, net);
  return INT2FIX(net->prefixlen);
}

/**
 * (see Subnets::Net4#prefixlen)
 */
VALUE
method_net6_prefixlen(VALUE self) {
  net6_t *net;
  Data_Get_Struct(self, net6_t, net);
  return INT2FIX(net->prefixlen);
}

/**
 * Test if this network includes +v+.
 *
 * A String must parse as an IP4 or Net4.  An IP4 must be included
 * within the range defined by this network.  A Net4 must both have a
 * prefixlen greater than or equal to that of this network, and have
 * an address included within the range defined by this network.
 *
 * @param [String, IP, Net] v
 */
VALUE
method_net4_include_p(VALUE self, VALUE v) {
  net4_t *net;
  Data_Get_Struct(self, net4_t, net);

  if (CLASS_OF(v) == IP4) {
    ip4_t *ip;
    Data_Get_Struct(v, ip4_t, ip);
    return net4_include_p(*net, *ip) ? Qtrue : Qfalse;
  } else if (CLASS_OF(v) == Net4) {
    net4_t *other;
    Data_Get_Struct(v, net4_t, other);
    return net4_include_net4_p(*net, *other) ? Qtrue : Qfalse;
  } else if (CLASS_OF(v) == IP6 || CLASS_OF(v) == Net6) {
    return Qfalse;
  } else {
    v = StringValue(v);

    {
      net4_t other;
      if (read_net4_strict(RSTRING_PTR(v), &other)) {
        return net4_include_net4_p(*net, other) ? Qtrue : Qfalse;
      }
    }
    {
      ip4_t ip;
      if (read_ip4_strict(RSTRING_PTR(v), &ip)) {
        return net4_include_p(*net, ip) ? Qtrue : Qfalse;
      }
    }

    return Qfalse;
  }
}

/**
 * Test if this network includes +v+.
 *
 * A String must parse as an IP6 or Net6.  An IP6 must be included
 * within the range defined by this network.  A Net6 must both have a
 * prefixlen greater than or equal to that of this network, and have
 * an address included within the range defined by this network.
 *
 * @param [String, IP, Net] v
 */
VALUE
method_net6_include_p(VALUE self, VALUE v) {
  net6_t *net;
  Data_Get_Struct(self, net6_t, net);

  if (CLASS_OF(v) == IP6) {
    ip6_t *ip;
    Data_Get_Struct(v, ip6_t, ip);
    return net6_include_p(*net, *ip) ? Qtrue : Qfalse;
  } else if (CLASS_OF(v) == Net6) {
    net6_t *other;
    Data_Get_Struct(v, net6_t, other);
    return net6_include_net6_p(*net, *other) ? Qtrue : Qfalse;
  } else if (CLASS_OF(v) == IP4 || CLASS_OF(v) == Net4) {
    return Qfalse;
  } else if (rb_obj_is_kind_of(v, rb_cString)) {
    const char *buf = StringValueCStr(v);

    {
      net6_t other;
      if (read_net6_strict(buf, &other)) {
        return net6_include_net6_p(*net, other) ? Qtrue : Qfalse;
      }
    }
    {
      ip6_t ip;
      if (read_ip6_strict(buf, &ip)) {
        return net6_include_p(*net, ip) ? Qtrue : Qfalse;
      }
    }
  }

  return Qfalse;
}

/**
 * Return a String in dotted-decimal notation.
 *
 * @return [String]
 */
VALUE
method_ip4_to_s(VALUE self) {
  ip4_t *ip;
  Data_Get_Struct(self, ip4_t, ip);

  char buf[16];

  ip4_snprint(*ip, buf, 16);
  return rb_str_new2(buf);
}

/**
 * Return a String of colon-separated hexadecimal parts with multiple
 * zeros compresses with a double-colon.
 *
 * @return [String]
 */
VALUE
method_ip6_to_s(VALUE self) {
  ip6_t *ip;
  Data_Get_Struct(self, ip6_t, ip);

  char buf[64];

  ip6_snprint(*ip, buf, 64);
  return rb_str_new2(buf);
}

/**
 * Return a String in CIDR notation.
 *
 * @return [String]
 */
VALUE
method_net4_to_s(VALUE self) {
  net4_t *net;
  Data_Get_Struct(self, net4_t, net);

  char buf[32];

  net4_snprint(*net, buf, 32);
  return rb_str_new2(buf);
}

/**
 * Return a String in CIDR notation.
 *
 * @return [String]
 */
VALUE
method_net6_to_s(VALUE self) {
  net6_t *net;
  Data_Get_Struct(self, net6_t, net);

  char buf[64];

  net6_snprint(*net, buf, 64);
  return rb_str_new2(buf);
}

VALUE
method_ip4_to_i(VALUE self) {
  ip4_t *ip;
  Data_Get_Struct(self, ip4_t, ip);
  return RB_UINT2NUM(*ip);
}

VALUE
method_ip6_to_i(VALUE self) {
  VALUE ret;
  ip6_t *ip;
  Data_Get_Struct(self, ip6_t, ip);

  ID lshift = rb_intern("<<");
  ID plus = rb_intern("+");

  ret = RB_INT2NUM(0);

  for (int i=0; i<8; i++) {
    VALUE hextet = RB_UINT2NUM(ip->x[i]);
    VALUE inc = rb_funcall(hextet, lshift, 1, RB_INT2NUM(16*(7-i)));
    ret = rb_funcall(ret, plus, 1, inc);
  }

  return ret;
}

/**
 * @return [Boolean]
 */
VALUE
method_ip4_eql_p(VALUE self, VALUE other) {
  if (CLASS_OF(other) != CLASS_OF(self)) {
    return Qfalse;
  }

  ip4_t *a, *b;
  Data_Get_Struct(self, ip4_t, a);
  Data_Get_Struct(other, ip4_t, b);

  if (a != b) {
    return Qfalse;
  }
  return Qtrue;
}

/**
 * @return [Boolean]
 */
VALUE
method_net4_eql_p(VALUE self, VALUE other) {
  if (CLASS_OF(other) != CLASS_OF(self)) {
    return Qfalse;
  }

  net4_t *a, *b;
  Data_Get_Struct(self, net4_t, a);
  Data_Get_Struct(other, net4_t, b);

  if (a->prefixlen != b->prefixlen) {
    return Qfalse;
  }
  if (a->address != b->address) {
    return Qfalse;
  }
  return Qtrue;
}

/**
 * @return [Boolean]
 */
VALUE
method_ip6_eql_p(VALUE self, VALUE other) {
  if (CLASS_OF(other) != CLASS_OF(self)) {
    return Qfalse;
  }

  ip6_t *a, *b;
  Data_Get_Struct(self, ip6_t, a);
  Data_Get_Struct(other, ip6_t, b);

  return ip6_eql_p(*a, *b) ? Qtrue : Qfalse;
}

/**
 * @return [Boolean]
 */
VALUE
method_net6_eql_p(VALUE self, VALUE other) {
  if (CLASS_OF(other) != CLASS_OF(self)) {
    return Qfalse;
  }

  net6_t *a, *b;
  Data_Get_Struct(self, net6_t, a);
  Data_Get_Struct(other, net6_t, b);

  if (a->prefixlen != b->prefixlen) {
    return Qfalse;
  }

  for (int i=0; i<8; i++) {
    if (a->address.x[i] != b->address.x[i]) return Qfalse;
  }
  return Qtrue;
}

/**
 * @return [Integer]
 */
VALUE
method_ip4_hash(VALUE self) {
  ip4_t *ip;
  Data_Get_Struct(self, ip4_t, ip);
  return hash(UINT2NUM(ip));
}

/**
 * @return [Integer]
 */
VALUE
method_net4_hash(VALUE self) {
  net4_t *net;
  Data_Get_Struct(self, net4_t, net);
  return xor(hash(INT2FIX(net->prefixlen)), hash(UINT2NUM(net->address)));
}

/**
 * @return [Integer]
 */
VALUE
method_ip6_hash(VALUE self) {
  ip6_t *ip;
  Data_Get_Struct(self, ip6_t, ip);

  VALUE ret = hash(INT2FIX(ip->x[0]));
  for (int i=1; i<8; i++) {
    ret = xor(ret, hash(INT2FIX(ip->x[i])));
  }
  return ret;
}

/**
 * @return [Integer]
 */
VALUE
method_net6_hash(VALUE self) {
  net6_t *net;
  Data_Get_Struct(self, net6_t, net);

  VALUE ret = hash(INT2FIX(net->prefixlen));
  for (int i=0; i<8; i++) {
    ret = xor(ret, hash(INT2FIX(net->address.x[i])));
  }
  return ret;
}

VALUE
method_net4_network(VALUE self) {
  net4_t *addr;
  Data_Get_Struct(self, net4_t, addr);

  return net4_new(Net4, net4_network(*addr));
}

VALUE
method_net4_address(VALUE self) {
  net4_t *net;
  Data_Get_Struct(self, net4_t, net);
  return ip4_new(IP4, net->address);
}

VALUE
method_net6_address(VALUE self) {
  net6_t *net;
  Data_Get_Struct(self, net6_t, net);
  return ip6_new(IP6, net->address);
}

VALUE
method_net4_mask(VALUE self) {
  net4_t *net;
  Data_Get_Struct(self, net4_t, net);
  return ip4_new(IP4, net->mask);
}

VALUE
method_net6_mask(VALUE self) {
  net6_t *net;
  Data_Get_Struct(self, net6_t, net);
  return ip6_new(IP6, net->mask);
}

/**
 * @return [Array<Fixnum>] 16-bit hextets, most significant first
 */
VALUE
method_ip6_hextets(VALUE self) {
  ip6_t *ip;
  Data_Get_Struct(self, ip6_t, ip);

  VALUE hextets = rb_ary_new();
  for (int i=0; i<8; i++) {
    rb_ary_push(hextets, INT2FIX(ip->x[i]));
  }
  return hextets;
}

/**
 * (see Subnets::IP6#hextets)
 */
VALUE
method_net6_hextets(VALUE self) {
  net6_t *net;
  Data_Get_Struct(self, net6_t, net);

  VALUE hextets = rb_ary_new();
  for (int i=0; i<8; i++) {
    rb_ary_push(hextets, INT2FIX(net->address.x[i]));
  }
  return hextets;
}

/**
 * @return [Subnets::Net4] the smallest subnet that includes all of
 * the subnets in +nets+
 *
 * @param nets [Array<Subnets::Net4>]
 */
VALUE
method_net4_summarize(VALUE class, VALUE nets) {
  net4_t result;

  for (ssize_t i = 0; i < RARRAY_LEN(nets); i++) {
    VALUE rbnet = RARRAY_AREF(nets, i);

    assert_kind_of(rbnet, Net4);

    const net4_t *net;
    Data_Get_Struct(rbnet, net4_t, net);

    if (i == 0) {
      result.address = (net->address & net->mask);
      result.prefixlen = net->prefixlen;
      result.mask = net->mask;
    } else {
      if (result.prefixlen > net->prefixlen) {
        result.prefixlen = net->prefixlen;
        result.mask = net->mask;
        result.address &= result.mask;
      }

      while (result.address != (net->address & result.mask)) {
        result.prefixlen -= 1;
        result.mask = mk_mask4(result.prefixlen);
        result.address &= result.mask;
      }
    }
  }

  return net4_new(class, result);
}

/**
 * @return [Subnets::Net6] the smallest subnet that includes all of
 * the subnets in +nets+
 *
 * @param nets [Array<Subnets::Net6>]
 */
VALUE
method_net6_summarize(VALUE class, VALUE nets) {
  net6_t result;

  for (ssize_t i = 0; i < RARRAY_LEN(nets); i++) {
    VALUE rbnet = RARRAY_AREF(nets, i);

    assert_kind_of(rbnet, Net6);

    net6_t *net;
    Data_Get_Struct(rbnet, net6_t, net);

    if (i == 0) {
      result.address = ip6_band(net->address, net->mask);
      result.prefixlen = net->prefixlen;
      result.mask = net->mask;
    } else {
      if (result.prefixlen > net->prefixlen) {
        result.prefixlen = net->prefixlen;
        result.mask = net->mask;
        result.address = ip6_band(result.address, result.mask);
      }

      while(!ip6_eql_p(result.address, ip6_band(net->address, result.mask))) {
        result.prefixlen -= 1;
        result.mask = mk_mask6(result.prefixlen);
        result.address = ip6_band(result.address, result.mask);
      }
    }
  }

  return net6_new(class, result);
}

/**
 * Try parsing +str+ as Net4, Net6, IP4, IP6.
 *
 * @return [Net4, Net6, IP4, IP6]
 * @raise {ParseError}
 */
VALUE
method_subnets_parse(VALUE mod, VALUE str) {
  str = StringValue(str);
  const char *s = StringValueCStr(str);
  {
    net4_t net;
    if (read_net4_strict(s, &net)) return net4_new(Net4, net);
  }
  {
    net6_t net;
    if (read_net6_strict(s, &net)) return net6_new(Net6, net);
  }
  {
    ip4_t ip;
    if (read_ip4_strict(s, &ip)) return ip4_new(IP4, ip);
  }
  {
    ip6_t ip;
    if (read_ip6_strict(s, &ip)) return ip6_new(IP6, ip);
  }

  raise_parse_error("{v4,v6}{net,ip}", s);
  return Qnil;
}

/**
 * Test if any element in +nets+ includes +v+. For array elements
 * +obj+ that are not Net4 or Net6, calls +obj#===(v)+ to test for
 * inclusion.
 *
 * @see Subnets::IP4#include?
 * @see Subnets::IP6#include?
 * @param [Array<Net,Object>] nets
 * @param [String, IP, Net] v
 */
VALUE
method_subnets_include_p(VALUE self, VALUE nets, VALUE v) {
  int is_net4 = 0, is_net6 = 0, is_ip4 = 0, is_ip6 = 0;
  net4_t net4;
  net6_t net6;
  ip4_t ip4;
  ip6_t ip6;

  if (CLASS_OF(v) == IP4) {
    ip4_t *_ip4;
    is_ip4 = !0;
    Data_Get_Struct(v, ip4_t, _ip4);
    ip4 = *_ip4;
  } else if (CLASS_OF(v) == IP6) {
    ip6_t *_ip6;
    is_ip6 = !0;
    Data_Get_Struct(v, ip6_t, _ip6);
    ip6 = *_ip6;
  } else if (CLASS_OF(v) == Net4) {
    net4_t *_net4;
    is_net4 = !0;
    Data_Get_Struct(v, net4_t, _net4);
    net4 = *_net4;
  } else if (CLASS_OF(v) == Net6) {
    net6_t *_net6;
    is_net6 = !0;
    Data_Get_Struct(v, net6_t, _net6);
    net6 = *_net6;
  } else {
    const char *buf = StringValueCStr(v);

    if (read_net4_strict(buf, &net4)) is_net4 = !0;
    else if (read_net6_strict(buf, &net6)) is_net6 = !0;
    else if (read_ip4_strict(buf, &ip4)) is_ip4 = !0;
    else if (read_ip6_strict(buf, &ip6)) is_ip6 = !0;
  }

  for (ssize_t i = 0; i < RARRAY_LEN(nets); i++) {
    VALUE rbnet = RARRAY_AREF(nets, i);

    if (CLASS_OF(rbnet) == Net4) {
      if (is_net4) {
        net4_t *net;
        Data_Get_Struct(rbnet, net4_t, net);
        if (net4_include_net4_p(*net, net4)) {
          return Qtrue;
        }
      } else if (is_ip4) {
        net4_t *net;
        Data_Get_Struct(rbnet, net4_t, net);
        if (net4_include_p(*net, ip4)) {
          return Qtrue;
        }
      }
    }

    else if (CLASS_OF(rbnet) == Net6) {
      if (is_net6) {
        net6_t *net;
        Data_Get_Struct(rbnet, net6_t, net);
        if (net6_include_net6_p(*net, net6)) {
          return Qtrue;
        }
      } else if (is_ip6) {
        net6_t *net;
        Data_Get_Struct(rbnet, net6_t, net);
        if (net6_include_p(*net, ip6)) {
          return Qtrue;
        }
      }
    }

    else {
      VALUE ret = rb_funcall(rbnet, rb_intern("==="), 1, v);
      if (RTEST(ret)) return Qtrue;
    }
  }

  return Qfalse;
}

VALUE
method_ip_inspect(VALUE ip) {
  VALUE fmt = rb_str_new_cstr("#<%s %s>");
  VALUE args = rb_ary_new_from_args(2,
                                    rb_funcall(ip, rb_intern("class"), 0),
                                    rb_funcall(ip, rb_intern("to_s"), 0));
  return rb_funcall(fmt, rb_intern("%"), 1, args);
}

VALUE
method_net_inspect(VALUE net) {
  VALUE fmt = rb_str_new_cstr("#<%s address=%s prefixlen=%d mask=%s>");
  VALUE args = rb_ary_new_from_args(4,
                                    rb_funcall(net, rb_intern("class"), 0),
                                    rb_funcall(net, rb_intern("address"), 0),
                                    rb_funcall(net, rb_intern("prefixlen"), 0),
                                    rb_funcall(net, rb_intern("mask"), 0));
  return rb_funcall(fmt, rb_intern("%"), 1, args);
}

/**
 *
 */
void Init_Subnets() {
  rb_intern_hash = rb_intern("hash");
  rb_intern_xor = rb_intern("^");
  
  // Subnets
  Subnets = rb_define_module("Subnets");
  rb_define_singleton_method(Subnets, "parse", method_subnets_parse, 1);
  rb_define_singleton_method(Subnets, "include?", method_subnets_include_p, 2);

  // Subnets::ParseError
  ParseError = rb_define_class_under(Subnets, "ParseError", rb_eArgError);

  // Subnets::IP
  IP = rb_define_class_under(Subnets, "IP", rb_cObject);
  rb_define_method(IP, "inspect", method_ip_inspect, 0);

  // Subnets::IP4
  IP4 = rb_define_class_under(Subnets, "IP4", IP);
  rb_define_singleton_method(IP4, "random", method_ip4_random, -1);
  rb_define_singleton_method(IP4, "new", method_ip4_new, 1);
  rb_define_method(IP4, "==", method_ip4_eql_p, 1);
  rb_define_alias(IP4, "eql?", "==");
  rb_define_method(IP4, "hash", method_ip4_hash, 0);
  rb_define_method(IP4, "to_s", method_ip4_to_s, 0);
  rb_define_method(IP4, "to_i", method_ip4_to_i, 0);

  rb_define_method(IP4, "~", method_ip4_not, 0);
  rb_define_method(IP4, "|", method_ip4_bor, 1);
  rb_define_method(IP4, "^", method_ip4_xor, 1);
  rb_define_method(IP4, "&", method_ip4_band, 1);

  // Subnets::IP6
  IP6 = rb_define_class_under(Subnets, "IP6", IP);
  rb_define_singleton_method(IP6, "random", method_ip6_random, -1);
  rb_define_method(IP6, "==", method_ip6_eql_p, 1);
  rb_define_alias(IP6, "eql?", "==");
  rb_define_method(IP6, "hash", method_ip6_hash, 0);
  rb_define_method(IP6, "to_s", method_ip6_to_s, 0);
  rb_define_method(IP6, "to_i", method_ip6_to_i, 0);
  rb_define_method(IP6, "hextets", method_ip6_hextets, 0);

  rb_define_method(IP6, "~", method_ip6_not, 0);
  rb_define_method(IP6, "|", method_ip6_bor, 1);
  rb_define_method(IP6, "^", method_ip6_xor, 1);
  rb_define_method(IP6, "&", method_ip6_band, 1);

  // Subnets::Net
  Net = rb_define_class_under(Subnets, "Net", rb_cObject);
  rb_define_method(Net, "inspect", method_net_inspect, 0);

  // Subnets::Net4
  Net4 = rb_define_class_under(Subnets, "Net4", Net);
  rb_define_singleton_method(Net4, "parse", method_net4_parse, 1);
  rb_define_singleton_method(Net4, "random", method_net4_random, -1);
  rb_define_singleton_method(Net4, "new", method_net4_new, 2);
  rb_define_singleton_method(Net4, "summarize", method_net4_summarize, 1);
  rb_define_method(Net4, "==", method_net4_eql_p, 1);
  rb_define_alias(Net4, "eql?", "==");
  rb_define_method(Net4, "hash", method_net4_hash, 0);
  rb_define_method(Net4, "to_s", method_net4_to_s, 0);
  rb_define_method(Net4, "prefixlen", method_net4_prefixlen, 0);
  rb_define_method(Net4, "include?", method_net4_include_p, 1);
  rb_define_alias(Net4, "===", "include?");

  rb_define_method(Net4, "address", method_net4_address, 0);
  rb_define_method(Net4, "mask", method_net4_mask, 0);

  // Subnets::Net6
  Net6 = rb_define_class_under(Subnets, "Net6", Net);
  rb_define_singleton_method(Net6, "parse", method_net6_parse, 1);
  rb_define_singleton_method(Net6, "random", method_net6_random, -1);
  rb_define_singleton_method(Net6, "new", method_net6_new, 2);
  rb_define_singleton_method(Net6, "summarize", method_net6_summarize, 1);
  rb_define_method(Net6, "==", method_net6_eql_p, 1);
  rb_define_alias(Net6, "eql?", "==");
  rb_define_method(Net6, "hash", method_net6_hash, 0);
  rb_define_method(Net6, "to_s", method_net6_to_s, 0);
  rb_define_method(Net6, "prefixlen", method_net6_prefixlen, 0);
  rb_define_method(Net6, "include?", method_net6_include_p, 1);
  rb_define_method(Net6, "hextets", method_net6_hextets, 0);
  rb_define_alias(Net6, "===", "include?");

  rb_define_method(Net6, "address", method_net6_address, 0);
  rb_define_method(Net6, "mask", method_net6_mask, 0);
}

void Init_subnets() {
  // this is so YARD picks up the (case sensitive) Subnets docstring
  Init_Subnets();
}
