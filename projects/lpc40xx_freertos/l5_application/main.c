#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "board_io.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "sj2_cli.h"
#include "task.h"

void task1(void *p) {
  while (1) {
    printf("Task 1 Running!!!\n");
    vTaskDelay(1000);
  }
}

void task2(void *p) {
  while (1) {
    vTaskDelay(1000);
    printf("Task 2 Running!!!\n");
  }
}

int main(void) {

  sj2_cli__init();
  xTaskCreate(task1, "t1", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(task2, "t2", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  vTaskStartScheduler();

  return 0;
}

#if 0
// -------------Code from producer consumer lab-------------------------------

#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "board_io.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "queue.h"
#include "task.h"

static QueueHandle_t switch_queue;

typedef enum { switch__off, switch__on } switch_e;

switch_e get_switch_input_from_switch0() {
  // The switch I'm using is SW3 (Port 0 Pin 29)
  switch_e sw_val;
  if (LPC_GPIO0->PIN & (1 << 29))
    sw_val = switch__on;
  else
    sw_val = switch__off;
  return sw_val;
}

void producer(void *p) { // Low prio
  while (1) {
    const switch_e switch_value = get_switch_input_from_switch0();

    printf("P: B4 Send\n");
    xQueueSend(switch_queue, &switch_value, 0);
    printf("P: Af Send\n");

    vTaskDelay(1000);
  }
}

void consumer(void *p) { // High prio
  switch_e switch_value;
  while (1) {
    printf("C: B4 Reci\n");
    xQueueReceive(switch_queue, &switch_value, portMAX_DELAY);
    printf("C: Af Reci = %d\n", switch_value);
  }
}

int main(void) {
  // Initialization
  gpio__construct_as_input(0, 29);
  switch_queue = xQueueCreate(1, sizeof(switch_e));

  xTaskCreate(producer, "Producer", (512U * 4) / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(consumer, "Consumer", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  vTaskStartScheduler();

  return 0;
}

#endif

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
