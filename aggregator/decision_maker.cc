#include "decision_maker.h"
#include <stdio.h>
#include <iostream>

DecisionMaker::DecisionMaker(){
  center_frequency = 0;
  nodeID = 0;
  values_thrown = 0;
}

DecisionMaker::~DecisionMaker(){
  occ_values.clear();
  noise_values.clear();
}

std::vector<float> DecisionMaker::getDecision(std::vector<float> occ_vector, double center_frequency,std::vector<float> noise_vector){
  std::cout << "---------------------" << nodeID << "------------------------\n";
  if(occ_values.size()==0) {
    struct timeval ts;
    gettimeofday(&ts, NULL);
    startTime = ts.tv_sec + ts.tv_usec/1e6;
  }

  if(this->center_frequency == 0) {
    this->center_frequency = center_frequency;
    this->previous_center_frequency = center_frequency;
  }
  if(center_frequency != this->center_frequency) {
    frequency_change = true;
    printf("Frequency Changed\n");
    values_thrown = 0;
  }
  if(!frequency_change){
    struct timeval tp;
    gettimeofday(&tp, NULL);
    double currentTime  = tp.tv_sec + tp.tv_usec/1e6;
    double secondsPassed = currentTime - startTime;
    //printf("secondsPassed: %f\n", secondsPassed);
    values_thrown++; // values thrown away when the frequency_change happens
    if(secondsPassed < 0.05){
      if(values_thrown > 5){ // 5 values are thrown away
        occ_values.push_back(occ_vector);
        noise_values.push_back(noise_vector);
      }
      printf("secondsPassed: %f\n",secondsPassed );
      printf("values thrown: %d\n",values_thrown );
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
    std::fill(result.begin(), result.end(), 0);
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

std::vector<float> DecisionMaker::noise_average(){
  std::vector<float> result;
  if(noise_values.size() > 0){
    int size = noise_values.at(0).size();
    for(int j = 0; j < size; j++) {
      float temp_result = 0;
      for(int i = 0; i < noise_values.size(); i++){
        //std::cout << "Value Size :" << occ_values.at(i).size() << "\n";
        temp_result = temp_result + noise_values.at(i).at(j);
      }
      temp_result = temp_result/noise_values.size();
      result.push_back(temp_result);
    }
    //std::cout << "Hello " << "\n";
    //std::cout << "Result Size: " << result.size() << "\n";
  }
  noise_values.clear();
  return result;
}

std::vector<float> DecisionMaker::max_occ(){
  std::vector<float> result;
  printf("OCC SIZE:%lu\n",occ_values.size());
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
