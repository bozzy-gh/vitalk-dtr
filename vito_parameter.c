#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

#include "vito_parameter.h"
#include "vito_io.h"
#include "vito_dtr.h"
#include "fehlerliste.h"

#define CACHE_TIME 2


/*
 * translation hints
 *
 * HEIZKREIS  -> HEATING_CIRCUIT
 * BRENNER    -> BURNER
 * WARMWASSER -> HOT_WATER
 * ALLGEMEIN  -> GENERAL
 *
 */

// Das prologue() Makro wird verwendet, um den Anfang der
// Parameterfunktionen zu bauen:
#define prologue() \
   static time_t old_time = 0; \
   uint8_t vitomem[30]; \
   time_t new_time = time(NULL) / (CACHE_TIME); \
   if ( new_time > old_time ) \
   {

// Das epilogue() Makro wird verwendet, um das Ende der
// Parameterfunktionen zu bauen:
#define epilogue() \
   old_time = new_time; \
   } \
   return cache;

// OBACHT:
// wenn in den vito_read() Aufrufen der Parameterfunktionen ein
// Fehler auftritt (-1), muss gleich mit return beendet werden,
// damit old_time=new_time nicht erreicht wird und beim naechsten
// Aufruf ein fehlerhafter Wert aus dem Cache statt einem Fehler
// zurueckgegeben wird.

////////////////////////// PARAMETERFUNKTIONEN ////////////////////

/* -------------------------------- */
const char * const read_deviceid( void )
{
  static char cache[10];
  prologue()
    if ( vito_read( 0x00f8, 2, vitomem ) < 0 )
      return "NULL";
  // Normalerweise sind die Parameter in Little Endian
  // Byteorder, aber bei der Deviceid hat sich offenbar
  // die umgekehrte Interpretation durchgesetzt:
  sprintf( cache, "0x%4x", (vitomem[0] << 8) + vitomem[1] );
  epilogue()
}

/* -------------------------------- */
const char * const read_systemtime( void )
{
  static char cache[16];
  prologue()
    if ( vito_read( 0x088e, 8, vitomem ) < 0 )
      return "NULL";
  sprintf( cache, "%02X%02X%02X%02X%02X%02X%02X%02X", vitomem[0], vitomem[1], vitomem[2], vitomem[3], vitomem[4], vitomem[5], vitomem[6], vitomem[7] );
  epilogue()
}

/* -------------------------------- */
const char * const write_systemtime( const char * value_str )
{
    uint8_t vito_date[8];
    time_t tt;
    struct tm* t;

    time(&tt);
    t = localtime(&tt);
    vito_date[0] = TOBCD((t->tm_year + 1900) / 100);
    vito_date[1] = TOBCD(t->tm_year - 100 ); // according to the range settable on the Vitodens LCD frontpanel
    vito_date[2] = TOBCD(t->tm_mon + 1);
    vito_date[3] = TOBCD(t->tm_mday);
    vito_date[4] = TOBCD(t->tm_wday);
    vito_date[5] = TOBCD(t->tm_hour);
    vito_date[6] = TOBCD(t->tm_min);
    vito_date[7] = TOBCD(t->tm_sec);
    if ( vito_write(0x088e, 8, vito_date) < 0)
    {
        return "Vitodens communication Error on setting systemtime";
    }
    else
    {
    return "OK";
    }
}
/*   modes : https://gist.github.com/mqu/9519e39ccc474f111ffb#file-rvitalk-rb-L662
 *  - for 0x20CB device :
 *  - mode       : 0x2323
 *  - eco mode   : 0x2302 -> 0x2331   ; see https://github.com/mqu/vitalk/issues/1
 *  - party mode : 0x2303 -> 0x2330
 */
/* MQU -------------------------------- */
const char * const read_mode( void )
{
  static char cache[5];
  prologue()
    if ( vito_read( 0x2323, 1, vitomem ) < 0 )
      return "NULL";
  sprintf( cache, "%u", vitomem[0] );
  epilogue()
}

/* MQU -------------------------------- */
const char * const read_eco_mode( void )
{
  static char cache[5];
  prologue()
    if ( vito_read( 0x2331, 1, vitomem ) < 0 )
      return "NULL";
  sprintf( cache, "%u", vitomem[0] );
  epilogue()
}

