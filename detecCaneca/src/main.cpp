/*
#-----------------------------------------------------------------------------
# Filename:    main.cpp
#-----------------------------------------------------------------------------
#      ___  ____  ___  ____  _________ 
#     / _ \/ __ \/ _ )/ __ \/_  __/ _ |
#    / , _/ /_/ / _  / /_/ / / / / __ |
#   /_/|_|\____/____/\____/ /_/ /_/ |_|
#-----------------------------------------------------------------------------
#
# Summary:     Cod to teste the can detector
# Obs: To enter in debug mode put ./debugador
*/

//rgb e hsv funfando juntas os dois

#include "../lib/robotalib.h"    /* the include file */

#define PTHREAD_THREADS_MAX 1024    //define o max de threads

/************************************************/  //define a direção 
/*
# 0* = tras
# 1* = frente
# 2* = esquerda
# 3* = direita
# * = potencia
*/

#define direita  30                 
#define esquerda  20
#define frente  13

int potencia=5;
int last_msg; //ultima mensagem enviada pela serial

using namespace cv;

char programName[64]="main.cpp"; /* program name */

serialArduino uno; 

/************************************************/  //variaveis de imagem global
Mat frame;
Mat frame_hsv;
Mat resposta_frame;
Mat resposta_hsv;
Mat resposta_fh;
/************************************************/  

int colorRange =15;       //radio de detcção rgb e hsv

/************************************************/  //declara algumas threads
pthread_mutex_t emframe = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t emframe_hsv = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t emframe_rgb = PTHREAD_MUTEX_INITIALIZER; 

pthread_cond_t newframe;              /*frame foi atualizado*/
pthread_cond_t newhsvFilter;          /*libera rgb filter, final process*/
pthread_cond_t newrgbFilter;          /*libera hsv filter, final process*/
/************************************************/  

/********************FUNCTIONS*******************/
void videoStart();                      //inicia parametros da camera
void takeOriginal_RgbTrack();           //trabalha a parte de testes dos valores rgb salvos em results.dat
void takeHsv_RgbTrack();                //trabalha a parte de testes dos valores hsv dalvos em results.dat
void printfval();                       //printa os valores de leitura de results.dat
void valtovalue();                      //armazena as leituras do arquivo em variaveis de leitura
void setInitParameters();               //inicia parametros, se results.dat não exise

void *streaming(void *);                //thread que faz a captura de imagens da camera deixando tudo atualizado o quanto poder
void *filter_lergb (void *);            //thread que faz o filtro rgb
void *filter_lehsv (void *);            //thread que faz o filtro hsv
void *filter_leanalise (void *);        //filtro que faz a fusao dos dois filtros

infoImg lata_x(Mat image);              //funcao que recebe a imagem do filtro final e responde a posicao da lata
/********************FUNCTIONS*******************/

/********************DECLARATIONS****************/
rgb value;        /*rgb*/
rgb valueh;       /*hsv*/
infoImg localCan; /*info of imag process*/
bool matlabBool=false;  /*boolean to control Matlab functions*/
Cronometer crono;       /*cronometer 1*/
Cronometer crono2;      /**cronometer 2*/
/********************DECLARATIONS****************/

/**********************MAIN**********************/
int main(int argc, char *argv[])
{
    uno.begin(9600,"/dev/ttyUSB0"); //inicia a comunicacao serial
    usleep(10);                     //espeta a comunicacao ser feita
    uno.sendByte(0);                //manda o robo para


    start_fps();					//inicia a captura do fps

	setInitParameters();           

    if (argc == 2)                  /*if argument exist run matlaab recorder*/
    {
        matlabBool=true;
        for(int i=0;i<3;i++)
        {
            printf("Matlab recorder ON : %d s\n",3-i);
            sleep(1);
        }
    }
    else
        for(int i=0;i<3;i++)
            {
                printf("adicionne argument to use matlab\n");
                printf("Matlab recorder OFF : %d s\n",3-i);
                sleep(1);
            }


   	data.read(); //le o results.dat
    if(checkFile(file,fileName))    //f data doesnt exist return the number of the beast
        return 666;
    
   	valtovalue();

    /********************************************/

    
    pthread_t get_img; 
    pthread_t filter_rgb,filter_hsv,filter_analise; //filters

    
    pthread_create(&get_img, NULL, streaming , NULL); //take imag from camera
    pthread_create(&filter_rgb, NULL, filter_lergb , NULL);         // 
    pthread_create(&filter_hsv, NULL, filter_lehsv , NULL);         // filters
    pthread_create(&filter_analise, NULL, filter_leanalise , NULL); //

    
    pthread_join(get_img,NULL);
    pthread_join(filter_rgb,NULL);
    pthread_join(filter_hsv,NULL);
    pthread_join(filter_analise,NULL);
    
    return 0;
}
/**********************MAIN**********************/

