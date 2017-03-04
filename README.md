# mruby-augeas

[mruby](http://mruby.org/) bindings for [augeas](http://augeas.net)

## Usage

To include `mruby-augeas` in your build of `mruby`, you need to have the
[augeas](http://augeas.net) libraries installed. Then add this to your
`build_config.rb`:

```ruby
  conf.gem :github => "hercules-team/mruby-augeas"
```

The bindings themselves follow the
[ruby-augeas](https://github.com/hercules-team/ruby-augeas) bindings as
closely as is possible under `mruby`.
