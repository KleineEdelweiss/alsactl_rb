/* IN-PROCESS FILE - 2021-May-09 */
// Include Ruby
#include <ruby.h>

// Include other libraries
#include <stdio.h>
#include <math.h>

// ALSA includes
#include <alsa/asoundlib.h>
#include <alsa/version.h>

// Expansion to create a mixer pointer and then get the
// data from it
#define MIXLOADER base_mixer_obj* mixer; TypedData_Get_Struct(self, base_mixer_obj, &base_mixer_type, mixer);
  
// Don't try to handle anything on NULL mixer
#define CHK_MIX if (mixer->handle == NULL || \
  mixer->element == NULL) { return Qnil; }

// Shorthand to allocate the channel arrays
#define MAX_CHANNELS (SND_MIXER_SCHN_LAST + 1)

// Shorthand early return on error
#define CRIT_CHECK if (err != 0) { return INT2NUM(err); }

/* 
 * Document-module: AlsaCore
 * 
 * This module provides the concrete methods from which
 * to extend custom mixers for volume control.
 * 
 * A simple example of a default/Master mixer object is
 * available and usable as +AlsaCtl::DefMixer+.
 * 
 * = Example
 *   
 *   require 'AlsaCore'
 *   
 *   class MyMixer < AlsaCore::BaseMixer
 */

// Basic type initiators used in this file
VALUE AlsaCore = Qnil;
VALUE BaseMixer = Qnil;
// End type initiators

/* 
 * Document-class: AlsaCore::BaseMixer
 * 
 * The BaseMixer class provides purely protected methods
 * that can be used from an inheriting class. +AlsaCtl::DefMixer+
 * inherits this class and provides a simple interface without
 * complex mixer management operations.
 * 
 * BaseMixer provides methods for 
 * 1. Resource management
 * 2. Connection
 * 3. Disconnection
 * 4. Setting ALL the volumes of a mixer
 * 5. Per-channel volume setting of the mixer
 * 6. Per-channel retrieval of mixer volume information
 * 7. Enumeration of channels within the mixer
 * 8. Key error handling for common operations
 * 
 * The API can be found on https://www.alsa-project.org/alsa-doc/alsa-lib/
 * 
 *---
 * Document-method: pro_initialize
 *   
 *   (base_mixer_m_initialize)
 *   
 *   Protected method initializes the BaseMixer class.
 * 
 *   Instantiates the wrapped struct. Does not take any
 *   arguments.
 * 
 *   self.new -> BaseMixer
 *   self.initialize -> BaseMixer
 * 
 *---
 * Document-method: pro_connect
 *   
 *   (method_base_mixer_connect)
 *   
 *   Connects to a requested ALSA mixer.
 * 
 *   Takes 2 strings, a card name and element name (identifier).
 *   +DefMixer+ automatically passes the values "default" and "Master"
 *   to generate the default mixer element for the system.
 *   
 *   self.connect(char*, char*) -> BaseMixer | error
 *   
 *   pro_connect("default", "Master") -> new BaseMixer
 * 
 *---
 * Document-method: pro_close
 * 
 *   (method_base_mixer_disconnect)
 *   
 *   Disconnects the mixer and sets its struct member pointers
 *   to NULL.
 * 
 *   Takes no arguments.
 * 
 *   Returns true, if no error.
 * 
 *---
 * Document-method: pro_enum
 * 
 *   (method_base_mixer_enum_channels)
 *   
 *   Returns a hash of valid channels for the mixer, on success.
 *   On failure, returns +nil+.
 * 
 *   Takes no arguments.
 * 
 *---
 * Document-method: pro_cvolume_get
 * 
 *   (method_base_mixer_cvolume_get)
 *   Gets the volume settings for a selected channel on the mixer.
 * 
 *   Returns a hash with the volume, on success.
 * 
 *   On failure, returns either:
 *   +nil+, if the mixer is disconnected
 *   +false+, if the mixer doesn't have this channel
 * 
 *   Takes 1 argument, the channel number to check.
 *   
 *   pro_cvolume_get(1) -> {:name, :max, :min, :volume, :percent}
 * 
 *---
 * Document-method: pro_cvolume_set
 * 
 *   (method_base_mixer_cvolume_set)
 *   
 *   Sets the volume for a specific channel.
 * 
 *   Takes 2 arguments, the channel number (+int+) and the volume to use (+long+).
 *   When inheriting this class, you need to convert the values correctly,
 *   as 1 will not be 1%, but more like 0.1%, if your mixer uses 2^16 or something.
 * 
 *   Returns the output of method_base_mixer_cvolume_get for the same channel,
 *   on success.
 * 
 *   Returns +nil+, on failure.
 *   
 *   pro_cvolume_set(1, 32768) -> Sets channel 1 to 32768 (usually 50%)
 * 
 *---
 * Document-method: pro_volume_set
 * 
 *   (method_base_mixer_volume_set_all)
 *   
 *   Sets the volumes for the entire mixer, all channels. This method
 *   uses a separate call from ALSA, +snd_mixer_selem_set_playback_volume_all+,
 *   which does not require any channels.
 * 
 *   Takes 1 argument, the volume (+long+).
 *   
 *   pro_volume_set(32768) -> Sets all channels to 32768 (probably 50%)
 */

