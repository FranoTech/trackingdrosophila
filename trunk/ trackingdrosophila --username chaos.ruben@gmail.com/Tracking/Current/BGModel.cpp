/*!
 * BGModel.cpp
 *
 * Modelado de fondo mediante distribución Gaussiana
 *
 *
 *
 *
 *  Created on: 27/06/2011
 *      Author: chao
 */

#include "BGModel.h"

//int FRAMES_TRAINING = 20;
//int HIGHT_THRESHOLD = 20;
//int LOW_THRESHOLD = 10;
//double ALPHA = 0 ;

int g_slider_position = 50;

// Float 1-Channel
IplImage *Imedian;
IplImage *ImedianF;
IplImage *IdiffF;
IplImage *Idiff;
IplImage *IdesF; /// Desviación típica. Coma flotante 32 bit
IplImage *Ides; /// Desviación típica.
IplImage *IvarF; /// Varianza
IplImage *Ivar;
IplImage *IhiF; /// La mediana mas x veces la desviación típica
IplImage *IlowF; /// La mediana menos x veces la desviación típica


//Byte 1-Channel
IplImage *ImGray; /// Imagen preprocesada
IplImage *ImGrayF; /// Imagen preprocesada float
IplImage *Imaskt;
IplImage* fgTemp;

extern float TiempoGlobal ;


StaticBGModel* initBGModel( CvCapture* t_capture, BGModelParams* Param){

	struct timeval ti, tf, tif, tff; // iniciamos la estructura
	int TiempoParcial;
	int num_frames = 0;
	IplImage* frame = cvQueryFrame( t_capture );
			if ( !frame ) {
				error(2);
				exit(-1);
			}
			if ( (cvWaitKey(10) & 255) == 27 ) exit(-1);

	 if( Param == NULL ) DefaultBGMParams( Param );

	//Iniciar estructura para el modelo de fondo estático
	StaticBGModel* bgmodel;

	bgmodel = ( StaticBGModel*) malloc( sizeof( StaticBGModel));
	if ( !bgmodel ) {error(4);return(0);}

	bgmodel->PCentroX = 0;
	bgmodel->PCentroY = 0;
	bgmodel->PRadio = 0;

	AllocateBGMImages(frame, bgmodel );

	// Obtencion de mascara del plato
	printf("Localizando plato... ");
	gettimeofday(&ti, NULL);
	if  ( Param->FLAT_FRAMES_TRAINING >0){
		MascaraPlato( t_capture, bgmodel, Param->FLAT_FRAMES_TRAINING);
		if (bgmodel->PRadio == 0  ) {error(3);return(0);}
	}
	else{
		bgmodel->DataFROI = cvRect(0,
		0,
		frame->width,
		frame->height ); // Datos para establecer ROI si no se quiere detectar el plato
		cvZero(bgmodel->ImFMask);
	}

	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
												(tf.tv_usec - ti.tv_usec)/1000.0;
	TiempoGlobal= TiempoGlobal + TiempoParcial ;
	printf(" %5.4g segundos\n", TiempoGlobal/1000);


	/// Acumulamos el fondo para obtener la mediana y la varianza de cada pixel en 20 frames  ////
	while( num_frames < Param->FRAMES_TRAINING ){
		frame = cvQueryFrame( t_capture );
		if ( !frame ) {
			error(2);
			break;
		}
		if ( (cvWaitKey(10) & 255) == 27 ) break;


//		int max_buffer;
//		IplImage* rawImage;

		ImPreProcess( frame, ImGray, bgmodel->ImFMask, false, bgmodel->DataFROI);

		accumulateBackground( ImGray, bgmodel->Imed ,bgmodel->IDesv, bgmodel->DataFROI, 0);

		num_frames += 1;
	}

	return bgmodel;
}

