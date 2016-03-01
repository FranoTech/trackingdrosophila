# 6 Tracking #

## 6.1 Que se entiende por tracking? ##
> Explicar y definir Tracking, track, blob, buffer, nacimiento, muerte, tiempo de vida (de un track), track
> activo, track durmiendo...

## 6.2 Generalidades acerca del algoritmo de rastreo ##

Hasta aquí el programa trabaja solo con información espacial, es decir, los datos del instante t.
En este punto disponemos de la estructura STFrame  ( frameDataIn ) que contiene los datos necesatios del frame t
para poder iniciar el rastreo. Estos son:
> typedef struct {
> > int num\_frame; //!< Identificación del Frame procesado.
> > IplImage**Frame;//!< Imagen fuente de 8 bit de niveles de gris preprocesada.
> > IplImage** BGModel;//!< Imagen de 8 bits que contiene el  BackGround Model Dinámico.
> > IplImage**IDesvf;//!< Imagen de 32 bits que contiene la Desviación Típica del modelo de fondo dinámico.
> > IplImage** FG;  //!< Imagen que contiene el Foreground.
> > IplImage**ImKalman;
> > STStatFrame** Stats; //!< estadísticas.
> > tlcde**Flies; //!< Puntero a lista circular doblemente enlazada (tlcde) con los datos de cada Mosca.**


> tlcde**Tracks; //!< Puntero a lista circular doblemente enlazada (tlcde) con cada Trak. Creado por Tracking
> }STFrame;**

  * - La función de rastreo recibe los datos del frame del instante t y lo almacena temporalmente, de forma que se mantendrán en
  * memoria las estructuras correspondientes a MAX\_BUFFER frames ( framesBuff, buffer de datos  ).
  * Los buffers son colas FIFO. En este punto ya disponemos de la información temporal.
  * - Asi mismo se crea una cola FIFO para almacenar los datos de cada track. Habrá un track por cada objeto a rastrear
  * - Se crea a su vez una cola FIFO para almacenar las identidades ( id y color ), de modo que sean únicas.
  * -
## 6.3 Detalles del algoritmo de rastreo ##

Antes de comenzar con los detalles del algoritmo, vamos a introducir una serie se definiciones relativas a los Tracks y que se usarán a lo largo del texto:

Según su estado, podemos encontrar :

  1. Tracks activos: Serían tracks en estado CAM\_CONTROL o KALMAN\_CONTROL. Están recibiendo nuevos datos
> > a) CAM\_CONTROL: Tienen asignacion válida y única
> > b) KALMAN\_CONTROL: Tiene asignacio válida pero no única.

> 2) Tracks durmiendo: Han perdido el objetivo a rastrear

> Según el origen del track distinguimos:

  1. Tracks válidos: Generados por moscas. LLevan un tiempo rastreando con éxito.
> 2) Tracks "posiblemente" válidos. Serían los nuevos tracks.No se sabe con certeza si han sido generados por moscas.
> 2) Falsos tracks: Tracks generados por espurios o por "fantasmas (ghost)"
> En tracking se realiza la función de rastreo propiamente dicha. en este punto disponemos de todos los datos necesarios para
> identificar y seguir a cada mosca, tratando de no perderla.

> Una vez hechas las definiciones, se resumen a continuación los principales cometidos del algoritmo de rastreo:

  1. Añadir nuevo elemento al buffer de datos.
> 2) En AsignarIdentidades se validan blobs y se resuelven las asociaciones en base a las predicciones
> > de kalman mediante el algoritmo Hungaro.

> 3) En validarTracks se eliminan falsos tracks en base al ;
> - Kalman( frameDataIn , Identities, lsTracks):
> > -  Crea un track para cada blob.
> > -  Inicia un filtro de kalman por cada track.
> > -  Genera nueva medida en base a los datos obtenidos de la camara y de asignar identidades.
> > -  Filtra de la dirección y resuelve ambiguedad en la orientación.
> > -  Actualiza de los parámetros de rastreo: Track y fly (en base a los nuevos datos).
> > -  Realiza la predicción para t+1.

