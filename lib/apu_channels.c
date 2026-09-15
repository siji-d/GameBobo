#include <apu.h>


u8 duty_table[4] = {
    0b00000001, //12.5%
    0b10000001, //25%
    0b10000111, //50%
    0b01111110  //75%
};

u8 noise_divisors[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };
u8 wave_shift[4] = { 4, 0, 1, 2 };

u16 ch1_period() { return (NR14_PERIOD_HI << 8) | NR13_PERIOD_LO; }
u16 ch2_period() { return (NR24_PERIOD_HI << 8) | NR23_PERIOD_LO; }
u16 ch3_period() { return (NR34_PERIOD_HI << 8) | NR33_PERIOD_LO; }

/*
CHANNEL 1 - PULSE W/ SWEEP
*/

u16 ch1_sweep_calc_freq() {
    apuContext *ctx = get_apu_context();

    if (NR10_SWEEP_DIR) {
        ctx->ch1_sweep_negate_used = true;
    }

    u16 delta = ctx->ch1_shadow_freq >> NR10_SWEEP_STEP;
    u16 new_freq = NR10_SWEEP_DIR ? (ctx->ch1_shadow_freq - delta)
                                  : (ctx->ch1_shadow_freq + delta);

    /* Overflow disables the channel permanently until re-triggered. */
    if (new_freq > 2047) {
        ctx->ch1_enabled = false;
    }

    return new_freq;
}

void ch1_clock_sweep() {
    apuContext *ctx = get_apu_context();

    if (ctx->ch1_sweep_timer > 0) {
        ctx->ch1_sweep_timer--;
    }

    if (ctx->ch1_sweep_timer != 0) {
        return;
    }

    /* A pace of 0 reloads as 8 but performs no frequency update. */
    ctx->ch1_sweep_timer = NR10_SWEEP_PACE ? NR10_SWEEP_PACE : 8;

    if (!ctx->ch1_sweep_enabled || NR10_SWEEP_PACE == 0) {
        return;
    }

    u16 new_freq = ch1_sweep_calc_freq();

    if (new_freq <= 2047 && NR10_SWEEP_STEP > 0) {
        ctx->ch1_shadow_freq = new_freq;

        AREG(AUD_NR13) = new_freq & 0xFF;
        AREG(AUD_NR14) = (AREG(AUD_NR14) & 0xF8) | ((new_freq >> 8) & 0x7);

        /* Spec: a second overflow check runs immediately, result discarded. */
        ch1_sweep_calc_freq();
    }
}

void ch1_trigger() {
    apuContext *ctx = get_apu_context();

    ctx->ch1_enabled = true;
    
    bool was_zero = (ctx->ch1_length_timer == 0);
    if (was_zero) {
        ctx->ch1_length_timer = 64;
    }

    // Extra-clock quirk: if length is already enabled, the counter was
    // just reloaded from 0, and the frame sequencer's *next* step won't
    // clock length, the reload is immediately clocked once.
    if (was_zero && NR14_LEN_ENABLE && (ctx->frame_seq_step & 1)) {
        ctx->ch1_length_timer--;
        if (ctx->ch1_length_timer == 0) {
            ctx->ch1_enabled = false;
        }
    }

    ctx->ch1_freq_timer = (2048 - ch1_period()) * 4;

    ctx->ch1_volume = NR12_INIT_VOL;
    ctx->ch1_env_period_timer = NR12_ENV_PACE;
    ctx->ch1_env_running = true;

    ctx->ch1_shadow_freq = ch1_period();
    ctx->ch1_sweep_timer = NR10_SWEEP_PACE ? NR10_SWEEP_PACE : 8;
    ctx->ch1_sweep_enabled = (NR10_SWEEP_PACE != 0) || (NR10_SWEEP_STEP != 0);

    if (NR10_SWEEP_STEP > 0) {
        ch1_sweep_calc_freq(); /* immediate overflow check on trigger */
    }

    /* Triggering a channel whose DAC is off leaves it disabled. */
    if (!NR12_DAC_ENABLE) {
        ctx->ch1_enabled = false;
    }
}

void ch1_step() {
    apuContext *ctx = get_apu_context();

    if (ctx->ch1_freq_timer > 0) {
        ctx->ch1_freq_timer--;
    }

    if (ctx->ch1_freq_timer == 0) {
        ctx->ch1_freq_timer = (2048 - ch1_period()) * 4;
        ctx->ch1_duty_pos = (ctx->ch1_duty_pos + 1) & 0x7;
    }
}

u8 ch1_output() {
    apuContext *ctx = get_apu_context();

    if (!ctx->ch1_enabled || !NR12_DAC_ENABLE) {
        return 0;
    }

    u8 bit = (duty_table[NR11_DUTY] >> (7 - ctx->ch1_duty_pos)) & 1;
    return bit ? ctx->ch1_volume : 0;
}

/*
CHANNEL 2 - PULSE
*/

void ch2_trigger() {
    apuContext *ctx = get_apu_context();

    ctx->ch2_enabled = true;

    bool was_zero = (ctx->ch2_length_timer == 0);
    if (was_zero) {
        ctx->ch2_length_timer = 64;
    }
    if (was_zero && NR24_LEN_ENABLE && (ctx->frame_seq_step & 1)) {
        ctx->ch2_length_timer--;
        if (ctx->ch2_length_timer == 0) {
            ctx->ch2_enabled = false;
        }
    }
    ctx->ch2_freq_timer = (2048 - ch2_period()) * 4;

    ctx->ch2_volume = NR22_INIT_VOL;
    ctx->ch2_env_period_timer = NR22_ENV_PACE;
    ctx->ch2_env_running = true;

    if (!NR22_DAC_ENABLE) {
        ctx->ch2_enabled = false;
    }
}

