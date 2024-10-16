// Write your code here
#include <cmath>
#include <iomanip>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// Struct to hold task information
struct Task {
  char name;
  unsigned wcet;
  unsigned period;
  unsigned initial_wcet; // We need to store initial WCET for reset
};

// Struct to hold task interval information
struct TaskInterval {
  char name;
  unsigned start;
  unsigned end;
  bool stopped; // Represents if the task was interrupted
};

// Function to calculate the greatest common divisor (GCD) using Euclidean
// algorithm
unsigned long long gcd(unsigned long long a, unsigned long long b) {
  while (b != 0) {
    unsigned long long temp = b;
    b = a % b;
    a = temp;
  }
  return a;
}

// Function to calculate the least common multiple (LCM) using GCD
unsigned long long lcm(unsigned long long a, unsigned long long b) {
  return (a * b) / gcd(a, b);
}

// Function to compare tasks based on their periods
bool compareTasks(const Task &a, const Task &b) { return a.period < b.period; }

std::string calculations(const std::string &input) {

  std::stringstream input_string(input);
  std::stringstream returnString;
  std::vector<Task> tasks;
  char task_name;
  unsigned task_wcet;
  unsigned task_period;

  // Parse input string
  while (input_string >> task_name >> task_wcet >> task_period) {
    tasks.push_back({task_name, task_wcet, task_period, task_wcet});
  }

  // Calculate utilization
  double utilization = 0.0;
  for (const auto &task : tasks) {
    utilization += static_cast<double>(task.wcet) / task.period;
  }

  // Calculate hyperperiod
  unsigned hyperperiod = 1;
  for (const auto &task : tasks) {
    hyperperiod = lcm(hyperperiod, task.period);
  }

  // Format utilization with precision 2
  returnString << std::fixed << std::setprecision(2) << utilization << " ";
  returnString << std::to_string(hyperperiod) + " ";

  // Sort tasks based on their periods
  std::sort(tasks.begin(), tasks.end(), compareTasks);

  // Check schedulability
  double threshold = tasks.size() * (std::pow(2.0, 1.0 / tasks.size()) - 1);

  if (utilization > 1) {
    returnString << "notSchedulable"; // case where util is > 1
    return returnString.str();
  } else if (utilization > threshold || utilization < 0) {
    returnString << "unknown"; // case 2
    return returnString.str();
  }

  else {
    // Generate scheduling diagram
    std::vector<TaskInterval> task_intervals;
    for (unsigned tick = 0; tick < hyperperiod; ++tick) {
      bool task_running = false;
      for (auto &task : tasks) {
        if (tick % task.period == 0) {
          task.wcet = task.initial_wcet; // Reset WCET
          for (auto &interval : task_intervals) {
            // if (interval.name == task.name) {
            interval.stopped = true; // Reset stopped flag
                                     //}
          }
        }
      }
      for (auto &task : tasks) {
        if (task.wcet > 0) {
          task_running = true;
          if (task.wcet == task.initial_wcet) {
            // Fresh task, create new interval
            task_intervals.push_back({task.name, tick, tick + 1, false});
          } else {
            // Task still running, extend interval or create new one
            bool found = false;
            for (auto &interval : task_intervals) {
              if (interval.name == task.name && !interval.stopped) {
                interval.end = tick + 1; // Extend interval
                found = true;
                break;
              }
            }
            if (!found) {
              task_intervals.push_back({task.name, tick, tick + 1, false});
            }
          }
          --task.wcet;
          break; // Move to next tick
        }
      }
      if (!task_running) {
        // No task is running at this time, insert idle interval
        task_intervals.push_back({'I', tick, tick + 1, false});
      }
    }

    // Merge adjacent idle intervals
    for (size_t i = 0; i < task_intervals.size() - 1; ++i) {
      if (task_intervals[i].name == 'I' && task_intervals[i + 1].name == 'I') {
        // Merge adjacent idle intervals
        task_intervals[i].end = task_intervals[i + 1].end;
        task_intervals.erase(task_intervals.begin() + i + 1);
        --i; // Adjust index after erasing
      }
    }

    // Generate scheduling diagram string
    std::string diagram = "";
    char last_task = ' ';
    for (const auto &interval : task_intervals) {
      if (interval.name != 'I') {
        diagram += interval.name;
        diagram += "(" + std::to_string(interval.end - interval.start) + "), ";
        last_task = interval.name;
      } else {
        if (last_task != 'I') {
          diagram +=
              "Idle(" + std::to_string(interval.end - interval.start) + "), ";
          last_task = 'I';
        }
      }
    }
    // Remove trailing comma and space
    diagram.pop_back();
    diagram.pop_back();

    returnString << diagram;
  }
  return returnString.str();
}

void fireman(int) {
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

//Function below is based off of Rincon boiler plate server.cpp file

int main(int argc, char *argv[]) {

  int sockfd, newsockfd, portno, clilen;
  struct sockaddr_in serv_addr, cli_addr;

  // Check the commandline arguments
  if (argc != 2) {
    std::cerr << "Port not provided" << std::endl;
    exit(0);
  }

  // Create the socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    std::cerr << "Error opening socket" << std::endl;
    exit(0);
  }

  // Populate the sockaddr_in structure
  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  // Bind the socket with the sockaddr_in structure
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    std::cerr << "Error binding" << std::endl;
    exit(0);
  }

  // Set the max number of concurrent connections
  listen(sockfd, 5);
  clilen = sizeof(cli_addr);

  signal(SIGCHLD, fireman);
  while (true) {
    // Accept a new connection
    newsockfd =
        accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
    if (fork() == 0) {

      if (newsockfd < 0) {
        std::cerr << "Error accepting new connections" << std::endl;
        exit(0);
      }
      int n, msgSize = 0;
      n = read(newsockfd, &msgSize, sizeof(int));
      if (n < 0) {
        std::cerr << "Error reading from socket" << std::endl;
        exit(0);
      }
      char *tempBuffer = new char[msgSize + 1];
      bzero(tempBuffer, msgSize + 1);
      n = read(newsockfd, tempBuffer, msgSize + 1);
      if (n < 0) {
        std::cerr << "Error reading from socket"
                  << " server side" << std::endl;
        exit(0);
      }
      std::string buffer = tempBuffer;
      delete[] tempBuffer;

      buffer = calculations(buffer);
      msgSize = buffer.size();
      n = write(newsockfd, &msgSize, sizeof(int));
      if (n < 0) {
        std::cerr << "Error writing to socket" << std::endl;
        exit(0);
      }
      n = write(newsockfd, buffer.c_str(), msgSize);
      if (n < 0) {
        std::cerr << "Error writing to socket" << std::endl;
        exit(0);
      }
    }
  }
  close(newsockfd);
  close(sockfd);
  return 0;
}