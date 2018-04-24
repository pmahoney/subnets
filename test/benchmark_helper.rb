require 'benchmark'
require 'ipaddr'
require 'ipaddress'
require 'socket'

require 'subnets'
require 'well_known_subnets'

# RANDOM_IPS = (
#   (1..1000).map{random_ipv4(CLOUDFRONT_SUBNETS.sample)} +
#   (1..3000).map{PRIVATE_SUBNETS_IPV4.sample}
#   (1..4000).map{random_ipv4}
# ).shuffle

def random_ip(subnet)
  net = Subnets.parse(subnet)
  Subnets::IP4.random & ~net.mask | net.address
end

def plotbarslogscale(prefix: '%-15.15s %5.2f ', width:, min:, max:, tics:[], data:{})
  pos = proc { |x| (Math.log10(x) - Math.log10(min)) * width/Math.log10(max) }

  # https://en.wikipedia.org/wiki/Block_Elements
  eighths = {
    8 => "\u2588",
    7 => "\u2589",
    6 => "\u258a",
    5 => "\u258b",
    4 => "\u258c",
    3 => "\u258d",
    2 => "\u258e",
    1 => "\u258f",
    0 => "",
  }

  bar = proc do |w|
    eighths[8]*(w.floor) + eighths[((w - w.floor)*8).round]
  end

  prefixwidth = 0
  data.each do |k,v|
    str = prefix % [k,v]
    prefixwidth = [prefixwidth, str.size].max
    puts(str + bar.call(pos.call(v)))
  end

  if tics.size > 0
    ticbar = ''
    labelbar = ''
    tics.each do |tic|
      x = pos.call(tic).round
      ticbar += (' '*(x - ticbar.size) + "'")
      labelbar += (' '*(x - labelbar.size) + "#{tic}")
    end

    puts(' '*prefixwidth + ticbar)
    puts(' '*prefixwidth + labelbar)
  end
end
