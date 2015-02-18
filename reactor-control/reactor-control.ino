/* Main sketch for system control and log of a pilot reactor
Cpyright (C) 2015 Diego Arellano: contacto [at] arellano.cc
License GPL v3 */

#include <SHT1x.h> // librería sensor temperatura y humedad.
#define dataPin 10 // pin 10 como entrada de datos.
#define clockPin 11 // pin 11 como sincronización de reloj.
SHT1x sht1x(dataPin, clockPin);

const int ledPin = 13; // Pin en el que se conecta LED para parpadear.
int incomingByte = 0; // For incoming serial data.

// Variables para el tiempo entre muestras.
long previous_time = 0;
const int time_interval = 2000; // 2 segundos
  
int try_hum = 0; // Intentos de aumentar HR.
int try_t = 0; // Interntos de modificar temperatura.

int i0 = 1; // Fase inicial del proceso.
int i;

void setup()
{
  pinMode(ledPin, OUTPUT); // Para parpadeo de LED.
  Serial.begin(9600);
  Serial.println("Humidity and Temperature sensor starting up'\n'");
}

void loop()
{
  if ( Serial.available() > 0 ){
    // Recibe informacion.
    i = Serial.read(); // Almacena la fase del proceso.
    i0 = i; // Guarda la fase en el valor por defecto, para que no vualva a la fase inicial.
  }
  else {
    i = i0;
  }
  
  float temp_c;
  float hum;
  
  // Read values from the sensor.
  unsigned long current_time = millis();
  // Solo si han transcurrido dos segundos entre medidas, tomar medición.
  if ( current_time - previous_time > time_interval ) {
    previous_time = current_time;
    temp_c = sht1x.readTemperatureC();
    hum = sht1x.readHumidity();
 
    //Print values
    Serial.print("Temperature: ");
    Serial.print(temp_c, DEC);
    Serial.print("C'\n'");
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.print("%'\n'");
  
    // Read values from the O2 sensor
  
    // Humidity control
    hum_control(i, try_hum, hum);
  
    // Temperature control
    tem_control(i, try_t, temp_c);
  
    // O2 control
    // o2_control(i, try_o2, o2);
  }
}

void hum_control( int i, int try_hum, float hum ){
  if ( i <= 2 ){
    // El control de humedad es identico se este en la fase 1 o 2.
    if ( hum >= 55 ){
      // HR no puede ser mayor a 60%
      Serial.print("HR maximo alcanzado '\n'");
      // Incluir codigo para impedir el ingreso de aire no recirculado y agua.
      flash( 1 ); // Hace parpadear el LED.
    }
    else if ( hum >= 50 ){
      try_hum = 0; // Reinicia el contador de intentos de aumentar HR.
    }
    else {
      // HR no puede ser menor a 45%.
      if ( try_hum < 1 ){
        // Es la primera vez que se intenta aumentar HR.
        Serial.print("HR cerca de minimo permitido '\n'");
        // Incluir codigo para ingresar aire no recirculado por 10 segundos.
        flash ( 2 );
        ++try_hum;
      }
      else {
        // Es la segunda o enesima vez que se intenta aumentar HR.
        Serial.print("HR cerca de minimo permitido. Intento ");
        Serial.print(try_hum);
        Serial.print(" '\n'");
        /* Incluir codigo para ingresar agua por 10 segundos.
        Este codigo se repetira hasta que se alcance la humedad deseada. */
        flash(3); // Parpadea 3 veces el led.
        ++try_hum;
      }
    }
  }
  else {
    /* El control de humedad es identico en las 3 ultimas fases,
    no puede ingresar agua al sistema. */  
    if ( hum >= 55 ){
      // HR no puede ser mayor a 60%
      Serial.print("HR maximo alcanzado '\n'");
      // Incluir codigo para impedir el ingreso de aire no recirculado y agua.
      flash ( 4 );
    }
    else if ( hum >= 50 ){
      try_hum = 0; // Reinicia el contador de intentos de aumentar HR.
    }
    else {
      // HR no puede ser menor a 45%
      Serial.print("HR cerca de minimo permitido. Intento ");
      Serial.print(try_hum);
      Serial.print(" '\n'");
      // Incluir codigo para ingresar aire no recirculado por 10 segundos. Este codigo se repetira hasta que se alcance la humedad deseada.
      flash( 5 );
      ++try_hum; 
    }
  }
}

// Contadores para almacenar mas de un valor de temperatura.
int counter_1 = 0;
int counter_2 = 0;
int counter_4 = 0;
int counter_5 = 0;
  
