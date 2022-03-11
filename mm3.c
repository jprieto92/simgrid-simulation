#include <stdio.h>
#include "simgrid/msg.h"
#include "xbt/synchro.h"


int client(int argc, char *argv[]);
int server(int argc, char *argv[]);


#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */
#define N_TASKS  10000
#define ARRIVAL_RATE    5.0     // lamda = 5, inter arrival time = 1/5
#define SERVICE_RATE    6.0       // Mu = 6  service time = 1 /6

#define SCHEDULING_QUEUE_TYPE 3	/* indica el tipo de planificacion: 
				0: Round Robin
        			1: RANDOM
				2: smallerQueue
				3: power of two choices*/

#define SCHEDULING_SERVER_TYPE 0 /* indica el tipo de planificacion: 
				 0: First-shorter
        			 1: First-longuer*/

int *Ncola;          	// tamanio de la cola
int *FinCola;		// indicacion de fin de cola
xbt_mutex_t *mutex;
xbt_cond_t  *cond;
xbt_dynar_t *client_requests ;   // cola de peticiones
int num_servers = 10;

struct ClientRequest {
	int    n_task;
	double t_arrival;   /* momento en el que llega */
	double t_service;   /* tiempo de servicio asignado */
};

	

/*-------------  UNIFORM [0, 1) RANDOM NUMBER GENERATOR  -------------*/
double uniform(void)
  {
        return drand48();
  }


/*-------------  UNIFORM (0, 1) RANDOM NUMBER GENERATOR  -------------*/
double uniform_pos(void)
{
        double g;

        g = uniform();
        while (g == 0.0)
                g = uniform();

        return g;
}


/*--------------  EXPONENTIAL RANDOM VARIATE GENERATOR  --------------*/
/* The exponential distribution has the form

   p(x) dx = exp(-x/landa) dx/landa

   for x = 0 ... +infty
*/
double exponential(double landa)
{
        /* 'exponential' returns a psuedo-random variate from a negative     */
        /* exponential distribution with mean 1/landa.                        */

        double u = uniform_pos();
        double mean = 1.0 / landa;

        return -mean * log(u);
}

/*-------- SMALLER QUEUE SELECTOR --------*/
/* 'smallerQueue' returns the index of server with smaller queue */
int smallerQueue()
{
	int i = 0;
	int min = 0;
	int aux = Ncola[0];
	
	for(i = 0; i < num_servers; i++) {
		if(Ncola[i] < aux){
			aux = Ncola[i];
			min = i;
		}
	}

	return min;
}

/*-------- POWER OF TWO CHOICES SELECTOR --------*/
/* 'twoChoices' returns the index of server with smaller queue between two random servers */
int twoChoices()
{
	int serverId1 = rand() % 10;
	int serverId2 = rand() % 10;
	int min = 0;
	
	if(Ncola[serverId1] <= Ncola[serverId2]){
		min = serverId1;
	}else{
		min = serverId2;
	}

	return min;
}

/** client function  */
int client(int argc, char *argv[])
{
	int number_of_tasks = N_TASKS;
  	double task_comp_size = 0;
  	double task_comm_size = 0;
  	char sprintf_buffer[64];
  	char mailbox[256];
  	msg_task_t task = NULL;
  	int i;
  	struct ClientRequest req, *r;
  	double tiempo_llegada;
	int serverId = 0;

	for (i = 0; i < number_of_tasks; i++) {

      		switch(SCHEDULING_QUEUE_TYPE){
			case 0:
				/*Case 0: Round-Robin*/
				serverId = i % num_servers;
				break;
			case 1:
				/*Case 1: Random*/
				serverId = rand() % num_servers;
				break;
			case 2:
				/*Case 2: Smaller Queue*/
				serverId = smallerQueue();
				break;
			case 3:
				/*Case 3: Power	of two choices*/
				serverId = twoChoices();
				break;
		}
		/* espera la llegada de una peticion */
		/* 50 peticiones por segundo, lamda = 50 */
      		tiempo_llegada = exponential((double)ARRIVAL_RATE);
      		MSG_process_sleep(tiempo_llegada);

      		/* crea la tarea */
      		sprintf(sprintf_buffer, "Task_%d", i);
      		req.t_arrival = MSG_get_clock();                  // tiempo de llegada
      		req.t_service = exponential((double)SERVICE_RATE);   // tiempo de servicio asignada a la tarea,  tiempo medio = 1/6 segundo
		req.n_task = i;

      		task = MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size,NULL);

      		r = (struct ClientRequest *) malloc(sizeof(struct ClientRequest));
      		*r = req;
      		MSG_task_set_data(task, (void *) r );

		
      		/* envia la tarea a la cola */
      		sprintf(mailbox, "QueueHost_%d", serverId);
      		MSG_task_isend(task, mailbox);   
	}

    	/* finalizar */
	
	for(i = 0; i < num_servers; i++) {

		sprintf(mailbox, "QueueHost_%d", i);
		msg_task_t finalize = MSG_task_create("finalize", 0, 0, FINALIZE);
		MSG_task_send(finalize, mailbox);
	}
  	return 0;
}                               /* end_of_client */

// ordena dos elementos de tipo struct ClientRequest
static int sort_function(const void *e1, const void *e2)
{

        struct ClientRequest *c1 = *(void **) e1;
        struct ClientRequest *c2 = *(void **) e2;

        if (c1->t_service == c2->t_service)
                return 0;

        else    if (c1->t_service < c2->t_service)
                        return -1;
                else
                        return 1;
}

