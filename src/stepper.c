/*
  stepper.c - stepper motor driver: executes motion plans using stepper motors
  Part of Grbl

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "grbl.h"

#ifdef STM32F103C8
typedef int bool;
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "misc.h"
void TIM_Configuration(TIM_TypeDef* TIMER, u16 Period, u16 Prescaler, u8 PP);
#endif


// Some useful constants.
#define DT_SEGMENT (1.0f/(ACCELERATION_TICKS_PER_SECOND*60.0f)) // min/segment
#define REQ_MM_INCREMENT_SCALAR 1.25f
#define RAMP_ACCEL 0
#define RAMP_CRUISE 1
#define RAMP_DECEL 2
#define RAMP_DECEL_OVERRIDE 3

#define PREP_FLAG_RECALCULATE bit(0)
#define PREP_FLAG_HOLD_PARTIAL_BLOCK bit(1)
#define PREP_FLAG_PARKING bit(2)
#define PREP_FLAG_DECEL_OVERRIDE bit(3)
const PORTPINDEF step_pin_mask[N_AXIS] =
{
	1 << X_STEP_BIT,
	1 << Y_STEP_BIT,
	1 << Z_STEP_BIT,
// --- YSV 22-06-2018
	#if defined(AA_AXIS) || defined(AB_AXIS) || defined(ABC_AXIS)
	1 << A_STEP_BIT,
	#endif
	#if defined(AB_AXIS) || defined(ABC_AXIS)
	1 << B_STEP_BIT,
	#endif
	#ifdef ABC_AXIS
	1 << C_STEP_BIT,
	#endif
// ---
};
const PORTPINDEF direction_pin_mask[N_AXIS] =
{
	1 << X_DIRECTION_BIT,
	1 << Y_DIRECTION_BIT,
	1 << Z_DIRECTION_BIT,
// --- YSV 22-06-2018
	#if defined(AA_AXIS) || defined(AB_AXIS) || defined(ABC_AXIS)
	1 << A_DIRECTION_BIT,
	#endif
	#if defined(AB_AXIS) || defined(ABC_AXIS)
	1 << B_DIRECTION_BIT,
	#endif
	#ifdef ABC_AXIS
	1 << C_DIRECTION_BIT,
	#endif
// ---
};
const PORTPINDEF limit_pin_mask[N_AXIS] =
{
	1 << X_LIMIT_BIT,
	1 << Y_LIMIT_BIT,
	1 << Z_LIMIT_BIT,
// --- YSV 22-06-2018
	#if defined(AA_AXIS) || defined(AB_AXIS) || defined(ABC_AXIS)
	1 << A_LIMIT_BIT,
	#endif
	#if defined(AB_AXIS) || defined(ABC_AXIS)
	1 << B_LIMIT_BIT,
	#endif
	#ifdef ABC_AXIS
	1 << C_LIMIT_BIT,
	#endif
// ---
};

// Define Adaptive Multi-Axis Step-Smoothing(AMASS) levels and cutoff frequencies. The highest level
// frequency bin starts at 0Hz and ends at its cutoff frequency. The next lower level frequency bin
// starts at the next higher cutoff frequency, and so on. The cutoff frequencies for each level must
// be considered carefully against how much it over-drives the stepper ISR, the accuracy of the 16-bit
// timer, and the CPU overhead. Level 0 (no AMASS, normal operation) frequency bin starts at the
// Level 1 cutoff frequency and up to as fast as the CPU allows (over 30kHz in limited testing).
// NOTE: AMASS cutoff frequency multiplied by ISR overdrive factor must not exceed maximum step frequency.
// NOTE: Current settings are set to overdrive the ISR to no more than 16kHz, balancing CPU overhead
// and timer accuracy.  Do not alter these settings unless you know what you are doing.
#ifdef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
	#define MAX_AMASS_LEVEL 3
	// AMASS_LEVEL0: Normal operation. No AMASS. No upper cutoff frequency. Starts at LEVEL1 cutoff frequency.
	#define AMASS_LEVEL1 (F_CPU/8000) // Over-drives ISR (x2). Defined as F_CPU/(Cutoff frequency in Hz)
	#define AMASS_LEVEL2 (F_CPU/4000) // Over-drives ISR (x4)
	#define AMASS_LEVEL3 (F_CPU/2000) // Over-drives ISR (x8)

  #if MAX_AMASS_LEVEL <= 0
    error "AMASS must have 1 or more levels to operate correctly."
  #endif
#endif
#ifdef WIN32
#include <process.h> 
unsigned char PORTB = 0;
unsigned char DDRD = 0;
unsigned char DDRB = 0;
unsigned char PORTD = 0;
LARGE_INTEGER Win32Frequency;
LONGLONG nTimer1Out = 0;
LONGLONG nTimer0Out = 0;
#endif


// Stores the planner block Bresenham algorithm execution data for the segments in the segment
// buffer. Normally, this buffer is partially in-use, but, for the worst case scenario, it will
// never exceed the number of accessible stepper buffer segments (SEGMENT_BUFFER_SIZE-1).
// NOTE: This data is copied from the prepped planner blocks so that the planner blocks may be
// discarded when entirely consumed and completed by the segment buffer. Also, AMASS alters this
// data for its own use.
typedef struct {
  uint32_t steps[N_AXIS];
  uint32_t step_event_count;
  PORTPINDEF direction_bits;
  #ifdef VARIABLE_SPINDLE
    uint8_t is_pwm_rate_adjusted; // Tracks motions that require constant laser power/rate
  #endif
} st_block_t;
static st_block_t st_block_buffer[SEGMENT_BUFFER_SIZE-1];

// Primary stepper segment ring buffer. Contains small, short line segments for the stepper
// algorithm to execute, which are "checked-out" incrementally from the first block in the
// planner buffer. Once "checked-out", the steps in the segments buffer cannot be modified by
// the planner, where the remaining planner block steps still can.
typedef struct {
  uint16_t n_step;           // Number of step events to be executed for this segment
  uint16_t cycles_per_tick;  // Step distance traveled per ISR tick, aka step rate.
  uint8_t  st_block_index;   // Stepper block data index. Uses this information to execute this segment.
  #ifdef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
    uint8_t amass_level;    // Indicates AMASS level for the ISR to execute this segment
  #else
    uint8_t prescaler;      // Without AMASS, a prescaler is required to adjust for slow timing.
  #endif
  #ifdef VARIABLE_SPINDLE
    uint8_t spindle_pwm;
  #endif
} segment_t;
static segment_t segment_buffer[SEGMENT_BUFFER_SIZE];

// Stepper ISR data struct. Contains the running data for the main stepper ISR.
typedef struct {
  // Used by the bresenham line algorithm
  uint32_t counter_x,        // Counter variables for the bresenham line tracer
           counter_y,
           counter_z;
// --- YSV 22-06-2018
  #if defined(AA_AXIS) || defined(AB_AXIS) || defined(ABC_AXIS)
  uint32_t counter_a;
  #endif
  #if defined(AB_AXIS) || defined(ABC_AXIS)
  uint32_t counter_b;
  #endif
  #ifdef ABC_AXIS
  uint32_t counter_c;
  #endif
// ---
  #ifdef STEP_PULSE_DELAY
    #ifdef STM32F103C8
 	uint16_t step_bits;  // Stores out_bits output to complete the step pulse delay
	#else
 	uint8_t step_bits;  // Stores out_bits output to complete the step pulse delay
	#endif
  #endif

  uint8_t execute_step;     // Flags step execution for each interrupt.
#ifndef WIN32
  uint16_t step_pulse_time;  // Step pulse reset time after step rise
#else
  LONGLONG step_pulse_time;
#endif
  PORTPINDEF step_outbits;         // The next stepping-bits to be output
  PORTPINDEF dir_outbits;
  #ifdef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
    uint32_t steps[N_AXIS];
  #endif

  uint16_t step_count;       // Steps remaining in line segment motion
  uint8_t exec_block_index; // Tracks the current st_block index. Change indicates new block.
  st_block_t *exec_block;   // Pointer to the block data for the segment being executed
  segment_t *exec_segment;  // Pointer to the segment being executed
} stepper_t;
static stepper_t st;

// Step segment ring buffer indices
static volatile uint8_t segment_buffer_tail;
static uint8_t segment_buffer_head;
static uint8_t segment_next_head;

// Step and direction port invert masks.
static PORTPINDEF step_port_invert_mask;
static PORTPINDEF dir_port_invert_mask;

// Used to avoid ISR nesting of the "Stepper Driver Interrupt". Should never occur though.
static volatile uint8_t busy;

// Pointers for the step segment being prepped from the planner buffer. Accessed only by the
// main program. Pointers may be planning segments or planner blocks ahead of what being executed.
static plan_block_t *pl_block;     // Pointer to the planner block being prepped
static st_block_t *st_prep_block;  // Pointer to the stepper block data being prepped

// Segment preparation data struct. Contains all the necessary information to compute new segments
// based on the current executing planner block.
typedef struct {
  uint8_t st_block_index;  // Index of stepper common data block being prepped
  uint8_t recalculate_flag;

  float dt_remainder;
  float steps_remaining;
  float step_per_mm;
  float req_mm_increment;

  #ifdef PARKING_ENABLE
    uint8_t last_st_block_index;
    float last_steps_remaining;
    float last_step_per_mm;
    float last_dt_remainder;
  #endif

  uint8_t ramp_type;      // Current segment ramp state
  float mm_complete;      // End of velocity profile from end of current planner block in (mm).
                          // NOTE: This value must coincide with a step(no mantissa) when converted.
  float current_speed;    // Current speed at the end of the segment buffer (mm/min)
  float maximum_speed;    // Maximum speed of executing block. Not always nominal speed. (mm/min)
  float exit_speed;       // Exit speed of executing block (mm/min)
  float accelerate_until; // Acceleration ramp end measured from end of block (mm)
  float decelerate_after; // Deceleration ramp start measured from end of block (mm)

  #ifdef VARIABLE_SPINDLE
    float inv_rate;    // Used by PWM laser mode to speed up segment calculations.
    uint8_t current_spindle_pwm;
  #endif
} st_prep_t;
static st_prep_t prep;


/*    BLOCK VELOCITY PROFILE DEFINITION
          __________________________
         /|                        |\     _________________         ^
        / |                        | \   /|               |\        |
       /  |                        |  \ / |               | \       s
      /   |                        |   |  |               |  \      p
     /    |                        |   |  |               |   \     e
    +-----+------------------------+---+--+---------------+----+    e
    |               BLOCK 1            ^      BLOCK 2          |    d
                                       |
                  time ----->      EXAMPLE: Block 2 entry speed is at max junction velocity

  The planner block buffer is planned assuming constant acceleration velocity profiles and are
  continuously joined at block junctions as shown above. However, the planner only actively computes
  the block entry speeds for an optimal velocity plan, but does not compute the block internal
  velocity profiles. These velocity profiles are computed ad-hoc as they are executed by the
  stepper algorithm and consists of only 7 possible types of profiles: cruise-only, cruise-
  deceleration, acceleration-cruise, acceleration-only, deceleration-only, full-trapezoid, and
  triangle(no cruise).

                                        maximum_speed (< nominal_speed) ->  +
                    +--------+ <- maximum_speed (= nominal_speed)          /|\
                   /          \                                           / | \
 current_speed -> +            \                                         /  |  + <- exit_speed
                  |             + <- exit_speed                         /   |  |
                  +-------------+                     current_speed -> +----+--+
                   time -->  ^  ^                                           ^  ^
                             |  |                                           |  |
                decelerate_after(in mm)                             decelerate_after(in mm)
                    ^           ^                                           ^  ^
                    |           |                                           |  |
                accelerate_until(in mm)                             accelerate_until(in mm)

  The step segment buffer computes the executing block velocity profile and tracks the critical
  parameters for the stepper algorithm to accurately trace the profile. These critical parameters
  are shown and defined in the above illustration.
*/


