#include "FreeRTOS.h"
#include "task.h"

#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "mp3_decoder.h"
#include "mp3_isr.h"
#include "mp3_lcd.h"
#include "mp3_rotary_encoder.h"
#include "queue.h"
#include "semphr.h"
#include "song_list.h"

extern int current_state;
extern int middle_button_function;
extern bool pause;
extern volatile size_t song_index;
extern volatile size_t menu_index;
extern SemaphoreHandle_t Sem_mp3_control;
extern TaskHandle_t player_handle;
extern QueueHandle_t Q_songname;
extern QueueHandle_t Q_songdata;

// Note: You may want another separate array for falling vs. rising edge callbacks
static function_pointer_t gpio0_callbacksR[32]; // Rising Edge
static function_pointer_t gpio2_callbacksR[32]; // Rising Edge

void mp3__gpio_attach_interrupt(gpio_s portpin, function_pointer_t callback) {
  if (portpin.port_number == GPIO__PORT_0) {
    gpio0_callbacksR[portpin.pin_number] = callback;
    LPC_GPIOINT->IO0IntEnF |= (1 << portpin.pin_number);
  } else if (portpin.port_number == GPIO__PORT_2) {
    gpio2_callbacksR[portpin.pin_number] = callback;
    LPC_GPIOINT->IO2IntEnF |= (1 << portpin.pin_number);
  }
}

void mp3__interrupt_dispatcher(void) {
  // Check which pin generated the interrupt
  int pin = -1;  // temp var to save the pin
  int port = -1; // temp var to save the port
  for (int i = 0; i < 32; i++) {
    if ((LPC_GPIOINT->IO0IntStatF & (1 << i))) {
      pin = i;
      port = 0;
      break;
    } else if ((LPC_GPIOINT->IO2IntStatF & (1 << i))) {
      pin = i;
      port = 2;
      break;
    }
  }

  if (pin < 0 || port < 0)
    return; // If pin and port are not set, there was no interrupt

  const int pin_that_generated_interrupt = pin; // put the pin that generated the interrupt into the const variable
  function_pointer_t attached_user_handler; // Once the correct lookup table function is found, it will be stored here
  if (port == 0) {                          // Check which port triggered the interrupt
    attached_user_handler = gpio0_callbacksR[pin_that_generated_interrupt];
  } else if (port == 2) {
    attached_user_handler = gpio2_callbacksR[pin_that_generated_interrupt];
  }
  // Invoke the user registered callback
  attached_user_handler();
  // Clear the interrupts for all pins on both ports
  LPC_GPIOINT->IO0IntClr = 0xFFFFFFFF;
  LPC_GPIOINT->IO2IntClr = 0xFFFFFFFF;
}

void gpio_interrupt(void) { mp3__interrupt_dispatcher(); }

static void change_middle_button_functionality(int menu_option) {
  if (menu_option == middle_song_select) {
    mp3__gpio_attach_interrupt(middle_button, select_song_using_center_button_ISR);
    middle_button_function = middle_song_select;
  } else if (menu_option == middle_menu_select) {
    mp3__gpio_attach_interrupt(middle_button, select_menu_using_center_button_ISR);
    middle_button_function = middle_menu_select;
  } else if (menu_option == middle_set_options) {
    mp3__gpio_attach_interrupt(middle_button, confirm_updates_using_center_button_ISR);
    middle_button_function = middle_set_options;
  } else if (menu_option == middle_pause) {
    mp3__gpio_attach_interrupt(middle_button, play_pause_ISR);
    middle_button_function = middle_pause;
  }
}

static void attach_music_list_interrupts() {
  mp3__gpio_attach_interrupt(up_button, move_up_music_list_ISR);
  mp3__gpio_attach_interrupt(down_button, move_down_music_list_ISR);
  change_middle_button_functionality(middle_song_select);
}

void interrupt_init() {
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio_interrupt, "ISR");
  NVIC_EnableIRQ(GPIO_IRQn);
  attach_music_list_interrupts();
  mp3__gpio_attach_interrupt(right_button, play_next_ISR);
  mp3__gpio_attach_interrupt(left_button, play_prev_ISR);
}

static void attach_treble_control_interrupts() {
  mp3__gpio_attach_interrupt(up_button, increase_treble_ISR);
  mp3__gpio_attach_interrupt(down_button, decrease_treble_ISR);
}

static void attach_bass_control_interrupts() {
  mp3__gpio_attach_interrupt(up_button, increase_bass_ISR);
  mp3__gpio_attach_interrupt(down_button, decrease_bass_ISR);
}

