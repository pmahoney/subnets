Gem::Specification.new do |s|
  s.name = 'subnets'
  s.version = '1.0.1pre'
  s.licenses    = ['MIT']
  s.platform = Gem::Platform::RUBY
  s.has_rdoc = true
  s.extra_rdoc_files = ["README.md", "COPYING"]
  s.summary = 'C-extension for dealing with IP addresses and subnets'
  s.description = <<~EOF
    C implementation of IPv4, IPv6 address and network parser with
    fast determination of IP membership within a given subnet.
  EOF
  s.author = 'Patrick Mahoney'
  s.email = 'patrick.mahoney@raise.com'
  s.homepage = 'https://github.com/raisemarketplace/subnets'
  s.files = Dir['lib/**/*.rb', 'ext/**/*.{c,h,rb}', 'test/**/*.rb']
  s.extensions = Dir['ext/**/extconf.rb']

  s.add_development_dependency('minitest')
  s.add_development_dependency('minitest-reporters')
  s.add_development_dependency('rake')
  s.add_development_dependency('rake-compiler')
  s.add_development_dependency('yard')

  # for benchmarks
  s.add_development_dependency('ipaddr')
  s.add_development_dependency('ipaddress')
  s.add_development_dependency('netaddr')
  s.add_development_dependency('rack')
  s.add_development_dependency('rpatricia')
  s.add_development_dependency('actionpack')
end
