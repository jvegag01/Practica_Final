#include<pthread.h>
#include<stdlib.h>
#include <unistd.h>
#include<stdio.h>
#include<signal.h>
#include<sys/wait.h>
#include<time.h>

pthread_mutex_t fichero,colaClientes,ascensor,maquinas;
pthread_cond_t subirAscensor;
int nClientes;
int nClientesMax;
int nMaquinas;
int nAscensor;
int estadoAscensor;     // 0 parado, 1 funcionando

struct cliente{
    int id,atendido,tipo,ascensor,pretipo,entrada,expulsion;
  /* tipo=0(normal),1(VIP),2(en máquina auto-checking) atendido=0(no atendido),1(esta siendo atendido),2(ya ha sido atendido)
                                     ascensor=0(no va al ascensor), 1(se dirige al ascensor)  pretipo=0(normal), 1(VIP)  entrada=0(primera vez), 1(otra)*/
};

struct recepcionista{
    int tipo,clientesAtendidos,clienteAtendido;   /* tipo=0(normal) o 1(Vip) y contador de clientes que lleva atendidos  clienteAtendido=0(si no esta atendiendo) o pos del cliente(atiende a un cliente)*/
};

struct recepcionista *recepcionistas;
struct cliente *clientes;
int *maquinasCheckIn;       // 0(libre) o 1(ocupada)
FILE *logFile;


void *accionesCliente(void *arg);
void clienteNormal();
void clienteVip();
void terminar();
void writeLogMessage(int id, char *msg);
void cogeAscensor(int pos);
int calculaAleatorios(int min, int max);
int probabilidad(int probabilidad);
int posibilidad2(int posibilidad1, int posibilidad2);
void modificaNMaquinas();
void modificaNClientes();

void *HiloRecepcionista(void *arg){ 
    int i=0,atencion,tiempo;  
    int pos=*(int *)arg;

    pthread_mutex_lock(&colaClientes);

    if(recepcionistas[pos].tipo==0){
        while(clientes[i].id==0 && i<20 || clientes[i].id!=0 && i<20 && clientes[i].atendido!=0 || clientes[i].id!=0 && i<20 && clientes[i].tipo!=0){
            i++;
        }
        if(i!=20){
        recepcionistas[pos].clienteAtendido=i;
        clientes[recepcionistas[pos].clienteAtendido].atendido=1;
        pthread_mutex_lock(&fichero);
	    writeLogMessage(pos+1,"encuentra normal");
	    pthread_mutex_unlock(&fichero);
        }
    }else{
        while(clientes[i].id==0 && i<20 || clientes[i].id!=0 && i<20 && clientes[i].atendido!=0 || clientes[i].id!=0 && i<20 && clientes[i].tipo!=1){
            i++;
        }
        if(i!=20){
        recepcionistas[pos].clienteAtendido=i;
        clientes[recepcionistas[pos].clienteAtendido].atendido=1;
        pthread_mutex_lock(&fichero);
	    writeLogMessage(pos+1,"encuentra VIP");
	    pthread_mutex_unlock(&fichero);
        }
    }

    pthread_mutex_unlock(&colaClientes);
    if(i==20){
        sleep(1);
        HiloRecepcionista((void*)&pos);
    }else{
        recepcionistas[pos].clienteAtendido=i;
    }

    // pthread_mutex_lock(&colaClientes);
    // clientes[recepcionistas[pos].clienteAtendido].atendido=1;
    // pthread_mutex_lock(&fichero);
	// writeLogMessage(recepcionistas[pos].clienteAtendido,"ATIENDE AL CLIENTE");
	// pthread_mutex_unlock(&fichero);
    // pthread_mutex_unlock(&colaClientes);

    atencion = posibilidad2(10, 10);
    if(atencion==1){
        tiempo=calculaAleatorios(2,6);
    }else if(atencion==2){
        tiempo=calculaAleatorios(6,10);
    }else{
        tiempo=calculaAleatorios(1,4);
    }

    pthread_mutex_lock(&fichero);
	writeLogMessage(pos+1,"Se comienza a atender al cliente");
	pthread_mutex_unlock(&fichero);

    sleep(tiempo);

    pthread_mutex_lock(&fichero);
	writeLogMessage(pos+1,"Ha finalizado la atención");
    if(atencion==1){
        writeLogMessage(pos+1,"La atención ha finalizado y el cliente estaba mal identificado");
    }else if(atencion==2){
        writeLogMessage(pos+1,"La atención ha finalizado y el cliente no tiene el pasaporte vacunal");
    }else{
        writeLogMessage(pos+1,"La atención ha finalizado y el cliente tiene todo en regla");
    }
	pthread_mutex_unlock(&fichero);

    pthread_mutex_lock(&colaClientes);
    clientes[recepcionistas[pos].clienteAtendido].atendido=2;
    if(atencion==2){
        clientes[recepcionistas[pos].clienteAtendido].expulsion=1;            // Da la orden para que se marche
    }
    pthread_mutex_unlock(&colaClientes);

    recepcionistas[pos].clientesAtendidos++;

    if(recepcionistas[pos].tipo==0){
        if(recepcionistas[pos].clientesAtendidos % 5 == 0){
            pthread_mutex_lock(&fichero);
	        writeLogMessage(pos+1,"Comienza el descanso");
	        pthread_mutex_unlock(&fichero);

            sleep(5);

            pthread_mutex_lock(&fichero);
	        writeLogMessage(pos+1,"Fin del descanso");
	        pthread_mutex_unlock(&fichero);
        }
    }
    HiloRecepcionista((void*)&pos);
}


