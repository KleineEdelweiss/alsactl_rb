# lib/alsactl.rb

# This module include a C wrapper for
# controlling basic Linux ALSA volume
# settings on the system
require_relative("./alsactl/mixers")
require_relative("./alsactl/version")