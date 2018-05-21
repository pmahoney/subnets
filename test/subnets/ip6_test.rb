require 'test_helper'

module Subnets
  class TestIP6 < Minitest::Test
    include EqlAndHash

    # for EqlAndHash
    def klass
      IP6
    end

    # for EqlAndHash
    def constructor_args
      [[1,2,3,4,5,6,7,8]]
    end

    # for EqlAndHash
    def other_constructor_args
      [[5,5,5,5,5,5,5,5]]
    end

  end
end