/************************************************/


void takeOriginal_RgbTrack()
{
	if((value.b>=colorRange && value.g>=colorRange && value.r>=colorRange)&&(value.b<=230 && value.g<=230 && value.r<=230))
		inRange(frame, Scalar(value.b-25, value.g-25, value.r-25), Scalar(value.b+25, value.g+25, value.r+25), resposta_frame);
	else resposta_frame=frame;
}
/************************************************/

void takeHsv_RgbTrack()
{
	if((valueh.b>=25 && valueh.g>=25 && valueh.r>=25)&&(valueh.b<=230 && valueh.g<=230 && valueh.r<=230))
		inRange(frame_hsv, Scalar(valueh.b-25, valueh.g-25, valueh.r-25), Scalar(valueh.b+25, valueh.g+25, valueh.r+25),resposta_hsv);
	else resposta_hsv=frame_hsv;
}
/************************************************/

void printfval()
{
	printf("vals:%d,%d,%d,%d,%d,%d\n",val[0],val[1],val[2],val[3],val[4],val[5]);
}
/************************************************/

void valtovalue()
{
	value.r=val[0];
   	value.g=val[1];
   	value.b=val[2];
   	valueh.r=val[3];
   	valueh.g=val[4];
   	valueh.b=val[5];
}
/************************************************/

infoImg lata_x(Mat image)
{
	infoImg infoImg;                                        //inicia os parametros para guardar as informações dala
    vector<vector<Point> > contours;
    Canny( image, image, image.rows, image.cols, 3 );       //encontra os contornos da imagem
    vector<Vec4i> hierarchy;                                //

    /* salva as informações para fazer o calculo de area e centro de massa */
    findContours( image, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE ); 
    
    //pega os momentos
    vector<Moments> mu(contours.size() );
    vector<Point2i> mc(contours.size() );

    
    for( int i = 0; i <(int) contours.size(); i++ )
    {
        mu[i] = moments( Mat(contours[i]), false );
    }
    
    printf("<><><><><><><><><><><>\n");


    float max_area=0.0;
    int can_i=0;
    int soma_x=0;
    int soma_y=0;

    /* calcula os minimos qudrados */
    for (int i = 0; i <(int) contours.size(); i++)
    	if((int)mu[i].m00!=0)
    	{
            max_area=contourArea(contours[i])+max_area;
            soma_x=((int)mu[i].m10/(int)mu[i].m00)*contourArea(contours[i])+soma_x;
            soma_y=((int)mu[i].m01/(int)mu[i].m00)*contourArea(contours[i])+soma_y;
    	}

    soma_x=soma_x/max_area;
    soma_y=soma_y/max_area;
    printf("AREA:                      %f\n",max_area );

    /* evita algum tipo de ruido muito gritante */
    if (soma_x<700 && soma_x>-700 && soma_y<700 && soma_y>-700)
    {
        infoImg.x=soma_x;
        infoImg.y=soma_y;
        infoImg.erro=soma_x-image.cols/2;
    }
    else
    {
        soma_x=infoImg.x=image.cols/2;
        soma_y=infoImg.y=image.rows/2;
        infoImg.erro=0;
    }
    
    /* implementacao do filtro e primeiro grau */
    float filterTime=0.25;
    SampleTime=1/(fps+0.01);

    printf("***************************\n");
    printf("%d ; %d\n",soma_x,soma_y );
    printf("%f ; %f ; %f ; %d\n",SampleTime,filterTime,Newvalx,infoImg.x);

    //filtro de primeira ordem x e y
    Newvalx=(Newvalx+0.00001)*( 1-(SampleTime+0.00001)/(filterTime+SampleTime))+((SampleTime+0.00001)/(filterTime+SampleTime))*infoImg.x;
    Newvaly=(Newvaly+0.00001)*( 1-(SampleTime+0.00001)/(filterTime+SampleTime))+((SampleTime+0.00001)/(filterTime+SampleTime))*infoImg.y;
    //Newvalx=infoImg.x;
    //Newvaly=infoImg.y;

    
    printf("***************************\n");
    printf("%f  XXXXX    %f\n",Newvalx,(float)infoImg.x);
    printf("%f  YYYYY    %f\n",Newvaly,(float)infoImg.y);
    printf("alfa=%f\n", ( SampleTime/(filterTime+SampleTime)) );
    printf("sec=%f\n", SampleTime);
    printf("***************************\n");
    


    infoImg.x=(int)Newvalx;
    infoImg.y=(int)Newvaly;

    printf("center>>%d,%d<<\n",infoImg.x,infoImg.y);
    
    printf("<><><><><><><><><><><>\n\n");
    return infoImg;
}

