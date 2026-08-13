#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <emu.h>
#include <cpu.h>
#include <bus.h>
#include <instructions.h>


#define PROG_ADDR 0xC000
#define SAFE_SP   0xD000


extern cpuContext ctx;

static void reset_ctx() {
    memset(&ctx, 0, sizeof(ctx));
}

static instruction make_inst(inType type, addrMode mode, regType r1, regType r2, condType cond) {
    instruction inst = {type, mode, r1, r2, cond, 0};
    return inst;
}

static void write_bytes(u16 addr, u8 *bytes, int len) {
    for (int i = 0; i < len; i++) {
        bus_write(addr + i, bytes[i]);
    }
}

// Writes a program at PROG_ADDR, points PC at it, and runs exactly one
// real cpu_step() (fetch_instruction + fetch_data + execute).
static void run_program(u8 *bytes, int len) {
    write_bytes(PROG_ADDR, bytes, len);
    ctx.regs.pc = PROG_ADDR;
    cpu_step();
}


/* ---------------------------------------------------------- */
/* Flag macros                                                */
/* ---------------------------------------------------------- */

START_TEST(test_bit_macro_reads_correctly) {
    reset_ctx();
    ctx.regs.f = 0b10100000;
    ck_assert_uint_eq(BIT(ctx.regs.f, 7), 1);
    ck_assert_uint_eq(BIT(ctx.regs.f, 6), 0);
    ck_assert_uint_eq(BIT(ctx.regs.f, 5), 1);
    ck_assert_uint_eq(BIT(ctx.regs.f, 4), 0);
} END_TEST

START_TEST(test_bit_set_sets_and_clears_without_disturbing_others) {
    reset_ctx();
    ctx.regs.f = 0x00;

    BIT_SET(ctx.regs.f, 4, 1);
    ck_assert_uint_eq(ctx.regs.f, 0b00010000);

    BIT_SET(ctx.regs.f, 7, 1);
    ck_assert_uint_eq(ctx.regs.f, 0b10010000);

    BIT_SET(ctx.regs.f, 4, 0);
    ck_assert_uint_eq(ctx.regs.f, 0b10000000);
} END_TEST

/* ---------------------------------------------------------- */
/* Register utilities (read_reg / set_reg / is16Bit)          */
/* ---------------------------------------------------------- */

START_TEST(test_set_and_read_reg_8bit_roundtrip) {
    reset_ctx();
    set_reg(RG_A, 0x42);
    ck_assert_uint_eq(read_reg(RG_A), 0x42);

    set_reg(RG_B, 0xFF);
    ck_assert_uint_eq(read_reg(RG_B), 0xFF);
} END_TEST

START_TEST(test_set_and_read_reg_16bit_roundtrip) {
    reset_ctx();
    set_reg(RG_HL, 0xBEEF);
    ck_assert_uint_eq(read_reg(RG_HL), 0xBEEF);
    ck_assert_uint_eq(ctx.regs.h, 0xBE);
    ck_assert_uint_eq(ctx.regs.l, 0xEF);
} END_TEST

START_TEST(test_is16bit_classification) {
    ck_assert_uint_eq(is16Bit(RG_BC), true);
    ck_assert_uint_eq(is16Bit(RG_HL), true);
    ck_assert_uint_eq(is16Bit(RG_SP), true);
    ck_assert_uint_eq(is16Bit(RG_A), false);
    ck_assert_uint_eq(is16Bit(RG_B), false);
} END_TEST

/* ---------------------------------------------------------- */
/* NOP                                                        */
/* ---------------------------------------------------------- */

START_TEST(test_nop_touches_nothing) {
    reset_ctx();
    ctx.regs.a = 0x42;
    ctx.regs.f = 0xB0;

    instruction inst = make_inst(IN_NOP, AM_IMP, RG_NONE, RG_NONE, CND_NONE);
    ctx.inst = &inst;

    inst_get_processor(IN_NOP)(&ctx);

    ck_assert_uint_eq(ctx.regs.a, 0x42);
    ck_assert_uint_eq(ctx.regs.f, 0xB0);
} END_TEST

/* ---------------------------------------------------------- */
/* 8-bit ALU: ADD / ADC / SUB / SBC / AND / XOR / OR / CP      */
/* ---------------------------------------------------------- */

START_TEST(test_add_basic_no_flags) {
    reset_ctx();
    instruction inst = make_inst(IN_ADD, AM_R_R, RG_A, RG_B, CND_NONE);
    ctx.inst = &inst;
    set_reg(RG_A, 0x10);
    ctx.fetched_data = 0x05;

    inst_get_processor(IN_ADD)(&ctx);

    ck_assert_uint_eq(read_reg(RG_A), 0x15);
    ck_assert_uint_eq(BIT(ctx.regs.f, 7), 0);
    ck_assert_uint_eq(BIT(ctx.regs.f, 4), 0);
} END_TEST

START_TEST(test_add_sets_zero_and_carry) {
    reset_ctx();
    instruction inst = make_inst(IN_ADD, AM_R_R, RG_A, RG_B, CND_NONE);
    ctx.inst = &inst;
    set_reg(RG_A, 0xFF);
    ctx.fetched_data = 0x01;

    inst_get_processor(IN_ADD)(&ctx);

    ck_assert_uint_eq(read_reg(RG_A), 0x00);
    ck_assert_uint_eq(BIT(ctx.regs.f, 7), 1);
    ck_assert_uint_eq(BIT(ctx.regs.f, 5), 1);
    ck_assert_uint_eq(BIT(ctx.regs.f, 4), 1);
} END_TEST

