/*
  defaults.h - defaults settings configuration file
  Part of Grbl

  Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC

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

/* The defaults.h file serves as a central default settings selector for different machine
   types, from DIY CNC mills to CNC conversions of off-the-shelf machines. The settings
   files listed here are supplied by users, so your results may vary. However, this should
   give you a good starting point as you get to know your machine and tweak the settings for
   your nefarious needs.
   NOTE: Ensure one and only one of these DEFAULTS_XXX values is defined in config.h */


#include "config.h"

#ifndef defaults_h

#ifdef DEFAULTS_3040T

#define X_MICROSTEPS 16
#define Y_MICROSTEPS 16
//#define Z_MICROSTEPS 8
#define Z_MICROSTEPS 16

#define STEPS_PER_REV 200.0f  //每圈步数    42步进电机不需要变，后面计算时会与上面xyz_MICROSTEPS结合。
#define MM_PER_REV 40        //每圈移动距离

#define DEFAULT_X_STEPS_PER_MM (STEPS_PER_REV * X_MICROSTEPS / MM_PER_REV)		// $100		steps/mm 每1mm需要转几圈
#define DEFAULT_Y_STEPS_PER_MM (STEPS_PER_REV * Y_MICROSTEPS / MM_PER_REV)		// $101
#define DEFAULT_Z_STEPS_PER_MM (STEPS_PER_REV * Z_MICROSTEPS / MM_PER_REV)		// $102

/*
#define DEFAULT_X_MAX_RATE 3000.0f // mm/min	$110
#define DEFAULT_Y_MAX_RATE 3000.0f // mm/min	$111
#define DEFAULT_Z_MAX_RATE 500.0f // mm/min	$112
*/

#define DEFAULT_X_MAX_RATE 6000.0f // mm/min	$110
#define DEFAULT_Y_MAX_RATE 6000.0f // mm/min	$111
#define DEFAULT_Z_MAX_RATE 6000.0f // mm/min	$112

/*
#define DEFAULT_X_ACCELERATION (50.0f * 60 * 60) // 10*60*60 mm/min^2 = 10 mm/sec^2	$120
#define DEFAULT_Y_ACCELERATION (50.0f * 60 * 60) // 10*60*60 mm/min^2 = 10 mm/sec^2	$121
#define DEFAULT_Z_ACCELERATION (10.0f * 60 * 60) // 10*60*60 mm/min^2 = 10 mm/sec^2	$122
*/

#define DEFAULT_X_ACCELERATION (3000.0f * 60 * 60) // 10*60*60 mm/min^2 = 10 mm/sec^2	$120
#define DEFAULT_Y_ACCELERATION (3000.0f * 60 * 60) // 10*60*60 mm/min^2 = 10 mm/sec^2	$121
#define DEFAULT_Z_ACCELERATION (3000.0f * 60 * 60) // 10*60*60 mm/min^2 = 10 mm/sec^2	$122

#define DEFAULT_X_MAX_TRAVEL 280.0f // mm NOTE: Must be a positive value.	$130
#define DEFAULT_Y_MAX_TRAVEL 377.0f // mm NOTE: Must be a positive value.	$131
#define DEFAULT_Z_MAX_TRAVEL 53.0f // mm NOTE: Must be a positive value.	$132

#define DEFAULT_STEP_PULSE_MICROSECONDS 4		//usec		$0
#define DEFAULT_STEPPER_IDLE_LOCK_TIME 25 // msec (0-254, 255 keeps steppers enabled)	$1
#define DEFAULT_STEPPING_INVERT_MASK 0		//					$2
#define DEFAULT_DIRECTION_INVERT_MASK 4		//					$3  invert Z
#define DEFAULT_INVERT_ST_ENABLE 0 // false						$4
#define DEFAULT_INVERT_LIMIT_PINS 0 // false					$5
#define DEFAULT_INVERT_PROBE_PIN 0 // true						$6

#define DEFAULT_STATUS_REPORT_MASK 3 // MPos enabled	$10
#define DEFAULT_JUNCTION_DEVIATION 0.01f // mm				$11
#define DEFAULT_ARC_TOLERANCE 0.002f // mm						$12
#define DEFAULT_REPORT_INCHES 0 // false							$13

#define DEFAULT_SOFT_LIMIT_ENABLE 0 // false					$20
#define DEFAULT_HARD_LIMIT_ENABLE 1  // true					$21
#define DEFAULT_HOMING_ENABLE 1  // false							$22
#define DEFAULT_HOMING_DIR_MASK 1 // move positive dir $23
#define DEFAULT_HOMING_FEED_RATE 25.0f // mm/min			$24
#define DEFAULT_HOMING_SEEK_RATE 500.0f // mm/min			$25
#define DEFAULT_HOMING_DEBOUNCE_DELAY 250 // msec (0-65k)	$26
#define DEFAULT_HOMING_PULLOFF 1.0f // mm							$27

#define DEFAULT_SPINDLE_RPM_MAX 8000.0f 		//rpm			$30
#define DEFAULT_SPINDLE_RPM_MIN 0.0f 			//rpm				$31
#define DEFAULT_LASER_MODE 0 // false									$32

#define DEFAULT_ANALOG_MAX 10000.0f //analog value		$40
#define DEFAULT_VARIABLE_SPINDLE_ENABLE_PIN 0               //$50

#endif


