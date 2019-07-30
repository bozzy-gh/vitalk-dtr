#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "vito_parameter.h"
#include "vito_io.h"
#include "telnet.h"


short unsigned int vitalkport;

extern fd_set master_fds; /* file descriptor list for select() */
extern fd_set read_fds;   /* select() call result */

static int fd_listener; /* listening socket descriptor */
static struct sockaddr_in serveraddr; /* server address */
static struct sockaddr_in clientaddr; /* client address */

// Telnet callable commands:
const char *commands[] =
{  "help",              // 0
   "h",                 // 1
   "get",               // 2
   "g",                 // 3
   "set",               // 4
   "s",                 // 5
   "list",              // 6
   "gc",                // 7
   "gvu",               // 8
   "frame_debug",       // 9
   "rg",                //10
   "rs",                //11
   "exit",              //12
   "quit",              //13
   "q",                 //14
   "\0"
};

// Help text:
static void print_help( int fd )
{
  dprintf(fd,
    "Short Help Text:\n"
    "  h, help                 - Show this Help text\n"
    "  frame_debug <on/off>    - Set state of Frame Debugging\n"
    "  list [class]            - Show Parameter List\n"
    "  g, get <p_name>         - Query Parameter\n"
    "  s, set <p_name> <value> - Set Parameter\n"
    "  gvu <p_name>            - Get Value of Parameter with Unit\n"
    "  gc <class>              - Query a Class of Parameters\n"
    "     Parameter Classes:\n"
    "       P_ALL        0\n"
    "       P_ERRORS     1\n"
    "       P_GENERAL    2\n"
    "       P_BOILER     3\n"
    "       P_HOTWATER   4\n"
    "       P_HEATING    5\n"
    "       P_BURNER     6\n"
    "       P_HYDRAULIC  7\n"
    "       P_SOLAR      8\n"
    "  rg <address> [<bytes>]  - Query RAW Memory Address\n"
    "  rs <address> <value(s)> - Set RAW Memory Address\n"
    "  q, quit, exit           - Quit session\n"
    "\n$"
  );
}

// Close telnet socket:
static void quit_session( int fd )
{
  dprintf(fd, "Quitting...\n");
  close(fd);
  FD_CLR(fd, &master_fds);
}

// Parameterklasse abfragen:
static void get_class( int fd, int p_class )
{
  int i=0;
  while( parameter_liste[i].p_name )
    {
      if ( ( p_class == 0 && parameter_liste[i].p_class > 1 ) || // ganze liste ohne Errors
       p_class == parameter_liste[i].p_class )               // spezifizierte Klasse
    //dprintf(fd, "%02u: %20s: %s %s\n",
    dprintf(fd, "%20s: %s %s;\n",
        parameter_liste[i].p_name,
        get_v(parameter_liste[i].p_name),
        get_u(parameter_liste[i].p_name)
           );
      i++;
    }
  dprintf(fd, "\n$");
}

// Liste aller Parameter
static void print_listall( int fd, int p_class )
{
  int i=0;
  while( parameter_liste[i].p_name )
    {
      if ( p_class == 0 || p_class == parameter_liste[i].p_class )
          dprintf(fd, "%02u: %20s: %s\n",
              parameter_liste[i].p_class,
              parameter_liste[i].p_name,
              parameter_liste[i].p_description
                 );
      i++;
    }
  dprintf(fd, "\n$");
}

