#include <vector>

class DecisionMaker {
private:
std::vector<float> occ_values;
float center_frequency;
float previous_center_frequency;
public:
  DecisionMaker();
  ~DecisionMaker();
  float getDecision(float occ, float center_frequency);
  float get_previous_center_frequency();
  float average();
};