#ifdef DEFAULTS_GENERIC
  // Grbl generic default settings. Should work across different machines.
  #define DEFAULT_X_STEPS_PER_MM 250.0f
  #define DEFAULT_Y_STEPS_PER_MM 250.0f
  #define DEFAULT_Z_STEPS_PER_MM 250.0f
  #define DEFAULT_X_MAX_RATE 500.0f // mm/min
  #define DEFAULT_Y_MAX_RATE 500.0f // mm/min
  #define DEFAULT_Z_MAX_RATE 500.0f // mm/min
  #define DEFAULT_X_ACCELERATION (10.0f*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
  #define DEFAULT_Y_ACCELERATION (10.0f*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
  #define DEFAULT_Z_ACCELERATION (10.0f*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
  #define DEFAULT_X_MAX_TRAVEL 200.0f // mm NOTE: Must be a positive value.
  #define DEFAULT_Y_MAX_TRAVEL 200.0f // mm NOTE: Must be a positive value.
  #define DEFAULT_Z_MAX_TRAVEL 200.0f // mm NOTE: Must be a positive value.
  #define DEFAULT_SPINDLE_RPM_MAX 1000.0f // rpm
  #define DEFAULT_SPINDLE_RPM_MIN 0.0f // rpm
  #define DEFAULT_STEP_PULSE_MICROSECONDS 10
  #define DEFAULT_STEPPING_INVERT_MASK 0
  #define DEFAULT_DIRECTION_INVERT_MASK 0
  #define DEFAULT_STEPPER_IDLE_LOCK_TIME 25 // msec (0-254, 255 keeps steppers enabled)
  #define DEFAULT_STATUS_REPORT_MASK 1 // MPos enabled
  #define DEFAULT_JUNCTION_DEVIATION 0.01f // mm
  #define DEFAULT_ARC_TOLERANCE 0.002f // mm
  #define DEFAULT_REPORT_INCHES 0 // false
  #define DEFAULT_INVERT_ST_ENABLE 0 // false
  #define DEFAULT_INVERT_LIMIT_PINS 0 // false
  #define DEFAULT_SOFT_LIMIT_ENABLE 0 // false
  #define DEFAULT_HARD_LIMIT_ENABLE 0  // false
  #define DEFAULT_INVERT_PROBE_PIN 0 // false
  #define DEFAULT_LASER_MODE 0 // false
  #define DEFAULT_HOMING_ENABLE 0  // false
  #define DEFAULT_HOMING_DIR_MASK 0 // move positive dir
  #define DEFAULT_HOMING_FEED_RATE 25.0f // mm/min
  #define DEFAULT_HOMING_SEEK_RATE 500.0f // mm/min
  #define DEFAULT_HOMING_DEBOUNCE_DELAY 250 // msec (0-65k)
  #define DEFAULT_HOMING_PULLOFF 1.0f // mm
#endif

#ifdef DEFAULTS_SHERLINE_5400
  // Description: Sherline 5400 mill with three NEMA 23 Keling  KL23H256-21-8B 185 oz-in stepper motors,
  // driven by three Pololu A4988 stepper drivers with a 30V, 6A power supply at 1.5A per winding.
  #define MICROSTEPS 2
  #define STEPS_PER_REV 200.0
  #define MM_PER_REV (0.050*MM_PER_INCH) // 0.050 inch/rev leadscrew
  #define DEFAULT_X_STEPS_PER_MM (STEPS_PER_REV*MICROSTEPS/MM_PER_REV)
  #define DEFAULT_Y_STEPS_PER_MM (STEPS_PER_REV*MICROSTEPS/MM_PER_REV)
  #define DEFAULT_Z_STEPS_PER_MM (STEPS_PER_REV*MICROSTEPS/MM_PER_REV)
  #define DEFAULT_X_MAX_RATE 635.0 // mm/min (25 ipm)
  #define DEFAULT_Y_MAX_RATE 635.0 // mm/min
  #define DEFAULT_Z_MAX_RATE 635.0 // mm/min
  #define DEFAULT_X_ACCELERATION (50.0*60*60) // 50*60*60 mm/min^2 = 50 mm/sec^2
  #define DEFAULT_Y_ACCELERATION (50.0*60*60) // 50*60*60 mm/min^2 = 50 mm/sec^2
  #define DEFAULT_Z_ACCELERATION (50.0*60*60) // 50*60*60 mm/min^2 = 50 mm/sec^2
  #define DEFAULT_X_MAX_TRAVEL 225.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Y_MAX_TRAVEL 125.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Z_MAX_TRAVEL 170.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_SPINDLE_RPM_MAX 2800.0 // rpm
  #define DEFAULT_SPINDLE_RPM_MIN 0.0 // rpm
  #define DEFAULT_STEP_PULSE_MICROSECONDS 10
  #define DEFAULT_STEPPING_INVERT_MASK 0
  #define DEFAULT_DIRECTION_INVERT_MASK ((1<<Y_AXIS)|(1<<Z_AXIS))
  #define DEFAULT_STEPPER_IDLE_LOCK_TIME 25 // msec (0-254, 255 keeps steppers enabled)
  #define DEFAULT_STATUS_REPORT_MASK 1 // MPos enabled
  #define DEFAULT_JUNCTION_DEVIATION 0.01 // mm
  #define DEFAULT_ARC_TOLERANCE 0.002 // mm
  #define DEFAULT_REPORT_INCHES 0 // true
  #define DEFAULT_INVERT_ST_ENABLE 0 // false
  #define DEFAULT_INVERT_LIMIT_PINS 0 // false
  #define DEFAULT_SOFT_LIMIT_ENABLE 0 // false
  #define DEFAULT_HARD_LIMIT_ENABLE 0  // false
  #define DEFAULT_INVERT_PROBE_PIN 0 // false
  #define DEFAULT_LASER_MODE 0 // false
  #define DEFAULT_HOMING_ENABLE 0  // false
  #define DEFAULT_HOMING_DIR_MASK 0 // move positive dir
  #define DEFAULT_HOMING_FEED_RATE 50.0 // mm/min
  #define DEFAULT_HOMING_SEEK_RATE 635.0 // mm/min
  #define DEFAULT_HOMING_DEBOUNCE_DELAY 250 // msec (0-65k)
  #define DEFAULT_HOMING_PULLOFF 1.0 // mm
#endif
#ifdef DEFAULTS_POCKETNC_FR4
	// Description: Pocket NC FR4 CNC mill.
