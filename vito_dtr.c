//#define _POSIX_C_SOURCE 199309L

#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <linux/i2c-dev.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "gpio.h"

#define ms2ns_MULTIPLIER 1000000        // 1 millisecond = 1,000,000 nanoseconds
#define adsaddress 0x48                 // indirizzo I2C ADS1115
#define adsparamsR 0b0000001111100011   // parametri configurazione ADS1115 (LSB+MSB!!!)
#define adsparams0 0b0000001111010011   // parametri configurazione ADS1115 (LSB+MSB!!!)
#define adsparams1 0b0000001111110011   // parametri configurazione ADS1115 (LSB+MSB!!!)
#define adsparams2 0b0000001111000011   // parametri configurazione ADS1115 (LSB+MSB!!!)
#define adspgafactor 2.0                // moltiplicatore PGA impostato su parametri ADS1115
#define ads4bitdecimals 16.0            // divisore precisione decimale (2^4bit) ADS1115
#define adsconvtime 150                 // ms ritardo per attesa conversione ADS1115
#define i2cdevice "/dev/i2c-1"          // device BUS I2C da utilizzare
#define T0 25.0                         // T0 dell'NTC
#define R0 10.0                         // R0 dell'NTC
#define Bf 3850.0                       // fattore B dell'NTC
#define CK 273.15                       // differenza tra Celsius e Kelvin


static uint8_t valvestate = 255;
static uint8_t valvemutex = 0;


// Exit and print error code
static void exit_on_error (const char *s)
{
    perror(s);
    abort();
}

int dtr_read( int location, uint8_t *vitomem )
{
    int fd;

    uint16_t adsparams;

    int32_t SMBUS_word;
    uint16_t RSENS;
    uint16_t RREF;
    double_t VSENS;
    double_t VREF;
    double_t TSENS;

    struct timespec ConvSleepValue = { 0, adsconvtime * ms2ns_MULTIPLIER };

    switch (location)
    {
        case 0:
            adsparams = adsparams0;
            break;
        case 1:
            adsparams = adsparams1;
            break;
        default:
            adsparams = adsparams2;
    }

    // Open I2C device
    if ((fd = open(i2cdevice, O_RDWR)) < 0) exit_on_error ("Can't open I2C device");

    // Set I2C slave address
    if (ioctl(fd, I2C_SLAVE, adsaddress) < 0) exit_on_error ("Can't talk to slave"); ////?

    // Start sensor ADC conversion
    if (i2c_smbus_write_word_data(fd, 1, adsparams) < 0) exit_on_error ("Failed to write to the i2c bus");
    // Sleep for ADC conversion
    nanosleep(&ConvSleepValue, NULL);
    // Read sensor ADC value
    if ((SMBUS_word = i2c_smbus_read_word_data(fd, 0)) < 0) exit_on_error ("Failed to read from the i2c bus");
    RSENS = ntohs(SMBUS_word);

    // Start reference ADC conversion
    if (i2c_smbus_write_word_data(fd, 1, adsparamsR) < 0) exit_on_error ("Failed to write to the i2c bus [2]");
    // Sleep for ADC conversion
    nanosleep(&ConvSleepValue, NULL);
    // Read reference ADC value
    if ((SMBUS_word = i2c_smbus_read_word_data(fd, 0)) < 0) exit_on_error ("Failed to read from the i2c bus [2]");
    RREF = ntohs(SMBUS_word);

    VSENS = RSENS * adspgafactor / ads4bitdecimals;
    VREF  = RREF  * adspgafactor / ads4bitdecimals;

    TSENS = (1.0 / ((1.0 / (T0 + CK)) + (log((20.0 - (11.0 * VSENS / VREF)) / R0) / Bf))) - CK;

//    printf("Data read from sensor %d: %.1f\n", location, TSENS);

    close(fd);

    uint8_t result[2];

    div_t output;
    output = div((int) (TSENS * 10), 256);

    result[0] = (uint8_t) (output.rem);
    result[1] = (uint8_t) (output.quot);

    // Zu lesenden Speicherbereich aus der empfangenen Payload
    // kopieren:
    memcpy( vitomem, &result[0], 2 );

    return 0;
}

int valve_open()
{
    /*
     * Set GPIO directions
     */
    GPIODirection(17, GPIO_OUT);
    GPIODirection(27, GPIO_OUT);
    GPIODirection(22, GPIO_OUT);

    if ((-1 == GPIOWrite(17, 0)) || (-1 == GPIOWrite(27, 0)))
        return(-1);

    GPIOWrite(22, 1);

    if (-1 == GPIOWrite(17, 1))
        return(-1);

    sleep(10);

    if (-1 == GPIOWrite(17, 0))
        return(-1);

    return 0;
}

