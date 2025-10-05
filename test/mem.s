# ======================================================
# EXTENDED MEMORY TEST
# ======================================================

addi x1, x0, 0x80        # Base address
addi x2, x0, 0x12        # Test value
sb x2, 0(x1)             # Store byte
lb x3, 0(x1)             # Load byte signed
lbu x4, 0(x1)            # Load byte unsigned

addi x5, x0, 0x1234
sh x5, 2(x1)
lh x6, 2(x1)
lhu x7, 2(x1)

addi x8, x0, 0x12345678
sw x8, 8(x1)
lw x9, 8(x1)
lwu x10, 8(x1)

addi x11, x0, 0x12345678
sd x11, 16(x1)
ld x12, 16(x1)

.word 0xfeedfeed
