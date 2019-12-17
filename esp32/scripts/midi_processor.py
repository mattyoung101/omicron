import argparse
from mido import MidiFile
import time

MAX_NOTES = 4

parser = argparse.ArgumentParser(description="Splits MIDI files into separate tracks and outputs a byte array for "
"running on the robot.")
parser.add_argument("-i", help="the input file", dest="input", type=str, required=True)
parser.add_argument("-o", help="the output file", dest="output", type=str, required=False)
args = parser.parse_args()

# We'll use a bit field, 16 bit integer, first 12 bits are frequency and final 4 bits are ID
# id is either 3 = note on, 4 = note off

# basically make 4 channels, for each time event in the midi, put the notes on separate channels
# if we cant - invalid midi! or just drop the extra note and do it for the first 4 only?

mid = MidiFile(args.input)
mid_out = MidiFile(args.input)
messages = [x for x in mid if x.type == "note_on" or x.type == "note_off"]

def note_to_frequency(m):
    return 27.5 * 2 ** ((m - 21) / 12.0)

out = f"// Generated by midi_processor.py (original file: {args.input})\n"
out += f"""const music_note_t SONG_{args.input.upper().split(".")[0]}[{len(messages)}] = {{"""

notes_out = []

for i, message in enumerate(messages):
    freq = int(note_to_frequency(message.note))
    time = int(message.time * 1000) # time in ms
    note_out = f"\x7b{int(message.type == 'note_on')}, {freq}, {time}\x7d"
    note_out += "\n" if i % 7 == 0 else ""
    notes_out.append(note_out)

out += ", ".join(notes_out)
out += "};"
print(out)