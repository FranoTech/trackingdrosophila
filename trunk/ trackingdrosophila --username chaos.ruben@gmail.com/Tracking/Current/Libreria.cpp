/*!
 * Libreria.cpp
 *
 *  Created on: 12/07/2011
 *      Author: chao
 */

#include "Libreria.h"

// Tratamiento de Imagenes

int getAVIFrames(char * fname) {
	char tempSize[4];
	// Trying to open the video file
	ifstream  videoFile( fname , ios::in | ios::binary );
	// Checking the availablity of the file
	if ( !videoFile ) {
		cout << "Couldn’t open the input file " << fname << endl;
		exit( 1 );
	}
	// get the number of frames
	videoFile.seekg( 0x30 , ios::beg );
	videoFile.read( tempSize , 4 );
	int frames = (unsigned char ) tempSize[0] + 0x100*(unsigned char ) tempSize[1] + 0x10000*(unsigned char ) tempSize[2] +    0x1000000*(unsigned char ) tempSize[3];
	videoFile.close(  );
	return frames;
}

void invertirBW( IplImage* Imagen )
{
	//añadir para invertir con  roi
	if (Imagen->roi){
		for( int y=Imagen->roi->yOffset;  y< Imagen->roi->yOffset + Imagen->roi->height ; y++){

				uchar* ptr = (uchar*) ( Imagen->imageData + y*Imagen->widthStep + 1*Imagen->roi->xOffset);

				for (int x = 0; x<Imagen->roi->width; x++) {
					if (ptr[x] == 255) ptr[x] = 0;
					else ptr[x] = 255;
				}
		}
	}
	else{
		for( int y=0;  y< Imagen->height ; y++){
				uchar* ptr = (uchar*) ( Imagen->imageData + y*Imagen->widthStep);

				for (int x = 0; x<Imagen->width; x++) {
					if (ptr[x] == 255) ptr[x] = 0;
					else ptr[x] = 255;
				}
		}
	}
}

// Recibe la imagen del video y devuelve la imagen en un canal de niveles
// de gris con el plato extraido.

void ImPreProcess( IplImage* src,IplImage* dst, IplImage* ImFMask,bool bin, CvRect ROI){

//cvNamedWindow( "Im", CV_WINDOW_AUTOSIZE);
	// Imagen a un canal de niveles de gris
	cvCvtColor( src, dst, CV_BGR2GRAY);
	cvSetImageROI( dst, ROI );

	if (bin == true){
		cvAdaptiveThreshold( dst, dst,
			255, CV_ADAPTIVE_THRESH_MEAN_C,CV_THRESH_BINARY,75,40);
		//bin = false;
	}
// Filtrado gaussiano 5x

	cvSmooth(dst,dst,CV_GAUSSIAN,5,5);

// Extraccion del plato

	cvResetImageROI( dst );
	cvAndS(dst, cvRealScalar( 0 ) , dst, ImFMask );
}


///////////////////// INTERFAZ PARA MANIPULAR UNA LCDE //////////////////////////////
//


// Crear un nuevo elemento de la lista

Elemento *nuevoElemento()
{
  Elemento *q = (Elemento *)malloc(sizeof(Elemento));
  if (!q) error(4);
  return q;
}

void iniciarLcde(tlcde *lcde)
{
  lcde->ultimo = lcde->actual = NULL;
  lcde->numeroDeElementos = 0;
  lcde->posicion = -1;
}

void insertar(void *e, tlcde *lcde)
{
  // Obtener los parámetros de la lcde
  Elemento *ultimo = lcde->ultimo;
  Elemento *actual = lcde->actual;
  int numeroDeElementos = lcde->numeroDeElementos;
  int posicion = lcde->posicion;

  // Añadir un nuevo elemento a la lista a continuación
  // del elemento actual; el nuevo elemento pasa a ser el
  // actual
  Elemento *q = NULL;

  if (ultimo == NULL) // lista vacía
  {
    ultimo = nuevoElemento();
    // Las dos líneas siguientes inician una lista circular
    ultimo->anterior = ultimo;
    ultimo->siguiente = ultimo;
    ultimo->dato = e;      // asignar datos
    actual = ultimo;
    posicion = 0;          // ya hay un elemento en la lista
  }
  else // existe una lista
  {
    q = nuevoElemento();

    // Insertar el nuevo elemento después del actual
    actual->siguiente->anterior = q;
    q->siguiente = actual->siguiente;
    actual->siguiente = q;
    q->anterior = actual;
    q->dato = e;

    // Actualizar parámetros.
    posicion++;

    // Si el elemento actual es el último, el nuevo elemento
    // pasa a ser el actual y el último.
    if( actual == ultimo )
      ultimo = q;

    actual = q; // el nuevo elemento pasa a ser el actual
  } // fin else

  numeroDeElementos++; // incrementar el número de elementos

  // Actualizar parámetros de la lcde
  lcde->ultimo = ultimo;
  lcde->actual = actual;
  lcde->numeroDeElementos = numeroDeElementos;
  lcde->posicion = posicion;
}


