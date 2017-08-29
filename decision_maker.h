#include <vector>
#include <ctime>
#include <cstdlib>
#include <sys/time.h>

class DecisionMaker {
private:
std::vector<std::vector<float>> occ_values;
double center_frequency;
double previous_center_frequency;
public:
  DecisionMaker();
  ~DecisionMaker();
  std::vector<float> getDecision(std::vector<float> occ_vector, double center_frequency);
  double get_previous_center_frequency();
  std::vector<float> average();
  std::vector<float> max_occ();
  long int startTime;
  long int currentTime;
  bool frequency_change = false;
};