START_TEST(test_add_16bit_hl_bc) {
    reset_ctx();
    instruction inst = make_inst(IN_ADD, AM_R_R, RG_HL, RG_BC, CND_NONE);
    ctx.inst = &inst;
    set_reg(RG_HL, 0x0FFF);
    ctx.fetched_data = 0x0001;

    inst_get_processor(IN_ADD)(&ctx);

    ck_assert_uint_eq(read_reg(RG_HL), 0x1000);
    ck_assert_uint_eq(BIT(ctx.regs.f, 5), 1); // carry out of bit 11
    ck_assert_uint_eq(BIT(ctx.regs.f, 4), 0); // no carry out of bit 15
} END_TEST

START_TEST(test_add_sp_r8_positive) {
    reset_ctx();
    instruction inst = make_inst(IN_ADD, AM_R_D8, RG_SP, RG_NONE, CND_NONE);
    ctx.inst = &inst;
    ctx.regs.sp = 0x0005;
    ctx.fetched_data = 0x03;

    inst_get_processor(IN_ADD)(&ctx);

    ck_assert_uint_eq(ctx.regs.sp, 0x0008);
    ck_assert_uint_eq(BIT(ctx.regs.f, 7), 0); // Z always forced to 0 for ADD SP,r8
} END_TEST

// NOTE: this test encodes the FIXED behavior discussed earlier.
// It will fail until the proc_adc half-carry threshold (0x100 -> 0x10) is corrected.
START_TEST(test_adc_sets_half_carry_correctly) {
    reset_ctx();
    instruction inst = make_inst(IN_ADC, AM_R_R, RG_A, RG_B, CND_NONE);
    ctx.inst = &inst;
    ctx.regs.a = 0x0F;
    ctx.fetched_data = 0x01;
    BIT_SET(ctx.regs.f, 4, 0); // carry in = 0

    inst_get_processor(IN_ADC)(&ctx);

    ck_assert_uint_eq(ctx.regs.a, 0x10);
    ck_assert_uint_eq(BIT(ctx.regs.f, 5), 1);
    ck_assert_uint_eq(BIT(ctx.regs.f, 4), 0);
} END_TEST

START_TEST(test_adc_includes_carry_in) {
    reset_ctx();
    instruction inst = make_inst(IN_ADC, AM_R_R, RG_A, RG_B, CND_NONE);
    ctx.inst = &inst;
    ctx.regs.a = 0x01;
    ctx.fetched_data = 0x01;
    BIT_SET(ctx.regs.f, 4, 1); // carry in = 1

    inst_get_processor(IN_ADC)(&ctx);

    ck_assert_uint_eq(ctx.regs.a, 0x03); // 1 + 1 + carry(1)
} END_TEST

START_TEST(test_sub_basic) {
    reset_ctx();
    instruction inst = make_inst(IN_SUB, AM_R_R, RG_A, RG_B, CND_NONE);
    ctx.inst = &inst;
    set_reg(RG_A, 0x10);
    ctx.fetched_data = 0x05;

    inst_get_processor(IN_SUB)(&ctx);

    ck_assert_uint_eq(read_reg(RG_A), 0x0B);
    ck_assert_uint_eq(BIT(ctx.regs.f, 6), 1);
    ck_assert_uint_eq(BIT(ctx.regs.f, 4), 0);
} END_TEST

START_TEST(test_sub_sets_carry_on_borrow) {
    reset_ctx();
    instruction inst = make_inst(IN_SUB, AM_R_R, RG_A, RG_B, CND_NONE);
    ctx.inst = &inst;
    set_reg(RG_A, 0x00);
    ctx.fetched_data = 0x01;

    inst_get_processor(IN_SUB)(&ctx);

    ck_assert_uint_eq(BIT(ctx.regs.f, 4), 1);
} END_TEST

// NOTE: encodes FIXED behavior — will fail until proc_sbc uses
// ctx->fetched_data instead of read_reg(reg_1) when building `val`.
START_TEST(test_sbc_subtracts_operand_and_carry) {
    reset_ctx();
    instruction inst = make_inst(IN_SBC, AM_R_R, RG_A, RG_B, CND_NONE);
    ctx.inst = &inst;
    ctx.regs.a = 0x10;
    ctx.fetched_data = 0x05;
    BIT_SET(ctx.regs.f, 4, 1); // carry in = 1

    inst_get_processor(IN_SBC)(&ctx);

    ck_assert_uint_eq(ctx.regs.a, 0x0A); // 0x10 - (0x05 + 1)
} END_TEST

START_TEST(test_and_masks_and_sets_h_flag) {
    reset_ctx();
    instruction inst = make_inst(IN_AND, AM_R_R, RG_A, RG_B, CND_NONE);
    ctx.inst = &inst;
    ctx.regs.a = 0xFF;
    ctx.fetched_data = 0x0F;

    inst_get_processor(IN_AND)(&ctx);

    ck_assert_uint_eq(ctx.regs.a, 0x0F);
    ck_assert_uint_eq(BIT(ctx.regs.f, 5), 1); // AND always sets H per spec
    ck_assert_uint_eq(BIT(ctx.regs.f, 4), 0);
} END_TEST

