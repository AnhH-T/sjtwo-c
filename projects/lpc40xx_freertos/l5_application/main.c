#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "i2c.h"
#include "i2c_slave_functions.h"
#include "i2c_slave_init.h"

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

struct {
  char song_Name[512];
} songName_t;

struct {
  uint32_t song_Bytes;
} songBytes_t;

void open_file_and_send_song_data_bytes() {
  songBytes_t bytes_to_send;
  songName_t songName;
  FIL file; // File handle
  f_open(&file);
  while (!file.end()) {
    read_from_file(bytes_512);
    xQueueSend(Q_songdata, &bytes_to_send.song_Bytes, portMAX_DELAY);
  }
  close_file();
}

void mp3_reader_task(void *p) {
  Q_songname name;
  while (1) {
    xQueueReceive(Q_songname, &name, portMAX_DELAY);
    printf("Received song to play: %s\n", name);
    open_file_and_send_song_data_bytes();
  }
}

// Player task receives song data over Q_songdata to send it to the MP3 decoder
void mp3_player_task(void *p) {
  char bytes_512[512];

  while (1) {
    xQueueReceive(Q_songdata, &bytes_512[0], portMAX_DELAY);
    for (int i = 0; i < sizeof(bytes_512); i++) {
      while (!mp3_decoder_needs_data()) {
        vTaskDelay(1);
      }

      spi_send_to_mp3_decoder(bytes_512[i]);
    }
  }
}

int main(void) {
  sj2_cli__init();
  vTaskStartScheduler();
  return 0;
}

//----------------------------------------------------------

static volatile uint8_t slave_memory[256];
bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory) {
  if (memory_index > 255)
    return false;
  else
    *memory = slave_memory[memory_index];
  return true;
}

bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value) {
  if (memory_index > 255)
    return false;
  else
    slave_memory[memory_index] = memory_value;
  return true;
}