// Stepper state initialization. Cycle should only start if the st.cycle_start flag is
// enabled. Startup init and limits call this function but shouldn't start the cycle.
void st_wake_up()
{
#ifdef  USE_RESET_BTN_AS_ESTOP
	if(system_control_get_state() & CONTROL_PIN_INDEX_RESET) { // if reset button is pressed
		mc_reset();
		return;
	}
#endif
  // Enable stepper drivers.
  if (bit_istrue(settings.flags,BITFLAG_INVERT_ST_ENABLE)) 
  {
	  SetStepperDisableBit();
  }
  else 
  { 
	  ResetStepperDisableBit();
  }

  // Delay before drivers are enabled
  #ifdef STP_DRIVERS_ENABLE_DELAY
  delay_ms(STP_DRIVERS_ENABLE_DELAY);
  #endif

  // Initialize stepper output bits to ensure first ISR call does not step.
  st.step_outbits = step_port_invert_mask;


#ifdef AVRTARGET
  // Initialize step pulse timing from settings. Here to ensure updating after re-writing.
  #ifdef STEP_PULSE_DELAY
    // Set total step pulse time after direction pin set. Ad hoc computation from oscilloscope.
    st.step_pulse_time = -(((settings.pulse_microseconds+STEP_PULSE_DELAY-2)*TICKS_PER_MICROSECOND) >> 3);
    // Set delay between direction pin write and step command.
    OCR0A = -(((settings.pulse_microseconds)*TICKS_PER_MICROSECOND) >> 3);
  #else // Normal operation
    // Set step pulse time. Ad hoc computation from oscilloscope. Uses two's complement.
  st.step_pulse_time = -(((settings.pulse_microseconds - 2)*TICKS_PER_MICROSECOND) >> 3);
  #endif
#elif defined (WIN32)
  st.step_pulse_time = (settings.pulse_microseconds)*TICKS_PER_MICROSECOND;
#elif defined(STM32F103C8)
	#ifdef STEP_PULSE_DELAY
    TIM3->SR = ~TIM_SR_CC1IF; // clear CC1IF flag
	TIM3->CCR1 = (STEP_PULSE_DELAY - 1) * TICKS_PER_MICROSECOND + 1; //+1 for STEP_PULSE_DELAY=1, -1 to be closer to exact delay
    st.step_pulse_time = (settings.pulse_microseconds + (STEP_PULSE_DELAY - 1)) * TICKS_PER_MICROSECOND + 1;
	#else // Normal operation
    // A value, determined with a logic analyzer is substracted to get close to the desired pulse time
  	st.step_pulse_time = (settings.pulse_microseconds)*TICKS_PER_MICROSECOND - 58;
	#endif
#endif


  // Enable Stepper Driver Interrupt
#ifdef AVRTARGET
  TIMSK1 |= (1<<OCIE1A);
#endif
#ifdef WIN32
  nTimer1Out = 1;
#endif
#if defined (STM32F103C8)
  TIM3->ARR = st.step_pulse_time; // don't subtract 1!
  TIM3->EGR = TIM_PSCReloadMode_Immediate;
  TIM3->SR = ~TIM_SR_UIF;

  TIM2->ARR = st.exec_segment->cycles_per_tick - 1;
  /* Set the Autoreload value */
#ifndef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING        
  TIM2->PSC = st.exec_segment->prescaler;
#endif
  TIM2->EGR = TIM_PSCReloadMode_Immediate;
  TIM2->CR1 |= TIM_CR1_CEN;
#endif
}


// Stepper shutdown
void st_go_idle()
{
  // Disable Stepper Driver Interrupt. Allow Stepper Port Reset Interrupt to finish, if active.
#ifdef AVRTARGET
  TIMSK1 &= ~(1<<OCIE1A); // Disable Timer1 interrupt
  TCCR1B = (TCCR1B & ~((1<<CS12) | (1<<CS11))) | (1<<CS10); // Reset clock to no prescaling.
#endif
#ifdef WIN32
  nTimer1Out = 0;
#endif
#ifdef STM32F103C8
  TIM2->CR1 &= ~TIM_CR1_CEN;
  TIM2->CNT = 0;
#endif

  busy = false;

  // Set stepper driver idle state, disabled or enabled, depending on settings and circumstances.
  bool pin_state = false; // Keep enabled.
  if (((settings.stepper_idle_lock_time != 0xff) || sys_rt_exec_alarm || sys.state == STATE_SLEEP) && sys.state != STATE_HOMING) {
    // Force stepper dwell to lock axes for a defined amount of time to ensure the axes come to a complete
    // stop and not drift from residual inertial forces at the end of the last movement.
    delay_ms(settings.stepper_idle_lock_time);
    pin_state = true; // Override. Disable steppers.
  }
  if (bit_istrue(settings.flags,BITFLAG_INVERT_ST_ENABLE)) { pin_state = !pin_state; } // Apply pin invert.
  if (pin_state) 
  { 
	  SetStepperDisableBit();
  }
  else 
  { 
	  ResetStepperDisableBit();
  }
}