#define DEFAULT_X_STEPS_PER_MM 800.0
#define DEFAULT_Y_STEPS_PER_MM 800.0
#define DEFAULT_Z_STEPS_PER_MM 800.0
#define DEFAULT_X_MAX_RATE 300.0 // mm/min
#define DEFAULT_Y_MAX_RATE 300.0 // mm/min
#define DEFAULT_Z_MAX_RATE 300.0 // mm/min
#define DEFAULT_X_ACCELERATION (30.0*60*60) // 15*60*60 mm/min^2 = 15 mm/sec^2
#define DEFAULT_Y_ACCELERATION (30.0*60*60) // 15*60*60 mm/min^2 = 15 mm/sec^2
#define DEFAULT_Z_ACCELERATION (30.0*60*60) // 15*60*60 mm/min^2 = 15 mm/sec^2
#define DEFAULT_X_MAX_TRAVEL 225.0 // mm NOTE: Must be a positive value.
#define DEFAULT_Y_MAX_TRAVEL 125.0 // mm NOTE: Must be a positive value.
#define DEFAULT_Z_MAX_TRAVEL 170.0 // mm NOTE: Must be a positive value.
#define DEFAULT_SPINDLE_RPM_MAX 7000.0 // rpm
#define DEFAULT_SPINDLE_RPM_MIN 0.0 // rpm
#define DEFAULT_STEP_PULSE_MICROSECONDS 10
#define DEFAULT_STEPPING_INVERT_MASK 0
#define DEFAULT_DIRECTION_INVERT_MASK ((1<<Y_AXIS)|(1<<Z_AXIS))
#define DEFAULT_STEPPER_IDLE_LOCK_TIME 250 // msec (0-254, 255 keeps steppers enabled)
#define DEFAULT_STATUS_REPORT_MASK 3 // WPos enabled
#define DEFAULT_JUNCTION_DEVIATION 0.01 // mm
#define DEFAULT_ARC_TOLERANCE 0.002 // mm
#define DEFAULT_REPORT_INCHES 0 // false
#define DEFAULT_INVERT_ST_ENABLE 1 // false
#define DEFAULT_INVERT_LIMIT_PINS 0 // false
#define DEFAULT_SOFT_LIMIT_ENABLE 0 // false
#define DEFAULT_HARD_LIMIT_ENABLE 0  // false
#define DEFAULT_INVERT_PROBE_PIN 0 // false
#define DEFAULT_LASER_MODE 0 // false
#define DEFAULT_HOMING_ENABLE 1  // false
#define DEFAULT_HOMING_DIR_MASK 1 // move positive dir
#define DEFAULT_HOMING_FEED_RATE 100.0 // mm/min
#define DEFAULT_HOMING_SEEK_RATE 300.0 // mm/min
#define DEFAULT_HOMING_DEBOUNCE_DELAY 250 // msec (0-65k)
#define DEFAULT_HOMING_PULLOFF 3.0 // mm
#endif
#ifdef DEFAULTS_SHAPEOKO
  // Description: Shapeoko CNC mill with three NEMA 17 stepper motors, driven by Synthetos
  // grblShield with a 24V, 4.2A power supply.
  #define MICROSTEPS_XY 8
  #define STEP_REVS_XY 400
  #define MM_PER_REV_XY (0.08*18*MM_PER_INCH) // 0.08 in belt pitch, 18 pulley teeth
  #define MICROSTEPS_Z 2
  #define STEP_REVS_Z 400
  #define MM_PER_REV_Z 1.250 // 1.25 mm/rev leadscrew
  #define DEFAULT_X_STEPS_PER_MM (MICROSTEPS_XY*STEP_REVS_XY/MM_PER_REV_XY)
  #define DEFAULT_Y_STEPS_PER_MM (MICROSTEPS_XY*STEP_REVS_XY/MM_PER_REV_XY)
  #define DEFAULT_Z_STEPS_PER_MM (MICROSTEPS_Z*STEP_REVS_Z/MM_PER_REV_Z)
  #define DEFAULT_X_MAX_RATE 1000.0 // mm/min
  #define DEFAULT_Y_MAX_RATE 1000.0 // mm/min
  #define DEFAULT_Z_MAX_RATE 1000.0 // mm/min
  #define DEFAULT_X_ACCELERATION (15.0*60*60) // 15*60*60 mm/min^2 = 15 mm/sec^2
  #define DEFAULT_Y_ACCELERATION (15.0*60*60) // 15*60*60 mm/min^2 = 15 mm/sec^2
  #define DEFAULT_Z_ACCELERATION (15.0*60*60) // 15*60*60 mm/min^2 = 15 mm/sec^2
  #define DEFAULT_X_MAX_TRAVEL 200.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Y_MAX_TRAVEL 200.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Z_MAX_TRAVEL 200.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_SPINDLE_RPM_MAX 10000.0 // rpm
  #define DEFAULT_SPINDLE_RPM_MIN 0.0 // rpm
  #define DEFAULT_STEP_PULSE_MICROSECONDS 10
  #define DEFAULT_STEPPING_INVERT_MASK 0
  #define DEFAULT_DIRECTION_INVERT_MASK ((1<<Y_AXIS)|(1<<Z_AXIS))
  #define DEFAULT_STEPPER_IDLE_LOCK_TIME 255 // msec (0-254, 255 keeps steppers enabled)
  #define DEFAULT_STATUS_REPORT_MASK 1 // MPos enabled
  #define DEFAULT_JUNCTION_DEVIATION 0.02 // mm
  #define DEFAULT_ARC_TOLERANCE 0.002 // mm
  #define DEFAULT_REPORT_INCHES 0 // false
  #define DEFAULT_INVERT_ST_ENABLE 0 // false
  #define DEFAULT_INVERT_LIMIT_PINS 0 // false
  #define DEFAULT_SOFT_LIMIT_ENABLE 0 // false
  #define DEFAULT_HARD_LIMIT_ENABLE 0  // false
  #define DEFAULT_INVERT_PROBE_PIN 0 // false
  #define DEFAULT_LASER_MODE 0 // false
  #define DEFAULT_HOMING_ENABLE 0  // false
  #define DEFAULT_HOMING_DIR_MASK 0 // move positive dir
  #define DEFAULT_HOMING_FEED_RATE 25.0 // mm/min
  #define DEFAULT_HOMING_SEEK_RATE 250.0 // mm/min
  #define DEFAULT_HOMING_DEBOUNCE_DELAY 250 // msec (0-65k)
  #define DEFAULT_HOMING_PULLOFF 1.0 // mm
#endif

