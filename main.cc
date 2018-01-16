/******************************************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/

#include "helper.h"

// Errors
#define ERROR_WRONG_NUMBER_OF_ARGUMENTS 		1
#define ERROR_CLOCK_GETTIME                 2
#define ERROR_TIMEDOUT                      3

// Semaphores
#define MUTEX                               0
#define FULL                                1
#define EMPTY                               2
#define MESSAGE_SEM                         3

const int NUMBER_OF_SEMAPHORES = 4;
const int TIMEOUT = 20;
const int MAX_SLEEP_BETWEEN_JOBS = 5;
const int MAX_JOB_DURATION = 10;

struct job_t
{
  int id;
  int duration;
};

struct prod_param_t
{
  int id;
  int number_of_jobs;
  int queue_size;
  job_t *queue_ptr;
  int *job_counter_ptr;
  int *in_ptr;
  int *sem_id_ptr;
};

struct cons_param_t
{
  int id;
  int queue_size;
  job_t *queue_ptr;
  int *job_counter_ptr;
  int *out_ptr;
  int *sem_id_ptr;
};

void *producer (void *id);
void *consumer (void *id);

int main (int argc, char **argv)
{
  if(argc != 5)
  {
    std::cerr << "Error: Provide four arguments" << endl;
    return ERROR_WRONG_NUMBER_OF_ARGUMENTS;
  }

  // Fetch Command line arguments
  int queue_size = check_arg(argv[1]);
  int number_of_jobs = check_arg(argv[2]);
  int number_of_producers = check_arg(argv[3]);
  int number_of_consumers = check_arg(argv[4]);

  // Initialize the queue as an array of jobs and a pointer to that array
  srand(time(NULL));
  job_t queue[queue_size];
  job_t *queue_ptr = &queue[0];
  int job_counter = 0, in = 0, out = 0;

  cout << "Queue size is: "  << queue_size << endl << "Number of jobs is: "
  << number_of_jobs << endl << "Number of producers is: " << number_of_producers
  << endl << "Number of consumers is: " << number_of_consumers << endl << endl << endl;

  // Create semaphores
  int sem_id = sem_create(SEM_KEY, NUMBER_OF_SEMAPHORES);

  // Set-up and initialize semaphores
  sem_init(sem_id, MUTEX, 1);
  sem_init(sem_id, FULL, 0);
  sem_init(sem_id, EMPTY, queue_size);
  sem_init(sem_id, MESSAGE_SEM, 1);

  // Create consumer and producer threads
  pthread_t producers[number_of_producers];
  pthread_t consumers[number_of_consumers];
  prod_param_t *producers_param;
  producers_param = new prod_param_t[number_of_producers];
  cons_param_t *consumers_param;
  consumers_param = new cons_param_t[number_of_consumers];

  for(int i = 0; i < number_of_producers; i++)
  {
    producers_param[i].id = i;
    producers_param[i].number_of_jobs = number_of_jobs;
    producers_param[i].queue_size = queue_size;
    producers_param[i].queue_ptr = queue_ptr;
    producers_param[i].job_counter_ptr = &job_counter;
    producers_param[i].in_ptr = &in;
    producers_param[i].sem_id_ptr = &sem_id;
    pthread_create(&producers[i], NULL, producer, (void *) &producers_param[i]);
  }

  for(int i = 0; i < number_of_consumers; i++)
  {
    consumers_param[i].id = i;
    consumers_param[i].queue_size = queue_size;
    consumers_param[i].queue_ptr = queue_ptr;
    consumers_param[i].job_counter_ptr = &job_counter;
    consumers_param[i].out_ptr = &out;
    consumers_param[i].sem_id_ptr = &sem_id;
    pthread_create(&consumers[i], NULL, consumer, (void *) &consumers_param[i]);
  }

  for (int i = 0; i < number_of_producers; i++)
    pthread_join(producers[i], NULL);

  for (int i = 0; i < number_of_consumers; i++)
    pthread_join(consumers[i], NULL);

    sem_close(sem_id);
    return 0;
}

void *producer(void *parameter)
{
  // Extract producer parameters
  prod_param_t *producer_param = static_cast<prod_param_t*>(parameter);
  int producer_id = producer_param->id;
  int number_of_jobs = producer_param->number_of_jobs;
  int queue_size = producer_param->queue_size;
  job_t *queue = producer_param->queue_ptr;
  int *job_counter_ptr = producer_param->job_counter_ptr;
  int &job_counter = *job_counter_ptr;
  int *in_ptr = producer_param->in_ptr;
  int &in = *in_ptr;
  int *sem_id_ptr = producer_param->sem_id_ptr;
  int &sem_id = *sem_id_ptr;

  struct timespec ts;
  int error_code;
  job_t job;

  while(true)
  {
    sleep(rand() % MAX_SLEEP_BETWEEN_JOBS + 1);
    if(clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
      cerr << "Error: Clock time cannot be retrieved" << endl;
      pthread_exit((void*) ERROR_CLOCK_GETTIME);
    }
    ts.tv_sec = TIMEOUT;
    while((error_code = sem_timedwait(sem_id, EMPTY, &ts)) == -1 && errno == EINTR)
      continue;
    if(error_code == -1)
    {
      if(errno == EAGAIN)
      {
        sem_wait(sem_id, MESSAGE_SEM);
        cout << "Producer(" << producer_id << "): "
        << "Empty slot not available after 20 seconds. Quitting." << endl;
        sem_signal(sem_id, MESSAGE_SEM);
        pthread_exit((void*) 0);
      } else
      {
        sem_wait(sem_id, MESSAGE_SEM);
        cerr << "Error: Timedout error. Make sure you have cleaned a semaphore array."
        " Use: ipcrm {shm|msg|sem} id (or ipcrm -a to remove all) \n";
        sem_signal(sem_id, MESSAGE_SEM);
        pthread_exit((void*) ERROR_TIMEDOUT);
      }
    } else
    {
      sem_wait(sem_id, MUTEX);

      /*************************** CRITICAL SECTION ***************************/
      // Create jobs
      job.id = in + 1;
      job.duration = rand() % MAX_JOB_DURATION + 1;

      // Insert jobs to the queue
      if(job_counter < queue_size)
      {
        queue[in] = job;
        job_counter++;
        in = (in + 1) % queue_size;
        sem_wait(sem_id, MESSAGE_SEM);
        cout << "Producer(" << producer_id << "): Job id " << job.id
        << " duration " << job.duration << endl;
        sem_signal(sem_id, MESSAGE_SEM);
        number_of_jobs--;
      }
      /************************ END OF CRITICAL SECTION ***********************/
      sem_signal(sem_id, MUTEX);
      sem_signal(sem_id, FULL);

      if(number_of_jobs == 0)
      {
        sem_wait(sem_id, MESSAGE_SEM);
        cout << "Producer(" << producer_id << "): "
        << "No more jobs to generate. Quitting." << endl;
        sem_signal(sem_id, MESSAGE_SEM);
        pthread_exit((void*) 0);
      }
    }
  }
}

