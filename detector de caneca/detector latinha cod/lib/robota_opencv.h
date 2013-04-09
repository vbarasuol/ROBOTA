/*
#-----------------------------------------------------------------------------
# Filename:    robota_opencv.h
#-----------------------------------------------------------------------------
#      ___  ____  ___  ____  _________ 
#     / _ \/ __ \/ _ )/ __ \/_  __/ _ |
#    / , _/ /_/ / _  / /_/ / / / / __ |
#   /_/|_|\____/____/\____/ /_/ /_/ |_|
#-----------------------------------------------------------------------------
#
# Summary:     simple lib of opencv
*/

#include <iostream>
#include <time.h>
#include <sys/time.h>
#include <cstdio>
#include <cstring>
#include <opencv/cv.h>
#include <opencv/highgui.h>
using namespace cv;   // declara o namespaec cv::Mat


// drawCross( center, color, d, drawing )  

// limpa_linha(int numero_de_linhas);
// desenha_seta(Mat image, Point2f coco1, Point2f coco2,Scalar cor);

/*DEFININDO ALGUMAS VARIAVEIS LEGAIS*/
Scalar red (Scalar(0,0,255));                       //scalar pronto para criar objts
Scalar green (Scalar(0,255,0));                     
Scalar blue (Scalar(255,0,0));   
Scalar cinza (Scalar(190,190,190));
static const double pi = 3.14159265358979323846;    

/************************************************/  //variaveis para a contagem de fps  
time_t start, end;
double fps;
double sec;
int counter=0;
int numframes=0;
/************************************************/ 

#define start_fps() time(&start)  //inicia a contagem de tempo

void end_fps()      //finaliza a contagem de tempo
{
    time(&end);
    counter++;  // calcula o fps
    sec = difftime (end, start);
    fps = counter / sec;
    printf("FPS = %.2f\n", fps);   //mostra o fsp
}

#define drawCross( center, color, d, drawing )                      	\
    	line( drawing, Point( center.x - d, center.y - d ),             \
    	Point( center.x + d, center.y + d ), color, 2, CV_AA, 0);       \
    	line( drawing, Point( center.x + d, center.y - d ),             \
    	Point( center.x - d, center.y + d ), color, 2, CV_AA, 0 )
///// EXEMPLO drawCross(cvPoint(X,Y),Scalar(0,0,255), int RAIO,cv Mat);

/************************************************/  //funcao para limpar linhas no temrinal
void limpa_linha(int numero_de_linhas)
{
	for (int i = 0; i < numero_de_linhas; ++i)
	{
		printf("\033[K" );
		printf("\033[1A");
        printf("\033[K" );
	}
}

void desenha_seta(Mat image, Point2f coco1, Point2f coco2,Scalar cor)
{
    line( image, coco1, coco2, cor);
    double Theta=atan2( (double) coco1.y - coco2.y, (double) coco1.x - coco2.x );
    double hypotenuse = 10;//sqrt( pow(coco1.y - coco2.y,2) + pow(coco1.x - coco2.x,2) );
    coco1.x=(int)(hypotenuse*cos(Theta)/4-hypotenuse*sin(Theta)/4+coco2.x);
    coco1.y=(int)(hypotenuse*sin(Theta)/4+hypotenuse*cos(Theta)/4+coco2.y);
    line(image,coco1,coco2,cor);  //Draw Line
    coco1.x=(int)(hypotenuse*cos(Theta)/4+hypotenuse*sin(Theta)/4+coco2.x);
    coco1.y=(int)(hypotenuse*sin(Theta)/4-hypotenuse*cos(Theta)/4+coco2.y);
    line(image,coco1,coco2,cor);  //Draw Lin
}

/************************************************/  //rgb struct
struct rgb
{
	int r;
	int g;
	int b;
};

/************************************************/  //struct para dados da lata
struct infoImg
{
    int x;
    int y;
    int erro;
};

