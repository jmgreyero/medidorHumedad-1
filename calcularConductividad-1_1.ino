/*****************************************

  medidor de la conductividad del suelo
  v_1.1-20130226
  
  para el ARDUINO-UNO-r_3 con el shield DFR0019 (no imprescindible)
  compilador arduino-1.5.1r2
  el esquema KiCad está en MedidorConductividadSuelo.sch y en
  MedidorConductividadSuelo-sch.pdf

  -se mide la tensión en bornes de dos electrodos usando una resistencia
    en serie y un convertidor AD
  -el valor leído es proporcional a la resistencia entre electrodos
  -para evitar la polarización de los electrodos, se va alternando la
    polaridad haciendo la media de las dos medidas
  -se considera la medida correcta cuando el error entre dos lecturas
    consecutivas es menor que maxErrorAD
  -la resistencia depende del tipo de suelo y es proporcional
    al grado de humedad y a la concentración salina en el agua presente
    por lo que puede servir para valorar la concentración de abono y
    a partir de las constantes del sensor se puede calcular la conductividad
    del medio
  -en el display podemos ver el valor de la lectura en uS o bien el
    valor leído por el AD pulsando E_displayAD
  -por el port serie sale el valor seleccionado cada MAX_CICLOS_DISPLAY 
  -en el led tenemos de 0% a 100% respecto a los valores límite
    preestablecidos
  -con la opción DEBUG podemos ver las principales variables para poder
    comprobar el funcionamiento y ajustar los límites
   
******************************************/

#define  DEBUG  1

// hacer la media móvil cada MAX_LECTURAS
#define  MAX_LECTURAS        4
#define  ULTIMA_LECTURA      (MAX_LECTURAS-1)

// display de resultados cada MAX_CICLOS_DISPLAY de medida
#define  MAX_CICLOS_DISPLAY  1

// no empezar el display hasta que se estabilice la media
#define  RETARDO_DISPLAY      (MAX_LECTURAS+2)

// un led nos indica el valor relativo de la conductividad
// el periodo del led es proporcional a la conductividad
// y varía entre minLedOn y maxLedOn en tiempoLed
// entre los valores minLectura y maxLectura del AD
const int minLedOn = 5;      // ms
const int maxLedOn = 995;    // ms
int  periodoLed;
int  tiempoLedOn;
int  ledOn;
int  tiempoLed=0;                     // reloj para controlar el led
int  retardoDisplay=RETARDO_DISPLAY;  // al inicio espera a tener valores correctos

// terminales del ARDUINO-UNO que utilizo en este programa
// según el esquema medidorConductividadSuelo.sch
int S_electrodo             = 3;     // salida al electrodo del sensor
int S_resistenciaRetorno    = 5;     // resistencia en serie con el sensor
int S_led_1                 = 13;    // led piloto
int E_sensor                = A0;    // lectura de la tension
int E_displayAD             = 8;     // selector display conductividad/AD

// valores experimentales del AD obtenidos probando mi sonda
const int maxLectura = 650;    // tierra muy seca
const int minLectura = 195;    // la sonda sumergida en un recipiente
                               // con agua del grifo con sal
                               // ajustar los valores para cada caso

const int maxValorAD = 1023;  // el convertidor AD del Arduino es de 10 bits
const int maxErrorAD=1;       // consideramos que la lectura del primer pulso es
                              // válida cuando el error de dos lecturas
                              // sucesivas es <=maxErrorAD
                              // el segundo pulso debe durar lo mismo que el
                              // primero para mantener el valor de la tensión
                              // media en bornes de los electrodos igual a 0
// almacen temporal de lecturas del AD
int  lecturas[MAX_LECTURAS];

// valor medio de las ultimas lecturas
int  lecturaMedia;           // para el display y los cálculos
int  lecturaMediaLed;        // para la salida al led

unsigned int relojInterno;          // contador que lleva el reloj del ciclo
unsigned int maxRelojInterno=6000;  // duración máxima de un pulso de lectura
unsigned int relojInternoPaso_1;    // duración del primer pulso de lectura

// retardo minimo de la lectura del AD tras cada activacion del sensor
int  retardoLectura =  1000;

//***********************************************
// para calcular la conductividad g a partir de la constante de la sonda Kr
// y de la resistencia R calculada a partir de la medida del AD
//
// la constante de la sonda es
//
//  Kr=ln((2*W/D)-1)/(L*pi)    dimensiones en cm
//  g = Kr/R
//
// para a partir de R en ohm tener g en uS multiplicamos Kr*1e6

// estas son las dimensiones de mi sonda formada por dos electrodos
// iguales (en cm):
// L = 7.7  longitud
// W = 1.2  separación entre electrodos  
// D = 0.5  diámetro del electrodo
float Kr=51938.642;

// las constantes eléctricas del circuito
float R1 = 2200.0;
float AD_max= 1023.0;    // valor máximo de la lectura del AD (10 bits)

//***********************************************

void setup()
{
  // port serie, del que solo se usa la salida
  Serial.begin(9600);
  // entrada auxiliar del display
  pinMode(E_displayAD,INPUT);
  digitalWrite(E_displayAD,HIGH);  // incorporar resistencia pull-up
  // led auxiliar
  pinMode(S_led_1, OUTPUT);
  digitalWrite(S_led_1,LOW);
  // salida a los electrodos
  pinMode(S_electrodo, OUTPUT);
  pinMode(S_resistenciaRetorno, OUTPUT);
}