/* "The Stepper Driver Interrupt" - This timer interrupt is the workhorse of Grbl. Grbl employs
   the venerable Bresenham line algorithm to manage and exactly synchronize multi-axis moves.
   Unlike the popular DDA algorithm, the Bresenham algorithm is not susceptible to numerical
   round-off errors and only requires fast integer counters, meaning low computational overhead
   and maximizing the Arduino's capabilities. However, the downside of the Bresenham algorithm
   is, for certain multi-axis motions, the non-dominant axes may suffer from un-smooth step
   pulse trains, or aliasing, which can lead to strange audible noises or shaking. This is
   particularly noticeable or may cause motion issues at low step frequencies (0-5kHz), but
   is usually not a physical problem at higher frequencies, although audible.
     To improve Bresenham multi-axis performance, Grbl uses what we call an Adaptive Multi-Axis
   Step Smoothing (AMASS) algorithm, which does what the name implies. At lower step frequencies,
   AMASS artificially increases the Bresenham resolution without effecting the algorithm's
   innate exactness. AMASS adapts its resolution levels automatically depending on the step
   frequency to be executed, meaning that for even lower step frequencies the step smoothing
   level increases. Algorithmically, AMASS is acheived by a simple bit-shifting of the Bresenham
   step count for each AMASS level. For example, for a Level 1 step smoothing, we bit shift
   the Bresenham step event count, effectively multiplying it by 2, while the axis step counts
   remain the same, and then double the stepper ISR frequency. In effect, we are allowing the
   non-dominant Bresenham axes step in the intermediate ISR tick, while the dominant axis is
   stepping every two ISR ticks, rather than every ISR tick in the traditional sense. At AMASS
   Level 2, we simply bit-shift again, so the non-dominant Bresenham axes can step within any
   of the four ISR ticks, the dominant axis steps every four ISR ticks, and quadruple the
   stepper ISR frequency. And so on. This, in effect, virtually eliminates multi-axis aliasing
   issues with the Bresenham algorithm and does not significantly alter Grbl's performance, but
   in fact, more efficiently utilizes unused CPU cycles overall throughout all configurations.
     AMASS retains the Bresenham algorithm exactness by requiring that it always executes a full
   Bresenham step, regardless of AMASS Level. Meaning that for an AMASS Level 2, all four
   intermediate steps must be completed such that baseline Bresenham (Level 0) count is always
   retained. Similarly, AMASS Level 3 means all eight intermediate steps must be executed.
   Although the AMASS Levels are in reality arbitrary, where the baseline Bresenham counts can
   be multiplied by any integer value, multiplication by powers of two are simply used to ease
   CPU overhead with bitshift integer operations.
     This interrupt is simple and dumb by design. All the computational heavy-lifting, as in
   determining accelerations, is performed elsewhere. This interrupt pops pre-computed segments,
   defined as constant velocity over n number of steps, from the step segment buffer and then
   executes them by pulsing the stepper pins appropriately via the Bresenham algorithm. This
   ISR is supported by The Stepper Port Reset Interrupt which it uses to reset the stepper port
   after each pulse. The bresenham line tracer algorithm controls all stepper outputs
   simultaneously with these two interrupts.

   NOTE: This interrupt must be as efficient as possible and complete before the next ISR tick,
   which for Grbl must be less than 33.3usec (@30kHz ISR rate). Oscilloscope measured time in
   ISR is 5usec typical and 25usec maximum, well below requirement.
   NOTE: This ISR expects at least one step to be executed per segment.
*/
// TODO: Replace direct updating of the int32 position counters in the ISR somehow. Perhaps use smaller
// int8 variables and update position counters only when a segment completes. This can get complicated
// with probing and homing cycles that require true real-time positions.
#ifdef STM32F103C8
void TIM2_IRQHandler(void)
#endif
#ifdef AVRTARGET
ISR(TIMER1_COMPA_vect)
#endif
#ifdef WIN32
void Timer1Proc()
#endif
{
#ifdef STM32F103C8
	if ((TIM2->SR & TIM_SR_UIF) != 0)                  // check interrupt source
	{
		TIM2->SR = ~TIM_SR_UIF; // clear UIF flag
		TIM2->CNT = 0;
	}
	else
	{
		return;
	}
#endif

  if (busy) {return; } // The busy-flag is used to avoid reentering this interrupt
#ifdef AVRTARGET
  // Set the direction pins a couple of nanoseconds before we step the steppers
  DIRECTION_PORT = (DIRECTION_PORT & ~DIRECTION_MASK) | (st.dir_outbits & DIRECTION_MASK);
#endif
#ifdef STM32F103C8
  //GPIO_Write(DIRECTION_PORT, (GPIO_ReadOutputData(DIRECTION_PORT) & ~DIRECTION_MASK) | (st.dir_outbits & DIRECTION_MASK));
  //DIRECTION_PORT->ODR = ((DIRECTION_PORT->ODR & ~DIRECTION_MASK) | (st.dir_outbits & DIRECTION_MASK));
  DIRECTION_PORT->ODR = ((DIRECTION_PORT->ODR & ~DIRECTION_MASK) | ((~st.dir_outbits) & DIRECTION_MASK));//������
  TIM3->SR = ~( TIM_SR_UIF | TIM_SR_CC1IF);
#endif


#ifdef AVRTARGET
  // Then pulse the stepping pins
  #ifdef STEP_PULSE_DELAY
    st.step_bits = (STEP_PORT & ~STEP_MASK) | st.step_outbits; // Store out_bits to prevent overwriting.
  #else  // Normal operation
    STEP_PORT = (STEP_PORT & ~STEP_MASK) | st.step_outbits;
  #endif
#endif
#ifdef STM32F103C8
    // Then pulse the stepping pins
    #ifdef STEP_PULSE_DELAY
      st.step_bits = st.step_outbits; // Store out_bits to prevent overwriting.
    #else  // Normal operation
      STEP_PORT->ODR = ((STEP_PORT->ODR & ~STEP_MASK) | st.step_outbits);
      //GPIO_Write(STEP_PORT, (GPIO_ReadOutputData(STEP_PORT) & ~STEP_MASK) | st.step_outbits);
    #endif
#endif


  // Enable step pulse reset timer so that The Stepper Port Reset Interrupt can reset the signal after
  // exactly settings.pulse_microseconds microseconds, independent of the main Timer1 prescaler.
#ifdef AVRTARGET
  TCNT0 = st.step_pulse_time; // Reload Timer0 counter
  TCCR0B = (1<<CS01); // Begin Timer0. Full speed, 1/8 prescaler
#endif
#ifdef WIN32
  nTimer0Out = st.step_pulse_time;
#endif
#ifdef STM32F103C8
  TIM3->CR1 |= TIM_CR1_CEN;
#endif
  busy = true;
#ifdef AVRTARGET
  sei(); // Re-enable interrupts to allow Stepper Port Reset Interrupt to fire on-time.
         // NOTE: The remaining code in this ISR will finish before returning to main program.
#endif

  // If there is no step segment, attempt to pop one from the stepper buffer
  if (st.exec_segment == NULL) {
    // Anything in the buffer? If so, load and initialize next step segment.
    if (segment_buffer_head != segment_buffer_tail) {
      // Initialize new step segment and load number of steps to execute
      st.exec_segment = &segment_buffer[segment_buffer_tail];

      #ifndef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
        // With AMASS is disabled, set timer prescaler for segments with slow step frequencies (< 250Hz).
#ifdef AVRTARGET
        TCCR1B = (TCCR1B & ~(0x07<<CS10)) | (st.exec_segment->prescaler<<CS10);
#endif
      #endif

      // Initialize step segment timing per step and load number of steps to execute.
#ifdef AVRTARGET
      OCR1A = st.exec_segment->cycles_per_tick;
#endif
#ifdef WIN32
#ifndef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
	  nTimer1Out = st.exec_segment->cycles_per_tick * (st.exec_segment->prescaler + 1);
#else
	  nTimer1Out = st.exec_segment->cycles_per_tick;
#endif
#endif
#ifdef STM32F103C8
	  TIM2->ARR = st.exec_segment->cycles_per_tick - 1;
	  /* Set the Autoreload value */
#ifndef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING        
	  TIM2->PSC = st.exec_segment->prescaler;
#endif
#endif
      st.step_count = st.exec_segment->n_step; // NOTE: Can sometimes be zero when moving slow.
      // If the new segment starts a new planner block, initialize stepper variables and counters.
      // NOTE: When the segment data index changes, this indicates a new planner block.
      if ( st.exec_block_index != st.exec_segment->st_block_index ) {
        st.exec_block_index = st.exec_segment->st_block_index;
        st.exec_block = &st_block_buffer[st.exec_block_index];

        // Initialize Bresenham line and distance counters
// -- YSV 22-06-2018
		#ifdef AA_AXIS
        st.counter_x = st.counter_y = st.counter_z = st.counter_a = (st.exec_block->step_event_count >> 1);
		#elif defined AB_AXIS
        st.counter_x = st.counter_y = st.counter_z = st.counter_a = st.counter_b = (st.exec_block->step_event_count >> 1);
		#elif defined ABC_AXIS
        st.counter_x = st.counter_y = st.counter_z = st.counter_a = st.counter_b = st.counter_c = (st.exec_block->step_event_count >> 1);
		#else
        st.counter_x = st.counter_y = st.counter_z = (st.exec_block->step_event_count >> 1);
		#endif
// ---
      }
      st.dir_outbits = st.exec_block->direction_bits ^ dir_port_invert_mask;

      #ifdef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
        // With AMASS enabled, adjust Bresenham axis increment counters according to AMASS level.
        st.steps[X_AXIS] = st.exec_block->steps[X_AXIS] >> st.exec_segment->amass_level;
        st.steps[Y_AXIS] = st.exec_block->steps[Y_AXIS] >> st.exec_segment->amass_level;
        st.steps[Z_AXIS] = st.exec_block->steps[Z_AXIS] >> st.exec_segment->amass_level;
// --- YSV 22-06-2018
		#if defined(AA_AXIS) || defined(AB_AXIS) || defined(ABC_AXIS)
		st.steps[A_AXIS] = st.exec_block->steps[A_AXIS] >> st.exec_segment->amass_level;
		#endif
		#if defined(AB_AXIS) || defined(ABC_AXIS)
		st.steps[B_AXIS] = st.exec_block->steps[B_AXIS] >> st.exec_segment->amass_level;
		#endif
		#ifdef ABC_AXIS
		st.steps[C_AXIS] = st.exec_block->steps[C_AXIS] >> st.exec_segment->amass_level;
		#endif
// ---
      #endif

      #ifdef VARIABLE_SPINDLE
        // Set real-time spindle output as segment is loaded, just prior to the first step.
        spindle_set_speed(st.exec_segment->spindle_pwm);
      #endif

    } else {
      // Segment buffer empty. Shutdown.
      st_go_idle();
      // Ensure pwm is set properly upon completion of rate-controlled motion.
      #ifdef VARIABLE_SPINDLE
      if (st.exec_block->is_pwm_rate_adjusted) { spindle_set_speed(SPINDLE_PWM_OFF_VALUE); }
      #endif
      system_set_exec_state_flag(EXEC_CYCLE_STOP); // Flag main program for cycle end
      return; // Nothing to do but exit.
    }
  }


  // Check probing state.
  if (sys_probe_state == PROBE_ACTIVE) { probe_state_monitor(); }

  // Reset step out bits.
  st.step_outbits = 0;

  // Execute step displacement profile by Bresenham line algorithm
  #ifdef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
    st.counter_x += st.steps[X_AXIS];
  #else
    st.counter_x += st.exec_block->steps[X_AXIS];
  #endif
  if (st.counter_x > st.exec_block->step_event_count) {
    st.step_outbits |= (1<<X_STEP_BIT);
    st.counter_x -= st.exec_block->step_event_count;
    if (st.exec_block->direction_bits & (1<<X_DIRECTION_BIT)) { sys_position[X_AXIS]--; }
    else { sys_position[X_AXIS]++; }
  }
  #ifdef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
    st.counter_y += st.steps[Y_AXIS];
  #else
    st.counter_y += st.exec_block->steps[Y_AXIS];
  #endif
  if (st.counter_y > st.exec_block->step_event_count) {
    st.step_outbits |= (1<<Y_STEP_BIT);
    st.counter_y -= st.exec_block->step_event_count;
    if (st.exec_block->direction_bits & (1<<Y_DIRECTION_BIT)) { sys_position[Y_AXIS]--; }
    else { sys_position[Y_AXIS]++; }
  }
  #ifdef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
    st.counter_z += st.steps[Z_AXIS];
  #else
    st.counter_z += st.exec_block->steps[Z_AXIS];
  #endif
  if (st.counter_z > st.exec_block->step_event_count) {
    st.step_outbits |= (1<<Z_STEP_BIT);
    st.counter_z -= st.exec_block->step_event_count;
    if (st.exec_block->direction_bits & (1<<Z_DIRECTION_BIT)) { sys_position[Z_AXIS]--; }
    else { sys_position[Z_AXIS]++; }
  }
// --- YSV 22-06-2018
  #if defined(AA_AXIS) || defined(AB_AXIS) || defined(ABC_AXIS)
  #ifdef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
    st.counter_a += st.steps[A_AXIS];
  #else
    st.counter_a += st.exec_block->steps[A_AXIS];
  #endif
  if (st.counter_a > st.exec_block->step_event_count) {
    st.step_outbits |= (1<<A_STEP_BIT);
    st.counter_a -= st.exec_block->step_event_count;
    if (st.exec_block->direction_bits & (1<<A_DIRECTION_BIT)) { sys_position[A_AXIS]--; }
    else { sys_position[A_AXIS]++; }
  }
  #endif
  #if defined(AB_AXIS) || defined(ABC_AXIS)
  #ifdef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
    st.counter_b += st.steps[B_AXIS];
  #else
    st.counter_b += st.exec_block->steps[B_AXIS];
  #endif
  if (st.counter_b > st.exec_block->step_event_count) {
    st.step_outbits |= (1<<B_STEP_BIT);
    st.counter_b -= st.exec_block->step_event_count;
    if (st.exec_block->direction_bits & (1<<B_DIRECTION_BIT)) { sys_position[B_AXIS]--; }
    else { sys_position[B_AXIS]++; }
  }
  #endif
  #ifdef ABC_AXIS
  #ifdef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
    st.counter_c += st.steps[C_AXIS];
  #else
    st.counter_c += st.exec_block->steps[C_AXIS];
  #endif
  if (st.counter_c > st.exec_block->step_event_count) {
    st.step_outbits |= (1<<C_STEP_BIT);
    st.counter_c -= st.exec_block->step_event_count;
    if (st.exec_block->direction_bits & (1<<C_DIRECTION_BIT)) { sys_position[C_AXIS]--; }
    else { sys_position[C_AXIS]++; }
  }
  #endif
// ---
  // During a homing cycle, lock out and prevent desired axes from moving.
  if (sys.state == STATE_HOMING) { st.step_outbits &= sys.homing_axis_lock; }

  st.step_count--; // Decrement step events count
  if (st.step_count == 0) {
    // Segment is complete. Discard current segment and advance segment indexing.
    st.exec_segment = NULL;
#ifndef WIN32
	uint8_t segment_tail_next = segment_buffer_tail + 1;
	if (segment_tail_next == SEGMENT_BUFFER_SIZE)
		segment_tail_next = 0;
	segment_buffer_tail = segment_tail_next;
#else
    if ( ++segment_buffer_tail == SEGMENT_BUFFER_SIZE) 
	{ 
		segment_buffer_tail = 0; 
	}
#endif
  }

  st.step_outbits ^= step_port_invert_mask;  // Apply step port invert mask
  busy = false;
}


