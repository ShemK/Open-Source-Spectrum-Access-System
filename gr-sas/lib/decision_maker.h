#include <vector>

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
  float max();
};
