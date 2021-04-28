#include <ruby.h>
#include <alsa/asoundlib.h>
#include <alsa/version.h>
#include <stdio.h>

VALUE VolumeCtl = Qnil;
VALUE MasterMixer = Qnil;

// ---------------------------
// MASTER MIXER METHODS BELOW
// ---------------------------

// Define the master mixer struct
typedef struct {
  snd_mixer_t* handle;
  snd_mixer_elem_t* element;
  int id;
} master_mixer;

// Free method for master mixer
void master_mixer_free(void* data) {
  free(data);
} // End master mixer free

// Size of master mixer
size_t master_mixer_size(const void* data) {
  return sizeof(data);
} // End size of master mixer

// Define the Ruby struct type to encapsulate the
// master sound mixer
static const rb_data_type_t master_mixer_type = {
  .wrap_struct_name = "master_mixer",
  .function = {
    .dmark = NULL,
    .dfree = master_mixer_free,
    .dsize = master_mixer_size,
  },
  .data = NULL,
  .flags = RUBY_TYPED_FREE_IMMEDIATELY,
}; // End mixer handle type definition

// Define allocation of mixer handle object
VALUE master_mixer_alloc(VALUE self) {
  // Allocate the size for a mixer handle and its opened element
  master_mixer* mixer = malloc(sizeof(master_mixer));
  // Wrap it up!
  return TypedData_Wrap_Struct(self, &master_mixer_type, mixer);
} // End mixer handle allocation

// Initializer method for mixer handle
VALUE master_mixer_m_initialize(VALUE self) {
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
  
  master_mixer mm = {
    .handle = handle,
    .element = elem,
    .id = -999,
  };
  
  master_mixer* mixer;
  // Unwrap
  TypedData_Get_Struct(self, master_mixer, &master_mixer_type, mixer);
  //mixer = (master_mixer *)malloc(sizeof(mm)); // Attach the mixer
  *mixer = mm;
  return self;
} // End mix handle initializer

// Get the volume of the master mixer
VALUE method_master_mixer_volume(VALUE self) {
  long min, max, current;
  master_mixer* mixer;
  // Unwrap
  TypedData_Get_Struct(self, master_mixer, &master_mixer_type, mixer);
  
  snd_mixer_handle_events(mixer->handle);
  snd_mixer_selem_get_playback_volume_range(mixer->element, &min, &max);
  snd_mixer_selem_get_playback_volume(mixer->element, SND_MIXER_SCHN_MONO, &current);
  
  VALUE volumes = rb_hash_new();
  rb_hash_aset(volumes, rb_id2sym(rb_intern("max")), LONG2NUM(max));
  rb_hash_aset(volumes, rb_id2sym(rb_intern("min")), LONG2NUM(min));
  rb_hash_aset(volumes, rb_id2sym(rb_intern("current")), LONG2NUM(current));
  
  return volumes;
} // End volume getter for master mixer

// Test MasterMixer
VALUE method_master_mixer_test(VALUE self) {
  master_mixer* mixer;
  // Unwrap
  TypedData_Get_Struct(self, master_mixer, &master_mixer_type, mixer);
  //mixer = (master_mixer *)malloc(sizeof(mm)); // Attach the mixer
  //snd_mixer_selem_get_playback_volume_range(self.element, &min, &max);
  return LONG2FIX(mixer->id);
} // End test of master mixer

// Test MasterMixer set
VALUE method_master_mixer_test_set(VALUE self, VALUE new_id) {
  master_mixer* mixer;
  // Unwrap
  TypedData_Get_Struct(self, master_mixer, &master_mixer_type, mixer);
  mixer->id = NUM2INT(new_id);
  return LONG2FIX(mixer->id);
} // End test of master mixer

/* Get the current default sink for ALSA,
 * for use with the below methods.
 */
VALUE method_get_default_sink(VALUE self) {
  return Qnil;
} // End get_dsink

/* Get the volume from the current default sink
 * in ALSA.
 */
VALUE method_get_default_volume(VALUE self) {
  return Qnil;
} // End get_dvolume

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
} // End 

/* Check to see if the volume is of an acceptable type,
 * and then set it to the user's defined volume. This will
 * call 'pmod_dvolume', which will change the volume up, down,
 * mute, or to a specific number.
 */
VALUE method_set_default_volume(int argc, VALUE* volmodv, VALUE self) {
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
    puts("'set_dvolume' ERROR: volmod should be either FIXNUM/INT or");
    puts("  FIXNUM/INT and one of the characters, '+' or '-'");
    return Qnil;
  }
  return Qnil;
} // End set_dvolume

// Mute the default sink volume
VALUE method_mute_default(VALUE self) {
  return Qnil;
} // End dmute

// Unmute the default sink volume
VALUE method_unmute_default(VALUE self) {
  return Qnil;
} // End dunmute

// Initialize the VolumeCtl module
void Init_volumectl() {
  VolumeCtl = rb_define_module("VolumeCtl");
  
  // Master Mixer
  MasterMixer = rb_define_class_under(VolumeCtl, "MasterMixer", rb_cData);
  rb_define_alloc_func(MasterMixer, master_mixer_alloc);
  rb_define_method(MasterMixer, "initialize", master_mixer_m_initialize, 0);
  rb_define_method(MasterMixer, "volume", method_master_mixer_volume, 0);
  rb_define_method(MasterMixer, "test", method_master_mixer_test, 0);
  rb_define_method(MasterMixer, "test_s", method_master_mixer_test_set, 1);
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