float t0; // Valor anterior de temperatura.
float dif_t; // Diferencia entre los valores de temperatura.
// Los valores de las siguientes variables puede modificarse usando parseInt.
float t_cons = 70.0; // Temperatura a mantener en fase 3.
float dif_max = 2.5; // Tolerancia a cambio de temperatura en fase 3.
int dif_t_min = 20; // La temperatura minima a la que se llegara en fase 4. t_cons - dif_t_min.

void tem_control ( int i, int try_t, float temp_c ) {
  /* Parte del control necesita comparar la temperatura de un periodo anterior.
  Para esto es necesario almacenar al menos dos valores de temperatura. */
  switch ( i ){
    case 1:
    {
      // Fase inicial de descomposicion aerobia. Temperatura aumenta hasta 40 °C.
      if ( counter_1 == 0 ){
        t0 = temp_c; // La primera vez que se ejecuta el programa, se almacena la primera medicion.
        dif_t = 0.0; // La primera vez que se ejecuta el programa no se calcula la diferencia entre las medidas.
        break;
      }
      else {
        ++counter_1;
        if ( temp_c >= 37 ){
          // La temperatura en esta fase no puede superar los 40 °C.
          Serial.print("Temperatura cerca de su valor maximo. ");
          if ( try_t < 1 ){
            Serial.print("Intento 1 '\n'");
            // Incluir codigo para ingresar aire no recirculado por 10 segundos.
            flash( 6 );
            ++try_t;
            break;
          }
          else {
            Serial.print("Intento ");
            Serial.print(try_t);
            Serial.print(" '\n'");
            // Incluir codigo para ingresar agua por 10 segundos. Este codigo se repetira hasta que se alcance la temperatura deseada.
            flash( 7 );
            ++try_t;
            break;
          }
        }
        else {
          dif_t = temp_c - t0;
          try_t = 0;
          /* La temperatura en un periodo de tiempo no deberia ser menor a la temperatura de tiempo anterior.
          Esto debido a que el sistema esta aumentando su temperatura. */
          if ( dif_t < 0 ){
            // Se debe hacer algo si la temperatura disminuye.
            Serial.print("El sistema se esta enfriando '\n'");
            // Ingresar codigo que impida el ingreso de aire no recirculado y de agua.
            flash ( 8 );
            break;
          }
        }
        t0 = temp_c; // Se actualiza la temperatura de comparacion para la siguiente vez que se ejecute el codigo.
        break;
      }
    }
    case 2:
    {
      // Segunda fase de descomposicion aerobia. Temperatura aumenta hasta un maximo, i. e. 70 °C.
      /* Se deben cumplir condiciones similares que en el caso anterior,
      la temperatura no puede sobrepasar un maximo y no puede disminuir la temperatura. */
      if ( counter_2 == 0 ){
        t0 = temp_c; // La primera vez que se ejecuta el programa, se almacena la primera medicion.
        dif_t = 0; // La primera vez que se ejecuta el programa no se calcula la diferencia entre las medidas.
        break;
      }
      else {
        ++counter_2;
        if ( temp_c >= t_cons - dif_max ){
          // La temperatura en esta fase no puede superar los 70 °C.
          Serial.print("Temperatura cerca de su valor maximo. ");
          if ( try_t < 1 ){
            Serial.print("Intento 1 '\n'");
            // Incluir codigo para ingresar aire no recirculado por 10 segundos.
            flash( 9 );
            ++try_t;
            break;
          }
          else {
            Serial.print("Intento ");
            Serial.print(try_t);
            Serial.print(" '\n'");
            // Incluir codigo para ingresar agua por 10 segundos. Este codigo se repetira hasta que se alcance la temperatura deseada.
            flash( 10 );
            ++try_t;
            break;
          }
        }
        else {
          dif_t = temp_c - t0;
          try_t = 0;
          /* La temperatura en un periodo de tiempo no deberia ser menor a la temperatura de tiempo anterior.
          Esto debido a que el sistema esta aumentando su temperatura. */
          if ( dif_t < 0 ){
            // Se debe hacer algo si la temperatura disminuye.
            Serial.print("El sistema se esta enfriando '\n'");
            // Ingresar codigo que impida el ingreso de aire no recirculado y de agua.
            flash ( 11 );
            break;
          }
        }
        t0 = temp_c; // Se actualiza la temperatura de comparacion para la siguiente vez que se ejecute el codigo.
        break;
      }
    }
    case 3:
    {
      /* Tercera fase de descomposicion aerobia. La temperatura debe mantenerse constante.
      Debido a que la fase se cambia manualmente, al cambiar a la fase 3,
      se debee indicar la temperatura que se quiere mantener constante.
      Tambien puede modificarse manualmente el valor de tolerancia.
      En esta fase no es necesario comparara valores previos. */
      dif_t = temp_c - t_cons;
      if ( abs( dif_t ) >= dif_max ){
        // La temperatura esta por sobre el rango permitido.
        if ( temp_c > t_cons ){
          // La temperatura es mayor de lo permitido.
          Serial.print("Temperatura excede temperatura maxima a mantener '\n'");
          if ( try_t < 1 ){
            // Primer intento de disminuir la temperatura.
            Serial.print("Intento 1 '\n'");
            // Incluir codigo para ingresar aire no recirculado por 10 segundos.
            flash( 12 );
            ++try_t;
            break;
          }
          else {
            Serial.print("Intento ");
            Serial.print(try_t);
            Serial.print(" '\n'");
            // Incluir codigo para ingresar agua por 10 segundos. Este codigo se repetira hasta que se alcance la temperatura deseada.
            flash( 13 );
            ++try_t;
            break;
          }
        }
        else {
          // La temperatura es menor a lo permitido.
          Serial.print("Temperatura es menor a temperatura maxima a mantener '\n'");
          Serial.print("Intento ");
          Serial.print(try_t);
          Serial.print(" '\n'");
          // Incluir codigo para impedir el ingreso de aire no recirculado o agua. Este codigo se repetira hasta que se alcance la temperatura deseada.
          flash( 14 );
          ++try_t;
          break;
        }
      }
      else {
        // Todo esta en orden.
        try_t = 0;
        break;
      }
    }
    case 4:
    {
      /* Cuarta fase de descomposicion aerobia. La temperatura disminuye hasta en 20 grados.
      El sistema no puede enfriarse mas alla de lo permitido, y no puede aumentar su temperatura.
      Puede modificarse manualmente la temperatura minima a la que llega el sistema. */
      if ( counter_4 == 0 ){
        t0 = temp_c; // La primera vez que se ejecuta el programa, se almacena la primera medicion.
        dif_t = 0; // La primera vez que se ejecuta el programa no se calcula la diferencia entre las medidas.
        break;
      }
      else {
        ++counter_4;
        float t_min = t_cons - dif_t_min;
        if ( temp_c < t_min ){
          // La temperatura en esta fase no puede disminuir mas alla de lo permitido.
          Serial.print("Temperatura en su valor minimo. ");
          Serial.print("Intento ");
          Serial.print(try_t);
          Serial.print(" '\n'");
          // Incluir codigo para impedir el ingreso de aire no recirculado y agua.
          flash( 15 );
          ++try_t;
          break;
        }
        else { 
          dif_t = temp_c - t0;
          /* La temperatura en un periodo de tiempo no deberia ser mayor a la temperatura de tiempo anterior.
          Esto debido a que el sistema esta disminuyendo su temperatura. */
          if ( dif_t > 0 ){
            // Se debe hacer algo si la temperatura aumenta.
            Serial.print("El sistema se esta calentando '\n'");
            // Ingresar codigo para ingresar aire no recirculado o agua.
            flash ( 16 );
            break;
          }
        }
        t0 = temp_c; // Se actualiza la temperatura de comparacion para la siguiente vez que se ejecute el codigo.
        break;
      }
    }
    case 5:
    {
      // Fase final de la descomposicion aerobia. El sistema se enfria hasta alcanzar la temperatura ambiente.
      if ( counter_5 == 0 ){
        t0 = temp_c; // La primera vez que se ejecuta el programa, se almacena la primera medicion.
        dif_t = 0; // La primera vez que se ejecuta el programa no se calcula la diferencia entre las medidas.
        break;
      }
      else {
        dif_t = temp_c - t0;
        /* La temperatura en un periodo de tiempo no deberia ser mayor a la temperatura de tiempo anterior.
        Esto debido a que el sistema esta disminuyendo su temperatura. */
        if ( dif_t > 0 ){
          // Se debe hacer algo si la temperatura aumenta.
          Serial.print("El sistema se esta calentando '\n'");
          // Ingresar codigo para ingresar aire no recirculado o agua.
          flash ( 17 );
          break;
        }
        t0 = temp_c; // Se actualiza la temperatura de comparacion para la siguiente vez que se ejecute el codigo.
        break;
      }
    }
  }
}

void flash (int n)
{
  for (int j = 0; j < n; j++)
  {
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
}
