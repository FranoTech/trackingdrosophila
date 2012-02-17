/*
 * validacion.cpp
 *
 *  Created on: 22/09/2011
 *      Author: chao
 *
 *  Esta función se encarga de validar los datos de la estructura rellenada por segmentación
 *
 */
// OJO en limpieza de foreground se hace un filtrado por area

// Inicialmente implementar solo el aumento y disminucion del umbral sin casos especiales

#include "validacion.hpp"



////////////////// VALIDACION /////////////////

void Validacion2(IplImage *Imagen,
		STFrame* FrameData,
		SHModel* SH,
		IplImage* Mask,
		ValParams* Params){


	//Inicializar estructura para almacenar los datos cada mosca
	STFly *FlyData = NULL;
	BGModelParams* ValBGParams = NULL;

	int Exceso;//Flag que indica si la Pxi minima no se alcanza por exceso o por defecto.
	double Circul; // Para almacenar la circularidad del blob actual

	double Besthres=0;// el mejor de los umbrales perteneciente a la mejor probabilidad
	double BestPxi=0; // la mejor probabilidad obtenida para cada mosca

	// Establecemos los parámetros de validación por defecto
	if ( Params == NULL ){
		iniciarValParams( &Params, SH);
	}

	// Establecemos parámetros para los umbrales en la resta de fondo por defecto
	if ( ValBGParams == NULL){
		iniciarBGModParams( &ValBGParams);
	}

	// almacenar una copia del FG para restaurar el blob en caso de que falle la validación
	cvCopy( FrameData->FG, Mask );

	// Recorremos los blobs uno por uno y los validamos.
	// Bucle for desde el primero hasta el ultimo individuo de la Lista FrameData->Flie del frame actual.
	// Los blobs resultantes de la fisión se añaden al final para ser validados de nuevo
	if(SHOW_VALIDATION_DATA) mostrarFliesFrame( FrameData );
	for(int i=0;i<FrameData->Flies->numeroDeElementos;i++){

		// Almacenar la Roi del blob visitado antes de ser validado
		FlyData=(STFly *)obtener(i, FrameData->Flies);
		CvRect FlyDataRoi=FlyData->Roi;
		// Inicializar LOW_THRESHOLD y demas valores
		setBGModParams( ValBGParams);
		FlyData->bestTresh = ValBGParams->LOW_THRESHOLD;

		// calcular su circularidad
		Circul = CalcCircul( FlyData );

		// Comprobar si existe Exceso o Defecto de area, en caso de Exceso la probabilidad del blob es muy pequeña
		// y el blob puede corresponder a varios blobs que forman parte de una misma moscas o puede corresponder a
		// un espurio.
		Exceso = CalcProbFly( SH ,FlyData, Params); // Calcular la px de la mosca.
		BestPxi  =	FlyData->Px;
		STFly* MejorFly = FlyData;// = NULL;
		// comprobar si está dentro de los límites:

		//	Params->MaxLowTH = *ObtenerMaximo(Imagen, FrameData ,FlyData->Roi);

		// En caso de EXCESO de Area
		/* Incrementar paulatinamente el umbral de resta de fondo hasta
		 * que haya fisión o bien desaparzca el blob sin fisionarse o bien
		 * no se supere la pximin.
		 * - Si hay fisión:
		 * 		-si los blobs tienen una probabilidad mayor que la pximin fin segmentacion
		 * 		-si alguno de ellos tiene una probabilidad menor a la pximin, restaurar original y aplicar EM
		 * - Si no hay fisión: Si el blob desaparece antes de alcanzar la pximin o
		 *  bien alcanza la pximin, restablecer el tamaño original del blob para aplicar EM
		 *
		 */
		if( Exceso > 0){
			//Inicializar estructura para almacenar la lista de los blobs divididos.
			tlcde* TempSeg=NULL;

			setBGModParams( ValBGParams);
			// sustituir por un for con el número máximo de iteraciones.
			while( !FlyData->flag_seg && !FlyData->failSeg ){
				// primero probar con el umbral anterior. si el blob desaparece o no da un resutado satisfactorio
				// proceder desde el umbral predetermindado

				// Incrementar umbral
				ValBGParams->LOW_THRESHOLD += 1;

				// Restar fondo
				BackgroundDifference( Imagen, FrameData->BGModel,FrameData->IDesvf,FrameData->FG,ValBGParams, FlyDataRoi);
				//verMatrizIm(FrameData->FG, FlyDataRoi);
				// Segmentar
				TempSeg = segmentacion2(Imagen, FrameData->BGModel,FrameData->FG, FlyDataRoi, NULL);

				if( SHOW_VALIDATION_DATA){
						cvShowImage( "Foreground",FrameData->FG);
						cvWaitKey(0);
				}
				// Comprobar si se ha conseguido dividir el blob y verificar si el resultado es optimo

				// 1) No hay fisión. El blob desaparece antes de alcanzar la pximin. Restauramos el blob y pasamos al siguiente elemento.
				if(TempSeg->numeroDeElementos < 1 || TempSeg == NULL)	{
					RestaurarElMejor( FrameData->Flies,  FrameData->FG, MejorFly,  FlyDataRoi, i );
					if( SHOW_VALIDATION_DATA){
							cvShowImage( "Foreground",FrameData->FG);
							cvWaitKey(0);
					}
					// liberar lista
					liberarListaFlies( TempSeg );
					break;
				}

				// 2) No hay fisión: Almacenar la probabilidad mas alta, así como el umbral correspondiente a esa probabilidad
				// y continuar incrementando el umbral en caso de no rebasar la pximin.
				else if(  TempSeg->numeroDeElementos == 1 ) {

					FlyData=(STFly *)obtener(0, TempSeg);
					Exceso = CalcProbFly( SH , FlyData, Params);
					// no se rebasa la pximin continuar
					if (Exceso > -1 ){
						if(FlyData->Px > BestPxi ){
							MejorFly = FlyData;
							BestPxi=FlyData->Px;
							Besthres = ValBGParams->LOW_THRESHOLD;
						}
						liberarListaFlies( TempSeg );
						continue;
					}
					// se ha rebasado la pxmin. Restaurar y pasar al siguiente
					else {
						RestaurarElMejor( FrameData->Flies,  FrameData->FG, MejorFly,  FlyDataRoi, i );
						//Restaurar(FrameData->Flies, FrameData->FG, Mask, FlyDataRoi, i);
						liberarListaFlies( TempSeg );
						if( SHOW_VALIDATION_DATA){
								cvShowImage( "Foreground",FrameData->FG);
								cvWaitKey(0);
						}
						break;
					}
				}
				// 3) Hay fisión
				else if ( TempSeg->numeroDeElementos > 1 ){
					// comprobar si las pxi son válidas. En caso de que todas lo sean eliminar de la lista el blob segmentado
					// y añadirlas a la lista al final.
					int exito = comprobarPxi( TempSeg, SH, Params);
					if (exito){
						borrarEl(i,FrameData->Flies); // se borra el elemento dividido
						for(int j = 0; j < TempSeg->numeroDeElementos; j++){ // se añaden los nuevos al final
							FlyData=(STFly *)obtener(j, TempSeg);
							anyadirAlFinal(FlyData, FrameData->Flies );
						}
						printf("\n Lista Flies tras FISION\n");
						if(SHOW_VALIDATION_DATA) mostrarFliesFrame( FrameData );
						i--; // decrementar i para no saltarnos un elemento de la lista
					}
					// Se ha rebasado la pximin en alguno de los blobs resultantes. Restauramos y pasamos al sig. blob
					else{
						liberarListaFlies( TempSeg );
						Restaurar(FrameData->Flies, FrameData->FG, Mask, FlyDataRoi, i);
					}
				}
				free( TempSeg );
				TempSeg = NULL;
			}// Fin del While
			//liberarListaFlies( TempSeg );
			if(TempSeg) free( TempSeg );
			TempSeg = NULL;
		} //Fin Exceso

		/* En el caso de DEFECTO disminuir el umbral, para maximizar la probabilidad del blob
		 * Si no es posible se elimina (spurio)
		 */

		else if ( Exceso < 0 ){

			STFly* MejorFly = NULL;
			CvRect newRoi;
			tlcde* TempSeg=NULL;

			setBGModParams( ValBGParams);
			newRoi = FlyDataRoi;
			BestPxi = 0;
			int j=0 ;
			while( j< Params->MaxDecLTHIters ){
				newRoi.height +=1;
				newRoi.width +=1;
				newRoi.x-=1;
				newRoi.y-=1;
				ValBGParams->LOW_THRESHOLD -=1;
				if(ValBGParams->LOW_THRESHOLD == Params->MinLowTH){
					Restaurar(FrameData->Flies, FrameData->FG, Mask, FlyDataRoi, i);
					break;
				}

			//while(Exceso < 0 && ValBGParams->LOW_THRESHOLD > Params->MinLowTH && Params->MaxDecLTHIters){

				// Restar fondo
				BackgroundDifference( Imagen, FrameData->BGModel,FrameData->IDesvf,FrameData->FG,ValBGParams, newRoi);

				// Segmentar
				TempSeg = segmentacion2(Imagen, FrameData->BGModel,FrameData->FG, newRoi, NULL);
				//TempSeg = segmentacion(Imagen, FrameData, newRoi,NULL);
				if( SHOW_VALIDATION_DATA){
					cvShowImage( "Foreground",FrameData->FG);
					cvWaitKey(0);
				}
				if (TempSeg == NULL||TempSeg->numeroDeElementos <1) continue;
				// Calcular la pxi
				FlyData=(STFly *)obtener(0, TempSeg);
				Exceso = CalcProbFly( SH ,FlyData, Params); // Calcular la px de la mosca.

				// mientras la probabilidad se incremente continuamos decrementando el umbral
				j++;
				if(FlyData->Px >= BestPxi ){
					MejorFly = (STFly *)obtener(0, TempSeg);
					BestPxi=FlyData->Px;
					Besthres=ValBGParams->LOW_THRESHOLD;
				}
				else if(FlyData->Px < BestPxi || j == Params->MaxDecLTHIters){
					// cuando deje de mejorar o bien se llegue al máximo de iteraciones
					//nos quedamos con aquel que dio la mejor probabilidad. Lo insertamos en la lista en la misma posición.
					if( SHOW_VALIDATION_DATA){
							cvShowImage( "Foreground",FrameData->FG);
							cvWaitKey(0);
					}
					sustituirEl( MejorFly, FrameData->Flies, i);
					break;
				}
				liberarListaFlies( TempSeg );
				if(TempSeg) {
					free( TempSeg );
					TempSeg = NULL;
				}

			}// FIN WHILE
			liberarListaFlies( TempSeg );
			if(TempSeg) free( TempSeg );

		}// FIN DEFECTO

	}//FOR

}//Fin de Validación

