# Subnets

Subnets is a C-extension for Ruby with IPv4 and IPv6 address and
network classes. The motivating goal is to provide a fast test whether
a given set of subnets includes a given IP.

```ruby
Subnets.include?(subnets, '192.168.1.1')
```

[Rack::Request#ip](https://github.com/rack/rack/blob/2.0.4/lib/rack/request.rb#L419-L421)
and
[ActionDispatch::Request#remote_ip](https://github.com/rails/rails/blob/v5.2.0/actionpack/lib/action_dispatch/middleware/remote_ip.rb#L176-L179)
perform such a test. `ActionDispatch`, which uses `ipaddr` by default,
explicitly calls out that the technique is too slow to run on every
request. Rack uses a regular expression of IPv4 and IPv6 private
address spaces which, while comparably fast, is not easily extended to
support arbitrary subnets.

(See also [this answer explaining request.ip
vs. request.remote_ip](https://stackoverflow.com/a/43014286/454156))

A [benchmark](test/private_networks_benchmark.rb) tests if a random IP
(75% private IPs) is within any of the private IPv4 ranges. Lower is
better. Plotted on logscale. (Ruby 2.5.0p0, 2.5 GHz Intel Core i7).

```
$ bundle exec rake benchmark TEST=test/private_networks_benchmark
    
 ipaddr         : 46.25μs/ip ██████████████████████████████████▋
 ipaddress      : 63.88μs/ip ██████████████████████████████████████▏
 netaddr        : 31.03μs/ip ██████████████████████████████▎
*subnets        :  4.19μs/ip ████████▏
 rack (regexp)  :  5.25μs/ip ██████████▋
                             '         '       '      '         '
                             2         5       10     20        50
```

## Usage

```ruby
require 'subnets'

subnets = %w(127.0.0.1/32 10.0.0.0/8 172.16.0.0/12 192.168.0.0/16
  ::1/128 fc00::/7).map(&Subnets.method(:parse))

Subnets.include?(subnets, '192.168.1.1') #=> true
Subnets.include?(subnets, '203.0.113.12') #=> false
```

## Similar Gems

There are several IP gems, all of which are implemented in pure-Ruby
and not performance oriented.

- [ipaddr](https://github.com/ruby/ipaddr): (Ruby stdlib) A set of
  methods to manipulate an IP address. Both IPv4 and IPv6 are
  supported.
- [ipaddress](https://github.com/ipaddress-gem/ipaddress): A Ruby
  library designed to make the use of IPv4 and IPv6 addresses simple,
  powerful and enjoyable. It provides a complete set of methods to
  handle IP addresses for any need, from simple scripting to full
  network design.
- [netaddr](https://github.com/dspinhirne/netaddr-rb): A Ruby library
  for performing calculations on IPv4 and IPv6 subnets. There is also
  limited support for EUI addresses.

## Production Ready?

This has not been used in production.

## Safe?

The IPv4 and IPv6 parsers are written in C. In addition to the unit
test suite, the parsers have had minimal (16+ hours) of fuzzing with
[American fuzzy lop](http://lcamtuf.coredump.cx/afl/). There is medium
confidence that the parsers will not read out-of-bounds.

## Correct?

The unit test suite tests parsing of a variety of valid and invalid
IPv4 and IPv6 networks.

## Fast?

Yes, for checking if an array of subnets includes a given IP at least.
