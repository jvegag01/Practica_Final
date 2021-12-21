#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>;
#include<signal.h>

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


int recepcionista_1,recepcionista_2,recepcionista_vip;      
struct recepcionista *recepcionistas;
struct cliente *clientes;
int *maquinasCheckIn;       // 0(libre) o 1(ocupada)
FILE *logFile;


int main(){
    if(signal(SIGUSR1,clienteNormal)==-1){
        perror("Error en la llamada a signal");
        exit(-1);
    }

    if(signal(SIGUSR2,clienteVip)==-1){
        perror("Error en la llamada a signal");
        exit(-1);
    }

    if(signal(SIGINT,terminar)==-1){
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
    


    return 0;
}


void clienteNormal(){
    if(signal(SIGUSR1,clienteNormal)==-1){
        perror("Error en la llamada a signal");
        exit(-1);
    }
}

void clienteVip(){
    if(signal(SIGUSR2,clienteNormal)==-1){
        perror("Error en la llamada a signal");
        exit(-1);
    }
}

void terminar(){
    if(signal(SIGINT,terminar)==-1){
        perror("Error en la llamada a signal");
        exit(-1);
    }
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