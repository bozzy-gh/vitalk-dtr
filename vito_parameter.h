// Struktur zur Verwaltung der Parameter
struct s_parameter {
  const char * const p_name;         // Parameter Kurzname
  const char * const p_description;  // Beschreibung des Parameters
  const char * const p_einheit;      // Einheit (String)
  const int p_class;                 // Parameterklasse, siehe #define
  const char * const (* const f_read) (void); // Funktion zum lesen aus der Vitodens
  const char * const (* const f_write) (const char *value_str); // Funktion zum Schreiben in die Vitodens
};

// Parameter liste exportieren:
extern const struct s_parameter parameter_liste[];

/*
 * HEIZKREIS  -> HEATING CIRCUIT
 * BRENNER    -> BURNER
 * WARMWASSER -> HOT WATER
 * ALLGEMEIN  -> GENERAL
 */

// Parameterklassen:
#define P_ALL        0
#define P_ERRORS     1
#define P_GENERAL    2
#define P_BOILER     3
#define P_HOTWATER   4
#define P_HEATING    5
#define P_BURNER     6
#define P_HYDRAULIC  7

#define FROMBCD(x)      (((x) >> 4) * 10 + ((x) & 0xf))
#define TOBCD(x)        (((x) / 10 * 16) + ((x) % 10))

// Prototypen:
const char * const get_v( const char *name );
const char * const get_u( const char *name );
const char * const get_r( const char *address, const char *len );
const char * const set_v( const char *name, const char *value );
const char * const set_r( const char *address, const char *value );
