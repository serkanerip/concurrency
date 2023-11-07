#include <cassert>
#include <iostream>
#include <semaphore>
#include <thread>
#ifdef __x86_64__
#include <emmintrin.h>
#endif

int data[5];
bool is_ready;
int actual_sum;

extern void _mm_mfence();

void sync_memory() {
  #ifdef __x86_64__
    _mm_mfence();
  #else
    __asm__ __volatile__("dmb ish");
  #endif
}

void set_initial_values() {
    data[0] = 9;
    data[1] = 9;
    data[2] = 9;
    data[3] = 9;
    data[4] = 9;
    actual_sum = 0;
    is_ready = false;
}

void init_data(std::binary_semaphore &start, std::counting_semaphore<2> &end)
{
  while (true)
  {
    start.acquire();
    for (int i = 0; i < 5; ++i)
      data[i] = i + 1;
    is_ready = true;
    end.release();
  }
}

void sum_data(int &sum, std::binary_semaphore &start, std::counting_semaphore<2> &end)
{
  while (true)
  {
    start.acquire();
    if (!is_ready)
    {
      end.release();
      continue;
    }

    sync_memory(); // MEMORY BARRIER

    for (int i = 0; i < 5; ++i)
      sum += data[i];
    end.release();
  }
}

int main()
{
  set_initial_values();
  std::binary_semaphore start_t0(0);
  std::binary_semaphore start_t1(0);
  std::counting_semaphore<2> end(0);

  std::thread t1([&]
                 { init_data(start_t0, end); });
  std::thread t2([&]
                 { sum_data(actual_sum, start_t1, end); });

  for (int i = 0;; i++)
  {
    set_initial_values();

    start_t0.release();
    start_t1.release();

    end.acquire();
    end.acquire();

    if (actual_sum == 15 || actual_sum == 0)
    {
      std::cout << "ALL GOOD! Sum: " << actual_sum << ", Iteration: " << i << std::endl;
    }
    else
    {
      std::cout << "ERROR! Sum: " << actual_sum << ", Iteration: " << i << std::endl;
      assert(false);
    }
  }
  return 0;
}