int main(int argc, char *argv[]){
    nClientesMax=atoi(argv[1]);
    nMaquinas=atoi(argv[2]);

    logFile= fopen("logs.txt","w");     // Inicializa el fichero para los logs
    if(argc!=3){
        writeLogMessage(argc,"ERROR. El número de argumentos es distinto de 2");
        return(-1);
    }

    if(nClientesMax<1){
        writeLogMessage(nClientesMax,"ERROR. El número de clientes debe ser 1 o mayor");
        return(-1);
    }

     if(nMaquinas<1){
        writeLogMessage(nMaquinas,"ERROR. El número de máquinas de check-in debe ser 1 o mayor");
        return(-1);
    }

    if(signal(SIGUSR1,clienteNormal)==SIG_ERR){
        perror("Error en la llamada a signal");
        exit(-1);
    }

    if(signal(SIGUSR2,clienteVip)==SIG_ERR){
        perror("Error en la llamada a signal");
        exit(-1);
    }

    if(signal(SIGSEGV,modificaNMaquinas)==SIG_ERR){
        perror("Error en la llamada a signal");
        exit(-1);
    }

    if(signal(SIGPIPE,modificaNClientes)==SIG_ERR){	//Ponemos señal 13 para realizar la modificacion I
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
    clientes=(struct cliente *)malloc(sizeof(struct cliente)*nClientesMax);
    recepcionistas=(struct recepcionista *)malloc(sizeof(struct recepcionista)*3);
    maquinasCheckIn=(int*)malloc(sizeof(int)*nMaquinas);
    int i;
    for(i=0;i<nClientesMax;i++){              // Inicializa las listas de clientes
        
        clientes[i].id=0;
        clientes[i].atendido=0;
        clientes[i].tipo=0;
        clientes[i].ascensor=0;
	    clientes[i].pretipo=0;
	    clientes[i].entrada=0;
        clientes[i].expulsion=0;
    }

    for(i=0;i<nMaquinas;i++){                 //Inicializa las listas de máquinas check-in
        maquinasCheckIn[i]=0;
    }

    for(i=0;i<3;i++){                           //Inicializa las listas de recepcionistas
        recepcionistas[i].clientesAtendidos=0;
        recepcionistas[i].clienteAtendido=0;
        if(i==2){                                   // El recepcionista 0 y 1 son normales y el 2 es Vip
            recepcionistas[i].tipo=1;
        }else{
            recepcionistas[i].tipo=0;
        }
    }

    
    estadoAscensor=0;                   // El ascensor está parado
    nAscensor=0;                          // Nadie está en el ascensor

    if (pthread_cond_init(&subirAscensor,NULL)!=0) exit(-1);        // Se inicializa la condición

    pthread_t recepcionista0,recepcionista1,recepcionista2;
    int cero=0,uno=1,dos=2;
    pthread_create(&recepcionista0,NULL,HiloRecepcionista,(void*)&cero);  // De momento sin atributos
    pthread_create(&recepcionista1,NULL,HiloRecepcionista,(void*)&uno);
    pthread_create(&recepcionista2,NULL,HiloRecepcionista,(void*)&dos);
    
    while(1){
        pause();
    }

    return 0;
}
;
void nuevoCliente(int tipo){
    int i;
    pthread_mutex_lock(&colaClientes);
    i=0;

    while(clientes[i].id!=0 && i<nClientesMax){
        i++;
    }

    if(i!=nClientesMax){      // Hay espacio libre en la cola de clientes
        struct cliente nuevoCliente;
        pthread_t hiloCliente;

        nClientes++;
        nuevoCliente.id=nClientes;
        nuevoCliente.atendido=0;
        if(probabilidad(10)==1){
            tipo=2;
        }
        nuevoCliente.tipo=tipo;
        nuevoCliente.ascensor=0;
	    nuevoCliente.pretipo=0;
	    nuevoCliente.entrada=0;
        nuevoCliente.expulsion=0;
        clientes[i]=nuevoCliente;

        pthread_create(&hiloCliente,NULL,accionesCliente,(void*)&i);        // Le paso la posición del cliente en la lista a la manejadora
 	pthread_mutex_unlock(&colaClientes);
    }else{
    pthread_mutex_unlock(&colaClientes);
    pthread_mutex_lock(&fichero);
    writeLogMessage(-1, "No ha podido entrar");
    pthread_mutex_unlock(&fichero);
    }
}

void *accionesCliente(void *arg){
    int pos=*(int *)arg;
	    pthread_mutex_lock(&colaClientes);
	    if(clientes[pos].entrada==0){
		clientes[pos].entrada=1;
		pthread_mutex_unlock(&colaClientes);
		pthread_mutex_lock(&fichero);
		writeLogMessage(pos+1,"Entra un cliente");

        pthread_mutex_lock(&colaClientes);
    	switch(clientes[pos].tipo){
     		case 0:writeLogMessage(pos+1,"El cliente es de tipo NORMAL.");break;
  			case 1:writeLogMessage(pos+1,"El cliente es de tipo VIP.");break;
   			case 2:writeLogMessage(pos+1,"El cliente es de tipo Máquinas.");break;
   		}

		pthread_mutex_unlock(&fichero);
	}	
	if(clientes[pos].tipo == 2){	//Comprobamos si el cliente va a maquinas o no
		pthread_mutex_unlock(&colaClientes);
		pthread_mutex_lock(&maquinas);
		int i=0,ocupaMaquina=0;                            // ocupaMaquina=0 (no ha ocupado una máquina) o 1 (ha ocupado una máquina)
        while(i<nMaquinas && ocupaMaquina==0){
			if(maquinasCheckIn[i] == 0){	//Comprueba si hay maquina libre
				maquinasCheckIn[i] = 1;
                ocupaMaquina=1;
				pthread_mutex_unlock(&maquinas);
				sleep(6);
				pthread_mutex_lock(&maquinas);
				maquinasCheckIn[i] = 0;
				pthread_mutex_unlock(&maquinas);
				if(probabilidad(30)==1){	//Comprueba si va a ascensores
				// Va a ascensores
				cogeAscensor(pos);	
				}else{	//Marcha
					pthread_mutex_lock(&fichero);
					writeLogMessage(pos+1,"El cliente se marcha tras pasar por las maquinas de checkIn.");	//Escribe que se va
					pthread_mutex_unlock(&fichero);
					pthread_mutex_lock(&colaClientes);
					clientes[pos].id=0;                     // Se libera espacio en la cola
                    nClientes--;
					pthread_mutex_unlock(&colaClientes);
                    pthread_exit(NULL);
				}
			}
            i++;
        }	//No hay maquina libre

		pthread_mutex_unlock(&maquinas);
		sleep(3);
		if(probabilidad(50) == 1){	//Se queda en maquinas 
			pthread_mutex_lock(&fichero);
			writeLogMessage(pos+1, "El cliente vuelve a intentarlo en maquinas");
			pthread_mutex_unlock(&fichero);
			accionesCliente((void*)&pos);	
      	}else{
			pthread_mutex_lock(&colaClientes);
			clientes[pos].tipo=clientes[pos].pretipo;
			pthread_mutex_lock(&fichero);
			writeLogMessage(pos+1, "El cliente se cansa de esperar a las maquinas y prueba suerte en las colas");
			pthread_mutex_unlock(&fichero);
            pthread_mutex_unlock(&colaClientes);
			accionesCliente((void*)&pos);
		}
	}else{	//No va a maquinas
                    pthread_mutex_unlock(&colaClientes);
                    // sleep(1);
					pthread_mutex_lock(&fichero);
					writeLogMessage(pos+1, "NO VA MAQUINAS");
                    pthread_mutex_unlock(&fichero);
                    pthread_mutex_lock(&colaClientes);
		if(clientes[pos].atendido == 0){
					pthread_mutex_lock(&fichero);
					writeLogMessage(pos+1, "YA ESTA DENTRO");
                    pthread_mutex_unlock(&fichero);
				int destino = posibilidad2(20, 10);  
				if(destino == 1){
					//Va a maquinas
					clientes[pos].tipo = 2;
					pthread_mutex_lock(&fichero);
					writeLogMessage(pos+1, "El cliente se cansa de esperar y se va a maquinas");
                    pthread_mutex_unlock(&fichero);
                    pthread_mutex_unlock(&colaClientes);
					accionesCliente((void*)&pos);
				}else if(destino == 2){		//Se marcha directamente
					//Se marcha
                    clientes[pos].id=0;
                    nClientes--;            //Se marcha el cliente
           			pthread_mutex_lock(&fichero);
           			writeLogMessage(pos+1,"El cliente se marcha directamente porque se cansa de esperar.");  //Escribe en el log
            		pthread_mutex_unlock(&fichero);
                    pthread_mutex_unlock(&colaClientes);
             		pthread_exit(NULL);
				}else{
					if(probabilidad(5)==1){
						//Se va al baño
			            clientes[pos].id=0;
                       	nClientes--;            //Se marcha el cliente
         			    pthread_mutex_lock(&fichero);
                 		writeLogMessage(pos+1,"El cliente se marcha tras ir al baño y perder su sitio.");  //Escribe en el log
                      	pthread_mutex_unlock(&fichero);
                        pthread_mutex_unlock(&colaClientes);
                   		pthread_exit(NULL);
					}else{	//Se queda en la cola
						pthread_mutex_lock(&fichero);
						writeLogMessage(pos+1, "El cliente decide seguir esperando en la cola");
						pthread_mutex_unlock(&fichero);
                        pthread_mutex_unlock(&colaClientes);
                        sleep(3);
						accionesCliente((void*)&pos);
					}
				}
			}
		while(clientes[pos].atendido== 1){
            pthread_mutex_lock(&fichero);
            writeLogMessage(pos+1,"ESTA SIENDO ATENDIDO");  //Escribe en el log
            pthread_mutex_unlock(&fichero);
			pthread_mutex_unlock(&colaClientes);
			sleep(2);
			pthread_mutex_lock(&colaClientes);
		}		//Esta siendo atendido

        // Ya ha sido atendido
        if(clientes[pos].expulsion==1){
            clientes[pos].id=0;
            nClientes--;		//Se marcha el cliente
            pthread_mutex_unlock(&colaClientes);
            pthread_mutex_lock(&fichero);
            writeLogMessage(pos+1,"El cliente se marcha por no tener el certificado de vacunación");	//Escribe en el log
            pthread_mutex_unlock(&fichero);
            pthread_exit(NULL);
        }
		pthread_mutex_unlock(&colaClientes);

        if(probabilidad(30)==1){
            // Va a ascensor
			pthread_mutex_lock(&fichero);
			writeLogMessage(pos+1, "El cliente ha sido atendido y decide ir a los ascensores");
			pthread_mutex_unlock(&fichero);
			cogeAscensor(pos);
        }else{
            //No va a ascensor
            pthread_mutex_lock(&colaClientes);
            clientes[pos].id=0;
            nClientes--;		//Se marcha el cliente
            pthread_mutex_unlock(&colaClientes);
            pthread_mutex_lock(&fichero);
            writeLogMessage(pos+1,"El cliente comunica que se va a su habitación.");	//Escribe en el log
            pthread_mutex_unlock(&fichero);
            pthread_exit(NULL);
        }
    }
}

void cogeAscensor(int pos){
	pthread_mutex_lock(&colaClientes);
	clientes[pos].ascensor = 1;
	pthread_mutex_unlock(&colaClientes);
	pthread_mutex_lock(&ascensor);
	if(estadoAscensor == 1){	//El ascensor esta funcionando
		pthread_mutex_unlock(&ascensor);
		sleep(3);
		pthread_mutex_lock(&fichero);
		writeLogMessage(pos+1, "El cliente espera al ascensor.");
		pthread_mutex_unlock(&fichero);
		cogeAscensor(pos);		
	}else{	//El ascensor no esta funcionando
		nAscensor++;
		if(nAscensor<6){	//Todavia no esta lleno
            pthread_mutex_unlock(&ascensor);
		    pthread_mutex_lock(&fichero);
		    writeLogMessage(pos+1, "El cliente entra en el ascensor.");
		    pthread_mutex_unlock(&fichero);
            pthread_mutex_lock(&ascensor);
            pthread_cond_wait(&subirAscensor,&ascensor);
		}else{	//esta lleno
            estadoAscensor=1;
            pthread_mutex_unlock(&ascensor);
            sleep(calculaAleatorios(6,3));
            pthread_mutex_lock(&ascensor);
            
            for(int i=0;i<5;i++){
                pthread_cond_signal(&subirAscensor);        // Desbloquea a 1 cliente del ascensor
            }
		}


        if(nAscensor==1){
            estadoAscensor=0;       // El último en salir cierra la puerta
        }
        nAscensor--;
        pthread_mutex_unlock(&ascensor);
		pthread_mutex_lock(&fichero);
		writeLogMessage(pos+1, "El cliente sale del ascensor.");
		pthread_mutex_unlock(&fichero);

		pthread_mutex_lock(&colaClientes);
        clientes[pos].id=0;
        nClientes--;            //Se marcha el cliente
        pthread_mutex_unlock(&colaClientes);
        pthread_exit(NULL);
	}
}

void clienteNormal(){
    if(signal(SIGUSR1,clienteNormal)==SIG_ERR){
        perror("Error en la llamada a signal");
        exit(-1);
    }
    		pthread_mutex_lock(&fichero);
		writeLogMessage(1, "SEÑAL LLEGA");
		pthread_mutex_unlock(&fichero);
    nuevoCliente(0);
}

void clienteVip(){
    if(signal(SIGUSR2,clienteVip)==SIG_ERR){
        perror("Error en la llamada a signal");
        exit(-1);
    }
        		pthread_mutex_lock(&fichero);
		writeLogMessage(1, "SEÑAL LLEGA VIP");
		pthread_mutex_unlock(&fichero);
    nuevoCliente(1);
}

void modificaNMaquinas(){                               // Aumenta en 1 el número de máquinas
    if(signal(SIGSEGV,modificaNMaquinas)==SIG_ERR){	//Ponemos señal 11 para realizar la modificacion II
        perror("Error en la llamada a signal");
        exit(-1);
    }
    pthread_mutex_lock(&maquinas);
    nMaquinas++;
    maquinasCheckIn=realloc(maquinasCheckIn,nMaquinas);
    pthread_mutex_unlock(&maquinas);
}

void modificaNClientes(){                               // Aumenta en 1 el número de clientes
    if(signal(SIGPIPE,modificaNClientes)==SIG_ERR){	//Ponemos señal 13 para realizar la modificacion I
        perror("Error en la llamada a signal");
        exit(-1);
    }
    pthread_mutex_lock(&colaClientes);
    nClientesMax++;
    clientes=realloc(clientes,nClientesMax);
    pthread_mutex_unlock(&colaClientes);
}

void terminar(){
    if(signal(SIGINT,terminar)==SIG_ERR){
        perror("Error en la llamada a signal");
        exit(-1);
    }
    exit(0);
}

int probabilidad(int probabilidad) {
    srand(time(NULL));
    if((rand() % (100-1+1)+1)<=probabilidad){
        return 1;
    }
    return 0;
}

int posibilidad2(int posibilidad1, int posibilidad2){
	srand(time(NULL));
    
    int variable = rand() % (100-1+1)+1;
	if(variable <= posibilidad1){
		return 1;
	}else if(variable <= posibilidad2+posibilidad1){
		return 2;
	}else{
		return 0;
	}
}

int calculaAleatorios(int min, int max) {
    return rand() % (max-min+1) + min;
}

void writeLogMessage(int id, char *msg){
    //Calcula la hora actual
    time_t now=time(0);
    struct tm *tlocal= localtime(&now);
    char stnow[25];
    strftime(stnow,25,"%d/%m/%y %H:%M:%S",tlocal);
    //Escribe en el log
    logFile= fopen("logs.txt","a");
    fprintf(logFile, "[%s] %d: %s\n",stnow,id,msg);
    fclose(logFile);
}
