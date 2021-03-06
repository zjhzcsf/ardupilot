// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-

/// @file	AP_MotorsHeli.h
/// @brief	Motor control class for Traditional Heli

#ifndef __AP_MOTORS_HELI_H__
#define __AP_MOTORS_HELI_H__

#include <inttypes.h>

#include <AP_Common/AP_Common.h>
#include <AP_Math/AP_Math.h>            // ArduPilot Mega Vector/Matrix math Library
#include <RC_Channel/RC_Channel.h>      // RC Channel Library
#include "AP_Motors_Class.h"
#include "AP_MotorsHeli_RSC.h"

// maximum number of swashplate servos
#define AP_MOTORS_HELI_NUM_SWASHPLATE_SERVOS    3

// servo output rates
#define AP_MOTORS_HELI_SPEED_DEFAULT            125     // default servo update rate for helicopters
#define AP_MOTORS_HELI_SPEED_DIGITAL_SERVOS     125     // update rate for digital servos
#define AP_MOTORS_HELI_SPEED_ANALOG_SERVOS      125     // update rate for analog servos

// default swash min and max angles and positions
#define AP_MOTORS_HELI_SWASH_ROLL_MAX           2500
#define AP_MOTORS_HELI_SWASH_PITCH_MAX          2500
#define AP_MOTORS_HELI_COLLECTIVE_MIN           1250
#define AP_MOTORS_HELI_COLLECTIVE_MAX           1750
#define AP_MOTORS_HELI_COLLECTIVE_MID           1500

// swash min and max position while in stabilize mode (as a number from 0 ~ 100)
#define AP_MOTORS_HELI_MANUAL_COLLECTIVE_MIN    0
#define AP_MOTORS_HELI_MANUAL_COLLECTIVE_MAX    100

// swash min while landed or landing (as a number from 0 ~ 1000
#define AP_MOTORS_HELI_LAND_COLLECTIVE_MIN      0

// default main rotor speed (ch8 out) as a number from 0 ~ 1000
#define AP_MOTORS_HELI_RSC_SETPOINT             700

// default main rotor critical speed
#define AP_MOTORS_HELI_RSC_CRITICAL             500

// RSC output defaults
#define AP_MOTORS_HELI_RSC_IDLE_DEFAULT         0
#define AP_MOTORS_HELI_RSC_POWER_LOW_DEFAULT    200
#define AP_MOTORS_HELI_RSC_POWER_HIGH_DEFAULT   700

// default main rotor ramp up time in seconds
#define AP_MOTORS_HELI_RSC_RAMP_TIME            1       // 1 second to ramp output to main rotor ESC to full power (most people use exterrnal govenors so we can ramp up quickly)
#define AP_MOTORS_HELI_RSC_RUNUP_TIME           10      // 10 seconds for rotor to reach full speed

// flybar types
#define AP_MOTORS_HELI_NOFLYBAR                 0
#define AP_MOTORS_HELI_FLYBAR                   1

class AP_HeliControls;

/// @class      AP_MotorsHeli
class AP_MotorsHeli : public AP_Motors {
public:

    /// Constructor
    AP_MotorsHeli( uint16_t         loop_rate,
                   uint16_t         speed_hz = AP_MOTORS_HELI_SPEED_DEFAULT) :
        AP_Motors(loop_rate, speed_hz)
    {
        AP_Param::setup_object_defaults(this, var_info);

        // initialise flags
        _heliflags.landing_collective = 0;
        _heliflags.rotor_runup_complete = 0;
    };

    // init
    void Init();

    // set update rate to motors - a value in hertz
    // you must have setup_motors before calling this
    virtual void set_update_rate( uint16_t speed_hz ) = 0;

    // enable - starts allowing signals to be sent to motors
    virtual void enable() = 0;

    // output_min - sets servos to neutral point with motors stopped
    void output_min();

