/**
 * @author Shaokai Lin <shaokai@berkeley.edu>
 * @brief Format of the instruction set
 *
 * VM Instruction Set
 * - ADV    rs1,    rs2 : ADVance the logical time of a reactor (rs1) by a specified amount (rs2). Add a delay_until here.
 * - ADV2   rs1,    rs2 : Lock-free version of ADV. The compiler needs to guarantee only a single thread can update a reactor's tag.
 * - BIT    rs1,        : (Branch-If-Timeout) Branch to a location (rs1) if all reactors reach timeout.
 * - DU     rs1,    rs2 : Delay Until a physical timepoint (rs1) plus an offset (rs2) is reached.
 * - EIT    rs1         : Execute a reaction (rs1) If Triggered. FIXME: Combine with a branch.
 * - EXE    rs1         : EXEcute a reaction (rs1) (used for known triggers such as startup, shutdown, and timers).
 * - INC    rs1,    rs2 : INCrement a counter (rs1) by an amount (rs2).
 * - INC2   rs1,    rs2 : Lock-free version of INC. The compiler needs to guarantee single writer.
 * - JMP    rs1         : JuMP to a location (rs1).
 * - SAC                : (Sync-And-Clear) synchronize all workers until all execute SAC and let the last idle worker reset all counters to 0.
 * - STP                : SToP the execution.
 * - WU     rs1,    rs2 : Wait Until a counting variable (rs1) to reach a desired value (rs2).
 */
typedef enum {
    ADV,
    ADV2,
    BIT,
    DU,
    EIT,
    EXE,
    INC,
    INC2,
    JMP,
    SAC,
    STP,
    WU,
} opcode_t;

typedef struct inst_t {
    opcode_t        op;
    long long int   rs1;
    long long int   rs2;
} inst_t;