START_TEST(test_xor_self_yields_zero) {
    reset_ctx();
    instruction inst = make_inst(IN_XOR, AM_R_R, RG_A, RG_A, CND_NONE);
    ctx.inst = &inst;
    ctx.regs.a = 0x5A;
    ctx.fetched_data = 0x5A;

    inst_get_processor(IN_XOR)(&ctx);

    ck_assert_uint_eq(ctx.regs.a, 0x00);
    ck_assert_uint_eq(BIT(ctx.regs.f, 7), 1);
} END_TEST

START_TEST(test_or_combines_bits) {
    reset_ctx();
    instruction inst = make_inst(IN_OR, AM_R_R, RG_A, RG_B, CND_NONE);
    ctx.inst = &inst;
    ctx.regs.a = 0x0F;
    ctx.fetched_data = 0xF0;

    inst_get_processor(IN_OR)(&ctx);

    ck_assert_uint_eq(ctx.regs.a, 0xFF);
} END_TEST

START_TEST(test_cp_does_not_modify_a) {
    reset_ctx();
    instruction inst = make_inst(IN_CP, AM_R_R, RG_A, RG_B, CND_NONE);
    ctx.inst = &inst;
    ctx.regs.a = 0x10;
    ctx.fetched_data = 0x10;

    inst_get_processor(IN_CP)(&ctx);

    ck_assert_uint_eq(ctx.regs.a, 0x10); // A must be unchanged
    ck_assert_uint_eq(BIT(ctx.regs.f, 7), 1);
    ck_assert_uint_eq(BIT(ctx.regs.f, 6), 1);
} END_TEST

/* ---------------------------------------------------------- */
/* INC / DEC (register-only, no memory)                       */
/* ---------------------------------------------------------- */

START_TEST(test_inc_register) {
    reset_ctx();
    instruction inst = make_inst(IN_INC, AM_R, RG_C, RG_NONE, CND_NONE);
    ctx.inst = &inst;
    ctx.opcode = 0x0C; // INC C
    set_reg(RG_C, 0x0F);

    inst_get_processor(IN_INC)(&ctx);

    ck_assert_uint_eq(read_reg(RG_C), 0x10);
    ck_assert_uint_eq(BIT(ctx.regs.f, 5), 1);
} END_TEST

START_TEST(test_dec_register_sets_zero) {
    reset_ctx();
    instruction inst = make_inst(IN_DEC, AM_R, RG_C, RG_NONE, CND_NONE);
    ctx.inst = &inst;
    ctx.opcode = 0x0D; // DEC C
    set_reg(RG_C, 0x01);

    inst_get_processor(IN_DEC)(&ctx);

    ck_assert_uint_eq(read_reg(RG_C), 0x00);
    ck_assert_uint_eq(BIT(ctx.regs.f, 7), 1);
    ck_assert_uint_eq(BIT(ctx.regs.f, 6), 1);
} END_TEST

START_TEST(test_inc_16bit_does_not_touch_flags) {
    reset_ctx();
    instruction inst = make_inst(IN_INC, AM_R, RG_BC, RG_NONE, CND_NONE);
    ctx.inst = &inst;
    ctx.opcode = 0x03; // INC BC
    ctx.regs.f = 0xB0;
    set_reg(RG_BC, 0xFFFF);

    inst_get_processor(IN_INC)(&ctx);

    ck_assert_uint_eq(read_reg(RG_BC), 0x0000);
    ck_assert_uint_eq(ctx.regs.f, 0xB0); // flags untouched for 16-bit INC
} END_TEST

/* ---------------------------------------------------------- */
/* LD                                                          */
/* ---------------------------------------------------------- */

START_TEST(test_ld_register_to_register) {
    reset_ctx();
    instruction inst = make_inst(IN_LD, AM_R_R, RG_B, RG_A, CND_NONE);
    ctx.inst = &inst;
    ctx.dest_is_mem = false;
    set_reg(RG_A, 0x99);
    ctx.fetched_data = read_reg(RG_A);

    inst_get_processor(IN_LD)(&ctx);

    ck_assert_uint_eq(read_reg(RG_B), 0x99);
} END_TEST

/* ---------------------------------------------------------- */
/* Rotates                                                     */
/* ---------------------------------------------------------- */

START_TEST(test_rlca_wraps_top_bit_to_carry_and_bit0) {
    reset_ctx();
    ctx.regs.a = 0b10000001;

    inst_get_processor(IN_RLCA)(&ctx);

    ck_assert_uint_eq(ctx.regs.a, 0b00000011);
    ck_assert_uint_eq(BIT(ctx.regs.f, 4), 1);
    ck_assert_uint_eq(BIT(ctx.regs.f, 7), 0); // RLCA always clears Z
} END_TEST

START_TEST(test_rrca_wraps_bottom_bit_to_carry_and_bit7) {
    reset_ctx();
    ctx.regs.a = 0b00000001;

    inst_get_processor(IN_RRCA)(&ctx);

    ck_assert_uint_eq(ctx.regs.a, 0b10000000);
    ck_assert_uint_eq(BIT(ctx.regs.f, 4), 1);
} END_TEST