> - Aplicación de Heurísticas. Se usa la información temporal del buffer para decidir si eliminar o no tracks.

### El filtro de kalman ###

#### Origenes del filtro de kalman ####

#### Qué es el filtro de kalman ####

#### Ecuaciones ####

#### Aplicación ####

> Suponemos que ya se ha realizado una iteración de forma que:

> -# Se ha generado un track por cada blob ( un track por cada nueva etiqueta = -1 ) y se han introducido en una lista doblemente enlazada
> -# Se ha iniciado un filtro de kalman para cada track.
> -# Se ha realizado la primera predicción para hacer la primera asignación de identidades.

> Tras aplicar asignaridentidades y eliminar aquellos tracks que no sean válidos:

**- Tenemos en cada track un puntero al blob del frame t+1 con mayor probabilidad de asignación. NULL en caso de que no haya.**		- Asimismo en cada fly disponemos una lista con el o los tracks candidatos a ser asignados a dicha fly.
		Estas asignaciones pueden ser únicas o no. Entendemos por única cuando un track es asignado a un blob exclusivamente.
**En caso de que la máxima probabilidad de la asignación en base a la predicción de kalman haya sido menor que un umbral; esto es la mínima probabilidad de asignación**		será la probabilidad correspondiente a la máxima distancia permitida o máximo salto ( medido en cuerpos y con algo de exceso ) que pueda dar una mosca, dicho
**track quedará sin posible asignación => FlySig = NULL. Pasará a estado SLEEPING; se ha perdido el objetivo a rastrear.**
 	 	En general primero se suele hacer la predicción para frame t ( posible estado del blob en el frame t) en base
**al modelo y luego se incorpora la medida de t con la cual se hace la corrección. En nuestro caso el proceso es:**		Se incorpora la medida del frame t. Se hace la corrección y se predice para el frame t+1, de modo que en cada track y para cada
**frame disponemos por un lado de la predicción para el frame t hecha en el t-1, con la cual se hace la corrección, y por otro lado de** 		la predicción para el frame t+1. Así usamos el dato de la predicción para realizar la asignación de identidades.

> En kalman se realizan principalmente las siguientes acciones, explicadas en detalle en los siguientes apartados
> > En la primera iteración

  * # Genera un track por cada blob ( un track por cada nueva etiqueta = -1 ) y los introduce en una lista doblemente enlazada
  * # Inicia un filtro de kalman para cada track.
  * # Realiza la primera predicción ( que posteriormente será usada por asignarIdentidades para establecer la nueva medida.

  * En cada iteración:

  * # Inicia un filtro de kalman por cada track.
  * # Genera nueva medida en base a los datos obtenidos de la camara y de asignar identidades.
  * # Establece dirección en base al a dirección filtrada por kalman y resuelve ambigüedad en la orientación.
  * # Actualiza de los parámetros de rastreo: Track y fly (en base a los nuevos datos).
  * # Realiza la predicción para t+1.
  * # Crea un track para cada blob si es necesario.
  * 
  * Algoritmo:
  * # Para cada track i:
  * -# Generar medida:
  * - Establece el estado del track dependiendo de si hay o no nuevos datos
  * -# SLEEPING: Sin nueva medida
  * -# CAM\_CONTROL: Asignación única entre Track y nueva medida.
  * -# KALMAN\_CONTROL: Varios Tracks comparten una nueva medida.
  * - Genera o no la nueva medida. en caso afirmativo ésta dependerá del estado del Track.\n
  * Z_{K}(i) = { x\_Zk, y\_Zk, vx\_Zk, vy\_Zk, phiZk } \n
  * - Genera el ruido asociado a la nueva medida.
  * RZ_{k}(i) = { R\_x, R\_y, R\_vx, R\_vy, RphiZk }
  * - Actualiza los parámetros del track y de fly con la nueva medida
  * -# Corrección:
  * - Realiza la corrección con la nueva medida.\n
  * X\_k\_pos(i) = X\_kpre(i) + K(i)( Z\_k(i)- X\_kpre(i) ) \n
  * P\_k\_pos(i) = P\_kpre(i) + K(i)( R\_k(i)- P\_kpre(i) ) \n
  * donde:\n
  * K(i) = Pk\_pre H<sup>T ( H Pk_pre H</sup>T + R\_Zk)^(-1)
  * - Resuelve la ambiguedad en la orientación y establede la dirección ya filtrada.
  * # Genera nuevos tracks si fly->etiqueta == -1 (Fly sin etiquetar).
  * # Realiza la predicción para (t+1) de cada track\n
  * X\_k\_pre(i) = F(k) X\_k\_pre(k-1)(i) + V(i)
  * P\_k\_pre(i) = (F P\_k\_pre(k-1)(i) F(k)) + Q(i)
  * donde:\n
  * \f[Q(i) = f(F,B y H) \f](.md)  Ruido asociado al sistema.\n
  * \f[V(i) = N(0, R\_Zk(i)) \f](.md)  Ruido asociado a la medida.
  * 