void accumulateBackground( IplImage* ImGray, IplImage* BGMod,IplImage *Ides,CvRect ROI , IplImage* mask = NULL ) {
	// si la máscara es null, se crea una inicializada a cero, lo que implicará
	// la actualización de todos los pixeles del background
	int flag = 0;
	if (mask == NULL){
		mask = cvCreateImage(cvGetSize( ImGray ), 8, 1 );
		cvZero( mask );
		flag = 1;
	}

	// Se inicializa el fondo con la primera imagen
	static int first = 1; // solo para el modelo inicial
	if ( first == 1 ){
			cvCopy( ImGray, BGMod );
	}

	cvSetImageROI( ImGray, ROI );
	cvSetImageROI( Idiff, ROI );
	cvSetImageROI( mask, ROI );
	cvSetImageROI( BGMod, ROI );
	cvSetImageROI( Ides, ROI );

	// Se estima la mediana

//	cvConvertScale(BGMod ,ImedianF,1,0); // A float
//	cvConvertScale(ImGray ,ImGrayF,1,0); // A float


//	cvConvertScale( ImedianF,BGMod,1,0); // A int
//	cvShowImage( "Foreground",mask);
//	cvWaitKey(0);
	// Se actualiza el fondo usando la máscara.

	for (int y = ROI.y; y< ROI.y + ROI.height; y++){
		uchar* ptr = (uchar*) ( ImGray->imageData + y*ImGray->widthStep + 1*ROI.x);
		uchar* ptr1 = (uchar*) ( mask->imageData + y*mask->widthStep + 1*ROI.x);
		uchar* ptr2 = (uchar*) ( BGMod->imageData + y*BGMod->widthStep + 1*ROI.x);
		for (int x = 0; x < ROI.width; x++){
			// Incrementar o decrementar fondo en una unidad
			if ( ptr1[x] == 0 ){ // si el pixel de la mascara es 0 actualizamos
				if ( ptr[x] < ptr2[x] ) ptr2[x] = ptr2[x]-1;
				if ( ptr[x] > ptr2[x] ) ptr2[x] = ptr2[x]+1;
			}
		}
	}

	//Estimamos la desviación típica
	// La primera vez iniciamos la varianza al valor Idiff, que será un valor
	//muy pequeño apropiado para fondos con poco movimiento (unimodales).

	cvAbsDiff( ImGray, BGMod, Idiff);

	if ( first == 1 ){
		cvCopy( Idiff, Ides);
		cvAbsDiff( ImGray, BGMod, Idiff);
		cvConvertScale( Idiff, Idiff, 0.25, 0);
		first = 0;
	}
	for (int y = ROI.y; y< ROI.y + ROI.height; y++){
		uchar* ptr1 = (uchar*) ( mask->imageData + y*mask->widthStep + 1*ROI.x);
		uchar* ptr3 = (uchar*) ( Idiff->imageData + y*Idiff->widthStep + 1*ROI.x);
		uchar* ptr4 = (uchar*) ( Ides->imageData + y*Ides->widthStep + 1*ROI.x);
		for (int x= 0; x<ROI.width; x++){
			if ( ptr1[x] == 0 ){
			// Incrementar o decrementar desviación típica en una unidad
				if ( ptr3[x] < ptr4[x] ) ptr4[x] = ptr4[x]-1;
				if ( ptr3[x] > ptr4[x] ) ptr4[x] = ptr4[x]+1;
			}
		}
	}

	// Corregimos la estimación de la desviación mediante la función de error
	// Así se asegura que la fracción correcta de datos esté dentro de una desvición estándar
	// ( escalado horizontal de la normal.
	cvConvertScale(Ides,Ides,0.7,0);

	cvResetImageROI( ImGray );
	cvResetImageROI( Idiff );
	cvResetImageROI( mask );
	cvResetImageROI( BGMod );
	cvResetImageROI( Ides );

	if (flag == 1) cvReleaseImage( &mask );

}

