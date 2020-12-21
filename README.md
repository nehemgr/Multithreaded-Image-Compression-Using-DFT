# Multithreaded-Image-Compression-Using-DFT
This project focuses on building an image compressor executable that implements multithreading to leverage the system's resources, and was implemented in Linux using C. This is done in a thread-safe and data-safe manner by the use of appropriate mutexes. The user enters the total number of threads the system must use, the compression factor (a value between 1 to 10) and the source and destination image file names. A separate file is included that performs exactly the same, but sequentially and the comparisons are provided. The focus here is only to multithread the calculation of the DFTs of smaller 16x16 sub-images, and hence the DFT calculations were implemented using the existing FFTW libraries.

## Running the Project
Make sure you are running the code in Linux. In the terminal run the executable using the syntax:\
```
./img_prj <number_of_threads> <compression_factor [1-10] <input_img.jpg> <output_img.jpg>
```
* The number of threads can be any positive integer. Do note that giving this parameter as 1, does not mean that this implements sequential code. This is because, always N+1 threads will be used (where N is the entered parameter). The implicit thread will be used for copying the pixels from the source to the destination image.
* The compression factor must be any integer between 1 and 10, where 1 implies that the image is not compressed at all and 10 means that it is compressed at maximum. 
* The input and output filetypes must be either jpg or jpeg.

## Project Demo
![alt text](<https://github.com/nehemgr/Multithreaded-Image-Compression-Using-DFT/blob/main/Report/sample_run.jpg>)

![alt text](<https://github.com/nehemgr/Multithreaded-Image-Compression-Using-DFT/blob/main/Report/mountains.jpg>)
Original File - mountains.jpg, 1920x1080, 288.8 KB

![alt text](<https://github.com/nehemgr/Multithreaded-Image-Compression-Using-DFT/blob/main/Report/mountains_new.jpg>)
New File - mountains_new.jpg, 1920x1080, Compression Factor 5, 123.5 KB

## Results
The testing of the times were done on a freshly booted system, with no other applications open, to ensure that the program would have complete access to all the 4 processor cores of the machine. These are summarized in the graph below. The point corresponding to threads as zero, is the code without multithreading.

![alt text](<https://github.com/nehemgr/Multithreaded-Image-Compression-Using-DFT/blob/main/Report/result_graph.JPG>)

The degree of compression in the file size is tabulated below for the aforementioned image mountains.jpg.

![alt text](<https://github.com/nehemgr/Multithreaded-Image-Compression-Using-DFT/blob/main/Report/result_table.JPG>)
