/**
 * @author Shaokai Lin <shaokai@berkeley.edu>
 * @brief Format of the instruction set
 *
 * VM Instruction Set
 * - ADDI rs1, rs2, rs3 : [Lock-free] Add to an integer variable (rs2) by an amount (rs3), and store the result in a destination variable (rs1).
 * - ADV    rs1,    rs2 : ADVance the logical time of a reactor (rs1) based on a variable offset (rs2) plus a fixed time duration (rs3).
 * - ADV2   rs1,    rs2 : Lock-free version of ADV. The compiler needs to guarantee only a single thread can update a reactor's tag.
 * - BIT    rs1,        : (Branch-If-Timeout) Branch to a location (rs1) if all reactors reach timeout.
 * - DU     rs1,    rs2 : Delay Until the physical time reaches a variable offset (rs1) plus a fixed time duration (rs2).
 * - EIT    rs1         : Execute a reaction (rs1) If Triggered. FIXME: Combine with a branch.
 * - EXE    rs1         : EXEcute a reaction (rs1) (used for known triggers such as startup, shutdown, and timers).
 * - JMP    rs1         : JuMP to a location (rs1).
 * - SAC                : (Sync-Advance-Clear) synchronize all workers until all execute SAC, let the last idle worker reset all counters to 0, and advance all reactors' logical time to a variable offset (rs1) plus a fixed time duration (rs2).
 * - STP                : SToP the execution.
 * - WU     rs1,    rs2 : Wait Until a counting variable (rs1) to reach a desired value (rs2).
 */
typedef enum {
    ADDI,
    ADV,
    ADV2,
    BIT,
    DU,
    EIT,
    EXE,
    JMP,
    SAC,
    STP,
    WU,
} opcode_t;

typedef struct inst_t {
    opcode_t op;
    uint64_t rs1;
    uint64_t rs2;
    uint64_t rs3;
} inst_t;