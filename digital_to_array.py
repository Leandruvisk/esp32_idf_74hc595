import csv
from datetime import datetime

INPUT_CSV = "digital.csv"
OUTPUT_C = "main/hc595_sequence.c"

events = []

with open(INPUT_CSV, newline="") as f:
    reader = csv.DictReader(f)
    last_time = None

    for row in reader:
        t = datetime.fromisoformat(row["Time [s]"])

        if last_time is None:
            delta_us = 0
        else:
            delta_us = int((t - last_time).total_seconds() * 1_000_000)

        last_time = t

        events.append((
            int(row["SER"]),
            int(row["CLK"]),
            int(row["RCK"]),
            delta_us
        ))

with open(OUTPUT_C, "w") as f:
    f.write(
        "#include <stdint.h>\n\n"
        "typedef struct {\n"
        "    uint8_t ser;\n"
        "    uint8_t clk;\n"
        "    uint8_t rck;\n"
        "    uint32_t delay_us;\n"
        "} hc595_step_t;\n\n"
    )

    f.write("const hc595_step_t hc595_sequence[] = {\n")
    for e in events:
        f.write(f"    {{{e[0]}, {e[1]}, {e[2]}, {e[3]}}},\n")
    f.write("};\n\n")

    f.write(
        "const uint32_t hc595_sequence_len = "
        "sizeof(hc595_sequence) / sizeof(hc595_sequence[0]);\n"
    )

print(f"Arquivo gerado: {OUTPUT_C}")