#ifdef DEFAULTS_SHAPEOKO_2
  // Description: Shapeoko CNC mill with three NEMA 17 stepper motors, driven by Synthetos
  // grblShield at 28V.
  #define MICROSTEPS_XY 8
  #define STEP_REVS_XY 200
  #define MM_PER_REV_XY (2.0*20) // 2mm belt pitch, 20 pulley teeth
  #define MICROSTEPS_Z 2
  #define STEP_REVS_Z 200
  #define MM_PER_REV_Z 1.250 // 1.25 mm/rev leadscrew
  #define DEFAULT_X_STEPS_PER_MM (MICROSTEPS_XY*STEP_REVS_XY/MM_PER_REV_XY)
  #define DEFAULT_Y_STEPS_PER_MM (MICROSTEPS_XY*STEP_REVS_XY/MM_PER_REV_XY)
  #define DEFAULT_Z_STEPS_PER_MM (MICROSTEPS_Z*STEP_REVS_Z/MM_PER_REV_Z)
  #define DEFAULT_X_MAX_RATE 5000.0 // mm/min
  #define DEFAULT_Y_MAX_RATE 5000.0 // mm/min
  #define DEFAULT_Z_MAX_RATE 500.0 // mm/min
  #define DEFAULT_X_ACCELERATION (250.0*60*60) // 25*60*60 mm/min^2 = 25 mm/sec^2
  #define DEFAULT_Y_ACCELERATION (250.0*60*60) // 25*60*60 mm/min^2 = 25 mm/sec^2
  #define DEFAULT_Z_ACCELERATION (50.0*60*60) // 25*60*60 mm/min^2 = 25 mm/sec^2
  #define DEFAULT_X_MAX_TRAVEL 290.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Y_MAX_TRAVEL 290.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Z_MAX_TRAVEL 100.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_SPINDLE_RPM_MAX 10000.0 // rpm
  #define DEFAULT_SPINDLE_RPM_MIN 0.0 // rpm
  #define DEFAULT_STEP_PULSE_MICROSECONDS 10
  #define DEFAULT_STEPPING_INVERT_MASK 0
  #define DEFAULT_DIRECTION_INVERT_MASK ((1<<X_AXIS)|(1<<Z_AXIS))
  #define DEFAULT_STEPPER_IDLE_LOCK_TIME 255 // msec (0-254, 255 keeps steppers enabled)
  #define DEFAULT_STATUS_REPORT_MASK 1 // MPos enabled
  #define DEFAULT_JUNCTION_DEVIATION 0.02 // mm
  #define DEFAULT_ARC_TOLERANCE 0.002 // mm
  #define DEFAULT_REPORT_INCHES 0 // false
  #define DEFAULT_INVERT_ST_ENABLE 0 // false
  #define DEFAULT_INVERT_LIMIT_PINS 0 // false
  #define DEFAULT_SOFT_LIMIT_ENABLE 0 // false
  #define DEFAULT_HARD_LIMIT_ENABLE 0  // false
  #define DEFAULT_INVERT_PROBE_PIN 0 // false
  #define DEFAULT_LASER_MODE 0 // false
  #define DEFAULT_HOMING_ENABLE 0  // false
  #define DEFAULT_HOMING_DIR_MASK 0 // move positive dir
  #define DEFAULT_HOMING_FEED_RATE 25.0 // mm/min
  #define DEFAULT_HOMING_SEEK_RATE 250.0 // mm/min
  #define DEFAULT_HOMING_DEBOUNCE_DELAY 250 // msec (0-65k)
  #define DEFAULT_HOMING_PULLOFF 1.0 // mm
#endif

#ifdef DEFAULTS_SHAPEOKO_3
  // Description: Shapeoko CNC mill with three NEMA 23 stepper motors, driven by CarbideMotion
  #define MICROSTEPS_XY 8
  #define STEP_REVS_XY 200
  #define MM_PER_REV_XY (2.0*20) // 2mm belt pitch, 20 pulley teeth
  #define MICROSTEPS_Z 8
  #define STEP_REVS_Z 200
  #define MM_PER_REV_Z (2.0*20) // 2mm belt pitch, 20 pulley teeth
  #define DEFAULT_X_STEPS_PER_MM (MICROSTEPS_XY*STEP_REVS_XY/MM_PER_REV_XY)
  #define DEFAULT_Y_STEPS_PER_MM (MICROSTEPS_XY*STEP_REVS_XY/MM_PER_REV_XY)
  #define DEFAULT_Z_STEPS_PER_MM (MICROSTEPS_Z*STEP_REVS_Z/MM_PER_REV_Z)
  #define DEFAULT_X_MAX_RATE 5000.0 // mm/min
  #define DEFAULT_Y_MAX_RATE 5000.0 // mm/min
  #define DEFAULT_Z_MAX_RATE 5000.0 // mm/min
  #define DEFAULT_X_ACCELERATION (400.0*60*60) // 400*60*60 mm/min^2 = 400 mm/sec^2
  #define DEFAULT_Y_ACCELERATION (400.0*60*60) // 400*60*60 mm/min^2 = 400 mm/sec^2
  #define DEFAULT_Z_ACCELERATION (400.0*60*60) // 400*60*60 mm/min^2 = 400 mm/sec^2
  #define DEFAULT_X_MAX_TRAVEL 425.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Y_MAX_TRAVEL 465.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Z_MAX_TRAVEL 80.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_SPINDLE_RPM_MAX 10000.0 // rpm
  #define DEFAULT_SPINDLE_RPM_MIN 0.0 // rpm
  #define DEFAULT_STEP_PULSE_MICROSECONDS 10
  #define DEFAULT_STEPPING_INVERT_MASK 0
  #define DEFAULT_DIRECTION_INVERT_MASK ((1<<X_AXIS)|(1<<Z_AXIS))
  #define DEFAULT_STEPPER_IDLE_LOCK_TIME 255 // msec (0-254, 255 keeps steppers enabled)
  #define DEFAULT_STATUS_REPORT_MASK 1 // MPos enabled
  #define DEFAULT_JUNCTION_DEVIATION 0.02 // mm
  #define DEFAULT_ARC_TOLERANCE 0.01 // mm
  #define DEFAULT_REPORT_INCHES 0 // false
  #define DEFAULT_INVERT_ST_ENABLE 0 // false
  #define DEFAULT_INVERT_LIMIT_PINS 0 // false
  #define DEFAULT_SOFT_LIMIT_ENABLE 0 // false
  #define DEFAULT_HARD_LIMIT_ENABLE 0  // false
  #define DEFAULT_INVERT_PROBE_PIN 0 // false
  #define DEFAULT_LASER_MODE 0 // false
  #define DEFAULT_HOMING_ENABLE 0  // false
  #define DEFAULT_HOMING_DIR_MASK 0 // move positive dir
  #define DEFAULT_HOMING_FEED_RATE 100.0 // mm/min
  #define DEFAULT_HOMING_SEEK_RATE 1000.0 // mm/min
  #define DEFAULT_HOMING_DEBOUNCE_DELAY 25 // msec (0-65k)
  #define DEFAULT_HOMING_PULLOFF 5.0 // mm