// Init Telnet Socket:
void telnet_init( void )
{
  FD_ZERO(&master_fds);

  /* create socket */
  if ((fd_listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
  {
    fprintf( stderr, "Error creating Telnet Socket: %s\n", strerror(errno));
    exit(1);
  }

  // Set REUSEADDR option
  // This allows the server to be restarted just after being closed:
  const int optVal = 1;
  if ( setsockopt(fd_listener, SOL_SOCKET, SO_REUSEADDR, (void*) &optVal, sizeof(int)) == -1 )
  {
    fprintf( stderr, "Error setting Telnet Socket Options: %s\n", strerror(errno));
    exit(1);
  }

  /* bind */
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = INADDR_ANY;
  serveraddr.sin_port = htons(vitalkport);
  memset(&(serveraddr.sin_zero), '\0', sizeof(serveraddr.sin_zero) );
  if (bind(fd_listener, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
  {
    fprintf( stderr, "Error bind Telnet Socket: %s\n", strerror(errno));
    exit(1);
  }

  /* listen */
  if (listen(fd_listener, 10) == -1) // "10" = listen backlog
  {
    fprintf( stderr, "Error listen on Telnet Socket: %s\n", strerror(errno));
    exit(1);
  }
  else
  {
      fprintf( stderr, "Now listening to telnet Port %d\n", vitalkport );
  }

  /* add the listener to the master set */
  FD_SET(fd_listener, &master_fds);
}

// The following function is called periodically by the select() activity.
// It must by the select () return given descriptor amount be handed over:
// ---Es muss die von select() zurueckgegebene Descriptormenge uebergeben werden:
void telnet_task( void )
{
  int i;
  int fd_new;
  static char buffers[MAX_DESCRIPTOR + 1][TELNET_BUFFER_SIZE];
  static int buf_ptr[MAX_DESCRIPTOR + 1];
  int result;
  int cnum;
  int cfnd;

  // Traverse over all possible Filedescriptors:
  for (i = 0; i <= MAX_DESCRIPTOR; i++)
  {
    if ( FD_ISSET(i, &read_fds) )
    { // New connection?
      if ( i == fd_listener )
      { // New connection!
        socklen_t addrlen = sizeof(clientaddr);
        if ( ( fd_new = accept(fd_listener, (struct sockaddr *)&clientaddr, &addrlen)) == -1)
          fprintf(stderr, "Error accepting Connection!\n");
        else
        {
          if ( fd_new > MAX_DESCRIPTOR )
          {
            fprintf(stderr, "Max Number of Telnet Connections reached!\n");
            close( fd_new );
          }
          else
          {
//            fprintf(stdout, "New connection from %s on socket %d\n",
//              inet_ntoa(clientaddr.sin_addr), fd_new);
            FD_SET(fd_new, &master_fds); /* add to master set */
            buffers[i][0]='\0';
            buf_ptr[i] = 0;
            dprintf( fd_new, "Welcome at viTalk, the Vitodens telnet Interface. (%u)\n$", fd_new);
          }
        }
      }
      else
      { // Data from a client:
        result = recv(i, &buffers[i][buf_ptr[i]], 1, MSG_DONTWAIT); // Not effective, but simple
        if ( result <= 0 )
        {
          if ( result < 0 )
            fprintf( stderr, "recv() error: %s\n", strerror(errno));
          // Connection was terminated:
//          fprintf(stdout, "Socket %d hung up.\n", i);
          close(i);
          FD_CLR(i, &master_fds);
        }
        else
        { // Receive actual data:
          buf_ptr[i]++;
          buffers[i][buf_ptr[i]]='\0';
          if (( strchr(buffers[i], '\n' )) || // Newline in the buffer?
              ( buf_ptr[i] > TELNET_BUFFER_SIZE - 4 )) // Or buffer "pretty full"?
          { // Then we will process it:
            char command[24] = "";
            char value1[24] = "";
            char value2[36] = "";

            sscanf( buffers[i], "%20s %20s %32s %*s\n", command, value1, value2 );

            buf_ptr[i] = 0; // For the next buffer filling

            // Scan for known command:
            if ( strlen(command) != 0 )
            {
              cfnd = 0;
              for ( cnum=0; commands[cnum][0]; cnum++ )
              {
                if ( strcmp( commands[cnum], command ) == 0 )
                {
                  cfnd = 1;
                  switch ( cnum )
                  {
                    case 0:
                    case 1:
                      print_help(i);
                      break;
                    case 2:
                    case 3:
                      // get parameter value by name
                      dprintf( i, "%s\n$", get_v(value1) );
                      break;
                    case 4:
                    case 5:
                      // set parameter value by name
                      dprintf( i, "%s\n$", set_v(value1, value2));
                      break;
                    case 6:
                      // list all parameters (class: name)
                      print_listall( i, atoi(value1));
                      break;
                    case 7:
                      // get class parameters values
                      get_class( i, atoi(value1) );
                      break;
                    case 8:
                      // get parameter value by name with unit
                      dprintf( i, "%s %s\n$", get_v(value1), get_u(value1));
                      break;
                    case 9:
                      // set frame debugging on/off
                      if ( strcmp( value1, "on" ) == 0 )
                        frame_debug = 1;
                      else
                        frame_debug = 0;
                      break;
                    case 10:
                      // get raw memory value(s) by address
                      dprintf( i, "%s\n$", get_r(value1, value2));
                      break;
                    case 11:
                      // set raw memory value(s) by address
                      dprintf( i, "%s\n$", set_r(value1, value2));
                      break;
                    case 12:
                    case 13:
                    case 14:
                      // quit session
                      quit_session(i);
                  }
                }
              }
              if (cfnd == 0)
                dprintf( i, "Unknown command!\n$");
            }
            else
              dprintf( i, "$");
          }
        }
      }
    }
  }
}
