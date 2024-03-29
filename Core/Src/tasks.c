#include "tasks.h"

#include <stdio.h>

#include "key.h"
#include "main.h"
#include "update.h"
#include "power.h"

#define TASKS_LIST_MAX 32

typedef enum {
  TASK_TICK_25MS = 25,
  TASK_TICK_1S = 1000,
  TASK_TICK_5S = 5000,
  TASK_TICK_10S = 10000,
} TASK_TICK;

typedef struct {
  uint32_t refer;
  uint32_t tick;
  void (*run)(void);
} TASK;

typedef struct {
  uint32_t num;
  TASK list[TASKS_LIST_MAX];
  uint32_t tick_run;
  uint32_t tick_sleep;
} TASKS;

static TASKS tasks;
static uint32_t g_tick;

static KEY_VALUE getKey(void) {
  if (LL_GPIO_IsInputPinSet(KEY_GPIO_Port, KEY_Pin) == 1) {
    return K_PRESS;
  } else {
    return K_RELEASE;
  }
}

static void task_25ms(void) {
  static KEY key = {
      .status = KS_RELEASE,
      .count = 0,
      .get = getKey,
  };
  static KEY_EVENT k_ev;

  k_ev = key_status_check(&key, 20);
  switch (k_ev) {
    case KE_PRESS: {
      printf("[KEY]: PRESS\n");
    } break;
    case KE_RELEASE: {
      printf("[KEY]: RELEASE\n");
      reboot_for_update();
    } break;
    case KE_LONG_PRESS: {
      printf("[KEY]: LONG_PRESS\n");
    } break;
    case KE_LONG_RELEASE: {
      printf("[KEY]: LONG_RELEASE\n");
    } break;
    default: {
    } break;
  }
}

static void task_1s(void) {
  enter_stop2();
}

static void task_5s(void) {
  uart_printf("5s\n");
}

static void task_10s(void) {
  uart_printf("10s\n");
}

int8_t task_register(uint32_t tick, void (*run)(void)) {
  if (tasks.num >= TASKS_LIST_MAX) {
    return -1;
  }
  
  tasks.list[tasks.num].refer = tick;
  tasks.list[tasks.num].run = run;
  tasks.list[tasks.num].tick = tick;
  tasks.num += 1;

  return 0;
}

void tasks_init(void) {
  memset(&tasks, 0, sizeof(TASKS));

  g_tick = HAL_GetTick();
  tasks.tick_run = 1000;
  tasks.tick_sleep = 5000 - tasks.tick_run;

  task_register(TASK_TICK_25MS, task_25ms);
  task_register(TASK_TICK_1S, task_1s);
  task_register(TASK_TICK_5S, task_5s);
  task_register(TASK_TICK_10S, task_10s);
}

void tasks_update(void) {
  for (int i = 0; i < tasks.num; ++i) {
    if (tasks.list[i].refer > tasks.tick_run) {
      if (tasks.list[i].tick > tasks.tick_sleep) {
        tasks.list[i].tick -= tasks.tick_sleep;
      } else {
        tasks.list[i].tick = 0;
      }
    }
  }
}

void tasks_poll(void) {
  if (HAL_GetTick() - g_tick < 1) {
    return;
  }

  g_tick = HAL_GetTick();

  for (int i = 0; i < tasks.num; ++i) {
    if (tasks.list[i].tick > 0) {
      tasks.list[i].tick -= 1;
      continue;
    }

    tasks.list[i].tick = tasks.list[i].refer;
    tasks.list[i].run();
  }
}