/* The Stepper Port Reset Interrupt: Timer0 OVF interrupt handles the falling edge of the step
   pulse. This should always trigger before the next Timer1 COMPA interrupt and independently
   finish, if Timer1 is disabled after completing a move.
   NOTE: Interrupt collisions between the serial and stepper interrupts can cause delays by
   a few microseconds, if they execute right before one another. Not a big deal, but can
   cause issues at high step rates if another high frequency asynchronous interrupt is
   added to Grbl.
*/
// This interrupt is enabled by ISR_TIMER1_COMPAREA when it sets the motor port bits to execute
// a step. This ISR resets the motor port after a short period (settings.pulse_microseconds)
// completing one step cycle.
#ifdef STM32F103C8
void TIM3_IRQHandler(void)
#endif
#ifdef AVRTARGET
ISR(TIMER0_OVF_vect)
#endif
#ifdef WIN32
void Timer0Proc()
#endif
{
#ifdef STM32F103C8
#ifdef STEP_PULSE_DELAY
	if ((TIM3->SR & TIM_SR_CC1IF) != 0) // check interrupt source
	{
		STEP_PORT->ODR = ((STEP_PORT->ODR & ~STEP_MASK) | st.step_bits); // Begin step pulse.
		TIM3->SR = ~TIM_SR_CC1IF; // clear CC1IF flag
	} else
#endif
	{
		STEP_PORT->ODR = ((STEP_PORT->ODR & ~STEP_MASK) | (step_port_invert_mask & STEP_MASK));
		TIM3->CR1 &= ~TIM_CR1_CEN;
		TIM3->SR = ~(TIM_SR_UIF | TIM_SR_CC1IF); // clear UIF flag
		TIM3->CNT = 0;
	}
#endif

#ifdef AVRTARGET
  // Reset stepping pins (leave the direction pins)
  STEP_PORT = (STEP_PORT & ~STEP_MASK) | (step_port_invert_mask & STEP_MASK);
  TCCR0B = 0; // Disable Timer0 to prevent re-entering this interrupt when it's not needed.
#endif
#ifdef WIN32
  nTimer0Out = 0;
#endif
}
#ifdef AVRTARGET
#ifdef STEP_PULSE_DELAY
  // This interrupt is used only when STEP_PULSE_DELAY is enabled. Here, the step pulse is
  // initiated after the STEP_PULSE_DELAY time period has elapsed. The ISR TIMER2_OVF interrupt
  // will then trigger after the appropriate settings.pulse_microseconds, as in normal operation.
  // The new timing between direction, step pulse, and step complete events are setup in the
  // st_wake_up() routine.
  ISR(TIMER0_COMPA_vect)
  {
    STEP_PORT = st.step_bits; // Begin step pulse.
  }