#endif

#ifdef DEFAULTS_X_CARVE_500MM
  // Description: X-Carve 3D Carver CNC mill with three 200 step/rev motors driven by Synthetos
  // grblShield at 24V.
  #define MICROSTEPS_XY 8
  #define STEP_REVS_XY 200
  #define MM_PER_REV_XY (2.0*20) // 2mm belt pitch, 20 pulley teeth
  #define MICROSTEPS_Z 2
  #define STEP_REVS_Z 200
  #define MM_PER_REV_Z 2.117 // ACME 3/8-12 Leadscrew
  #define DEFAULT_X_STEPS_PER_MM (MICROSTEPS_XY*STEP_REVS_XY/MM_PER_REV_XY)
  #define DEFAULT_Y_STEPS_PER_MM (MICROSTEPS_XY*STEP_REVS_XY/MM_PER_REV_XY)
  #define DEFAULT_Z_STEPS_PER_MM (MICROSTEPS_Z*STEP_REVS_Z/MM_PER_REV_Z)
  #define DEFAULT_X_MAX_RATE 8000.0 // mm/min
  #define DEFAULT_Y_MAX_RATE 8000.0 // mm/min
  #define DEFAULT_Z_MAX_RATE 500.0 // mm/min
  #define DEFAULT_X_ACCELERATION (500.0*60*60) // 25*60*60 mm/min^2 = 25 mm/sec^2
  #define DEFAULT_Y_ACCELERATION (500.0*60*60) // 25*60*60 mm/min^2 = 25 mm/sec^2
  #define DEFAULT_Z_ACCELERATION (50.0*60*60) // 25*60*60 mm/min^2 = 25 mm/sec^2
  #define DEFAULT_X_MAX_TRAVEL 290.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Y_MAX_TRAVEL 290.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Z_MAX_TRAVEL 100.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_SPINDLE_RPM_MAX 10000.0 // rpm
  #define DEFAULT_SPINDLE_RPM_MIN 0.0 // rpm
  #define DEFAULT_STEP_PULSE_MICROSECONDS 10
  #define DEFAULT_STEPPING_INVERT_MASK 0
  #define DEFAULT_DIRECTION_INVERT_MASK ((1<<X_AXIS)|(1<<Y_AXIS))
  #define DEFAULT_STEPPER_IDLE_LOCK_TIME 255 // msec (0-254, 255 keeps steppers enabled)
  #define DEFAULT_STATUS_REPORT_MASK 1 // MPos enabled
  #define DEFAULT_JUNCTION_DEVIATION 0.02 // mm
  #define DEFAULT_ARC_TOLERANCE 0.002 // mm
  #define DEFAULT_REPORT_INCHES 0 // false
  #define DEFAULT_INVERT_ST_ENABLE 0 // false
  #define DEFAULT_INVERT_LIMIT_PINS 0 // false
  #define DEFAULT_SOFT_LIMIT_ENABLE 0 // false
  #define DEFAULT_HARD_LIMIT_ENABLE 0  // false
  #define DEFAULT_INVERT_PROBE_PIN 0 // false
  #define DEFAULT_LASER_MODE 0 // false
  #define DEFAULT_HOMING_ENABLE 0  // false
  #define DEFAULT_HOMING_DIR_MASK 3 // move positive dir
  #define DEFAULT_HOMING_FEED_RATE 25.0 // mm/min
  #define DEFAULT_HOMING_SEEK_RATE 750.0 // mm/min
  #define DEFAULT_HOMING_DEBOUNCE_DELAY 250 // msec (0-65k)
  #define DEFAULT_HOMING_PULLOFF 1.0 // mm
#endif

#ifdef DEFAULTS_X_CARVE_1000MM
  // Description: X-Carve 3D Carver CNC mill with three 200 step/rev motors driven by Synthetos
  // grblShield at 24V.
  #define MICROSTEPS_XY 8
  #define STEP_REVS_XY 200
  #define MM_PER_REV_XY (2.0*20) // 2mm belt pitch, 20 pulley teeth
  #define MICROSTEPS_Z 2
  #define STEP_REVS_Z 200
  #define MM_PER_REV_Z 2.117 // ACME 3/8-12 Leadscrew
  #define DEFAULT_X_STEPS_PER_MM (MICROSTEPS_XY*STEP_REVS_XY/MM_PER_REV_XY)
  #define DEFAULT_Y_STEPS_PER_MM (MICROSTEPS_XY*STEP_REVS_XY/MM_PER_REV_XY)
  #define DEFAULT_Z_STEPS_PER_MM (MICROSTEPS_Z*STEP_REVS_Z/MM_PER_REV_Z)
  #define DEFAULT_X_MAX_RATE 8000.0 // mm/min
  #define DEFAULT_Y_MAX_RATE 8000.0 // mm/min
  #define DEFAULT_Z_MAX_RATE 500.0 // mm/min
  #define DEFAULT_X_ACCELERATION (500.0*60*60) // 25*60*60 mm/min^2 = 25 mm/sec^2
  #define DEFAULT_Y_ACCELERATION (500.0*60*60) // 25*60*60 mm/min^2 = 25 mm/sec^2
  #define DEFAULT_Z_ACCELERATION (50.0*60*60) // 25*60*60 mm/min^2 = 25 mm/sec^2
  #define DEFAULT_X_MAX_TRAVEL 740.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Y_MAX_TRAVEL 790.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Z_MAX_TRAVEL 100.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_SPINDLE_RPM_MAX 10000.0 // rpm
  #define DEFAULT_SPINDLE_RPM_MIN 0.0 // rpm
  #define DEFAULT_STEP_PULSE_MICROSECONDS 10
  #define DEFAULT_STEPPING_INVERT_MASK 0
  #define DEFAULT_DIRECTION_INVERT_MASK ((1<<X_AXIS)|(1<<Y_AXIS))
  #define DEFAULT_STEPPER_IDLE_LOCK_TIME 255 // msec (0-254, 255 keeps steppers enabled)
  #define DEFAULT_STATUS_REPORT_MASK 1 // MPos enabled
  #define DEFAULT_JUNCTION_DEVIATION 0.02 // mm
  #define DEFAULT_ARC_TOLERANCE 0.002 // mm
  #define DEFAULT_REPORT_INCHES 0 // false
  #define DEFAULT_INVERT_ST_ENABLE 0 // false
  #define DEFAULT_INVERT_LIMIT_PINS 0 // false
  #define DEFAULT_SOFT_LIMIT_ENABLE 0 // false
  #define DEFAULT_HARD_LIMIT_ENABLE 0  // false
  #define DEFAULT_INVERT_PROBE_PIN 0 // false
  #define DEFAULT_LASER_MODE 0 // false
  #define DEFAULT_HOMING_ENABLE 0  // false
  #define DEFAULT_HOMING_DIR_MASK 3 // move positive dir
  #define DEFAULT_HOMING_FEED_RATE 25.0 // mm/min
  #define DEFAULT_HOMING_SEEK_RATE 750.0 // mm/min
  #define DEFAULT_HOMING_DEBOUNCE_DELAY 250 // msec (0-65k)
  #define DEFAULT_HOMING_PULLOFF 1.0 // mm
