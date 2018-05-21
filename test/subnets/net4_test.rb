require 'test_helper'

module Subnets
  class TestNet4 < Minitest::Test
    include EqlAndHash
    include Summarize

    def klass
      Net4
    end

    def constructor_args
      [1,24]
    end

    def other_constructor_args
      [345728,16]
    end

    def test_new_creates_net4
      assert_instance_of Net4, Net4.new(1, 1)
    end

    def test_new_rejects_invalid_prefixlen
      assert_raises(ArgumentError) { Net4.new(1, 33) }
    end

    class ParseBadInput < Minitest::Test
      data = [
        'a', '12', '1.2.3', '1/2', '',
      ]

      data.each_with_index do |s, i|
        define_method("test_parse_fail_%02d" % i) do
          assert_raises(ParseError) { Net4.parse(s) }
        end
      end
    end

    class Parse < Minitest::Test
      data = ['127.0.0.1/32', '192.168.0.0/24', '10.1.0.0/16', '1.2.3.4/2']

      data.each_with_index do |cidr, i|
        define_method("test_parse_%02d" % i) do
          assert_instance_of Net4, Net4.parse(cidr)
        end
      end
    end

    def test_includes_ip
      net = Net4.parse '192.168.0.0/24'
      assert_include net, '192.168.0.0'
      assert_include net, Subnets.parse('192.168.0.2')
      assert_include net, '192.168.0.255'

      refute_include net, '192.168.1.0'
      refute_include net, Subnets.parse('10.168.0.2')
    end

    def test_includes_net
      net = Net4.parse '192.168.0.0/24'
      assert_include net, '192.168.0.0/24'
      assert_include net, Net4.parse('192.168.0.2/26')
      refute_include net, '192.168.0.0/22'
      refute_include net, Net4.parse('10.168.0.0/16')
    end

    def test_random_roundtrip
      random = Random.new
      start = Time.now
      until Time.now - start > TIMED_TEST_DURATION
        1000.times do
          net = Net4.random(random).to_s
          assert_equal net, Net4.parse(net).to_s
        end
      end
    end

    def test_summarize
      data = {
        '192.168.0.0/24' => ['192.168.0.0/25', '192.168.0.128/25'],
        '10.0.0.0/8' => ['10.0.0.0/24', '10.250.2.3/19'],
      }

      data.each do |summ, nets|
        summ = Subnets.parse(summ)
        nets = nets.map(&Subnets.method(:parse))

        assert_equal summ, Net4.summarize(nets)
        assert_summarizes summ, nets
      end
    end
  end
end
