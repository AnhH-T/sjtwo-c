#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "ff.h"
#include "gpio.h"
#include "mp3_project.h"
#include "periodic_scheduler.h"
#include "queue.h"
#include "sj2_cli.h"

#include "songname.h"

typedef struct {
  char data[512];
} songdata_s;

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

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
    vTaskDelay(1000);
    printf("Requested song [%s] has been received\n", song.name);
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

int main(void) {
  Q_songdata = xQueueCreate(5, sizeof(songdata_s));
  Q_songname = xQueueCreate(1, sizeof(songname_s));
  sj2_cli__init();
  mp3_decoder_init();
  xTaskCreate(mp3_reader_task, "reader", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(mp3_player_task, "player", 2048 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}