void setInitParameters()
{
    value.r=value.g=value.b=valueh.r=valueh.g=valueh.b=75; /*to set a inicial parameter*/
}

/******************************THREAD FUNCTIONS***************************/

void *streaming( void *)        /*take image from camera and make atualization of the image of analize*/
{
    if(!cap.isOpened()) {puts("can't open camera");}
    while(1)
    {
        pthread_mutex_lock(&emframe);
        cap >> frame;
        pthread_mutex_unlock(&emframe);
        usleep(10);
        pthread_cond_signal(&newframe);

    }
    return NULL;
}


void *filter_lehsv (void *)     /*make the filter of hsv image*/
{
    sleep(1);

    while(1)
    {   

        Mat image;
        pthread_cond_init(&newframe, NULL);
        
        pthread_mutex_lock(&emframe);
        frame.copyTo(image);
        pthread_mutex_unlock(&emframe);

        pthread_mutex_lock(&emframe_hsv);
        cvtColor(image,frame_hsv,CV_RGB2HSV);
        takeHsv_RgbTrack();
        pthread_mutex_unlock(&emframe_hsv);

        pthread_cond_signal(&newhsvFilter);
        
    }
}


void *filter_lergb (void *)     /*make the rgb filter*/
{
    
    sleep(1);

    while(1)
    {   
        
        Mat image;
        pthread_cond_init(&newframe, NULL);

        pthread_mutex_lock(&emframe);
        frame.copyTo(image);
        pthread_mutex_unlock(&emframe);

        pthread_mutex_lock(&emframe_rgb);
        takeOriginal_RgbTrack();
        pthread_mutex_unlock(&emframe_rgb);

        pthread_cond_signal(&newrgbFilter);
        /*        
        namedWindow("rgb", CV_WINDOW_FREERATIO);
        imshow("rgb",resposta_hsv);
        waitKey(30);
        */
    }
}

