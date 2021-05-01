/* NON-TESTING FILE */
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
#define MIXLOADER mixer_obj* mixer; \
TypedData_Get_Struct(self, mixer_obj, &mixer_type, mixer);

// Shorthand to grab the volumes and assign them for comparison
#define VOLLOADER VALUE c = method_mixer_volume(self); \
long max = NUM2LONG(rb_hash_aref(c, ID2SYM(rb_intern("max")))); \
long min = NUM2LONG(rb_hash_aref(c, ID2SYM(rb_intern("min")))); \
long curr = NUM2LONG(rb_hash_aref(c, ID2SYM(rb_intern("current")))); \
int curr_p = NUM2LONG(rb_hash_aref(c, ID2SYM(rb_intern("current_p"))));

// Shorthand for vout conversion
#define VOUT long vout = round((((long) volume) * max) / 100);

// Basic type initiators used in this file
VALUE VolumeCtl = Qnil;
VALUE Mixer = Qnil;
// End type initiators

// ---------------------------
// MIXER STRUCT BELOW
// ---------------------------

// Define the mixer struct
typedef struct {
  snd_mixer_t* handle; // Mixer handle
  snd_mixer_elem_t* element; // Mixer element
  int unmute_vol; // Volume, if mute turned off
  int channels[SND_MIXER_SCHN_LAST + 1]; // Array to store all channels
} mixer_obj; // End Mixer struct def

// ---------------------------
// MIXER METHODS BELOW
// ---------------------------

// Free method for mixer
void mixer_free(void* data) {
  free(data);
} // End mixer free

// Size of mixer
size_t mixer_size(const void* data) {
  return sizeof(data);
} // End size of mixer

// Define the Ruby struct type to encapsulate the
// master sound mixer
static const rb_data_type_t mixer_type = {
  .wrap_struct_name = "mixer_obj",
  .function = {
    .dmark = NULL,
    .dfree = mixer_free,
    .dsize = mixer_size,
  },
  .data = NULL,
  .flags = RUBY_TYPED_FREE_IMMEDIATELY,
}; // End mixer type definition

// Define allocation of mixer handle object
VALUE mixer_alloc(VALUE self) {
  // Allocate the size for a mixer handle and its opened element
  mixer_obj* mixer = malloc(sizeof(mixer_obj));
  return TypedData_Wrap_Struct(self, &mixer_type, mixer); // Wrap it up!
} // End mixer handle allocation

// Initializer method for mixer handle
VALUE mixer_m_initialize(VALUE self) {
  // The below are set on connect() and voided on disconnect()
  snd_mixer_t *handle = NULL; // NULL handle
  snd_mixer_elem_t* elem = NULL; // NULL element
  
  // Create a Mixer struct pointer to access
  // from the class
  mixer_obj mm = {
    .handle = handle,
    .element = elem,
    .unmute_vol = 0,
    .channels = { 0 },
  };
  
  MIXLOADER; // Shorthand load the mixer
  *mixer = mm; // Bind the struct to the class object's pointer
  return self;
} // End mix handle initializer

// Connect with the ALSA server
VALUE method_mixer_connect(int argc, VALUE *card_info, VALUE self) {
  // Scan for whether the Mixer was passed card data
  VALUE card_name, elem_name;
  rb_scan_args(argc, card_info, "02", &card_name, &elem_name);
  
  snd_mixer_selem_id_t *sid; // Create mixer element ID
  
  // Default mixer will be default/Master
  // I will abstract this, later, but using the Master
  // should be fine, for the time being, for most cases.
  const char *card = "default";
  const char *selem_name = "Master";
  
  MIXLOADER; // Shorthand load the mixer
  
  // Open and load a mixer object
  snd_mixer_open(&mixer->handle, 0);
  snd_mixer_attach(mixer->handle, card);
  snd_mixer_selem_register(mixer->handle, NULL, NULL);
  snd_mixer_load(mixer->handle);
  
  // Attach a mixer element to the handle
  snd_mixer_selem_id_alloca(&sid);
  snd_mixer_selem_id_set_index(sid, 0);
  snd_mixer_selem_id_set_name(sid, selem_name);
  
  // Instantiate the mixer element attached to the handle
  mixer->element = snd_mixer_find_selem(mixer->handle, sid);
  
  // Attach all the channels to the object
  for (int i = 0; i < (SND_MIXER_SCHN_LAST + 1); i++) {
    // Channels that exist have a 1. Channels that don't stay 0
    mixer->channels[i] = snd_mixer_selem_has_playback_channel(mixer->element, i);
  }
  
  return self;
} // End connect method

// Disconnect from the ALSA server
VALUE method_mixer_disconnect(VALUE self) {
  MIXLOADER; // Shorthand load the mixer
  snd_mixer_close(mixer->handle); // Close the mixer
  return Qtrue;
} // End disconnect method

// Enumerate the channels
VALUE method_mixer_enum_channels(VALUE self) {
  MIXLOADER; // Shorthand load the mixer
  VALUE channels = rb_hash_new(); // Hash of channels
  for (int i = 0; i < (sizeof(mixer->channels) / sizeof(int)); i++) {
    // Check if the channel exists; if so, give it a true key.
    // Ignore any channels not present -- a bad index returns NIL, anyway
    if (mixer->channels[i] == 1) { rb_hash_aset(channels, INT2NUM(i), Qtrue); }
  }
  return channels;
} // End channel enumerator

