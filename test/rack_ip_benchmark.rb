require 'benchmark_helper'

require 'rack'

def rack_trusted_proxy_benchmark(name, request)
  total = count = hits = 0
  until total >= 3
    ip = random_ip((['0.0.0.0/0'] + PRIVATE_SUBNETS_IPV4).sample).to_s

    100.times {
      is_trusted = false
      total += Benchmark.measure {
        is_trusted = request.trusted_proxy?(ip)
      }.total
      hits += 1 if is_trusted
      count += 1
    }
  end

  puts
  puts "checked %d IPs @ %.2fμs/ip (%d%% trusted)" %
       [count, 1e6*total/count, 100.0*hits/count]
  plotbarslogscale(
    prefix: '%-12.12s %4.2fμs/ip ', width: 36, min: 1, max: 5, tics: [1,2,5],
    data: { name => 1e6*total/count })
end

rack_trusted_proxy_benchmark(
  'rack', Rack::Request.new({}))

subnets = PRIVATE_SUBNETS.map(&Subnets.method(:parse))
def subnets.trusted_proxy?(ip)
  Subnets.include?(self, ip)
end

rack_trusted_proxy_benchmark(
  'subnets', subnets)

subnets_cf = (PRIVATE_SUBNETS + CLOUDFRONT_SUBNETS).map(&Subnets.method(:parse))
def subnets_cf.trusted_proxy?(ip)
  Subnets.include?(self, ip)
end

rack_trusted_proxy_benchmark(
  'subnets +CF', subnets_cf)
