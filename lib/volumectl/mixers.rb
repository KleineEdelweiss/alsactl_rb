# lib/volumectl/mixer.rb

require_relative "../mixcore/mixcore"

# This extends the regular VolumeCtl module and
# includes classes abstracted from the C code
# in the extension.
# 
# I realized that the vast majority of the actual
# operations should be performed in an abstracted
# class; and only core, concrete operations needed
# to be included in the C code portion.
module Mixers
  # Finalized computation
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
  
  # Compute the volume to set a channel
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
  
  # This is a default mixer type that will handle normal
  # mixer capabilities. It will make use of the BaseMixer
  # setters and getters for the volume, handle general
  # muting operations, and store data that was previously
  # loaded into the original VolumeCtl::Mixer
  # (now MixCore::BaseMixer) class.
  class DefMixer < MixCore::BaseMixer
    attr_reader :channels, :mute_state
    
    # Constructor
    def initialize
      pro_initialize # Call the hidden, protected constructor
      reset # Reset/initialize instance variables
    end # end constructor
    
    # Reset the variables (in case reloading mixer)
    def reset() @mute_state = @channels = nil end
    
    # Connect to the current default mixer
    # Enumerate the channels, right after
    def connect
      pro_connect("default", "Master") 
      enum
    end # End connect/enum
    
    # Disconnect method
    def disconnect()
      pro_disconnect # Disconnect the mixer
      reset # Reset the instance variables
    end
    alias :close :disconnect
    
    # Enumerate the channels
    def enum() @channels = pro_enum end
    
    # Get the volumes from a channel
    def c_vget(channel) pro_cvolume_get(channel) end
    alias :c_volume :c_vget
    
    # Get all the volumes and return them as
    # a clean hash
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
    
    # Set a single channel's volume
    def c_vset(channel, perc_change, type_change=nil)
      curr = c_vget(channel) # curr must not be nil to continue
      if curr and (Integer === channel) and (Integer === perc_change) then
        adjustment = Mixers.compute(curr[:min], curr[:max],
          curr[:volume], perc_change, type_change)
        pro_cvolume_set(channel, adjustment)
      else false end
    end # End individual channel volume setter
    
    # Set all the volumes
    def vset(perc_change, type_change=nil)
      curr = c_vget(0) # Get the default/lowest channel
      if curr and (Integer === perc_change) then
        adjustment = Mixers.compute(curr[:min], curr[:max],
          curr[:volume], perc_change, type_change)
        pro_volume_set(adjustment)
      else false end
    end 
    alias :set_volume :vset # End setter for all volumes
    
    # Mute the mixer
    def mute
      @mute_state = {}
      volume.each do |idx, v|
        @mute_state[idx] = v[:percent]
      end
      vset(0, nil)
    end # End mute
    
    # Unmute the mixer
    def unmute
      if !@mute_state.empty? then
        @mute_state.each do |idx, v|
          c_vset(idx, v, nil)
        end
      end
      @mute_state.clear
    end # End unmute
  end # End DefMixer class
end # End Mixers modules