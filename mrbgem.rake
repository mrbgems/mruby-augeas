MRuby::Gem::Specification.new('mruby-augeas') do |spec|
  spec.license = 'MIT'
  spec.author  = 'lutter'
  spec.version = '0.1.0'
  spec.summary = 'Augeas bindings'

  spec.cc.flags << '-I /usr/include/libxml2'
  spec.linker.libraries << 'augeas'
  add_dependency 'mruby-hash-ext'
end
