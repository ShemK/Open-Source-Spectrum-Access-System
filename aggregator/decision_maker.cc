#include "decision_maker.h"
#include <stdio.h>
#include <iostream>

DecisionMaker::DecisionMaker(){
  center_frequency = 0;
  nodeID = 0;
}

DecisionMaker::~DecisionMaker(){
  occ_values.clear();
}

std::vector<float> DecisionMaker::getDecision(std::vector<float> occ_vector, double center_frequency){

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

    if(secondsPassed < 0.2){
      occ_values.push_back(occ_vector);
      //printf("OCC SIZE:%lu\n",occ_values.size());
      std::vector<float> temp;
      return temp;
    } else{
      times_up++;
      printf("Times Up\n");
    }
  } else{
    times_up = 0;
  }
  previous_center_frequency = this->center_frequency;
  //printf("Changed Frequency: %f\n",this->center_frequency);
  //std::cout << "Changed Frequency " << this->center_frequency << std::endl;
  this->center_frequency = center_frequency;
  //float result = max_occ();
  std::vector<float> result = max_occ();//average();
  frequency_change = false;

  if(times_up > 5){
    printf("-------------------------------------------------------------\n");
    printf("----------Error with sensor %d----------------------------\n",nodeID);
    printf("-------------------------------------------------------------\n");
  }
  return result;
}

std::vector<float> DecisionMaker::average(){
  std::vector<float> result;
  if(occ_values.size() > 0){
    int size = occ_values.at(0).size();
    for(int j = 0; j < size; j++) {
      float temp_result = 0;
      for(int i = 0; i < occ_values.size(); i++){
        //std::cout << "Value Size :" << occ_values.at(i).size() << "\n";
        temp_result = temp_result + occ_values.at(i).at(j);
      }
      temp_result = temp_result/occ_values.size();
      result.push_back(temp_result);
    }
    //std::cout << "Hello " << "\n";
    //std::cout << "Result Size: " << result.size() << "\n";
  }
  occ_values.clear();
  return result;
}

std::vector<float> DecisionMaker::max_occ(){
  std::vector<float> result;
  if(occ_values.size() > 0){
    int size = occ_values.at(0).size();
    for(int j = 0; j < size; j++) {
      float temp_result = 0;
      for(int i = 0; i < occ_values.size(); i++){
        //std::cout << "Value Size :" << occ_values.at(i).size() << "\n";
        if(occ_values.at(i).at(j)>temp_result){
          temp_result = occ_values.at(i).at(j);
        }
      }
      result.push_back(temp_result);
    }
  }
  occ_values.clear();
  return result;
}

double DecisionMaker::get_previous_center_frequency(){
  return previous_center_frequency;
}
