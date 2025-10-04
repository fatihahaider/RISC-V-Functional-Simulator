# ======================================================
# LOAD + STORE TEST (no pseudo)
# ======================================================

# Load address of data section
auipc x1, 0            # PC-relative upper imm = current page
addi  x1, x1, data_section - .   # x1 = &data_section

# Offsets for each variable
addi x2, x1, 0          # x2 -> num1
addi x3, x1, 8          # x3 -> num2
addi x4, x1, 16         # x4 -> res

ld x5, 0(x2)            # 5
ld x6, 0(x3)            # 10
add x7, x5, x6           # 15
sd x7, 0(x4)             # store result
ld x8, 0(x4)             # reload to check (expect 15)

.word 0xfeedfeed

.data
data_section:
    .dword 5
    .dword 10
    .dword 0
