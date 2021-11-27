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

// Helpers

// -------End of Helpers

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