#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<signal.h>
#include<sys/wait.h>
#include<time.h>

pthread_mutex_t fichero,colaClientes,ascensor,maquinas;
pthread_cond_t subirAscensor;
int nClientes;
int nAscensor;
int estadoAscensor;     // 0 parado, 1 funcionando

struct cliente{
    int id,atendido,tipo,ascensor;  /* tipo=0(normal),1(VIP),2(en máquina auto-checking) atendido=0(no atendido),1(esta siendo atendido),2(ya ha sido atendido)
                                     ascensor=0(no va al ascensor), 1(se dirige al ascensor)*/
};

struct recepcionista{
    int tipo,clientesAtendidos;     /* tipo=0(normal) o 1(Vip) y contador de clientes que lleva atendidos*/
};

struct recepcionista *recepcionistas;
struct cliente *clientes;
int *maquinasCheckIn;       // 0(libre) o 1(ocupada)
FILE *logFile;


void *accionesCliente();
void clienteNormal();
void clienteVip();
void terminar();
void writeLogMessage(char *id, char *msg);

void *HiloRecepcionista(){       // PENDIENTE

}


int main(){
    if(signal(SIGUSR1,clienteNormal)==SIG_ERR){
        perror("Error en la llamada a signal");
        exit(-1);
    }

    if(signal(SIGUSR2,clienteVip)==SIG_ERR){
        perror("Error en la llamada a signal");
        exit(-1);
    }

    if(signal(SIGINT,terminar)==SIG_ERR){
        perror("Error en la llamada a signal");
        exit(-1);
    }

    if(pthread_mutex_init(&fichero,NULL)!=0) exit(-1);
    if(pthread_mutex_init(&colaClientes,NULL)!=0) exit(-1);
    if(pthread_mutex_init(&ascensor,NULL)!=0) exit(-1);
    if(pthread_mutex_init(&maquinas,NULL)!=0) exit(-1);

    nClientes=0;
    clientes=(struct cliente *)malloc(sizeof(struct cliente)*20);
    recepcionistas=(struct recepcionista *)malloc(sizeof(struct recepcionista)*3);
    maquinasCheckIn=(int*)malloc(sizeof(int)*5);
    int i;
    for(i=0;i<20;i++){              // Inicializa las listas de clientes, máquinas check-in y recepcionistas.
        if(i<5){
            if(i<3){
                recepcionistas[i].clientesAtendidos=0;
                if(i==2){                                   // El recepcionista 0 y 1 son normales y el 2 es Vip
                    recepcionistas[i].tipo=1;
                }else{
                     recepcionistas[i].tipo=0;
                }
            }
            maquinasCheckIn[i]=0;
        }
        clientes[i].id=0;
        clientes[i].atendido=0;
        clientes[i].tipo=0;
        clientes[i].ascensor=0;
    }

    logFile= fopen("logs.txt","a");     // Inicializa el fichero para los logs
    estadoAscensor=0;                   // El ascensor está parado
    nAscensor=0;                          // Nadie está en el ascensor

    if (pthread_cond_init(&subirAscensor,NULL)!=0) exit(-1);        // Se inicializa la condición

    pthread_t recepcionista0,recepcionista1,recepcionista2;
    pthread_create(&recepcionista0,NULL,HiloRecepcionista,NULL);  // De momento sin atributos
    pthread_create(&recepcionista1,NULL,HiloRecepcionista,NULL);
    pthread_create(&recepcionista2,NULL,HiloRecepcionista,NULL);
    
    while(1){
        pause();
    }

    return 0;
}

void nuevoCliente(){
    int tipoCliente,i;
    pthread_mutex_lock(&colaClientes);
    i=0;
    wait(&tipoCliente);                        // Espera a la señal para designar el tipo de cliente
    tipoCliente=WEXITSTATUS(tipoCliente);

    while(clientes[i].id!=0 && i<20){
        i++;
    }

    if(i!=20){      // Hay espacio libre en la cola de clientes
        struct cliente nuevoCliente;
        pthread_t hiloCliente;

        clientes[i]=nuevoCliente;
        nClientes++;
        nuevoCliente.id=nClientes;
        nuevoCliente.atendido=0;
        if(calculaAleatorios(10)==1){
            tipoCliente=2;
        }
        nuevoCliente.tipo=tipoCliente;
        nuevoCliente.ascensor=0;
        pthread_create(&hiloCliente,NULL,accionesCliente,i);        // Le paso la posición del cliente en la lista a la manejadora
    }
    pthread_mutex_unlock(&colaClientes);
}

