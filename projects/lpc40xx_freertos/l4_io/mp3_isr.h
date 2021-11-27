#pragma once

#include "gpio.h"
#include <stdio.h>

enum State {
  next,
  previous,
  paused,
  idle,
  movedown_music_select,
  moveup_music_select,
  movedown_menu_select,
  moveup_menu_select,
  songpicker,
  menupicker,
  menuconfirm,
  increasing_volume,
  decreasing_volume,
  increasing_bass,
  decreasing_bass,
  increasing_treble,
  decreasing_treble,
  bass_control,
  treble_control
};
enum menu_option { change_song, change_volume, change_bass, change_treble };
enum middle_button_current_function { middle_pause, middle_menu_select, middle_song_select, middle_set_options };

// Function pointer type (demonstrated later in the code sample)
typedef void (*function_pointer_t)(void);

// Allow the user to attach their callbacks
void mp3__gpio_attach_interrupt(gpio_s portpin, function_pointer_t callback);

// Our main() should configure interrupts to invoke this dispatcher where we will invoke user attached callbacks
// You can hijack 'interrupt_vector_table.c' or use API at lpc_peripherals.h
void mp3__interrupt_dispatcher(void);

void interrupt_init();

// Interrupt Functions
void play_next_ISR();
void play_prev_ISR();
void play_pause_ISR();
void move_up_music_list_ISR();
void move_down_music_list_ISR();
void move_up_menu_list_ISR();
void move_down_menu_list_ISR();
void increase_volume_ISR();
void decrease_volume_ISR();
void increase_bass_ISR();
void decrease_bass_ISR();
void increase_treble_ISR();
void decrease_treble_ISR();
void select_song_using_center_button_ISR();
void select_menu_using_center_button_ISR();
void confirm_updates_using_center_button_ISR();

// Helpers
void print_3_songs_with_selector();

// ISR Handlers:
void pause_handler();
void next_handler();
void previous_handler();
void moveup_music_select_handler();
void movedown_music_select_handler();
void moveup_menu_select_handler();
void movedown_menu_select_handler();
void songpicker_handler();
void volume_handler();
void bass_handler();
void treble_handler();
void volume_increase_handler();
void volume_decrease_handler();
void bass_increase_handler();
void bass_decrease_handler();
void treble_increase_handler();
void treble_decrease_handler();
void menuconfirm_handler();
void menupicker_handler();