tlcde* ValidarBLOB(IplImage *Imagen,
		STFrame* FrameData,
		SHModel* SH,
		CvRect Segroi,
		IplImage* Mask){

	//Inicializar estructura para almacenar los datos cada mosca

	STFly *FlyData = NULL;
	//Inicializar estructura para almacenar los datos cada mosca
	BGModelParams* ValBGParams;
	ValParams* Params;

	double Pxi; // La probabilidad del blob actual
	bool Exceso;//Flag que indica si la Pxi minima no se alcanza por exceso o por defecto.
	double Circul; // Para almacenar la circularidad del blob actual

	double Besthres=0;// el mejor de los umbrales perteneciente a la mejor probabilidad
	double BestPxi=0; // la mejor probabilidad obtenida para cada mosca

	// Establecemos los parámetros de validación por defecto
	if ( Params == NULL ){
		iniciarValParams( &Params, SH);
	}

	// Establecemos parámetros para los umbrales en la resta de fondo por defecto
	if ( ValBGParams == NULL){
		iniciarBGModParams( &ValBGParams);
	}

	// almacenar una copia del BLOB para restaurar el blob en caso de que falle la validación
	cvSetImageROI(Mask, Segroi);
	cvSetImageROI(FrameData->FG, Segroi);
	cvCopy( Mask, FrameData->FG);
	cvResetImageROI(Mask);
	cvResetImageROI(FrameData->FG);
	cvCopy( FrameData->FG, Mask );

		// Inicializar LOW_THRESHOLD y demas valores

		Besthres=ValBGParams->LOW_THRESHOLD;

		CvRect FlyDataRoi= Segroi;

		// calcular su probabilidad
		FlyData->Px = CalcProbFly( SH ,FlyData, Params);
		BestPxi  =	FlyData->Px;

		// calcular su circularidad
		Circul = CalcCircul( FlyData );

		// Comprobar si existe Exceso o Defecto de area, en caso de Exceso la probabilidad del blob es muy pequeña
		// y el blob puede corresponder a varios blobs que forman parte de una misma moscas o puede corresponder a
		// un espurio.
		Exceso = CalcProbFly( SH ,FlyData, Params); // Calcular la px de la mosca.
		// comprobar si está dentro de los límites:
		if( Exceso && FlyData->Px > Params->PxiMax) Exceso = 1; // exceso
		else if( !Exceso && FlyData->Px < Params->PxiMin ) Exceso = -1; // defecto
		else Exceso = 0; // dentro de los límites
		//	Params->MaxLowTH = *ObtenerMaximo(Imagen, FrameData ,FlyData->Roi);

		// En caso de EXCESO de Area
		/* Incrementar paulatinamente el umbral de resta de fondo hasta
		 * que haya fisión o bien desaparzca el blob sin fisionarse o bien
		 * no se supere la pximin.
		 * - Si hay fisión:
		 * 		-si los blobs tienen una probabilidad mayor que la pximin fin segmentacion
		 * 		-si alguno de ellos tiene una probabilidad menor a la pximin, restaurar original y aplicar EM
		 * - Si no hay fisión: Si el blob desaparece antes de alcanzar la pximin o
		 *  bien alcanza la pximin, restablecer el tamaño original del blob para aplicar EM
		 *
		 */
		if( Exceso){
			//Inicializar estructura para almacenar la lista de los blobs divididos.
			tlcde* TempSeg=NULL;

			setBGModParams( ValBGParams);
			// sustituir por un for con el número máximo de iteraciones.
			while( !FlyData->flag_seg && !FlyData->failSeg ){


				// Incrementar umbral
				ValBGParams->LOW_THRESHOLD += 1;

				// Restar fondo
				BackgroundDifference( Imagen, FrameData->BGModel,FrameData->IDesvf,FrameData->FG,ValBGParams, Segroi);

				// Segmentar
				TempSeg = segmentacion2(Imagen, FrameData->BGModel,FrameData->FG, Segroi, NULL);

				// Comprobar si se ha conseguido dividir el blob y verificar si el resultado es optimo

				// 1) No hay fisión. El blob desaparece antes de alcanzar la pximin. Restauramos el blob y pasamos al siguiente elemento.
				if(TempSeg->numeroDeElementos < 1){
					FlyData=(STFly *)obtener(0,TempSeg);
					FlyData->failSeg = true;
					// restaurar
					cvSetImageROI(Mask, FlyDataRoi);
					cvSetImageROI(FrameData->FG, FlyDataRoi);
					cvCopy( Mask, FrameData->FG);
					cvResetImageROI(Mask);
					cvResetImageROI(FrameData->FG);
					if( ValBGParams) free(ValBGParams);
					return TempSeg;
				}
				// 2) No hay fisión: Almacenar la probabilidad mas alta, así como el umbral correspondiente a esa probabilidad
				// y continuar incrementando el umbral en caso de no rebasar la pximin.
				else if(  TempSeg->numeroDeElementos == 1 ) {
					FlyData=(STFly *)obtener(0, TempSeg);
					Exceso = CalcProbFly( SH , FlyData, Params );
					if (!Exceso ){
						if(FlyData->Px > Params->PxiMin) continue;

						else { // no hay fisión y se ha rebasado la pxmin. Restaurar
							FlyData=(STFly *)obtener(0,TempSeg);
							FlyData->failSeg = true;
							// restaurar
							cvSetImageROI(Mask, FlyDataRoi);
							cvSetImageROI(FrameData->FG, FlyDataRoi);
							cvCopy( Mask, FrameData->FG);
							cvResetImageROI(Mask);
							cvResetImageROI(FrameData->FG);
							if( ValBGParams) free(ValBGParams);
							return TempSeg;

						}
					}
					if(FlyData->Px > BestPxi ){
						BestPxi=FlyData->Px;
						Besthres=ValBGParams->LOW_THRESHOLD;
					}
				}
				// 3) Hay fisión
				else if ( TempSeg->numeroDeElementos > 1 ){
					// comprobar si las pxi son válidas.
					for(int j = 0; j < TempSeg->numeroDeElementos; j++){
						// Calcular Pxi del/los blob/s resultante/s
						FlyData=(STFly *)obtener(j, TempSeg);
						Pxi = CalcProbFly( SH , FlyData, Params );
						if (FlyData->Px < Params->PxiMin )
							FlyData->flag_seg = true;
						else break;
					}
					// si el último elemento es correcto, todos son correctos.
					FlyData=(STFly *)obtener(TempSeg->numeroDeElementos-1, TempSeg);
					if (FlyData->flag_seg){
						if( ValBGParams) free(ValBGParams);
						return TempSeg;
					}
					// Se ha rebasado la pximin en alguno de los blobs resultantes. Restauramos
					else{
						FlyData->failSeg = true;
						cvSetImageROI(Mask, FlyDataRoi);
						cvSetImageROI(FrameData->FG, FlyDataRoi);
						cvCopy( Mask, FrameData->FG);
						cvResetImageROI(Mask);
						cvResetImageROI(FrameData->FG);
						if( ValBGParams) free(ValBGParams);
						return TempSeg;
					}

				}

			}// Fin del While

			if(ValBGParams) free( ValBGParams);

		} //Fin Exceso

		/* En el caso de DEFECTO disminuir el umbral, hasta que la probabilidad del blob Pxi sea inferior
		 * al Umbral maximo y si este no ha desaparecido al disminuir el umbral, validar el blob como
		 * mosca y mantener su posición dentro de la cola, en el caso de que el blob desaparezca durante
		 * la disminución del umbral o antes de que el umbral alcance el valor de cero validar como NO
		 * mosca y poner el failSeg a true para eliminar el blob de la cola.
		 */

		else if ( Exceso < 0 ){

			tlcde* TempSeg=NULL;
			setBGModParams( ValBGParams);

			for( int j = 0;
					ValBGParams->LOW_THRESHOLD == Params->MinLowTH, j< Params->MaxDecLTHIters;
					j++, ValBGParams->LOW_THRESHOLD -=1 ){
			//while(Exceso < 0 && ValBGParams->LOW_THRESHOLD > Params->MinLowTH && Params->MaxDecLTHIters){

				// Restar fondo
				BackgroundDifference( Imagen, FrameData->BGModel,FrameData->IDesvf,FrameData->FG,ValBGParams, Segroi);

				// Segmentar
				TempSeg = segmentacion2(Imagen, FrameData->BGModel,FrameData->FG, Segroi, NULL);
				// Calcular la pxi
				Exceso = CalcProbFly( SH ,FlyData, Params); // Calcular la px de la mosca.
				// comprobar si está dentro de los límites:
				if( Exceso && FlyData->Px > Params->PxiMax) Exceso = 1; // exceso
				else if( !Exceso && FlyData->Px < Params->PxiMin ) Exceso = -1; // defecto
				else Exceso = 0; // dentro de los límites
				// si la probabilidad mejora actualizamos la lista con el blob
				if(Pxi > BestPxi ){
					FlyData = (STFly *)obtener(0, TempSeg);
					BestPxi=Pxi;
					Besthres=ValBGParams->LOW_THRESHOLD;
				}
				// nos quedamos con el blob que dio la mejor probabilidad. Lo insertamos en la lista en la misma posición.
				else {
					if( ValBGParams) free(ValBGParams);
					return TempSeg;
				}

			}
			if( ValBGParams) free(ValBGParams);
		}// FIN DEFECTO
}//Fin de Validación