#endif
#endif




// Generates the step and direction port invert masks used in the Stepper Interrupt Driver.
void st_generate_step_dir_invert_masks()
{
  uint8_t idx;
  step_port_invert_mask = 0;
  dir_port_invert_mask = 0;
  for (idx=0; idx<N_AXIS; idx++) {
    if (bit_istrue(settings.step_invert_mask,bit(idx))) { step_port_invert_mask |= step_pin_mask[idx]; }
    if (bit_istrue(settings.dir_invert_mask,bit(idx))) { dir_port_invert_mask |= direction_pin_mask[idx]; }
  }
}


// Reset and clear stepper subsystem variables
void st_reset()
{
  // Initialize stepper driver idle state.
  st_go_idle();
  // Initialize stepper algorithm variables.
  memset(&prep, 0, sizeof(st_prep_t));
  memset(&st, 0, sizeof(stepper_t));
  st.exec_segment = NULL;
  pl_block = NULL;  // Planner block pointer used by segment buffer
  segment_buffer_tail = 0;
  segment_buffer_head = 0; // empty = tail
  segment_next_head = 1;
  busy = false;

  st_generate_step_dir_invert_masks();
  st.dir_outbits = dir_port_invert_mask; // Initialize direction bits to default.

  // Initialize step and direction port pins.
#ifdef AVRTARGET
  STEP_PORT = (STEP_PORT & ~STEP_MASK) | step_port_invert_mask;
  DIRECTION_PORT = (DIRECTION_PORT & ~DIRECTION_MASK) | dir_port_invert_mask;
#endif
#ifdef STM32F103C8
  while(TIM3->CR1 & TIM_CR1_CEN); // wait for end of tim3 work to prevent cutoff last step pulse
//#ifdef STEP_PULSE_DELAY
//	TIM3->DIER &= ~TIM_DIER_CC1IE; //compare interrupt disable
//#endif
  TIM3->SR = ~(TIM_SR_UIF | TIM_SR_CC1IF); // clear UIF and CC1IF flags
  STEP_PORT->ODR = ((STEP_PORT->ODR & ~STEP_MASK) | (step_port_invert_mask & STEP_MASK));
  DIRECTION_PORT->ODR = ((DIRECTION_PORT->ODR & ~DIRECTION_MASK) | (dir_port_invert_mask & DIRECTION_MASK));
  //GPIO_Write(STEP_PORT, (GPIO_ReadOutputData(STEP_PORT) & ~STEP_MASK) | (step_port_invert_mask & STEP_MASK));
  //GPIO_Write(DIRECTION_PORT, (GPIO_ReadOutputData(DIRECTION_PORT) & ~DIRECTION_MASK) | (dir_port_invert_mask & DIRECTION_MASK));
//#ifdef STEP_PULSE_DELAY
//	TIM3->DIER |= TIM_DIER_CC1IE; //compare interrupt enable
//#endif
#endif
}

#ifdef WIN32
void Timer1Thread(void *pVoid)
{
	LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;

	for (;;)
	{
		while (nTimer1Out == 0)
			Sleep(0);
		QueryPerformanceCounter(&StartingTime);
		do
		{
			QueryPerformanceCounter(&EndingTime);
			ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
		} while (ElapsedMicroseconds.QuadPart < nTimer1Out);
		Timer1Proc();
	}
}

void Timer0Thread(void *pVoid)
{
	LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;

	for (;;)
	{
		while (nTimer0Out == 0)
			Sleep(0);
		QueryPerformanceCounter(&StartingTime);
		do
		{
			QueryPerformanceCounter(&EndingTime);
			ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
		} while (ElapsedMicroseconds.QuadPart < nTimer0Out);
		Timer0Proc();
	}
}

#endif

// Initialize and start the stepper motor subsystem
void stepper_init()
{
  // Configure step and direction interface pins
#ifdef STM32F103C8
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_STEPPERS_DISABLE_PORT, ENABLE);
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = STEPPERS_DISABLE_MASK;
	GPIO_Init(STEPPERS_DISABLE_PORT, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_STEP_PORT, ENABLE);
	GPIO_InitStructure.GPIO_Pin = STEP_MASK;
	GPIO_Init(STEP_PORT, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_DIRECTION_PORT, ENABLE);
	GPIO_InitStructure.GPIO_Pin = DIRECTION_MASK;
	GPIO_Init(DIRECTION_PORT, &GPIO_InitStructure);

	RCC->APB1ENR |= RCC_APB1Periph_TIM2;
	TIM_Configuration(TIM2, 1, 1, 1);
	RCC->APB1ENR |= RCC_APB1Periph_TIM3;
	TIM_Configuration(TIM3, 1, 1, 1);

	TIM2->CR1 &= ~TIM_CR1_CEN;
	TIM2->SR &= ~TIM_SR_UIF;
	TIM2->CNT = 0;

	TIM3->CR1 &= ~TIM_CR1_CEN;
	TIM3->SR &= ~(TIM_SR_UIF | TIM_SR_CC1IF);
	TIM3->CNT = 0;
	//
#ifdef STEP_PULSE_DELAY
	TIM3->DIER |= TIM_DIER_CC1IE; //compare interrupt enable
#endif

	NVIC_SetPriority(TIM3_IRQn, 0);
	NVIC_SetPriority(TIM2_IRQn, 1);
	NVIC_EnableIRQ(TIM3_IRQn);
	NVIC_EnableIRQ(TIM2_IRQn);
#endif
#ifdef AVRTARGET
  STEP_DDR |= STEP_MASK;
  STEPPERS_DISABLE_DDR |= 1<<STEPPERS_DISABLE_BIT;
  DIRECTION_DDR |= DIRECTION_MASK;

  // Configure Timer 1: Stepper Driver Interrupt
  TCCR1B &= ~(1<<WGM13); // waveform generation = 0100 = CTC
  TCCR1B |=  (1<<WGM12);
  TCCR1A &= ~((1<<WGM11) | (1<<WGM10));
  TCCR1A &= ~((1<<COM1A1) | (1<<COM1A0) | (1<<COM1B1) | (1<<COM1B0)); // Disconnect OC1 output
  // TCCR1B = (TCCR1B & ~((1<<CS12) | (1<<CS11))) | (1<<CS10); // Set in st_go_idle().
  // TIMSK1 &= ~(1<<OCIE1A);  // Set in st_go_idle().

  // Configure Timer 0: Stepper Port Reset Interrupt
  TIMSK0 &= ~((1<<OCIE0B) | (1<<OCIE0A) | (1<<TOIE0)); // Disconnect OC0 outputs and OVF interrupt.
  TCCR0A = 0; // Normal operation
  TCCR0B = 0; // Disable Timer0 until needed
  TIMSK0 |= (1<<TOIE0); // Enable Timer0 overflow interrupt
  #ifdef STEP_PULSE_DELAY
    TIMSK0 |= (1<<OCIE0A); // Enable Timer0 Compare Match A interrupt
  #endif
#endif
#ifdef WIN32
	QueryPerformanceFrequency(&Win32Frequency);

	_beginthread(Timer1Thread, 0, NULL);
	_beginthread(Timer0Thread, 0, NULL);
#endif
}


// Called by planner_recalculate() when the executing block is updated by the new plan.
void st_update_plan_block_parameters()
{
  if (pl_block != NULL) { // Ignore if at start of a new block.
    prep.recalculate_flag |= PREP_FLAG_RECALCULATE;
    pl_block->entry_speed_sqr = prep.current_speed*prep.current_speed; // Update entry speed.
    pl_block = NULL; // Flag st_prep_segment() to load and check active velocity profile.
  }
}


// Increments the step segment buffer block data ring buffer.
static uint8_t st_next_block_index(uint8_t block_index)
{
  block_index++;
  if ( block_index == (SEGMENT_BUFFER_SIZE-1) ) { return(0); }
  return(block_index);
}


