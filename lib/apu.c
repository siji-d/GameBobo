#include <apu.h>

static apuContext ctx;

apuContext *get_apu_context() {
    return &ctx;
}

static float sample_buffer[APU_BUFFER_SIZE * 2];
static int buffer_write_pos = 0;
static int buffer_read_pos = 0;
static int buffered_frames = 0;

#define T_CYCLES_PER_SAMPLE ((double)4194304.0 / (double)APU_SAMPLE_RATE)
#define FRAME_SEQ_PERIOD 8192

static const u8 read_masks[0x17] = {
    [AUD_NR10] = 0x80, [AUD_NR11] = 0x3F, [AUD_NR12] = 0x00,
    [AUD_NR13] = 0xFF, [AUD_NR14] = 0xBF,
    [0x05]     = 0xFF, /* FF15, unused */
    [AUD_NR21] = 0x3F, [AUD_NR22] = 0x00,
    [AUD_NR23] = 0xFF, [AUD_NR24] = 0xBF,
    [AUD_NR30] = 0x7F, [AUD_NR31] = 0xFF, [AUD_NR32] = 0x9F,
    [AUD_NR33] = 0xFF, [AUD_NR34] = 0xBF,
    [0x0F]     = 0xFF, /* FF1F, unused */
    [AUD_NR41] = 0xFF, [AUD_NR42] = 0x00, [AUD_NR43] = 0x00,
    [AUD_NR44] = 0xBF,
    [AUD_NR50] = 0x00, [AUD_NR51] = 0x00, [AUD_NR52] = 0x70
};

void apu_init() {
    memset(&ctx, 0, sizeof(ctx));

    AREG(AUD_NR10) = 0x80;
    AREG(AUD_NR11) = 0xBF;
    AREG(AUD_NR12) = 0xF3;
    AREG(AUD_NR14) = 0xBF;
    AREG(AUD_NR21) = 0x3F;
    AREG(AUD_NR22) = 0x00;
    AREG(AUD_NR24) = 0xBF;
    AREG(AUD_NR30) = 0x7F;
    AREG(AUD_NR31) = 0xFF;
    AREG(AUD_NR32) = 0x9F;
    AREG(AUD_NR34) = 0xBF;
    AREG(AUD_NR41) = 0xFF;
    AREG(AUD_NR42) = 0x00;
    AREG(AUD_NR43) = 0x00;
    AREG(AUD_NR44) = 0xBF;
    AREG(AUD_NR50) = 0x77;
    AREG(AUD_NR51) = 0xF3;
    AREG(AUD_NR52) = 0xF1;

    ctx.ch4_lfsr = 0x7FFF;

    buffer_write_pos = 0;
    buffer_read_pos = 0;
    buffered_frames = 0;
}


void clock_length_counters() {
    if (NR14_LEN_ENABLE && ctx.ch1_length_timer > 0) {
        if (--ctx.ch1_length_timer == 0) ctx.ch1_enabled = false;
    }
    if (NR24_LEN_ENABLE && ctx.ch2_length_timer > 0) {
        if (--ctx.ch2_length_timer == 0) ctx.ch2_enabled = false;
    }
    if (NR34_LEN_ENABLE && ctx.ch3_length_timer > 0) {
        if (--ctx.ch3_length_timer == 0) ctx.ch3_enabled = false;
    }
    if (NR44_LEN_ENABLE && ctx.ch4_length_timer > 0) {
        if (--ctx.ch4_length_timer == 0) ctx.ch4_enabled = false;
    }
}

void clock_single_envelope(bool *running, u8 *volume, u8 *period_timer, u8 pace, bool dir_up) {
    if (pace == 0 || !*running) {
        return; // pace 0 disables the envelope entirely
    }

    if (*period_timer > 0) {
        (*period_timer)--;
    }

    if (*period_timer == 0) {
        *period_timer = pace;

        if (dir_up && *volume < 15) {
            (*volume)++;
        } else if (!dir_up && *volume > 0) {
            (*volume)--;
        }

        //Envelope stops once it saturates at either end.
        if (*volume == 0 || *volume == 15) {
            *running = false;
        }
    }
}

