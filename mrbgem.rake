MRuby::Gem::Specification.new('mruby-augeas') do |spec|
  spec.license = 'MIT'
  spec.author  = 'lutter'
  spec.version = '0.1.0'
  spec.summary = 'Augeas bindings'

  add_dependency 'mruby-hash-ext'

  unless spec.respond_to?(:search_package) && spec.search_package('augeas')
    spec.cc.flags << '-I /usr/include/libxml2'
    spec.linker.libraries << 'augeas'
  end
end
