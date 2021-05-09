# lib/alsactl/mixer.rb

require_relative "../alsacore/alsacore"

##
# This extends the regular AlsaCore module and
# includes classes abstracted from the C code
# in the extension.
# 
# I realized that the vast majority of the actual
# operations should be performed in an abstracted
# class; and only core, concrete operations needed
# to be included in the C code portion.
module AlsaCtl
  ##
  # Finalized computation for a volume setting.
  # 
  # This is not intended to be used directly, but
  # it helps separate functionality.
  # 
  # Returns the volume limit as a +FixedNum+, used
  # as a +long+ in the C code portion of the extension.
  def self.compute_final(min, max, adjustment)
    case
    when adjustment > max
      max
    when adjustment < min
      min
    else
      adjustment
    end
  end # End finalized computation
  
  ##
  # Compute the volume to set a channel.
  # 
  # Makes use of +compute_final+, as well as a type for
  # the change. The type will be a symbol of any of the
  # following values:
  # 
  # 1. +:raise+, +:up+, or +:increase+ to raise the volume.
  # 2. +:lower+, +:down+, or +:decrease+ to lower the volume.
  # 3. Any other value (use +nil+) to set the volume to a number.
  def self.compute(min, max, current, perc_change, type_change)
    change = ((max * perc_change) / 100).round
    case type_change
    when :raise, :up, :increase
      self.compute_final(min, max, (current + change))
    when :lower, :down, :decrease
      self.compute_final(min, max, (current - change))
    else
      self.compute_final(min, max, change)
    end
  end # End abstracted compute method
  
  ##
  # This is a default mixer type that will handle normal
  # mixer capabilities. It will make use of the +BaseMixer+
  # setters and getters for the volume, handle general
  # muting operations, and store data that was previously
  # loaded into the original +VolumeCtl::Mixer+
  # (now +AlsaCore::BaseMixer+) class.
  class DefMixer < AlsaCore::BaseMixer
    ##
    # 1. List of channels for the mixer object
    # 2. List of volume states, used for muting and unmuting
    attr_reader :channels, :mute_state
    
    ##
    # The +DefMixer+ constructor takes no arguments.
    # 
    # It gets the pointers to the +mixer->handle+ and
    # +mixer->element+ objects in the wrapped +BaseMixer+
    # class in the C code, utilizing the protected init method.
    # 
    # It then clears up the instance variables.
    def initialize
      pro_initialize # Call the hidden, protected constructor
      reset # Reset/initialize instance variables
    end # end constructor
    
    ##
    # Reset the variables (in case reloading mixer).
    # 
    # +reload+ sets the +@mute_state+ and +@channels+
    # to +nil+, so the mixer does not try to access
    # invalid data. 
    def reset() @mute_state = @channels = nil end
    
    ##
    # Connect to the current mixer and enumerate
    # the channels. See: +enum+ for more information.
    def connect
      pro_connect("default", "Master") 
      enum
    end # End connect/enum
    
    ##
    # Disconnect from the mixer. You should call this,
    # when you are done using it, so that there's less of a
    # chance of errors with GC.
    def disconnect()
      pro_disconnect # Disconnect the mixer
      reset # Reset the instance variables
    end
    alias :close :disconnect # End disconnect method and aliases
    
    ##
    # Enumerate the channels.
    # 
    # Will return the channels from the underlying +BaseMixer+,
    # but it is unlikely that they need to be processed,
    # so the return value can usually be disregarded,
    # unless it is +nil+, which means it failed.
    def enum() @channels = pro_enum end
    
    ##
    # Get the volumes from a channel.
    # 
    # +c_vget+ takes as an argument a single number,
    # used to identify the channel the mixer should find
    # the volume for.
    # 
    # = Example
    #   
    #   [mixer].c_vget 0 -> 
    #       {:name=>"Front Left", :max=>65536, :min=>0, :volume=>32768, :percent=>50}
    def c_vget(channel) pro_cvolume_get(channel) end
    alias :c_volume :c_vget
    
    ##
    # Get all the volumes and return them as
    # a clean hash.
    # 
    # +vget+, alias +volume+, returns a has with ALL the
    # volumes by channel number/ID, listed with the following fields:
    # 
    # 1. +:name+ -- the name of the channel, such as "Front Left"
    # 2. +:max+ -- the maximum volume allowed for the channel (probably 65_536)
    # 3. +:min+ -- the minimum volumes allowed for the channel (usually 0)
    # 4. +:volume+ -- the current volume as a C +long+
    # 5. +:percent+ -- the current volume as a percent of max
    # 
    # This method takes no arguments and returns a hash.
    def vget
      volumes = Hash.new
      if @channels then # @channels is nil, if not enumerated and will raise errors
        @channels.each do |k,v|
          cv = c_vget(k) # Get volume for each channel
          volumes[k] = cv
        end # If no channels, print an error to STDERR
      else $stderr.puts "ERROR: No channels detected. Is mixer connected ('.connect')?" end
      volumes # Return volumes, unless
    end 
    alias :volume :vget # End getter of volumes
    
    ##
    # Set a single channel's volume.
    # 
    # Takes as args, +channel (Int)+, +perc_change (Int)+
    # (percent to change), and +type_change+ (the direction of the 
    # volume adjustment). +type_change+ is optional and defaults to +nil+.
    # 
    # Returns the value of +c_vget+ for the same channel.
    # 
    # = Example
    #   
    #   [mixer].c_vget 0, 10, :up -> 
    #       {:name=>"Front Left", :max=>65536, :min=>0, :volume=>32768, :percent=>50}
    #       
    # = Example
    #   
    #   [mixer].c_vget 0, 10 -> 
    #       {:name=>"Front Left", :max=>65536, :min=>0, :volume=>32768, :percent=>50}
    def c_vset(channel, perc_change, type_change=nil)
      curr = c_vget(channel) # curr must not be nil to continue
      if curr and (Integer === channel) and (Integer === perc_change) then
        adjustment = AlsaCtl.compute(curr[:min], curr[:max],
          curr[:volume], perc_change, type_change)
        pro_cvolume_set(channel, adjustment)
      else false end
    end # End individual channel volume setter
    
    ##
    # Set all the volumes.
    # 
    # This is a bypass method. ALSA offers the function,
    # +snd_mixer_selem_set_playback_volume_all+, which allows us to
    # shorthand +c_vset+ for every single channel in an iteration loop.
    # 
    # Takes only 2 arguments, +perc_change+, and +type_change+, of which
    # +type_change+ is also optional and defaults to +nil+.
    # 
    # Returns the same values as +volume+ / +vget+.
    # 
    # This method uses the 0, default channel, usually "Front Left", as
    # the basis from which to set the volume.
    # 
    # Aliased to +set_volume+, as well.
    # 
    # = Example
    #   
    #   [mixer].vset 100 -> Sets all channels to volume 100%
    #   
    # = Example
    #   
    #   [mixer].vset 40, :up -> Increases all channels by 40% (up to +:max+)
    def vset(perc_change, type_change=nil)
      curr = c_vget(0) # Get the default/lowest channel
      if curr and (Integer === perc_change) then
        adjustment = AlsaCtl.compute(curr[:min], curr[:max],
          curr[:volume], perc_change, type_change)
        pro_volume_set(adjustment)
      else false end
    end 
    alias :set_volume :vset # End setter for all volumes
    
    ##
    # Mute the mixer.
    # 
    # Stores the current values of each channel as a hash in
    # +@mute_state+. Then uses +vset+ to set all channels to 0%.
    # 
    # Returns true on success, or an error message from inside the C code.
    # 
    # = Example
    #   
    #   [mixer].mute -> true
    def mute
      @mute_state = {}
      volume.each do |idx, v|
        @mute_state[idx] = v[:percent]
      end
      vset(0, nil)
    end # End mute
    
    ##
    # Unmute the mixer.
    # 
    # Checks if +@mute_states+ is empty. If not, uses +c_vset+ to
    # return all channels to their previous state. Only works, if the
    # mixer was not closed, beforehand.
    # 
    # ALWAYS returns an empty hash, +{}+, so the result can be
    # ignored. It is just the output of +@mute_state.clear+.
    # 
    # = Example
    #   
    #   [mixer].unmute -> {}
    def unmute
      if !@mute_state.empty? then
        @mute_state.each do |idx, v|
          c_vset(idx, v, nil)
        end
      end
      @mute_state.clear
    end # End unmute
  end # End DefMixer class
end # End AlsaCtl module