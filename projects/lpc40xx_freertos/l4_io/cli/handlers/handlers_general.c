#include "cli_handlers.h"

#include "FreeRTOS.h"
#include "task.h"

static void cli__task_list_print(sl_string_t user_input_minus_command_name, app_cli__print_string_function cli_output);

app_cli_status_e cli__crash_me(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                               app_cli__print_string_function cli_output) {
  uint32_t *bad_pointer = (uint32_t *)0x00000001;
  *bad_pointer = 0xDEADBEEF;
  return APP_CLI_STATUS__SUCCESS;
}

app_cli_status_e cli__task_list(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                                app_cli__print_string_function cli_output) {
  const int sleep_time = sl_string__to_int(user_input_minus_command_name);
  if (sleep_time > 0) {
    vTaskResetRunTimeStats();
    vTaskDelay(sleep_time);
  }

  // re-use user_input_minus_command_name as 'output_string' to save memory:
  sl_string_t output_string = user_input_minus_command_name;
  cli__task_list_print(output_string, cli_output);

  return APP_CLI_STATUS__SUCCESS;
}

static void cli__task_list_print(sl_string_t output_string, app_cli__print_string_function cli_output) {
  void *unused_cli_param = NULL;

#if (0 != configUSE_TRACE_FACILITY)
  // Enum to char : eRunning, eReady, eBlocked, eSuspended, eDeleted
  static const char *const task_status_table[] = {"running", " ready ", "blocked", "suspend", "deleted"};

  // Limit the tasks to avoid heap allocation.
  const unsigned portBASE_TYPE max_tasks = 10;
  TaskStatus_t status[max_tasks];
  uint32_t total_cpu_runtime = 0;
  uint32_t total_tasks_runtime = 0;

  const uint32_t total_run_time = portGET_RUN_TIME_COUNTER_VALUE();
  const unsigned portBASE_TYPE task_count = uxTaskGetSystemState(&status[0], max_tasks, &total_cpu_runtime);

  sl_string__printf(output_string, "%10s  Status Pr Stack CPU%%          Time\n", "Name");
  cli_output(unused_cli_param, output_string);

  for (unsigned priority_number = 0; priority_number < configMAX_PRIORITIES; priority_number++) {
    /* Print in sorted priority order */
    for (unsigned i = 0; i < task_count; i++) {
      const TaskStatus_t *task = &status[i];
      if (task->uxBasePriority == priority_number) {
        total_tasks_runtime += task->ulRunTimeCounter;

        const unsigned cpu_percent = (0 == total_cpu_runtime) ? 0 : task->ulRunTimeCounter / (total_cpu_runtime / 100);
        const unsigned time_us = task->ulRunTimeCounter;
        const unsigned stack_in_bytes = (sizeof(void *) * task->usStackHighWaterMark);

        sl_string__printf(output_string, "%10s %s %2u %5u %4u %10u us\n", task->pcTaskName,
                          task_status_table[task->eCurrentState], (unsigned)task->uxBasePriority, stack_in_bytes,
                          cpu_percent, time_us);
        cli_output(unused_cli_param, output_string);
      }
    }
  }

  sl_string__printf(output_string, "Overhead: %u uS\n", (unsigned)(total_run_time - total_tasks_runtime));
  cli_output(unused_cli_param, output_string);
#else
  cli_output(unused_cli_param, "Unable to provide you the task information along with their CPU and stack usage.\n");
  cli_output(unused_cli_param, "configUSE_TRACE_FACILITY macro at FreeRTOSConfig.h must be non-zero\n");
#endif
}

//=======================================
#include "queue.h"
QueueHandle_t song_name_queue;
uint8_t pause; //We could use a pause flag appoarch as alternative

static void cli__mp3_play(sl_string_t filename, app_cli__print_string_function cli_output) {
  cli_output(NULL, "SENDING songname file: ");
  cli_output(NULL, filename);
  cli_output(NULL, "\n");
  if (xQueueSend(song_name_queue, filename, 0)) {
    cli_output(NULL, "SUCESS: SONGNAME WAS SENT TO THE QUEUE\n");
  } else {
    cli_output(NULL, "FAILED: SONGNAME WAS NOT SENT TO THE QUEUE\n");
  }
}

static void cli__mp3_pause(app_cli__print_string_function cli_output) {
  cli_output(NULL, "PAUSING song\n");
  TaskHandle_t player_task = xTaskGetHandle("player");
  vTaskSuspend(player_task);
}

static void cli__mp3_resume(app_cli__print_string_function cli_output) {
  cli_output(NULL, "Resuming song\n");
  TaskHandle_t player_task = xTaskGetHandle("player");
  vTaskResume(player_task);
}

app_cli_status_e cli__mp3(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                          app_cli__print_string_function cli_output) {

  sl_string_t s = user_input_minus_command_name;

  // If the user types 'taskcontrol suspend led0' then we need to suspend a task with the name of 'led0'
  // In this case, the user_input_minus_command_name will be set to 'suspend led0' with the command-name removed
  if (sl_string__begins_with_ignore_case(s, "play")) {
    sl_string__erase_first_word(s, ' '); // remove first word
    cli__mp3_play(s, cli_output);
  } else if (sl_string__begins_with_ignore_case(s, "pause")) {
    cli__mp3_pause(cli_output);
  } else if (sl_string__begins_with_ignore_case(s, "resume")) {
    cli__mp3_resume(cli_output);
  } else {
    cli_output(NULL, "Did you mean to say suspend or resume?\n");
  }

  return APP_CLI_STATUS__SUCCESS;
}