// Inicializar los Parametros para la validación

void iniciarValParams( ValParams** Parameters, SHModel* SH){

	ValParams* Params;
	Params = ( ValParams *) malloc( sizeof( ValParams) );
	if( *Parameters == NULL )
	{
		Params->UmbralCirc = 0;
		Params->Umbral_H = 5;		// Establece el tamaño máximo del blob válido en 3 desviaciones típicas de la media
		Params->PxiMax= CalcPxMax( SH, Params->Umbral_H );			 // establece la máxima probabilidad permitida para defecto.
		Params->Umbral_L = 1.5;			 // Establece el tamaño mínimo del blob válido en 3 desviaciones típicas de la media
		Params->PxiMin = CalcPxMin( SH, Params->Umbral_L );			 // establece la mínima probabilidad permitida para exceso.
		Params->MaxDecLTHIters = 15; // número máximo de veces que se podrá decrementar el umbral bajo. Ojo:el valor
		Params->MaxIncLTHIters= 20;  // número máximo de veces que se podrá incrementar el umbral bajo
		Params->MaxLowTH = 20; 		 // límite superior para el umbral bajo ( exceso )
		Params->MinLowTH = 1;		 // límite inferior para el umbral bajo ( defecto )
		Params->UmbralProb = 3;

//		if (CREATE_TRACKBARS == 1){
//			if(first==1){
//				cvCreateTrackbar("UmbralProb","Foreground",&Params->UmbralProb,1);
//				cvCreateTrackbar("UmbralCirc","Foreground",&Params->UmbralCirc,1);
//				first=0;
//			}
//		}
	}
	else
	{
		Params=*Parameters;
	}

	*Parameters=Params;
}