#endif

#ifdef DEFAULTS_ZEN_TOOLWORKS_7x7
  // Description: Zen Toolworks 7x7 mill with three Shinano SST43D2121 65oz-in NEMA 17 stepper motors.
  // Leadscrew is different from some ZTW kits, where most are 1.25mm/rev rather than 8.0mm/rev here.
  // Driven by 30V, 6A power supply and TI DRV8811 stepper motor drivers.
  #define MICROSTEPS 8
  #define STEPS_PER_REV 200.0
  #define MM_PER_REV 8.0 // 8 mm/rev leadscrew
  #define DEFAULT_X_STEPS_PER_MM (STEPS_PER_REV*MICROSTEPS/MM_PER_REV)
  #define DEFAULT_Y_STEPS_PER_MM (STEPS_PER_REV*MICROSTEPS/MM_PER_REV)
  #define DEFAULT_Z_STEPS_PER_MM (STEPS_PER_REV*MICROSTEPS/MM_PER_REV)
  #define DEFAULT_X_MAX_RATE 6000.0 // mm/min
  #define DEFAULT_Y_MAX_RATE 6000.0 // mm/min
  #define DEFAULT_Z_MAX_RATE 6000.0 // mm/min
  #define DEFAULT_X_ACCELERATION (600.0*60*60) // 600*60*60 mm/min^2 = 600 mm/sec^2
  #define DEFAULT_Y_ACCELERATION (600.0*60*60) // 600*60*60 mm/min^2 = 600 mm/sec^2
  #define DEFAULT_Z_ACCELERATION (600.0*60*60) // 600*60*60 mm/min^2 = 600 mm/sec^2
  #define DEFAULT_X_MAX_TRAVEL 190.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Y_MAX_TRAVEL 180.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Z_MAX_TRAVEL 150.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_SPINDLE_RPM_MAX 10000.0 // rpm
  #define DEFAULT_SPINDLE_RPM_MIN 0.0 // rpm
  #define DEFAULT_STEP_PULSE_MICROSECONDS 10
  #define DEFAULT_STEPPING_INVERT_MASK 0
  #define DEFAULT_DIRECTION_INVERT_MASK ((1<<Y_AXIS))
  #define DEFAULT_STEPPER_IDLE_LOCK_TIME 25 // msec (0-254, 255 keeps steppers enabled)
  #define DEFAULT_STATUS_REPORT_MASK 1 // MPos enabled
  #define DEFAULT_JUNCTION_DEVIATION 0.02 // mm
  #define DEFAULT_ARC_TOLERANCE 0.002 // mm
  #define DEFAULT_REPORT_INCHES 0 // false
  #define DEFAULT_INVERT_ST_ENABLE 0 // false
  #define DEFAULT_INVERT_LIMIT_PINS 0 // false
  #define DEFAULT_SOFT_LIMIT_ENABLE 0 // false
  #define DEFAULT_HARD_LIMIT_ENABLE 0  // false
  #define DEFAULT_INVERT_PROBE_PIN 0 // false
  #define DEFAULT_LASER_MODE 0 // false
  #define DEFAULT_HOMING_ENABLE 0  // false
  #define DEFAULT_HOMING_DIR_MASK 0 // move positive dir
  #define DEFAULT_HOMING_FEED_RATE 25.0 // mm/min
  #define DEFAULT_HOMING_SEEK_RATE 250.0 // mm/min
  #define DEFAULT_HOMING_DEBOUNCE_DELAY 250 // msec (0-65k)
  #define DEFAULT_HOMING_PULLOFF 1.0 // mm
#endif

