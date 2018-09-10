#include "decision_maker.h"
#include <stdio.h>
#include <iostream>

DecisionMaker::DecisionMaker(){
  center_frequency = 0;
}

DecisionMaker::~DecisionMaker(){
  occ_values.clear();
}

float DecisionMaker::getDecision(float occ, double center_frequency){

  if(occ_values.size()==0) {
    struct timeval ts;
    gettimeofday(&ts, NULL);
    startTime = ts.tv_sec;
  }

  if(this->center_frequency == 0) {
    this->center_frequency = center_frequency;
    this->previous_center_frequency = center_frequency;
  }
  if(center_frequency != this->center_frequency) {
    frequency_change = true;
    printf("Frequency Changed\n");
  }
  if(!frequency_change){
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int currentTime  = tp.tv_sec;
    double secondsPassed = currentTime - startTime;
    //printf("secondsPassed: %f\n", secondsPassed);

    if(secondsPassed < 1){
      occ_values.push_back(occ);
      //printf("OCC SIZE:%lu\n",occ_values.size());
      return -1;
    } else{
      printf("Times Up\n");
    }
  }
  previous_center_frequency = this->center_frequency;
  //printf("Changed Frequency: %f\n",this->center_frequency);
  //std::cout << "Changed Frequency " << this->center_frequency << std::endl;
  this->center_frequency = center_frequency;
  float result = max_occ();
  //float result = average();
  frequency_change = false;
  return result;

}

float DecisionMaker::average(){
  float result = 0;
  for(int i = 0; i < occ_values.size(); i++) {
    result = result + occ_values.at(i);
  }
  result = result/occ_values.size();
  occ_values.clear();
  return result;
}

float DecisionMaker::max_occ(){
  float result = 0;
  for(int i = 0; i < occ_values.size(); i++) {
    if(occ_values[i]>result){
      result = occ_values[i];
    }
    //result = result + occ_values.at(i);
  }
  //result = result/occ_values.size();
  occ_values.clear();
  return result;
}

double DecisionMaker::get_previous_center_frequency(){
  return previous_center_frequency;
}
