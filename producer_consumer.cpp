#include <pthread.h>
#include <unistd.h>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition_producer = PTHREAD_COND_INITIALIZER;
static pthread_cond_t condition_consumer = PTHREAD_COND_INITIALIZER;

bool debug_is_enabled = false;
bool end = false;
bool is_finished = false;

struct consumer {
  int sleep_time = -1;
  int* shared_variable;
  int* count;

  consumer(int sleep_time, int* shared_variable, int* count)
      : sleep_time(sleep_time),
        shared_variable(shared_variable),
        count(count){};
};

struct producer {
  int* shared_variable;

  producer(int* shared_variable) : shared_variable(shared_variable){};
};

struct interrupter {
  int size_consumers = -1;
  pthread_t* all_consumers;

  interrupter(int size_consumers, pthread_t* all_consumers)
      : size_consumers(size_consumers), all_consumers(all_consumers){};
};

int get_tid() {
  // 1 to 3+N thread ID
  static std::atomic<int> count{0};
  thread_local static int* tid;
  // new выделяет место на heap
  if (tid == 0) {
    tid = new int(++count);
  }
  return *tid;
}

void* producer_routine(void* arg) {
  // read data, loop through each value and update the value, notify consumer,
  // wait for consumer to process
  auto* producer_struct = static_cast<producer*>(arg);

  std::vector<int> numbers;

  std::string s;
  getline(std::cin, s);

  std::string::size_type size = s.length();
  char* const buffer = new char[size + 1];

  strcpy(buffer, s.c_str());

  char* p = strtok(buffer, " ");
  while (p) {
    numbers.push_back(std::stoi(p));
    p = strtok(NULL, " ");
  }

  for (auto num : numbers) {
    pthread_mutex_lock(&mutex);
    //присваиваем разделяемой переменной данные
    *(producer_struct->shared_variable) = num;
    pthread_cond_signal(&condition_consumer);

    while (*(producer_struct->shared_variable) != 0) {
      pthread_cond_wait(&condition_producer, &mutex);
    }

    pthread_mutex_unlock(&mutex);
  }

  is_finished = true;
  pthread_mutex_lock(&mutex);
  pthread_cond_broadcast(&condition_consumer);
  pthread_mutex_unlock(&mutex);

  delete[] buffer;
  delete (producer_struct);
  return nullptr;
}

void* consumer_routine(void* arg) {
  (void)arg;
  // for every update issued by producer, read the value and add to sum
  // return pointer to result (for particular consumer)
  auto* consumer_struct = static_cast<consumer*>(arg);
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

  //пока все данные не считаны заходим в уловие
  while (!is_finished) {
    pthread_mutex_lock(&mutex);
    //пока данные не будут готовы ждем оповещения от продьюсера
    while (!is_finished && *(consumer_struct->shared_variable) == 0) {
      pthread_cond_wait(&condition_consumer, &mutex);
    }

    //проверяем что данные есть и считываем их
    if (*(consumer_struct->shared_variable) != 0) {
      *(consumer_struct->count) += *(consumer_struct->shared_variable);
      if (debug_is_enabled) {
        std::fprintf(stderr, "tid=%d psum=%d\n", get_tid(),
                     *(consumer_struct->count));
      }
      *(consumer_struct->shared_variable) = 0;
    }
    //оповещаем продьюсера что мы забрали данные
    pthread_cond_signal(&condition_producer);
    pthread_mutex_unlock(&mutex);

    //уходим спать
    usleep(std::rand() % consumer_struct->sleep_time);
  }

  delete (consumer_struct);
  return nullptr;
}

void* consumer_interruptor_routine(void* arg) {
  // interrupt random consumer while producer is running

  auto* interrupter_struct = (interrupter*)arg;

  //пока есть данные -> идет попытка закрытия потока
  while (!is_finished) {
    pthread_cancel(
        interrupter_struct
            ->all_consumers[std::rand() % interrupter_struct->size_consumers]);
  }

  delete (interrupter_struct);
  return nullptr;
}

// the declaration of run threads can be changed as you like
int run_threads(int consumers_size, int sleep_time, bool debug) {
  // start N threads and wait until they're done
  // return aggregated sum of values
  debug_is_enabled = false;
  end = false;
  is_finished = false;

  //дебаг
  debug_is_enabled = debug;

  //разделяемая переменная
  int shared_variable = 0;

  //ответ
  int answer = 0;

  //массив ссылок на psum каждого консьюмера
  int* consumers_variable_count = new int[consumers_size];

  //массив потоков
  pthread_t* consumers_pointers = new pthread_t[consumers_size];

  //ссылки на потоки продьюсера и интеррапора
  pthread_t producer_pointer, interrupter_pointer;

  for (int i = 0; i < consumers_size; i++) {
    consumers_variable_count[i] = 0;
  }

  //создание продьюсера
  pthread_create(&producer_pointer, nullptr, producer_routine,
                 new producer(&shared_variable));

  //тут я передаю каждому треду свой указатель на count, чтобы в конце
  //суммироввать.
  for (int i = 0; i < consumers_size; i++) {
    pthread_create(&consumers_pointers[i], nullptr, consumer_routine,
                   new consumer(1000 * sleep_time + 1, &shared_variable,
                                &consumers_variable_count[i]));
  }

  pthread_create(&interrupter_pointer, nullptr, consumer_interruptor_routine,
                 new interrupter(consumers_size, consumers_pointers));

  //джоиним продьюсер, джоиним консьюмеров
  pthread_join(producer_pointer, nullptr);
  pthread_join(interrupter_pointer, nullptr);

  //суммировать все ответы с потоков
  for (int i = 0; i < consumers_size; ++i) {
    pthread_join(consumers_pointers[i], nullptr);
    answer += consumers_variable_count[i];
  }

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&condition_producer);
  pthread_cond_destroy(&condition_consumer);
  delete[] consumers_variable_count;
  delete[] consumers_pointers;
  return answer;
}