void UpdateBGModel( IplImage* tmp_frame, IplImage* BGModel,IplImage* DESVI, BGModelParams* Param, CvRect DataROI, IplImage* Mask){


	if ( Mask == NULL ) accumulateBackground( tmp_frame, BGModel,DESVI, DataROI , 0);
	else accumulateBackground( tmp_frame, BGModel,DESVI, DataROI ,Mask);

//	RunningBGGModel( tmp_frame, BGModel, DESVI, Param->ALPHA,DataROI );
//	RunningVariance

}
void RunningBGGModel( IplImage* Image, IplImage* median, IplImage* Idesv, double ALPHA,CvRect dataroi ){

	IplImage* ImTemp;
	CvSize sz = cvGetSize( Image );
	ImTemp = cvCreateImage( sz, 8, 1);

	cvCopy( Image, ImTemp);

	cvSetImageROI( ImTemp, dataroi );
	cvSetImageROI( median, dataroi );
	cvSetImageROI( Idiff, dataroi );
	cvSetImageROI( Idesv, dataroi );

	// Para la mediana
	cvConvertScale(ImTemp, ImTemp, ALPHA,0);
	cvConvertScale( median, median, (1 - ALPHA),0);
	cvAdd(ImTemp , median , median );

	// Para la desviación típica
	cvConvertScale(Idiff, ImTemp, ALPHA,0);
    cvConvertScale( Idesv, Idesv, (1 - ALPHA),0);
	cvAdd(ImTemp , Idesv , Idesv );

	cvResetImageROI( median );
	cvResetImageROI( Idiff );
	cvResetImageROI( Idesv );

	cvReleaseImage( &ImTemp );

	DeallocateTempImages();
}
void BackgroundDifference( IplImage* ImGray, IplImage* bg_model,IplImage* Ides,IplImage* fg,BGModelParams* Param, CvRect dataroi){


	cvSetImageROI( ImGray, dataroi );
	cvSetImageROI( bg_model, dataroi );
	cvSetImageROI( fg, dataroi );
	cvSetImageROI( Idiff, dataroi );
    cvSetImageROI( Ides, dataroi );


	cvZero(fg); // Iniciamos el nuevo primer plano a cero

//	setHighThreshold( bg_model, HIGHT_THRESHOLD );
//	setLowThreshold( bg_model, LOW_THRESHOLD );

//	cvInRange( ImGray, IhiF,IlowF, Imaskt);
//	cvAbsDiff( ImGray, bg_model, Idiff);
//	cvDiv( Idiff,Ides,Idiff );// Calcular |I(p)-u(p)|/0(p)

	for (int y = dataroi.y; y < dataroi.y + dataroi.height; y++){
		uchar* ptr1 = (uchar*) ( ImGray->imageData + y*ImGray->widthStep + 1*dataroi.x);
		uchar* ptr2 = (uchar*) ( bg_model->imageData + y*bg_model->widthStep + 1*dataroi.x);
		uchar* ptr3 = (uchar*) ( Idiff->imageData + y*Idiff->widthStep + 1*dataroi.x);
		uchar* ptr4 = (uchar*) ( Ides->imageData + y*Ides->widthStep + 1*dataroi.x);
		uchar* ptr5 =  (uchar*) ( fg->imageData + y*fg->widthStep + 1*dataroi.x);
		for (int x = 0; x<dataroi.width; x++){
			// Calcular (I(p)-u(p)) /0(p)
			if ( (ptr1[x]-ptr2[x]) >= 0 ) ptr3[x] = 0;
			else ptr3[x] = abs( ptr1[x]-ptr2[x]);
			// Si la desviación tipica del pixel supera en HiF veces la
			// desviación típica del modelo, el pixel se clasifica como
			//foreground ( 255 ), en caso contrario como background
			if ( ptr3[x] > Param->LOW_THRESHOLD ) ptr5[x] = 255;
			else ptr5[x] = 0;
		}
	}


//			cvCreateTrackbar( "ALPHA",
//							  "Foreground",
//							  &g_slider_position,
//							  100,
//							  onTrackbarSlide ( pos, Param ) );


	   	cvResetImageROI( ImGray );
		cvResetImageROI( bg_model );
		cvResetImageROI( fg );
		cvResetImageROI( Idiff );
		cvResetImageROI( Ides );


		FGCleanup( fg, Ides,Param, dataroi );

//	printf(" Alpha = %f\n",ALPHA);
//	cvShowImage( "Foreground",fg);
//	cvWaitKey(0);
}
void FGCleanup( IplImage* FG, IplImage* DES, BGModelParams* Param, CvRect dataroi){

	static CvMemStorage* mem_storage = NULL;
	static CvSeq* contours = NULL;

	cvSetImageROI( FG, dataroi);
	// Aplicamos morfologia: Erosión y dilatación
	if( Param->MORFOLOGIA == true){
		IplConvKernel* KernelMorph = cvCreateStructuringElementEx(3, 3, 0, 0, CV_SHAPE_ELLIPSE, NULL);
		cvMorphologyEx( FG, FG, 0, KernelMorph, CV_MOP_CLOSE, Param->CVCLOSE_ITR );
		cvMorphologyEx( FG, FG, 0, KernelMorph, CV_MOP_OPEN , Param->CVCLOSE_ITR );

		cvReleaseStructuringElement( &KernelMorph );
	}

	cvResetImageROI( FG);
	cvCopy(FG, fgTemp);
	cvZero ( FG );
	// Buscamos los contornos cuya area se encuentre en un rango determinado
	if( mem_storage == NULL ){
		mem_storage = cvCreateMemStorage(0);
	} else {
		cvClearMemStorage( mem_storage);
	}
	cvSetImageROI( fgTemp, dataroi);
	CvContourScanner scanner = cvStartFindContours(
								fgTemp,
								mem_storage,
								sizeof(CvContour),
								CV_RETR_EXTERNAL,
								CV_CHAIN_APPROX_SIMPLE,
								cvPoint(dataroi.x,dataroi.y) );
	CvSeq* c;

	int numCont = 0;
	while( (c = cvFindNextContour( scanner )) != NULL ) {
		CvSeq* c_new;
		int flag = 1;
		double area = cvContourArea( c );
		area = fabs( area );

		if ( ( (area < Param->MIN_CONTOUR_AREA)&&Param->MIN_CONTOUR_AREA > 0) ||
				( (area > Param->MAX_CONTOUR_AREA)&& Param->MAX_CONTOUR_AREA > 0) ) {
			flag = 1;
		}
		else{
			// Eliminamos los contornos que no sobrevivan al HIGHT_THRESHOLD
			// Primero obtenermos ROI del contorno del LOW_THRESHOLD
//			cvResetImageROI(Idiff);
//			cvResetImageROI(DES);


			CvRect ContROI = cvBoundingRect( c );
			cvSetImageROI( Idiff, ContROI );
			cvSetImageROI( DES , ContROI );

			for (int y = ContROI.y; y< ContROI.y + ContROI.height; y++){
				uchar* ptr3 = (uchar*) ( Idiff->imageData + y*Idiff->widthStep + 1*ContROI.x);
				uchar* ptr4 = (uchar*) ( DES->imageData + y*DES->widthStep + 1*ContROI.x);
				for (int x= 0; x<ContROI.width; x++){
					// Si alguno de los pixeles del blob supera en HiF veces la
					// desviación típica del modelo,desactivamos el flag para no
					// eliminar el contorno
					if ( ptr3[x] > Param->HIGHT_THRESHOLD*ptr4[x] ){
						flag = 0;
						break;
					}
				}
				if (flag == 0) break;
			}
			// Dibujamos el contorno
			if ( flag == 0){
				c_new = cvConvexHull2( c, mem_storage, CV_CLOCKWISE, 1); //
				cvSubstituteContour( scanner, c_new );
//				cvSetImageROI( FG, dataroi);
				cvDrawContours( FG, c, CVX_WHITE,CVX_WHITE,-1,CV_FILLED,8 );
//				cvResetImageROI( FG);
				numCont++;
			}
			else{
				cvSubstituteContour( scanner, NULL ); // eliminamos el contorno
			}
			cvResetImageROI( Idiff);
			cvResetImageROI( DES );
		}
	}
	contours = cvEndFindContours( & scanner );
	cvResetImageROI( fgTemp);


}