/* MQU -------------------------------- */
const char * const read_party_mode( void )
{
  static char cache[5];
  prologue()
    if ( vito_read( 0x2330, 1, vitomem ) < 0 )
      return "NULL";
  sprintf( cache, "%u", vitomem[0] );
  epilogue()
}

/* -------------------------------- */
const char * const write_mode( const char * value_str )
{
  uint8_t content[3];
  int mode;

  mode = atoi( value_str );
  // Dauernd reduziert und dauernd normal unterstuetzt meine Vitodens offenbar nicht:
  if ( mode < 0 || mode > 2 )
    return "Illegal Mode!";

  content[0] = mode & 0xff; // unnoetig, aber deutlicher
  if ( vito_write(0x2323, 1, content) < 0 )
    return "Vitodens communication Error";
  else
    return "OK";
}

/* MQU -------------------------------- */
const char * const write_eco_mode( const char * value_str )
{
  uint8_t content[3];
  int mode;

  mode = atoi( value_str );
  // Dauernd reduziert und dauernd normal unterstuetzt meine Vitodens offenbar nicht:
  if ( mode < 0 || mode > 1 )
    return "Illegal Mode!";

  content[0] = mode & 0xff; // unnoetig, aber deutlicher
  if ( vito_write(0x2331, 1, content) < 0 )
    return "Vitodens communication Error";
  else
    return "OK";
}

/* MQU -------------------------------- */
const char * const write_party_mode( const char * value_str )
{
  uint8_t content[3];
  int mode;

  mode = atoi( value_str );
  // Dauernd reduziert und dauernd normal unterstuetzt meine Vitodens offenbar nicht:
  if ( mode < 0 || mode > 1 )
    return "Illegal Mode!";

  content[0] = mode & 0xff; // unnoetig, aber deutlicher
  if ( vito_write(0x2330, 1, content) < 0 )
    return "Vitodens communication Error";
  else
    return "OK";
}

/* -------------------------------- */
const char * const read_mode_text( void )
{
  const char * const mode = read_mode();

  if ( strcmp(mode,"NULL") == 0)
    return "NULL";

   //:mode => ['water heating only', 'continuous reduced', 'constant normal', 'heating + hot water', 'heating + hot water', 'Off'],
   // may be :  0=only Water Heating; 1=Continuous reduced; 2=constant normal; 3=heat+WH; 4=heat + WH ; 5=off

  switch ( atoi(mode) )
    {
    case 0: return "Off";
    case 1: return "Hot Water";
    case 2: return "Heating and hot water";
    case 5: return "Off";
    default: return "UNKNOWN";
    }
}

/* -------------------------------- */
// Das Auslesen der Fehlerliste ist etwas konfus, denn in dem
// vito.xml file von ceteris paribus werden 9 byte pro Eintrag
// gelesen. Ich sehe den Sinn aber nicht? Ich muss mal noch beobachten was passiert
// wenn mehr als ein Eintrag in der Fehlerliste steht!
const char * const read_errors( void )
{
  static char cache[50];
  prologue()

  int i;

  // Die 10 Fehlermeldungen einlesen:
  for ( i=0; i <= 9; i++)
    {
      if ( vito_read( 0x7507 + (i*0x0009), 1, vitomem ) < 0 )
    return "NULL";

      sprintf( cache + (4*i), "%03u,", vitomem[0] );
    }
  cache[39]='\0'; //letztes Komma ueberschreiben

  epilogue()
}

/* -------------------------------- */
// Die 10 Fehlerspeicherplaetze als Textstrings zurueckgeben:
const char * const read_errors_text( void )
{
  static char cache[80*10];
  char zeile[80];
  const char * errors;
  int n_error;
  int i;

  errors = read_errors();
  if ( strcmp( errors, "NULL" ) == 0 )
    return "NULL";

  cache[0] = '\0';
  for ( i = 0; i <= 9; i++ ) // 10 Fehler
    {
      n_error = atoi(errors + (i*4));
      assert ( n_error >= 0 && n_error <= 255 );
      sprintf( zeile, "0x%02x %s\n", n_error, fehlerliste[n_error] );
      strcat( cache, zeile );
    }

  return cache;
}


//////////////////// BOILER
/* -------------------------------- */
const char * const read_abgas_temp( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x0808, 2, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%3.2f", ( vitomem[0] + (vitomem[1] << 8)) / 10.0 );
  epilogue()
}