// ---------------------------
// MIXER STRUCT BELOW
// ---------------------------

// Define the base mixer struct
typedef struct {
  snd_mixer_t* handle; // Mixer handle
  snd_mixer_elem_t* element; // Mixer element
} base_mixer_obj; // End Mixer struct def

// ---------------------------
// MIXER METHODS BELOW
// ---------------------------

// Free method for base mixer
void base_mixer_free(void* data) {
  free(data);
} // End mixer free

// Size of mixer
size_t base_mixer_size(const void* data) {
  return sizeof(data);
} // End size of mixer

// Define the Ruby struct type to encapsulate the
// master sound mixer
static const rb_data_type_t base_mixer_type = {
  .wrap_struct_name = "base_mixer_obj",
  .function = {
    .dmark = NULL,
    .dfree = base_mixer_free,
    .dsize = base_mixer_size,
  },
  .data = NULL,
  .flags = RUBY_TYPED_FREE_IMMEDIATELY,
}; // End base mixer type definition

// Define allocation of mixer handle object
VALUE base_mixer_alloc(VALUE self) {
  // Allocate the size for a mixer handle and its opened element
  base_mixer_obj* mixer = malloc(sizeof(base_mixer_obj));
  return TypedData_Wrap_Struct(self, &base_mixer_type, mixer); // Wrap it up!
} // End mixer handle allocation

// Initialize / constructor
VALUE base_mixer_m_initialize(VALUE self) {
  // The below are set on connect() and voided on disconnect()
  snd_mixer_t *handle = NULL; // NULL handle
  snd_mixer_elem_t* elem = NULL; // NULL element
  
  // Create a Base Mixer struct pointer to access
  // from the class
  base_mixer_obj mm = {
    .handle = handle,
    .element = elem,
  };
  
  MIXLOADER; // Shorthand load the mixer
  *mixer = mm; // Bind the struct to the class object's pointer
  return self;
} // End mix handle initializer

// Connect with the ALSA server
VALUE method_base_mixer_connect(VALUE self, VALUE card_name, VALUE elem_name) {
  int err = 0; // Placeholder for errors
  snd_mixer_selem_id_t *sid; // Create mixer element ID
  
  // Default mixer will be default/Master
  const char *card = StringValueCStr(card_name); // ex: "default"
  const char *selem_name = StringValueCStr(elem_name); // ex: "Master"
  
  MIXLOADER; // Shorthand load the mixer
  // If the mixer is already connected, raise an error
  if (mixer->handle != NULL ||  mixer->element != NULL) {
    rb_raise(rb_eRuntimeError,
      "\n::ERROR: Already connected to mixer! Disconnect first!::\n");
    return Qnil;
  }
  
  // The below will use CRIT_CHECK, as these errors are
  // non-recoverable, and continuation in other methods
  // will result in seg faults.
  // 
  // Open and load a mixer object
  err = snd_mixer_open(&mixer->handle, 0);
  CRIT_CHECK;
  err = snd_mixer_attach(mixer->handle, card);
  CRIT_CHECK;
  err = snd_mixer_selem_register(mixer->handle, NULL, NULL);
  CRIT_CHECK;
  err = snd_mixer_load(mixer->handle);
  CRIT_CHECK;
  
  // Attach a mixer element to the handle
  snd_mixer_selem_id_alloca(&sid);
  snd_mixer_selem_id_set_index(sid, 0);
  snd_mixer_selem_id_set_name(sid, selem_name);
  
  // Instantiate the mixer element attached to the handle
  mixer->element = snd_mixer_find_selem(mixer->handle, sid);
  
  // Checker for valid channels moved to Ruby side
  return self;
} // End connect method

// Disconnect from the ALSA server
VALUE method_base_mixer_disconnect(VALUE self) {
  MIXLOADER; // Shorthand load the mixer
  if (mixer->handle != NULL) {
    int err = snd_mixer_close(mixer->handle); // Close the mixer
    CRIT_CHECK;
    mixer->element = NULL;
    mixer->handle = NULL;
  } // If it's already NULL, there is nothing to close,
  // So returning TRUE is still saying the mixer is disconnected
  return Qtrue;
} // End disconnect method

