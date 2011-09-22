/*
 * validacion.cpp
 *
 *  Created on: 22/09/2011
 *      Author: chao
 */

#include "validacion.hpp"

void validacion(IplImage *Imagen, STCapas* Capa, CvRect Segroi){

	// Recorremos los blobs uno por uno y los validamos. La función es recursiva.

	static CvMemStorage* mem_storage = NULL;
	static CvSeq* contours = NULL;

	if( mem_storage == NULL ){
		mem_storage = cvCreateMemStorage(0);
	} else {
		cvClearMemStorage( mem_storage);
	}
	CvContourScanner scanner = cvStartFindContours(
			Capa->FG, mem_storage, sizeof(CvContour),CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );
	CvSeq* c;

	int numCont = 0;
	while( (c = cvFindNextContour( scanner )) != NULL ) {
		CvSeq* c_new;
		CvRect ContROI = cvBoundingRect( c );
		//			cvSetImageROI( Idiff, ContROI );
		//			cvSetImageROI( DES , ContROI );

			for (int y = ContROI.y; y< ContROI.y + ContROI.height; y++){
				uchar* ptr3 = (uchar*) ( Idiff->imageData + y*Idiff->widthStep + 1*ContROI.x);
				uchar* ptr4 = (uchar*) ( DES->imageData + y*DES->widthStep + 1*ContROI.x);
				for (int x= 0; x<ContROI.width; x++){
					// Si alguno de los pixeles del blob supera en HiF veces la
					// desviación típica del modelo,desactivamos el flag para no
					// eliminar el contorno
					if ( ptr3[x] > HIGHT_THRESHOLD*ptr4[x] ){
						flag = 0;
						break;
					}
				}
				if (flag == 0) break;
			}
		//			cvResetImageROI( Idiff);
		//			cvResetImageROI( DES );
				}
}