/* -------------------------------- */
// boiler_temp
const char * const read_k_ist_temp( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x0802, 2, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%3.2f", ( vitomem[0] + (vitomem[1] << 8)) / 10.0 );
  epilogue()
}

const char * const read_k_ist_temp_tp( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x0810, 2, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%3.2f", ( vitomem[0] + (vitomem[1] << 8)) / 10.0 );
  epilogue()
}

// boiler_temp_set
const char * const read_k_soll_temp( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x555a, 2, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%3.2f", ( vitomem[0] + (vitomem[1] << 8)) / 10.0 );
  epilogue()
}

//////////////////// HOT WATER
/* -------------------------------- */
const char * const read_ww_soll_temp( void)
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x6300, 1, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%u", vitomem[0] );
  epilogue()
}

/* -------------------------------- */
const char * const write_ww_soll_temp( const char *value_str )
{
  uint8_t content[3];
  int temp;

  temp = atoi( value_str );
  if ( temp < 5 || temp > 60 )
    return "WW_soll_temp: range exceeded!";

  content[0] = temp & 0xff; // unnoetig, aber deutlicher
  if ( vito_write(0x6300, 1, content) < 0 )
    return "Vitodens communication Error";
  else
    return "OK";
}

/* -------------------------------- */
// target Boiler Offset
const char * const read_ww_offset( void)
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x6760, 1, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%u", vitomem[0] );
  epilogue()
}

/* -------------------------------- */
// hot_water_temp_lp
const char * const read_ww_ist_temp_tp( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x0812, 2, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%3.2f", ( vitomem[0] + (vitomem[1] << 8)) / 10.0 );
  epilogue()
}

/* -------------------------------- */
// hot_water_temp
const char * const read_ww_ist_temp( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x0804, 2, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%3.2f", ( vitomem[0] + (vitomem[1] << 8)) / 10.0 );
  epilogue()
}


/////////////////// OUTDOOR TEMPERATURE
/* -------------------------------- */
const char * const read_outdoor_temp_tp( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x5525, 2, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%3.2f", ((int16_t)( vitomem[0] + (vitomem[1] << 8))) / 10.0 );
  epilogue()
}

/* -------------------------------- */
const char * const read_outdoor_temp_smooth( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x5527, 2, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%3.2f", ((int16_t)( vitomem[0] + (vitomem[1] << 8))) / 10.0 );
  epilogue()
}

/* -------------------------------- */
const char * const read_outdoor_temp( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x0800, 2, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%3.2f", ((int16_t)( vitomem[0] + (vitomem[1] << 8))) / 10.0 );
  epilogue()
}

/* -------------------------------- */
const char * const read_indoor_temp( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x0896, 2, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%3.2f", ((int16_t)( vitomem[0] + (vitomem[1] << 8))) / 10.0 );
  epilogue()
}

/////////////////// BURNER
/* -------------------------------- */
const char * const read_starts(void )
{
  static char cache[20];
  prologue()
    if ( vito_read( 0x088A, 4, vitomem) < 0 )
      return "NULL";

  unsigned int value;

  value = vitomem[0] + (vitomem[1] << 8) + (vitomem[2] << 16) + (vitomem[3] << 24);
      sprintf( cache, "%u", value );

  epilogue()
}

/* -------------------------------- */
const char * const read_runtime( void )
{
  static char cache[20];
  prologue()
    if ( vito_read( 0x0886, 4, vitomem) < 0 )
      return "NULL";

  unsigned int value;

  value = vitomem[0] + (vitomem[1] << 8) + (vitomem[2] << 16) + (vitomem[3] << 24);
      sprintf( cache, "%u", value );

  epilogue()
}

/* -------------------------------- */
const char * const read_runtime_h( void )
{
  static char cache[20];
  const char *result;

  result = read_runtime();

  if ( strcmp( result, "NULL" ) == 0 )
    return "NULL";

  sprintf( cache, "%06.1f", atoi(result) / 3600.0 );
  return cache;
}

/* -------------------------------- */
const char * const read_power( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0xa38f, 1, vitomem) < 0 )
      return "NULL";

  sprintf( cache, "%3.1f", vitomem[0] / 2.0 );
  epilogue()
}