void *valve_open_t(void *threadid)
{
    if (valvemutex == 0)
    {
        valvemutex = 1;

        /*
         * Set GPIO directions
         */
        GPIODirection(17, GPIO_OUT);
        GPIODirection(27, GPIO_OUT);
        GPIODirection(22, GPIO_OUT);

        if (!((-1 == GPIOWrite(17, 0)) || (-1 == GPIOWrite(27, 0))))
            if (!(-1 == GPIOWrite(17, 1)))
            {
                valvestate = 1;
                GPIOWrite(22, 1);
                sleep(10);
                GPIOWrite(17, 0);
            }
        valvemutex = 0;
    }

//    pthread_exit(NULL);
    return NULL;
}

int valve_close()
{
    /*
     * Set GPIO directions
     */
    GPIODirection(17, GPIO_OUT);
    GPIODirection(27, GPIO_OUT);
    GPIODirection(22, GPIO_OUT);

    if ((-1 == GPIOWrite(17, 0)) || (-1 == GPIOWrite(27, 0)))
        return(-1);

    if (-1 == GPIOWrite(27, 1))
        return(-1);

    sleep(8);

    if (-1 == GPIOWrite(27, 0))
        return(-1);

    GPIOWrite(22, 0);

    return 0;
}

void *valve_close_t(void *threadid)
{
    if (valvemutex == 0)
    {
        valvemutex = 1;

        /*
         * Set GPIO directions
         */
        GPIODirection(17, GPIO_OUT);
        GPIODirection(27, GPIO_OUT);
        GPIODirection(22, GPIO_OUT);

        if (!((-1 == GPIOWrite(17, 0)) || (-1 == GPIOWrite(27, 0))))
            if (!(-1 == GPIOWrite(27, 1)))
            {
                valvestate = 0;
                GPIOWrite(22, 0);
                sleep(8);
                GPIOWrite(27, 0);
            }
        valvemutex = 0;
    }

//    pthread_exit(NULL);
    return NULL;
}

int valve_cycle()
{
    if (valve_open() < 0)
        return(-1);

    sleep(1);

    if (valve_close() < 0)
        return(-1);

    return(0);
}

//void VSSave()
//{
//    char buffer[2];
//    ssize_t bytes_written;
//    int fd;
//
//    if ((fd = open("/var/lib/vitalk/valvestate", O_WRONLY | O_CREAT | O_TRUNC)) < 0)
//        return;
//
//    bytes_written = snprintf(buffer, 2, "%d", valvestate);
//    write(fd, buffer, bytes_written);
//    close(fd);
//}

//void VSLoad()
//{
//    char value_str[2];
//    int fd;
//
//    fd = open("/var/lib/vitalk/valvestate", O_RDONLY);
//    if (-1 == fd)
//        return;
//
//    if (-1 == read(fd, value_str, 2))
//        return;
//
//    close(fd);
//
//    valvestate = atoi(value_str);
//}

int vito_dtr_read( int location, int size, uint8_t *vitomem )
{
    if (location <= 2)
        return dtr_read(location, vitomem);

//// gestire size di ritorno (deve essere come richiesto)
//    if (valvestate == 255)
//        VSLoad();
    if (location == 3)
    {
        memcpy( vitomem, &valvestate, 1 ); //// meglio subordinarlo a location
        return 0;
    }

    return(-1);
}

int vito_dtr_write( int location, int size, uint8_t *vitomem )
{
     pthread_t thd;
     int trc;

    if (location == 3)
    {
        /*
         * Set GPIO directions
         */
//        GPIODirection(17, GPIO_OUT);
//        GPIODirection(27, GPIO_OUT);
//        GPIODirection(22, GPIO_OUT);

        /*
         * Write GPIO value
         */
        switch (*vitomem)
        {
            case 0:
                //if (valve_close() < 0)
                //    return(-1);
                trc = pthread_create(&thd, NULL, valve_close_t, NULL);
                if (trc)
                    return(-1);
                //pthread_exit(NULL);
                //valvestate = 0;
                break;
            case 1:
                //if (valve_open() < 0)
                //    return(-1);
                trc = pthread_create(&thd, NULL, valve_open_t, NULL);
                if (trc)
                    return(-1);
                //pthread_exit(NULL);
                //valvestate = 1;
                break;
            case 2:
                if (valve_cycle() < 0)
                    return(-1);
                valvestate = 0;
                break;
            default:
                break;
        }
//        if (valvestate != 255)
//            VSSave();

        return 0;
    }

    return(-1);
}