void MascaraPlato(CvCapture* t_capture,
				StaticBGModel* Flat,int frTrain){

	//int* centro_x, int* centro_y, int* radio
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* circles = NULL;
	IplImage* Im;

// LOCALIZACION

	int num_frames = 0;

//	t_capture = cvCaptureFromAVI( "Drosophila.avi" );
	if ( !t_capture ) {
		fprintf( stderr, "ERROR: capture is NULL \n" );
		getchar();
		return ;
	}

	// Obtención del centro de máximo radio del plato en 50 frames

//	GlobalTime = (double)cvGetTickCount() - GlobalTime;
//	printf( " %.1f\n", GlobalTime/(cvGetTickFrequency()*1000.) );

	while (num_frames < frTrain) {

		IplImage* frame = cvQueryFrame( t_capture );
		if ( !frame ) {
			fprintf( stderr, "ERROR: frame is null...\n" );
			getchar();
			break;
		}
		if ( (cvWaitKey(10) & 255) == 27 ) break;

//		VerEstadoBGModel( frame );
		Im = cvCreateImage(cvSize(frame->width,frame->height),8,1);
//
		// Imagen a un canal de niveles de gris
		cvCvtColor( frame , Im, CV_BGR2GRAY);

		// Filtrado gaussiano 5x
		cvSmooth(Im,Im,CV_GAUSSIAN,5,0,0,0);


		circles = cvHoughCircles(Im, storage,CV_HOUGH_GRADIENT,4,5,100,300, 150,
				cvRound( Im->height/2 ));
		int i;
		for (i = 0; i < circles->total; i++)
		{

			// Excluimos los radios que se salgan de la imagen
			 float* p = (float*)cvGetSeqElem( circles, i );
			 if(  (  cvRound( p[0]) < cvRound( p[2] )  ) ||
				  ( ( Im->width - cvRound( p[0]) ) < cvRound( p[2] )  ) ||
				  (  cvRound( p[1] )  < cvRound( p[2] ) ) ||
				  ( ( Im->height - cvRound( p[1]) ) < cvRound( p[2] ) )
				) continue;

			 // Buscamos el de mayor radio de entre todos los frames;
			 if (  Flat->PRadio < cvRound( p[2] ) ){
				 Flat->PRadio = cvRound( p[2] );
				 Flat->PCentroX = cvRound( p[0] );
				 Flat->PCentroY = cvRound( p[1] );
			 }
		}
		num_frames +=1;
		cvClearMemStorage( storage);
	}
	cvReleaseMemStorage( &storage );

	printf("\nPlato localizado : ");
	printf("Centro x : %d , Centro y %d : , Radio: %d \n"
			,Flat->PCentroX,Flat->PCentroY , Flat->PRadio);
	Flat->DataFROI = cvRect(Flat->PCentroX-Flat->PRadio,
					Flat->PCentroY-Flat->PRadio,
					2*Flat->PRadio,
					2*Flat->PRadio ); // Datos para establecer ROI del plato
	// Creacion de mascara

	printf("Creando máscara del plato... ");
//	GlobalTime = (double)cvGetTickCount() - GlobalTime;
//	printf( " %.1f\n", GlobalTime/(cvGetTickFrequency()*1000.) );
	for (int y = 0; y< Im->height; y++){
		//puntero para acceder a los datos de la imagen
		uchar* ptr = (uchar*) ( Flat->ImFMask->imageData + y*Flat->ImFMask->widthStep);
		// Los pixeles fuera del plato se ponen a 0;
		for (int x= 0; x<Flat->ImFMask->width; x++){
			if ( sqrt( pow( abs( x - Flat->PCentroX ) ,2 )
					+ pow( abs( y - Flat->PCentroY  ), 2 ) )
					> Flat->PRadio) ptr[x] = 255;
			else	ptr[x] = 0;
		}
	}
	cvReleaseImage(&Im);
	return;
}