/////////////////// HYDRAULICS
/* -------------------------------- */
// valve position
const char * const read_ventil( void )
{
  static char cache[5];
  prologue()
    if ( vito_read( 0x0a10, 1, vitomem) < 0 )
      return "NULL";

  sprintf( cache, "%u", vitomem[0] );
  epilogue()
}

/* -------------------------------- */
// switching_valve
const char * const read_ventil_text( void )
{
  const char *result;

  result = read_ventil();

  if ( strcmp( result, "NULL" ) == 0 )
    return "NULL";

  switch (atoi(result))
    {
    case 0: return "undefined";
    case 1: return "heating";
    case 2: return "middle position";
    case 3: return "hot water";
    default: return "UNKNOWN";
    }
}

/* -------------------------------- */
const char * const read_pump_power( void )
{
  static char cache[5];
  prologue()
    if ( vito_read( 0x0a3c, 1, vitomem) < 0 )
      return "NULL";

  sprintf( cache, "%u", vitomem[0] );
  epilogue()
}

/* -------------------------------- */
const char * const read_flow( void )
{
  static char cache[10];
  prologue()
     if ( vito_read( 0x0c24, 2, vitomem) < 0 )
      return "NULL";

  sprintf( cache, "%u", (vitomem[0] + (vitomem[1] << 8)) );
  epilogue()
}

/////////////////// HEATING CIRCUIT
/* -------------------------------- */
// circuit_flow_temp
const char * const read_vl_soll_temp( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x2544, 2, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%3.2f", ( vitomem[0] + (vitomem[1] << 8)) / 10.0 );
  epilogue()
}

/* -------------------------------- */
// norm_room_temp
const char * const read_raum_soll_temp( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x2306, 1, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%u", vitomem[0] );
  epilogue()
}

/* -------------------------------- */
const char * const write_raum_soll_temp( const char *value_str )
{
  uint8_t content[3];
  int temp;

  temp = atoi( value_str );

  if ( temp < 10 || temp > 30 )
    return "Raum_soll_temp: range exceeded!";

  content[0] = temp & 0xff; // unnoetig, aber deutlicher
  if ( vito_write(0x2306, 1, content) < 0 )
    return "Vitodens communication Error";
  else
    return "OK";
}

/* -------------------------------- */
// reduce_room_temp
const char * const read_red_raum_soll_temp( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x2307, 1, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%u", vitomem[0] );
  epilogue()
}

/* -------------------------------- */
const char * const write_red_raum_soll_temp( const char *value_str )
{
  uint8_t content[3];
  int temp;

  temp = atoi( value_str );

  if ( temp < 10 || temp > 30 )
    return "Raum_soll_temp: range exceeded!";

  content[0] = temp & 0xff; // unnoetig, aber deutlicher
  if ( vito_write(0x2307, 1, content) < 0 )
    return "Vitodens communication Error";
  else
    return "OK";
}

