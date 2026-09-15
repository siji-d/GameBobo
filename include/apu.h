#include <common.h>
#include <string.h>

typedef struct {
    u8 aud_regs[0x17];

    u8 wave_ram[16];
    
    u16 frame_seq_div_counter;
    u8  frame_seq_step;
    float capacitor_left;
    float capacitor_right;
    bool ch3_wave_access_window;

    /* Channel 1 (pulse + sweep) */
    bool ch1_enabled;
    bool ch1_dac_enabled;
    u16  ch1_freq_timer;
    u8   ch1_duty_pos;
    u8   ch1_length_timer;
    u8   ch1_volume;
    u8   ch1_env_period_timer;
    bool ch1_env_running;
    u16  ch1_shadow_freq;
    u8   ch1_sweep_timer;
    bool ch1_sweep_enabled;
    bool ch1_sweep_negate_used;

    /* Channel 2 (pulse) */
    bool ch2_enabled;
    bool ch2_dac_enabled;
    u16  ch2_freq_timer;
    u8   ch2_duty_pos;
    u8   ch2_length_timer;
    u8   ch2_volume;
    u8   ch2_env_period_timer;
    bool ch2_env_running;

    /* Channel 3 (wave) */
    bool ch3_enabled;
    bool ch3_dac_enabled;
    bool ch3_dac_powered;
    u16  ch3_freq_timer;
    u8   ch3_wave_pos;
    u16  ch3_length_timer;
    u8   ch3_sample_buffer;

    /* Channel 4 (noise) */
    bool ch4_enabled;
    bool ch4_dac_enabled;
    u16  ch4_freq_timer;
    u16  ch4_lfsr;
    u8   ch4_length_timer;
    u8   ch4_volume;
    u8   ch4_env_period_timer;
    bool ch4_env_running;

    //Sample downsampling accumulator (CPU clock -> audio sample rate)
    double sample_accumulator;

    //Latest mixed output, written by apu_tick, read by the audio backend
    float output_left;
    float output_right;

} apuContext;

apuContext *get_apu_context();

void apu_init();
void apu_tick();

u8 apu_read(u16 addr);
void apu_write(u16 addr, u8 val);

u8 apu_wave_ram_read(u16 addr);
void apu_wave_ram_write(u16 addr, u8 val);

void ch1_step();
void ch2_step();
void ch3_step();
void ch4_step();

void ch1_trigger();
void ch2_trigger();
void ch3_trigger();
void ch4_trigger();

u8 ch1_output();
u8 ch2_output();
u8 ch3_output();
u8 ch4_output();

u16 ch1_sweep_calc_freq();
void ch1_clock_sweep();

int  apu_samples_available();
int  apu_read_samples(float *out, int max_samples);

#define APU_SAMPLE_RATE 44100
#define APU_BUFFER_SIZE 8192


#define AUD_NR10 0x00
#define AUD_NR11 0x01
#define AUD_NR12 0x02
#define AUD_NR13 0x03
#define AUD_NR14 0x04

#define AUD_NR21 0x06
#define AUD_NR22 0x07
#define AUD_NR23 0x08
#define AUD_NR24 0x09

#define AUD_NR30 0x0A
#define AUD_NR31 0x0B
#define AUD_NR32 0x0C
#define AUD_NR33 0x0D
#define AUD_NR34 0x0E

#define AUD_NR41 0x10
#define AUD_NR42 0x11
#define AUD_NR43 0x12
#define AUD_NR44 0x13

#define AUD_NR50 0x14
#define AUD_NR51 0x15
#define AUD_NR52 0x16

#define AREG(offset) (get_apu_context()->aud_regs[offset])

/*
REGISTER BIT MACROS
*/

//NRx1 - Channel x(1,2) length timer and duty cycle
#define NR11_DUTY ((AREG(AUD_NR11) >> 6) & 0x3)
#define NR11_LEN_TIMER (AREG(AUD_NR11) & 0x3F)

#define NR21_DUTY ((AREG(AUD_NR21) >> 6) & 0x3)
#define NR21_LEN_TIMER (AREG(AUD_NR21) & 0x3F)

#define NR31_LEN_TIMER (AREG(AUD_NR31))

#define NR41_LEN_TIMER (AREG(AUD_NR41) & 0x3F)