void clock_envelopes() {
    clock_single_envelope(&ctx.ch1_env_running, &ctx.ch1_volume,
                          &ctx.ch1_env_period_timer, NR12_ENV_PACE, NR12_ENV_DIR);
    clock_single_envelope(&ctx.ch2_env_running, &ctx.ch2_volume,
                          &ctx.ch2_env_period_timer, NR22_ENV_PACE, NR22_ENV_DIR);
    clock_single_envelope(&ctx.ch4_env_running, &ctx.ch4_volume,
                          &ctx.ch4_env_period_timer, NR42_ENV_PACE, NR42_ENV_DIR);
}

void frame_sequencer_step() {
    switch (ctx.frame_seq_step) {
        case 0: clock_length_counters(); break;
        case 2: clock_length_counters(); ch1_clock_sweep(); break;
        case 4: clock_length_counters(); break;
        case 6: clock_length_counters(); ch1_clock_sweep(); break;
        case 7: clock_envelopes(); break;
        default: break; /* 1, 3, 5 idle */
    }

    ctx.frame_seq_step = (ctx.frame_seq_step + 1) & 0x7;
}

float dac_output(u8 digital, bool dac_enabled) {
    if (!dac_enabled) {
        return 0.0f;
    }
    return ((float)digital / 7.5f) - 1.0f;
}

// models a capacitor HPF
float high_pass(float *capacitor, float in, bool dacs_enabled) {
    if (!dacs_enabled) {
        return 0.0f;
    }

    float out = in - *capacitor;
    /* 0.999958 ^ (4194304 / sample_rate) */
    *capacitor = in - out * 0.996f;
    return out;
}

void mix_and_buffer_sample() {
    float c1 = dac_output(ch1_output(), NR12_DAC_ENABLE);
    float c2 = dac_output(ch2_output(), NR22_DAC_ENABLE);
    float c3 = dac_output(ch3_output(), NR30_DAC_ENABLE);
    float c4 = dac_output(ch4_output(), NR42_DAC_ENABLE);

    float left = 0.0f;
    float right = 0.0f;

    if (NR51_CH_LEFT(1))  left  += c1;
    if (NR51_CH_LEFT(2))  left  += c2;
    if (NR51_CH_LEFT(3))  left  += c3;
    if (NR51_CH_LEFT(4))  left  += c4;

    if (NR51_CH_RIGHT(1)) right += c1;
    if (NR51_CH_RIGHT(2)) right += c2;
    if (NR51_CH_RIGHT(3)) right += c3;
    if (NR51_CH_RIGHT(4)) right += c4;

    // Average the four channels, then apply master volume (0-7 -> 1/8-8/8).
    left  = (left  / 4.0f) * ((float)(NR50_VOL_LEFT  + 1) / 8.0f);
    right = (right / 4.0f) * ((float)(NR50_VOL_RIGHT + 1) / 8.0f);

    bool any_dac = NR12_DAC_ENABLE || NR22_DAC_ENABLE ||
                   NR30_DAC_ENABLE || NR42_DAC_ENABLE;

    left  = high_pass(&ctx.capacitor_left,  left,  any_dac);
    right = high_pass(&ctx.capacitor_right, right, any_dac);

    ctx.output_left = left;
    ctx.output_right = right;

    /* Drop the sample if the consumer has fallen behind rather than
    overwriting unread data. */
    if (buffered_frames >= APU_BUFFER_SIZE) {
        return;
    }

    sample_buffer[buffer_write_pos * 2]     = left;
    sample_buffer[buffer_write_pos * 2 + 1] = right;

    buffer_write_pos = (buffer_write_pos + 1) % APU_BUFFER_SIZE;
    buffered_frames++;
}