void *filter_leanalise (void *)
{
    Mat rgbFilter;
    Mat hsvFilter;
    Mat finalFilter;
    Mat copyFrame;
    sleep(2);    

    int lelesco;
    crono.startCrono();

    while(1)
    {
        crono2.startCrono();
        pthread_cond_init(&newrgbFilter, NULL);
        pthread_cond_init(&newhsvFilter, NULL);
        pthread_mutex_lock(&emframe);
        frame.copyTo(copyFrame);
        pthread_mutex_unlock(&emframe);
        
        pthread_mutex_lock(&emframe_rgb);
        resposta_frame.copyTo(rgbFilter);
        pthread_mutex_unlock(&emframe_rgb);
        
        pthread_mutex_lock(&emframe_hsv);
        resposta_hsv.copyTo(hsvFilter);
        pthread_mutex_unlock(&emframe_hsv);
        
        bitwise_and(hsvFilter,rgbFilter,finalFilter,Mat());
        
        localCan = lata_x(finalFilter);
        
        /*
        namedWindow("hsv", CV_WINDOW_FREERATIO);
        imshow("hsv",finalFilter);
        waitKey(30);
        */

        printf("<<<<<<<<<<%d,%d,%d>>>>>>>>>>>.\n",localCan.x,localCan.y,localCan.erro );
        drawCross ( cvPoint(localCan.x,localCan.y),Scalar(0,0,255), 20, copyFrame);
        desenha_seta(copyFrame, cvPoint(copyFrame.cols/2,copyFrame.rows) , cvPoint(localCan.x,localCan.y) ,cinza);
        printf("ANGULO : %fº\n",atan(-(copyFrame.cols/2-localCan.x+0.0000000001)/(copyFrame.rows-localCan.y+0.0000000001))*180/pi);
        
        //BARRAS
        lelesco++;
        float PW,PL;
        PW=sin(lelesco);
        PL=-PW;
        line(copyFrame,Point(copyFrame.cols-2,copyFrame.rows/2),Point(copyFrame.cols-2,(int)(PW*239+copyFrame.rows/2)),green,3); //bars
        line(copyFrame,Point(copyFrame.cols-7,copyFrame.rows/2),Point(copyFrame.cols-7,(int)(PL*239+copyFrame.rows/2)),red,3);   //bars
        //QUADRANTES
        int numQuadrantes=4;
        for(int i=1;i<numQuadrantes;i++)
        {
            line(copyFrame,Point(copyFrame.cols*i/numQuadrantes,0),Point(copyFrame.cols*i/numQuadrantes,copyFrame.rows),laranja);
            line(copyFrame,Point(0,copyFrame.rows*i/numQuadrantes),Point(copyFrame.cols,copyFrame.rows*i/numQuadrantes),laranja);
        }
        //CAIXA
        line(copyFrame,Point(copyFrame.cols/3,4*copyFrame.rows/5),Point(copyFrame.cols*2/3,4*copyFrame.rows/5),blue);
        line(copyFrame,Point(copyFrame.cols/3,4*copyFrame.rows/5),Point(copyFrame.cols/3,copyFrame.rows),blue);
        line(copyFrame,Point(copyFrame.cols*2/3,4*copyFrame.rows/5),Point(copyFrame.cols*2/3,copyFrame.rows),blue);
        //CRUZ
        //line(copyFrame,Point(copyFrame.cols/2,0),Point(copyFrame.cols/2,copyFrame.rows),laranja);
        //line(copyFrame,Point(0,copyFrame.rows/2),Point(copyFrame.cols,copyFrame.rows/2),laranja);
        //TEXTO
        char text[20];
        sprintf(text, "X=%d, Y=%d",(copyFrame.cols/2 - localCan.x),(copyFrame.rows/2 - localCan.y));
        putText(copyFrame, text, Point(5,15), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0,255,0));
        sprintf(text, "Theta: %.2f degrees",atan(-(copyFrame.cols/2-localCan.x+0.0000000001)/(copyFrame.rows-localCan.y+0.0000000001))*180/pi);
        putText(copyFrame, text, Point(5,30), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0,255,0));
    
        
        /*
        # Para: Victor
        # Aqui esta é o melhor lugar para trabalhar com isso, a melhor ideia seria uma function para fazer o calculo de PW e PL
        # onde todos os dados ja estão prontos
        */


        /* logica para seguir a caneca */
        if (localCan.erro<-copyFrame.cols/4 && last_msg!=3) 
        {
            uno.sendByte(esquerda+potencia);
            last_msg=3;
        }
        else
            if(localCan.erro>+copyFrame.cols/4 && last_msg!=4) 
            {
                uno.sendByte(direita+potencia);
                last_msg=4;
            }
            
            
        if (last_msg!=0 && localCan.erro>-copyFrame.cols/4 && localCan.erro<copyFrame.cols/4 && localCan.y>copyFrame.rows/2)
        {
            uno.sendByte(frente);
            last_msg=0;
        }

        if (last_msg!=0 && localCan.erro>-copyFrame.cols/4 && localCan.erro<copyFrame.cols/4 && localCan.y<copyFrame.rows/2)
        {
            uno.sendByte(0);
            last_msg=0;
        }
    

        /*function to save data for matlab*/
        if(matlabBool==true)
        {
            sprintf(text, "MATLAB: ON %d%%",( (int)matlab.i-1 ));
            putText(copyFrame, text, Point(5,45), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0,255,0));

            while(crono2.finishCrono(false)<0.25){};
            for(int i=0;i<matlab.i;i++)printf("#");
            printf(":  %d%%\n",(int)matlab.i-1);
            if(matlab.testLim())
            {
                matlab.i++;
                matlab.x[matlab.i]=localCan.x;
                matlab.y[matlab.i]=localCan.y;
                matlab.timexy[matlab.i]=crono.finishCrono(true);
                matlab.theta[matlab.i]=atan(-(copyFrame.cols/2-localCan.x+0.0000000001)/(copyFrame.rows-localCan.y+0.0000000001))*180/pi;
            }
            else
            {
                matlabBool=false;
                matlab.saveMatlab();
            }
        }
        else
            sprintf(text, "MATLAB: OFF");
            putText(copyFrame, text, Point(5,45), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0,255,0));

        /* mostra o resultado do local da caneca */
        //namedWindow("resposta", CV_WINDOW_FREERATIO);
        imshow("resposta",copyFrame);
        end_fps();
        waitKey(30);
    }
}