// Get the volume of the mixer
VALUE method_mixer_volume(VALUE self) {
  long min, max, current; // Volume values
  MIXLOADER; // Shorthand load the mixer
  snd_mixer_handle_events(mixer->handle); // Refresh events on handle
  snd_mixer_selem_get_playback_volume_range(mixer->element, &min, &max); // Get  volume range
  // Get the default playback volume channel
  snd_mixer_selem_get_playback_volume(mixer->element, SND_MIXER_SCHN_MONO, &current);
  
  // Get the percentage of the total (round to 0 decimals,
  // for reasons logical to audio use)
  int perc = (int) round(( ((double) current / (double) max) ) * 100);
  
  // Initialize the unmute volume, if it has not yet been set
  if (mixer->unmute_vol < 1) {
    mixer->unmute_vol = (long) ((double) max / (double) 2);
  } // Default unmute is 50%
  
  // Create a volume hash object -- will add the other channels, after
  VALUE volumes = rb_hash_new();
  rb_hash_aset(volumes, rb_id2sym(rb_intern("max")), LONG2NUM(max));
  rb_hash_aset(volumes, rb_id2sym(rb_intern("min")), LONG2NUM(min));
  rb_hash_aset(volumes, rb_id2sym(rb_intern("current")), LONG2NUM(current));
  rb_hash_aset(volumes, rb_id2sym(rb_intern("current_p")), INT2NUM(perc));
  
  return volumes;
} // End volume getter for mixer

// Get a specific channel's volume
VALUE method_mixer_volume_channel(VALUE self, VALUE cid) {
  long min, max, current; // Volume values
  MIXLOADER; // Shorthand load the mixer
  snd_mixer_handle_events(mixer->handle); // Refresh events on handle
  snd_mixer_selem_get_playback_volume_range(mixer->element, &min, &max); // Get  volume range
  
  // Get the volume for the channel, but only if cid (channel id) is valid
  if (RB_TYPE_P(cid, T_FIXNUM)){
    int cid_fix = NUM2INT(cid);
    if (mixer->channels[cid_fix] == 1) {
      snd_mixer_selem_get_playback_volume(mixer->element, cid_fix, &current);
      VALUE channel = rb_hash_new(); // Channel has id and percent
      rb_hash_aset(channel, rb_id2sym(rb_intern("volume")), LONG2NUM(current));
      // Get the percentage of the total (round to 0 decimals,
      // for reasons logical to audio use)
      int perc = (int) round(( ((double) current / (double) max) ) * 100);
      rb_hash_aset(channel, rb_id2sym(rb_intern("percent")), INT2NUM(perc));
      return channel;
    } else { return Qfalse; } // False if channel doesn't exist
  } else { return Qnil; } // Nil if channel id is not a number
} // End getter for single-channel volume

// Get the volume by channel of the mixer
VALUE method_mixer_volume_all(VALUE self) {
  long min, max, current; // Volume values
  MIXLOADER; // Shorthand load the mixer
  // Create an array of longs for each channel
  int len = (sizeof(mixer->channels) / sizeof(int));
  long c_volumes[len]; // Length of the channel array
  snd_mixer_handle_events(mixer->handle); // Refresh events on handle
  snd_mixer_selem_get_playback_volume_range(mixer->element, &min, &max); // Get  volume range
  // Create a volume hash object -- will add each channel in the subsequent
  // iterator loop.
  VALUE volumes = rb_hash_new();
  // Iterate through all the channels
  for (int i = 0; i < (len); i++) {
    if (mixer->channels[i] == 1) { // Only set channels that exist on this device
      snd_mixer_selem_get_playback_volume(mixer->element, i, &current);
      VALUE channel = rb_hash_new(); // Channel has id and percent
      rb_hash_aset(channel, rb_id2sym(rb_intern("volume")), LONG2NUM(current));
      // Get the percentage of the total (round to 0 decimals,
      // for reasons logical to audio use)
      int perc = (int) round(( ((double) current / (double) max) ) * 100);
      rb_hash_aset(channel, rb_id2sym(rb_intern("percent")), INT2NUM(perc));
      // Attach the channel to the output hash
      rb_hash_aset(volumes, INT2NUM(i), channel);
    } // End setting of only valid channels
  } // End iteration over channels
  return volumes;
} // End volume-by-channel getter for mixer

// ---------------------------
// MIXER PRIVATE METHODS BELOW
// ---------------------------

// Raise the volume by percent or up to max
VALUE method_mixer_volume_up(VALUE self, VALUE volume) {
  MIXLOADER; // Shorthand load the mixer
  VOLLOADER; // Shorthand load the volumes
  VOUT; // Shorthand adjust volume
  int mod_v = curr + vout; // Add value to current
  int new_v = (int) (mod_v > max ? max : mod_v); // Can't be greater than max
  
  // Set the volume
  snd_mixer_selem_set_playback_volume_all(mixer->element, new_v);
  return method_mixer_volume(self);
} // End volume raise