##### Estableciendo el estado del track #####


> Se establece el estado en función de si hay o no nueva medida y en caso de que la haya, del tipo que sea.
> - 0) Si no hay datos => Estado track = SLEEPING
> - 1) Un track apunta a un blob. CAM\_CONTROL
> - 2) Varios tracks que apuntan al mismo blob KALMAN\_CONTROL

##### Generar medida #####

> Se genera la medida a partir de los nuevos datos obtenidos de la cámara ( flySig (t+1) );
> Es necesario generar una medida propiamente dicha, además de una incertidumbre o error en la medida;

###### Medida ######

> Se rellenan las variables del vector \n Z\_k. Z\_K = { x\_Zk, y\_Zk, vx\_Zk, vy\_Zk, phiZk } en función del
> estado del Track y establece dicho vector.
  * 
  * Se supone que el estrado del trak ha sido establecido previamente de la siguiente forma
  * -# Si no hay datos => Estado track = SLEEPING (0)
  * -# Correspondencia uno a uno entre blob y track. CAM\_CONTROL (1)
  * -# Varios tracks que apuntan al mismo blob KALMAN\_CONTROL (2).
  * 
  * - Para SLEEPING 0: No se hace nada.
  * - En los casos  CAM\_CONTROL y KALMAN\_CONTROL la velocidad, la dirección y el desplazamiento se establecen a partir del vector posición
  * mediante la función EUDistance( int, int , float**, float** ).
  * - Para CAM\_CONTROL\n
    * Si la dirección y la predicción difieren en más de 180º, se suma a la dirección tantas
    * vueltas como sean necesarias para que ésta difiera en menos de 180º\n
    * - \f[if|phiXk-phiZk| > PI and phiXk > phiZk => phiZk = phiZk + n2PI \f](.md)
    * - \f[if|phiXk-phiZk| > PI and phiXk < phiZk => phiZk = phiZk - n2PI \f](.md)
  * - Para KALMAN\_CONTROL\n
  * La nueva dirección establece como la orientación del blob. Cuando se genere el ruido, éste será elevado
  * 
###### Generando ruido ######
  * 
  * Aquí se genera la incertidumbre en la medida Vk; Vk->N(0,R\_Zk).
  * Generará el ruido a partir del estado del track
  * es decir, rellena las variables del vector R\_Zk = { R\_x, R\_y, R\_vx, R\_vy, RphiZk }.
  * 
  * Se supone que el estrado del trak ha sido establecido previamente de la siguiente forma
  * -# Si no hay datos => Estado track = SLEEPING (0)
  * -# Correspondencia uno a uno entre blob y track. CAM\_CONTROL (1)
  * -# Varios tracks que apuntan al mismo blob KALMAN\_CONTROL (2).
  * 
  * - Para SLEEPING 0: No se hace nada.
  * 
  * - Para CAM\_CONTROL\n
  * R\_Zk(i) = { 1, R\_x, 2R\_x, 2R\_y, RphiZk }\n
  * donde:\n R\_phiZk(i) = generarR\_PhiZk( );\n
  * El error en la dirección dependerá de la velocidad y de la velocidad angular de la diferencia
  * entre la nueva medida y la filtrada.
  * 
  * - Para KALMAN\_CONTROL ( type 0 ) el ruido será elevado (1000)\n
  * R\_Zk(i) = { Roi.width/2, Roi.height/2, 2R\_x, 2R\_y , 1000 }
  * 