//void onTrackbarSlide(pos, BGModelParams* Param) {
//   Param->ALPHA = pos / 100;
//}
void DefaultBGMParams( BGModelParams *Parameters){
    //init parameters
	 BGModelParams *Params;
	 Params = ( BGModelParams *) malloc( sizeof( BGModelParams) );
    if( Parameters == NULL )
      {
    	Params->FLAT_FRAMES_TRAINING = 0;
    	Params->FRAMES_TRAINING = 20;
    	Params->ALPHA = 0.5 ;
    	Params->MORFOLOGIA = 0;
    	Params->CVCLOSE_ITR = 1;
    	Params->MAX_CONTOUR_AREA = 1000 ;
    	Params->MIN_CONTOUR_AREA = 5;
    	Params->HIGHT_THRESHOLD = 20;
    	Params->LOW_THRESHOLD = 10;
    }
    else
    {
        Params = Parameters;
    }

}


/// localiza en memoria las imágenes necesarias para la ejecución

void AllocateBGMImages( IplImage* I , StaticBGModel* bgmodel){

	// Crear imagenes y redimensionarlas en caso de que cambien su tamaño
	AllocateTempImages( I );
	CvSize size = cvGetSize( I );

//	if( !FrameData->BGModel ||
//		 FrameData->BGModel->width != size.width ||
//		 FrameData->BGModel->height != size.height ) {

		bgmodel->Imed = cvCreateImage(size,8,1);
		bgmodel->ImFMask = cvCreateImage(size,8,1);
		bgmodel->IDesv= cvCreateImage(size,8,1);

		cvZero( bgmodel->Imed);
		cvZero( bgmodel->IDesv);
		cvZero( bgmodel->ImFMask);

}