/* ---------------------------------------------------------- */
/* DAA / CPL / SCF / CCF                                       */
/* ---------------------------------------------------------- */

START_TEST(test_cpl_inverts_a_and_sets_n_h) {
    reset_ctx();
    ctx.regs.a = 0b10101010;

    inst_get_processor(IN_CPL)(&ctx);

    ck_assert_uint_eq(ctx.regs.a, 0b01010101);
    ck_assert_uint_eq(BIT(ctx.regs.f, 6), 1);
    ck_assert_uint_eq(BIT(ctx.regs.f, 5), 1);
} END_TEST

START_TEST(test_scf_sets_carry_clears_n_h) {
    reset_ctx();
    ctx.regs.f = 0b01100000; // N and H set beforehand

    inst_get_processor(IN_SCF)(&ctx);

    ck_assert_uint_eq(BIT(ctx.regs.f, 4), 1);
    ck_assert_uint_eq(BIT(ctx.regs.f, 6), 0);
    ck_assert_uint_eq(BIT(ctx.regs.f, 5), 0);
} END_TEST

START_TEST(test_ccf_toggles_carry) {
    reset_ctx();
    BIT_SET(ctx.regs.f, 4, 1); // C = 1

    inst_get_processor(IN_CCF)(&ctx);
    ck_assert_uint_eq(BIT(ctx.regs.f, 4), 0);

    inst_get_processor(IN_CCF)(&ctx);
    ck_assert_uint_eq(BIT(ctx.regs.f, 4), 1);
} END_TEST

START_TEST(test_daa_after_bcd_addition) {
    reset_ctx();
    // 0x09 + 0x01 = 0x0A in binary, DAA should correct to 0x10 in BCD
    ctx.regs.a = 0x0A;
    BIT_SET(ctx.regs.f, 6, 0); // N = 0 (came from addition)
    BIT_SET(ctx.regs.f, 5, 0); // H = 0

    inst_get_processor(IN_DAA)(&ctx);

    ck_assert_uint_eq(ctx.regs.a, 0x10);
} END_TEST

/* ---------------------------------------------------------- */
/* Stack: PUSH / POP round-trip                                */
/* Uses WRAM address range for SP (0xC000-0xDFFF) so it        */
/* doesn't depend on a loaded cartridge.                       */
/* ---------------------------------------------------------- */

START_TEST(test_push_pop_16bit_roundtrip) {
    reset_ctx();
    ctx.regs.sp = 0xD000;

    instruction push_inst = make_inst(IN_PUSH, AM_R, RG_BC, RG_NONE, CND_NONE);
    ctx.inst = &push_inst;
    set_reg(RG_BC, 0xBEEF);
    inst_get_processor(IN_PUSH)(&ctx);

    ck_assert_uint_eq(ctx.regs.sp, 0xCFFE); // SP decremented by 2

    instruction pop_inst = make_inst(IN_POP, AM_R, RG_DE, RG_NONE, CND_NONE);
    ctx.inst = &pop_inst;
    inst_get_processor(IN_POP)(&ctx);

    ck_assert_uint_eq(read_reg(RG_DE), 0xBEEF);
    ck_assert_uint_eq(ctx.regs.sp, 0xD000); // SP restored
} END_TEST

/* ---------------------------------------------------------- */
/* Jumps / calls / returns                                     */
/* ---------------------------------------------------------- */

START_TEST(test_jp_unconditional) {
    reset_ctx();
    instruction inst = make_inst(IN_JP, AM_D16, RG_NONE, RG_NONE, CND_NONE);
    ctx.inst = &inst;
    ctx.fetched_data = 0x1234;

    inst_get_processor(IN_JP)(&ctx);

    ck_assert_uint_eq(ctx.regs.pc, 0x1234);
} END_TEST

START_TEST(test_jp_conditional_not_taken) {
    reset_ctx();
    instruction inst = make_inst(IN_JP, AM_D16, RG_NONE, RG_NONE, CND_Z);
    ctx.inst = &inst;
    ctx.regs.pc = 0x0100;
    ctx.fetched_data = 0x9999;
    BIT_SET(ctx.regs.f, 7, 0); // Z = 0, so CND_Z should NOT trigger the jump

    inst_get_processor(IN_JP)(&ctx);

    ck_assert_uint_eq(ctx.regs.pc, 0x0100); // unchanged
} END_TEST

START_TEST(test_call_pushes_return_addr_and_jumps) {
    reset_ctx();
    ctx.regs.sp = 0xD000;
    ctx.regs.pc = 0x0150;

    instruction inst = make_inst(IN_CALL, AM_D16, RG_NONE, RG_NONE, CND_NONE);
    ctx.inst = &inst;
    ctx.fetched_data = 0x0200;

    inst_get_processor(IN_CALL)(&ctx);

    ck_assert_uint_eq(ctx.regs.pc, 0x0200);
    ck_assert_uint_eq(ctx.regs.sp, 0xCFFE);

    u16 pushed = bus_read(0xCFFE) | (bus_read(0xCFFF) << 8);
    ck_assert_uint_eq(pushed, 0x0150);
} END_TEST