> > 2.1.4.2.2.1) RPhiZk. Ruido en la dirección.

  * El error en la dirección dependerá de la velocidad y de la velocidad angular de la diferencia
  * entre la nueva medida y la filtrada.
  * A medida que aumentamos la resolución ó aumenta la velocidad del blob, la precisión en la medida del ángulo aumenta.
  * -# Calculamos el error en la precisión.


> errorV = max{  atan( |vy|/(|vx|- 1/2 ) -  atan( |vy|/|vx| ), atan( |vy| / |vx|) -  atan( (|vy|- 1/2) /|vx|)}**180/PI**

  * -# 	Establece el error debido a la diferencia entre la nueva medida y la filtrada

> phiDif = abs( phiXk - phiZk );

  * 
    * n base a los dos errores anteriores, devuelve el error
> > \f[|phiXk-phiZk| > PI/2 and (Vx||Vy) <= 1  => R\_phiZk = 90  \f](.md)


> \f[|phiXk-phiZk| > PI/2 and (Vx||Vy) <= 2  => R\_phiZk = 45  \f](.md)

> \f[|phiXk-phiZk| > PI/2 and (Vx||Vy)  > 2  => R\_phiZk = |phiXk-phiZk| + errorV  \f](.md)

> \f[|phiXk-phiZk| <= PI/2 		         => R\_phiZk = errorV \f](.md)

###### Actualizando datos ######

> Una vez generada la nueva medida, se actualiza los parámetros de Track y de  fly con  los nuevos datos

> Dependiendo del estado del track se actualizará de una forma o de otra:
> -# Se actualiza el track con la nueva fly:
> - Si Estado Track = SELEEPING(0) se establece Fly Actual y Fly Sig a NULL.
> - Si no la fly siguiente pasa a ser fly actual y fly siguiente se pone a NULL .
> -# Se actualiza y etiqueta flyActual en función del estado del track:
> - Correspondencia uno a uno entre blob y track. CAM\_CONTROL(1).
> > -# Se identifica a la fly con la id y el color del track.
> > -# Se actualizan otros parámetros del track como distancia total, numframe, etc
> > -# Se actualizan otros parámetros de fly actual como el estado (background o foreground), etc

> - Varios tracks que apuntan al mismo blob  KALMAN\_CONTROL(2)
> > -# Se identifica a la fly con id = 0 y color blanco.
> > -# Se establece la dirección del blob como su orientación.
> > -# Se establece la distancia total en 0;

##### Corrección #####


> En la fase de corrección se aportan los nuevos datos generados al filtro de kalman de forma que para cada
> Track:
> - Realiza la corrección con la nueva medida.\n
  * X\_k\_pos(i) = X\_kpre(i) + K(i)( Z\_k(i)- X\_kpre(i) ) \n
  * P\_k\_pos(i) = P\_kpre(i) + K(i)( R\_k(i)- P\_kpre(i) ) \n
  * donde:\n
  * K(i) = Pk\_pre H<sup>T ( H Pk_pre H</sup>T + R\_Zk)^(-1)
  * - Resuelve la ambiguedad en la orientación y establede la dirección ya filtrada.

##### Creación de nuevos Tracks #####
> Es en este punto donde se reserva memoria y se inicializa cada track; esto es: se le asigna una id, un color y se inicia el filtro de kalman
  * se le asigna al blob la id del track y se establece éste como fly Actual.
  * Solo se crearán nuevos tracks para aquellos
  * blobs que lleguen a este punto sin etiquetar ( id == -1), lo cual implica que no hay ningún track rastreando ese blob.
  * En este punto se crea el track independientemente de su validez.Más adelante se verificará si ese track es válido o no.
> > Los parámetros que se inician para el filtro de kalman son;


> - Matriz de transición F
> - Matriz R inicial. Errores en medidas. R\_Zk = { R_, R\_y, R\_vx, R\_vy, RphiZk }
> - Vector de estado inicial X\_K = { x\_k, y\_k, vx\_k, vy\_k, phiXk }
> - Matriz de medida H
> - Error asociado al modelo del sistema Q = f(F,B y H).
> - Incertidumbre en predicción. (P\_k' = F P\_k-1 Ft) + Q
> - Incertidumbre tras añadir medida. P\_k = ( I - K\_k H) P\_k'_

> ##### Fase de predicción #####

> Solo resta realizar la predicción para (t+1) de cada track. Se realiza la prediccion unicamente para los tracks
> activos.

  * X\_k\_pre(i) = F(k) X\_k\_pre(k-1)(i) + V(i)
  * P\_k\_pre(i) = (F P\_k\_pre(k-1)(i) F(k)) + Q(i)
  * donde:\n
  * \f[Q(i) = f(F,B y H) \f](.md)  Ruido asociado al sistema.\n
  * \f[V(i) = N(0, R\_Zk(i)) \f](.md)  Ruido asociado a la medida.


### Asignación de identidades ###

#### Algoritmo Húngaro ####


### Heurísticas ###

#### Tratamiento de nuevos tracks ####

> Cuando kalman origina un nuevo track t,en t+1 se le busca su primera asignación
> En validarTracks se eliminan los tracks creados en t y sin asignación única en t+1.
> Si durante x frames el track tiene asignaciones correctas, se considerará un track
> válido

> Despertando Tracks:
> DespertarTracks trabaja por un lado con los tracks durmiendo de t y por otro lado con los
> Tracks nuevos, que no han sido eliminados por validarTrack, es decir, recien creados
> y "posibles" tracks válidos( =>estan en CAM\_CONTROL y EstateCount == 2).//
> Mientras haya tracks durmiendo, cada nuevo track creado en las inmediaciones
> de un track durmiendo, se intercambiará la id con el track
> durmiendo mas cercano y los parámetros de su fly serán actualizados con
> los del track durmiendo y este será eliminado.

#### Eliminar falsos tracks ####

> Cabe distinguir, en función de las características del blob que originó el falso track, los siguientes tipos:

> tipo 1) Según su tipo de movimiento:
    1. 1) Blob Estático o espurio: Un reflejo inmóvil por ejemplo .
    1. 2) Blob Móvil: Reflejo creado por una mosca que se mueve por el borde del plato ó un espurio que por
