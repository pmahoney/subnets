require 'test_helper'

module Subnets
  class TestIP4 < Minitest::Test
    include EqlAndHash

    def klass
      IP4
    end

    def constructor_args
      [1]
    end

    def other_constructor_args
      [345728]
    end
  end
end
