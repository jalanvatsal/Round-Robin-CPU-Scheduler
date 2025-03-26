#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time; // Amount of CPU time a process needs to complete its execution

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  u32 first_start_time; // Tracks when the process first gets the CPU
  u32 remaining_time; // Tracks remaining burst time
  u32 end_time;  // Tracks when the process completes
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

int cmp(const void *a, const void *b){
  
  const struct process *p1 = (const struct process *)a;
  const struct process *p2 = (const struct process *)b;

  if(p1 -> arrival_time != p2 -> arrival_time){
    return p1 -> arrival_time - p2 -> arrival_time;
  }

  return (p1 -> pid) - (p2 -> pid);

}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0; // Formula for 1 process = end_time - arrival_time - burst_time, must accumulate for all of them
  u32 total_response_time = 0; // Formula for 1 process = first_start_time - arrival_time, must accumulate for all of them

  /* Your code here */
  if(quantum_length > 0){
    
      // Initialize Process Data (For 3 Extra Fields)
    for (u32 i = 0; i < size; ++i){
      data[i].first_start_time = -1;
      data[i].remaining_time = data[i].burst_time;
      data[i].end_time = -1;
    };

    // Sort the queue by arrival time and PID as secondary parameter
    qsort(data,size,sizeof(struct process),cmp);

    u32 current_time = 0;
    u32 next_process_index = 0;
    u32 completed_processes = 0;

    // Add processes to the Ready Queue for the first time
    while(next_process_index < size && data[next_process_index].arrival_time <= current_time){
      TAILQ_INSERT_TAIL(&list,&(data[next_process_index]),pointers);
      next_process_index++;
    }

    while(completed_processes < size){
      
      if(!TAILQ_EMPTY(&list)){

        struct process * curr = TAILQ_FIRST(&list);
        TAILQ_REMOVE(&list, curr, pointers);

        if (curr -> first_start_time == -1){
          curr -> first_start_time = current_time;
          total_response_time += (curr -> first_start_time - curr -> arrival_time);
        }

        // Run Time Slice For Process
        u32 run_time = 0;
        if(curr -> remaining_time < quantum_length){
          run_time = curr -> remaining_time;
        } else{
          run_time = quantum_length;
        }

        curr -> remaining_time -= run_time;
        current_time += run_time;

        // Add any Processes that arrived During the Quantum to the Queue
        while(next_process_index < size && data[next_process_index].arrival_time <= current_time){
          TAILQ_INSERT_TAIL(&list,&(data[next_process_index]),pointers);
          next_process_index++;
        }

        if(curr -> remaining_time == 0){
          curr -> end_time = current_time;
          total_waiting_time += ((curr -> end_time) - (curr -> arrival_time) - (curr -> burst_time));
          completed_processes++;

        } else{
          TAILQ_INSERT_TAIL(&list,curr,pointers);
        }

      } else{

        if(next_process_index < size){
          // Queue is currently empty, but more processes are left to be processed. So speed up "CPU Idle Time"
          current_time = data[next_process_index].arrival_time;

          // Add the next processes to the queue based on updated clock
          while(next_process_index < size && data[next_process_index].arrival_time <= current_time){
            TAILQ_INSERT_TAIL(&list,&(data[next_process_index]),pointers);
            next_process_index++;
          }

        } else{
          // We have processed everything
          break;
        }
        
      }

    }
      
  }  
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}