    // output_test - spin a motor at the pwm value specified
    //  motor_seq is the motor's sequence number from 1 to the number of motors on the frame
    //  pwm value is an actual pwm value that will be output, normally in the range of 1000 ~ 2000
    virtual void        output_test(uint8_t motor_seq, int16_t pwm) = 0;

    // slow_start - ignored by helicopters
    void                slow_start(bool true_false) {};

    //
    // heli specific methods
    //

    // parameter_check - returns true if helicopter specific parameters are sensible, used for pre-arm check
    bool parameter_check(bool display_msg) const;

    // has_flybar - returns true if we have a mechical flybar
    virtual bool has_flybar() const { return AP_MOTORS_HELI_NOFLYBAR; }

    // get_collective_mid - returns collective mid position as a number from 0 ~ 1000
    int16_t get_collective_mid() const { return  _collective_mid; }

    // get_collective_out - returns collective position from last output as a number from 0 ~ 1000
    int16_t get_collective_out() const { return _collective_out; }

    // set_collective_for_landing - limits collective from going too low if we know we are landed
    void set_collective_for_landing(bool landing) { _heliflags.landing_collective = landing; }

    // get_rsc_mode - gets the rotor speed control method (AP_MOTORS_HELI_RSC_MODE_CH8_PASSTHROUGH or AP_MOTORS_HELI_RSC_MODE_SETPOINT)
    uint8_t get_rsc_mode() const { return _rsc_mode; }

    // get_rsc_setpoint - gets contents of _rsc_setpoint parameter (0~1000)
    int16_t get_rsc_setpoint() const { return _rsc_setpoint; }

    // set_desired_rotor_speed - sets target rotor speed as a number from 0 ~ 1000
    virtual void set_desired_rotor_speed(int16_t desired_speed) = 0;

    // get_desired_rotor_speed - gets target rotor speed as a number from 0 ~ 1000
    virtual int16_t get_desired_rotor_speed() const = 0;

    // get_main_rotor_speed - gets estimated or measured main rotor speed
    virtual int16_t get_main_rotor_speed() const = 0;

    // return true if the main rotor is up to speed
    bool rotor_runup_complete() const { return _heliflags.rotor_runup_complete; }

    // rotor_speed_above_critical - return true if rotor speed is above that critical for flight
    virtual bool rotor_speed_above_critical() const = 0;

    // calculate_scalars - must be implemented by child classes
    virtual void calculate_scalars() = 0;

    // var_info for holding Parameter information
    static const struct AP_Param::GroupInfo var_info[];

    // set_delta_phase_angle for setting variable phase angle compensation and force recalculation of collective factors
    // ignored unless overloaded by child classes
    virtual void set_delta_phase_angle(int16_t angle){};

    // get_motor_mask - returns a bitmask of which outputs are being used for motors or servos (1 means being used)
    //  this can be used to ensure other pwm outputs (i.e. for servos) do not conflict
    virtual uint16_t get_motor_mask() = 0;

    // set_radio_passthrough used to pass radio inputs directly to outputs
    void set_radio_passthrough(int16_t radio_roll_input, int16_t radio_pitch_input, int16_t radio_throttle_input, int16_t radio_yaw_input);

    // reset_radio_passthrough used to reset all radio inputs to center
    void reset_radio_passthrough();

    // output - sends commands to the motors
    void    output();

    // supports_yaw_passthrough
    virtual bool supports_yaw_passthrough() const { return false; }

protected:

    // output - sends commands to the motors
    void        output_armed_stabilizing();
    void        output_armed_not_stabilizing();
    void        output_armed_zero_throttle();
    void        output_disarmed();

    // update_motor_controls - sends commands to motor controllers
    virtual void update_motor_control(RotorControlState state) = 0;

    // reset_flight_controls - resets all controls and scalars to flight status
    void reset_flight_controls();

    // update the throttle input filter
    void                update_throttle_filter();

    // move_actuators - moves swash plate and tail rotor
    virtual void move_actuators(int16_t roll_out, int16_t pitch_out, int16_t coll_in, int16_t yaw_out) = 0;

