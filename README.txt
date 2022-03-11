Para la compilación del programa, se proporciona un Makefile.

Para la elección de la configuración deseada, se deben de modificar los valores de las siguiente variables en el fichero mm3.c:

#define SCHEDULING_QUEUE_TYPE 3 /* indica el tipo de planificacion:
                                0: Round Robin
                                1: RANDOM
                                2: smallerQueue
                                3: power of two choices*/

#define SCHEDULING_SERVER_TYPE 0 /* indica el tipo de planificacion:
                                 0: First-shorter
                                 1: First-longuer*/ 
