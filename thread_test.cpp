#include <iostream>
#include <string.h>
#include <fstream>
#include "BufferQ.h"


struct Con{
    BufferQ<int> *myQueue;
    int consumer;
};

void *consume(void *_arg){
    Con *y = (Con *) _arg;
    BufferQ<int> *myQueue = (BufferQ<int> *) y->myQueue;
    std::cout << "Consumer Thread: " << y->consumer << "\n";
    int count = 0;
    std::string file = "thread_"+std::to_string(y->consumer);
    std::cout << "Filename: " << file << "\n";
    std::ofstream out(file.c_str());
    while(count < 1000){   
        int s = 0;
        int *x = myQueue->dequeue(y->consumer,s);
        if(x!=NULL){
            if(x[0]-count !=0){
            //std::cout << "mismatch: " << x[0]-count << "\n";
            }
            out << x[0]-count << "\n";
            //delete [] x;
        }

        count++;
    }
    out.close();

}

void *produce(void *_arg){
    BufferQ<int> *myQueue = (BufferQ<int> *) _arg;
    int count = 0;
    while(count < 1000){
        //std::cout << "Producer: " << count << "\n";
        int p[3] = {10,1,3};
        p[0] = count;
        myQueue->enqueue(p,3);
        count++;
    }
    std::cout << "Done Producing \n" ;
    myQueue->stopConsumers();
}


int main(){
    const int consumers = 2;
    BufferQ<int> myQueue(10000,consumers);

    pthread_t thread1;
    pthread_t threads[consumers];

    Con x[consumers];
    
    int status = pthread_create(&thread1,NULL,produce,(void *) &myQueue);
    for(int i = 0; i < consumers;i++){
        x[i].myQueue = &myQueue;
        x[i].consumer = i;
        status = pthread_create(&threads[i],NULL,consume,(void *) &x[i]);
    }
    

    pthread_join(thread1,NULL);
    for(int i = 0; i < consumers;i++){
        pthread_join(threads[i],NULL);
    }
    
} 