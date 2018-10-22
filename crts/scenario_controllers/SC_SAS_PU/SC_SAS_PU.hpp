#ifndef _SC_SAS_PU_
#define _SC_SAS_PU_

#include "scenario_controller.hpp"

class SC_SAS_PU : public ScenarioController {

private:
  unsigned int debugLevel;
public:
  SC_SAS_PU(int argc, char **argv);
  ~SC_SAS_PU();
  virtual void execute();
  virtual void initialize_node_fb();
};

#endif
