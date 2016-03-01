
# 3 Material #

## 3.1 Sistema de adquisición de imagen ##

En este apartado se realizarán una serie de recomendaciones sobre el sistema de adquisición de imagenes que pemitirán aumentar la fiabilidad y robustez del sistema.

### 3.1.1 Cámara y óptica ###

El punto de partida de todo sistema de percepción visual es la captación de la imagen. Este proceso implica la utilización de algún tipo de sensor sobre el que se obtendrá la representación de la escena.
En este capítulo se abordan las características deseables de dicho sistema.

#### 3.1.1.1 Cámara. ####

Con el fin de aumentar la fiabilidad y robustez del sistema, se recomienda el uso de una cámara con las siguientes características:

  1. **Sensor**:  de tipo CCD. Reduce el ruido e incrementa la sensibilidad. Ofrecen una mayor uniformidad ( respuesta de los diferentes píxeles ante la misma iluminación de entrada ).
  1. **Resolución**:  de 800x600 o superior, a costa de una pérdida en la velocidad de procesamiento; se conseguiría una reducción significativa en los errores de indentificación y posición de los objetos a rastrear.
  1. **Tasa de fotograma**: Consideramos que un ratio de 20 ó 25 frames por segundo es suficiente para un funcionamiento correcto, consiguiendo por añadidura una menor carga de procesamiento.
  1. **Color**: El algoritmo internamente trabaja con imagenes en 256 niveles de gris. Una cámara monocromática reduciría el tiempo de cálculo, aunque este no sería un factor determinante.

#### 3.1.1.2 Óptica. ####

La distancia focal y el enfoque se encuentran íntimamente relacionados con el tamaño del sensor y la distancia de la cámara. Para su cálculo se emplea la siguiente fórmula:

### 3.1.2 Iluminación ###

La iluminación juega un papel vital en cualquier sistema de visión artificial ya que proporciona las condiciones ópticas bajo las cuales se lleva a cabo la adquisición de la imagen. Una iluminación estable y que realce las características de interés en la escena hace mucha más sencillo y fiable el posterior procesamiento de la imagen. La iluminación existente en el entorno nunca es aceptable ya que proporciona imágenes con sombras, brillos molestos o intensidad variable.

La aplicación de una iluminación adecuada es determinante para el éxito de la aplicación de rastreo automático, siendo un factor que afecta radicalmente a la complejidad del algoritmo y a la fiabilidad del sistema.

El objetivo será optar por el sistema de iluminación que proporciones el contraste más alto.

#### 3.1.2.1 Características de la fuente de iluminación. ####

  1. **Nivel de luz**: En lo posible el sistema debe estar apantallado para evitar cualquier entrada de luz ambiente. Si no es posible un apantallamiento, la fuente de luz ha de ser de una intensidad tal que permita enmascarar las variaciones ambientales.
  1. **Distribución espectral**: La luz ha de ser blanca ó monocromática.
  1. **Patrón de radiación**: Para evitar brillos y sombras es deseable un patrón de luz hemisférico. En su defecto también sería correcto un patrón anular.
  1. **Estabilidad**: Siempre se debe apantallar el entorno instalando un carenado que evite las perturbaciones de la luz exterior, a fin de maximizar la estabilidad de la iluminación. Si no es posible, la fuente de luz ha de ser lo suficientemente potente para enmascarar la luz ambiente.

#### 3.1.2.2 Fuentes de iluminación ####

  1. **Tubo fluorescente**: Suministra una luz difusa que minimiza las sombras. El fluorescente ha de ser circular con la cámara posicionada en el centro del anillo luminoso. Ha de funcionar a alta frecuencia ( al menos 25Khz ) para evitar parpadeo.
  1. **LED**: Es ampliamente utilizado en los sistemas de visión devido a que presenta una serie de características muy deseables:
    * Funciona a baja temperatura.
    * Tiene un bajo consumo.
    * Pequeño tamaño y flexibilidad en el diseño.
    * Elevada duración y fiabilidad.

#### 3.1.2.3 Técnicas de iluminación: ####
La apariencia del objeto depende en gran medida de la posición de la fuente con respecto al objeto y a la cámara.

Existen un conjunto de técnicas de iluminación basadas en distintas configuraciones geométricas de la fuente de luz con respecto al objeto. Entre entre todas las técnicas, las más adecuadas para la aplicación son las siguientes:
  1. **Retroiluminación**: Se pueden obtener de forma sencilla y estable muy buenas imágenes iluminando con una fuente difusa a contraluz. Ésta técnica permite imágenes de gran contraste entre el objeto y el fondo que son fácilmente procesables. Hay que tener cuidado de no sobresaturar el sensor con una elevada iluminación, ya que el objeto parecerá más pequeño; habrá que reducir la apertura o la velocidad del obturador.
  1. **Iluminación frontal**: En la configuración más típica las luces forman 45º con el eje óptico de la cámara que se encuentra sobre el objeto. Esta configuración no es deseable pues proyecta sombras, produce brillos, y hace aparecer zonas oscuras. Para evitarlo es preciso emplear fuentes de iluminación difusa con patrones anulares ó hemisféricos.