#ifdef PARKING_ENABLE
  // Changes the run state of the step segment buffer to execute the special parking motion.
  void st_parking_setup_buffer()
  {
    // Store step execution data of partially completed block, if necessary.
    if (prep.recalculate_flag & PREP_FLAG_HOLD_PARTIAL_BLOCK) {
      prep.last_st_block_index = prep.st_block_index;
      prep.last_steps_remaining = prep.steps_remaining;
      prep.last_dt_remainder = prep.dt_remainder;
      prep.last_step_per_mm = prep.step_per_mm;
    }
    // Set flags to execute a parking motion
    prep.recalculate_flag |= PREP_FLAG_PARKING;
    prep.recalculate_flag &= ~(PREP_FLAG_RECALCULATE);
    pl_block = NULL; // Always reset parking motion to reload new block.
  }


  // Restores the step segment buffer to the normal run state after a parking motion.
  void st_parking_restore_buffer()
  {
    // Restore step execution data and flags of partially completed block, if necessary.
    if (prep.recalculate_flag & PREP_FLAG_HOLD_PARTIAL_BLOCK) {
      st_prep_block = &st_block_buffer[prep.last_st_block_index];
      prep.st_block_index = prep.last_st_block_index;
      prep.steps_remaining = prep.last_steps_remaining;
      prep.dt_remainder = prep.last_dt_remainder;
      prep.step_per_mm = prep.last_step_per_mm;
      prep.recalculate_flag = (PREP_FLAG_HOLD_PARTIAL_BLOCK | PREP_FLAG_RECALCULATE);
      prep.req_mm_increment = REQ_MM_INCREMENT_SCALAR/prep.step_per_mm; // Recompute this value.
    } else {
      prep.recalculate_flag = false;
    }
    pl_block = NULL; // Set to reload next block.
  }
#endif


