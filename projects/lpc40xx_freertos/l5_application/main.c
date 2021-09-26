#include <stdio.h>

#include "FreeRTOS.h"
#include "adc.h"
#include "board_io.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "math.h"
#include "pwm1.h"
#include "queue.h"
#include "task.h"

static QueueHandle_t adc_to_pwm_task_queue;

void pwm_task(void *p) {
  int adc_reading = 0;
  // Setting frequency, preferably over 30 Hz. Under 30Hz you will notice it flicker
  pwm1__init_single_edge(100);

  // Locate a GPIO pin that a PWM channel will control
  // Parameters: port, pin, function | note: GPIO__FUNCTION_1 is PWM function for P2.0 - P2.5
  // Equivalent to manually setting IOCON of a pin
  gpio__construct_with_function(2, 0, GPIO__FUNCTION_1);

  // Continue to vary the duty cycle in the loop
  while (1) {
    if (xQueueReceive(adc_to_pwm_task_queue, &adc_reading, 100)) {
      adc_reading = floor((100 * adc_reading / 4095));
      fprintf(stderr, "adc_reading received from Queue: %d%% \n", adc_reading);
      fprintf(stderr, "MR0: %ld, MR1: %ld \n", LPC_PWM1->MR0, LPC_PWM1->MR1);
      pwm1__set_duty_cycle(PWM1__2_0, adc_reading);
    }
  }
}

void adc_task(void *p) {
  adc__initialize();

  // Configure burst mode for channel 5
  adc__enable_burst_mode(ADC__CHANNEL_5);

  // Configure a pin, such as P1.31 with FUNC 011 to route this pin as ADC channel 5
  LPC_IOCON->P1_31 &= ~(1 << 7); // Set pin to analog mode
  gpio__construct_with_function(1, 31, GPIO__FUNCTION_3);

  int adc_reading = 0;

  while (1) {
    adc_reading = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_5);
    float voltage = ((float)adc_reading / (float)4095) * (float)3.3;
    fprintf(stderr, "Sending Value to Queue: %d\n", adc_reading);
    fprintf(stderr, "In Volts: %.2f V\n", (double)voltage);
    xQueueSend(adc_to_pwm_task_queue, &adc_reading, 0);
    vTaskDelay(100);
  }
}

int main(void) {
  adc_to_pwm_task_queue = xQueueCreate(10, sizeof(int));

  xTaskCreate(pwm_task, "PWM Task", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(adc_task, "ADC Task", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  vTaskStartScheduler();

  return 0;
}

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
