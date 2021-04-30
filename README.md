### OVERVIEW ###
VolumeCtl is a C-Ruby API wrapper for the ALSA library on Linux. By default, it handles the Master/default mixer object (default volume).

### USAGE ###
```ruby
m = VolumeCtl::Mixer.new # Creates a new mixer object
m.connect # or m.connect("default", "Master"), for example
m.volume # Returns the volume
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
1) Add a way to get data from EACH channel
1) Add a way to MODIFY data for EACH/SPECIFIED channel(s)
1) Do a test install, after I clean up a bit more
1) Publish gem
1) Abstract VolumeCtl::Mixer, so you can use mixers other than the default
1) Add some error-handling, such as trying to operate on a disconnected mixer
1) After abstracting for non-default mixers, add error-handling, in case of a failed connection

### INSTALLATION ###
