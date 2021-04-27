#include <ruby.h>
#include <alsa/asoundlib.h>
#include <alsa/version.h>
#include <stdio.h>

VALUE VolumeCtl = Qnil;

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
}

// Initialize the VolumeCtl module
void Init_volumectl() {
  VolumeCtl = rb_define_module("VolumeCtl");
  
  // Get the default sink
  rb_define_module_function(VolumeCtl, "get_dsink", method_get_default_sink, 0);
  
  // Set the default sink volume
  rb_define_module_function(VolumeCtl, "get_dvolume", method_get_default_volume, 0);
  rb_define_module_function(VolumeCtl, "set_dvolume", method_set_default_volume, -1);
  rb_define_private_method(VolumeCtl, "pmod_dvolume", method_volume_mod, 1);
  
  // Mute and unmute default sink
  rb_define_module_function(VolumeCtl, "dmute", method_mute_default, 0);
  rb_define_module_function(VolumeCtl, "dunmute", method_unmute_default, 0);
  
  // Test the server
  rb_define_module_function(VolumeCtl, "test", method_test, 0);
} // End init