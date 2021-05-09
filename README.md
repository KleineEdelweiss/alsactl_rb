### OVERVIEW ###
AlsaCtl is a C-Ruby API wrapper with a pure Ruby middleware, for the ALSA library on Linux. By default, it handles the ``default/Master`` mixer object (current default volume controller). The base class, ``AlsaCore::BaseMixer`` cannot be instantiated, and all methods outside of allocation are protected, so it acts as an abstract-only class. However, you may derive from it, such as the ``AlsaCtl::DefMixer`` object ``(class DefMixer < AlsaCore::BaseMixer)``.

### USAGE ###
```ruby
m = AlsaCtl::DefMixer.new # Creates a new default mixer object
m.connect # By default, calls BaseMixer.connect("default", "Master")
m.disconnect # Disconnects from the ALSA server -- do NOT use after, will cause crash (until I update)
m.close # Same as above

m.enum # List off all the valid channels for the device

m.volume # Returns the volumes by channel
m.vget # Same as above
m.c_vget :channel_id # Get the volume of a specific channel
# ex 1: m.c_vget 3 <- gets the volume of the 4th channel (indices start at 0)
# ex 2: m.c_vget 300000 <- will return FALSE, b/c this channel isn't valid
# ex 3: m.c_vget "pizza" <- will return NIL, b/c this TYPE isn't valid

m.set_volume :percent, :change_type # Sets the volume of all channels
m.vset :percent, :change_type # Same as above
# ex 1: m.set_volume 15, -1 <- Turns the volume down by 15%
# ex 2: m.set_volume 15, 1 <- Turns the volume up by 15%
# ex 3: m.set_volume 15, 0 <- Turns the volume to 15%
m.c_vset :channel, :percent, :change_type # Set single channel's volume
# ex 1: m.c_vset 1, 15, :lower (or :down, :decrease) <- Turns the volume down by 15%
# ex 2: m.c_vset 1, 15, :up (or :raise, :increase) <- Turns the volume up by 15%
# ex 3: m.c_vset 1, 15, nil (or anything else) <- Turns the volume to 15%

m.mute # Mutes the volume, storing the current volume or else a 50% default
m.unmute # Unmutes the volume
```

### TO-DO ###
1) Start the next version, 0.2.0 _WITH_ 
1) Create derived class from ``DefMixer`` with non-flat volume modulation (such as for use in spatially-oriented physical speaker configurations)
1) Derive other desirable classes for other common controllers
1) EVENTUALLY, add PCM and capture classes

### INSTALLATION ###
```
gem install alsactl
```