//Inicializar los parametros para el modelado de fondo

void iniciarBGModParams( BGModelParams** Parameters){
	BGModelParams* Params;
	Params = ( BGModelParams *) malloc( sizeof( BGModelParams) );
	if( *Parameters == NULL )
	{
		Params->FRAMES_TRAINING = 0;
		Params->ALPHA = 0 ;
		Params->MORFOLOGIA = 0;
		Params->CVCLOSE_ITR = 0;
		Params->MAX_CONTOUR_AREA = 0 ;
		Params->MIN_CONTOUR_AREA = 0;
		Params->HIGHT_THRESHOLD = 20;
		Params->LOW_THRESHOLD = 15;
	}
	else	Params=*Parameters;

	*Parameters=Params;
}

void setBGModParams( BGModelParams* Params){

		Params->FRAMES_TRAINING = 0;
		Params->ALPHA = 0 ;
		Params->MORFOLOGIA = 0;
		Params->CVCLOSE_ITR = 0;
		Params->MAX_CONTOUR_AREA = 0 ;
		Params->MIN_CONTOUR_AREA = 0;
		Params->HIGHT_THRESHOLD = 20;
		Params->LOW_THRESHOLD = 15;

}
//Probabilidad de todas las moscas

double CalcProbTotal(tlcde* Lista,SHModel* SH,ValParams* Params,STFly* FlyData){

	double PX=1;
	double Pxi=0;

	for(int i=0;i < Lista->numeroDeElementos;i++){

		FlyData=(STFly *)obtener(i, Lista);
		CalcProbFly( SH, FlyData,Params);
		PX=PX*Pxi;
	}

	return PX;
}