START_TEST(test_ret_pops_pc) {
    reset_ctx();
    ctx.regs.sp = 0xD000;

    instruction call_inst = make_inst(IN_CALL, AM_D16, RG_NONE, RG_NONE, CND_NONE);
    ctx.inst = &call_inst;
    ctx.regs.pc = 0x0150;
    ctx.fetched_data = 0x0200;
    inst_get_processor(IN_CALL)(&ctx);

    instruction ret_inst = make_inst(IN_RET, AM_IMP, RG_NONE, RG_NONE, CND_NONE);
    ctx.inst = &ret_inst;
    inst_get_processor(IN_RET)(&ctx);

    ck_assert_uint_eq(ctx.regs.pc, 0x0150);
    ck_assert_uint_eq(ctx.regs.sp, 0xD000);
} END_TEST

/* ---------------------------------------------------------- */
/* fetch_data() — pure register addressing modes only          */
/* (no bus/cart dependency required for these)                 */
/* ---------------------------------------------------------- */

START_TEST(test_fetch_data_am_imp_is_noop) {
    reset_ctx();
    instruction inst = make_inst(IN_NOP, AM_IMP, RG_NONE, RG_NONE, CND_NONE);
    ctx.inst = &inst;
    ctx.fetched_data = 0xAAAA;

    fetch_data();

    ck_assert_uint_eq(ctx.fetched_data, 0xAAAA); // untouched
    ck_assert_uint_eq(ctx.dest_is_mem, false);
} END_TEST

START_TEST(test_fetch_data_am_r) {
    reset_ctx();
    instruction inst = make_inst(IN_INC, AM_R, RG_A, RG_NONE, CND_NONE);
    ctx.inst = &inst;
    set_reg(RG_A, 0x77);

    fetch_data();

    ck_assert_uint_eq(ctx.fetched_data, 0x77);
} END_TEST

START_TEST(test_fetch_data_am_r_r) {
    reset_ctx();
    instruction inst = make_inst(IN_LD, AM_R_R, RG_B, RG_C, CND_NONE);
    ctx.inst = &inst;
    set_reg(RG_C, 0x33);

    fetch_data();

    ck_assert_uint_eq(ctx.fetched_data, 0x33);
} END_TEST

START_TEST(test_fetch_data_null_inst_returns_early) {
    reset_ctx();
    ctx.inst = NULL;
    ctx.fetched_data = 0x55;

    fetch_data(); // should return immediately without touching fetched_data

    ck_assert_uint_eq(ctx.fetched_data, 0x55);
} END_TEST

/* ---------------------------------------------------------- */
/* Instruction table regressions                               */
/* (these directly guard against the missing-opcode bug we     */
/* found earlier)                                               */
/* ---------------------------------------------------------- */

START_TEST(test_opcode_table_has_daa_cpl_scf_ccf) {
    ck_assert_int_eq(instruction_by_opcode(0x27)->type, IN_DAA);
    ck_assert_int_eq(instruction_by_opcode(0x2F)->type, IN_CPL);
    ck_assert_int_eq(instruction_by_opcode(0x37)->type, IN_SCF);
    ck_assert_int_eq(instruction_by_opcode(0x3F)->type, IN_CCF);
} END_TEST

START_TEST(test_opcode_table_spot_checks) {
    ck_assert_int_eq(instruction_by_opcode(0x00)->type, IN_NOP);
    ck_assert_int_eq(instruction_by_opcode(0xC3)->type, IN_JP);
    ck_assert_int_eq(instruction_by_opcode(0x76)->type, IN_HALT);
    ck_assert_int_eq(instruction_by_opcode(0xCB)->type, IN_CB);
} END_TEST


/* ---------------------------------------------------------- */
/* Load instructions, decoded and executed via real cpu_step   */
/* ---------------------------------------------------------- */

START_TEST(test_ld_r_d8_via_pipeline) {
    reset_ctx();
    u8 prog[] = {0x06, 0x42}; // LD B, 0x42
    run_program(prog, 2);

    ck_assert_uint_eq(read_reg(RG_B), 0x42);
    ck_assert_uint_eq(ctx.regs.pc, PROG_ADDR + 2);
} END_TEST

START_TEST(test_ld_r_r_via_pipeline) {
    reset_ctx();
    set_reg(RG_B, 0x77);
    u8 prog[] = {0x78}; // LD A, B
    run_program(prog, 1);

    ck_assert_uint_eq(read_reg(RG_A), 0x77);
    ck_assert_uint_eq(ctx.regs.pc, PROG_ADDR + 1);
} END_TEST

START_TEST(test_ld_mr_r_via_pipeline) {
    reset_ctx();
    set_reg(RG_HL, 0xC010);
    set_reg(RG_A, 0x99);
    u8 prog[] = {0x77}; // LD (HL), A
    run_program(prog, 1);

    ck_assert_uint_eq(bus_read(0xC010), 0x99);
} END_TEST

START_TEST(test_ld_r_mr_via_pipeline) {
    reset_ctx();
    bus_write(0xC020, 0x55); // pre-seed memory
    set_reg(RG_HL, 0xC020);
    u8 prog[] = {0x7E}; // LD A, (HL)
    run_program(prog, 1);

    ck_assert_uint_eq(read_reg(RG_A), 0x55);
} END_TEST

