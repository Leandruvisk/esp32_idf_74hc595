import csv

arquivo = "digital.csv"

prev_clk = 0
prev_rck = 0

shift_bits = []
frames = []

with open(arquivo, newline='') as f:
    reader = csv.DictReader(f)
    for row in reader:
        ser = int(row["SER"])
        clk = int(row["CLK"])
        rck = int(row["RCK"])

        # Borda de subida do CLK → shift
        if prev_clk == 0 and clk == 1:
            shift_bits.append(ser)

        # Borda de subida do RCK → latch
        if prev_rck == 0 and rck == 1 and shift_bits:
            frames.append(shift_bits.copy())
            shift_bits.clear()

        prev_clk = clk
        prev_rck = rck


def bits_to_int(bits, msb_first=True):
    if not msb_first:
        bits = bits[::-1]
    value = 0
    for b in bits:
        value = (value << 1) | b
    return value


print("\n===== FRAMES 74HC595 =====\n")

for i, bits in enumerate(frames):
    val = bits_to_int(bits, msb_first=True)
    print(f"Frame {i}")
    print(" Bits :", ''.join(str(b) for b in bits))
    print(f" HEX  : 0x{val:02X}")
    print(f" DEC  : {val}")
    print("-" * 30)
