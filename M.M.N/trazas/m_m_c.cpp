/* Definiciones externas para el sistema de colas simple */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../lcgrand.cpp" /* Encabezado para el generador de numeros aleatorios */

#define LIMITE_COLA 100 /* Capacidad maxima de la cola */
#define OCUPADO 1       /* Indicador de Servidor Ocupado */
#define LIBRE 0         /* Indicador de Servidor Libre */
#define LIMITE_SERVIDORES 100  /* Numero máximo de servidores en paralelo */

int sig_tipo_evento, num_clientes_espera, num_esperas_requerido, num_eventos,
    num_entra_cola, estado_servidor[LIMITE_SERVIDORES], num_servidores;
float area_num_entra_cola, area_estado_servidor[LIMITE_SERVIDORES], media_entre_llegadas, media_atencion,
    tiempo_simulacion, tiempo_llegada[LIMITE_COLA + 1], tiempo_ultimo_evento, tiempo_sig_evento[3],
    total_de_esperas, tiempo_salida[LIMITE_SERVIDORES + 1];
FILE *parametros, *resultados, *traza;

void inicializar(void);
void controltiempo(void);
void llegada(void);
void salida(void);
void reportes(void);
void actualizar_estad_prom_tiempo(void);
float expon(float mean);

int main(void) /* Funcion Principal */
{
    /* Abre los archivos de entrada y salida */

    /* Inicializa la simulacion. */

    inicializar();

    /* Corre la simulacion mientras no se llegue al numero de clientes especificaco en el archivo de entrada*/

    while (num_clientes_espera < num_esperas_requerido)
    {

        /* Determina el siguiente evento */

        controltiempo();

        /* Invoca la funcion del evento adecuado. */

        switch (sig_tipo_evento)
        {
        case 1:
            llegada();
            break;
        case 2:
            salida();
            break;
        }
    }

    /* Invoca el generador de reportes y termina la simulacion. */

    reportes();

    fclose(parametros);
    fclose(resultados);
    fclose(traza);

    return 0;
}

void inicializar(void) /* Funcion de inicializacion. */
{
    parametros = fopen("param.txt", "r");
    resultados = fopen("result_m_m_c.txt", "w");
    traza = fopen("traza_m_m_c.txt","w");

    /* Especifica el numero de eventos para la funcion controltiempo. */

    num_eventos = 2;

    /* Lee los parametros de enrtrada. */

    fscanf(parametros, "%f %f %d %d", &media_entre_llegadas, &media_atencion,
           &num_esperas_requerido, &num_servidores);

    /* Inicializa el reloj de la simulacion. */

    tiempo_simulacion = 0.0;

    /* Inicializa las variables de estado */
    for(int n = 0; n < num_servidores; n++){
        estado_servidor[n] = LIBRE;
    }
    for(int n = 0; n < LIMITE_SERVIDORES; n++){
        tiempo_salida[n] = 1.0e+30;
    }
    num_entra_cola = 0;
    tiempo_ultimo_evento = 0.0;

    /* Inicializa los contadores estadisticos. */

    num_clientes_espera = 0;
    total_de_esperas = 0.0;
    area_num_entra_cola = 0.0;
    for(int n = 0; n < num_servidores; n++){
        area_estado_servidor[0] = 0.0;
    }

    /* Inicializa la lista de eventos. Ya que no hay clientes, el evento salida
       (terminacion del servicio) no se tiene en cuenta */

    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
    tiempo_sig_evento[2] = 1.0e+30;

    fprintf(traza, "Inicializacion\n\n");
    fprintf(traza, "tiempo evento de llegada: %f, tiempo evento de salida: %f\n",tiempo_sig_evento[1], tiempo_sig_evento[2]);

}

