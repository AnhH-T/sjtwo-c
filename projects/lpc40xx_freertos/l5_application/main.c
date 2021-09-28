#include <stdio.h>

#include "FreeRTOS.h"
#include "board_io.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "task.h"

#include "semphr.h"
#include "ssp2_lab.h"

static SemaphoreHandle_t spi_bus_mutex;

typedef struct {
  uint8_t manufacturer_id;
  uint8_t device_id_1;
  uint8_t device_id_2;
} adesto_flash_id_s;

void configure_ssp2_pin() {
  gpio__construct_with_function(1, 0, GPIO__FUNCTION_4); // SCK2
  gpio__construct_with_function(1, 1, GPIO__FUNCTION_4); // MOSI
  gpio__construct_with_function(1, 4, GPIO__FUNCTION_4); // MISO
  // LPC_IOCON->P1_4 |= (4 << 0); Example of doing it manually instead of constructing
  gpio__construct_as_output(1, 10); // CS: Configuring direction as output

  // Extra configs for logic analyzer trigger
  gpio_s trigger = gpio__construct_with_function(2, 0, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(trigger);
}

// Implement Adesto flash memory CS signal as a GPIO driver
void adesto_cs(void) {
  LPC_GPIO1->PIN &= ~(1 << 10);
  LPC_GPIO2->PIN &= ~(1 << 0); // For trigger
}
void adesto_ds(void) {
  LPC_GPIO1->PIN |= (1 << 10);
  LPC_GPIO2->PIN |= (1 << 0); // For trigger
}

adesto_flash_id_s adesto_read_signature(void) {
  adesto_flash_id_s data = {0};
  uint8_t gunk;
  adesto_cs();
  gunk = ssp2_lab__exchange_byte(0x9F); // Send 9F and takes the trash
  data.manufacturer_id = ssp2_lab__exchange_byte(0x00);
  data.device_id_1 = ssp2_lab__exchange_byte(0x00);
  data.device_id_2 = ssp2_lab__exchange_byte(0x00);
  adesto_ds();

  return data;
}

void spi_task(void *p) {
  while (1) {
    adesto_flash_id_s id = adesto_read_signature();
    // printf the members of the 'adesto_flash_id_s' struct
    printf("Manufacture ID: %.2x\n", id.manufacturer_id);
    printf("Device ID 1: %.2x\n", id.device_id_1);
    printf("Device ID 2: %.2x\n", id.device_id_2);
    vTaskDelay(500);
  }
}

void spi_id_verification_task(void *p) {
  while (1) {
    if (xSemaphoreTake(spi_bus_mutex, 1000)) {
      const adesto_flash_id_s id = adesto_read_signature();
      printf("%s - Manufacture ID: %.2x\n", pcTaskGetName(NULL), id.manufacturer_id);
      printf("%s - Device ID 1: %.2x\n", pcTaskGetName(NULL), id.device_id_1);
      printf("%s - Device ID 2: %.2x\n", pcTaskGetName(NULL), id.device_id_2);
      xSemaphoreGive(spi_bus_mutex);
      vTaskDelay(500);
      // When we read a manufacturer ID we do not expect, we will kill this task
      if (0x1F != id.manufacturer_id) {
        fprintf(stderr, "Manufacturer ID read failure\n");
        vTaskSuspend(NULL); // Kill this task
      }
    }
  }
}

int main(void) {
  // Initialization
  const uint32_t spi_clock_mhz = 24;
  ssp2_lab__init(spi_clock_mhz);
  configure_ssp2_pin();

  spi_bus_mutex = xSemaphoreCreateMutex();
  // xTaskCreate(spi_task, "SPI Task", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(spi_id_verification_task, "SPI1 Task", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(spi_id_verification_task, "SPI2 Task", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
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
//   // uart3_init();                                                                     // Also include:  uart3_init.h
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