void *borrarEl(int i,tlcde *lcde)
{
	int n = 0;
	// comprobar si la posicion está en los limites
	if ( i >= lcde->numeroDeElementos || i < 0 ) return NULL;
	// posicionarse en el elemento i
	irAlPrincipio( lcde );
	for( n = 0; n < i; n++ ) irAlSiguiente( lcde );
	// borrar
	  // Obtener los parámetros de la lcde
	  Elemento *ultimo = lcde->ultimo;
	  Elemento *actual = lcde->actual;
	  int numeroDeElementos = lcde->numeroDeElementos;
	  int posicion = lcde->posicion;

	  // La función borrar devuelve los datos del elemento
	  // apuntado por actual y lo elimina de la lista.
	  Elemento *q = NULL;
	  void *datos = NULL;

	  if( ultimo == NULL ) return NULL;  // lista vacía.
	  if( actual == ultimo ) // se trata del último elemento.
	  {
	    if( numeroDeElementos == 1 ) // hay un solo elemento
	    {
	      datos = ultimo->dato;
	      free(ultimo);
	      ultimo = actual = NULL;
	      numeroDeElementos = 0;
	      posicion = -1;
	    }
	    else // hay más de un elemento
	    {
	      actual = ultimo->anterior;
	      ultimo->siguiente->anterior = actual;
	      actual->siguiente = ultimo->siguiente;
	      datos = ultimo->dato;
	      free(ultimo);
	      ultimo = actual;
	      posicion--;
	      numeroDeElementos--;
	    }  // fin del bloque else
	  }    // fin del bloque if( actual == ultimo )
	  else // el elemento a borrar no es el último
	  {
	    q = actual->siguiente;
	    actual->anterior->siguiente = q;
	    q->anterior = actual->anterior;
	    datos = actual->dato;
	    free(actual);
	    actual = q;
	    numeroDeElementos--;
	  }

	  // Actualizar parámetros de la lcde
	  lcde->ultimo = ultimo;
	  lcde->actual = actual;
	  lcde->numeroDeElementos = numeroDeElementos;
	  lcde->posicion = posicion;

	  return datos;

}

void *borrar(tlcde *lcde)
{
  // Obtener los parámetros de la lcde
  Elemento *ultimo = lcde->ultimo;
  Elemento *actual = lcde->actual;
  int numeroDeElementos = lcde->numeroDeElementos;
  int posicion = lcde->posicion;

  // La función borrar devuelve los datos del elemento
  // apuntado por actual y lo elimina de la lista.
  Elemento *q = NULL;
  void *datos = NULL;

  if( ultimo == NULL ) return NULL;  // lista vacía.
  if( actual == ultimo ) // se trata del último elemento.
  {
    if( numeroDeElementos == 1 ) // hay un solo elemento
    {
      datos = ultimo->dato;
      free(ultimo);
      datos = NULL;
      ultimo = actual = NULL;
      numeroDeElementos = 0;
      posicion = -1;
    }
    else // hay más de un elemento
    {
      actual = ultimo->anterior;
      ultimo->siguiente->anterior = actual;
      actual->siguiente = ultimo->siguiente;
      datos = ultimo->dato;
      free(ultimo);
      ultimo = actual;
      posicion--;
      numeroDeElementos--;
    }  // fin del bloque else
  }    // fin del bloque if( actual == ultimo )
  else // el elemento a borrar no es el último
  {
    q = actual->siguiente;
    actual->anterior->siguiente = q;
    q->anterior = actual->anterior;
    datos = actual->dato;
    free(actual);
    actual = q;
    numeroDeElementos--;
  }

  // Actualizar parámetros de la lcde
  lcde->ultimo = ultimo;
  lcde->actual = actual;
  lcde->numeroDeElementos = numeroDeElementos;
  lcde->posicion = posicion;

  return datos;
}



void irAlSiguiente(tlcde *lcde)
{
  // Avanza la posición actual al siguiente elemento.
  if (lcde->posicion < lcde->numeroDeElementos - 1)
  {
    lcde->actual = lcde->actual->siguiente;
    lcde->posicion++;
  }
}

void irAlAnterior(tlcde *lcde)
{
  // Retrasa la posición actual al elemento anterior.
  if ( lcde->posicion > 0 )
  {
    lcde->actual = lcde->actual->anterior;
    lcde->posicion--;
  }
}