/// Limpia de la memoria las imagenes usadas durante la ejecución
void DeallocateBGM( StaticBGModel* BGModel ){

	if(BGModel){
		DeallocateTempImages();
		cvReleaseImage(&BGModel->IDesv);
		cvReleaseImage(&BGModel->ImFMask);
		cvReleaseImage(&BGModel->Imed);
		free(BGModel);
	}
}

void AllocateTempImages( IplImage *I ) {  // I is just a sample for allocation purposes

        CvSize sz = cvGetSize( I );
        if( !Ides ||
            Ides->width != sz.width ||
        	Ides->height != sz.height ) {

        		cvReleaseImage( &ImedianF );
        		cvReleaseImage( &IvarF );
        		cvReleaseImage( &Ivar );
        		cvReleaseImage( &Ides);
        		cvReleaseImage( &IdesF );
        		cvReleaseImage( &IdiffF );
        		cvReleaseImage( &Idiff);
        		cvReleaseImage( &IhiF );
        		cvReleaseImage( &IlowF );

        		cvReleaseImage( &ImGray );
        		cvReleaseImage( &ImGrayF );
        		cvReleaseImage( &Imaskt );
        		cvReleaseImage(&fgTemp);

        		ImedianF = cvCreateImage( sz, IPL_DEPTH_32F, 1 );
				IvarF = cvCreateImage( sz, 8, 1 );
				Ivar =  cvCreateImage( sz, 8, 1 );
				Ides =  cvCreateImage( sz, 8, 1 );
				IdesF = cvCreateImage( sz, IPL_DEPTH_32F, 1 );
				IdiffF = cvCreateImage( sz, IPL_DEPTH_32F, 1 );
				Idiff = cvCreateImage( sz, 8, 1 );
				IhiF = cvCreateImage( sz, 8, 1 );
				IlowF = cvCreateImage( sz, 8, 1 );

			    ImGray = cvCreateImage( sz, 8, 1 );
				ImGrayF = cvCreateImage( sz, IPL_DEPTH_32F, 1 );
				Imaskt = cvCreateImage( sz, 8, 1 );
				fgTemp = cvCreateImage(sz, IPL_DEPTH_8U, 1);

				cvZero( ImedianF );
				cvZero( IvarF );
				cvZero( Ivar);
				cvZero( IdesF );
				cvZero( Ides );
				cvZero( IdiffF );
				cvZero( Idiff );
				cvZero( IhiF );
				cvZero( IlowF );
				cvZero(ImGray);
				cvZero(ImGrayF);
				cvZero(Imaskt);
				cvZero(fgTemp);
        	}
}

void DeallocateTempImages() {

		cvReleaseImage( &ImedianF );
        cvReleaseImage( &IvarF );
        cvReleaseImage( &Ivar );

        cvReleaseImage( &IdesF );
        cvReleaseImage( &Ides );

        cvReleaseImage( &IdiffF );
        cvReleaseImage( &Idiff );

        cvReleaseImage( &IhiF );
        cvReleaseImage( &IlowF );
        cvReleaseImage( &Imaskt );
        cvReleaseImage( &ImGray );
        cvReleaseImage( &ImGrayF );
        cvReleaseImage( &fgTemp );

 		ImedianF = NULL;
			IvarF = NULL;
			Ivar =  NULL;
			Ides = NULL;
			IdesF = NULL;
			IdiffF =NULL;
			Idiff = NULL;
			IhiF = NULL;
			IlowF = NULL;

		    ImGray = NULL;
			ImGrayF = NULL;
			Imaskt = NULL;
			fgTemp = NULL;
}









