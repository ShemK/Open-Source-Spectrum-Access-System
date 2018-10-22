#include <vector>
#include <ctime>
#include <cstdlib>
#include <sys/time.h>

class DecisionMaker {
private:
std::vector<std::vector<float>> occ_values;
std::vector<std::vector<float>> noise_values;
double center_frequency;
double previous_center_frequency;
public:
  DecisionMaker();
  ~DecisionMaker();
  std::vector<float> getDecision(std::vector<float> occ_vector, double center_frequency,std::vector<float> noise_vector);
  double get_previous_center_frequency();
  std::vector<float> average();
  std::vector<float> max_occ();
  std::vector<float> noise_average();
  double startTime;
  double currentTime;
  bool frequency_change = false;
  int times_up;
  int values_thrown;
  int nodeID;
};