// Enumerate the channels
VALUE method_base_mixer_enum_channels(VALUE self) {
  MIXLOADER; // Shorthand load the mixer
  CHK_MIX; // Check if mixer is null or not, before attempting operations
  VALUE channels = rb_hash_new(); // Hash of channels
  for (int i = 0; i < (MAX_CHANNELS); i++) {
    // Check if the channel exists; if so, give it a true key.
    // Ignore any channels not present -- a bad index returns NIL, anyway
    if (snd_mixer_selem_has_playback_channel(mixer->element, i)) {
      rb_hash_aset(channels, INT2NUM(i), Qtrue);
    }
  }
  return channels;
} // End channel enumerator

// Get the volume for a single channel
VALUE method_base_mixer_cvolume_get(VALUE self, VALUE cid) {
  long min, max, current; // Volume values
  MIXLOADER; // Shorthand load the mixer
  CHK_MIX; // Check if mixer is null or not, before attempting operations
  snd_mixer_handle_events(mixer->handle); // Refresh events on handle
  snd_mixer_selem_get_playback_volume_range(mixer->element, &min, &max); // Get  volume range
  
  // Get the volume for the channel, but only if cid (channel id) is valid
  if (RB_TYPE_P(cid, T_FIXNUM)){
    int cid_fix = NUM2INT(cid);
    if (snd_mixer_selem_has_playback_channel(mixer->element, cid_fix)) {
      snd_mixer_selem_get_playback_volume(mixer->element, cid_fix, &current);
      VALUE channel = rb_hash_new(); // Channel has volume and percent
      // Name of channel, if available
      rb_hash_aset(channel, rb_id2sym(rb_intern("name")),
        rb_str_new2(snd_mixer_selem_channel_name(cid_fix)));
      // Minimum and maximum values
      rb_hash_aset(channel, rb_id2sym(rb_intern("max")), LONG2NUM(max));
      rb_hash_aset(channel, rb_id2sym(rb_intern("min")), LONG2NUM(min));
      // Current channel volume
      rb_hash_aset(channel, rb_id2sym(rb_intern("volume")), LONG2NUM(current));
      // Get the percentage of the total (round to 0 decimals,
      // for reasons logical to audio use)
      int perc = (int) round(( ((double) current / (double) max) ) * 100);
      rb_hash_aset(channel, rb_id2sym(rb_intern("percent")), INT2NUM(perc));
      return channel;
    } else { return Qfalse; } // False if channel doesn't exist
  } else { return Qnil; } // Nil if channel id is not a number
} // End getter for single-channel volume

// Set volume on a single channel
VALUE method_base_mixer_cvolume_set(VALUE self, VALUE channel, VALUE volume) {
  MIXLOADER; // Shorthand load the mixer
  CHK_MIX; // Check if mixer is null or not, before attempting operations
  int ch = NUM2INT(channel); // Cast the channel to an INT
  // Set the volume for one channel
  snd_mixer_selem_set_playback_volume(mixer->element, ch, NUM2LONG(volume));
  return method_base_mixer_cvolume_get(self, channel);
} // End set channel volume to specified

// Set ALL the channels' volumes on the mixer
VALUE method_base_mixer_volume_set_all(VALUE self, VALUE volume) {
  MIXLOADER; // Shorthand load the mixer
  CHK_MIX; // Check if mixer is null or not, before attempting operations
  // Set the volume for all channels
  int err = snd_mixer_selem_set_playback_volume_all(mixer->element, NUM2LONG(volume));
  return (err ? Qfalse : Qtrue);
} // End set volume to specified

// Initialize the AlsaCore module
void Init_alsacore() {
  AlsaCore = rb_define_module("AlsaCore"); // Module
  
  // Mixer class
  BaseMixer = rb_define_class_under(AlsaCore, "BaseMixer", rb_cData);
  rb_define_alloc_func(BaseMixer, base_mixer_alloc);
  rb_define_protected_method(BaseMixer, "pro_initialize", base_mixer_m_initialize, 0);
  
  // Connection methods
  rb_define_protected_method(BaseMixer, "pro_connect", method_base_mixer_connect, 2);
  rb_define_protected_method(BaseMixer, "pro_disconnect", method_base_mixer_disconnect, 0);
  rb_define_protected_method(BaseMixer, "pro_close", method_base_mixer_disconnect, 0);
  
  // Enumerate the channels
  rb_define_protected_method(BaseMixer, "pro_enum", method_base_mixer_enum_channels, 0);
  
  // Get a channel's volume
  rb_define_protected_method(BaseMixer, "pro_cvolume_get", method_base_mixer_cvolume_get, 1);
  
  // Setters
  rb_define_protected_method(BaseMixer, "pro_volume_set", method_base_mixer_volume_set_all, 1);
  rb_define_protected_method(BaseMixer, "pro_cvolume_set", method_base_mixer_cvolume_set, 2);
} // End init