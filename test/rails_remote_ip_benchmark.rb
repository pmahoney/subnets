require 'benchmark_helper'

require 'action_dispatch/middleware/remote_ip'

def benchmark(name, getip, mkrequest)
  total = count = request_count = untrusted = 0

  until total >= 1
    request = mkrequest.call

    total += Benchmark.measure {
      untrusted += getip.send(:filter_proxies, request).size
    }.total

    count += request.size
    request_count += 1
  end

  trusted = count - untrusted
  puts
  puts "checked %d requests at %.2fμs/req finding %d trusted ips (%d%%)" %
       [request_count, total/request_count*1e6, trusted, 100.0*trusted/count]

  plotbarslogscale(prefix: '%-15.15s %7.2fμs/req ',
                   width: 36, min: 1, max: 2000, tics: [1,10,100,1000],
                   data: { name => total/request_count*1e6 })
end

pub_priv_priv = ->() {
  [
    random_ip('0.0.0.0/0'),
    random_ip(PRIVATE_SUBNETS_IPV4.sample),
    random_ip(PRIVATE_SUBNETS_IPV4.sample),
  ].map(&:to_s)
}

############################################################
# as-is rails remote_ip

proxies = ActionDispatch::RemoteIp::TRUSTED_PROXIES
getip = ActionDispatch::RemoteIp::GetIp.new(nil, nil, proxies)
benchmark('remote_ip', getip, pub_priv_priv)

############################################################
# as-is rails remote_ip with CloudFront

pub_cf_priv_priv = ->() {
  [
    random_ip('0.0.0.0/0'),
    random_ip(CLOUDFRONT_SUBNETS.sample),
    random_ip(PRIVATE_SUBNETS_IPV4.sample),
    random_ip(PRIVATE_SUBNETS_IPV4.sample),
  ].map(&:to_s)
}

proxies = ActionDispatch::RemoteIp::TRUSTED_PROXIES +
          CLOUDFRONT_SUBNETS.map(&IPAddr.method(:new))
getip = ActionDispatch::RemoteIp::GetIp.new(nil, nil, proxies)
benchmark('remote_ip +CF', getip, pub_cf_priv_priv)

############################################################
# naive replacement of IPAddr with Subnets-derived objects

proxies = ActionDispatch::RemoteIp::TRUSTED_PROXIES.
            map{|p| "#{p}/#{p.prefix}"}.map(&Subnets.method(:parse))
getip = ActionDispatch::RemoteIp::GetIp.new(nil, nil, proxies)
benchmark('subnets', getip, pub_priv_priv)

proxies = ActionDispatch::RemoteIp::TRUSTED_PROXIES.
            map{|p| "#{p}/#{p.prefix}"}.map(&Subnets.method(:parse)) +
          CLOUDFRONT_SUBNETS.map(&Subnets.method(:parse))
getip = ActionDispatch::RemoteIp::GetIp.new(nil, nil, proxies)
benchmark('subnets +CF', getip, pub_cf_priv_priv)

############################################################
# hack to use fast Subnets.include?

Identity = Object.new
def Identity.===(v); v; end

# instead of letting Ruby call the block for every element of the
# list, call it once to extract the ip to be tested, then use fast
# Subnets.include? method.
proxies2 = proxies.dup

def proxies2.any?(&block)
  ip = block.call(Identity)
  Subnets.include?(self, ip)
end

getip = ActionDispatch::RemoteIp::GetIp.new(nil, nil, proxies2)
benchmark('hack +CF', getip, pub_cf_priv_priv)

############################################################
# Alternate impl to really use fast Subnets.include?

Alternate = Struct.new(:proxies) do
  def filter_proxies(ips)
    ips.reject{|ip| Subnets.include?(self.proxies, ip)}
  end
end

getip = Alternate.new(proxies)
benchmark('alt +CF', getip, pub_cf_priv_priv)