//NRx2 - Channel x volume & envelope
#define NR12_INIT_VOL ((AREG(AUD_NR12) >> 4) & 0xF)
#define NR12_ENV_DIR ((AREG(AUD_NR12) >> 3) & 0x1)
#define NR12_ENV_PACE (AREG(AUD_NR12) & 0x7)
#define NR12_DAC_ENABLE ((AREG(AUD_NR12) & 0xF8) != 0)

#define NR22_INIT_VOL ((AREG(AUD_NR22) >> 4) & 0xF)
#define NR22_ENV_DIR ((AREG(AUD_NR22) >> 3) & 0x1)
#define NR22_ENV_PACE (AREG(AUD_NR22) & 0x7)
#define NR22_DAC_ENABLE ((AREG(AUD_NR22) & 0xF8) != 0)

#define NR32_OUTPUT_LVL ((AREG(AUD_NR32) >> 5) & 0x3)

#define NR42_INIT_VOL ((AREG(AUD_NR42) >> 4) & 0xF)
#define NR42_ENV_DIR ((AREG(AUD_NR42) >> 3) & 0x1)
#define NR42_ENV_PACE (AREG(AUD_NR42) & 0x7)
#define NR42_DAC_ENABLE ((AREG(AUD_NR42) & 0xF8) != 0)


//NRx3/NRx4 - Channel x period, control
#define NR13_PERIOD_LO (AREG(AUD_NR13)) //should not really be needed since this is write only but whatever
#define NR14_PERIOD_HI (AREG(AUD_NR14) & 0x7) //^^^
#define NR14_TRIGGER ((AREG(AUD_NR14) >> 7) & 0x1)
#define NR14_LEN_ENABLE ((AREG(AUD_NR14) >> 6) & 0x1)

#define NR23_PERIOD_LO (AREG(AUD_NR23)) //should not really be needed since this is write only but whatever
#define NR24_PERIOD_HI (AREG(AUD_NR24) & 0x7) //^^^
#define NR24_TRIGGER ((AREG(AUD_NR24) >> 7) & 0x1)
#define NR24_LEN_ENABLE ((AREG(AUD_NR24) >> 6) & 0x1)

#define NR33_PERIOD_LO (AREG(AUD_NR33))
#define NR34_PERIOD_HI (AREG(AUD_NR34) & 0x7)
#define NR34_TRIGGER ((AREG(AUD_NR34) >> 7) & 0x1)
#define NR34_LEN_ENABLE ((AREG(AUD_NR34) >> 6) & 0x1)

#define NR44_TRIGGER ((AREG(AUD_NR44) >> 7) & 0x1)
#define NR44_LEN_ENABLE ((AREG(AUD_NR44) >> 6) & 0x1)

//Channel 4 stuff
#define NR43_CLOCK_SHIFT ((AREG(AUD_NR43) >> 4) & 0xF)
#define NR43_WIDTH ((AREG(AUD_NR43) >> 3) & 0x1)
#define NR43_DIVISOR (AREG(AUD_NR43) & 0x7)


//SPECIAL FUNCTION REGISTERS
//NR10 - Channel 1 Sweep
#define NR10_SWEEP_PACE ((AREG(AUD_NR10) >> 4) & 0x7)
#define NR10_SWEEP_DIR ((AREG(AUD_NR10) >> 3) & 0x1)
#define NR10_SWEEP_STEP (AREG(AUD_NR10) & 0x7)

//NR30 - Channel 3 DAC Enable
#define NR30_DAC_ENABLE ((AREG(AUD_NR30) >> 7) & 0x1)

//NR50 - Master volume / VIN panning
#define NR50_VOL_LEFT    ((AREG(AUD_NR50) >> 4) & 0x7)
#define NR50_VOL_RIGHT   (AREG(AUD_NR50) & 0x7)

//NR51 - Channel panning (bit order: 0-3=CH1..CH4 right, 4-7=CH1..CH4 left)
#define NR51_CH_RIGHT(n) ((AREG(AUD_NR51) >> ((n) - 1)) & 0x1)
#define NR51_CH_LEFT(n) ((AREG(AUD_NR51) >> (((n) - 1) + 4)) & 0x1)

//NR52 - Audio Master Control
#define NR52_AUDIO_ON ((AREG(AUD_NR52) >> 7) & 0x1)
#define NR52_CH_STATUS(n) ((AREG(AUD_NR52) >> ((n) - 1)) & 0x1) //read only, doesn't enable/disable the respective channels