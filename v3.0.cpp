////////////////////////////////////////////////////////////////////////
// Julien Desprez
// main.cpp
// playing around with combinations of images
// assumes images from identical cameras 
// I.E. same width x height in pixels from each camera in the system
// currently recursively stitches 3 videos together
////////////////////////////////////////////////////////////////////////


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <signal.h>
#include <QtNetwork>

//Constants to simplify modification
#define CAMERA_VIDEO_WIDTH 640 //width in pixels of each subframe
#define CAMERA_VIDEO_HEIGHT 480 //height in pixels of each subframe
#define SYSTEM_SIZE 3 //number of cameras / images

void stopper(int);
void bandwidthCheck(IplImage *,char *, int);
void* camThread(void *);
void* camThread1(void *);
void* camThread2(void *);

uchar *blu;
CvMat *encode;

int compression[3] = {CV_IMWRITE_JPEG_QUALITY,30,0};



QUdpSocket *udpSocket;

QString hri_address;
int udp_port;

bool stop = true;


CvCapture* capture1 = cvCaptureFromCAM(0);
  CvCapture* capture2 =cvCaptureFromCAM(1);
  CvCapture* capture3 =cvCaptureFromCAM(2);


IplImage* images[SYSTEM_SIZE + 1]; //array to store the images, where the image at 0 is the main image to be displayed


int main(int argc, char *argv[])
{
  
  pthread_t thread_s;
  pthread_t thread_v;
  pthread_t thread_t;

  //CvVideoWriter *writer = 0; //for writing video
  
  //writer=cvCreateVideoWriter("test.avi",CV_FOURCC('M','J','P','G'),
                           //25,cvSize(1920,480),1);


  //initialize the stream from the first available capture device on the pc
  //CvCapture* capture = cvCaptureFromCAM(0);
  
  //currently displaying video files instead of live capture devices because
  //opencv is bitchy and doesn't like me cloning my one webcam 3 times
  
  
  
  
  //CvCapture* capture3 =cvCaptureFromAVI("/home/darkside/Videos/out1.avi");

  signal(SIGINT, &stopper);
  
  
   
   udp_port = 45454;
   hri_address = "192.168.1.50";

   udpSocket = new QUdpSocket();
  
  int i,z;  //iterators
  z = SYSTEM_SIZE; //set for countdown later
  
  //initialize each of the images being used for the system
  for(i = 0; i < SYSTEM_SIZE; i++)
  {
      images[i] = NULL;
  }
   
   //Load the images into their respective places
   //unfortunately this must be done one at a time 
   cvGrabFrame(capture1);
   images[1] = cvRetrieveFrame(capture1);
  
   cvGrabFrame(capture2);
   images[2] = cvRetrieveFrame(capture2);

  cvGrabFrame(capture3);
  images[3] = cvRetrieveFrame(capture3);
  
  pthread_create(&thread_s, NULL, camThread, NULL);
  pthread_create(&thread_v, NULL, camThread2, NULL);
  pthread_create(&thread_t, NULL, camThread1, NULL);

  
  //create a larger image to contain all the images of the system horizontally.
  //This step must be done after declaring at least images[1] because this createImage
  //call relies on it to determine the number of channels for the main image. This is useful
  //because you do not need to know in advance the number of channels in an image
  //which is a non intuitive detail of an image.
  images[0] = cvCreateImage( cvSize(CAMERA_VIDEO_WIDTH * SYSTEM_SIZE , CAMERA_VIDEO_HEIGHT), IPL_DEPTH_8U, images[1]->nChannels ); 
  
  cvZero(images[0]); //clear the array of data for the main image
  
  
  //create a window automatically sized to the image it displays, currently autosize is the only flag that works
  //also moves the window to the upper lefthand corner of the screen
  cvNamedWindow("Panorama Display", CV_WINDOW_AUTOSIZE); 
  cvMoveWindow("Panorama Display", 0, 0);
  
  try
  {
        //Set the ROI for each of the individual images to be the whole image so that
        //nothing will be left out when the images are dumped into the larger image
        //This only has to be done once. This can be done recursively because all the
        //images should be the same width and height.
        for(i = 1; i <= SYSTEM_SIZE; i++)
        {
            cvSetImageROI(images[i],cvRect(0,0,CAMERA_VIDEO_WIDTH,CAMERA_VIDEO_HEIGHT));
        }
  }
  catch(cv::Exception)
  {

  }
  
  IplImage* bayer = NULL;
  IplImage* rgb3 = NULL;
  //recursively dump the sub images into the larger image (images[0])
  //it is somewhat system intensive to set the ROI for the main image each time
  //but I have yet to find a better way to do it
  //loops forever
    
  
  bayer = cvCreateImage( cvGetSize(images[0]), IPL_DEPTH_8U, 1);
  rgb3 = cvCreateImage( cvGetSize(images[0]), IPL_DEPTH_8U, 3);
  
  while(stop)
  {
      try
      {
          //Grab frame from each capture device before one creates and image from it
          

          //cvGrabFrame(capture2);
          //images[2] = cvRetrieveFrame(capture2);

          //cvGrabFrame(capture3);
          //images[3] = cvRetrieveFrame(capture3);


 
 
        while(z >= 1)
        {
            for(i = 1; i <= SYSTEM_SIZE; i++)
            {
              //set region of interest for the large image
              cvSetImageROI(images[0],cvRect(CAMERA_VIDEO_WIDTH * (SYSTEM_SIZE - z),0,CAMERA_VIDEO_WIDTH,CAMERA_VIDEO_HEIGHT));
              cvCopy(images[i], images[0]); //copy first image into the main image

              // reset the ROI so that it can be set again for the next image
              cvResetImageROI(images[0]);
              z--;
            }

            //cvReleaseImage(&images[1]);
        }

		
	   // bandwidthCheck(images[0],"jpeg",50);

          // set channel of interest to 1
            cvSetImageCOI(images[0],1);
            // bucket for channel 1 of grabbed image
            
            // copy channel 1 of grabbed image to bucket
            cvCopy(images[0],bayer,NULL);
            // create interleaved 8 bit, 3 channel image
           
            // convert single channel bayer color space to 3 channel RGB
            cvCvtColor(bayer,rgb3,CV_BayerRG2RGB);
          


          //cvWriteFrame(writer,rgb3); //write frame
          //cvShowImage("Panorama Display", rgb3); //put the frame into the main window
         // cvSaveImage("derp.jpg",rgb3);
		
	  //bandwidthCheck(rgb3,"jpeg",50);	  

          
       
          encode = cvEncodeImage(".jpg",rgb3,compression);
          
          blu = encode->data.ptr;

          udpSocket->writeDatagram((char*)blu,encode->step,QHostAddress(hri_address),udp_port);
          
          

          //cvWaitKey(20);           // wait 20 ms, necessary for code to work

          //reset iterator so the drop through works again
          z = SYSTEM_SIZE;
          
          //cvReleaseImage(&images[0]);
          
          //cvReleaseImage(&rgb3);
          //cvReleaseImage(&bayer);
          
      }
      catch(cv::Exception)
     {
         printf("Lulz");
     }
  }

  cvReleaseCapture(&capture1);
  cvReleaseCapture(&capture2);
  cvReleaseCapture(&capture3);
  printf("\nfinished\n"); 
  return 0;
}