static void attach_volume_control_interrupts() {
  mp3__gpio_attach_interrupt(up_button, increase_volume_ISR);
  mp3__gpio_attach_interrupt(down_button, decrease_volume_ISR);
}

// Interrupt Functions
void play_next_ISR() {
  current_state = next;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void play_prev_ISR() {
  current_state = previous;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void play_pause_ISR() {
  current_state = paused;
  pause = !pause;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void move_up_music_list_ISR() {
  current_state = moveup_music_select;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void move_down_music_list_ISR() {
  current_state = movedown_music_select;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void move_up_menu_list_ISR() {
  current_state = moveup_menu_select;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void move_down_menu_list_ISR() {
  current_state = movedown_menu_select;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void increase_volume_ISR() {
  current_state = increasing_volume;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void decrease_volume_ISR() {
  current_state = decreasing_volume;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void increase_bass_ISR() {
  current_state = increasing_bass;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void decrease_bass_ISR() {
  current_state = decreasing_bass;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void increase_treble_ISR() {
  current_state = increasing_treble;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void decrease_treble_ISR() {
  current_state = decreasing_treble;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void select_song_using_center_button_ISR() {
  current_state = songpicker;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void select_menu_using_center_button_ISR() {
  current_state = menupicker;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
void confirm_updates_using_center_button_ISR() {
  current_state = menuconfirm;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
// -----End of Interrupts-----

// Helpers for interrupts
static void reload_default_menu_interrupts() {
  mp3__gpio_attach_interrupt(up_button, move_up_menu_list_ISR);
  mp3__gpio_attach_interrupt(down_button, move_down_menu_list_ISR);
  change_middle_button_functionality(middle_pause);
}
static void increment_song_index() {
  song_index++;
  if (song_index >= song_list__get_item_count()) {
    song_index = 0;
  }
}
static void decrement_song_index() {
  if (song_index == 0) {
    song_index = song_list__get_item_count();
  }
  song_index--;
}

static void toggle_menu_index() {
  if (menu_index == 0) {
    menu_index = 1;
  } else
    menu_index = 0;
}

void print_3_songs_with_selector() {
  decrement_song_index();
  lcd_print_string(song_list__get_name_for_item(song_index), 1);
  increment_song_index();
  lcd_print_string(song_list__get_name_for_item(song_index), 2);
  increment_song_index();
  lcd_print_string(song_list__get_name_for_item(song_index), 3);
  decrement_song_index();
  lcd_print_arrow_on_right_side(2);
}

static void print_song_info_and_send_song_into_queue() {
  lcd_print_string("Now Playing: ", 0);
  lcd_print_song_details_in_line_1_and_2(song_list__get_name_for_item(song_index));
  xQueueSend(Q_songname, song_list__get_name_for_item(song_index), portMAX_DELAY);
}

static const char *get_menu_option(int option) {
  char *menu_item = "";
  if (option == change_song)
    menu_item = "Select Song";
  else if (option == settings)
    menu_item = "Sound Settings";

  return menu_item;
}

static void print_menu_options() {
  lcd_print_string(get_menu_option(menu_index), 1);
  toggle_menu_index();
  lcd_print_string(get_menu_option(menu_index), 2);
  toggle_menu_index();
}

static void force_unpause() {
  if (pause) {
    vTaskResume(player_handle);
    pause = false;
  }
}

static void print_volume_percentage_to_lcd() {
  uint8_t volume_percentage = mp3_get_volume_percentage();
  if (volume_percentage >= 100) {
    lcd_print_single_char_at_cursor_position('1', 1, 2);
    lcd_print_single_char_at_cursor_position('0', 2, 2);
    lcd_print_single_char_at_cursor_position('0', 3, 2);
  } else if (volume_percentage < 10) {
    char number_str[4];
    sprintf(number_str, "%d", volume_percentage);
    lcd_print_single_char_at_cursor_position(' ', 1, 2);
    lcd_print_single_char_at_cursor_position('0', 2, 2);
    lcd_print_single_char_at_cursor_position(number_str[0], 3, 2);
  } else {
    char number_str[4];
    sprintf(number_str, "%d", volume_percentage);
    lcd_print_single_char_at_cursor_position(' ', 1, 2);
    lcd_print_single_char_at_cursor_position(number_str[0], 2, 2);
    lcd_print_single_char_at_cursor_position(number_str[1], 3, 2);
  }
}

static void print_treble_bar_to_lcd() {
  uint8_t treble = mp3_get_treble();
  bool negative = (treble & (1 << 3));
  int temp = 0;
  if (negative) {
    treble &= ~(1 << 3);
    lcd_print_single_char_at_cursor_position('-', 0, 2);
    temp = 8 - treble;
  } else {
    lcd_print_single_char_at_cursor_position('+', 0, 2);
    temp = treble;
  }
  char treblenum = temp + '0';
  lcd_print_single_char_at_cursor_position(treblenum, 1, 2);
}

static void print_bass_bar_to_lcd() {
  uint8_t bass = mp3_get_bass();
  uint8_t index = 3;
  lcd_print_single_char_at_cursor_position('[', 2, 2);
  while (index < 17) {
    if (index < bass + 3)
      lcd_print_single_char_at_cursor_position('=', index, 2);
    else
      lcd_print_single_char_at_cursor_position(' ', index, 2);
    index++;
  }
  lcd_print_single_char_at_cursor_position(']', 17, 2);
}
// ------End of Helpers--------

// ISR Handlers:
void pause_handler() {
  if (!pause)
    vTaskResume(player_handle);
  else
    vTaskSuspend(player_handle);
}

void next_handler() {
  increment_song_index();
  lcd_clear();
  print_song_info_and_send_song_into_queue();
  reload_default_menu_interrupts();
  force_unpause();
}

void previous_handler() {
  decrement_song_index();
  lcd_clear();
  print_song_info_and_send_song_into_queue();
  reload_default_menu_interrupts();
  force_unpause();
}

void moveup_music_select_handler() {
  lcd_print_string("Pick a song to play:", 0);
  if (middle_button_function != middle_song_select)
    change_middle_button_functionality(middle_song_select);
  decrement_song_index();
  print_3_songs_with_selector();
}

void movedown_music_select_handler() {
  lcd_print_string("Pick a song to play:", 0);
  if (middle_button_function != middle_song_select)
    change_middle_button_functionality(middle_song_select);
  increment_song_index();
  print_3_songs_with_selector();
}

void toggle_menu_select_handler() {
  static int execute_once = 0;
  if (!execute_once) {
    lcd_print_string("Menu:", 0);
    print_menu_options();
    execute_once = 1;
  }
  if (middle_button_function != middle_menu_select)
    change_middle_button_functionality(middle_menu_select);
  toggle_menu_index();
  int cursor_line_index = menu_index + 1;
  if (cursor_line_index == 1) {
    lcd_print_single_char_at_cursor_position('<', 19, 1);
    lcd_print_single_char_at_cursor_position(' ', 19, 2);
  } else if (cursor_line_index == 2) {
    lcd_print_single_char_at_cursor_position('<', 19, 2);
    lcd_print_single_char_at_cursor_position(' ', 19, 1);
  }
}

void songpicker_handler() {
  lcd_clear();
  print_song_info_and_send_song_into_queue();
  reload_default_menu_interrupts();
  force_unpause();
}

void volume_handler() {
  attach_volume_control_interrupts();
  print_volume_percentage_to_lcd();
}

void bass_handler() {
  attach_bass_control_interrupts();
  print_bass_bar_to_lcd();
}

void treble_handler() {
  attach_treble_control_interrupts();
  print_treble_bar_to_lcd();
}

void volume_increase_handler() {
  mp3_increase_vol_by_10_percent();
  print_volume_percentage_to_lcd();
}
void volume_decrease_handler() {
  mp3_decrease_vol_by_10_percent();
  print_volume_percentage_to_lcd();
}
void bass_increase_handler() {
  mp3_increase_bass();
  print_bass_bar_to_lcd();
}
void bass_decrease_handler() {
  mp3_decrease_bass();
  print_bass_bar_to_lcd();
}
void treble_increase_handler() {
  mp3_increase_treble();
  print_treble_bar_to_lcd();
}
void treble_decrease_handler() {
  mp3_decrease_treble();
  print_treble_bar_to_lcd();
}

void menuconfirm_handler() {
  lcd_clear();
  lcd_print_string("Now Playing: ", 0);
  lcd_print_song_details_in_line_1_and_2(song_list__get_name_for_item(song_index));
  reload_default_menu_interrupts();
}

void menupicker_handler() {
  lcd_clear();
  if (menu_index == change_song) {
    attach_music_list_interrupts();
    moveup_music_select_handler();
  } else if (menu_index == settings) {
    lcd_print_string("Volume| Bass |Treble", 1);
    // Prep next screen print layout
    for (int i = 0; i < 4; i++) {
      lcd_print_single_char_at_cursor_position('|', 6, i);
      lcd_print_single_char_at_cursor_position('|', 13, i);
    }
    lcd_print_single_char_at_cursor_position('%', 4, 2);
    volume_handler();
  }

  if (middle_button_function != middle_song_select) {
    change_middle_button_functionality(middle_set_options);
  }
}
// -----End of Handlers-------