void controltiempo(void) /* Funcion controltiempo */
{
    fprintf(traza, "\nControl Tiempo (ts = %f)\n\n",tiempo_simulacion);
    int i;
    float min_tiempo_sig_evento = 1.0e+29;

    tiempo_sig_evento[2] = 1.0e+29;

    //// Determinar el menor de los posibles tiempos de salida ////

    for (int k = 0; k < num_servidores; k++){
        if(tiempo_salida[k] <= tiempo_sig_evento[2]){
            tiempo_sig_evento[2] = tiempo_salida[k]; 
        }
    }

    fprintf(traza, "tiempo sig evento de salida: %f\n", tiempo_sig_evento[2]);
    
    sig_tipo_evento = 0;

    /*  Determina el tipo de evento del evento que debe ocurrir. */

    for (i = 1; i <= num_eventos; ++i)
        if (tiempo_sig_evento[i] < min_tiempo_sig_evento)
        {
            min_tiempo_sig_evento = tiempo_sig_evento[i];
            sig_tipo_evento = i;
        }
    
    fprintf(traza, "min tiempo sig evento: %f\n",min_tiempo_sig_evento);
    fprintf(traza, "sig evento (0 = ninguno, 1 = llegada, 2 = salida): %d\n",sig_tipo_evento);

    /* Revisa si la lista de eventos esta vacia. */

    if (sig_tipo_evento == 0)
    {

        /* La lista de eventos esta vacia, se detiene la simulacion. */

        fprintf(resultados, "\nLa lista de eventos esta vacia %f", tiempo_simulacion);
        exit(1);
    }

    /* TLa lista de eventos no esta vacia, adelanta el reloj de la simulacion. */

    tiempo_simulacion = min_tiempo_sig_evento;

    /* Actualiza los acumuladores de area para las estadisticas de tiempo promedio. */
    float time_since_last_event;

    /* Calcula el tiempo desde el ultimo evento, y actualiza el marcador
    	del ultimo evento */

    time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
    tiempo_ultimo_evento = tiempo_simulacion;

    /* Actualiza el area bajo la funcion de numero_en_cola */
    area_num_entra_cola += num_entra_cola * time_since_last_event;

    fprintf(traza, "area num entra cola: %f\n",area_num_entra_cola);

    /*Actualiza el area bajo la funcion indicadora de servidor ocupado*/
    for(int n = 0; n < num_servidores; n++){
    area_estado_servidor[n] += estado_servidor[n] * time_since_last_event;
    fprintf(traza,"Servidor %d ocupado? %d, area estado servidor: %f\n",n, estado_servidor[n], area_estado_servidor[n]);
    }
    
}

void llegada(void) /* Funcion de llegada */
{
    fprintf( traza,"\nEvento Llegada (ts = %f)\n\n",tiempo_simulacion);
    float espera;

    /* Programa la siguiente llegada. */

    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);

    fprintf( traza,"Tiempo siguiente llegada programado a: %f\n",tiempo_sig_evento[1]);

    /* Reisa si el servidor esta OCUPADO. */
    
    int evento_programado = 0;    

    for(int n = 0; n < num_servidores; n++){
        if (estado_servidor[n] == LIBRE){
            /*  El servidor esta LIBRE, por lo tanto el cliente que llega tiene tiempo de eespera=0
           (Las siguientes dos lineas del programa son para claridad, y no afectan
           el reultado de la simulacion ) */

            espera = 0.0;
            total_de_esperas += espera;

            /* Incrementa el numero de clientes en espera, y pasa el servidor a ocupado */
            ++num_clientes_espera;
            estado_servidor[n] = OCUPADO;
            fprintf( traza,"Servidor %d LIBRE, pasa a OCUPADO. num clientes esperados incrementado: %d\n",n,num_clientes_espera);

            /* Programa una salida ( servicio terminado ). */

            tiempo_salida[n] = tiempo_simulacion + expon(media_atencion);
            tiempo_sig_evento[2] = tiempo_salida[n];
            fprintf( traza,"Tiempo siguiente salida programado a: %f\n",tiempo_sig_evento[2]);

            //// El evento ya ha sido programado ////
            evento_programado = 1;
            break;
        }
    }

    //// Si el evento no fue programado, significa que todos los servidores están ocupados ////
    if(evento_programado == 0){
        /* Servidor OCUPADO, aumenta el numero de clientes en cola */

        ++num_entra_cola;
        fprintf( traza,"Servidor ocupado, agregado a la cola. Num de clientes en cola: %d\n",num_entra_cola);
        /* Verifica si hay condici�n de desbordamiento */

        if (num_entra_cola > LIMITE_COLA)
        {

            /* Se ha desbordado la cola, detiene la simulacion */

            fprintf(resultados, "\nDesbordamiento del arreglo tiempo_llegada a la hora: ");
            fprintf(resultados, "%f", tiempo_simulacion);
            exit(2);
        }

        /* Todavia hay espacio en la cola, se almacena el tiempo de llegada del
        	cliente en el ( nuevo ) fin de tiempo_llegada */

        tiempo_llegada[num_entra_cola] = tiempo_simulacion;
    }

}

