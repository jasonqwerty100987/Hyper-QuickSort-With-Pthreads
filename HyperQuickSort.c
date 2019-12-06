#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#define NUM_THREADS  2
#define length(x) (sizeof(x)/sizeof((x)[0]))


pthread_barrier_t mybarrier;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
enum direction{up,down};
int *increments;
int *midindex_ptr;
int *pivot;
int *attribution;

int partition (int *arr, int low, int high)
{
    int pivot = *(arr+high);    // pivot
    int i = (low - 1);  // Index of smaller element

    for (int j = low; j <= high- 1; j++)
    {
        // If current element is smaller than the pivot
        if (*(arr+j) < pivot)
        {
            i++;    // increment index of smaller element
            int temp = *(arr+i);
            *(arr+i) = *(arr+j);
            *(arr+j) = temp;
        }
    }
    int temp = *(arr+i+1);
    *(arr+i+1) = *(arr+high);
    *(arr+high) = temp;
    return (i + 1);
}
// sequential quicksort implementation
void quickSort(int *arr, int low, int high)
{
    if (low < high)
    {
        /* pi is partitioning index, arr[p] is now
           at right place */
        int pi = partition(arr, low, high);

        // Separately sort elements before
        // partition and after partition
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

typedef struct sort_info
{
    int *data;
    int pid;
} sort_info;

// prototype of functions
void *thread_quicksort(void *arg);
void swap(int pid, int iteration ,int *local_array, int *global_array, enum direction dir);

int main()
{
    int threads = NUM_THREADS/1;
    clock_t t;
    int n = 2000000; // size of array which is need to be sorted
    int a[n];
    for(int i = 0; i < n; i++){
      a[i] = rand()%1000;
    }
    int *array = malloc(sizeof(int)*n);
    int pid[NUM_THREADS];
    for(int i = 0; i < n; i++){
      *(array+i) = a[i];
    }
    for(int i = 0; i < NUM_THREADS; i++){
	     pid[i] = i;
    }
    int b[NUM_THREADS*3];
    attribution = b;
    int something[NUM_THREADS];
    midindex_ptr = something;
    pthread_barrier_init(&mybarrier,NULL,NUM_THREADS);
    int something2[(int)log2(NUM_THREADS)];
    increments = something2;
    int temp = (int)(log2(NUM_THREADS));
    for(int i = 1; i <= sizeof(something2)/ sizeof(int); i++){
      *(increments + i) = (pow(pow(2,log2(NUM_THREADS)-i),-1))*NUM_THREADS;
      temp /= 2;
    }
    for(int i = 0; i < NUM_THREADS; i++){
        *(attribution+(i*3)) = i*length(a)/NUM_THREADS; // begining index
        *(attribution+(i*3)+1) = (i+1)*length(a)/NUM_THREADS-1; // ending index
        *(attribution+(i*3)+2) = (i+1)*length(a)/NUM_THREADS-1 - i*length(a)/NUM_THREADS + 1; // length
    }

    pthread_t thread[NUM_THREADS];
    pivot = malloc(sizeof(int)*(NUM_THREADS-1));
    sort_info info[NUM_THREADS];
    for(int i = 0; i < (NUM_THREADS); ++i){
        info[i].data = array;
        info[i].pid = pid[i];
    }
    t = clock();
    for(int i = 0; i < NUM_THREADS; i++){
      pthread_create(&thread[i],NULL,&thread_quicksort,&info[i]);
    }
    for(int i = 0; i < NUM_THREADS; i++){
      pthread_join(thread[i],NULL);
    }
    t = clock() - t;
    double time = (double)t/CLOCKS_PER_SEC;
    printf("Time takes to sort %d elements is %f seconds wtih %d threads\n", n,time, threads);
    return 1;
}

int getposition(int pid, int iteration)// a function used to determine which index each thread should use to put or get their cutoff index. The cutoff index is applied to the local_array in each threads.
{
    int counter = 0;
    int startpostion = 0;
    for(int i = (int)(log2(NUM_THREADS)); i > iteration; i--){
        startpostion += (int)pow(2,(int)log2(NUM_THREADS)-i);

    }
    for(int i = 0; i < NUM_THREADS; i+=*(increments + iteration)){
        if(pid == i){
            return (startpostion + counter);
        }else if(pid > i && pid < i+*(increments + iteration)){
            return (startpostion + counter);
        }
        counter++;
    }
    return counter;
}

void *thread_quicksort(void *arg)
{
    struct sort_info *si = (struct sort_info*)arg;
    int isSwaped;
    int *global_array = si->data;
    int pid = si->pid;
    int *local_array = malloc(sizeof(int)*(*(attribution + pid*3 + 1)-*(attribution + pid*3)+1));
    // initialize local array
    for(int i = 0; i < *(attribution + pid*3 + 2); i++){
      *(local_array + i) = *(global_array + i + *(attribution + pid*3));
    }
    int iteration = (int)log2(NUM_THREADS);
    pthread_barrier_wait(&mybarrier);

    // start iteration
    while(iteration >0){
      isSwaped = 0;
      int begin_index = *(attribution + pid*3);
      int end_index = *(attribution + pid*3 + 1);
      int length = *(attribution + pid*3 + 2);
       // sequential quickSort on each local array
      quickSort(local_array,0,length - 1);
        // update local_array to global _array;
      for(int i = 0; i < length;i++){
       *(global_array + begin_index + i) = *(local_array + i);
      }
      //broadcast median value
      int postion = getposition(pid, iteration);
      for(int i = 0; i < NUM_THREADS; i+=*(increments+iteration)) {
        if(pid == i){
          *(pivot + postion) = *(local_array + (length/2));
        }
      }
      pthread_barrier_wait(&mybarrier); // wait until every thread get its pivot value
      // division of "low list" and "high list"
      int midindex = (length)/2;
      // Each process divides its list into two lists: those smaller than (or equal) the pivot, those greater than the pivot
      // midindex is the index where seperate the whole list into two sub list.
      while(1){
          if(*(local_array + midindex) > *(pivot + postion) && *(local_array + midindex - 1) <= *(pivot + postion) || midindex == 0 || midindex == length){
              break;
          }else if(*(local_array + midindex) > *(pivot + postion)){
              midindex--;
          }else{
              midindex++;
          }
      }
      // broadcast its local midindex to all other thread to do the swap operation
      *(midindex_ptr + pid) = midindex;
      pthread_barrier_wait(&mybarrier);
      int starting_index_of_old_local; // starting index of lower_list
      int ending_index_of_old_local; // ending index of lower_list
      int starting_index_of_another_thread; // starting index of higher_list in higher thread
      int ending_index_of_another_thread; // ending index of higher_list in higher thread
      if(iteration == (int)log2(NUM_THREADS) && pid < NUM_THREADS/2){
        starting_index_of_old_local = *(attribution + pid*3); // starting position is the starting index stored in attribution array
        ending_index_of_old_local = starting_index_of_old_local + *(midindex_ptr + pid) - 1; // ending position is the cutoff - 1
        starting_index_of_another_thread = *(attribution + (pid + iteration)*3); // starting index of higher thread's local_array in the global array scope
        ending_index_of_another_thread = starting_index_of_another_thread + *(midindex_ptr + pid + iteration) - 1; // the ending postion is the ending index of the lower thread's local_array in the global array scope
        int size_of_new_local = ending_index_of_old_local - starting_index_of_old_local + ending_index_of_another_thread - starting_index_of_another_thread + 2;   // size of new local array
        free(local_array);
        local_array = malloc(sizeof(int) * size_of_new_local); // assign new local array
        swap(pid, iteration, local_array, global_array, up);
        isSwaped = 1;
      }else if(iteration < (int)log2(NUM_THREADS)){
        for(int i = 0; i * iteration < NUM_THREADS; i+=2){
            if( pid >= i * iteration && pid < (i+1) * iteration){
              starting_index_of_old_local = *(attribution + pid*3); // starting position is the starting index stored in attribution array
              ending_index_of_old_local = starting_index_of_old_local + *(midindex_ptr + pid) - 1; // ending position is the cutoff - 1
              starting_index_of_another_thread = *(attribution + (pid + iteration)*3); // starting index of higher thread's local_array in the global array scope
              ending_index_of_another_thread = starting_index_of_another_thread + *(midindex_ptr + pid + iteration) - 1; // the ending postion is the ending index of the lower thread's local_array in the global array scope
              int size_of_new_local = ending_index_of_old_local - starting_index_of_old_local + ending_index_of_another_thread - starting_index_of_another_thread + 2;   // size of new local array
              free(local_array);
              local_array = malloc(sizeof(int) * size_of_new_local); // assign new local array
              swap(pid, iteration, local_array, global_array, up);
              isSwaped = 1;
            }
        }
      }
      if(isSwaped == 0){ // if it is not the thread which broadcast its higher list to other thread, then receive array.
        starting_index_of_old_local = *(attribution + pid*3) + *(midindex_ptr + pid); // starting index of local array is the cutoff point + starting index in global array
        ending_index_of_old_local = *(attribution + pid*3 + 1); // ending index of at global array
        starting_index_of_another_thread = *(attribution + (pid - iteration)*3) + *(midindex_ptr + pid - iteration); // starting index of lower thread's local_array in the global array scope + the cutoff index of the lower threads
        ending_index_of_another_thread = *(attribution + (pid - iteration)*3 + 1);
        int size_of_new_local = ending_index_of_old_local - starting_index_of_old_local + ending_index_of_another_thread - starting_index_of_another_thread + 2;   // size of new local array
        free(local_array);
        local_array = malloc(sizeof(int) * size_of_new_local); // assign new local array
        swap(pid, iteration, local_array, global_array, down);
        isSwaped = 1;
      }
      pthread_barrier_wait(&mybarrier); // wait until all processes is ready for the next iteration
      iteration--;
      pthread_barrier_wait(&mybarrier); // wait until all processes is ready for the next iteration
    } // end of while loop
    // update the latest local_array into the global_array
    int length = *(attribution + pid*3 + 2);
    int begin_index = *(attribution + pid*3);
    quickSort(local_array,0,length - 1);
    for(int i = 0; i < length;i++){
      *(global_array + begin_index + i) = *(local_array + i);
    }
    pthread_barrier_wait(&mybarrier); // wait until all processes is ready for the next iteration
    return NULL;
}
// cutoff index is the index of where local_array[cutoff] > pivot
// after sort, each thread should merge their local_array into the global array to make sure swap() works properly
void swap(int pid, int iteration ,int *local_array, int *global_array, enum direction dir){
  int starting_index_of_old_local; // starting index of lower_list
  int ending_index_of_old_local; // ending index of lower_list
  int starting_index_of_another_thread; // starting index of higher_list in higher thread
  int ending_index_of_another_thread; // ending index of higher_list in higher thread
  // all indexs is in global_array scope
  if(dir == up){ // condition true if the threads need to broadcast its higher_list to a thread which has a higher pid
    starting_index_of_old_local = *(attribution + pid*3); // starting position is the starting index stored in attribution array
    ending_index_of_old_local = starting_index_of_old_local + *(midindex_ptr + pid) - 1; // ending position is the cutoff - 1
    starting_index_of_another_thread = *(attribution + (pid + iteration)*3); // starting index of higher thread's local_array in the global array scope
    ending_index_of_another_thread = starting_index_of_another_thread + *(midindex_ptr + pid + iteration) - 1; // the ending postion is the ending index of the lower thread's local_array in the global array scope
  }else if(dir == down){ // condition true if the threads need to broadcast its lower_list to a thread which has a lower pid
    starting_index_of_old_local = *(attribution + pid*3) + *(midindex_ptr + pid); // starting index of local array is the cutoff point + starting index in global array
    ending_index_of_old_local = *(attribution + pid*3 + 1); // ending index of at global array
    starting_index_of_another_thread = *(attribution + (pid - iteration)*3) + *(midindex_ptr + pid - iteration); // starting index of lower thread's local_array in the global array scope + the cutoff index of the lower threads
    ending_index_of_another_thread = *(attribution + (pid - iteration)*3 + 1); // the ending postion is the ending index of the lower thread's local_array in the global array scope
  }
  int size_of_new_local = ending_index_of_old_local - starting_index_of_old_local + ending_index_of_another_thread - starting_index_of_another_thread + 2;

  if(dir == up){ // condition true if the threads need to broadcast its higher_list to a thread which has a higher pid
    // fill in with old local array, and then fill in with the received array
    int k = 0;
    for(int i = starting_index_of_old_local; i <= ending_index_of_old_local; i++){ // copy elements from old local array
      *(local_array + k++) = *(global_array + i);
    }
    for(int i = starting_index_of_another_thread; i <= ending_index_of_another_thread; i++){ // copy elements from global array
      *(local_array + k++) = *(global_array + i);
    }
  }else if(dir == down){ // condition true if the threads need to broadcast its lower_list to a thread which has a lower pid
    // fill in the new local array with the elements in another thread first, and then fill in the new local array with the elements from the old local array
    int k = 0;
    for(int i = starting_index_of_another_thread; i <= ending_index_of_another_thread; i++){
      *(local_array + k++) = *(global_array + i);
    }
    for(int i = starting_index_of_old_local; i <= ending_index_of_old_local; i++ ){ // copy elements from old array
      *(local_array + k++) = *(global_array + i);
    }
  }
  pthread_barrier_wait(&mybarrier); // wait until all thread has their new local array
  // swap complete
  // the following code is used to update the new attribution matrix
  *(attribution + pid*3 + 2) = size_of_new_local;
  pthread_barrier_wait(&mybarrier); // wait until all thread updated their length of local _array
  int begin_index = 0;
  for(int i = 0; i < pid; i++){
    begin_index += *(attribution + i*3 + 2);
  }
  *(attribution + pid*3) = begin_index; // update begin index
  *(attribution + pid*3 + 1) = begin_index + *(attribution + pid*3 + 2)-1; // update endindex
  // update complete
}