START_TEST(test_ld_r_d16_via_pipeline) {
    reset_ctx();
    u8 prog[] = {0x21, 0xCD, 0xAB}; // LD HL, 0xABCD (little-endian)
    run_program(prog, 3);

    ck_assert_uint_eq(read_reg(RG_HL), 0xABCD);
    ck_assert_uint_eq(ctx.regs.pc, PROG_ADDR + 3);
} END_TEST

START_TEST(test_ld_a16_sp_via_pipeline) {
    reset_ctx();
    ctx.regs.sp = 0x1234;
    u8 prog[] = {0x08, 0x20, 0xC0}; // LD (0xC020), SP
    run_program(prog, 3);

    ck_assert_uint_eq(bus_read16(0xC020), 0x1234);
} END_TEST

/* ---------------------------------------------------------- */
/* Arithmetic reached through the pipeline                     */
/* ---------------------------------------------------------- */

START_TEST(test_inc_r_via_pipeline_sets_half_carry) {
    reset_ctx();
    set_reg(RG_B, 0x0F);
    u8 prog[] = {0x04}; // INC B
    run_program(prog, 1);

    ck_assert_uint_eq(read_reg(RG_B), 0x10);
    ck_assert_uint_eq(BIT(ctx.regs.f, 5), 1);
} END_TEST

START_TEST(test_add_r_r_via_pipeline) {
    reset_ctx();
    set_reg(RG_A, 0x10);
    set_reg(RG_B, 0x05);
    u8 prog[] = {0x80}; // ADD A, B
    run_program(prog, 1);

    ck_assert_uint_eq(read_reg(RG_A), 0x15);
} END_TEST

START_TEST(test_cp_d8_via_pipeline_does_not_modify_a) {
    reset_ctx();
    set_reg(RG_A, 0x10);
    u8 prog[] = {0xFE, 0x10}; // CP 0x10
    run_program(prog, 2);

    ck_assert_uint_eq(read_reg(RG_A), 0x10);
    ck_assert_uint_eq(BIT(ctx.regs.f, 7), 1);
} END_TEST

/* ---------------------------------------------------------- */
/* Branching: relative and absolute                            */
/* ---------------------------------------------------------- */

START_TEST(test_jr_nz_taken_via_pipeline) {
    reset_ctx();
    BIT_SET(ctx.regs.f, 7, 0); // Z = 0, so NZ is true
    u8 prog[] = {0x20, 0x05}; // JR NZ, +5
    run_program(prog, 2);

    // dest = (PROG_ADDR + 2) + 5
    ck_assert_uint_eq(ctx.regs.pc, PROG_ADDR + 2 + 5);
} END_TEST

START_TEST(test_jr_nz_not_taken_via_pipeline) {
    reset_ctx();
    BIT_SET(ctx.regs.f, 7, 1); // Z = 1, so NZ is false
    u8 prog[] = {0x20, 0x05}; // JR NZ, +5
    run_program(prog, 2);

    ck_assert_uint_eq(ctx.regs.pc, PROG_ADDR + 2); // no jump taken
} END_TEST

START_TEST(test_jp_a16_via_pipeline) {
    reset_ctx();
    u8 prog[] = {0xC3, 0x00, 0x02}; // JP 0x0200
    run_program(prog, 3);

    ck_assert_uint_eq(ctx.regs.pc, 0x0200);
} END_TEST

/* ---------------------------------------------------------- */
/* CALL / RET and PUSH / POP round trips via real cpu_step     */
/* ---------------------------------------------------------- */

START_TEST(test_call_then_ret_via_pipeline) {
    reset_ctx();
    ctx.regs.sp = SAFE_SP;

    u8 call_prog[] = {0xCD, 0x00, 0xC5}; // CALL 0xC500
    run_program(call_prog, 3);

    ck_assert_uint_eq(ctx.regs.pc, 0xC500);
    ck_assert_uint_eq(ctx.regs.sp, SAFE_SP - 2);
    ck_assert_uint_eq(bus_read16(SAFE_SP - 2), PROG_ADDR + 3); // correct return addr pushed

    u8 ret_prog[] = {0xC9}; // RET
    write_bytes(0xC500, ret_prog, 1);
    cpu_step(); // continues from ctx.regs.pc, already at 0xC500

    ck_assert_uint_eq(ctx.regs.pc, PROG_ADDR + 3);
    ck_assert_uint_eq(ctx.regs.sp, SAFE_SP);
} END_TEST

START_TEST(test_push_then_pop_via_pipeline) {
    reset_ctx();
    ctx.regs.sp = SAFE_SP;
    set_reg(RG_BC, 0xBEEF);

    u8 push_prog[] = {0xC5}; // PUSH BC
    run_program(push_prog, 1);
    ck_assert_uint_eq(ctx.regs.sp, SAFE_SP - 2);

    u8 pop_prog[] = {0xD1}; // POP DE
    write_bytes(ctx.regs.pc, pop_prog, 1);
    cpu_step();

    ck_assert_uint_eq(read_reg(RG_DE), 0xBEEF);
    ck_assert_uint_eq(ctx.regs.sp, SAFE_SP);
} END_TEST

/* ---------------------------------------------------------- */
/* LDH via HRAM (0xFF80 avoids depending on io.c)              */
/* ---------------------------------------------------------- */