> > aparece en t y vuelve a aparecer en t+1 en una posición lo suficientemente cercana.

> tipo 2) Según su duración
> > 2.1) Espurio o falso blob que aparece en t y desaparece en t+1.
> > 2.2) Espurios o falsos blobs que permanecen un tiempo inferior a la longitud del buffer.
> > 2.3) Falsos blobs que permanecen largo tiempo. Sería el caso de una mosca reflejada largo tiempo.


> TRACKS ACTIVOS

> TRACKS DURMIENDO:

> Supocición: Tras aplicar AsignarIdentidades se ha asignado un track ( que no esté durmiendo) a cada blob
> > en base a las predicciones de kalman. Entonces Kalman crea un track por cada blob que queda sin asignar .

> Suponemos que se ha creado un falso track para un espurio ( que sobrevivió a la validación ) en t.
> Suponemos que en t+n el track no es eliminado.

> caso 1) No hay ningún blob lo suficientemente cerca como para ser asignado a dicho track.
> => dicho espurio generaría un track en estado SLEEPING.
> Si no se encuentra dicha asignación en un intervalo de tiempo determinado, el track acabaría
> siendo eliminado en la fase de corrección. En caso contrario, nos encontraríamos con el  caso 2)

> caso 2) un blob que ya estaba siendo rastreado por un track con asignación única se encuentra lo
> > suficientemente cerca del falso track como para que el blob también sea asignado a éste nuevo track.//
> > =>1: en asignarIdentidades se interpretará que el blob oculta dos flies, y se intentará
> > validar para dividir el blob en  2, lo cual sería erróneo. 2: Además, habría dos tracks
> > siguiendo al mismo blob.
> > Un ejemplo de este caso se da por ejemplo en un track creado por un reflejo. Recordar que asignar identidades no se
> > asignan tracks en estado SLEEPING) Justo en el momento en que desaparece el reflejo, el track que lo seguía
> > sería asignado al blob que causó el reflejo ya que dicho track estaría en estado CAM\_CONTROL
> > el reflejo . También se daría en el caso de un espurio que persiste durante dos frames consecutivos.


