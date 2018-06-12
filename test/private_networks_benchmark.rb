require 'benchmark_helper'

require 'ipaddr'
require 'ipaddress'
require 'netaddr'
require 'subnets'
require 'rack'
require 'rpatricia'

nets = PRIVATE_SUBNETS_IPV4
ipaddr_nets = nets.map(&IPAddr.method(:new))
ipaddress_nets = nets.map(&IPAddress::IPv4.method(:new))
netaddr_nets = nets.map(&NetAddr::IPv4Net.method(:parse))
subnets_nets = nets.map(&Subnets.method(:parse))
rack_nets = Rack::Request.new({})
rpatricia_nets = nets.reduce(Patricia.new) {|acc, net| acc.add(net, true); acc }

# note: ipaddress and netaddr checks would need additional cases to handle ipv6 too
ipaddr_check = lambda {|ip| ipaddr_nets.any? { |net| net.include? ip } }
ipaddress_check = lambda {|ip| ipaddress_nets.any? { |net| net.include?(IPAddress::IPv4.new(ip)) } }
netaddr_check = lambda {|ip|  netaddr_nets.any? { |net| net.contains(NetAddr::IPv4.parse(ip)) } }
subnets_check = lambda {|ip| Subnets.include?(subnets_nets, ip) }
rack_check = lambda {|ip| rack_nets.trusted_proxy?(ip) }
rpatricia_check = lambda {|ip| rpatricia_nets.include?(ip) }

def ipaddr_check.name; 'ipaddr'; end
def ipaddress_check.name; 'ipaddress'; end
def netaddr_check.name; 'netaddr'; end
def subnets_check.name; 'subnets'; end
def rack_check.name; 'rack (regexp)'; end
def rpatricia_check.name; 'rpatricia'; end

results = {}

puts '#'*60
puts "# check if single IP is in the private IPv4 subnets"
[ipaddr_check, ipaddress_check, netaddr_check, subnets_check, rack_check, rpatricia_check].each do |check|
  total = count = hits = 0
  until total >= 2
    net = Subnets.parse((['0.0.0.0/0'] + PRIVATE_SUBNETS_IPV4).sample)
    ip = (Subnets::IP4.random & ~net.mask | net.address).to_s

    total += Benchmark.measure {
      100.times do |i|
        hits += 1 if check.call(ip)
      end
    }.total
    count += 100
  end
  puts "%10.10s: checked %8d (%6d hits, %2d%%) in %2.2f for %7.2fμs/ip" %
       [check.name, count, hits, 100.0*hits/count, total, total/count*1e6]

  results[check.name] = total/count*1e6
end

puts
plotbarslogscale(prefix: '%-15.15s: %5.2fμs/ip ', width: 46, min: 2, max: 65, tics: [2,5,10,20,50], data: results)
