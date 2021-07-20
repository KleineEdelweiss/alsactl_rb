# ext/alsacore/extconf.rb
require 'mkmf'

# Build AlsaCore
$LFLAGS = '-lasound'
have_library("asound")
have_header("alsa/asoundlib.h")
have_func("snd_mixer_open")
have_func("snd_mixer_close")
create_makefile("alsacore")