#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <string>
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

// Struct to hold arguments for pthread function
struct Arguments {
    std::string input;
    std::string* output;
    size_t iteration;

    // Constructor
    Arguments(const std::string& in, std::string* out, size_t iter)
        : input(in), output(out), iteration(iter) {}
};

// Function to calculate the greatest common divisor (GCD) using Euclidean algorithm
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
bool compareTasks(const Task& a, const Task& b) {
    return a.period < b.period;
}

void outputInfo(std::stringstream & entropy_values_sstr, std::vector<Task> tasks, size_t iteration, unsigned hyperperiod, double utilization)
{
  entropy_values_sstr << "CPU " << iteration
                      << "\nTask scheduling information: ";
  for (size_t i = 0; i < tasks.size(); ++i) {
      entropy_values_sstr << tasks[i].name << " (WCET: " << tasks[i].wcet
                          << ", Period: " << tasks[i].period << ")";
      if (i < tasks.size() - 1) {
          entropy_values_sstr << ", ";
      }
  }
  entropy_values_sstr << "\nTask set utilization: " << std::setprecision(2)
                      << std::fixed << utilization;
  entropy_values_sstr << "\nHyperperiod: " << hyperperiod;
}

// Function to parse input and calculate hyperperiod, utilization, and generate scheduling diagram
std::string parse(const std::string& input, size_t iteration) {
    std::stringstream input_string(input);
    std::stringstream entropy_values_sstr;
    std::vector<Task> tasks;
    char task_name;
    unsigned task_wcet;
    unsigned task_period;

    // Parse input string
    while (input_string >> task_name >> task_wcet >> task_period) {
        tasks.push_back({task_name, task_wcet, task_period, task_wcet});
    }

    // Calculate hyperperiod
    unsigned hyperperiod = 1;
    for (const auto& task : tasks) {
        hyperperiod = lcm(hyperperiod, task.period);
    }

    // Calculate utilization
    double utilization = 0.0;
    for (const auto& task : tasks) {
        utilization += static_cast<double>(task.wcet) / task.period;
    }

    // Output task scheduling information
   outputInfo(entropy_values_sstr, tasks, iteration, hyperperiod, utilization);

  // Sort tasks based on their periods
  std::sort(tasks.begin(), tasks.end(), compareTasks);

    // Check schedulability
    double threshold = tasks.size() * (std::pow(2.0, 1.0 / tasks.size()) - 1);
    if (utilization > 1) {
        entropy_values_sstr << "\nRate Monotonic Algorithm execution for CPU"
            << iteration << ":" << "\nThe task set is not schedulable";
    } else if (utilization > threshold || utilization < 0) {
        entropy_values_sstr <<"\nRate Monotonic Algorithm execution for CPU"
            << iteration << ":" << "\nTask set schedulability is unknown";
    } else {
        entropy_values_sstr << "\nRate Monotonic Algorithm execution for CPU"
                            << iteration << ":";

        // Generate scheduling diagram
        std::vector<TaskInterval> task_intervals;
        for (unsigned tick = 0; tick < hyperperiod; ++tick) {
            bool task_running = false;
            for (auto& task : tasks) {
                if (tick % task.period == 0) {
                    task.wcet = task.initial_wcet; // Reset WCET
                    for (auto& interval : task_intervals) {
                        //if (interval.name == task.name) {
                            interval.stopped = true; // Reset stopped flag
                        //}
                    }
                }
            }
            for (auto& task : tasks) {
                if (task.wcet > 0) {
                    task_running = true;
                    if (task.wcet == task.initial_wcet) {
                        // Fresh task, create new interval
                        task_intervals.push_back({task.name, tick, tick + 1, false});
                    } else {
                        // Task still running, extend interval or create new one
                        bool found = false;
                        for (auto& interval : task_intervals) {
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
        std::string diagram = "Scheduling Diagram for CPU " + std::to_string(iteration) + ": ";
        char last_task = ' ';
        for (const auto& interval : task_intervals) {
            if (interval.name != 'I') {
                diagram += interval.name;
                diagram += "(" + std::to_string(interval.end - interval.start) + "), ";
                last_task = interval.name;
            } else {
                if (last_task != 'I') {
                    diagram += "Idle(" + std::to_string(interval.end - interval.start) + "), ";
                    last_task = 'I';
                }
            }
        }
        // Remove trailing comma and space
        diagram.pop_back();
        diagram.pop_back();

        entropy_values_sstr << "\n" << diagram;
    }

    return entropy_values_sstr.str();
}

// Thread function
void* thread_function(void* arguments) {
    Arguments* args = static_cast<Arguments*>(arguments);
    std::string input = args->input;
    std::string* output = args->output;
    size_t iteration = args->iteration;

    *output = parse(input, iteration);

    return nullptr;
}

// Function to get inputs from user
std::vector<std::string> get_inputs() {
    std::vector<std::string> inputs;
    std::string input;
  for (int i = 0; i < 3; i++) {
    std::getline(std::cin, input);
    if (!input.empty()) {
      inputs.push_back(input);
    }
  }
    // while (std::getline(std::cin, input)) {
    //     if (!input.empty()) {
    //         inputs.push_back(input);
    //     }
    // }
    return inputs;
}

int main() {
    const std::vector<std::string> inputs = get_inputs();
    std::vector<std::string> outputs(inputs.size());
    std::vector<pthread_t> threads(inputs.size());
    std::vector<Arguments> arg_objects;

    // Prepare arguments
    for (size_t i = 0; i < inputs.size(); ++i) {
        arg_objects.emplace_back(inputs[i], &outputs[i], i + 1);
    }
    // Create threads
    for (size_t i = 0; i < inputs.size(); ++i) {
        pthread_create(&threads[i], nullptr, thread_function, &arg_objects[i]);
    }

    // Wait for threads to finish
    for (size_t i = 0; i < inputs.size(); ++i) {
        pthread_join(threads[i], nullptr);
    }

  // Print outputs
  for (const auto& output : outputs) {
      std::cout << output;
      if (&output != &outputs.back()) {
          std::cout << "\n\n\n";
      }
  }
    return 0;
}
// version 5 where everything is in the correct order except for the interrupt for B isnt working yet