void *accionesCliente(int pos){
	pthread_mutex_lock(&fichero);
	writeLogMessage(pos+1,"Entra un cliente");
    switch(clientes[pos].tipo){
        case 0:writeLogMessage(pos+1,"El cliente es de tipo NORMAL");break;
        case 1:writeLogMessage(pos+1,"El cliente es de tipo VIP");break;
        case 2:writeLogMessage(pos+1,"El cliente es de tipo Máquinas");break;
    }   
	pthread_mutex_unlock(&fichero);

	if(clientes[pos].tipo == 2){	//Comprobamos si el cliente va a maquinas o no
		pthread_mutex_lock(&maquinas);
		int i=0,ocupaMaquina=0;                            // ocupaMaquina=0 (no ha ocupado una máquina) o 1 (ha ocupado una máquina)
        while(i<5 && ocupaMaquina==0){
			if(maquinasCheckIn[i] == 0){	//Comprueba si hay maquina libre
				maquinasCheckIn[i] = 1;
                ocupaMaquina=1;
				pthread_mutex_unlock(&maquinas);
				sleep(6);
				if(calculaAleatorios(30)==1){	//Comprueba si va a ascensores
				// Va a ascensores


				}else{	//Marcha
					pthread_mutex_lock(&fichero);
					writeLogMessage(pos+1,"El cliente se marcha");	//Escribe que se va
					pthread_mutex_unlock(&fichero);
					pthread_exit(NULL);
					pthread_mutex_lock(&colaClientes);
					clientes[pos].id=0;                     // Se libera espacio en la cola
                    nClientes--;
					pthread_mutex_unlock(&colaClientes);
				}
			}
            i++;
        }	//No hay maquina libre

		pthread_mutex_unlock(&maquinas);
		sleep(3);
		if(calculaAleatorios(50) == 1){	//SE VA A RECEPCIÓN NORMAL
				
		}else{  // SE QUEDA EN MÁQUINAS

        }
	}else{	//No va a maquinas
		pthread_mutex_lock(&colaClientes);
		int atendido = 0;
		while(atendido == 0){	
			atendido = clientes[pos].atendido;
			if(atendido == 0){	//No esta siendo atendido
				//Comportamiento  
			}else{

                if(atendido==1){
                    //Bucle mientras esta siendo atendido
                }

                if(atendido==2){    // Ya ha sido atendido
                    if(posibilidad(30)==1){
                        // Va a ascensor
                    }else{
                        //No va a ascensor
                        pthread_mutex_lock(&colaClientes);
                        clientes[pos].id=0;
                        nClientes--;		//Se marcha el cliente
                        pthread_mutex_unlock(&colaClientes);
                        pthread_mutex_lock(&fichero);
                        writeLogMessage(pos+1,"El cliente se marcha");	//Escribe en el log
                        pthread_mutex_unlock(&fichero);
                        pthread_exit(NULL);
                    }
                }
			}
		}
	}
}



void clienteNormal(){
    if(signal(SIGUSR1,clienteNormal)==SIG_ERR){
        perror("Error en la llamada a signal");
        exit(-1);
    }
    exit(0);
}

void clienteVip(){
    if(signal(SIGUSR2,clienteNormal)==SIG_ERR){
        perror("Error en la llamada a signal");
        exit(-1);
    }
    exit(1);
}

void terminar(){
    if(signal(SIGINT,terminar)==SIG_ERR){
        perror("Error en la llamada a signal");
        exit(-1);
    }
}

int calculaAleatorios(int probabilidad) {
    srand(time(NULL));
    if((rand() % (100-1+1)+1)<=probabilidad){
        return 1;
    }
    return 0;
}

void writeLogMessage(char *id, char *msg){
    //Calcula la hora actual
    time_t now=time(0);
    struct tm *tlocal= localtime(&now);
    char stnow[25];
    strftime(stnow,25,"%d/%m/%y %H:%M:%S",tlocal);
    //Escribe en el log
    logFile= fopen("logs.txt","a");
    fprintf(logFile, "[%s] %s: %s\n",stnow,id,msg);
    fclose(logFile);
}