# Multithreaded-Image-Compression-Using-DFT
This project focuses on building an image compressor executable that implements multithreading to leverage the system's resources, and was implemented in Linux using C. This is done in a thread-safe and data-safe manner by the use of appropriate mutexes. The user enters the total number of threads the system must use, the compression factor (a value between 1 to 10) and the source and destination image file names. A separate file is included that performs exactly the same, but sequentially and the comparisons are provided. The focus here is only to multithread the calculation of the DFTs of smaller 16x16 sub-images, and hence the DFT calcuations were implemented using the exisiting FFTW libraries.

## Running the Project
Make sure you are running the . On the Command Line, run the FruitMain.m file. Then, follow these steps:
1. Enter the entire file path to the image that is to be classified.
2. Read the instructions and enter 'Y'.
3. In the Colour Thresholder App, select the HSV Colour Space.
4. Using the polygon selection tool, draw onto the ROI. Refine the selection using the appropriate sliders.
5. Click on Export. Click OK.
6. Close the Colour Thresholder App. Return to the Command Window and press any key to re-activate the program.
7. The binary mask and the foreground image will be displayed. The name of the fruit will be written in the Command Window.

## Project Demo
![alt text](<https://github.com/nehemgr/Fruit-Recognition/blob/main/Report/sample_run.jpg>)