void apu_tick() {
    if (!NR52_AUDIO_ON) {
        return;
    }

    ch1_step();
    ch2_step();
    ch3_step();
    ch4_step();

    ctx.frame_seq_div_counter++;
    if (ctx.frame_seq_div_counter >= FRAME_SEQ_PERIOD) {
        ctx.frame_seq_div_counter = 0;
        frame_sequencer_step();
    }

    ctx.sample_accumulator += 1.0;
    if (ctx.sample_accumulator >= T_CYCLES_PER_SAMPLE) {
        ctx.sample_accumulator -= T_CYCLES_PER_SAMPLE;
        mix_and_buffer_sample();
    }
}

u8 apu_read(u16 addr) {
    u8 offset = addr - 0xFF10;

    if (offset >= 0x17) {
        return 0xFF;
    }

    if (offset == AUD_NR52) {
        /* Bits 0-3 report live channel status, not what was written. */
        u8 status = (AREG(AUD_NR52) & 0x80);
        if (ctx.ch1_enabled) status |= 0x01;
        if (ctx.ch2_enabled) status |= 0x02;
        if (ctx.ch3_enabled) status |= 0x04;
        if (ctx.ch4_enabled) status |= 0x08;
        return status | read_masks[AUD_NR52];
    }

    return AREG(offset) | read_masks[offset];
}