void irAlPrincipio(tlcde *lcde)
{
  // Hace que la posición actual sea el principio de la lista.
  lcde->actual = lcde->ultimo->siguiente;
  lcde->posicion = 0;
}

void irAlFinal(tlcde *lcde)
{
  // El final de la lista es ahora la posición actual.
  lcde->actual = lcde->ultimo;
  lcde->posicion = lcde->numeroDeElementos - 1;
}

int irAl(int i, tlcde *lcde)
{
  int n = 0;
  if (i >= lcde->numeroDeElementos || i < 0) return 0;

  irAlPrincipio(lcde);
  // Posicionarse en el elemento i
  for (n = 0; n < i; n++)
    irAlSiguiente(lcde);
  return 1;
}

void *obtenerActual(tlcde *lcde)
{
  // La función obtener devuelve el puntero a los datos
  // asociados con el elemento actual.
  if ( lcde->ultimo == NULL ) return NULL; // lista vacía

  return lcde->actual->dato;
}

void *obtener(int i, tlcde *lcde)
{
  // La función obtener devuelve el puntero a los datos
  // asociados con el elemento de índice i.
  if (!irAl(i, lcde)) return NULL;
  return obtenerActual(lcde);
}

void modificar(void *pNuevosDatos, tlcde *lcde)
{
  // La función modificar establece nuevos datos para el
  // elemento actual.
  if(lcde->ultimo == NULL) return; // lista vacía

  lcde->actual->dato = pNuevosDatos;
}

void anyadirAlFinal(void *e, tlcde *lcde ){
	irAlFinal( lcde );
	insertar( e, lcde);
}

void anyadirAlPrincipio(void *e, tlcde *lcde ){
	irAlPrincipio( lcde );
	insertar( e, lcde);
}

///////////////////// Interfaz para gestionar buffer //////////////////////////////

void mostrarListaFlies(int pos,tlcde *lista)
{
	int n;
	tlcde* flies;
	STFrame* frameData;
	int pos_act;
	pos_act = lista->posicion;

	if (pos >= lista->numeroDeElementos || pos < 0) return;
	  irAlPrincipio(lista);
	  // Posicionarse en el frame pos
	for (n = 0; n < pos; n++) irAlSiguiente(lista);
	// obtenemos la lista de flies
	frameData = (STFrame*)obtenerActual( lista );
	flies = frameData->Flies;
	// Mostrar todos los elementos de la lista
	int i = 0, tam = flies->numeroDeElementos;
	STFly* flydata = NULL;
	for(int j = 0; j < 5; j++){
		while( i < tam ){
			flydata = (STFly*)obtener(i, flies);

			if (j == 0){
				if (i == 0) printf( "\netiquetas");
				printf( "\t%d",flydata->etiqueta);
			}
			if( j == 1 ){
				if (i == 0) printf( "\nPosiciones");
				int x,y;
				x = flydata->posicion.x;
				y = flydata->posicion.y;
				printf( "\t%d %d",x,y);
			}
			if( j == 2 ){
				if (i == 0) printf( "\nOrientacion");
				printf( "\t%0.1f",flydata->orientacion);
			}
			if( j == 3 ){
				if (i == 0) printf( "\nEstado\t");
				printf( "\t%d",flydata->Estado);
			}
			if( j == 4 ){
				if (i == 0) printf( "\nNumFrame");
				printf( "\t%d",flydata->num_frame);
			}
			i++;
		}
		i=0;
	}
	if (tam = 0 ) printf(" Lista vacía\n");
	// regresamos lista al punto en que estaba antes de llamar a la funcion de mostrar lista.
	irAlPrincipio(lista);
	for (n = 0; n < pos_act; n++) irAlSiguiente(lista);
}

void liberarListaFlies(tlcde *lista)
{
  // Borrar todos los elementos de la lista
  STFly *flydata = NULL;
  // Comprobar si hay elementos
  if (lista->numeroDeElementos == 0 ) return;
  // borrar: borra siempre el elemento actual

  irAlPrincipio(lista);
  flydata = (STFly *)borrar(lista);
  while (flydata)
  {
    free(flydata); // borrar el área de datos del elemento eliminado
    flydata = NULL;
    flydata = (STFly *)borrar(lista);
  }
}