void ch2_step() {
    apuContext *ctx = get_apu_context();

    if (ctx->ch2_freq_timer > 0) {
        ctx->ch2_freq_timer--;
    }

    if (ctx->ch2_freq_timer == 0) {
        ctx->ch2_freq_timer = (2048 - ch2_period()) * 4;
        ctx->ch2_duty_pos = (ctx->ch2_duty_pos + 1) & 0x7;
    }
}

u8 ch2_output() {
    apuContext *ctx = get_apu_context();

    if (!ctx->ch2_enabled || !NR22_DAC_ENABLE) {
        return 0;
    }

    u8 bit = (duty_table[NR21_DUTY] >> (7 - ctx->ch2_duty_pos)) & 1;
    return bit ? ctx->ch2_volume : 0;
}

/*
CHANNEL 3 - WAVE
*/

void ch3_trigger() {
    apuContext *ctx = get_apu_context();

    if (ctx->ch3_enabled) {
        u8 pos_byte = ctx->ch3_wave_pos / 2;
        if (pos_byte < 4) {
            ctx->wave_ram[0] = ctx->wave_ram[pos_byte];
        } else {
            u8 block = pos_byte & ~0x3;
            for (int i = 0; i < 4; i++) {
                ctx->wave_ram[i] = ctx->wave_ram[block + i];
            }
        }
    }

    ctx->ch3_enabled = true;

    bool was_zero = (ctx->ch3_length_timer == 0);
    if (was_zero) {
        ctx->ch3_length_timer = 256;
    }
    if (was_zero && NR34_LEN_ENABLE && (ctx->frame_seq_step & 1)) {
        ctx->ch3_length_timer--;
        if (ctx->ch3_length_timer == 0) {
            ctx->ch3_enabled = false;
        }
    }

    ctx->ch3_freq_timer = (2048 - ch3_period()) * 2;
    ctx->ch3_wave_pos = 0;

    if (!NR30_DAC_ENABLE) {
        ctx->ch3_enabled = false;
    }
}

void ch3_step() {
    apuContext *ctx = get_apu_context();

    ctx->ch3_wave_access_window = false;
    
    if (ctx->ch3_freq_timer > 0) {
        ctx->ch3_freq_timer--;
    }

    if (ctx->ch3_freq_timer == 0) {
        ctx->ch3_freq_timer = (2048 - ch3_period()) * 2;
        ctx->ch3_wave_pos = (ctx->ch3_wave_pos + 1) & 0x1F;

        u8 byte = ctx->wave_ram[ctx->ch3_wave_pos / 2];
        ctx->ch3_sample_buffer = (ctx->ch3_wave_pos & 1) ? (byte & 0x0F) : (byte >> 4);
        
        ctx->ch3_wave_access_window = true;
    }
}

u8 ch3_output() {
    apuContext *ctx = get_apu_context();

    if (!ctx->ch3_enabled || !NR30_DAC_ENABLE) {
        return 0;
    }

    return ctx->ch3_sample_buffer >> wave_shift[NR32_OUTPUT_LVL];
}

/*
CHANNEL 4 - NOISE
*/

void ch4_trigger() {
    apuContext *ctx = get_apu_context();

    ctx->ch4_enabled = true;

    bool was_zero = (ctx->ch4_length_timer == 0);
    if (was_zero) {
        ctx->ch4_length_timer = 64;
    }
    if (was_zero && NR44_LEN_ENABLE && (ctx->frame_seq_step & 1)) {
        ctx->ch4_length_timer--;
        if (ctx->ch4_length_timer == 0) {
            ctx->ch4_enabled = false;
        }
    }

    ctx->ch4_freq_timer = noise_divisors[NR43_DIVISOR] << NR43_CLOCK_SHIFT;

    ctx->ch4_volume = NR42_INIT_VOL;
    ctx->ch4_env_period_timer = NR42_ENV_PACE;
    ctx->ch4_env_running = true;

    ctx->ch4_lfsr = 0x7FFF; /* all bits set on trigger */

    if (!NR42_DAC_ENABLE) {
        ctx->ch4_enabled = false;
    }
}

void ch4_step() {
    apuContext *ctx = get_apu_context();

    if (ctx->ch4_freq_timer > 0) {
        ctx->ch4_freq_timer--;
    }

    if (ctx->ch4_freq_timer == 0) {
        ctx->ch4_freq_timer = noise_divisors[NR43_DIVISOR] << NR43_CLOCK_SHIFT;

        u8 xor_result = (ctx->ch4_lfsr & 1) ^ ((ctx->ch4_lfsr >> 1) & 1);

        ctx->ch4_lfsr >>= 1;
        ctx->ch4_lfsr |= (xor_result << 14);

        /* Width mode 1: also feed the result into bit 6, shortening the
         * sequence to 7 bits and giving the characteristic metallic tone. */
        if (NR43_WIDTH) {
            ctx->ch4_lfsr &= ~(1 << 6);
            ctx->ch4_lfsr |= (xor_result << 6);
        }
    }
}

u8 ch4_output() {
    apuContext *ctx = get_apu_context();

    if (!ctx->ch4_enabled || !NR42_DAC_ENABLE) {
        return 0;
    }

    /* Output is the inverted low bit of the LFSR. */
    return (~ctx->ch4_lfsr & 1) ? ctx->ch4_volume : 0;
}