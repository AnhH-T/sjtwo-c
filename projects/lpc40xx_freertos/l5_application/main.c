#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "delay.h"
#include "ff.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "periodic_scheduler.h"
#include "queue.h"
#include "semphr.h"
#include "sj2_cli.h"

#include "mp3_decoder.h"
#include "mp3_isr.h"
#include "mp3_lcd.h"
#include "mp3_rotary_encoder.h"
#include "song_list.h"
#include "songname.h"

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

typedef struct {
  char data[512];
} songdata_s;

volatile size_t song_index;
volatile size_t menu_index;

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

SemaphoreHandle_t Sem_mp3_control;

int current_state = idle;
int middle_button_function = middle_song_select;
bool pause = false;

// used to pause/play
TaskHandle_t player_handle;
void gpio_interrupt(void) { mp3__interrupt_dispatcher(); }

// Interrupt Functions
static void play_next_ISR() {
  current_state = next;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void play_prev_ISR() {
  current_state = previous;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void play_pause_ISR() {
  current_state = paused;
  pause = !pause;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void move_up_music_list_ISR() {
  current_state = moveup_music_select;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void move_down_music_list_ISR() {
  current_state = movedown_music_select;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void move_up_menu_list_ISR() {
  current_state = moveup_menu_select;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void move_down_menu_list_ISR() {
  current_state = movedown_menu_select;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void increase_volume_ISR() {
  current_state = increasing_volume;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void decrease_volume_ISR() {
  current_state = decreasing_volume;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void increase_bass_ISR() {
  current_state = increasing_bass;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void decrease_bass_ISR() {
  current_state = decreasing_bass;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void increase_treble_ISR() {
  current_state = increasing_treble;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void decrease_treble_ISR() {
  current_state = decreasing_treble;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void select_song_using_center_button_ISR() {
  current_state = songpicker;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void select_menu_using_center_button_ISR() {
  current_state = menupicker;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void confirm_updates_using_center_button_ISR() {
  current_state = menuconfirm;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
// -----End of Interrupts-----

// Helpers
void increment_song_index() {
  song_index++;
  if (song_index >= song_list__get_item_count()) {
    song_index = 0;
  }
}

void decrement_song_index() {
  if (song_index == 0) {
    song_index = song_list__get_item_count();
  }
  song_index--;
}

void increment_menu_index() {
  menu_index++;
  if (menu_index > 3) {
    menu_index = 0;
  }
}

void decrement_menu_index() {
  if (menu_index == 0) {
    menu_index = 4;
  }
  menu_index--;
}

void reload_default_menu_interrupts() {
  mp3__gpio_attach_interrupt(up_button, move_up_menu_list_ISR);
  mp3__gpio_attach_interrupt(down_button, move_down_menu_list_ISR);
  mp3__gpio_attach_interrupt(middle_button, play_pause_ISR);
  middle_button_function = middle_pause;
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

void print_song_info_and_send_song_into_queue() {
  lcd_print_string("Now Playing: ", 0);
  lcd_print_song_details_in_line_1_and_2(song_list__get_name_for_item(song_index));
  xQueueSend(Q_songname, song_list__get_name_for_item(song_index), portMAX_DELAY);
}

const char *get_menu_option(int option) {
  char *menu_item = "";
  if (option == change_song)
    menu_item = "Select Song";
  else if (option == change_volume)
    menu_item = "Volume Control";
  else if (option == change_bass)
    menu_item = "Bass Control";
  else if (option == change_treble)
    menu_item = "Treble Control";

  return menu_item;
}

void print_menu_options_with_selector() {
  decrement_menu_index();
  lcd_print_string(get_menu_option(menu_index), 1);
  increment_menu_index();
  lcd_print_string(get_menu_option(menu_index), 2);
  increment_menu_index();
  lcd_print_string(get_menu_option(menu_index), 3);
  decrement_menu_index();
  lcd_print_arrow_on_right_side(2);
}

void force_unpause() {
  if (pause) {
    vTaskResume(player_handle);
    pause = false;
  }
}

void print_volume_bar_to_lcd() {
  uint8_t index = 5;
  uint8_t volume_tenth_percentage = mp3_get_volume_percentage() / 10;
  lcd_print_single_char_at_cursor_position('[', 4, 2);
  while (index < 15) {
    if (index < volume_tenth_percentage + 5)
      lcd_print_single_char_at_cursor_position('=', index, 2);
    else
      lcd_print_single_char_at_cursor_position(' ', index, 2);
    index++;
  }
  lcd_print_single_char_at_cursor_position(']', 15, 2);
}

void print_treble_bar_to_lcd() {
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

void print_bass_bar_to_lcd() {
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
// -------End of Helpers

// ISR Handlers:
static void pause_handler() {
  if (!pause)
    vTaskResume(player_handle);
  else
    vTaskSuspend(player_handle);
}

static void next_handler() {
  increment_song_index();
  lcd_clear();
  print_song_info_and_send_song_into_queue();
  reload_default_menu_interrupts();
  force_unpause();
}

static void previous_handler() {
  decrement_song_index();
  lcd_clear();
  print_song_info_and_send_song_into_queue();
  reload_default_menu_interrupts();
  force_unpause();
}

static void moveup_music_select_handler() {
  lcd_print_string("Pick a song to play:", 0);
  if (middle_button_function != middle_song_select) {
    mp3__gpio_attach_interrupt(middle_button, select_song_using_center_button_ISR);
    middle_button_function = middle_song_select;
  }
  decrement_song_index();
  print_3_songs_with_selector();
}

static void movedown_music_select_handler() {
  lcd_print_string("Pick a song to play:", 0);
  if (middle_button_function != middle_song_select) {

    mp3__gpio_attach_interrupt(middle_button, select_song_using_center_button_ISR);
    middle_button_function = middle_song_select;
  }
  increment_song_index();
  print_3_songs_with_selector();
}

static void moveup_menu_select_handler() {
  lcd_print_string("Menu:", 0);
  if (middle_button_function != middle_menu_select) {
    mp3__gpio_attach_interrupt(middle_button, select_menu_using_center_button_ISR);
    middle_button_function = middle_menu_select;
  }
  decrement_menu_index();
  print_menu_options_with_selector();
}

static void movedown_menu_select_handler() {
  lcd_print_string("Menu:", 0);
  if (middle_button_function != middle_menu_select) {
    mp3__gpio_attach_interrupt(middle_button, select_menu_using_center_button_ISR);
    middle_button_function = middle_menu_select;
  }
  increment_menu_index();
  print_menu_options_with_selector();
}

static void songpicker_handler() {
  lcd_clear();
  print_song_info_and_send_song_into_queue();
  reload_default_menu_interrupts();
  force_unpause();
}

static void volume_handler() { // not done
  mp3__gpio_attach_interrupt(up_button, increase_volume_ISR);
  mp3__gpio_attach_interrupt(down_button, decrease_volume_ISR);
  print_volume_bar_to_lcd();
}

static void bass_handler() { // not done
  mp3__gpio_attach_interrupt(up_button, increase_bass_ISR);
  mp3__gpio_attach_interrupt(down_button, decrease_bass_ISR);
  print_bass_bar_to_lcd();
}

static void treble_handler() { // not done
  mp3__gpio_attach_interrupt(up_button, increase_treble_ISR);
  mp3__gpio_attach_interrupt(down_button, decrease_treble_ISR);
  print_treble_bar_to_lcd();
}

static void volume_increase_handler() {
  mp3_increase_vol_by_10_percent();
  print_volume_bar_to_lcd();
}
static void volume_decrease_handler() {
  mp3_decrease_vol_by_10_percent();
  print_volume_bar_to_lcd();
}
static void bass_increase_handler() {
  mp3_increase_bass();
  print_bass_bar_to_lcd();
}
static void bass_decrease_handler() {
  mp3_decrease_bass();
  print_bass_bar_to_lcd();
}
static void treble_increase_handler() {
  mp3_increase_treble();
  print_treble_bar_to_lcd();
}
static void treble_decrease_handler() {
  mp3_decrease_treble();
  print_treble_bar_to_lcd();
}

static void menuconfirm_handler() {
  lcd_clear();
  lcd_print_string("Now Playing: ", 0);
  lcd_print_song_details_in_line_1_and_2(song_list__get_name_for_item(song_index));
  reload_default_menu_interrupts();
}

static void menupicker_handler() {
  lcd_clear();
  if (menu_index == change_song) {
    mp3__gpio_attach_interrupt(middle_button, select_song_using_center_button_ISR);
    mp3__gpio_attach_interrupt(up_button, move_up_music_list_ISR);
    mp3__gpio_attach_interrupt(down_button, move_down_music_list_ISR);
    middle_button_function = middle_song_select;
    moveup_music_select_handler();
  } else if (menu_index == change_volume) {
    lcd_print_string("Volume: ", 0);
    volume_handler();
  } else if (menu_index == change_bass) {
    lcd_print_string("Bass: ", 0);
    bass_handler();
  } else if (menu_index == change_treble) {
    lcd_print_string("Treble: ", 0);
    treble_handler();
  }

  if (middle_button_function != middle_song_select) {
    mp3__gpio_attach_interrupt(middle_button, confirm_updates_using_center_button_ISR);
    middle_button_function = middle_set_options;
  }
}
// -----End of Handlers-------

// Init functions

void interrupt_init() {
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio_interrupt, "ISR");
  NVIC_EnableIRQ(GPIO_IRQn);
  mp3__gpio_attach_interrupt(up_button, move_up_music_list_ISR);
  mp3__gpio_attach_interrupt(down_button, move_down_music_list_ISR);
  mp3__gpio_attach_interrupt(middle_button, select_song_using_center_button_ISR);
  mp3__gpio_attach_interrupt(right_button, play_next_ISR);
  mp3__gpio_attach_interrupt(left_button, play_prev_ISR);
}
// -----End of inits-------

static void read_loop(FIL *file) {
  songdata_s buffer;
  UINT br = 1;
  FRESULT result;
  while ((br != 0) && (uxQueueMessagesWaiting(Q_songname) == 0)) {
    result = f_read(file, &buffer.data, sizeof(songdata_s), &br);
    if (FR_OK == result) {
      xQueueSend(Q_songdata, &buffer, portMAX_DELAY);
      // printf("Sent [%d] bytes onto the queue\n", br);
    } else
      printf("File Read Error!");
  }
}

static void read_mp3_file(songname_s song) {
  const char *filename = song.name;
  FIL file; // File handle
  FRESULT result = f_open(&file, filename, FA_READ);

  if (FR_OK == result) {
    read_loop(&file);
  } else {
    printf("ERROR: Failed to open: %s\n", filename);
  }

  f_close(&file);
}

void mp3_reader_task(void *p) {
  songname_s song;
  while (1) {
    xQueueReceive(Q_songname, &song, portMAX_DELAY);
    read_mp3_file(song);
  }
}

void mp3_player_task(void *p) {
  songdata_s bytes_512;
  uint8_t counter = 32;
  while (1) {
    xQueueReceive(Q_songdata, &bytes_512, portMAX_DELAY);
    for (int i = 0; i < 512; i++) {
      if (counter > 31) {
        while (!DREQ_Ready()) {
          ;
        }
        counter = 0;
      }
      sj2_send_music_data(bytes_512.data[i]);
      counter++;
    }
    // printf("%d: Received [%d] Bytes from Queue\n", count++, sizeof(bytes_512.data));
  }
}

void mp3_control(void *p) {
  while (1) {
    if (xSemaphoreTake(Sem_mp3_control, portMAX_DELAY)) {
      switch (current_state) {
      case paused:
        pause_handler();
        break;
      case next:
        next_handler();
        break;
      case previous:
        previous_handler();
        break;
      case moveup_music_select:
        moveup_music_select_handler();
        break;
      case movedown_music_select:
        movedown_music_select_handler();
        break;
      case moveup_menu_select:
        moveup_menu_select_handler();
        break;
      case movedown_menu_select:
        movedown_menu_select_handler();
        break;
      case songpicker:
        songpicker_handler();
        break;
      case menupicker:
        menupicker_handler();
        break;
      case menuconfirm:
        menuconfirm_handler();
        break;
      case increasing_volume:
        volume_increase_handler();
        break;
      case decreasing_volume:
        volume_decrease_handler();
        break;
      case increasing_bass:
        bass_increase_handler();
        break;
      case decreasing_bass:
        bass_decrease_handler();
        break;
      case increasing_treble:
        treble_increase_handler();
        break;
      case decreasing_treble:
        treble_decrease_handler();
        break;
      }

      current_state = idle;
    }
  }
}

int main(void) {
  Q_songdata = xQueueCreate(5, sizeof(songdata_s));
  Q_songname = xQueueCreate(1, sizeof(songname_s));
  song_index = 0;
  menu_index = 0;
  Sem_mp3_control = xSemaphoreCreateBinary();

  encoder_button_init();
  interrupt_init();
  mp3_decoder_init();
  lcd_init();
  song_list__populate();

  lcd_print_string("Pick a song to play:", 0);
  print_3_songs_with_selector();

  xTaskCreate(mp3_control, "control", 2048 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_reader_task, "reader", 2048 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_player_task, "player", 2048 / sizeof(void *), NULL, PRIORITY_HIGH, &player_handle);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails
  return 0;
}