/* Prepares step segment buffer. Continuously called from main program.

   The segment buffer is an intermediary buffer interface between the execution of steps
   by the stepper algorithm and the velocity profiles generated by the planner. The stepper
   algorithm only executes steps within the segment buffer and is filled by the main program
   when steps are "checked-out" from the first block in the planner buffer. This keeps the
   step execution and planning optimization processes atomic and protected from each other.
   The number of steps "checked-out" from the planner buffer and the number of segments in
   the segment buffer is sized and computed such that no operation in the main program takes
   longer than the time it takes the stepper algorithm to empty it before refilling it.
   Currently, the segment buffer conservatively holds roughly up to 40-50 msec of steps.
   NOTE: Computation units are in steps, millimeters, and minutes.
*/
void st_prep_buffer()
{
  // Block step prep buffer, while in a suspend state and there is no suspend motion to execute.
  if (bit_istrue(sys.step_control,STEP_CONTROL_END_MOTION)) { return; }

  while (segment_buffer_tail != segment_next_head) { // Check if we need to fill the buffer.

    // Determine if we need to load a new planner block or if the block needs to be recomputed.
    if (pl_block == NULL) {

      // Query planner for a queued block
      if (sys.step_control & STEP_CONTROL_EXECUTE_SYS_MOTION) { pl_block = plan_get_system_motion_block(); }
      else { pl_block = plan_get_current_block(); }
      if (pl_block == NULL) { return; } // No planner blocks. Exit.

      // Check if we need to only recompute the velocity profile or load a new block.
      if (prep.recalculate_flag & PREP_FLAG_RECALCULATE) {

        #ifdef PARKING_ENABLE
          if (prep.recalculate_flag & PREP_FLAG_PARKING) { prep.recalculate_flag &= ~(PREP_FLAG_RECALCULATE); }
          else { prep.recalculate_flag = false; }
        #else
          prep.recalculate_flag = false;
        #endif

      } else {

        // Load the Bresenham stepping data for the block.
        prep.st_block_index = st_next_block_index(prep.st_block_index);

        // Prepare and copy Bresenham algorithm segment data from the new planner block, so that
        // when the segment buffer completes the planner block, it may be discarded when the
        // segment buffer finishes the prepped block, but the stepper ISR is still executing it.
        st_prep_block = &st_block_buffer[prep.st_block_index];
        st_prep_block->direction_bits = pl_block->direction_bits;
        uint8_t idx;
        #ifndef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
          for (idx=0; idx<N_AXIS; idx++) { st_prep_block->steps[idx] = (pl_block->steps[idx] << 1); }
          st_prep_block->step_event_count = (pl_block->step_event_count << 1);
        #else
          // With AMASS enabled, simply bit-shift multiply all Bresenham data by the max AMASS
          // level, such that we never divide beyond the original data anywhere in the algorithm.
          // If the original data is divided, we can lose a step from integer roundoff.
          for (idx=0; idx<N_AXIS; idx++) { st_prep_block->steps[idx] = pl_block->steps[idx] << MAX_AMASS_LEVEL; }
          st_prep_block->step_event_count = pl_block->step_event_count << MAX_AMASS_LEVEL;
        #endif

        // Initialize segment buffer data for generating the segments.
        prep.steps_remaining = (float)pl_block->step_event_count;
        prep.step_per_mm = prep.steps_remaining/pl_block->millimeters;
        prep.req_mm_increment = REQ_MM_INCREMENT_SCALAR/prep.step_per_mm;
        prep.dt_remainder = 0.0f; // Reset for new segment block

        if ((sys.step_control & STEP_CONTROL_EXECUTE_HOLD) || (prep.recalculate_flag & PREP_FLAG_DECEL_OVERRIDE)) {
          // New block loaded mid-hold. Override planner block entry speed to enforce deceleration.
          prep.current_speed = prep.exit_speed;
          pl_block->entry_speed_sqr = prep.exit_speed*prep.exit_speed;
          prep.recalculate_flag &= ~(PREP_FLAG_DECEL_OVERRIDE);
        } else {
          prep.current_speed = sqrtf(pl_block->entry_speed_sqr);
        }
#ifdef VARIABLE_SPINDLE
        // Setup laser mode variables. PWM rate adjusted motions will always complete a motion with the
        // spindle off. 
        st_prep_block->is_pwm_rate_adjusted = false;
        if (settings.flags & BITFLAG_LASER_MODE) {
          if (pl_block->condition & PL_COND_FLAG_SPINDLE_CCW) {
            // Pre-compute inverse programmed rate to speed up PWM updating per step segment.
            prep.inv_rate = 1.0f / pl_block->programmed_rate;
            st_prep_block->is_pwm_rate_adjusted = true;
          }
        }
#endif
      }

			/* ---------------------------------------------------------------------------------
			 Compute the velocity profile of a new planner block based on its entry and exit
			 speeds, or recompute the profile of a partially-completed planner block if the
			 planner has updated it. For a commanded forced-deceleration, such as from a feed
			 hold, override the planner velocities and decelerate to the target exit speed.
			*/
			prep.mm_complete = 0.0f; // Default velocity profile complete at 0.0mm from end of block.
			float inv_2_accel = 0.5f/pl_block->acceleration;
			if (sys.step_control & STEP_CONTROL_EXECUTE_HOLD) { // [Forced Deceleration to Zero Velocity]
				// Compute velocity profile parameters for a feed hold in-progress. This profile overrides
				// the planner block profile, enforcing a deceleration to zero speed.
				prep.ramp_type = RAMP_DECEL;
				// Compute decelerate distance relative to end of block.
				float decel_dist = pl_block->millimeters - inv_2_accel*pl_block->entry_speed_sqr;
				if (decel_dist < 0.0f) {
					// Deceleration through entire planner block. End of feed hold is not in this block.
					prep.exit_speed = sqrtf(pl_block->entry_speed_sqr-2*pl_block->acceleration*pl_block->millimeters);
				} else {
					prep.mm_complete = decel_dist; // End of feed hold.
					prep.exit_speed = 0.0f;
				}
			} else { // [Normal Operation]
				// Compute or recompute velocity profile parameters of the prepped planner block.
				prep.ramp_type = RAMP_ACCEL; // Initialize as acceleration ramp.
				prep.accelerate_until = pl_block->millimeters;

				float exit_speed_sqr;
				float nominal_speed;
        if (sys.step_control & STEP_CONTROL_EXECUTE_SYS_MOTION) {
          prep.exit_speed = exit_speed_sqr = 0.0f; // Enforce stop at end of system motion.
        } else {
          exit_speed_sqr = plan_get_exec_block_exit_speed_sqr();
          prep.exit_speed = sqrtf(exit_speed_sqr);
        }

        nominal_speed = plan_compute_profile_nominal_speed(pl_block);
				float nominal_speed_sqr = nominal_speed*nominal_speed;
				float intersect_distance =
								0.5f*(pl_block->millimeters+inv_2_accel*(pl_block->entry_speed_sqr-exit_speed_sqr));

        if (pl_block->entry_speed_sqr > nominal_speed_sqr) { // Only occurs during override reductions.
          prep.accelerate_until = pl_block->millimeters - inv_2_accel*(pl_block->entry_speed_sqr-nominal_speed_sqr);
          if (prep.accelerate_until <= 0.0f) { // Deceleration-only.
            prep.ramp_type = RAMP_DECEL;
            // prep.decelerate_after = pl_block->millimeters;
            // prep.maximum_speed = prep.current_speed;

            // Compute override block exit speed since it doesn't match the planner exit speed.
            prep.exit_speed = sqrtf(pl_block->entry_speed_sqr - 2*pl_block->acceleration*pl_block->millimeters);
            prep.recalculate_flag |= PREP_FLAG_DECEL_OVERRIDE; // Flag to load next block as deceleration override.

            // TODO: Determine correct handling of parameters in deceleration-only.
            // Can be tricky since entry speed will be current speed, as in feed holds.
            // Also, look into near-zero speed handling issues with this.

          } else {
            // Decelerate to cruise or cruise-decelerate types. Guaranteed to intersect updated plan.
            prep.decelerate_after = inv_2_accel*(nominal_speed_sqr-exit_speed_sqr); // Should always be >= 0.0 due to planner reinit.
						prep.maximum_speed = nominal_speed;
            prep.ramp_type = RAMP_DECEL_OVERRIDE;
          }
				} else if (intersect_distance > 0.0f) {
					if (intersect_distance < pl_block->millimeters) { // Either trapezoid or triangle types
						// NOTE: For acceleration-cruise and cruise-only types, following calculation will be 0.0.
						prep.decelerate_after = inv_2_accel*(nominal_speed_sqr-exit_speed_sqr);
						if (prep.decelerate_after < intersect_distance) { // Trapezoid type
							prep.maximum_speed = nominal_speed;
							if (pl_block->entry_speed_sqr == nominal_speed_sqr) {
								// Cruise-deceleration or cruise-only type.
								prep.ramp_type = RAMP_CRUISE;
							} else {
								// Full-trapezoid or acceleration-cruise types
								prep.accelerate_until -= inv_2_accel*(nominal_speed_sqr-pl_block->entry_speed_sqr);
							}
						} else { // Triangle type
							prep.accelerate_until = intersect_distance;
							prep.decelerate_after = intersect_distance;
							prep.maximum_speed = sqrtf(2.0f*pl_block->acceleration*intersect_distance+exit_speed_sqr);
						}
					} else { // Deceleration-only type
            prep.ramp_type = RAMP_DECEL;
            // prep.decelerate_after = pl_block->millimeters;
            // prep.maximum_speed = prep.current_speed;
					}
				} else { // Acceleration-only type
					prep.accelerate_until = 0.0f;
					// prep.decelerate_after = 0.0f;
					prep.maximum_speed = prep.exit_speed;
				}
			}
      
      #ifdef VARIABLE_SPINDLE
        bit_true(sys.step_control, STEP_CONTROL_UPDATE_SPINDLE_PWM); // Force update whenever updating block.
      #endif
    }
    
    // Initialize new segment
    segment_t *prep_segment = &segment_buffer[segment_buffer_head];

    // Set new segment to point to the current segment data block.
    prep_segment->st_block_index = prep.st_block_index;

    /*------------------------------------------------------------------------------------
        Compute the average velocity of this new segment by determining the total distance
      traveled over the segment time DT_SEGMENT. The following code first attempts to create
      a full segment based on the current ramp conditions. If the segment time is incomplete
      when terminating at a ramp state change, the code will continue to loop through the
      progressing ramp states to fill the remaining segment execution time. However, if
      an incomplete segment terminates at the end of the velocity profile, the segment is
      considered completed despite having a truncated execution time less than DT_SEGMENT.
        The velocity profile is always assumed to progress through the ramp sequence:
      acceleration ramp, cruising state, and deceleration ramp. Each ramp's travel distance
      may range from zero to the length of the block. Velocity profiles can end either at
      the end of planner block (typical) or mid-block at the end of a forced deceleration,
      such as from a feed hold.
    */
    float dt_max = DT_SEGMENT; // Maximum segment time
    float dt = 0.0f; // Initialize segment time
    float time_var = dt_max; // Time worker variable
    float mm_var; // mm-Distance worker variable
    float speed_var; // Speed worker variable
    float mm_remaining = pl_block->millimeters; // New segment distance from end of block.
    float minimum_mm = mm_remaining-prep.req_mm_increment; // Guarantee at least one step.
    if (minimum_mm < 0.0f) { minimum_mm = 0.0f; }

    do {
      switch (prep.ramp_type) {
        case RAMP_DECEL_OVERRIDE:
          speed_var = pl_block->acceleration*time_var;
					if (prep.current_speed-prep.maximum_speed <= speed_var) {
            // Cruise or cruise-deceleration types only for deceleration override.
						mm_remaining = prep.accelerate_until;
            time_var = 2.0f*(pl_block->millimeters-mm_remaining)/(prep.current_speed+prep.maximum_speed);
            prep.ramp_type = RAMP_CRUISE;
            prep.current_speed = prep.maximum_speed;
          } else { // Mid-deceleration override ramp.
						mm_remaining -= time_var*(prep.current_speed - 0.5f*speed_var);
            prep.current_speed -= speed_var;
          }
          break;
        case RAMP_ACCEL:
          // NOTE: Acceleration ramp only computes during first do-while loop.
          speed_var = pl_block->acceleration*time_var;
          mm_remaining -= time_var*(prep.current_speed + 0.5f*speed_var);
          if (mm_remaining < prep.accelerate_until) { // End of acceleration ramp.
            // Acceleration-cruise, acceleration-deceleration ramp junction, or end of block.
            mm_remaining = prep.accelerate_until; // NOTE: 0.0 at EOB
            time_var = 2.0f*(pl_block->millimeters-mm_remaining)/(prep.current_speed+prep.maximum_speed);
            if (mm_remaining == prep.decelerate_after) { prep.ramp_type = RAMP_DECEL; }
            else { prep.ramp_type = RAMP_CRUISE; }
            prep.current_speed = prep.maximum_speed;
          } else { // Acceleration only.
            prep.current_speed += speed_var;
          }
          break;
        case RAMP_CRUISE:
          // NOTE: mm_var used to retain the last mm_remaining for incomplete segment time_var calculations.
          // NOTE: If maximum_speed*time_var value is too low, round-off can cause mm_var to not change. To
          //   prevent this, simply enforce a minimum speed threshold in the planner.
          mm_var = mm_remaining - prep.maximum_speed*time_var;
          if (mm_var < prep.decelerate_after) { // End of cruise.
            // Cruise-deceleration junction or end of block.
            time_var = (mm_remaining - prep.decelerate_after)/prep.maximum_speed;
            mm_remaining = prep.decelerate_after; // NOTE: 0.0 at EOB
            prep.ramp_type = RAMP_DECEL;
          } else { // Cruising only.
            mm_remaining = mm_var;
          }
          break;
        default: // case RAMP_DECEL:
          // NOTE: mm_var used as a misc worker variable to prevent errors when near zero speed.
          speed_var = pl_block->acceleration*time_var; // Used as delta speed (mm/min)
          if (prep.current_speed > speed_var) { // Check if at or below zero speed.
            // Compute distance from end of segment to end of block.
            mm_var = mm_remaining - time_var*(prep.current_speed - 0.5f*speed_var); // (mm)
            if (mm_var > prep.mm_complete) { // Typical case. In deceleration ramp.
              mm_remaining = mm_var;
              prep.current_speed -= speed_var;
              break; // Segment complete. Exit switch-case statement. Continue do-while loop.
            }
          }
          // Otherwise, at end of block or end of forced-deceleration.
          time_var = 2.0f*(mm_remaining-prep.mm_complete)/(prep.current_speed+prep.exit_speed);
          mm_remaining = prep.mm_complete;
          prep.current_speed = prep.exit_speed;
      }
      dt += time_var; // Add computed ramp time to total segment time.
      if (dt < dt_max) { time_var = dt_max - dt; } // **Incomplete** At ramp junction.
      else {
        if (mm_remaining > minimum_mm) { // Check for very slow segments with zero steps.
          // Increase segment time to ensure at least one step in segment. Override and loop
          // through distance calculations until minimum_mm or mm_complete.
          dt_max += DT_SEGMENT;
          time_var = dt_max - dt;
        } else {
          break; // **Complete** Exit loop. Segment execution time maxed.
        }
      }
    } while (mm_remaining > prep.mm_complete); // **Complete** Exit loop. Profile complete.

    #ifdef VARIABLE_SPINDLE
      /* -----------------------------------------------------------------------------------
        Compute spindle speed PWM output for step segment
      */

      if (st_prep_block->is_pwm_rate_adjusted || (sys.step_control & STEP_CONTROL_UPDATE_SPINDLE_PWM)) {
        if (pl_block->condition & (PL_COND_FLAG_SPINDLE_CW | PL_COND_FLAG_SPINDLE_CCW)) {
          float rpm = pl_block->spindle_speed;
          // NOTE: Feed and rapid overrides are independent of PWM value and do not alter laser power/rate.        
          if (st_prep_block->is_pwm_rate_adjusted) { rpm *= (prep.current_speed * prep.inv_rate); }
          // If current_speed is zero, then may need to be rpm_min*(100/MAX_SPINDLE_SPEED_OVERRIDE)
          // but this would be instantaneous only and during a motion. May not matter at all.
          prep.current_spindle_pwm = spindle_compute_pwm_value(rpm);
        }
        else {
          sys.spindle_speed = 0.0;
          prep.current_spindle_pwm = SPINDLE_PWM_OFF_VALUE;
        }
        bit_false(sys.step_control, STEP_CONTROL_UPDATE_SPINDLE_PWM);
      }
      prep_segment->spindle_pwm = prep.current_spindle_pwm; // Reload segment PWM value

    #endif
    
    /* -----------------------------------------------------------------------------------
       Compute segment step rate, steps to execute, and apply necessary rate corrections.
       NOTE: Steps are computed by direct scalar conversion of the millimeter distance
       remaining in the block, rather than incrementally tallying the steps executed per
       segment. This helps in removing floating point round-off issues of several additions.
       However, since floats have only 7.2 significant digits, long moves with extremely
       high step counts can exceed the precision of floats, which can lead to lost steps.
       Fortunately, this scenario is highly unlikely and unrealistic in CNC machines
       supported by Grbl (i.e. exceeding 10 meters axis travel at 200 step/mm).
    */
    float step_dist_remaining = prep.step_per_mm*mm_remaining; // Convert mm_remaining to steps
    float n_steps_remaining = ceilf(step_dist_remaining); // Round-up current steps remaining
    float last_n_steps_remaining = ceilf(prep.steps_remaining); // Round-up last steps remaining
	prep_segment->n_step = (uint16_t)(last_n_steps_remaining - n_steps_remaining); // Compute number of steps to execute.

    // Bail if we are at the end of a feed hold and don't have a step to execute.
    if (prep_segment->n_step == 0) {
      if (sys.step_control & STEP_CONTROL_EXECUTE_HOLD) {
        // Less than one step to decelerate to zero speed, but already very close. AMASS
        // requires full steps to execute. So, just bail.
        bit_true(sys.step_control,STEP_CONTROL_END_MOTION);
        #ifdef PARKING_ENABLE
          if (!(prep.recalculate_flag & PREP_FLAG_PARKING)) { prep.recalculate_flag |= PREP_FLAG_HOLD_PARTIAL_BLOCK; }
        #endif
        return; // Segment not generated, but current step data still retained.
      }
    }

    // Compute segment step rate. Since steps are integers and mm distances traveled are not,
    // the end of every segment can have a partial step of varying magnitudes that are not
    // executed, because the stepper ISR requires whole steps due to the AMASS algorithm. To
    // compensate, we track the time to execute the previous segment's partial step and simply
    // apply it with the partial step distance to the current segment, so that it minutely
    // adjusts the whole segment rate to keep step output exact. These rate adjustments are
    // typically very small and do not adversely effect performance, but ensures that Grbl
    // outputs the exact acceleration and velocity profiles as computed by the planner.
    dt += prep.dt_remainder; // Apply previous segment partial step execute time
    float inv_rate = dt/(last_n_steps_remaining - step_dist_remaining); // Compute adjusted step rate inverse

    // Compute CPU cycles per step for the prepped segment.
	uint32_t cycles = (uint32_t)ceilf((TICKS_PER_MICROSECOND * 1000000) *inv_rate * 60); // (cycles/step)

    #ifdef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
      // Compute step timing and multi-axis smoothing level.
      // NOTE: AMASS overdrives the timer with each level, so only one prescalar is required.
      if (cycles < AMASS_LEVEL1) { prep_segment->amass_level = 0; }
      else {
        if (cycles < AMASS_LEVEL2) { prep_segment->amass_level = 1; }
        else if (cycles < AMASS_LEVEL3) { prep_segment->amass_level = 2; }
        else { prep_segment->amass_level = 3; }
        cycles >>= prep_segment->amass_level;
        prep_segment->n_step <<= prep_segment->amass_level;
      }
      if (cycles < (1UL << 16)) { prep_segment->cycles_per_tick = cycles; } // < 65536 (4.1ms @ 16MHz)
      else { prep_segment->cycles_per_tick = 0xffff; } // Just set the slowest speed possible.
    #else
      // Compute step timing and timer prescalar for normal step generation.
      if (cycles < (1UL << 16)) { // < 65536  (4.1ms @ 16MHz)
        prep_segment->prescaler = 1; // prescaler: 0
        prep_segment->cycles_per_tick = cycles;
      } else if (cycles < (1UL << 19)) { // < 524288 (32.8ms@16MHz)
        prep_segment->prescaler = 2; // prescaler: 8
        prep_segment->cycles_per_tick = cycles >> 3;
      } else {
        prep_segment->prescaler = 3; // prescaler: 64
        if (cycles < (1UL << 22)) { // < 4194304 (262ms@16MHz)
          prep_segment->cycles_per_tick =  cycles >> 6;
        } else { // Just set the slowest speed possible. (Around 4 step/sec.)
          prep_segment->cycles_per_tick = 0xffff;
        }
      }
    #endif

    // Segment complete! Increment segment buffer indices, so stepper ISR can immediately execute it.
    segment_buffer_head = segment_next_head;
    if ( ++segment_next_head == SEGMENT_BUFFER_SIZE ) { segment_next_head = 0; }

    // Update the appropriate planner and segment data.
    pl_block->millimeters = mm_remaining;
    prep.steps_remaining = n_steps_remaining;
    prep.dt_remainder = (n_steps_remaining - step_dist_remaining)*inv_rate;

    // Check for exit conditions and flag to load next planner block.
    if (mm_remaining == prep.mm_complete) {
      // End of planner block or forced-termination. No more distance to be executed.
      if (mm_remaining > 0.0f) { // At end of forced-termination.
        // Reset prep parameters for resuming and then bail. Allow the stepper ISR to complete
        // the segment queue, where realtime protocol will set new state upon receiving the
        // cycle stop flag from the ISR. Prep_segment is blocked until then.
        bit_true(sys.step_control,STEP_CONTROL_END_MOTION);
        #ifdef PARKING_ENABLE
          if (!(prep.recalculate_flag & PREP_FLAG_PARKING)) { prep.recalculate_flag |= PREP_FLAG_HOLD_PARTIAL_BLOCK; }
        #endif
        return; // Bail!
      } else { // End of planner block
        // The planner block is complete. All steps are set to be executed in the segment buffer.
        if (sys.step_control & STEP_CONTROL_EXECUTE_SYS_MOTION) {
          bit_true(sys.step_control,STEP_CONTROL_END_MOTION);
          return;
        }
        pl_block = NULL; // Set pointer to indicate check and load next planner block.
        plan_discard_current_block();
      }
    }

  }
}


// Called by realtime status reporting to fetch the current speed being executed. This value
// however is not exactly the current speed, but the speed computed in the last step segment
// in the segment buffer. It will always be behind by up to the number of segment blocks (-1)
// divided by the ACCELERATION TICKS PER SECOND in seconds.
float st_get_realtime_rate()
{
  if (sys.state & (STATE_CYCLE | STATE_HOMING | STATE_HOLD | STATE_JOG | STATE_SAFETY_DOOR)){
    return prep.current_speed;
  }
  return 0.0f;
}
#ifdef STM32F103C8
void TIM_Configuration(TIM_TypeDef* TIMER, u16 Period, u16 Prescaler, u8 PP)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	TIM_TimeBaseStructure.TIM_Period = Period - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = Prescaler - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIMER, &TIM_TimeBaseStructure);

	TIM_ClearITPendingBit(TIMER, TIM_IT_Update);
	TIM_ITConfig(TIMER, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIMER, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	if (TIMER == TIM2) { NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn; }
	else if (TIMER == TIM3) { NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn; }
	else if (TIMER == TIM4) { NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn; }

	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = PP;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}
#endif