void *consumer (void *parameter)
{
  // Extract producer parameters
  cons_param_t *consumer_param = static_cast<cons_param_t*>(parameter);
  int consumer_id = consumer_param->id;
  int queue_size = consumer_param->queue_size;
  job_t *queue = consumer_param->queue_ptr;
  int *job_counter_ptr = consumer_param->job_counter_ptr;
  int &job_counter = *job_counter_ptr;
  int *out_ptr = consumer_param->out_ptr;
  int &out = *out_ptr;
  int *sem_id_ptr = consumer_param->sem_id_ptr;
  int &sem_id = *sem_id_ptr;

  struct timespec ts;
  int error_code;
  job_t job;

  while(true)
  {
    if(clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
      cerr << endl << "Error: Clock time cannot be retrieved" << endl;
      pthread_exit((void*) ERROR_CLOCK_GETTIME);
    }
    ts.tv_sec = TIMEOUT;
    while((error_code = sem_timedwait(sem_id, FULL, &ts)) == -1 && errno == EINTR)
      continue;
    if(error_code == -1)
    {
      if(errno == EAGAIN)
      {
        sem_wait(sem_id, MESSAGE_SEM);
        cout << "Consumer(" << consumer_id << "): " << "No jobs left. Quitting."
        << endl;
        sem_signal(sem_id, MESSAGE_SEM);
        pthread_exit((void*) 0);
      } else
      {
        sem_wait(sem_id, MESSAGE_SEM);
        cerr << "Error: Timedout error. Make sure you have cleaned a semaphore array."
        " Use: ipcrm {shm|msg|sem} id (or ipcrm -a to remove all) \n";
        sem_signal(sem_id, MESSAGE_SEM);
        pthread_exit((void*) ERROR_TIMEDOUT);
      }
    } else
    {
      sem_wait(sem_id, MUTEX);
      /*************************** CRITICAL SECTION ***************************/
      if(job_counter > 0)
      {
        job = queue[out];
        job_counter--;
        out = (out + 1) % queue_size;
        sem_wait(sem_id, MESSAGE_SEM);
        cout << "Consumer(" << consumer_id << "): Job id " << job.id
        << " executing sleep duration " << job.duration << endl;
        sem_signal(sem_id, MESSAGE_SEM);
      }
      /************************ END OF CRITICAL SECTION ***********************/
      sem_signal(sem_id, MUTEX);
      sem_signal(sem_id, EMPTY);
      sleep(job.duration);
      sem_wait(sem_id, MESSAGE_SEM);
      cout << "Consumer(" << consumer_id << "): Job id " << job.id
      << " completed." << endl;
      sem_signal(sem_id, MESSAGE_SEM);
    }
  }
}