START_TEST(test_ldh_write_then_read_via_pipeline) {
    reset_ctx();
    set_reg(RG_A, 0xAB);

    u8 write_prog[] = {0xE0, 0x80}; // LDH (0xFF80), A
    run_program(write_prog, 2);
    ck_assert_uint_eq(bus_read(0xFF80), 0xAB);

    reset_ctx(); // fresh cpu regs; HRAM persists (forked test process, single test)
    u8 read_prog[] = {0xF0, 0x80}; // LDH A, (0xFF80)
    run_program(read_prog, 2);
    ck_assert_uint_eq(read_reg(RG_A), 0xAB);
} END_TEST

/* ---------------------------------------------------------- */
/* Regression: previously-missing opcodes (0x27/0x2F/0x37/0x3F)*/
/* now actually dispatch correctly end-to-end                  */
/* ---------------------------------------------------------- */

START_TEST(test_scf_via_pipeline) {
    reset_ctx();
    u8 prog[] = {0x37}; // SCF
    run_program(prog, 1);

    ck_assert_uint_eq(BIT(ctx.regs.f, 4), 1);
} END_TEST

START_TEST(test_cpl_via_pipeline) {
    reset_ctx();
    set_reg(RG_A, 0xAA);
    u8 prog[] = {0x2F}; // CPL
    run_program(prog, 1);

    ck_assert_uint_eq(read_reg(RG_A), 0x55);
} END_TEST

/* ---------------------------------------------------------- */
/* Instruction-length regression sweep                         */
/* Confirms fetch_data() consumes exactly the right number of  */
/* operand bytes for a spread of addressing modes.              */
/* ---------------------------------------------------------- */

typedef struct {
    u8 bytes[3];
    int len;
    const char *desc;
} len_case;

START_TEST(test_instruction_lengths_via_pipeline) {
    len_case cases[] = {
        { {0x00},             1, "NOP" },
        { {0x01, 0x00, 0x00}, 3, "LD BC,d16" },
        { {0x04},             1, "INC B" },
        { {0x06, 0x00},       2, "LD B,d8" },
        { {0x0E, 0x00},       2, "LD C,d8" },
        { {0x11, 0x00, 0x00}, 3, "LD DE,d16" },
        { {0x18, 0x00},       2, "JR +0" },
        { {0x21, 0x00, 0x00}, 3, "LD HL,d16" },
        { {0x36, 0x00},       2, "LD (HL),d8" },
        { {0x3E, 0x00},       2, "LD A,d8" },
        { {0x80},             1, "ADD A,B" },
        { {0xC6, 0x00},       2, "ADD A,d8" },
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        reset_ctx();
        set_reg(RG_HL, 0xC080); // safe target for LD (HL),d8
        run_program(cases[i].bytes, cases[i].len);

        ck_assert_msg(ctx.regs.pc == PROG_ADDR + cases[i].len,
            "%s: expected PC delta %d, got %d",
            cases[i].desc, cases[i].len, ctx.regs.pc - PROG_ADDR);
    }
} END_TEST

/* ---------------------------------------------------------- */
/* Pure bus-level regression (no CPU involved)                 */
/* ---------------------------------------------------------- */

START_TEST(test_bus_read16_write16_roundtrip) {
    bus_write16(0xC030, 0xBEEF);
    ck_assert_uint_eq(bus_read16(0xC030), 0xBEEF);
    ck_assert_uint_eq(bus_read(0xC030), 0xEF); // low byte
    ck_assert_uint_eq(bus_read(0xC031), 0xBE); // high byte
} END_TEST


/* ---------------------------------------------------------- */
/* Suite wiring                                                */
/* ---------------------------------------------------------- */

Suite *instruction_pipeline_suite() {
    Suite *s = suite_create("instruction_pipeline");

    TCase *tc_load = tcase_create("load");
    tcase_add_test(tc_load, test_ld_r_d8_via_pipeline);
    tcase_add_test(tc_load, test_ld_r_r_via_pipeline);
    tcase_add_test(tc_load, test_ld_mr_r_via_pipeline);
    tcase_add_test(tc_load, test_ld_r_mr_via_pipeline);
    tcase_add_test(tc_load, test_ld_r_d16_via_pipeline);
    tcase_add_test(tc_load, test_ld_a16_sp_via_pipeline);
    suite_add_tcase(s, tc_load);

    TCase *tc_alu = tcase_create("alu");
    tcase_add_test(tc_alu, test_inc_r_via_pipeline_sets_half_carry);
    tcase_add_test(tc_alu, test_add_r_r_via_pipeline);
    tcase_add_test(tc_alu, test_cp_d8_via_pipeline_does_not_modify_a);
    suite_add_tcase(s, tc_alu);

    TCase *tc_branch = tcase_create("branching");
    tcase_add_test(tc_branch, test_jr_nz_taken_via_pipeline);
    tcase_add_test(tc_branch, test_jr_nz_not_taken_via_pipeline);
    tcase_add_test(tc_branch, test_jp_a16_via_pipeline);
    tcase_add_test(tc_branch, test_call_then_ret_via_pipeline);
    tcase_add_test(tc_branch, test_push_then_pop_via_pipeline);
    suite_add_tcase(s, tc_branch);

    TCase *tc_ldh = tcase_create("ldh");
    tcase_add_test(tc_ldh, test_ldh_write_then_read_via_pipeline);
    suite_add_tcase(s, tc_ldh);

    TCase *tc_regress = tcase_create("regressions");
    tcase_add_test(tc_regress, test_scf_via_pipeline);
    tcase_add_test(tc_regress, test_cpl_via_pipeline);
    tcase_add_test(tc_regress, test_instruction_lengths_via_pipeline);
    suite_add_tcase(s, tc_regress);

    TCase *tc_bus = tcase_create("bus");
    tcase_add_test(tc_bus, test_bus_read16_write16_roundtrip);
    suite_add_tcase(s, tc_bus);

    return s;
}