// Probabilidad de cada mosca

int CalcProbFly( SHModel* SH , STFly* FlyData, ValParams* Params ){
	int Exceso;
	FlyData->Px = exp( -abs( FlyData->areaElipse - SH->FlyAreaMedia) / SH->FlyAreaDes);
	//Si el area del blob es menor que la media, el error es por defecto,
	//si es mayor que la media el error será por exceso.
	if( Exceso && FlyData->Px < Params->PxiMax) Exceso = 1; // exceso
	else if( !Exceso && FlyData->Px < Params->PxiMin ) Exceso = -1; // defecto
	else Exceso = 0; // dentro de los límites
	return Exceso;


}

// Circularidad de cada mosca

double CalcCircul( STFly* FlyData){
	double circularidad;
	circularidad = ( 4*CV_PI*FlyData->a*FlyData->b*CV_PI)/(FlyData->perimetro*FlyData->perimetro);
	return circularidad;
}

// Probabilidades y  Umbrales

float CalcPxMin( SHModel* SH,float Umbral_L ){

	float area_L;	// area minima permitida. establece la probabilidad del umbral bajo.
	double Pth_L;	// Probabilidad correspondiente a dicha area.

	area_L = SH->FlyAreaMed - Umbral_L*SH->FlyAreaDes  ;
	Pth_L = exp( -abs(area_L - SH->FlyAreaMed) / SH->FlyAreaDes);

	return Pth_L;

}