#ifdef DEFAULTS_OXCNC
  // Grbl settings for OpenBuilds OX CNC Machine
  // http://www.openbuilds.com/builds/openbuilds-ox-cnc-machine.341/
  #define DEFAULT_X_STEPS_PER_MM 26.670
  #define DEFAULT_Y_STEPS_PER_MM 26.670
  #define DEFAULT_Z_STEPS_PER_MM 50
  #define DEFAULT_X_MAX_RATE 500.0 // mm/min
  #define DEFAULT_Y_MAX_RATE 500.0 // mm/min
  #define DEFAULT_Z_MAX_RATE 500.0 // mm/min
  #define DEFAULT_X_ACCELERATION (10.0*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
  #define DEFAULT_Y_ACCELERATION (10.0*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
  #define DEFAULT_Z_ACCELERATION (10.0*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
  #define DEFAULT_X_MAX_TRAVEL 500.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Y_MAX_TRAVEL 750.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Z_MAX_TRAVEL 80.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_SPINDLE_RPM_MAX 1000.0 // rpm
  #define DEFAULT_SPINDLE_RPM_MIN 0.0 // rpm
  #define DEFAULT_STEP_PULSE_MICROSECONDS 10
  #define DEFAULT_STEPPING_INVERT_MASK 0
  #define DEFAULT_DIRECTION_INVERT_MASK 0
  #define DEFAULT_STEPPER_IDLE_LOCK_TIME 25 // msec (0-254, 255 keeps steppers enabled)
  #define DEFAULT_STATUS_REPORT_MASK 1 // MPos enabled
  #define DEFAULT_JUNCTION_DEVIATION 0.02 // mm
  #define DEFAULT_ARC_TOLERANCE 0.002 // mm
  #define DEFAULT_REPORT_INCHES 0 // false
  #define DEFAULT_INVERT_ST_ENABLE 0 // false
  #define DEFAULT_INVERT_LIMIT_PINS 0 // false
  #define DEFAULT_SOFT_LIMIT_ENABLE 0 // false
  #define DEFAULT_HARD_LIMIT_ENABLE 0  // false
  #define DEFAULT_INVERT_PROBE_PIN 0 // false
  #define DEFAULT_LASER_MODE 0 // false
  #define DEFAULT_HOMING_ENABLE 0  // false
  #define DEFAULT_HOMING_DIR_MASK 0 // move positive dir
  #define DEFAULT_HOMING_FEED_RATE 25.0 // mm/min
  #define DEFAULT_HOMING_SEEK_RATE 500.0 // mm/min
  #define DEFAULT_HOMING_DEBOUNCE_DELAY 250 // msec (0-65k)
  #define DEFAULT_HOMING_PULLOFF 1.0 // mm
#endif

#ifdef DEFAULTS_SIMULATOR
  // Settings only for Grbl Simulator (www.github.com/grbl/grbl-sim)
  // Grbl generic default settings. Should work across different machines.
  #define DEFAULT_X_STEPS_PER_MM 1000.0
  #define DEFAULT_Y_STEPS_PER_MM 1000.0
  #define DEFAULT_Z_STEPS_PER_MM 1000.0
  #define DEFAULT_X_MAX_RATE 1000.0 // mm/min
  #define DEFAULT_Y_MAX_RATE 1000.0 // mm/min
  #define DEFAULT_Z_MAX_RATE 1000.0 // mm/min
  #define DEFAULT_X_ACCELERATION (100.0*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
  #define DEFAULT_Y_ACCELERATION (100.0*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
  #define DEFAULT_Z_ACCELERATION (100.0*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
  #define DEFAULT_X_MAX_TRAVEL 1000.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Y_MAX_TRAVEL 1000.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_Z_MAX_TRAVEL 1000.0 // mm NOTE: Must be a positive value.
  #define DEFAULT_SPINDLE_RPM_MAX 1000.0 // rpm
  #define DEFAULT_SPINDLE_RPM_MIN 0.0 // rpm
  #define DEFAULT_STEP_PULSE_MICROSECONDS 10
  #define DEFAULT_STEPPING_INVERT_MASK 0
  #define DEFAULT_DIRECTION_INVERT_MASK 0
  #define DEFAULT_STEPPER_IDLE_LOCK_TIME 25 // msec (0-254, 255 keeps steppers enabled)
  #define DEFAULT_STATUS_REPORT_MASK 1 // MPos enabled
  #define DEFAULT_JUNCTION_DEVIATION 0.01 // mm
  #define DEFAULT_ARC_TOLERANCE 0.002 // mm
  #define DEFAULT_REPORT_INCHES 0 // false
  #define DEFAULT_INVERT_ST_ENABLE 0 // false
  #define DEFAULT_INVERT_LIMIT_PINS 0 // false
  #define DEFAULT_SOFT_LIMIT_ENABLE 0 // false
  #define DEFAULT_HARD_LIMIT_ENABLE 0  // false
  #define DEFAULT_INVERT_PROBE_PIN 0 // false
  #define DEFAULT_LASER_MODE 0 // false
  #define DEFAULT_HOMING_ENABLE 0  // false
  #define DEFAULT_HOMING_DIR_MASK 0 // move positive dir
  #define DEFAULT_HOMING_FEED_RATE 25.0 // mm/min
  #define DEFAULT_HOMING_SEEK_RATE 500.0 // mm/min
  #define DEFAULT_HOMING_DEBOUNCE_DELAY 250 // msec (0-65k)
  #define DEFAULT_HOMING_PULLOFF 1.0 // mm
#endif

#ifdef DEFAULT_CNC3020
#define DEFAULT_X_STEPS_PER_MM 400.0f
#define DEFAULT_Y_STEPS_PER_MM 400.0f
#define DEFAULT_Z_STEPS_PER_MM 400.0f
#define DEFAULT_X_MAX_RATE 600.0f // mm/min
#define DEFAULT_Y_MAX_RATE 600.0f // mm/min
#define DEFAULT_Z_MAX_RATE 600.0f // mm/min
#define DEFAULT_X_ACCELERATION (45.0f*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
#define DEFAULT_Y_ACCELERATION (45.0f*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
#define DEFAULT_Z_ACCELERATION (45.0f*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
#define DEFAULT_X_MAX_TRAVEL 200.0f // mm NOTE: Must be a positive value.
#define DEFAULT_Y_MAX_TRAVEL 300.0f // mm NOTE: Must be a positive value.
#define DEFAULT_Z_MAX_TRAVEL 50.0f // mm NOTE: Must be a positive value.
#define DEFAULT_SPINDLE_RPM_MAX 100.0f // rpm
#define DEFAULT_SPINDLE_RPM_MIN 0.0f // rpm
#define DEFAULT_STEP_PULSE_MICROSECONDS 10
#define DEFAULT_STEPPING_INVERT_MASK 7
#define DEFAULT_DIRECTION_INVERT_MASK 7
#define DEFAULT_STEPPER_IDLE_LOCK_TIME 0 // msec (0-254, 255 keeps steppers enabled)
#define DEFAULT_STATUS_REPORT_MASK 1 // MPos enabled
#define DEFAULT_JUNCTION_DEVIATION 0.01f // mm
#define DEFAULT_ARC_TOLERANCE 0.002f // mm
#define DEFAULT_REPORT_INCHES 0 // false
#define DEFAULT_INVERT_ST_ENABLE 0 // false
#define DEFAULT_INVERT_LIMIT_PINS 0 // false
#define DEFAULT_SOFT_LIMIT_ENABLE 0 // false
#define DEFAULT_HARD_LIMIT_ENABLE 0  // false
#define DEFAULT_INVERT_PROBE_PIN 0 // false
#define DEFAULT_LASER_MODE 0 // false
#define DEFAULT_HOMING_ENABLE 1  // false
#define DEFAULT_HOMING_DIR_MASK 3 // move positive dir
#define DEFAULT_HOMING_FEED_RATE 25.0f // mm/min
#define DEFAULT_HOMING_SEEK_RATE 500.0f // mm/min
#define DEFAULT_HOMING_DEBOUNCE_DELAY 250 // msec (0-65k)
#define DEFAULT_HOMING_PULLOFF 1.5f // mm
#endif