Suite *cpu_suite() {
    Suite *s = suite_create("cpu");

    TCase *tc_flags = tcase_create("flags");
    tcase_add_test(tc_flags, test_bit_macro_reads_correctly);
    tcase_add_test(tc_flags, test_bit_set_sets_and_clears_without_disturbing_others);
    suite_add_tcase(s, tc_flags);

    TCase *tc_regs = tcase_create("register_utils");
    tcase_add_test(tc_regs, test_set_and_read_reg_8bit_roundtrip);
    tcase_add_test(tc_regs, test_set_and_read_reg_16bit_roundtrip);
    tcase_add_test(tc_regs, test_is16bit_classification);
    suite_add_tcase(s, tc_regs);

    TCase *tc_alu = tcase_create("alu");
    tcase_add_test(tc_alu, test_nop_touches_nothing);
    tcase_add_test(tc_alu, test_add_basic_no_flags);
    tcase_add_test(tc_alu, test_add_sets_zero_and_carry);
    tcase_add_test(tc_alu, test_add_16bit_hl_bc);
    tcase_add_test(tc_alu, test_add_sp_r8_positive);
    tcase_add_test(tc_alu, test_adc_sets_half_carry_correctly);
    tcase_add_test(tc_alu, test_adc_includes_carry_in);
    tcase_add_test(tc_alu, test_sub_basic);
    tcase_add_test(tc_alu, test_sub_sets_carry_on_borrow);
    tcase_add_test(tc_alu, test_sbc_subtracts_operand_and_carry);
    tcase_add_test(tc_alu, test_and_masks_and_sets_h_flag);
    tcase_add_test(tc_alu, test_xor_self_yields_zero);
    tcase_add_test(tc_alu, test_or_combines_bits);
    tcase_add_test(tc_alu, test_cp_does_not_modify_a);
    tcase_add_test(tc_alu, test_inc_register);
    tcase_add_test(tc_alu, test_dec_register_sets_zero);
    tcase_add_test(tc_alu, test_inc_16bit_does_not_touch_flags);
    suite_add_tcase(s, tc_alu);

    TCase *tc_ld = tcase_create("load");
    tcase_add_test(tc_ld, test_ld_register_to_register);
    suite_add_tcase(s, tc_ld);

    TCase *tc_rot = tcase_create("rotate_misc");
    tcase_add_test(tc_rot, test_rlca_wraps_top_bit_to_carry_and_bit0);
    tcase_add_test(tc_rot, test_rrca_wraps_bottom_bit_to_carry_and_bit7);
    tcase_add_test(tc_rot, test_cpl_inverts_a_and_sets_n_h);
    tcase_add_test(tc_rot, test_scf_sets_carry_clears_n_h);
    tcase_add_test(tc_rot, test_ccf_toggles_carry);
    tcase_add_test(tc_rot, test_daa_after_bcd_addition);
    suite_add_tcase(s, tc_rot);

    TCase *tc_stack = tcase_create("stack");
    tcase_add_test(tc_stack, test_push_pop_16bit_roundtrip);
    suite_add_tcase(s, tc_stack);

    TCase *tc_flow = tcase_create("control_flow");
    tcase_add_test(tc_flow, test_jp_unconditional);
    tcase_add_test(tc_flow, test_jp_conditional_not_taken);
    tcase_add_test(tc_flow, test_call_pushes_return_addr_and_jumps);
    tcase_add_test(tc_flow, test_ret_pops_pc);
    suite_add_tcase(s, tc_flow);

    TCase *tc_fetch = tcase_create("fetch_data");
    tcase_add_test(tc_fetch, test_fetch_data_am_imp_is_noop);
    tcase_add_test(tc_fetch, test_fetch_data_am_r);
    tcase_add_test(tc_fetch, test_fetch_data_am_r_r);
    tcase_add_test(tc_fetch, test_fetch_data_null_inst_returns_early);
    suite_add_tcase(s, tc_fetch);

    TCase *tc_table = tcase_create("instruction_table");
    tcase_add_test(tc_table, test_opcode_table_has_daa_cpl_scf_ccf);
    tcase_add_test(tc_table, test_opcode_table_spot_checks);
    suite_add_tcase(s, tc_table);

    return s;
}

int main() {
    SRunner *sr = srunner_create(cpu_suite());
    srunner_add_suite(sr, instruction_pipeline_suite());
    srunner_run_all(sr, CK_NORMAL);
    int nf = srunner_ntests_failed(sr);
    

    srunner_free(sr);

    return nf == 0 ? 0 : -1;
}