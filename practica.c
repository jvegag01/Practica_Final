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
    int id,atendido,tipo,ascensor;  /* atendido=0/1, tipo=0(normal),1(VIP),2(en máquina auto-checking) atendido=0(no atendido),1(esta siendo atendido),2(ya ha sido atendido)
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

    for(int i=0;i<20;i++){              // Inicializa las listas de clientes, máquinas check-in y recepcionistas.
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
        pthread_create(&hiloCliente,NULL,accionesCliente,NULL);
    }
}

void *accionesCliente(){

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