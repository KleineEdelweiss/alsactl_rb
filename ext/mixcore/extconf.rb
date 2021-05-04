# ext/mixcore/extconf.rb
require 'mkmf'

# Build MixCore
$LFLAGS = '-lasound'
have_library("asound")
have_header("alsa/asoundlib.h")
have_func("snd_mixer_open")
have_func("snd_mixer_close")
create_makefile("mixcore/mixcore")