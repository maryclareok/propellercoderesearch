#include "simpletools.h"
#include <stdio.h>

#include "barometer.h"
#include "r2u2_worker.h"
#include "sdcard.h"


/* Timing pin for oscilloscope. */
#define TIMING_PIN       0


/* LED pins. */
#define ERROR_LED        16
#define RUNNING_LED      23


/*
 * Report the error, turn on P16,
 * and stop the main cog.
 *
 * sdcard_log_error() uses fputs() to send
 * the location and result to stdout before
 * attempting to create ERROR.TXT.
 */
static void inline_error(
    const char *location,
    int result)
{
    print(
        "ERROR: %s, result=%d\n",
        location,
        result);

    sdcard_log_error(
        location,
        result);

    low(TIMING_PIN);
    high(ERROR_LED);

    while (1)
    {
        pause(1000);
    }
}

static volatile unsigned int loop_start_cnt = 0;
int main(void)
{
    unsigned int pressure;
    unsigned int temperature;
    unsigned int sample_number = 0;
    unsigned char has_prev = 0;

    int start_result;
    int save_result;

  

/*variables for timer for total code*/
    unsigned int total_cycles;


    unsigned int loop_end_cnt;
    /*
     *
     * Set initial pin states.
     */
    low(TIMING_PIN);
    low(ERROR_LED);
    low(RUNNING_LED);


    /*
     * Start the SD card once.
     *
     * sdcard_start():
     * 0 = success
     * 1 = mount failed
     * 2 = DATA.TXT open failed
     * 3 = header write failed
     * 4 = header flush failed
     */
    start_result = sdcard_start();

    if (start_result != 0)
    {
      
        inline_error(
            "SDSTART",
            start_result);
    }


    /*
     * Start the barometer.
     */
    barometer_start();


    /*
     * Start the R2U2 worker cog.
     *
     * r2u2_worker_start():
     * 1 = success
     * 0 = failure
     */
    start_result = r2u2_worker_start();

    if (start_result == 0)
    {
        inline_error(
            "R2START",
            0);
    }


    while (1)
    {
       

        /*
         * Active-code timing begins.
         */
        high(TIMING_PIN);
        /* counting total cycle*/
        loop_start_cnt = CNT;
         


        /*
         * Read the barometer.
         */
        pressure =
            barometer_read_pressure();
            /*
 * Measure the complete R2U2 request latency.
 */
         
            while(lockset(r2u2_lock)){}
                   


        r2u2_pressure = pressure;
        r2u2_has_prev = has_prev;
                /*
         * Publish the request after writing
         * the input values.
         */
        r2u2_sample_ready = 1;

        lockclr(r2u2_lock);

        temperature =
            barometer_read_temprature();
    




  /*
         * Save this sample.
         *
         * sdcard_save_sample():
         * 0 = success
         * 1 = data file missing
         * 2 = write failed
         * 3 = flush failed
         */
        save_result = sdcard_save_sample(
            sample_number,
            pressure,
            temperature);

        if (save_result != 0)
        {
            inline_error(
                "SDSAVE",
                save_result);
        }


        toggle(RUNNING_LED);



        /*
         * Active-code timing ends.
         */
        low(TIMING_PIN);
        loop_end_cnt = CNT;

        total_cycles =
        loop_end_cnt - loop_start_cnt;
        print(
            "TIMING_V3 sample=%u "
            "total_cycles=%u total_us=%u\n",

            sample_number,

            total_cycles,
            total_cycles /
                (CLKFREQ / 1000000));


        sample_number++;
        has_prev = 1;

        pause(1000);
    }
}