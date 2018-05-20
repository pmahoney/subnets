require 'test_helper'
require 'ipaddr'

module Subnets
  class TestNet6 < Minitest::Test
    include EqlAndHash
    include Summarize

    # for EqlAndHash
    def klass
      Net6
    end

    # for EqlAndHash
    def constructor_args
      [[1,2,3,4,5,6,7,8], 96]
    end
    
    class New < Minitest::Test
      def test_rejects_invalid_prefixlen
        assert_raises(ArgumentError) { Net6.new([0,0,0,0,0,0,0,0], 129) }
      end

      def test_rejects_invalid_hextets
        assert_raises(ArgumentError) { Net6.new([0,0,0,0,0,0,0], 128) }
        assert_raises(ArgumentError) { Net6.new([0,0,0,0,0,0,0,0,0], 128) }
      end

      def test_creates_net6
        assert_equal Net6.parse('1:2:44:55:ef::/128'),
                     Net6.new([0x1,0x2,0x44,0x55,0xef,0,0,0], 128)
      end
    end

    class ParseBadInput < Minitest::Test
      data = [
        { name: 'leading single colon', s: ':1::' },
        { name: 'non-hexadecimal', s: 'u' },
        { name: 'too short', s: '12:' },
        { name: 'too short', s: ':1' },
        { name: 'missing compression', s: '1/2' },
        { name: 'trailing single colon', s: '1::1:/1' },
        { name: 'leading zero in hextet', s: '045:1:2::/32' },
        { name: 'prefixlen too large', s: '1::/129' },
      ]

      data.each_with_index do |d, i|
        define_method("test_parse_fail_%02d #{d[:name]}" % i) do
          assert_raises(ParseError) { Net6.parse(d[:s]) }
        end
      end
    end

    class Parse < Minitest::Test
      data = [
        { name: 'zero', s: '::/0' },
        { name: 'localhost', s: '::1/128' },
        { name: 'first-one', s: '1::/128' },
        { name: 'one-one', s: '1::1/32' },
        { name: 'ip', s: 'ffe3::ff01/32' },
        { name: 'ip', s: '46db:20af:2b68:4034::871f:0/83' },
        { name: 'full', s: '1:2:3:4:5:6:7:8/96' },
        { name: 'many-zeros', s: '0:0:0:1:0:0:0:0/44', to_s: '0:0:0:1::/44' },
        { name: 'embedded ipv4', s: '::1.2.3.4/96', to_s: '::102:304/96' },
        { name: 'embedded ipv4', s: 'fe:55::1.2.3.4/96', to_s: 'fe:55::102:304/96' },
        { name: 'embedded ipv4', s: 'a:b:c:d:e:f:1.2.3.4/128', to_s: 'a:b:c:d:e:f:102:304/128' },
      ]

      data.each_with_index do |d, i|
        define_method("test_parse_%02d #{d[:name]}" % i) do
          assert_instance_of Net6, Net6.parse(d[:s])
        end

        define_method("test_to_s_%02d #{d[:name]}" % i) do
          assert_equal (d[:to_s] || d[:s]), Net6.parse(d[:s]).to_s
        end

        define_method("test_prefixlen_%02d #{d[:name]}" % i) do
          raise "#{d[:s]} didn't match regex" unless d[:s] =~ /\/(\d+)\z/
          assert_equal $1.to_i, Net6.parse(d[:s]).prefixlen
        end
      end
    end

    def test_random_roundtrip
      random = Random.new
      start = Time.now
      until Time.now - start > TIMED_TEST_DURATION
        1000.times do
          net = Net6.random(random, zeros: true).to_s
          assert_equal net, Net6.parse(net).to_s
        end
      end
    end

    def test_prefixlen
      (0..128).each do |i|
        assert_equal i, Net6.parse("::1/#{i}").prefixlen
      end
    end

    class Include < Minitest::Test
      def setup
        @net = Net6.parse '1:2:3:4:5:6:7:8/96'
      end

      def test_includes_ips
        ['1:2:3:4:5:6:7:9', '1:2:3:4:5:6:99::'].each do |ip|
          assert_include @net, ip
        end
      end

      def test_excludes_ips
        ['5::', '1:2:3:99::'].each do |ip|
          refute_include @net, ip
        end
      end

      def test_returns_false_with_non_ips_or_nets
        ['a', /a/, 2].each do |obj|
          refute_include @net, obj
        end
      end
    end
  end
end