float CalcPxMax( SHModel* SH,float Umbral_H ){

	float area_H;   // area que establece el umbral alto.
	double Pth_H;	// Probabilidad correspondiente al umbral alto

	area_H =  SH->FlyAreaMedia + Umbral_H*SH->FlyAreaDes ;
	Pth_H = exp( -abs(area_H - SH->FlyAreaMedia) / SH->FlyAreaDes);

	return Pth_H;

}



// Obtener la maxima distancia de los pixeles al fondo

double* ObtenerMaximo(IplImage* Imagen, STFrame* FrameData,CvRect Roi ){
	// obtener matriz de distancias normalizadas al background
	if (SHOW_VALIDATION_DATA == 1) printf(" \n\n Busqueda del máximo umbral...");
	IplImage* IDif = 0;
	IplImage* peso=0;
	CvSize size = cvSize(Imagen->width,Imagen->height); // get current frame size
	if( !IDif || IDif->width != size.width || IDif->height != size.height ) {
		cvReleaseImage( &IDif );
		cvReleaseImage( &peso );
		IDif=cvCreateImage(cvSize(FrameData->BGModel->width,FrameData->BGModel->height), IPL_DEPTH_8U, 1); // imagen diferencia abs(I(pi)-u(p(i))
		peso=cvCreateImage(cvSize(FrameData->BGModel->width,FrameData->BGModel->height), IPL_DEPTH_32F, 1);//Imagen resultado wi ( pesos)
		cvZero( IDif);
		cvZero(peso);
	}

	// |I(p)-u(p)|/0(p)
	cvAbsDiff(Imagen,FrameData->BGModel,IDif);
	cvDiv(IDif,FrameData->IDesvf,peso);

	// Buscar máximo
	double* Maximo=0;
	cvMinMaxLoc(peso,Maximo,0,0,0,FrameData->FG);

	return Maximo;
}