#ifdef ABC_AXIS_EXAMPLE
		#define DEFAULT_X_STEPS_PER_MM 5120.0f					// X axis step per mm // 400
		#define DEFAULT_Y_STEPS_PER_MM 5120.0f					// Y axis step per mm
		#define DEFAULT_Z_STEPS_PER_MM 5120.0f					// Z axis step per mm
#if defined(AA_AXIS) || defined(AB_AXIS) || defined(ABC_AXIS)
		#define DEFAULT_A_STEPS_PER_MM 5120.0f					// A axis step per mm
#endif
#if defined(AB_AXIS) || defined(ABC_AXIS)
		#define DEFAULT_B_STEPS_PER_MM 5120.0f					// B axis step per mm
#endif
#if defined(ABC_AXIS)
		#define DEFAULT_C_STEPS_PER_MM 5120.0f					// C axis step per mm
#endif
		#define DEFAULT_X_MAX_RATE 2000.0f 						// X axis mm/min // 600
		#define DEFAULT_Y_MAX_RATE 2000.0f 						// Y axis mm/min
		#define DEFAULT_Z_MAX_RATE 2000.0f 						// Z axis mm/min
#if defined(AA_AXIS) || defined(AB_AXIS) || defined(ABC_AXIS)
		#define DEFAULT_A_MAX_RATE 2000.0f 						// A axis mm/min
#endif
#if defined(AB_AXIS) || defined(ABC_AXIS)
		#define DEFAULT_B_MAX_RATE 2000.0f 						// B axis mm/min
#endif
#if defined(ABC_AXIS)
		#define DEFAULT_C_MAX_RATE 2000.0f 						// C axis mm/min
#endif
		#define DEFAULT_X_ACCELERATION (8000.0f*60*60) 			// X axis 10*60*60 mm/min^2 = 10 mm/sec^2 // 45
		#define DEFAULT_Y_ACCELERATION (8000.0f*60*60) 			// Y axis 10*60*60 mm/min^2 = 10 mm/sec^2
		#define DEFAULT_Z_ACCELERATION (8000.0f*60*60) 			// Z axis 10*60*60 mm/min^2 = 10 mm/sec^2
#if defined(AA_AXIS) || defined(AB_AXIS) || defined(ABC_AXIS)
		#define DEFAULT_A_ACCELERATION (8000.0f*60*60) 			// A axis 10*60*60 mm/min^2 = 10 mm/sec^2
#endif
#if defined(AB_AXIS) || defined(ABC_AXIS)
		#define DEFAULT_B_ACCELERATION (8000.0f*60*60) 			// B axis 10*60*60 mm/min^2 = 10 mm/sec^2
#endif
#if defined(ABC_AXIS)
		#define DEFAULT_C_ACCELERATION (8000.0f*60*60) 			// C axis 10*60*60 mm/min^2 = 10 mm/sec^2
#endif
		#define DEFAULT_X_MAX_TRAVEL 280.0f 					// X axis mm
		#define DEFAULT_Y_MAX_TRAVEL 280.0f 					// Y axis mm
		#define DEFAULT_Z_MAX_TRAVEL 280.0f 					// Z axis mm
#if defined(AA_AXIS) || defined(AB_AXIS) || defined(ABC_AXIS)
		#define DEFAULT_A_MAX_TRAVEL 280.0f 					// A axis mm
#endif
#if defined(AB_AXIS) || defined(ABC_AXIS)
		#define DEFAULT_B_MAX_TRAVEL 280.0f 					// B axis mm
#endif
#if defined(ABC_AXIS)
		#define DEFAULT_C_MAX_TRAVEL 360.0f 					// C axis mm
#endif
		#define DEFAULT_SPINDLE_RPM_MAX 1000.0f 				// rpm
		#define DEFAULT_SPINDLE_RPM_MIN 0.0f 					// rpm
		#define DEFAULT_STEP_PULSE_MICROSECONDS 6 				// usec
		#define DEFAULT_STEPPING_INVERT_MASK 0x0
		#define DEFAULT_DIRECTION_INVERT_MASK 0x1F
		#define DEFAULT_STEPPER_IDLE_LOCK_TIME 0 				// msec (0-254, 255 keeps steppers enabled)
		#define DEFAULT_STATUS_REPORT_MASK 0 					// 1 - MPos enabled, 0 - WPos enabled
		#define DEFAULT_JUNCTION_DEVIATION 0.01f 				// mm
		#define DEFAULT_ARC_TOLERANCE 0.002f 					// mm
		#define DEFAULT_REPORT_INCHES 0
		#define DEFAULT_INVERT_ST_ENABLE 1
		#define DEFAULT_INVERT_LIMIT_PINS 1
		#define DEFAULT_SOFT_LIMIT_ENABLE 0
		#define DEFAULT_HARD_LIMIT_ENABLE 0
		#define DEFAULT_INVERT_PROBE_PIN 0
		#define DEFAULT_LASER_MODE 0
		#define DEFAULT_HOMING_ENABLE 0
		#define DEFAULT_HOMING_DIR_MASK 0x1f
		#define DEFAULT_HOMING_FEED_RATE 10.0f 					// mm/min
		#define DEFAULT_HOMING_SEEK_RATE 100.0f 				// mm/min
		#define DEFAULT_HOMING_DEBOUNCE_DELAY 250 				// msec (0-65k)
		#define DEFAULT_HOMING_PULLOFF 1.5f 					// mm
#endif

#endif
