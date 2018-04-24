require 'rake/extensiontask'
require 'rake/testtask'

task :default => [:compile, :test]

Rake::ExtensionTask.new('subnets')

Rake::TestTask.new do |t|
  t.libs << "test"
  t.test_files = Dir['test/**/*_test.rb']
  t.verbose = true
end

# focus on one like: bundle exec rake benchmark TEST=test/custom_benchmark
Rake::TestTask.new(:benchmark) do |t|
  t.libs << "test"
  t.test_files = Dir['test/**/*_benchmark.rb']
  t.verbose = true
end

task :afl do
  sh "afl-gcc -o afltest test/afltest.c ext/subnets/ipaddr.c -Iext/subnets"
  sh "afl-fuzz -i test/afl-tests -o reports/afl-findings ./afltest"
end

