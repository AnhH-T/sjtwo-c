#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "acceleration.h"
#include "board_io.h"
#include "event_groups.h"
#include "ff.h"
#include "queue.h"
#include "sj2_cli.h"
#include "task.h"

#define BIT_1 (1 << 1)
#define BIT_2 (1 << 2)

void testWrite(int time[], int speed[], int arrSize) {
  for (int i = 0; i < arrSize; i++)
    printf("Time %d: %d, Speed %d: %d\n", i, time[i], i, speed[i]);
}

static QueueHandle_t sensor_queue;
static EventGroupHandle_t xCreatedEventGroup;

void write_file_using_fatfs_pi(int time[], int speed[], int arrSize) {
  const char *filename = "sensor.txt";
  FIL file; // File handle
  UINT bytes_written = 0;
  FRESULT result = f_open(&file, filename, (FA_WRITE | FA_OPEN_APPEND));

  if (FR_OK == result) {
    char string[64];
    for (int i = 0; i < arrSize; i++) {
      sprintf(string, "%i, %i\n", time[i], speed[i]);
      if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
      } else {
        printf("ERROR: Failed to write data to file\n");
      }
    }
    f_close(&file);
  } else {
    printf("ERROR: Failed to open: %s\n", filename);
  }
}

int get_sample_from_accel_z() {
  acceleration__axis_data_s temp;
  int average = 0;
  for (int i = 0; i < 100; i++) {
    temp = acceleration__get_data();
    average += temp.z;
  }
  average = average / 100;
  return average;
}

void producer(void *p) {
  int sensor_sample;
  while (1) {
    sensor_sample = get_sample_from_accel_z();

    if (xQueueSend(sensor_queue, &sensor_sample, 0)) {
      ;
    }

    xEventGroupSetBits(xCreatedEventGroup, BIT_1);
    vTaskDelay(100);
  }
}

void consumer(void *p) {
  int avg_speed, time[10], speed[10];
  int count = 0;
  while (1) {
    xQueueReceive(sensor_queue, &avg_speed, portMAX_DELAY);
    speed[count] = avg_speed;
    time[count] = xTaskGetTickCount();
    count++;

    if (count == 10) { // only write every 1 second
      count = 0;
      write_file_using_fatfs_pi(time, speed, 10);
      testWrite(time, speed, 10); // For seeing on telemetry
    }
    xEventGroupSetBits(xCreatedEventGroup, BIT_2);
  }
}

void watchdog_task(void *p) {
  EventBits_t uxBits;
  while (1) {
    // Check bit 1 and 2, clear it when function returns, wait for 200 ms max
    uxBits = xEventGroupWaitBits(xCreatedEventGroup, 0x06, pdTRUE, pdTRUE, 200);
    if ((uxBits & BIT_1) != 0) // Producer Check
      fprintf(stderr, "WATCHDOG: Producer Check-in CONFIRM\n");
    else
      fprintf(stderr, "WATCHDOG: Producer Check-in FAILED\n");
    if ((uxBits & BIT_2) != 0) // Consumer Check
      fprintf(stderr, "WATCHDOG: Consumer Check-in CONFIRM\n");
    else
      fprintf(stderr, "WATCHDOG: Consumer Check-in FAILED\n");
    vTaskDelay(1000);
  }
}