int comprobarPxi( tlcde* TempSeg,SHModel* SH, ValParams* Params){
	STFly* FlyData;
	int Exceso;
	for(int j = 0; j < TempSeg->numeroDeElementos; j++){
		// Calcular Pxi del/los blob/s resultante/s
		FlyData=(STFly *)obtener(j, TempSeg);
		 Exceso = CalcProbFly( SH , FlyData, Params );
		if ( Exceso >= 0  )	FlyData->flag_seg = true;
		else break;
	}
	// si el último elemento es correcto, todos son correctos.
	FlyData=(STFly *)obtener(TempSeg->numeroDeElementos-1, TempSeg);
	if (FlyData->flag_seg) return 1;
	else return 0;
}

void Restaurar(tlcde* lista, IplImage* Fg, IplImage* Origin, CvRect Roi, int elemento){
	STFly* FlyData;
	FlyData=(STFly *)obtener(elemento,lista);
	FlyData->failSeg = true;
	// restaurar
	cvSetImageROI(Origin, Roi);
	cvSetImageROI(Fg, Roi);
	cvCopy( Origin,Fg);
	cvResetImageROI(Origin);
	cvResetImageROI(Fg);
}

void RestaurarElMejor(tlcde* lista, IplImage* Fg, STFly* MejorFly, CvRect Roi, int elemento ){

	// sustituir el blob de la lista por el mejor
	MejorFly->failSeg = true;
	sustituirEl( MejorFly, lista, elemento);
	// restaurar el Fg
	dibujarBlob( MejorFly, Fg);
}


