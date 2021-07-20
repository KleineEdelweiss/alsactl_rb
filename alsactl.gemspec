# frozen_string_literal: true
require_relative "lib/alsactl/version.rb"

# Spec
Gem::Specification.new do |spec|
  spec.name = "alsactl"
  spec.version = AlsaCtl::VERSION
  spec.summary = "ALSA wrapper in the C-Ruby API"
  spec.description = <<~DESC
    ALSA wrapper in Ruby, utilizing a purely-abstract C-Ruby API backend
    and a pure Ruby middleware with extensibility.
  DESC
  spec.authors = ["Edelweiss"]
  
  # Website data
  spec.homepage = "https://github.com/KleineEdelweiss/alsactl_rb"
  spec.licenses = ["LGPL-3.0"]
  spec.metadata = {
    "homepage_uri"        => spec.homepage,
    "source_code_uri"     => "https://github.com/KleineEdelweiss/alsactl_rb",
    #"documentation_uri"   => "",
    "changelog_uri"       => "https://github.com/KleineEdelweiss/alsactl_rb/blob/master/CHANGELOG.md",
    "bug_tracker_uri"     => "https://github.com/KleineEdelweiss/alsactl_rb/issues"
    }
  
  # List of files
  spec.files = Dir.glob ["lib/**/*.rb", "ext/**/*.{c,cpp,h,hpp,rb}", "Rakefile"]
  
  # Rdoc options
  spec.extra_rdoc_files = Dir["README.md","CHANGELOG.md", "LICENSE.txt", "changes/*.md"]
  spec.rdoc_options += [
    "--title", "AlsaCtl -- ALSA wrapper in the C-Ruby API",
    "--main", "README.md",
    
    # Exclude task data from rdoc
    "--exclude", "Makefile",
    "--exclude", "Rakefile",
    "--exclude", "Gemfile",
    "--exclude", "alsactl.gemspec",
    "--exclude", "rdoc.sh",
    
    # Other options
    "--line-numbers",
    "--inline-source",
    "--quiet"
  ]
  
  # Minimum Ruby version
  spec.required_ruby_version = ">= 2.7.0"
  
  # Compiled extensions
  spec.extensions = ['ext/alsacore/extconf.rb']
end # End spec