> Consecuencia: Hay que eliminar el nuevo Track. ( o bien antes de validar o bien usar la validación para detectar si se trata
> de un único blob con una probabilidad suficientemente alta ). Así establecemos flySig = NULL en el track del blob recien creado,
> > de modo que sería automáticamente eliminado en validarTracks.


> Solución adoptada:

> - Tratamiento de  Falsos track de tipo 2.1 que dan lugar al caso 1.
> En validarTracks se eliminan los tracks creados en t y sin asignación única en t+1 con lo que el problema del track creado por un
> espurio de tipo 1 quedaría solucionado. Si el nuevo track tiene asignación válida y única durante 2 frames será considerado un
> "posible" track válido. Si el track permanece un número determinado de frames con asignación única y sin entrar en conflicto con
> otros tracks, es decir, su estado es CAM\_CONTROL, será considerado un track válido.



> - Para espurios del tipo 2.2 tambien se pueden dar el caso 1 y 2
> - Para espurios del tipo 2.3 casos 1 y 2

> corregirTracks
> > Aqui van a parar los tracks dormidos que no han podido ser despertados en despertar tracks.
> > Despertar tracks trabaja por un lado con los tracks durmiendo de t y por otro lado con los
> > Tracks que no han sido eliminados por validarTrack, es decir, recien creados
> > y "posibles" tracks válidos( =>estan en CAM\_CONTROL y EstateCount == 2).
> > Corregir tracks usa el buffer para intentar asignar aquellos tracks que no han sido asignados

> con tracks recien creados por que no cumplian la distancia minima.Se da prioridad a los tracks con
> mayor rango ( < = MAX\_TRACKs )
> transcurridos max buf ( countEstate = maxbuf ) de los nuevos asignamos el más antiguo.
> Si transcurre un periodo determinado sin que aparezcan nuevos traks, es decir, el track ya lleva
> durmiendo mucho tiempo:
> > - Si su id es <= MAXTrack, se sigue esperando,
> > - En cambio, si su id es mayor, se elimina.

> Aclaración:
> > El error en caso de eliminar un track correcto creado por un espurio de tipo 1 quedaría subsanado posteriormente ya que si en la
> > t + 3 se vuelve	a detectar otro blob/espurio en el mismo punto, kalman crearía un nuevo track repitiendose el proceso. Únicamente
> > si durante 2 iteraciones consecutivas el track tiene una nueva asignación y ésta es única se considerará que es un "posible" track válido

> "Posible" ya que no se establecerá dicho track como válido hasta verificar que no se trata de un
> > espurio de tipo 2.


> Para el tipo 2)
> caso 1: El track será automáticamente eliminado si ha nacido hace poco.

> Suponemos ahora que tras validar track tenemos una medida de la validez del track.

> Una vez que se alcance el número máximo de tracks y todos ellos sean válidos, se será mucho más estricto y se dará prioridad
> a los tracks ya creados y verificados.

> Aquí cabe la posibilidad de que una mosca que no se ha movido durante la detección del plato se mueva mínimamente
> en un instante dado. En este caso el track sería válido.

> Cuando un track pasa a estado sleeping deja su id. Si coincide que es necesrio asignar una nueva id en otro punto en el mismo
> instante, esta id se asignará al nuevo track y sería erroneo.

> Falsos blobs que permanecen largo tiempo. Sería el caso de una mosca "mirandose al espejo".