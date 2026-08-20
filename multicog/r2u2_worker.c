#include "simpletools.h"

#include "r2u2_worker.h"
#include "pressure_stable_spec.h"


#define PRESSURE_SIGNAL   0
#define HAS_PREV_SIGNAL   1
#define PRESSURE_FORMULA  0

#define R2U2_TEST_PIN    22
#define R2U2_STACK_SIZE  384
#define VERDICT_LED      21

volatile unsigned int r2u2_pressure = 0;
volatile unsigned char r2u2_has_prev = 0;

volatile unsigned char r2u2_stable = 1;
volatile unsigned char r2u2_verdict_seen = 0;

volatile unsigned char r2u2_sample_ready = 0;
volatile unsigned char r2u2_result_ready = 0;
int r2u2_lock = -1;

static r2u2_status_t r2u2_status;
/* Private R2U2 memory. */
static unsigned int
    r2u2_stack[R2U2_STACK_SIZE];

static r2u2_monitor_t monitor;


static r2u2_bz_instruction_t
    bz_instructions[R2U2_MAX_BZ_INSTRUCTIONS];

static r2u2_mltl_instruction_t
    mltl_instructions[R2U2_MAX_TL_INSTRUCTIONS];

static r2u2_value_t
    signal_values[R2U2_MAX_SIGNALS];

static r2u2_value_t
    booleanizer_values[R2U2_MAX_BZ_INSTRUCTIONS];

static r2u2_bool
    atomic_values[R2U2_MAX_ATOMICS];

static r2u2_scq_control_block_t
    queue_controls[R2U2_MAX_TL_INSTRUCTIONS];

static r2u2_verdict queue_memory[
    R2U2_MAX_QUEUE_SLOTS +
    (R2U2_MAX_TEMPORAL_OPERATORS *
     R2U2_TEMPORAL_METADATA_SIZE)
];


/* R2U2 verdict callback. */
static r2u2_status_t verdict_callback(
    r2u2_mltl_instruction_t instruction,
    r2u2_verdict *verdict)
{
    if (instruction.op2_value ==
        PRESSURE_FORMULA)
    {
        r2u2_stable =
            get_verdict_truth(*verdict) ? 1 : 0;

        r2u2_verdict_seen = 1;
    }

    return R2U2_OK;
}


/* Connect the R2U2 monitor to its memory arrays. */
static void setup_r2u2_monitor(void)
{
    monitor.time_stamp = 0;

    monitor.progress =
        R2U2_MONITOR_PROGRESS_FIRST_LOOP;

    monitor.bz_program_count.curr_program_count = 0;
    monitor.bz_program_count.max_program_count = 0;
    monitor.bz_instruction_tbl = bz_instructions;

    monitor.mltl_program_count.curr_program_count = 0;
    monitor.mltl_program_count.max_program_count = 0;
    monitor.mltl_instruction_tbl = mltl_instructions;

    monitor.out_file = 0;
    monitor.out_func = verdict_callback;

    monitor.signal_vector = signal_values;
    monitor.value_buffer = booleanizer_values;
    monitor.atomic_buffer = atomic_values;

    monitor.queue_arena.control_blocks =
        queue_controls;

    monitor.queue_arena.queue_mem =
        queue_memory;
}


/* Function executed continuously by the additional cog. */
static void r2u2_cog(void *par)
{
      unsigned int pressure;  
       /*unsigned int step_cycles;    */
      unsigned int step_start;
      unsigned char has_prev;
      r2u2_status_t step_status;
      
   
    (void)par;
     low(VERDICT_LED);
     low(R2U2_TEST_PIN);


    while (1)/*not in code without cog */
    {
        while (!r2u2_sample_ready)
        {
            /* Wait without a pause. */
        }
        while (lockset(r2u2_lock)){}
        pressure  = r2u2_pressure;
        has_prev = r2u2_has_prev; 

        r2u2_sample_ready = 0;
        lockclr(r2u2_lock);
        
        r2u2_verdict_seen = 0;
        r2u2_stable = 1;
/*this code waits for the pressure from barometer */
        r2u2_load_int_signal(
            &monitor,
            PRESSURE_SIGNAL,
            (r2u2_int)pressure);

        r2u2_load_bool_signal(
            &monitor,
            HAS_PREV_SIGNAL,
            r2u2_has_prev ? 1 : 0);
        high(R2U2_TEST_PIN);
        /*cnt to check clock timing */
         /* step_start = CNT;  */
        step_status =
            r2u2_step(&monitor);
        /*step_cycles =
    CNT - step_start;  */
     low(R2U2_TEST_PIN);
        
        /*
         * P21:
         * high = stable/pass
         * low  = violation/fail
         */
        if ((step_status == R2U2_OK) && r2u2_verdict_seen && r2u2_stable)
        
        {
            high(VERDICT_LED);
        }
        else
        {
            low(VERDICT_LED);
        }
       

    }
}


/* Initialize R2U2 and launch its worker cog. */
int r2u2_worker_start(void)
{
    
    int cog_result;
    

    setup_r2u2_monitor();

    r2u2_status =
        r2u2_load_specification(
            pressure_stable_bin,
            &monitor);

    if (r2u2_status != R2U2_OK)
    {
        return 0;
    }

    r2u2_sample_ready = 0;
    r2u2_result_ready = 1;
    r2u2_lock = locknew();
    if(r2u2_lock<0){
      return 0;
    }      
    cog_result = cogstart(
        r2u2_cog,
        NULL,
        r2u2_stack,
        sizeof(r2u2_stack));
  if(cog_result < 0 )
        {
  lockret(r2u2_lock);
  r2u2_lock = -1;
  return 0;
}
  return 1;  
  }