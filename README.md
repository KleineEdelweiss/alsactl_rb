### OVERVIEW ###
VolumeCtl is a C-Ruby API wrapper for the ALSA library on Linux. By default, it handles the Master/default mixer object (default volume).

### USAGE ###
```ruby
m = VolumeCtl::Mixer.new # Creates a new mixer object
m.connect # or m.connect("default", "Master"), for example
m.enum # List off all the valid channels for the device
m.volume # Returns the volume
m.volume_a # Returns the volumes of all valid channels
# .volume_a and .volume will be merged in one of the next updates
m.volume_c :channel_id # Get the volume of a specific channel
# ex 1: m.volume_c 3 <- gets the volume of the 4th channel (indices start at 0)
# ex 2: m.volume_c 300000 <- will return FALSE, b/c this channel isn't valid
# ex 3: m.volume_c "pizza" <- will return NIL, b/c this TYPE isn't valid
m.set_volume :percent, :change_type
# ex 1: m.set_volume 15, -1 <- Turns the volume down by 15%
# ex 2: m.set_volume 15, 1 <- Turns the volume up by 15%
# ex 3: m.set_volume 15, 0 <- Turns the volume to 15%
m.mute # Mutes the volume, storing the current volume or else a 50% default
m.unmute # Unmutes the volume
m.disconnect # Disconnects from the ALSA server -- do NOT use after, will cause crash (until I update)
```

### TO-DO ###
1) Check over gemspec
1) Add a way to MODIFY data for EACH/SPECIFIED channel(s)
1) Clean up the setter methods, b/c requiring an int is not intuitive...
1) Merge ``.volume`` and ``.volume_a``
1) Make the volume setters work by channel as ``.set_volume_c`` separate from ``.set_volume_a``
1) Make the method that sets all volumes have either relative or flat (relative to all channel-current volumes; flat, as in all channels identical, but value-set will automatically be flat)
1) Do a test install, after I clean up a bit more
1) Publish gem
1) Abstract VolumeCtl::Mixer, so you can use mixers other than the default
1) Add some error-handling, such as trying to operate on a disconnected mixer
1) After abstracting for non-default mixers, add error-handling, in case of a failed connection

### INSTALLATION ###