void loop() {
  int  lecturaSensor;
  int  i;
  int  periodoDisplay=0;                // para controlar la salida serie
  
  digitalWrite(S_electrodo,LOW);        // prepara los electrodos
  digitalWrite(S_resistenciaRetorno,LOW);
  
  for(i=0;i<MAX_LECTURAS;i++)  lecturas[i]=0;  // borrar el almacén de entradas
  periodoLed=maxLedOn+minLedOn;                // establecer el período del led
  
  ledOn=false;
  while (true){
    relojInterno=0;
    relojInternoPaso_1=maxRelojInterno;           // asegurar un valor máximo           
    digitalWrite(S_resistenciaRetorno,HIGH);      // activar sonda con R1 a positivo
    my_delay(retardoLectura);                     // esperar retardoLectura
    lecturaSensor=leerAD(true);                   // leer valor Vs en paso_1
    
    relojInternoPaso_1=relojInterno;              // los dos pasos deben durar lo mismo
    relojInterno=0;
    digitalWrite(S_resistenciaRetorno,LOW);       // cambio activacion
    digitalWrite(S_electrodo,HIGH);
    my_delay(retardoLectura);
    lecturaSensor += (maxValorAD-leerAD(false));  // Vs=ADmax-Vr
    digitalWrite(S_electrodo,LOW);
    
    lecturaSensor /=2;                            // media de las dos lecturas
    guardarLectura(lecturaSensor);                // guardarla en el almacén
    
    periodoDisplay++;                   // decidir si hay display en este ciclo
    if(periodoDisplay>=MAX_CICLOS_DISPLAY){
      periodoDisplay=0;
      if(retardoDisplay){              // al inicio espera a tener datos correctos
        retardoDisplay--;
        continue;
      }
                                       // decidir qué se envia al display
      if(digitalRead(E_displayAD)){    // pulsador libre == 1
#if DEBUG    
        Serial.print("AD=");
        Serial.println(lecturaMedia);
        Serial.print("tiempo=");
        Serial.println(relojInternoPaso_1);
        Serial.print(calcularResistencia(lecturaMedia),1);
        Serial.println(" ohm");
        Serial.print("%led=");
        Serial.println(lecturaMediaLed/10);        
#endif
        Serial.print(calcularConductividad(lecturaMedia),0);
        Serial.println(" uS/cm");
      }
      else{
        Serial.print("AD=");
        Serial.println(lecturaMedia);
      }
    }
  }
}

// temporizador para sincronizar el led
void my_delay(int t){
  int i;
  
  i= relojInterno + t;
  while(relojInterno<i){
      delay(1);                   // cada pulso del relojInterno es de 1ms
      relojInterno++;
      if(retardoDisplay)  continue;
      tiempoLed++;
      controlLed(tiempoLed);      // comprobar la activación del led
      if(tiempoLed>=periodoLed){
                                  // preparar el nuevo ciclo del led
        tiempoLed=0;
        tiempoLedOn=lecturaMediaLed;
      }
  }
}

//  leer el AD hasta que el error sea < maxErrorAD entre dos lecturas en el paso_1
//  el paso siguiente debe durar lo mismo que el primero para mantener la
//  simetría en la sonda
int  leerAD(int paso_1){
  int  ad_0,ad_1;
  
  ad_0=analogRead(E_sensor);
  while(true){                  // ir leyendo hasta el equilibrio (paso 1)
    my_delay(100);              // o hasta el maximo cada 100 pasos del reloj
    ad_1=analogRead(E_sensor);
    if(paso_1){
      if(abs(ad_0-ad_1) < maxErrorAD)  return ad_1;
    }                           // seguro de exceso de tiempo
    if(relojInterno>=relojInternoPaso_1)  return ad_1;
    ad_0=ad_1;
  }
}

// controlar el período de encendido del led
// queremos que el led esté más tiempo activado cuanto mayor sea la
// conductividad (mayor humedad en el terreno) que equivale a valor
// AD más alto
void  controlLed(int tiempoTranscurrido){
  
  if(retardoDisplay)  return;      // al principio se anula

  if(tiempoTranscurrido<=1){
    ledOn=true;
    digitalWrite(S_led_1,HIGH);
    return;
  }
  if(ledOn && tiempoTranscurrido>=tiempoLedOn){
    digitalWrite(S_led_1,LOW);
    ledOn=false;
  }
}

// guardar la lectura y hacer la media con las anteriores
void  guardarLectura(int ultimaLectura){
  int  i;
  
  lecturaMedia=0;
  for(i=0;i<ULTIMA_LECTURA;i++){
    lecturaMedia += lecturas[i];
    lecturas[i]=lecturas[i+1];
  }
  lecturas[i]=ultimaLectura;
  // hacer la media de todas las lecturas
  lecturaMedia = (lecturaMedia+ultimaLectura)/MAX_LECTURAS;
  // pasarla a porcentaje entre los limites para la activación del led
  // limitando los valores
  lecturaMediaLed=constrain(lecturaMedia,minLectura,maxLectura);
  lecturaMediaLed=periodoLed-map(lecturaMediaLed,minLectura,maxLectura,minLedOn,maxLedOn);
}

// a partir de la lectura del AD en bornes del sensor
float calcularResistencia(int ad){
  
  return R1*ad/(AD_max-ad);     // Rs = resistencia del sensor en ohm
}

// calcular la conductividad a partir de la lectura del AD
float calcularConductividad(int ad){
  return Kr/calcularResistencia(ad);      // conductividad
}

