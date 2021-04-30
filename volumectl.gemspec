# frozen_string_literal: true
require_relative "lib/volumectl/version.rb"

# Spec
Gem::Specification.new do |spec|
  spec.name = "volumectl"
  spec.version = VolumeCtl::VERSION
  spec.summary = "ALSA wrapper in Ruby"
  spec.description = <<~DESC
    ALSA wrapper in Ruby
  DESC
  spec.authors = ["Edelweiss"]
  
  # Website data
  spec.homepage = "https://github.com/KleineEdelweiss/volumectl_rb"
  spec.licenses = ["LGPL-3.0"]
  spec.metadata = {
    "homepage_uri"        => spec.homepage,
    "source_code_uri"     => "https://github.com/KleineEdelweiss/volumectl_rb",
    #"documentation_uri"   => "",
    #"changelog_uri"       => "https://github.com/KleineEdelweiss/volumectl_rb/blob/master/CHANGELOG.md",
    "bug_tracker_uri"     => "https://github.com/KleineEdelweiss/volumectl_rb/issues"
    }
  
  # List of files
  spec.files = Dir.glob("lib/**/*")
  
  # Rdoc options
  spec.extra_rdoc_files = Dir["README.md", "CHANGELOG.md", "LICENSE.txt"]
  spec.rdoc_options += [
    "--title", "VolumeCtl -- Generic Linux system access interface in Ruby",
    "--main", "README.md",
    "--line-numbers",
    "--inline-source",
    "--quiet"
  ]
  
  # Minimum Ruby version
  spec.required_ruby_version = ">= 2.7.0"
  
  # Compiled extensions
  spec.extensions = ['ext/volumectl/extconf.rb']
end # End spec