// Lower the volume by percent or down to min
VALUE method_mixer_volume_down(VALUE self, VALUE volume) {
  MIXLOADER; // Shorthand load the mixer
  VOLLOADER; // Shorthand load the volumes
  VOUT; // Shorthand adjust volume
  int mod_v = curr - vout; // Subtract value from current
  int new_v = (int) (mod_v < min ? min : mod_v); // Can't be lower than min
  
  // Set the volume
  snd_mixer_selem_set_playback_volume_all(mixer->element, new_v);
  return method_mixer_volume(self);
} // End volume lower

// Set volume to specified value
VALUE method_mixer_volume_value(VALUE self, VALUE volume) {
  MIXLOADER; // Shorthand load the mixer
  VOLLOADER; // Shorthand load the volumes
  VOUT; // Shorthand adjust volume
  long new_v; // Init new_v for multiple assignment possibility
  
  // Assign new_v to the matching block
  if (vout > max) { new_v = max; } // Too high
  else if (vout < min) { new_v = min; } // Too low
  else { new_v = vout; } // new_v is itself
  
  // Set the volume
  snd_mixer_selem_set_playback_volume_all(mixer->element, new_v);
  return method_mixer_volume(self);
} // End set volume to specified

// ---------------------------
// END MIXER PRIVATE METHODS
// ---------------------------

/* Abstract volume setter method -- calls volume_up,
 * volume_down, or volume_value.
 * 
 * -1 is for down, 0 is for value, 1 is for up
 */
VALUE method_mixer_set_volume_abstract(VALUE self, VALUE magnitude, VALUE adjustment) {
  VALUE adj_t = TYPE(adjustment); // Get the type of the adjustment
  // If type is correct AND magnitude is a number
  if (RB_TYPE_P(magnitude, T_FIXNUM) && adj_t == T_FIXNUM) {
    int adj_fix = NUM2INT(adjustment); // If number, cast to int
    int mag_fix = (int) abs (NUM2INT(magnitude)); // Cast the magnitude to int and take absolute value
    if (adj_fix > 0) {
      return method_mixer_volume_up(self, mag_fix);
    } else if (adj_fix < 0) {
      return method_mixer_volume_down(self, mag_fix);
    } else { return method_mixer_volume_value(self, mag_fix); }
    // Otherwise, the wrong types were passed
  } else {
    puts("'set_volume' ERROR: magnitude should be either FIXNUM/INT and");
    puts("  adjustment should be -1, 0, or 1 -- ");
    return Qnil;
  }
} // End abstract volume setter

// Mute the default sink volume
VALUE method_mixer_mute(VALUE self) {
  MIXLOADER; // Shorthand load the mixer
  // Get the current volume
  long current = NUM2LONG(rb_hash_aref(method_mixer_volume(self), ID2SYM(rb_intern("current"))));
  mixer->unmute_vol = current; // Set the current as the unmute
  snd_mixer_selem_set_playback_volume_all(mixer->element, 0); // Set everything to 0
  return LONG2NUM(mixer->unmute_vol);
} // End mute

// Unmute the mixer volume
VALUE method_mixer_unmute(VALUE self) {
  MIXLOADER; // Shorthand load the mixer
  method_mixer_volume(self); // Ensure default unmute_vol is set
  snd_mixer_selem_set_playback_volume_all(mixer->element, mixer->unmute_vol);
  return LONG2NUM(mixer->unmute_vol);
} // End unmute

// Initialize the VolumeCtl module
void Init_volumectl() {
  VolumeCtl = rb_define_module("VolumeCtl"); // Module
  
  // Mixer class
  Mixer = rb_define_class_under(VolumeCtl, "Mixer", rb_cData);
  rb_define_alloc_func(Mixer, mixer_alloc);
  rb_define_method(Mixer, "initialize", mixer_m_initialize, 0);
  rb_define_method(Mixer, "connect", method_mixer_connect, -1);
  rb_define_method(Mixer, "disconnect", method_mixer_disconnect, 0);
  rb_define_method(Mixer, "close", method_mixer_disconnect, 0);
  
  // Information methods
  rb_define_method(Mixer, "enum", method_mixer_enum_channels, 0);
  
  // Readers
  rb_define_method(Mixer, "volume", method_mixer_volume, 0);
  rb_define_method(Mixer, "volume_c", method_mixer_volume_channel, 1);
  rb_define_method(Mixer, "volume_a", method_mixer_volume_all, 0);
  
  // Manipulators
  rb_define_method(Mixer, "mute", method_mixer_mute, 0);
  rb_define_method(Mixer, "unmute", method_mixer_unmute, 0);
  rb_define_method(Mixer, "set_volume", method_mixer_set_volume_abstract, 2);
  
  // Private volume setters
  rb_define_private_method(Mixer, "pri_volume_raise", method_mixer_volume_up, 1);
  rb_define_private_method(Mixer, "pri_volume_lower", method_mixer_volume_down, 1);
  rb_define_private_method(Mixer, "pri_volume_value", method_mixer_volume_value, 1);
} // End init