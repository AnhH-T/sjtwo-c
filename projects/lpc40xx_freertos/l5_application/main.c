#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "ff.h"
#include "i2c.h"
#include "i2c_slave_functions.h"
#include "i2c_slave_init.h"
#include "uart_lab.h"

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

typedef struct {
  char song_Name[512];
} songName_s;

typedef struct {
  char byte[512];
} buffer_s;

static void open_file_and_send_song_data_bytes(songName_s *name) {
  buffer_s byte_recieve;
  FIL file;
  UINT br; /* File read/write count */
  FRESULT result;
  // char temp_byte;

  result = f_open(&file, name->song_Name, FA_READ);
  if (FR_OK == result) {
    do {
      result = f_read(&file, &byte_recieve.byte, sizeof(buffer_s), &br);
      if (FR_OK == result) {
        xQueueSend(Q_songdata, &byte_recieve.byte, portMAX_DELAY);
        printf("Sent song byte\n");
      } else
        printf("Couldnt read anything");
    } while (br != 0);
  } else
    printf("File cannot be opened");
}

void mp3_reader_task(void *p) {
  songName_s name;
  while (1) {
    xQueueReceive(Q_songname, name.song_Name, portMAX_DELAY);
    printf("\nReceived song: %s\n", name.song_Name);
    open_file_and_send_song_data_bytes(&name);
  }
}

// Player task receives song data over Q_songdata to send it to the MP3 decoder
void mp3_player_task(void *p) {
  char byte_recieved[512];
  char temp_byte; // single bute
  while (1) {
    // xQueueReceive(Q_songdata, &byte_recieved, portMAX_DELAY);
    xQueueReceive(Q_songdata, &byte_recieved, portMAX_DELAY);
    printf("Song byte recieved: 0x%x\n", byte_recieved);
    // send data to the mp3 decoder
  }
}

int main(void) {
  sj2_cli__init();
  Q_songname = xQueueCreate(1, sizeof(songName_s));
  Q_songdata = xQueueCreate(1, sizeof(buffer_s));
  xTaskCreate(mp3_reader_task, "Mp3_Reader_Task", 4096 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "Mp3_Player_Task", 4096 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  vTaskStartScheduler();
  return 0;
}

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
