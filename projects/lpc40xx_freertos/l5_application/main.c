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

#include "mp3_isr.h"
#include "mp3_lcd.h"
#include "mp3_project.h"
#include "mp3_rotary_encoder.h"
#include "song_list.h"
#include "songname.h"

enum State { next, previous, paused, idle, movedown, moveup, songpicker, playing, volume };

typedef struct {
  char data[512];
} songdata_s;

volatile size_t song_index;

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

SemaphoreHandle_t Sem_mp3_control;

int current_state = idle;
bool middle_button_is_pause_button = false;
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
static void move_up_list_ISR() {
  current_state = moveup;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void move_down_list_ISR() {
  current_state = movedown;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}
static void select_song_using_center_button_ISR() {
  current_state = songpicker;
  xSemaphoreGiveFromISR(Sem_mp3_control, NULL);
}

static void volume_ISR() {
  current_state = volume;
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

void force_unpause() {
  if (pause) {
    vTaskResume(player_handle);
    pause = false;
  }
}

static void pause_handler() {
  if (!pause)
    vTaskResume(player_handle);
  else
    vTaskSuspend(player_handle);
  vTaskDelay(200);
}

static void next_handler() {
  increment_song_index();
  lcd_clear();
  print_song_info_and_send_song_into_queue();
  force_unpause();
}

static void previous_handler() {
  decrement_song_index();
  lcd_clear();
  print_song_info_and_send_song_into_queue();
  force_unpause();
}

static void moveup_handler() {
  lcd_print_string("Pick a song to play:", 0);
  if (middle_button_is_pause_button) {
    mp3__gpio_attach_interrupt(middle_button, select_song_using_center_button_ISR);
    middle_button_is_pause_button = false;
  }
  decrement_song_index();
  print_3_songs_with_selector();
}

static void movedown_handler() {
  lcd_print_string("Pick a song to play:", 0);
  if (middle_button_is_pause_button) {
    mp3__gpio_attach_interrupt(middle_button, select_song_using_center_button_ISR);
    middle_button_is_pause_button = false;
  }
  increment_song_index();
  print_3_songs_with_selector();
}

static void songpicker_handler() {
  lcd_clear();
  print_song_info_and_send_song_into_queue();
  mp3__gpio_attach_interrupt(middle_button, play_pause_ISR);
  middle_button_is_pause_button = true;
  force_unpause();
}

static void volume_handler() {
  // to be done
}

// -----End of Helpers-------

// Init functions

void interrupt_init() {
  button_init();
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio_interrupt, "ISR");
  NVIC_EnableIRQ(GPIO_IRQn);
  mp3__gpio_attach_interrupt(up_button, move_up_list_ISR);
  mp3__gpio_attach_interrupt(down_button, move_down_list_ISR);
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
      if (current_state == paused)
        pause_handler();
      else if (current_state == next)
        next_handler();
      else if (current_state == previous)
        previous_handler();
      else if (current_state == moveup)
        moveup_handler();
      else if (current_state == movedown)
        movedown_handler();
      else if (current_state == songpicker)
        songpicker_handler();
      else if (current_state == volume)
        volume_handler(); // not done

      current_state = idle;
    }
  }
}

int main(void) {
  Q_songdata = xQueueCreate(5, sizeof(songdata_s));
  Q_songname = xQueueCreate(1, sizeof(songname_s));
  song_index = 0;
  Sem_mp3_control = xSemaphoreCreateBinary();

  interrupt_init();
  mp3_decoder_init();
  lcd_init();
  button_init();
  song_list__populate();

  lcd_print_string("Pick a song to play:", 0);
  print_3_songs_with_selector();

  xTaskCreate(mp3_control, "control", 2048 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_reader_task, "reader", 2048 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_player_task, "player", 2048 / sizeof(void *), NULL, PRIORITY_HIGH, &player_handle);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails
  return 0;
}