// Borra y libera el espacio del primer elemento del buffer ( el frame mas antiguo )
// El puntero actual seguirá apuntando al mismo elemento que apuntaba antes de llamar
// a la función.
int liberarPrimero(tlcde *FramesBuf ){

	STFrame *frameData = NULL;
	STFrame *frDataTemp = NULL;

//Guardamos la posición actual.
	int i = FramesBuf->posicion;
//

	if( FramesBuf->numeroDeElementos == 0 ) {printf("\nBuffer vacio");return 1;}
	irAl(0, FramesBuf);
	frameData = (STFrame*)obtenerActual( FramesBuf );
	 // por cada nuevo frame se libera el espacio del primer frame
	liberarListaFlies( frameData->Flies );
	free( frameData->Flies);
	//borra el primer elemento
	cvReleaseImage(&frameData->Frame);
	cvReleaseImage(&frameData->BGModel);
	cvReleaseImage(&frameData->FG);
	cvReleaseImage(&frameData->IDesv);
	cvReleaseImage(&frameData->ImMotion);
	cvReleaseImage(&frameData->OldFG);
	frameData = (STFrame *)borrar( FramesBuf );
	if( !frameData ) {
		printf( "Se ha borrado el último elemento.Buffer vacio" );
		frameData = NULL;
		return 0;
	}
	else{
		free( frameData );
		frameData = NULL;
		irAl(i - 1, FramesBuf);
		return 1;
	}
}

void liberarSTFrame( STFrame* frameData ){
	liberarListaFlies( frameData->Flies);
	free( frameData->Flies );
	cvReleaseImage(&frameData->BGModel);
	cvReleaseImage(&frameData->FG);
	cvReleaseImage(&frameData->IDesv);
	cvReleaseImage(&frameData->ImMotion);
	cvReleaseImage(&frameData->OldFG);
    free(frameData); // borrar el área de datos del elemento eliminado
}

void liberarBuffer(tlcde *FramesBuf)
{
  // Borrar todos los elementos del buffer
  STFrame *frameData = NULL;
  if (FramesBuf->numeroDeElementos == 0 ) return;
  // borrar: borra siempre el elemento actual
  irAlPrincipio(FramesBuf);
  frameData = (STFrame *)borrar( FramesBuf );
  while (frameData)
  {
	liberarListaFlies( frameData->Flies);
	free( frameData->Flies );
	cvReleaseImage(&frameData->BGModel);
	cvReleaseImage(&frameData->FG);
	cvReleaseImage(&frameData->IDesv);
	cvReleaseImage(&frameData->ImMotion);
	cvReleaseImage(&frameData->OldFG);
    free(frameData); // borrar el área de datos del elemento eliminado
    frameData = (STFrame *)borrar( FramesBuf );
  }
}

/////////////////////////// GESTION FICHEROS //////////////////////////////

int existe(char *nombreFichero)
{
  FILE *pf = NULL;
  // Verificar si el fichero existe
  int exis = 0; // no existe
  if ((pf = fopen(nombreFichero, "r")) != NULL)
  {
    exis = 1;   // existe
    fclose(pf);
  }
  return exis;
}

void crearFichero(char *nombreFichero )
{
  FILE *pf = NULL;
  // Abrir el fichero nombreFichero para escribir "w"
  if ((pf = fopen(nombreFichero, "wb")) == NULL)
  {
    printf("El fichero no puede crearse.");
    exit(1);
  }
 fclose(pf);
}

//!< El algoritmo mantiene un buffer de 50 frames ( 2 seg aprox ) para los datos y para las imagenes.
//!< Una vez que se han llenado los buffer, se almacena en fichero los datos de la lista
//!< Flies correspondientes al primer frame. La posición actual no se modifica.
//!< Si la lista está vacía mostrará un error. Si se ha guardado con éxito devuelve un uno.

int GuardarPrimero( tlcde* framesBuf , char *nombreFichero){

	tlcde* Flies = NULL;
	FILE * pf;
	STFly* fly = NULL;
	int posicion = framesBuf->posicion;

	if (framesBuf->numeroDeElementos == 0) {printf("\nBuffer vacío\n");return 0;}

	// obtenemos la direccion del primer elemento
	irAlPrincipio( framesBuf );
	STFrame* frameData = (STFrame*)obtenerActual( framesBuf );
	//obtenemos la lista

	Flies = frameData->Flies;
	if (Flies->numeroDeElementos == 0) {printf("\nElemento no guardado.Lista vacía\n");return 1;}
	int i = 0, tam = Flies->numeroDeElementos;
	// Abrir el fichero nombreFichero para escribir "w".
	if ((pf = fopen(nombreFichero, "a")) == NULL)
	{
	printf("El fichero no puede abrirse.");
	exit(1);
	}
	while( i < tam ){
		fly = (STFly*)obtener(i, Flies);
		fwrite(&fly, sizeof(STFly), 1, pf);
		if (ferror(pf))
		{
		  perror("Error durante la escritura");
		  exit(2);
		}
		i++;
	}
	fclose(pf);
	irAl( posicion, framesBuf);
	return 1;
}



