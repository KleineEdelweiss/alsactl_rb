# lib/volumectl.rb

# This module include a C wrapper for
# controlling basic Linux ALSA volume
# settings on the system
require_relative("./volumectl/volumectl")
require_relative("./volumectl/version")