void stopper(int signo)
{
  stop = false;
}


void bandwidthCheck(IplImage *imageToCheck, char *imageType, int compFactor)
{
	
	int compression[3];	
	compression[2] = 0;
	CvMat *encodedImage = NULL;

	if(0 < compFactor <= 100)
	{
	   compression[0] = CV_IMWRITE_JPEG_QUALITY;
	   compression[1] = compFactor;
	   encodedImage = cvEncodeImage(".jpg",imageToCheck,compression);
	   printf("\033[2J\033[1;1H"); //clears previous term output linux only
	}
	else
	{
	   printf("\033[2J\033[1;1H"); //clears previous terminal output
	   printf("Not a valid jpeg compression factor! Using default value\n");
	   encodedImage = cvEncodeImage(".jpg",imageToCheck);		
	}
				
	printf("Currently compressing image as %s, the bandwidth of the images in this format is %.01f KB/s \n",imageType,(double)(encodedImage->step/1000));
}

void* camThread(void * arg)
{
    while(1)
    {
        cvGrabFrame(capture1);
        images[1] = cvRetrieveFrame(capture1);
    }   
}

void* camThread2(void * arg)
{
    while(1)
    {
        cvGrabFrame(capture3);
        images[3] = cvRetrieveFrame(capture3);
    }   
}

void* camThread1(void * arg)
{
    while(1)
    {
        cvGrabFrame(capture2);
        images[2] = cvRetrieveFrame(capture2);
    }   
}
