#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "board_io.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "task.h"

#include "uart_lab.h"

void pin_init2() {
  // using UART 2
  LPC_IOCON->P2_8 &= ~(0b111 << 0); // reset IOCON
  LPC_IOCON->P2_9 &= ~(0b111 << 0); // reset IOCON
  LPC_IOCON->P2_8 |= (1 << 1);      // Initialize P2_8 to TXD (write) 
  LPC_IOCON->P2_9 |= (1 << 1);      // Initialize P2_9 to RXD (read)
}

void pin_init3() {
  // using UART 3
  LPC_IOCON->P4_28 &= ~(0b111 << 0); // reset IOCON
  LPC_IOCON->P4_29 &= ~(0b111 << 0); // reset IOCON
  LPC_IOCON->P4_28 |= (1 << 1);      // Initialize P2_8 to TXD (write)
  LPC_IOCON->P4_29 |= (1 << 1);      // Initialize P2_9 to RXD (read)
}

void uart_read_task(void *p) {
  char input_byte = 'x';
  printf("starting read task\n");
  while (1) {
    // Using interrupt method
    uart_lab__get_char_from_queue2(&input_byte, portMAX_DELAY);
    printf("received data: %c\n", input_byte);
    vTaskDelay(500);
  }
}

void uart_write_task(void *p) {
  printf("starting write task\n");
  while (1) {
    // Use uart_lab__polled_put() function to send something
    uart_lab__polled_put(UART_2, 't');
    vTaskDelay(500);
  }
}

// For part 3
void uart_3_sender_task(void *p) {
  char number_as_string[16] = {0};

  while (true) {
    const int number = 987654321;
    sprintf(number_as_string, "%i", number);

    // Send one char at a time to the other board including terminating NULL char
    for (int i = 0; i <= strlen(number_as_string); i++) {
      uart_lab__polled_put(UART_3, number_as_string[i]); // Only line modified
      fprintf(stderr, "Sent from 3: %c\n", number_as_string[i]);
    }

    fprintf(stderr, "Sent: %i over UART3 to UART 2\n", number);
    vTaskDelay(10000);
  }
}

void uart_2_receiver_task(void *p) {
  char number_as_string[16] = {0};
  int counter = 0;

  while (true) {
    char byte = 0;
    uart_lab__get_char_from_queue2(&byte, portMAX_DELAY);
    fprintf(stderr, "Received from 3: %c\n", byte);

    // This is the last char, so print the number
    if ('\0' == byte) {
      number_as_string[counter] = '\0';
      counter = 0;
      fprintf(stderr, "Received at UART 2 Receiver: %s\n", number_as_string);
    }
    // We have not yet received the NULL '\0' char, so buffer the data
    else {
      // Added code to read a char at a time
      number_as_string[counter] = byte;
      if (counter < 16)
        counter++;
    }
  }
}

void uart_2_sender_task(void *p) {
  vTaskDelay(5000);
  char number_as_string[16] = {0};

  while (true) {
    const int number = 123456789;
    sprintf(number_as_string, "%i", number);

    // Send one char at a time to the other board including terminating NULL char
    for (int i = 0; i <= strlen(number_as_string); i++) {
      uart_lab__polled_put(UART_2, number_as_string[i]);
      fprintf(stderr, "Sent from 2: %c\n", number_as_string[i]);
    }

    fprintf(stderr, "Sent: %i over UART2 to UART 3\n", number);
    vTaskDelay(10000);
  }
}

void uart_3_receiver_task(void *p) {
  char number_as_string[16] = {0};
  int counter = 0;

  while (true) {
    char byte = 0;
    uart_lab__get_char_from_queue3(&byte, portMAX_DELAY);
    fprintf(stderr, "Received from 2: %c\n", byte);

    // This is the last char, so print the number
    if ('\0' == byte) {
      number_as_string[counter] = '\0';
      counter = 0;
      fprintf(stderr, "Received at UART 3 Receiver: %s\n", number_as_string);
    }
    // We have not yet received the NULL '\0' char, so buffer the data
    else {
      number_as_string[counter] = byte;
      if (counter < 16)
        counter++;
    }
  }
}

int main(void) {
  // Initialization
  const uint32_t peripheral_clock = 96 * 1000 * 1000;
  const uint32_t baud_rate = 115200;
  // Code below is for part 2
  // pin_init2();
  // uart_lab__init(UART_2, peripheral_clock, baud_rate);
  // uart__enable_receive_interrupt(UART_2);
  // xTaskCreate(uart_read_task, "UART Read", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  // xTaskCreate(uart_write_task, "UART Write", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  // vTaskStartScheduler();

  // Code below is for part 3 UART 2 and UART 3 communication
  pin_init2();
  pin_init3();
  uart_lab__init(UART_2, peripheral_clock, baud_rate);
  uart_lab__init(UART_3, peripheral_clock, baud_rate);
  uart__enable_receive_interrupt(UART_2);
  uart__enable_receive_interrupt(UART_3);
  xTaskCreate(uart_2_sender_task, "UART2 Write", (512U * 4) / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(uart_3_receiver_task, "UART3 Read", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(uart_3_sender_task, "UART3 Write", (512U * 4) / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(uart_2_receiver_task, "UART2 Read", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
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
