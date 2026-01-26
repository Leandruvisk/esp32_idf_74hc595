import csv
import re
from decimal import Decimal, getcontext

# aumenta precisão decimal
getcontext().prec = 28

INPUT = "digital.csv"
OUTPUT = "../main/capture_data.c"


timestamps = []
sck = []
rck = []
ser = []

with open(INPUT, newline='') as f:
    reader = csv.DictReader(f)
    rows = list(reader)

t0 = float(rows[0]["Time [s]"])

for r in rows:
    t = float(r["Time [s]"])
    timestamps.append(int((t - t0) * 1_000_000_000))  # ns
    sck.append(int(r["SCK"]))
    rck.append(int(r["RCK"]))
    ser.append(int(r["SER"]))

def write_array(f, name, ctype, data):
    f.write(f"const {ctype} {name}[] = {{\n")
    for i, v in enumerate(data):
        f.write(f"    {v},")
        if (i + 1) % 8 == 0:
            f.write("\n")
    f.write("\n};\n\n")

with open(OUTPUT, "w") as f:
    f.write("#include <stdint.h>\n\n")
    write_array(f, "timestamp_ns", "uint64_t", timestamps)
    write_array(f, "sck", "uint8_t", sck)
    write_array(f, "rck", "uint8_t", rck)
    write_array(f, "ser", "uint8_t", ser)
    f.write(f"const uint32_t SIGNAL_LEN = {len(timestamps)};\n")
