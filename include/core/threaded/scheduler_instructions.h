/**
 * @author Shaokai Lin <shaokai@berkeley.edu>
 * @brief Format of the instruction set
 */
typedef enum {
    ADD,
    ADDI,
    ADV,
    ADVI,
    BEQ,
    BGE,
    BIT,
    BLT,
    BNE,
    DU,
    EIT,
    EXE,
    JAL,
    JALR,
    SAC,
    STP,
    WLT,
    WU,
} opcode_t;

typedef struct inst_t {
    opcode_t op;
    uint64_t rs1;
    uint64_t rs2;
    uint64_t rs3;
} inst_t;