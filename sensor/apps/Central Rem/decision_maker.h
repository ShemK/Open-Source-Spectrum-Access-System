#include <vector>
#include <ctime>
#include <cstdlib>
#include <sys/time.h>

class DecisionMaker {
private:
std::vector<float> occ_values;
double center_frequency;
double previous_center_frequency;
public:
  DecisionMaker();
  ~DecisionMaker();
  float getDecision(float occ, double center_frequency);
  double get_previous_center_frequency();
  float average();
  float max_occ();
  long int startTime;
  long int currentTime;
  bool frequency_change = false;
};
