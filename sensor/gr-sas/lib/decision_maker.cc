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
  if(this->center_frequency == 0){
    //printf("Starting Up\n");
    occ_values.push_back(occ);
    this->center_frequency = center_frequency;
    previous_center_frequency = this->center_frequency;
    return -1;
  } else{
    if(this->center_frequency==center_frequency){
      //printf("Same Frequency: %f\n",this->center_frequency);
      occ_values.push_back(occ);
      return -1;
    } else{
      previous_center_frequency = this->center_frequency;
      //printf("Changed Frequency: %f\n",this->center_frequency);
      std::cout << "Changed Frequency " << this->center_frequency << std::endl;
      this->center_frequency = center_frequency;
      float result = average();
      occ_values.push_back(occ);
      return result;
    }
  }
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


float DecisionMaker::max(){
  float result = 0;
  for(int i = 0; i < occ_values.size(); i++) {
    if(occ_values.at(i)>result)
    result = occ_values.at(i);
  }
  occ_values.clear();
  return result;
}

double DecisionMaker::get_previous_center_frequency(){
  return previous_center_frequency;
}
