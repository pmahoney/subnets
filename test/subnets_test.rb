require 'test_helper'
require 'subnets'

class SubnetsTest < Minitest::Test
  def test_parse_net4
    assert_instance_of Subnets::Net4, Subnets.parse('192.168.1.1/24')
    assert_raises(Subnets::ParseError) { Subnets.parse('192.168.1.256/24') }
  end

  def test_parse_net6
    assert_instance_of Subnets::Net6, Subnets.parse('1::1/2')
    assert_raises(Subnets::ParseError) { Subnets.parse(':1::') }
  end

  def test_include?
    nets = %w(
      192.168.5.0/24
      10.1.0.0/16
      11:22::/16
    ).map{|n| Subnets.parse(n)}

    nets << /^192\./
    nets << /someregex/
    
    assert Subnets.include?(nets, '192.168.5.4')
    assert Subnets.include?(nets, '10.1.1.1')
    assert Subnets.include?(nets, '11:22::33')
    assert Subnets.include?(nets, '11:22:33::')
    assert Subnets.include?(nets, 'someregex')

    refute Subnets.include?(nets, '1.2.3.4')
    refute Subnets.include?(nets, '::1')
    refute Subnets.include?(nets, '33::')
  end
end