void salida(void) /* Funcion de Salida. */
{
    fprintf( traza,"\nEvento Salida (ts = %f)\n\n",tiempo_simulacion);
    int i;
    float espera;
    int n;

    ////  Encontrar el servidor que se libera ////
    for(n = 0; n < num_servidores; n++){
        if(tiempo_simulacion == tiempo_salida[n]){
            break;
        }
    }

    /* Revisa si la cola esta vacia */

    if (num_entra_cola == 0)
    {

        //// Liberar el Servidor ////
        estado_servidor[n] = LIBRE;
        tiempo_salida[n] = 1.0e+30;
        tiempo_sig_evento[2] = tiempo_salida[n];
        fprintf( traza,"No hay nada en la cola. Tiempo del siguiente evento de salida desconocido\n");
    }

    else
    {

        /* La cola no esta vacia, disminuye el numero de clientes en cola. */
        fprintf( traza,"Hay %d elementos en la cola, quitando uno\n",num_entra_cola);
        --num_entra_cola;
        
        /*Calcula la espera del cliente que esta siendo atendido y
        actualiza el acumulador de espera */

        espera = tiempo_simulacion - tiempo_llegada[1];
        total_de_esperas += espera;

        fprintf( traza,"Tiempo de espera del ultimo elemento: %f, tiempo total de espera: %f\n",espera, total_de_esperas);

        /*Incrementa el numero de clientes en espera, y programa la salida. */
        ++num_clientes_espera;
        
        tiempo_salida[n] = tiempo_simulacion + expon(media_atencion);
        tiempo_sig_evento[2] = tiempo_salida[n];
        //tiempo_sig_evento[2] = tiempo_simulacion + expon(media_atencion);

        fprintf( traza,"Numero de clientes esperados incrementado: %d\n",num_clientes_espera);

        /* Mueve cada cliente en la cola ( si los hay ) una posicion hacia adelante */
        for (i = 1; i <= num_entra_cola; ++i)
            tiempo_llegada[i] = tiempo_llegada[i + 1];
    }
}

void reportes(void) /* Funcion generadora de reportes. */
{
    /* Escribe en el archivo de salida los encabezados del reporte y los parametros iniciales */

    fprintf(resultados, "Sistema de Colas Simple\n\n");
    fprintf(resultados, "Tiempo promedio de llegada%11.3f minutos\n\n",
            media_entre_llegadas);
    fprintf(resultados, "Tiempo promedio de atencion%16.3f minutos\n\n", media_atencion);
    fprintf(resultados, "Numero de servidores %14d\n\n", num_servidores);
    fprintf(resultados, "Numero de clientes %14d\n\n", num_esperas_requerido);

    /* Calcula y estima los estimados de las medidas deseadas de desempe�o */
    fprintf(resultados, "\n\nEspera promedio en la cola%11.3f minutos\n\n",
            total_de_esperas / num_clientes_espera);
    fprintf(resultados, "Numero promedio en cola%10.3f\n\n",
            area_num_entra_cola / tiempo_simulacion);
    float acum_uso_sistema = 0.0;
    for(int n = 0; n < num_servidores;n++){
        float uso_servidor = area_estado_servidor[n] / tiempo_simulacion; 
        acum_uso_sistema += uso_servidor;
        fprintf(resultados, "Uso del servidor %d: %15.3f\n\n", n,
            uso_servidor);
    }
    fprintf(resultados, "Uso promedio del sistema%12.3f\n\n",acum_uso_sistema/num_servidores);
    fprintf(resultados, "Tiempo de terminacion de la simulacion%12.3f minutos", tiempo_simulacion);
}

float expon(float media) /* Funcion generadora de la exponencias */
{
    /* Retorna una variable aleatoria exponencial con media "media"*/

    return -media * log(lcgrand(1));
}
