require 'minitest/autorun'
require 'minitest/reporters'

class JUnitReporterWithSuitePrefix < Minitest::Reporters::JUnitReporter
  def initialize(prefix, *args)
    super(*args)
    @prefix = prefix
  end

  def parse_xml_for(xml, suite, tests)
    super(xml, "#{@prefix}#{suite}", tests)
  end
end

Minitest::Reporters.use! [Minitest::Reporters::SpecReporter.new,
                          JUnitReporterWithSuitePrefix.new("ruby-#{RUBY_VERSION}/")]

require 'subnets'
require 'well_known_subnets'
require 'eql_and_hash'

TIMED_TEST_DURATION = (ENV['TIMED_TEST_DURATION'] || 1).to_i

def assert_include(obj, val)
  assert obj.include?(val), "#{obj} should include #{val}"
end

def refute_include(obj, val)
  refute obj.include?(val), "#{obj} should not include #{val}"
end
