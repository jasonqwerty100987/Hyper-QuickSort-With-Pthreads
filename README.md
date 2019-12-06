# Hyper-QuickSort With Pthreads
 Implemantation of Hyper-QuickSort, designed for MPI, with Pthreads
## Notes
- To compile this code, use gcc -pthread -HyperQuickSort.c -o -HyperQuickSort -lm, and use ./HyperQuickSort to run the program
- Currently, on my local machine, more than 8 threads will cause machine crash. Different machine may have a different result.
- The memory access overhead is huge due to the limitation of pthread and the nature of Hyper-QuickSort (Designed for MPI rather than pthread) // will be improved in the future iteration
- [Link](https://github.com/Minokis/MPI-Quicksort-Hypercubes) to the MPI implemantation of Hyper-QuickSort by [Minokis](https://github.com/Minokis) who has no connection with me.
- [Link](https://www.uio.no/studier/emner/matnat/ifi/INF3380/v10/undervisningsmateriale/inf3380-week12.pdf) to the illustration of Hyper-QuickSort (Page 11).
- For the uses of each functions, please view the Illustation.txt file.