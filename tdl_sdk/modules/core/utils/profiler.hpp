#pragma once
#include <sys/time.h>
#include <map>
#include <string>
#include <vector>
double get_cur_time_usecs();
double get_cur_time_millisecs();
class Timer {
 public:
  Timer(const std::string &name = "", int summary_cond_times = 100);
  ~Timer();

  void Tic();
  void Toc(int times = 1);
  void Config(const std::string &name, int summary_cond_times = 100);
  void TicToc(const std::string &str_step);

 private:
  void Summary();

 private:
  std::string name_;
  struct timeval start_;
  struct timeval end_;
  float total_time_;
  int times_ = 0;
  int summary_cond_times_;

  std::map<int, struct timeval> step_time_;
  std::vector<std::string> step_name_vec_;
  std::map<int, double> step_time_elpased_;
};

class FpsProfiler {
 public:
  FpsProfiler(const std::string &name, int summary_cond_cnts = 100);
  ~FpsProfiler();

  void Add(int cnts = 1);
  void Config(const std::string &name, int summary_cond_cnts = 100);

 private:
  float Elapse();
  void Summary();
  pthread_mutex_t lock_;
  std::string name_;
  struct timeval start_;
  struct timeval end_;

  int cnts_;
  int summary_cond_cnts_;
  float tmp_fps_ = 0;
  float average_fps_ = 0;
};
