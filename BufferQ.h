#ifndef BUFFERQ_H
#define BUFFERQ_H

#include <iostream>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <complex>
#include <vector>

template<class T>
class BufferQ{
private:
    int q_size;
    int consumers;

    pthread_cond_t cond;
    pthread_mutex_t consumer_mutex;

    std::vector<sem_t> phores;
    //sem_t *phores;
    pthread_mutex_t* mutices;

    int *mutex_bool;
    bool addingConsumer;
public:
   
    BufferQ(int q_size, int consumers);
    ~BufferQ();

    void enqueue(T *input,int size);
    T* dequeue(int consumer,int &size, bool wait = true);
    void printData(T *input, int size);
    void stopConsumers();
    void addConsumer();

    struct Slot{
        int rank;
        int size;
        T *data;
    };

    Slot *slots;
    int *head;
    int tail;
    bool consumer_stopped;
};

/// DEFINITIONS - Problem with templates having definitions in different files

template<class T>
BufferQ<T>::BufferQ(int q_size, int consumers){
    this->q_size = q_size;
    this->consumers = consumers;
    slots = new Slot[q_size];
    memset(slots,0,q_size*sizeof(Slot));
    head = new int[consumers];
    memset(head,0,consumers*sizeof(int));
    tail = 0;

    int status = pthread_cond_init(&cond,NULL);
    if(status < 0){
       perror("Failed to initialize condition\n");
    }
    status = pthread_mutex_init(&consumer_mutex,NULL);
    if(status < 0){
        perror("Failed to initialize mutex\n");
    }
    mutices = new pthread_mutex_t[q_size];
    mutex_bool = new int[q_size];
    for(int i = 0; i < q_size; i++){
        sem_t newPhore;
        phores.push_back(newPhore);
        sem_init(&phores[i],0,0);
        status = pthread_mutex_init(&mutices[i],NULL);
        mutex_bool[i] = 0;
    }
    consumer_stopped = false;
    addingConsumer = false;
}

template<class T>
BufferQ<T>::~BufferQ(){
    for(int i = 0; i < q_size; i++){
        delete slots[i].data;
    }
    delete []slots;
    delete []head;
    delete []mutices;
}

template<class T>
void BufferQ<T>::enqueue(T *input, int size){

    if(addingConsumer){
        pthread_mutex_lock(&mutices[tail]);

        pthread_mutex_lock(&mutices[tail]);
    }
    if(slots[tail].rank > 0){
        int p = 0;
        sem_getvalue(&phores[tail],&p);
        printf("Slot %d Full: %d - %d - %d\n",tail,slots[tail].rank,p,slots[tail].size);
        for(int i = 0; i < consumers;i++){
            sem_getvalue(&phores[head[i]],&p);
            std::cout << head[i] << "-" << p << "-" << mutex_bool[p] <<"\n";
        }
        usleep(1000);
        return;
    }
    // enqueue data at a tail
    if(slots[tail].size == 0){
        slots[tail].data = new T[size];
        slots[tail].size = size;
    }
    if(slots[tail].size!=size){
        delete [] slots[tail].data;
        slots[tail].data = new T[size];
        slots[tail].size = size;
    }
    memcpy(slots[tail].data,input,size*sizeof(T));
    slots[tail].rank = consumers;
    sem_post(&phores[tail]);

    int diff = 0;
    //std::cout << "Tail: " << tail << ": ";
    for(int i = 0; i < consumers-1; i++){
        //std::cout << tail-head[i] << ","; 
        int curr_diff = tail-head[i];
        if(curr_diff > diff ){
            diff = curr_diff;
        }
    }
    int curr_diff = tail-head[consumers-1];
    if(curr_diff > diff ){
        diff = curr_diff;
    }
    //std::cout << tail-head[consumers-1] << "\n";
    if(diff > q_size/10){
        std::cout << "Producer Faster than Consumers\n";
        usleep(10000);
    }
    
    
    //std::cout << "Unlocked\n";
    tail = (tail+1)%q_size;
}

template<class T>
T* BufferQ<T>::dequeue(int consumer, int &size,bool wait){
    if(consumer >= consumers){
        size = 0;
        return NULL;
    }
    if(consumer_stopped){
        size = -1;
        return NULL;
    }
    int curr_head = head[consumer];

    // check if there is new data
    if(slots[curr_head].rank == 0){
        if(!wait){
            int status = sem_trywait(&phores[curr_head]); 
            if(status < 0){
                size = 0;
                return NULL;
            }
        } else{
          sem_wait(&phores[curr_head]);
        }
        sem_post(&phores[curr_head]);
    }
    if(consumer_stopped){
        //pthread_exit(NULL);
        size = -1;
        return NULL;
    }
    // dequeue data at the head of the consumer
    size = slots[curr_head].size;
    
    pthread_mutex_lock(&mutices[curr_head]);
    mutex_bool[curr_head] = 1;
    slots[curr_head].rank--; // decrease the rank
    if(slots[curr_head].rank == 0){
        sem_trywait(&phores[curr_head]);
    }
    pthread_mutex_unlock(&mutices[curr_head]);
    head[consumer] = (head[consumer]+1)%q_size;
    
    mutex_bool[curr_head] = 0;
    return slots[curr_head].data;
    
}

template<class T>
void BufferQ<T>::printData(T *input, int size){
    int i = 0;
    for(i = 0; i < size-1; i++){
        std::cout << input[i] << ",";
    }
    std::cout << input[i] << "\n";
}
template<class T>
void BufferQ<T>::stopConsumers(){
    consumer_stopped = true;

    for(int i = 0; i < q_size; i++){
        sem_post(&phores[i]);
    }
}

// dynamic addition of consumer still in testing
template<class T>
void BufferQ<T>::addConsumer(){

}

#endif