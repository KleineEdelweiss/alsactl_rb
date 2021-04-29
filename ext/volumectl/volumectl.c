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

// Basic type initiators used in this file
VALUE VolumeCtl = Qnil;
VALUE Mixer = Qnil;
// End type initiators

// ---------------------------
// MIXER METHODS BELOW
// ---------------------------

// Define the mixer struct
typedef struct {
  snd_mixer_t* handle; // Mixer handle
  snd_mixer_elem_t* element; // Mixer element
  int unmute_vol; // Volume, if mute turned off
  
  int id; // This is really just a test field...
} mixer_obj; // End Mixer struct def

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
    .id = -999,
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
  
  return self;
} // End connect method

// Disconnect from the ALSA server
VALUE method_mixer_disconnect(VALUE self) {
  return Qnil;
} // End disconnect method

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
  
  // Initialize the unmute volume, if it has not
  // yet been set
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

/* Set the volume to a specified state for the
 * current default sink.
 * 
 * This private method will NOT set the volume for
 * separate channels to different numbers, but will,
 * instead, modify the volumes of ALL channels.
 * 
 * A separate method might be added to do the above.
 * 
 * This method will either modify the volume using a
 */
VALUE method_volume_mod(VALUE self, VALUE volume) {
  return volume;
} // End modify volume

/* Check to see if the volume is of an acceptable type,
 * and then set it to the user's defined volume. This will
 * call 'pmod_dvolume', which will change the volume up, down,
 * mute, or to a specific number.
 */
VALUE method_set_mixer_volume(int argc, VALUE* volmodv, VALUE self) {
  VALUE magnitude, adjustment;
  
  /* Scan in the arguments for the volume, &volume,
   * and the modification of the volume change, &adjustment.
   * 
   * Adjustment should be:
   * * (nothing) -- set the volume to a specific magnitude
   * * the character, '+' -- raise the volume by that magnitude
   * * 
   */
  rb_scan_args(argc, volmodv, "11", &magnitude, &adjustment);
  
  if (RB_TYPE_P(magnitude, T_FIXNUM)) {
    return magnitude;
  } else {
    puts("'set_volume' ERROR: volmod should be either FIXNUM/INT or");
    puts("  FIXNUM/INT and one of the characters, '+' or '-'");
    return Qnil;
  }
  return Qnil;
} // End set_volume

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

// ---------------------------
// MIXER TEST METHODS BELOW
// ---------------------------

// Test Mixer
VALUE method_mixer_test(VALUE self) {
  MIXLOADER; // Shorthand load the mixer
  return LONG2FIX(mixer->id);
} // End test of mixer

// Test Mixer set
VALUE method_mixer_test_set(VALUE self, VALUE new_id) {
  MIXLOADER; // Shorthand load the mixer
  mixer->id = NUM2INT(new_id); // Test setting the ID
  return LONG2FIX(mixer->id);
} // End set test of mixer

// ---------------------------
// END MIXER TEST METHODS
// ---------------------------

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
  
  // Volume controls
  rb_define_method(Mixer, "volume", method_mixer_volume, 0);
  rb_define_method(Mixer, "mute", method_mixer_mute, 0);
  rb_define_method(Mixer, "unmute", method_mixer_unmute, 0);
  
  // Test funcs
  rb_define_method(Mixer, "test", method_mixer_test, 0);
  rb_define_method(Mixer, "test_s", method_mixer_test_set, 1);
} // End init

/*
// Test for a connection
VALUE method_test(VALUE self) {
  long min, max, volume;
  int channel;
  snd_mixer_t *handle = NULL;
  snd_mixer_selem_id_t *sid;
  const char *card = "default";
  const char *selem_name = "Master";
  
  snd_mixer_open(&handle, 0);
  snd_mixer_attach(handle, card);
  snd_mixer_selem_register(handle, NULL, NULL);
  snd_mixer_load(handle);
  
  snd_mixer_selem_id_alloca(&sid);
  snd_mixer_selem_id_set_index(sid, 0);
  snd_mixer_selem_id_set_name(sid, selem_name);
  
  snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
  snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
  //snd_mixer_selem_set_playback_volume_all(elem, 32768);
  
  channel = -1;
  volume = -999;
  
  snd_mixer_handle_events(handle);
  snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_REAR_CENTER, &volume);
  printf("Volume on BS channel: %d\n", volume);
  
  volume = -999;
  snd_mixer_handle_events(handle);
  snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_WOOFER, &volume);
  printf("Volume on woofer channel: %d\n", volume);
  
  volume = -999;
  snd_mixer_handle_events(handle);
  snd_mixer_selem_get_playback_volume(elem, 5, &volume);
  printf("ALSO volume on woofer channel: %d\n", volume);
  
  snd_mixer_close(handle);
  
  return LONG2FIX(volume);
}*/