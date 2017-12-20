#include "loop.hpp"

Loop::Loop()
{
  sem_init(&mutex,0,1);

  fd_shm = shm_open("/shared_loop", O_CREAT | O_RDWR, 0777);
  if (fd_shm == -1)
  {
    perror("failed to create shared memory\n");
  }
  int status = ftruncate(fd_shm, sizeof(shared_struct));
  if (status == -1)
  {
    perror("Error Occured while truncating memory\n");
  }

  shared = (shared_struct *)mmap(NULL, sizeof(shared_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);

}

Loop::~Loop()
{

}


void Loop::transmit(std::vector<std::complex<float>> usrp_buffer,int buffer_len){
  //char *frame = new char[MAX_BUF];
  //int frame_len = buffer_len*sizeof(std::complex<float>);
  int sval;
  sem_getvalue(&mutex, &sval);
  //printf("Current TX semaphore Value: %d\n", sval);
  sem_wait(&mutex);
  shared->num_samples = buffer_len;
  memcpy(&shared->data[0],&usrp_buffer[0],buffer_len*sizeof(std::complex<float>));
  /*
  for(int i = 0; i < buffer_len; i++){
    shared->data[i] = usrp_buffer[i];
  }
  */
  //sem_post(&mutex);
  //delete[] frame;
}

int Loop::receive(std::vector<std::complex<float> > &recv){
  //sem_wait(&mutex);

  //if(sem_trywait(&mutex) == 0){
    int sval;
    sem_getvalue(&mutex, &sval);

    if(sval > 0){
      return 0;
    }
    //printf("Current RX semaphore Value: %d\n", sval);
    if(shared->data[0] != std::complex<float>(1000, -1000)){
      memcpy(&recv[0],&shared->data[0],shared->num_samples*sizeof(std::complex<float>));
      shared->data[0] = std::complex<float>(1000, -1000);
      if(sval < 1){
        sem_post(&mutex);
      } 
    return shared->num_samples;
    } else{
      if(sval < 1){
        sem_post(&mutex);
      } 
      return 0;
    }

  //} else{
  //  return 0;
  //}

}

void Loop::manipulate_header(char *payload){
  //printf("\n----------------Manipulating data-------------------\n");
  // mac address change
  char temp = payload[23];
  payload[23] =  payload[17];
  payload[17] = temp;
  //  printf("%02x --- %02x\n",payload[17] ,payload[23]);
  // ipaddress change

  temp = payload[45];
  payload[45] =  payload[41];
  payload[41] = temp;
  //  printf("%02x --- %02x\n",payload[41] ,payload[45]);
  /*
  for(int i = 0; i < 44; i++){
  printf("%02x ",payload[i]);
}
printf("\n");
printf("\n----------------Done Manipulating data-------------------\n");
*/
  temp = payload[48];
  payload[48] =  payload[46];
  payload[46] = temp;

  temp = payload[49];
  payload[49] =  payload[47];
  payload[47] = temp;
}