    // reset_swash_servo - free up swash servo for maximum movement
    static void reset_swash_servo(RC_Channel& servo);

    // init_outputs - initialise Servo/PWM ranges and endpoints
    virtual void init_outputs() = 0;

    // calculate_roll_pitch_collective_factors - calculate factors based on swash type and servo position
    virtual void calculate_roll_pitch_collective_factors() = 0;

    // flags bitmask
    struct heliflags_type {
        uint8_t landing_collective      : 1;    // true if collective is setup for landing which has much higher minimum
        uint8_t rotor_runup_complete    : 1;    // true if the rotors have had enough time to wind up
    } _heliflags;

    // parameters
    AP_Int16        _roll_max;                  // Maximum roll angle of the swash plate in centi-degrees
    AP_Int16        _pitch_max;                 // Maximum pitch angle of the swash plate in centi-degrees
    AP_Int16        _collective_min;            // Lowest possible servo position for the swashplate
    AP_Int16        _collective_max;            // Highest possible servo position for the swashplate
    AP_Int16        _collective_mid;            // Swash servo position corresponding to zero collective pitch (or zero lift for Assymetrical blades)
    AP_Int8         _servo_manual;              // Pass radio inputs directly to servos during set-up through mission planner
    AP_Int16        _rsc_setpoint;              // rotor speed when RSC mode is set to is enabledv
    AP_Int8         _rsc_mode;                  // Which main rotor ESC control mode is active
    AP_Int8         _rsc_ramp_time;             // Time in seconds for the output to the main rotor's ESC to reach full speed
    AP_Int8         _rsc_runup_time;            // Time in seconds for the main rotor to reach full speed.  Must be longer than _rsc_ramp_time
    AP_Int16        _land_collective_min;       // Minimum collective when landed or landing
    AP_Int16        _rsc_critical;              // Rotor speed below which flight is not possible
    AP_Int16        _rsc_idle_output;           // Rotor control output while at idle
    AP_Int16        _rsc_power_low;             // throttle value sent to throttle servo at zero collective pitch
    AP_Int16        _rsc_power_high;            // throttle value sent to throttle servo at maximum collective pitch

    // internal variables
    float           _rollFactor[AP_MOTORS_HELI_NUM_SWASHPLATE_SERVOS];
    float           _pitchFactor[AP_MOTORS_HELI_NUM_SWASHPLATE_SERVOS];
    float           _collectiveFactor[AP_MOTORS_HELI_NUM_SWASHPLATE_SERVOS];
    float           _roll_scaler = 1;                // scaler to convert roll input from radio (i.e. -4500 ~ 4500) to max roll range
    float           _pitch_scaler = 1;               // scaler to convert pitch input from radio (i.e. -4500 ~ 4500) to max pitch range
    float           _collective_scalar = 1;          // collective scalar to convert pwm form (i.e. 0 ~ 1000) passed in to actual servo range (i.e 1250~1750 would be 500)
    float           _main_rotor_power = 0;           // estimated main rotor power load, range 0-1.0f, used for RSC feedforward
    int16_t         _collective_out = 0;             // actual collective pitch value.  Required by the main code for calculating cruise throttle
    int16_t         _collective_mid_pwm = 0;         // collective mid parameter value converted to pwm form (i.e. 0 ~ 1000)
    int16_t         _delta_phase_angle = 0;          // phase angle dynamic compensation
    int16_t         _roll_radio_passthrough = 0;     // roll control PWM direct from radio, used for manual control
    int16_t         _pitch_radio_passthrough = 0;    // pitch control PWM direct from radio, used for manual control
    int16_t         _throttle_radio_passthrough = 0; // throttle control PWM direct from radio, used for manual control
    int16_t         _yaw_radio_passthrough = 0;      // yaw control PWM direct from radio, used for manual control
    int16_t         _collective_range = 0;           // maximum absolute collective pitch range (500 - 1000)
};

#endif  // AP_MOTORSHELI
