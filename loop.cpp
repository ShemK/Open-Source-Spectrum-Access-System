#include "loop.hpp"

Loop::Loop()
{
  theQ = new BufferQ<std::complex<float>>(100,1);
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
  delete theQ;
}


void Loop::transmit(std::vector<std::complex<float>> usrp_buffer,int buffer_len){

  theQ->enqueue(&usrp_buffer[0],buffer_len);

}

int Loop::receive(std::vector<std::complex<float> > &recv){

  int size = 0;
  std::complex<float> *x = theQ->dequeue(0,size,false);
  if(size > 0){
    memcpy(&recv[0],x,size*sizeof(std::complex<float>));
  }
  
  return size;
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
