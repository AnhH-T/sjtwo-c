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
#include "ssp2_lab.h"
#include "uart_lab.h"

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

typedef struct {
  char song_Name[512];
} songName_s;

static void open_file_and_send_song_data_bytes(songName_s *name) {
  song_data_s byte_recieve;
  FIL file;
  UINT br; /* File read/write count */
  FRESULT result;
  // char temp_byte;

  result = f_open(&file, name->song_Name, FA_READ);
  if (FR_OK == result) {
    do {
      result = f_read(&file, &byte_recieve.byte, sizeof(song_data_s), &br);
      if (FR_OK == result) {
        xQueueSend(Q_songdata, &byte_recieve.byte, portMAX_DELAY);
        // printf("Sent song byte\n");
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

//-----------------------------MILE STONE 2----------------------------------
// Player task receives song data over Q_songdata to send it to the MP3 decoder
typedef struct {
  uint8_t address;
  uint8_t sci_reg;
  uint8_t opcode_read;
  song_data_s *song_data;
} decoder_s;

void mp3_cs(void) { LPC_GPIO2->PIN &= ~(1 << 0); }
void mp3_ds(void) { LPC_GPIO2->PIN |= (1 << 0); }

static void send_song_data_byte_to_mp3_decoder(song_data_s byte_recieved) {
  // DREQ is low, the register will update
  // DREQ is high, incoming change
  decoder_s d;
  uint8_t read_instruction = 0x3; // ch 7.4
  uint8_t ram_read_or_write_address = 0x6;

  mp3_cs();
  d.opcode_read = ssp2_lab__exchange_byte(read_instruction); // send the read opcode instruction
  d.address = ssp2_lab__exchange_byte(ram_read_or_write_address);
  ssp2_lab__exchange_song_byte(byte_recieved);
  mp3_ds();
}

void mp3_player_task(void *p) {
  song_data_s byte_recieved;

  while (1) {
    // xQueueReceive(Q_songdata, &byte_recieved, portMAX_DELAY);
    xQueueReceive(Q_songdata, &byte_recieved, portMAX_DELAY);
    // printf("\nSong byte recieved\n");
    send_song_data_byte_to_mp3_decoder(byte_recieved);
  }
}

int main(void) {
  const uint32_t spi_clock_mhz = 24;
  sj2_cli__init();
  ssp2_lab__init(spi_clock_mhz);
  Q_songname = xQueueCreate(1, sizeof(songName_s));
  Q_songdata = xQueueCreate(1, sizeof(song_data_s));
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
