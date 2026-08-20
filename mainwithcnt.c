#include "simpletools.h"
#include <stdio.h>

#include "barometer.h"
#include "r2u2_worker.h"
#include "sdcard.h"


#define TIMING_PIN       0

#define ERROR_LED        16
#define VERDICT_LED      21
#define R2U2_TEST_PIN    22
#define RUNNING_LED      23


/*
 * Static timing variables avoid losing their values
 * across long function calls in the CMM build.
 */
static volatile unsigned int loop_start_cnt = 0;
static volatile unsigned int direct_start_cnt = 0;


/* Report an error and stop the main cog. */
static void inline_error(
    const char *location,
    int result)
{
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


int main(void)
{
    unsigned int pressure;
    unsigned int temperature;
    unsigned int sample_number = 0;

    unsigned char has_prev = 0;
    unsigned char pressure_stable;

    int start_result;
    int save_result;

    r2u2_status_t status;

    unsigned int loop_end_cnt;
    unsigned int total_cycles;

    unsigned int direct_r2u2_cycles;
    unsigned int step_cycles;
    unsigned int direct_overhead_cycles;


    /* Set initial pin states. */
    low(TIMING_PIN);
    low(ERROR_LED);
    low(VERDICT_LED);
    low(R2U2_TEST_PIN);
    low(RUNNING_LED);


    /* Start the SD card. */
    start_result = sdcard_start();

    if (start_result != 0)
    {
        inline_error(
            "SDSTART",
            start_result);
    }


    /* Start the barometer. */
    barometer_start();


    /*
     * Initialize R2U2 and load its specification.
     *
     * In this one-cog version, this function must
     * not call cogstart().
     */
    start_result = r2u2_worker_start();

    if (start_result == 0)
    {
        inline_error(
            "R2START",
            (int)r2u2_status);
    }


    while (1)
    {
        /*
         * Begin complete active-loop timing.
         */
        loop_start_cnt = CNT;

        high(TIMING_PIN);


        /* Read the barometer. */
        pressure =
            barometer_read_pressure();

        temperature =
            barometer_read_temprature();


        /*
         * Provide the input values used by
         * the direct R2U2 function.
         */
        r2u2_pressure = pressure;
        r2u2_has_prev = has_prev;


        /*
         * Time the complete direct R2U2 operation.
         *
         * r2u2_cog() must execute one R2U2 step
         * and return. It must not contain an
         * infinite worker loop in this version.
         */
        direct_start_cnt = CNT;

        r2u2_cog();

        direct_r2u2_cycles =
            CNT - direct_start_cnt;


        /*
         * r2u2_step_cycles is measured inside
         * r2u2_cog(), immediately around
         * r2u2_step().
         */
        step_cycles =
            r2u2_step_cycles;


        /*
         * This is signal loading, result handling,
         * function-call and timing-pin overhead
         * outside r2u2_step().
         */
        if (direct_r2u2_cycles >= step_cycles)
        {
            direct_overhead_cycles =
                direct_r2u2_cycles - step_cycles;
        }
        else
        {
            direct_overhead_cycles = 0;
        }


        /* Copy the R2U2 result. */
        status = r2u2_status;
        pressure_stable = r2u2_stable;


        if (status != R2U2_OK)
        {
            inline_error(
                "R2STEP",
                (int)status);
        }


        if (!r2u2_verdict_seen)
        {
            inline_error(
                "NOVRDICT",
                0);
        }


        /* Display the R2U2 verdict on P21. */
        if (pressure_stable)
        {
            high(VERDICT_LED);
        }
        else
        {
            low(VERDICT_LED);
        }


        /* Save the sample to the SD card. */
        save_result = sdcard_save_sample(
            sample_number,
            pressure,
            temperature,
            has_prev,
            pressure_stable,
            (int)status);

        if (save_result != 0)
        {
            inline_error(
                "SDSAVE",
                save_result);
        }


        toggle(RUNNING_LED);


        /*
         * End complete active-loop timing.
         */
        low(TIMING_PIN);

        loop_end_cnt = CNT;

        total_cycles =
            loop_end_cnt - loop_start_cnt;


        /*
         * Print after capturing total_cycles so
         * serial output is not included in timing.
         */
        print(
            "ONECOG sample=%u "
            "total_cycles=%u total_us=%u "
            "direct_cycles=%u direct_us=%u "
            "step_cycles=%u step_us=%u "
            "overhead_cycles=%u overhead_us=%u\n",
            sample_number,
            total_cycles,
            total_cycles / (CLKFREQ / 1000000),
            direct_r2u2_cycles,
            direct_r2u2_cycles /
                (CLKFREQ / 1000000),
            step_cycles,
            step_cycles / (CLKFREQ / 1000000),
            direct_overhead_cycles,
            direct_overhead_cycles /
                (CLKFREQ / 1000000));


        sample_number++;
        has_prev = 1;

        pause(1000);
    }
}