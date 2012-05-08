/*!
 * TrackingDrosophila.cpp
 *
 * Cuerpo principal del algoritmo de rastreo.
 *
 *  Created on: 27/06/2011
 *      Author: chao
 */

#include "TrackingDrosophila.hpp"

using namespace cv;
using namespace std;

extern double NumFrame ; /// contador de frames absolutos ( incluyendo preprocesado )


double NumFrame ; /// contador de frames absolutos ( incluyendo preprocesado )



/// PREPROCESADO
//Modelado de fondo
StaticBGModel* BGModel = NULL;
// Modelado de forma
SHModel* Shape;

/// PROCESADO
/// Estructura frame
STFrame* FrameDataIn = NULL;
STFrame* FrameDataOut = NULL;

///Parámetros fondo para procesado
BGModelParams *BGPrParams = NULL;
///Parámetros Validación para procesado
ValParams* valParams = NULL;

///HightGui
int g_slider_pos = 0;


int main(int argc, char* argv[]) {

	struct timeval ti,  tif, tinicio; // iniciamos la estructura de medida de tiempos
	float TiempoFrame;
	float TiempoGlobal;
	double TotalFrames = 0 ;
	double FPS;

	char nombreFichero[30];
	char nombreVideo[30];
//	char* Ficheros[4];

	if( argc<1) {help(); return -1;};


	///////////  INICIALIZACIÓN ////////////
	//  CAPTURA  //
	CvCapture* g_capture = NULL;/// puntero a una estructura de tipo CvCapture
	CvVideoWriter* VWriter;
	IplImage* frame;

	printf( "Iniciando captura...\n" );
	gettimeofday(&tinicio, NULL);//para obtener el tiempo transcurrido desde el inicio del programa
	g_capture = cvCaptureFromAVI( argv[1] );
	if ( !g_capture ) {
		error( 1 );
		help();
		return -1;
	}

//  Iniciar parámetros de visualización y creación de ventanas

	TotalFrames = cvGetCaptureProperty( g_capture, CV_CAP_PROP_FRAME_COUNT);
//	cvSetCaptureProperty( g_capture, CV_CAP_PROP_POS_AVI_RATIO,0.95);
//	float TotalTime = cvGetCaptureProperty( g_capture, CV_CAP_PROP_POS_MSEC);
//	cvSetCaptureProperty( g_capture, CV_CAP_PROP_POS_AVI_RATIO,0);
	if(!TotalFrames) TotalFrames = getAVIFrames(argv[1]); // en algun linux no funciona lo anterior
	FPS = cvGetCaptureProperty( g_capture, CV_CAP_PROP_FPS);
	FPS = 15;
	printf("\nInicializando parámetros...");
	frame = cvQueryFrame( g_capture );

	AllocDefaultVisParams( frame );


	TiempoGlobal = 0;
	NumFrame = 0;
	TiempoFrame = 0;
	////////// PRESENTACIÓN ////////////////
	if(SHOW_WINDOW && SHOW_PRESENT) DraWPresent(  );

	////////// INICIALIZACIÓN //////////////
	if (!Inicializacion( argc, argv,nombreFichero,nombreVideo ) ) return -1;
	VWriter = iniciarAvi( g_capture, nombreVideo);
	//////////  PREPROCESADO   ////////////

	if (!PreProcesado( argv[1], &BGModel, &Shape) )  Finalizar(NULL, NULL);

	/////////	PROCESADO   ///////////////

	printf("\n\nIniciando procesado...\n");
	if(SHOW_WINDOW) Transicion("Iniciando Tracking...", 1,1000, 50 );

	/*********** BUCLE PRINCIPAL DEL ALGORITMO ***********/
	for( int i = 0; i < INIT_DELAY; i++ ){ // retardo al inicio
		frame = cvQueryFrame( g_capture );
		NumFrame ++;
	}
    for( NumFrame = NumFrame; ; NumFrame++ ){

    	frame = cvQueryFrame(g_capture);
    	/*Posteriormente  Escribir en un fichero log el error. Actualizar el contador
    	  de frames absolutos. */
    	gettimeofday(&tif, NULL);
    	gettimeofday(&ti, NULL);
    	if( !frame ){
    		NumFrame = NumFrame + RetryCap( g_capture, frame );
    	}
    	if( !frame ) Finalizar(&g_capture, &VWriter);
		if ( (cvWaitKey(10) & 255) == 27 ) break;

		printf("\n//////////////////////////////////////////////////\n");
		printf("\t\t\tFRAME %0.f ",NumFrame);

		//////////  PROCESAR  ////////////
		FrameDataIn = Procesado(frame, BGModel, Shape, valParams, BGPrParams );

		//////////  RASTREAR  ////////////
		FrameDataOut = Tracking( FrameDataIn, 10, BGModel, VWriter,FPS );

		// SI BUFFER LLENO
		////////// ESTADISTICAS //////////

		CalcStatsFrame( FrameDataOut );

		//////////  VISUALIZAR     ////////////
//			VisualizarFr( FrameDataOut , BGModel, VWriter );
		if(SHOW_WINDOW) DraWWindow( frame, FrameDataOut, BGModel,VWriter, TRAKING );

		//////////  ALMACENAR ////////////
		if(!GuardarSTFrame( FrameDataOut, nombreFichero ) ){error(6);Finalizar(&g_capture, &VWriter);}

//			////////// LIBERAR MEMORIA  ////////////
		liberarSTFrame( FrameDataOut );

		FrameDataIn->GStats = SetGlobalStats( NumFrame, tif, tinicio, TotalFrames, FPS );


		printf("\n//////////////////////////////////////////////////\n");
		printf("\nTiempo de procesado del  Frame %.0f : %5.4g ms\n",NumFrame-1, FrameDataIn->GStats->TiempoFrame);
		printf("Segundos de video procesados: %0.f seg \n", FrameDataIn->GStats->TiempoGlobal);
		printf("Porcentaje completado: %.2f %% \n",((NumFrame-1)/TotalFrames)*100 );
		printf("\n//////////////////////////////////////////////////\n");
//		if(!SHOW_WINDOW)	VisualizarFr( FrameDataIn, BGModel, VWriter, visParams );
//		liberarSTFrame( FrameDataIn );
    }

    ///////// POST-PROCESADO ///////////////////


	///////// LIBERAR MEMORIA Y TERMINAR////////
    Finalizar(&g_capture, &VWriter);

}

void Finalizar(CvCapture **g_capture,CvVideoWriter**VWriter){

	CvCapture *capture;
	CvVideoWriter *Writer;

	// liberar imagenes y datos de preprocesado
	releaseDataPreProcess();
	//liberar modelo de fondo
	DeallocateBGM( BGModel );
	//liberar modelo de forma
	if( Shape) free(Shape);

	// liberar imagenes y datos de procesado
	releaseDataProcess(valParams, BGPrParams);
	if( FrameDataIn ) liberarSTFrame( FrameDataIn);
//	if( FrameDataOut ) liberarSTFrame( FrameDataOut);

	// liberar imagenes y datos de tracking
	ReleaseDataTrack( );

	// liberar estructura estadísticas
	releaseStats();
	// liberar imagenes y datos de visualización
	releaseVisParams( );
	DestroyWindows( );

	//liberar resto de estructuras
	capture = *g_capture;
	Writer = *VWriter;
	if (capture)cvReleaseCapture(&capture);
	if (Writer) cvReleaseVideoWriter(&Writer);

	exit (1);
}



