#include <stdio.h>
#include <math.h>
#include <X11/Xlib.h>

#define DIM 512

// declaring arrays
float f_image[DIM][DIM]; 
int i_image[DIM][DIM];
unsigned char c_image[DIM][DIM];
float h_image[DIM][DIM]; //convulation result - float data type

/******************************************************************/
/* This structure contains the coordinates of a box drawn with    */
/* the left mouse button on the image window.                     */
/* roi.x , roi.y  - left upper corner's coordinates               */
/* roi.width , roi.height - width and height of the box           */
/******************************************************************/
extern XRectangle roi;

float template[DIM][DIM];


/******************************************************************/
/* Main processing routine. This is called upon pressing the      */
/* Process button of the interface.                               */
/* image  - the original greyscale image                          */
/* size   - the actual size of the image                          */
/* proc_image - the image representation resulting from the       */
/*              processing. This will be displayed upon return    */
/*              from this function.                               */
/******************************************************************/
void process_image(image, size, proc_img)
unsigned char image[DIM][DIM];
int size[2];
unsigned char proc_img[DIM][DIM];
{
    printf("(size[0],size[1]): (%d,%d)\n",size[0],size[1]);
    printf("roi.height: %d\n", roi.height);
    printf("roi.width: %d\n", roi.width);
    // ----------------------------------------type casting char to float------------------
    // float f_image[DIM][DIM];   
    for (int i = 0; i < size[0]; i++) {
        for (int j = 0; j < size[1]; j++) {
            f_image[i][j]=(float)image[i][j];
        }
    }

    // since the image array received from image is in the form image[m][n] instead of image[n][m]
    // transpose the template to match template components to corresponding elements in image array for normalization
    // fill template with image region selected
    for(int i=0; i<roi.height; i++){
        for(int j=0; j<roi.width; j++){
            template[roi.x+i][roi.y+j]=f_image[roi.y+j][roi.x+i]/(roi.height*roi.width);
        }
    }

    //----------------------------------calculate average of template------------------------
    // calculate average of template
    float avg_template;
    float sum=0;
    for(int i=0; i<roi.height; i++){
        for(int j=0; j<roi.width; j++){
            sum = sum + template[roi.x+i][roi.y+j];
        }
    }
    avg_template = sum/(roi.height*roi.width); // k*l size template
    printf("average of the template: %f\n", avg_template);

    //----------------------------------calculate average of image------------------------
    // calculate average of image
    float avg_f_image;
    float total=0;
    for(int x=0; x<size[0]; x++){
        for(int y=0; y<size[1]; y++){
            total = total + f_image[x][y];
        }
    }
    avg_f_image = total/(size[0]*size[1]); // m*n size image
    printf("average of the image: %f\n", avg_f_image);


    // compute convolution and normalize: h(x,y) = Σ(i=-k to k)Σ(j=-l to l) ((image(x+i,y+j)-imgavg)*(template(i+k,j+l)-templateavg))
    // iterate through all locations of image where template can be placed and fill the remaining locations with neighbour's value
    // the template can be placed to (m-(k-1))*(n-(l-1)) locations for [m x n-image & 2k+1 x 2l+1-template]
    // 2k+1 x 2l+1 template, 2k+1=roi.height rows, 2l+1=roi.width cols 
    for(int x=1; x<=(size[0]-(((roi.height-1)/2)-1)); x++){      // k=(roi.height-1)/2; // 2k+1=roi.height rows
        for(int y=1; y<=(size[1]-(((roi.width-1)/2)-1)); y++){  // l=(roi.width-1)/2; // 2l+1=roi.width cols
            h_image[x][y]=0;
            // iterate through all rows and cols of template region and sum them up
            // here convolution is k*l expensive i.e ((roi.height-1)/2) * ((roi.width-1)/2)
            for (int i=-((roi.height-1)/2); i<=((roi.height-1)/2); i++){
                for(int j=-((roi.width-1)/2); j<=((roi.width-1)/2); j++){
                    // h(x,y) = Σ(i=-k to k)Σ(j=-l to l) ((image(x+i,y+j)-iavg)*template(i+k,j+l))
                    h_image[x][y] = (f_image[x+i][y+j]-avg_f_image)*(template[i+((roi.height-1)/2)][j+((roi.width-1)/2)]-avg_template) + h_image[x][y];
                }
            }
        }
    }

    // fill the edge locations with neighbour's value
    for(int x=0; x<size[0]; x++){
        for(int y=0; y<size[1]; y++){
            if(x==0 && y==0){
                h_image[x][y]=h_image[x+1][y+1];
            }
            else if(x==0){
                h_image[x][y]=h_image[x+1][y];
            }
            else if(y==0){
                h_image[x][y]=h_image[x][y+1];
            }
            else if(x==((size[0]-(((roi.height-1)/2)-1))+1)){
                h_image[x][y]=h_image[x-1][y];
            }
            else if(y==((size[1]-(((roi.width-1)/2)-1))+1)){
                h_image[x][y]=h_image[x][y-1];
            }
        }
    }

    // contrast calculation compensation for its influence
    // Contrast is usually measured in terms of the standard deviation of the pixels in the image
    // SD_f = sqrt(Σ(x=0 to n)Σ(y=0 to m)(f(x,y)-favg)^2)
    //-----------------------Standard deviation of image and template----------------------
    float SD_f_image;
    float var_f_image=0;
    for (int i = 0; i < size[0]; i++) { // Σ(y=0 to m)
        for (int j = 0; j < size[1]; j++) { // Σ(x=0 to n)
            var_f_image = (f_image[i][j]-avg_f_image)*(f_image[i][j]-avg_f_image) + var_f_image;
        }
    }
    SD_f_image = sqrt(var_f_image);
    printf("SD_f_image: %f \n", SD_f_image);


    float SD_template;
    float var_template=0;
    for (int i = 0; i < roi.height; i++) { // Σ(y=0 to m)
        for (int j = 0; j < roi.width; j++) { // Σ(x=0 to n)
            var_template = (template[roi.x+i][roi.y+j]-avg_template)*(template[roi.x+i][roi.y+j]-avg_template) + var_template;
        }
    }
    SD_template = sqrt(var_template);
    printf("SD_template: %f \n", SD_template);

    // Increasing contrast scales correlation i.e Higher contrast yields stronger positive and negative “matches”
    // Normalization can be used to compensate for this
    for(int i=0; i<size[0]; i++){
        for(int j=0; j<size[1]; j++){
            h_image[i][j]=h_image[i][j]/(SD_f_image*SD_template);
        }
    }

    // find max and min from the processed result 
    float max=-10000;
    float min=10000;
    for(int i=0; i<size[0]; i++){
        for(int j=0; j<size[1]; j++){
            if(h_image[i][j]>max){
                max=h_image[i][j];
            }
            if(h_image[i][j]<min){
                min=h_image[i][j];
            }
        }
    }

    // scaling range [min, max] to range [0, 255]
    // for scaling range [min, max] to [a,b], h(x,y) = ((h(x,y)-min)*(b-a))/(max-min) + a
    for(int i=0; i<size[0]; i++){
        for(int j=0; j<size[1]; j++){
            h_image[i][j]= ((h_image[i][j]-min)*(255-0))/(max-min)+0; 
        }
    }

    // ---------------------------------------type casting float to int------------------
    // int i_image[DIM][DIM];
    for (int i = 0; i < size[0]; i++) {
        for (int j = 0; j < size[1]; j++) {
            i_image[i][j]=(int)h_image[i][j];
        }
    }

    // printf("template: (%d,%d)\n",roi.x,roi.y);

    // i_image[roi.x-1][roi.y]=255;
    // i_image[roi.x][roi.y-1]=255;
    // i_image[roi.x+1][roi.y]=255;
    // i_image[roi.x][roi.y+1]=255;
    // i_image[roi.x-1][roi.y-1]=255;
    // i_image[roi.x+1][roi.y+1]=255;
    // i_image[roi.x][roi.y]=255;
    // printf("image:    %d\n",i_image[roi.x][roi.y]);

    // ----------------------------------------type casting int to char------------------
    // unsigned char proc_img[DIM][DIM];
    for (int i = 0; i < size[0]; i++) {
        for (int j = 0; j < size[1]; j++) {
            c_image[i][j]=(char)i_image[i][j];
        }
    }

    // passing values to processed image
    for (int i = 0; i < size[0]; i++) {
        for (int j = 0; j < size[1]; j++) {
            proc_img[i][j]=c_image[i][j];
        }
    }

}