/* -------------------------------- */
/* curve_slope */
const char * const read_neigung( void )
{
  static char cache[6];
  prologue()
    if ( vito_read( 0x27d3, 1, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%2.1f", vitomem[0] / 10.0 );
  epilogue()
}

/* -------------------------------- */
/* curve_level */
const char * const read_niveau( void )
{
  static char cache[5];
  prologue()
    if ( vito_read( 0x27d4, 1, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%u", vitomem[0] );
  epilogue()
}

/* -------------------------------- */
const char * const read_pp_max( void )
{
  static char cache[5];
  prologue()
    if ( vito_read( 0x27e6, 1, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%u", vitomem[0] );
  epilogue()
}

/* -------------------------------- */
const char * const write_pp_max( const char *value_str )
{
  uint8_t content[3];
  int temp;

  temp = atoi( value_str );

  if ( temp < 0 || temp > 100 )
    return "Max. Pumpenleistung: range exceeded!";

  content[0] = temp & 0xff; // unnoetig, aber deutlicher
  if ( vito_write(0x27e6, 1, content) < 0 )
    return "Vitodens communication Error";
  else
    return "OK";
}

/* -------------------------------- */
const char * const read_pp_min( void )
{
  static char cache[5];
  prologue()
    if ( vito_read( 0x27e7, 1, vitomem) < 0 )
      return "NULL";
  sprintf( cache, "%u", vitomem[0] );
  epilogue()
}

/* -------------------------------- */
const char * const write_pp_min( const char *value_str )
{
  uint8_t content[3];
  int temp;

  temp = atoi( value_str );

  if ( temp < 0 || temp > 100 )
    return "Min. Pumpenleistung: range exceeded!";

  content[0] = temp & 0xff; // unnoetig, aber deutlicher
  if ( vito_write(0x27e7, 1, content) < 0 )
    return "Vitodens communication Error";
  else
    return "OK";
}

//////////////////////////////////////////////////////////////////////////
// obacht: maximale Befehlslaenge 20 Zeichen, sonst klemmt der telnet-parser
// know to work for deviceid: 0x20cb
const struct s_parameter parameter_liste[] = {
  { "errors", "Error History (numerisch)", "", P_ERRORS, &read_errors, NULL },
  { "errors_text", "Error History (text)", "", P_ERRORS, &read_errors_text, NULL },
  { "deviceid", "Device ID", "", P_GENERAL, &read_deviceid, NULL },
  { "system_time", "System time", "", P_GENERAL, &read_systemtime, &write_systemtime },
  { "mode", "operating mode (numerisch)", "", P_GENERAL, &read_mode, &write_mode },
  { "eco_mode", "Econimic Mode", "", P_GENERAL, &read_eco_mode, &write_eco_mode },
  { "party_mode", "Party mode", "", P_GENERAL, &read_party_mode, &write_party_mode },
  { "mode_text", "operating mode (text)", "", P_GENERAL, &read_mode_text, NULL },
  { "indoor_temp", "Indoor temperature", "�C", P_GENERAL, &read_indoor_temp, NULL },
  { "outdoor_temp", "Outdoor temperature", "�C", P_GENERAL, &read_outdoor_temp, NULL },
  { "outdoor_temp_lp", "Outdoor temp / low_pass", "�C", P_GENERAL, &read_outdoor_temp_tp, NULL },
  { "outdoor_temp_smooth", "Outdoor temp / smooth", "�C", P_GENERAL, &read_outdoor_temp_smooth, NULL },
  { "boiler_temp", "Boiler temperature", "�C", P_BOILER, &read_k_ist_temp, NULL },
  { "boiler_temp_lp", "Boiler temp _ low pass", "�C", P_BOILER, &read_k_ist_temp_tp, NULL },
  { "set_boiler_temp", "Boiler setpoint temperature", "�C", P_BOILER, &read_k_soll_temp, NULL },
  { "boiler_gaz_temp", "Boiler flue gas temperature", "�C", P_BOILER, &read_abgas_temp, NULL },
  { "hot_water_set", "Hot water setting", "�C", P_HOTWATER, &read_ww_soll_temp, &write_ww_soll_temp },
  { "hot_water_temp", "Hot water temperature", "�C", P_HOTWATER, &read_ww_ist_temp, NULL },
  { "hot_water_temp_lp", "Hot water temperature low_pass", "�C", P_HOTWATER, &read_ww_ist_temp_tp, NULL },
  { "boiler_offet", "boiler Offset", "K", P_HOTWATER, &read_ww_offset, NULL },
  { "flow_temp_set", "flow temp setting", "�C", P_HEATING, &read_vl_soll_temp, NULL },
  { "norm_room_temp", "room temp setting", "�C", P_HEATING, &read_raum_soll_temp, &write_raum_soll_temp },
  { "red_room_temp", "reduced room temp setting", "�C", P_HEATING, &read_red_raum_soll_temp, &write_red_raum_soll_temp },
  { "curve_level", "Curve level", "K", P_HEATING, &read_niveau, NULL },
  { "curve_slope", "Curve  slope", "", P_HEATING, &read_neigung, NULL },
  { "pp_max", "Maximal pomp power", "%", P_HEATING, &read_pp_max, &write_pp_max },
  { "pp_min", "Minimal pomp power", "%", P_HEATING, &read_pp_min, &write_pp_min },
  { "starts", "Heater Starts", "", P_BURNER, &read_starts, NULL },
  { "runtime_h", "Runtime in hours", "h", P_BURNER, &read_runtime_h, NULL },
  { "runtime", "Runtime in seconds", "s", P_BURNER, &read_runtime, NULL },
  { "power", "Power in %", "%", P_BURNER, &read_power, NULL },
  { "valve_setting", "valve setting", "", P_HYDRAULIC, &read_ventil, NULL },
  { "valve_setting_text", "valve setting / text", "", P_HYDRAULIC, &read_ventil_text, NULL },
  { "pump_power", "Pump power", "%", P_HYDRAULIC, &read_pump_power, NULL },
/*  { "flow", "Volumenstrom", "l/h", P_HYDRAULIC, &read_flow, NULL }, */
  { NULL, NULL, NULL, 0, NULL, NULL }
};

/////////////////////////////////////////////////////////////////////////////////

// Parameter Wert nachschlagen nach Name
const char * const get_v( const char *name )
{
  int i=0;

  while( parameter_liste[i].p_name ) // Ende der Liste ?
    {
      if ( strcmp( name, parameter_liste[i].p_name ) == 0 ) // Parametername gefunden?
    {
      if ( parameter_liste[i].f_read ) // Gibts eine Zugriffsfunktion?
        return parameter_liste[i].f_read();
      else
        return "Function not implemented.";
    }
      i++;
    }

  return "Parameter not found.";
}

// Einheit fuer Parameter nachschlagen
const char * const get_u( const char *name )
{
  int i=0;

  while( parameter_liste[i].p_name ) // Ende der Liste
    {
      if ( strcmp( name, parameter_liste[i].p_name ) == 0 ) // Parametername gefunden?
    return parameter_liste[i].p_einheit;

      i++;
    }

  return "Parameter not found.";
}

// Parameterwert setzen
const char * const set_v( const char *name, const char *value )
{
  int i=0;

  while( parameter_liste[i].p_name ) // Ende der Liste ?
    {
      if ( strcmp( name, parameter_liste[i].p_name ) == 0 ) // Parametername gefunden?
    {
      if ( parameter_liste[i].f_write ) // Gibts eine Schreibfunktion?
          return parameter_liste[i].f_write( value );
      else
        return "Read only!";
    }
      i++;
    }

  return "Unknown Parameter!";
}

// Raw address reading

const char * const get_r( const char *address, const char *len ) {
    const int max_length = 64; // max is actually 55

    static char cache[256];
    uint8_t vitomem[max_length];
    int length = strlen(len) == 0 ? 1 : atoi(len);
    int addr = strtol ( address, NULL, 16);
    int i;

    if ( length > max_length )
      {
        fprintf( stderr, "Warning, Max Experimental Size exceeded! Defaulting to %d!\n", max_length );
        length = max_length;
      }


    if ( addr > 0xFFFF )
      {
//        length = 2;

        //check if address is correct
        if (addr>0x10003)
            return "NULL";

        if ( vito_dtr_read( addr - 0x10000, length, vitomem ) < 0 )
            return "NULL";
      }
    else
      {
        if ( vito_read( addr, length, vitomem ) < 0 )
            return "NULL";
      }

    cache[0]='\0';
    for (i = 0; i < length; i++) {
            char tmp[10];
            sprintf( tmp, "%u;", vitomem[i] );
            strcat(cache, tmp);
        }
    return cache;
}

// Raw address writing

const char * const set_r( const char *address, const char *value ) {
    uint8_t content[16];
    int addr = strtol ( address, NULL, 16);
    char *values;
    char *array[16];
    int i=0;
    int j;

    //check if address is correct
    if (addr<=0||addr>0x10003)
        return "Wrong address!";

    //split values into an array of strings
    values = strdup(value);
    array[i] = strtok(values,";");
    while((i<16)&&(array[i]!=NULL)) {
        i++;
        if (i<16)
            array[i] = strtok(NULL,";");
    }


    //check if there is a value to send
    if (i==0)
        return "No value to be sent";

    //convert strings to int
    for (j=0; j<i; j++)
        content[j]=atoi(array[j]);

    free(values);


    if ( addr > 0xFFFF )
      {
        if ( vito_dtr_write(addr - 0x10000, i, content) < 0 )
            return "VitoDTR communication Error";
        else
            return "OK";
      }
    else
      {
//        example array for fixing internal registers corruption with checksum
//        uint8_t fixdata[10] = { 16, 8, 30, 4, 5, 4, 30, 20, 0, 0 };
//        if ( vito_write(addr, 10, fixdata) < 0 )

        //send and pray!
        if ( vito_write(addr, i, content) < 0 )
            return "Vitodens communication Error";
        else
            return "OK";
      }

}