int main(void) {
  // Initialization
  acceleration__init();

  sensor_queue = xQueueCreate(1, sizeof(int));
  xCreatedEventGroup = xEventGroupCreate();
  sj2_cli__init();
  xTaskCreate(producer, "Producer", (512U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(consumer, "Consumer", (512U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(watchdog_task, "Watchdog", (512U * 4) / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  vTaskStartScheduler();

  return 0;
}

// ---------------------------------------------------------------------------------------------------------------------------

// Original code of main.c

// #include <stdio.h>

// #include "FreeRTOS.h"
// #include "task.h"

// #include "board_io.h"
// #include "common_macros.h"
// #include "periodic_scheduler.h"
// #include "sj2_cli.h"

// // 'static' to make these functions 'private' to this file
// static void create_blinky_tasks(void);
// static void create_uart_task(void);
// static void blink_task(void *params);
// static void uart_task(void *params);

// int main(void) {
//   create_blinky_tasks();
//   create_uart_task();

//   // If you have the ESP32 wifi module soldered on the board, you can try uncommenting this code
//   // See esp32/README.md for more details
//   // uart3_init();                                                                     // Also include:
//   uart3_init.h
//   // xTaskCreate(esp32_tcp_hello_world_task, "uart3", 1000, NULL, PRIORITY_LOW, NULL); // Include esp32_task.h

//   puts("Starting RTOS");

//   vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

//   return 0;
// }

// static void create_blinky_tasks(void) {
//   /**
//    * Use '#if (1)' if you wish to observe how two tasks can blink LEDs
//    * Use '#if (0)' if you wish to use the 'periodic_scheduler.h' that will spawn 4 periodic tasks, one for each LED
//    */
// #if (1)
//   // These variables should not go out of scope because the 'blink_task' will reference this memory
//   static gpio_s led0, led1;

//   // If you wish to avoid malloc, use xTaskCreateStatic() in place of xTaskCreate()
//   static StackType_t led0_task_stack[512 / sizeof(StackType_t)];
//   static StackType_t led1_task_stack[512 / sizeof(StackType_t)];
//   static StaticTask_t led0_task_struct;
//   static StaticTask_t led1_task_struct;

//   led0 = board_io__get_led0();
//   led1 = board_io__get_led1();

//   xTaskCreateStatic(blink_task, "led0", ARRAY_SIZE(led0_task_stack), (void *)&led0, PRIORITY_LOW, led0_task_stack,
//                     &led0_task_struct);
//   xTaskCreateStatic(blink_task, "led1", ARRAY_SIZE(led1_task_stack), (void *)&led1, PRIORITY_LOW, led1_task_stack,
//                     &led1_task_struct);
// #else
//   periodic_scheduler__initialize();
//   UNUSED(blink_task);
// #endif
// }

// static void create_uart_task(void) {
//   // It is advised to either run the uart_task, or the SJ2 command-line (CLI), but not both
//   // Change '#if (0)' to '#if (1)' and vice versa to try it out
// #if (0)
//   // printf() takes more stack space, size this tasks' stack higher
//   xTaskCreate(uart_task, "uart", (512U * 8) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
// #else
//   sj2_cli__init();
//   UNUSED(uart_task); // uart_task is un-used in if we are doing cli init()
// #endif
// }

// static void blink_task(void *params) {
//   const gpio_s led = *((gpio_s *)params); // Parameter was input while calling xTaskCreate()

//   // Warning: This task starts with very minimal stack, so do not use printf() API here to avoid stack overflow
//   while (true) {
//     gpio__toggle(led);
//     vTaskDelay(500);
//   }
// }

// // This sends periodic messages over printf() which uses system_calls.c to send them to UART0
// static void uart_task(void *params) {
//   TickType_t previous_tick = 0;
//   TickType_t ticks = 0;

//   while (true) {
//     // This loop will repeat at precise task delay, even if the logic below takes variable amount of ticks
//     vTaskDelayUntil(&previous_tick, 2000);

//     /* Calls to fprintf(stderr, ...) uses polled UART driver, so this entire output will be fully
//      * sent out before this function returns. See system_calls.c for actual implementation.
//      *
//      * Use this style print for:
//      *  - Interrupts because you cannot use printf() inside an ISR
//      *    This is because regular printf() leads down to xQueueSend() that might block
//      *    but you cannot block inside an ISR hence the system might crash
//      *  - During debugging in case system crashes before all output of printf() is sent
//      */
//     ticks = xTaskGetTickCount();
//     fprintf(stderr, "%u: This is a polled version of printf used for debugging ... finished in", (unsigned)ticks);
//     fprintf(stderr, " %lu ticks\n", (xTaskGetTickCount() - ticks));

//     /* This deposits data to an outgoing queue and doesn't block the CPU
//      * Data will be sent later, but this function would return earlier
//      */
//     ticks = xTaskGetTickCount();
//     printf("This is a more efficient printf ... finished in");
//     printf(" %lu ticks\n\n", (xTaskGetTickCount() - ticks));
//   }
// }
