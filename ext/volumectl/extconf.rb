# ext/volumectl/extconf.rb
require 'mkmf'

# Build VolumeCtl
$LFLAGS = '-lasound'
have_library("asound")
have_header("alsa/asoundlib.h")
have_func("snd_mixer_open")
have_func("snd_mixer_close")
create_makefile("volumectl/volumectl")