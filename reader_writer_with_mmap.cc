/* Works well with C++11 compiler. Include -lpthread while compiling*/


#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <pthread.h>
#include <iostream>
using namespace std;

#define SHM_FILE "/sai_harish_shm"

pthread_t thread1,thread2;
int flag = 0;

typedef class my_wrapper_class
{
  public:
  pthread_cond_t conda,condb;
  pthread_mutex_t mutex;
  int *t;
  my_wrapper_class()
  {
    conda = PTHREAD_COND_INITIALIZER;
    condb = PTHREAD_COND_INITIALIZER;
    mutex = PTHREAD_MUTEX_INITIALIZER;

    cout<<"object created"<<endl;
  }
  ~my_wrapper_class()
  {
    //int *y = t;
    //cout<<*y<<endl;
    cout<<"Done"<<endl;
  }
}wrapper;

wrapper obj;

void lg(const char *oper, int result) {
  printf("%s %d\n", oper, result);
  if (result < 0) {
    perror(oper);
  }
}

void* child_thread_func(void* arg)
{
  int i=0;
  //cout<<"Inside child thread"<<endl;
  for (i = 1;i<1000; i++) {
     usleep(5);
    pthread_mutex_lock(&obj.mutex); /* protect buffer */
    //cout<<"Inside child loop "<<endl;
    
    if(*obj.t == 0)     
      {
      //cout<<"Inside child loop while "<<endl;       
      pthread_cond_wait(&obj.condb, &obj.mutex);
      //cout<<"Inside child loop while "<<endl; 
    }
    //temp = *obj.t;
    
    printf("Child logs : %d\n", *obj.t);
    *obj.t = 0;
    
    pthread_mutex_unlock(&obj.mutex); /* release the buffer */
    pthread_cond_signal(&obj.conda);  /* wake up consumer */
  }


  return NULL;
}

void* parent_thread_func(void* arg)
{
  int i=0;
  //cout<<"Inside parent thread"<<endl;
  for (i = 1; i <= 1000; i++) {
    usleep(5); 
    //cout<<"Inside parent loop "<<endl;
    pthread_mutex_lock(&obj.mutex); 
    if(*obj.t != 0)       
      pthread_cond_wait(&obj.conda, &obj.mutex);
    *obj.t = i;
    
    printf("Parent increments to : %d\n", *obj.t);
    
    pthread_mutex_unlock(&obj.mutex);
    pthread_cond_signal(&obj.condb); 
    
  }
  return NULL;
}



void child(int *result) {

  pthread_create(&thread1,NULL,&child_thread_func,NULL);
  pthread_join(thread1,NULL);
  /*for (int i = 0; i < 5; ++i) {
    //strcpy(result, "child ::: hello parent\n");
    usleep(2);
    printf("child ::: %d\n", *result);
  }
  usleep(5);*/
}

void parent(int *result) {
  
  pthread_create(&thread2,NULL,&parent_thread_func,NULL);
  pthread_join(thread2,NULL);


 
}

int main() {
  int integerSize = sizeof(wrapper); 
  
  int descriptor = -1; 
  int mmap_flags = MAP_SHARED;

#ifdef SHM
  // Open the shared memory.
  descriptor = shm_open(SHM_FILE, 
      O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

  // Size up the shared memory.
  ftruncate(descriptor, integerSize);
#else
  //descriptor = -1;
  descriptor = creat("/dev/zero", S_IRUSR | S_IWUSR);
  mmap_flags |= MAP_ANONYMOUS;
#endif
  int *result = (int*)mmap(NULL, integerSize, 
      PROT_WRITE | PROT_READ, mmap_flags, 
      descriptor, 0 );

  

  perror("mmap");
  //printf("%X\n", result);

  obj.t = result;


  pid_t child_pid = fork();

  switch(child_pid) {
    case 0:
      child(obj.t);
      break;
    case -1:
      lg("fork", -1);
      break;
    default:
      parent(obj.t);
  }
  //usleep(5000000);
  lg("msync", msync(result, integerSize, MS_SYNC));
  lg("munmap", munmap(result, integerSize));

  

#ifdef SHM
  if (child_pid != 0) {
    lg("shm_unlink", shm_unlink(SHM_FILE));
  }
#endif

  return 0;
}
