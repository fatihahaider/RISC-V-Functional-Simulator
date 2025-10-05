# ======================================================
# LWU TEST
# ======================================================

# Initialize base address
lui x1, 0x0          # x1 = 0x00000000
addi x1, x1, 64      # memory base address = 0x40

# Store a 32-bit value: 0xFFFFFFFF (unsigned 4294967295)
addi x2, x0, -1      # x2 = 0xFFFFFFFFFFFFFFFF
sw   x2, 0(x1)       # store lower 4 bytes (0xFFFFFFFF)

# Load same memory using signed LW and unsigned LWU
lw   x3, 0(x1)       # sign-extend → 0xFFFFFFFFFFFFFFFF (-1)
lwu  x4, 0(x1)       # zero-extend → 0x00000000FFFFFFFF (4294967295)

# For clarity: difference should be visible in your debug print
# Expected:
#   x3 = 0xFFFFFFFFFFFFFFFF  -1 signed
#   x4 = 0x00000000FFFFFFFF  4294967295 unsigned

.word 0xfeedfeed