// ordena dos elementos de tipo struct ClientRequest de forma inversa
static int sort_function_inverse(const void *e1, const void *e2)
{

        struct ClientRequest *c1 = *(void **) e1;
        struct ClientRequest *c2 = *(void **) e2;

        if (c1->t_service == c2->t_service)
                return 0;

        else    if (c1->t_service < c2->t_service)
                        return 1;
                else
                        return -1;
}



/** queue function  */
int queue(int argc, char *argv[])
{
  	msg_task_t task = NULL;
  	struct ClientRequest *req;
  	int res;
	int i = 0;
	int n = 0;
	char mailbox[256];
	int serverId = atoi(argv[1]);

	sprintf(mailbox,"QueueHost_%d", serverId);
  	while (1) {
    		res = MSG_task_receive(&(task), mailbox);
    		xbt_assert(res == MSG_OK, "MSG_task_receive failed");

    		xbt_mutex_acquire(mutex[serverId]);

    		if (!strcmp(MSG_task_get_name(task), "finalize")) {
      			MSG_task_destroy(task);
			FinCola[serverId] = 1;
			xbt_cond_signal(cond[serverId]);
    			xbt_mutex_release(mutex[serverId]);
      			break;
    		}
		req = MSG_task_get_data(task);

		// insert

    		Ncola[serverId]++;
		xbt_dynar_push(client_requests[serverId], &req);
		
		switch(SCHEDULING_SERVER_TYPE){
			case 0:
				/*First-shorter*/
				xbt_dynar_sort(client_requests[serverId], sort_function);
				break;
			case 1:		
				/*First-longer*/
				xbt_dynar_sort(client_requests[serverId], sort_function_inverse);
				break;
		}

		if (Ncola[serverId] >0 )
			xbt_cond_signal(cond[serverId]);
    		xbt_mutex_release(mutex[serverId]);

    		MSG_task_destroy(task);
    		task = NULL;
	}  

  	return 0;
}                               /* end_of_server */


/** server function  */
int server(int argc, char *argv[])
{

	int res;
  	struct ClientRequest *req;
	msg_task_t request = NULL;
	msg_task_t task = NULL;
  	char lastmailbox[256], mailbox[256];
	int NcolaMedia = 0;
	double tiempo_medio_servicio = 0.0;
	double c;
	int tareas = 0;
	int i, n;
	double t_actual = 0.0, t_anterior = 0.0;

	int serverId = atoi(argv[1]);
	
	sprintf(mailbox,"QueueHost_%d", serverId);

	req = malloc(sizeof(struct ClientRequest));
	while (1) {

		// extrae una petición de la cola
    		xbt_mutex_acquire(mutex[serverId]);
		while ((Ncola[serverId] ==  0)   && (FinCola[serverId] == 0)) {
			xbt_cond_wait(cond[serverId], mutex[serverId]);
		}

		if ((FinCola[serverId] == 1) && (Ncola[serverId] == 0)) {
			xbt_mutex_release(mutex[serverId]);
			break;
		}

                xbt_dynar_shift(client_requests[serverId], &req);
		t_actual = req->t_service;

		tareas ++;
		Ncola[serverId]--;
    		xbt_mutex_release(mutex[serverId]);

		NcolaMedia = NcolaMedia + Ncola[serverId];

		MSG_process_sleep(req->t_service);
                c = MSG_get_clock();

		tiempo_medio_servicio = tiempo_medio_servicio + (c - (req->t_arrival));
                free(req);
		
		if(t_anterior != 0.0) {
		
			if(t_actual < t_anterior){
				//printf("Long to short\n");
			}else if(t_actual > t_anterior) {
				//printf("Short to long\n");
			}
		}
		t_anterior = t_actual;
	}	


	printf("--------------------------------------- \n");
	printf("Tareas ejecutadas[%s]:  %d\n",  MSG_host_get_name(MSG_host_self()),   tareas);
        printf("Tamanio medio de la cola[%s] = %g \n", MSG_host_get_name(MSG_host_self()),  (double) NcolaMedia / tareas);
        printf("Tiempo medio de servicio[%s]: %g\n", MSG_host_get_name(MSG_host_self()),(double) tiempo_medio_servicio / tareas);

        printf("--------------------------------------- \n");
}



/** Main function */
int main(int argc, char *argv[])
{
  	msg_error_t res = MSG_OK;
	char str[256];
	int i;
	Ncola = (int *) malloc(sizeof(int) * num_servers);
	FinCola = (int *) malloc(sizeof(int) * num_servers);
	for(i = 0; i < num_servers; i++) {
			
		Ncola[i] = 0;
		FinCola[i] = 0;
	}

  	srand48((int) time(NULL));

  	MSG_init(&argc, argv);
  	if (argc != 3) {
    		printf("Usage: %s <platform_file> <deployment_file>  \n", argv[0]);
    		exit(1);
  	}
	mutex = (xbt_mutex_t *) malloc(sizeof(xbt_mutex_t) * num_servers);
	cond = (xbt_cond_t *) malloc(sizeof(xbt_cond_t) * num_servers);
	for(i = 0; i < num_servers; i++) {

		mutex[i] = xbt_mutex_init();
		cond[i] = xbt_cond_init();
	}	
	client_requests = (xbt_dynar_t *) malloc(sizeof(xbt_dynar_t) * num_servers);
	for(i = 0; i < num_servers; i++) {

		client_requests[i] = xbt_dynar_new(sizeof(struct ClientRequest), NULL);
	}

  	MSG_create_environment(argv[1]);

  	MSG_function_register("client", client);
  	MSG_function_register("server", server);
  	MSG_function_register("queue", queue);

	MSG_launch_application(argv[2]);

  	res = MSG_main();
  	printf("Simulation time %g\n", MSG_get_clock());


	//xbt_dynar_free(&client_requests);

  	if (res == MSG_OK)
    		return 0;
  	else
    		return 1;
}