void apu_write(u16 addr, u8 val) {
    u8 offset = addr - 0xFF10;

    if (offset >= 0x17) {
        return;
    }

    if (!NR52_AUDIO_ON && offset != AUD_NR52) {
        if (offset == AUD_NR11) { AREG(offset) = (AREG(offset) & 0xC0) | (val & 0x3F); ctx.ch1_length_timer = 64 - (val & 0x3F); }
        else if (offset == AUD_NR21) { AREG(offset) = (AREG(offset) & 0xC0) | (val & 0x3F); ctx.ch2_length_timer = 64 - (val & 0x3F); }
        else if (offset == AUD_NR31) { AREG(offset) = val; ctx.ch3_length_timer = 256 - val; }
        else if (offset == AUD_NR41) { AREG(offset) = (AREG(offset) & 0xC0) | (val & 0x3F); ctx.ch4_length_timer = 64 - (val & 0x3F); }
        return;
    }

    switch (offset) {
        case AUD_NR52: {
            bool was_on = NR52_AUDIO_ON;
            bool now_on = (val & 0x80) != 0;

            AREG(AUD_NR52) = (AREG(AUD_NR52) & 0x7F) | (val & 0x80);

            if (was_on && !now_on) {
                u8 len11 = AREG(AUD_NR11) & 0x3F;
                u8 len21 = AREG(AUD_NR21) & 0x3F;
                u8 len31 = AREG(AUD_NR31);
                u8 len41 = AREG(AUD_NR41) & 0x3F;

                for (int i = 0; i < 0x17; i++) AREG(i) = 0;

                AREG(AUD_NR11) = len11;
                AREG(AUD_NR21) = len21;
                AREG(AUD_NR31) = len31;
                AREG(AUD_NR41) = len41;

                ctx.ch1_enabled = ctx.ch2_enabled = false;
                ctx.ch3_enabled = ctx.ch4_enabled = false;
            } else if (!was_on && now_on) {
                ctx.frame_seq_step = 0;
                ctx.frame_seq_div_counter = 0;
                ctx.ch1_duty_pos = ctx.ch2_duty_pos = 0;
                ctx.ch3_wave_pos = 0;
            }
        } return;

        /* Length loads take effect immediately, independent of trigger. */
        case AUD_NR10: {
            bool old_negate = NR10_SWEEP_DIR;
            AREG(offset) = val;
            bool new_negate = (val >> 3) & 1;
            if (old_negate && !new_negate && ctx.ch1_sweep_negate_used) {
                ctx.ch1_enabled = false;
            }
        } return;
        
        case AUD_NR11:
            AREG(offset) = val;
            ctx.ch1_length_timer = 64 - (val & 0x3F);
            return;

        case AUD_NR21:
            AREG(offset) = val;
            ctx.ch2_length_timer = 64 - (val & 0x3F);
            return;

        case AUD_NR31:
            AREG(offset) = val;
            ctx.ch3_length_timer = 256 - val;
            return;

        case AUD_NR41:
            AREG(offset) = val;
            ctx.ch4_length_timer = 64 - (val & 0x3F);
            return;

        /* Clearing a DAC (upper 5 bits of NRx2, or NR30 bit 7) immediately
         * disables the channel. */
        case AUD_NR12:
            AREG(offset) = val;
            if ((val & 0xF8) == 0) ctx.ch1_enabled = false;
            return;

        case AUD_NR22:
            AREG(offset) = val;
            if ((val & 0xF8) == 0) ctx.ch2_enabled = false;
            return;

        case AUD_NR42:
            AREG(offset) = val;
            if ((val & 0xF8) == 0) ctx.ch4_enabled = false;
            return;

        case AUD_NR30:
            AREG(offset) = val;
            if ((val & 0x80) == 0) ctx.ch3_enabled = false;
            return;

        /* NRx4 writes may carry a trigger in bit 7. */
        case AUD_NR14: {
            bool was_len = NR14_LEN_ENABLE;
            AREG(offset) = val;
            if (!was_len && NR14_LEN_ENABLE && (ctx.frame_seq_step & 1) && ctx.ch1_length_timer > 0) {
                ctx.ch1_length_timer--;
                if (ctx.ch1_length_timer == 0 && !(val & 0x80)) ctx.ch1_enabled = false;
            }
            if (val & 0x80) ch1_trigger();
        } return;

        case AUD_NR24: {
            bool was_len = NR24_LEN_ENABLE;
            AREG(offset) = val;
            if (!was_len && NR24_LEN_ENABLE && (ctx.frame_seq_step & 1) && ctx.ch2_length_timer > 0) {
                ctx.ch2_length_timer--;
                if (ctx.ch2_length_timer == 0 && !(val & 0x80)) ctx.ch2_enabled = false;
            }
            if (val & 0x80) ch2_trigger();
        } return;

        case AUD_NR34: {
            bool was_len = NR34_LEN_ENABLE;
            AREG(offset) = val;
            if (!was_len && NR34_LEN_ENABLE && (ctx.frame_seq_step & 1) && ctx.ch3_length_timer > 0) {
                ctx.ch3_length_timer--;
                if (ctx.ch3_length_timer == 0 && !(val & 0x80)) ctx.ch3_enabled = false;
            }
            if (val & 0x80) ch3_trigger();
        } return;

        case AUD_NR44: {
            bool was_len = NR44_LEN_ENABLE;
            AREG(offset) = val;
            if (!was_len && NR44_LEN_ENABLE && (ctx.frame_seq_step & 1) && ctx.ch4_length_timer > 0) {
                ctx.ch4_length_timer--;
                if (ctx.ch4_length_timer == 0 && !(val & 0x80)) ctx.ch4_enabled = false;
            }
            if (val & 0x80) ch4_trigger();
        } return;

        default:
            AREG(offset) = val;
            return;
    }
}

u8 apu_wave_ram_read(u16 addr) {
    if (ctx.ch3_enabled) {
        if (ctx.ch3_wave_access_window) {
            return ctx.wave_ram[ctx.ch3_wave_pos / 2];
        }
        return 0xFF;
    }
    return ctx.wave_ram[addr - 0xFF30];
}

void apu_wave_ram_write(u16 addr, u8 val) {
    if (ctx.ch3_enabled) {
        if (ctx.ch3_wave_access_window) {
            ctx.wave_ram[ctx.ch3_wave_pos / 2] = val;
        }
        return;
    }
    ctx.wave_ram[addr - 0xFF30] = val;
}

int apu_samples_available() {
    return buffered_frames;
}

int apu_read_samples(float *out, int max_samples) {
    int count = (buffered_frames < max_samples) ? buffered_frames : max_samples;

    for (int i = 0; i < count; i++) {
        out[i * 2]     = sample_buffer[buffer_read_pos * 2];
        out[i * 2 + 1] = sample_buffer[buffer_read_pos * 2 + 1];
        buffer_read_pos = (buffer_read_pos + 1) % APU_BUFFER_SIZE;
    }

    buffered_frames -= count;
    return count;
}