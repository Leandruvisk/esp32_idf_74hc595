import csv
from collections import defaultdict, Counter

CSV_FILE = "sinal.csv"
SHIFT_LEN = 16
MIN_REPEAT = 2

def zeros(b):
    return bin(b & 0xFF).count("0")

def is_blank(b):
    return (b & 0xFF) == 0xFF

def is_enable(b):
    return 1 <= zeros(b) <= 2

def is_segment(b):
    return zeros(b) >= 3


shift_reg = [0] * SHIFT_LEN
latched = []

last_clk = 0
last_rck = 0

# ===== leitura =====
with open(CSV_FILE, newline="") as f:
    reader = csv.DictReader(f)
    for row in reader:
        ser = int(row["SER"])
        clk = int(row["CLK"])
        rck = int(row["RCK"])

        if last_clk == 0 and clk == 1:
            shift_reg = shift_reg[1:] + [ser]

        if last_rck == 0 and rck == 1:
            v = 0
            for b in shift_reg:
                v = (v << 1) | b
            latched.append(((v >> 8) & 0xFF, v & 0xFF))

        last_clk = clk
        last_rck = rck


# ===== filtro REAL =====
digits = []
last = None
cnt = 0

for a, b in latched:

    # precisa existir pelo menos um byte de segmento
    if not (is_segment(a) or is_segment(b)):
        continue

    # descarta enable + nada
    if is_enable(a) and is_blank(b):
        continue
    if is_enable(b) and is_blank(a):
        continue

    pair = (a, b)

    if pair == last:
        cnt += 1
    else:
        if last and cnt >= MIN_REPEAT:
            digits.append(last)
        last = pair
        cnt = 1

if last and cnt >= MIN_REPEAT:
    digits.append(last)


# ===== resultado =====
print("=== DIGITOS DETECTADOS ===")
for d in digits:
    print(f"0x{d[0]:02X} 0x{d[1]:02X}")

print("\n=== FREQUÊNCIA ===")
for k, v in Counter(digits).most_common():
    print(f"0x{k[0]:02X} 0x{k[1]:02X} -> {v}x")

enable_map = defaultdict(list)

for a, b in digits:
    if is_enable(a) and is_segment(b):
        enable_map[a].append(b)
    elif is_enable(b) and is_segment(a):
        enable_map[b].append(a)

print("=== POR DÍGITO (enable → segmentos) ===")
for en, segs in enable_map.items():
    print(f"\nENABLE 0x{en:02X}")
    for s, c in Counter(segs).most_common():
        print(f"  SEG 0x{s:02X} -> {c}x")


# gera vetores 16-bit para testar segmentos no 74HC595

ENABLE_LEFT  = 0x7F   # enable que você já sabe que existe
ENABLE_RIGHT = 0xFE

vectors = []

print("=== TESTE BYTE A BYTE ===")

# testa cada bit do byte de segmentos
for bit in range(8):
    seg = ~(1 << bit) & 0xFF   # ativo em LOW (padrão provável)

    # enable no MSB
    v1 = (ENABLE_LEFT << 8) | seg
    vectors.append(v1)

    # enable no LSB
    v2 = (seg << 8) | ENABLE_LEFT
    vectors.append(v2)

# dump em formato C
print("\n=== ARRAY C ===\n")
print("static const uint16_t test_seq[] = {")
for v in vectors:
    print(f"    0x{v:04X},")
print("};")

print(f"\n#define TEST_SEQ_LEN {len(vectors)}")
