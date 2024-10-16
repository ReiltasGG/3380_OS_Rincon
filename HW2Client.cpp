// Write your code here

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sstream>
#include <string>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// Struct to hold arguments for pthread function
struct Arguments {
  std::string input;
  std::string *output;
  size_t iteration;

  char *serverIP; // argv[1]
  char *portno;   // argv[2]

  // Constructor
  Arguments(const std::string &in, std::string *out, size_t iter, char *servIP,
            char *pNum) {
    input = in;
    output = out;
    iteration = iter;
    serverIP = servIP;
    portno = pNum;
  }

  // Arguments(const std::string& in, std::string* out, size_t iter, char*
  // servIP, char* pNum)
  //     : input(in), output(out), iteration(iter), serverIP(servIP),
  //     portno(pNum) {}
};

struct Task {
  char name;
  unsigned wcet;
  unsigned period;
  unsigned initial_wcet; // We need to store initial WCET for reset
};

void outputInfo(std::stringstream &entropy_values_sstr, std::vector<Task> tasks,
                size_t iteration, unsigned hyperperiod, double utilization) {
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
  entropy_values_sstr << "\nHyperperiod: " << hyperperiod << "\n";
}

//Function below is based off of Rincon boiler plate client.cpp file

// Thread function
void *thread_function(void *arguments) {
  Arguments *args = static_cast<Arguments *>(arguments);

  std::string input = args->input;
  std::string *output = args->output;
  size_t iteration = args->iteration;
  int portno;

  int sockfd, n;
  portno = std::atoi(args->portno); // argv[2]
  std::string buffer = args->input;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    std::cerr << "ERROR opening socket" << std::endl;
    exit(0);
  }
  server = gethostbyname(args->serverIP); // server = gethostbyname(argv[1]);
  if (server == NULL) {
    std::cerr << "ERROR, no such host" << std::endl;
    exit(0);
  }
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(portno);
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    std::cerr << "ERROR connecting" << std::endl;
    exit(0);
  }

  int msgSize = sizeof(buffer);
  n = write(sockfd, &msgSize, sizeof(int));
  if (n < 0) {
    std::cerr << "ERROR writing to socket" << std::endl;
    exit(0);
  }
  n = write(sockfd, buffer.c_str(), msgSize);
  if (n < 0) {
    std::cerr << "ERROR writing to socket" << std::endl;
    exit(0);
  }
  n = read(sockfd, &msgSize, sizeof(int));
  if (n < 0) {
    std::cerr << "ERROR reading from socket" << std::endl;
    exit(0);
  }
  char *tempBuffer = new char[msgSize + 1];
  bzero(tempBuffer, msgSize + 1);
  n = read(sockfd, tempBuffer, msgSize);
  if (n < 0) {
    std::cerr << "ERROR reading from socket"  << std::endl;
    exit(0);
  }
  buffer = tempBuffer;
  delete[] tempBuffer;

  close(sockfd);

  std::vector<Task> tasks;

  char task_name;
  unsigned task_wcet;
  unsigned task_period;
  unsigned initial_wcet;
  std::stringstream ss(input);

  while (ss >> task_name >> task_wcet >> task_period) {
    tasks.push_back({task_name, task_wcet, task_period, task_wcet});
  }

  std::string resultString = buffer;

  // Find the position of the first space
  size_t spacePos = resultString.find(' ');

  // Extract the first part as a double
  double utility = std::stod(resultString.substr(0, spacePos));

  // Find the next space position starting from the position after the first
  // space
  size_t nextSpacePos = resultString.find(' ', spacePos + 1);

  // Extract the second part as an integer
  int hyperperiod =
      std::stoi(resultString.substr(spacePos + 1, nextSpacePos - spacePos - 1));

  // The rest is the remaining string
  std::string end = resultString.substr(nextSpacePos + 1);

  std::stringstream info;
  outputInfo(info, tasks, iteration, hyperperiod, utility);

  (*output) += info.str();

  if (buffer.find("notSchedulable") != std::string::npos) {
    (*output) += "Rate Monotonic Algorithm execution for CPU " +
                 std::to_string(iteration) + ":" +
                 "\nThe task set is not schedulable";
  } else if (buffer.find("unknown") != std::string::npos) {
    (*output) += "Rate Monotonic Algorithm execution for CPU " +
                 std::to_string(iteration) + ":" +
                 "\nTask set schedulability is unknown";
  } else {
    (*output) += "Rate Monotonic Algorithm execution for CPU " +
                 std::to_string(iteration) + ":" +
                 "\nScheduling Diagram for CPU " + std::to_string(iteration) +
                 ": " + end;
  }

  return nullptr;
}

// Function to get inputs from user
std::vector<std::string> get_inputs() {
  std::vector<std::string> inputs;
  std::string input;

  while (std::getline(std::cin, input)) {
    if (!input.empty()) {
      inputs.push_back(input);
    }
  }
  return inputs;
}

int main(int argc, char *argv[]) {
  const std::vector<std::string> inputs = get_inputs();
  std::vector<std::string> outputs(inputs.size());
  std::vector<pthread_t> threads(inputs.size());
  std::vector<Arguments> arg_objects;

  if (argc != 3) {
    std::cerr << "usage " << argv[0] << " hostname port" << std::endl;
    exit(0);
  }

  // Prepare arguments
  for (size_t i = 0; i < inputs.size(); ++i) {
    arg_objects.emplace_back(inputs[i], &outputs[i], i + 1, argv[1], argv[2]);
    // std:: cout << inputs[i] <<  &outputs[i] <<  i + 1 << " " << argv[1] << "
    // " <<  argv[2]<<  std::endl;
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
  for (const auto &output : outputs) {
    std::cout << output;
    if (&output != &outputs.back()) {
      std::cout << "\n\n\n";
    }
  }

  return 0;
}