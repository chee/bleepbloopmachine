import board
import time
import displayio
import adafruit_imageload
import gamepadshift
from freq import freq
import audioio
import digitalio
import analogio
from adafruit_waveform import sine, square
import audiomixer
import audiocore
from adafruit_display_text import label
import terminalio

font = terminalio.FONT

block_labels = []

def make_label(index):
    l = label.Label(terminalio.FONT, text="   ", color=0x0)
    l.y = ((2 + (index // 4)) * 16) + 6
    l.x = ((3 + (index % 4)) * 16) + 2
    return l

for i in range(0, 16):
    block_labels.append(make_label(i))


display = board.DISPLAY

colorsheet, palette = adafruit_imageload.load(
    "colors.bmp",
    bitmap=displayio.Bitmap,
    palette=displayio.Palette,
)

palette.make_transparent(2)

grid = displayio.TileGrid(
    colorsheet,
    pixel_shader=palette,
    width=10,
    height=8,
    tile_width=16,
    tile_height=16,
)

group = displayio.Group()

select_box = displayio.TileGrid(
    colorsheet,
    pixel_shader=palette,
    width=10,
    height=8,
    tile_width=16,
    tile_height=16,
    default_tile=11
)

group.append(grid)
group.append(select_box)

block_label_group_1 = displayio.Group()
block_label_group_2 = displayio.Group()
block_label_group_3 = displayio.Group()
block_label_group_4 = displayio.Group()
for i in range(0, 4):
    block_label_group_1.append(block_labels[i])
    block_label_group_2.append(block_labels[i + 4])
    block_label_group_3.append(block_labels[i + 8])
    block_label_group_4.append(block_labels[i + 12])

block_label_group = displayio.Group()
block_label_group.append(block_label_group_1)
block_label_group.append(block_label_group_2)
block_label_group.append(block_label_group_3)
block_label_group.append(block_label_group_4)
group.append(block_label_group)
display.show(group)

def wrapping_add(cur, max, inc=1, min=0):
    nxt = cur + inc
    return nxt if nxt <= max else min

def wrapping_sub(cur, max, inc=1, min=0):
    nxt = cur - inc
    return nxt if nxt >= min else max

MODE_LIVE = 0
MODE_MENU1 = 1
MODE_MENU2 = 2

WAVE_TYPE_SQUARE = 0
WAVE_TYPE_PULSE = 1
WAVE_TYPE_SINE = 2
WAVE_TYPE_NOISE = 3

class Pad():
    K_B = 0x01
    K_A = 0x02
    K_START = 0x04
    K_SELECT = 0x08
    K_DOWN = 0x10
    K_LEFT = 0x20
    K_RIGHT = 0x40
    K_UP = 0x80
    def __init__(self):
        self.buttons = gamepadshift.GamePadShift(
            digitalio.DigitalInOut(board.BUTTON_CLOCK),
            digitalio.DigitalInOut(board.BUTTON_OUT),
            digitalio.DigitalInOut(board.BUTTON_LATCH),
        )
        self.joy_x = analogio.AnalogIn(board.JOYSTICK_X)
        self.joy_y = analogio.AnalogIn(board.JOYSTICK_Y)
    def get_pressed(self):
        pressed = self.buttons.get_pressed()
        dead = 15000
        x = self.joy_x.value - 32767
        if x < -dead:
            pressed |= self.K_LEFT
        elif x > dead:
            pressed |= self.K_RIGHT
        y = self.joy_y.value - 32767
        if y < -dead:
            pressed |= self.K_UP
        elif y > dead:
            pressed |= self.K_DOWN
        return pressed

audio = audioio.AudioOut(left_channel=board.A0, right_channel=board.A1)

mixer = audiomixer.Mixer(voice_count=4, channel_count=2, samples_signed=False)

audio.play(mixer)

class PitchEnvelope():
    def start(self, buffy):
        pass
    def tick(self):
        pass

class AmpEnvelope():
    # attack and decay are in ticks
    attack = 10
    decay = 20
    current = 0
    start_vol = 0.1
    peak_vol = 1.0
    state = "attacking"
    attack_mul = 0
    decay_mul = 0
    def start(self, buffy):
        self.current = 0
        self.state = "attacking"
        self.buffer = buffy
    def tick(self):
        for i in range(len(self.buffer)):
            self.buffer[i] += 10


notes = ["c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b"]
octaves = ["0", "1", "2", "3", "4", "5", "6", "7", "8"]

class SoundBlock():
    index = -1
    note = 0
    intervals = [0, 0, 0]
    interval_mode = 0
    octave = 3
    pitch_env = PitchEnvelope()
    amp_env = AmpEnvelope()
    delay = 0
    pan = 0
    samplerate = 4000
    form = None
    buffer = None
    def __init__(self, index):
        self.index = index
    @property
    def label(self):
        return block_labels[self.index]
    def update_label(self, mode=MODE_LIVE):
        self.label.text = notes[self.note] + octaves[self.octave] if self.has_sound else ""
    @property
    def has_sound(self):
        return self.form is not None
    @property
    def freq(self):
        return freq[notes[self.note] + octaves[self.octave]]
    def clear(self):
        self.form = None
        self.update_label()
    def inc_note(self):
        self.note = wrapping_add(self.note, len(notes) - 1)
        self.update_label()
        self.make_form()
    def dec_note(self):
        self.note = wrapping_sub(self.note, len(notes) - 1)
        self.update_label()
        self.make_form()
    def inc_octave(self):
        self.octave = wrapping_add(self.octave, len(octaves) - 1)
        self.update_label()
        self.make_form()
    def dec_octave(self):
        self.octave = wrapping_sub(self.octave, len(octaves) - 1)
        self.update_label()
        self.make_form()
    def start(self):
        self.amp_env.start(self.buffer)
        self.pitch_env.start(self.buffer)
    def tick(self):
        self.amp_env.tick()
        self.pitch_env.tick()
    def make_form(self):
        self.buffer = sine.sine_wave(8000, self.freq)
        self.form = audiocore.RawSample(self.buffer, channel_count=2)

class Wave():
    name = None
    type = None
    sounds = []
    rate = 1
    index = -1
    audible_sound_index = -1
    def __init__(self, machine, name, type, index):
        self.machine = machine
        self.name = name
        self.type = type
        self.index = index
        for i in range(16):
            self.sounds.append(SoundBlock(i))
    @property
    def voice(self):
        return mixer.voice[self.index]
    def play(self):
        if self.tick_block.has_sound:
            self.voice.stop()
            self.voice.play(self.tick_block.form, loop=True)
            self.tick_block.start()
            self.audible_sound_index = self.tick_block_index
    def tick(self):
        if self.audible_sound_index > -1:
            self.sounds[self.audible_sound_index].tick()

    @property
    def tick_block_index(self):
        mod = 32
        div = 2
        return (self.machine.current_tick % (mod * self.rate)) // (div * self.rate)
    @property
    def tick_block(self):
        return self.sounds[self.tick_block_index]

class BleepBloopMachine():
    mode = MODE_LIVE
    bpm = 120
    current_tick = 0
    max_tick = 255
    selected_sound_index = 0
    selected_menu1_control = 0
    selected_menu2_control = 0
    selected_wave_index = 0
    waves = []
    last_keys = 0
    previous_rx = 3
    previous_ry = 2
    @property
    def selected_wave(self):
        return self.waves[self.selected_wave_index]
    @property
    def tick_length(self):
        return self.bpm / self.max_tick / 16
    @property
    def tick_block(self):
        return self.selected_wave.tick_block
    @property
    def selected_sound(self):
        return self.selected_wave.sounds[self.selected_sound_index]
    def coord(self, index):
        return (index % 4) + 3, (index // 4) + 2
    def update_tick_block_background(self):
        rx, ry = self.coord(self.selected_wave.tick_block_index)
        if self.previous_rx != rx or self.previous_ry != ry:
            grid[rx, ry] = 2
            grid[self.previous_rx, self.previous_ry] = 1
            self.previous_rx = rx
            self.previous_ry = ry
    def tick(self):
        time.sleep(self.tick_length)
        self.current_tick += 1
        self.update_tick_block_background()
        for wave in self.waves:
            wave.tick()
    def index_is_coordinate(self, index, x, y):
        return x == index % 4 and y == index // 4
    def change_selected_sound(self, direction):
        select_box[self.coord(self.selected_sound_index)] = 0xb
        if direction == "down" and self.selected_sound_index < 0xc:
            self.selected_sound_index += 4
        elif direction == "up" and self.selected_sound_index > 0x3:
            self.selected_sound_index -= 4
        elif direction == "left" and self.selected_sound_index > 0x0:
            self.selected_sound_index -= 1
        elif direction == "right" and self.selected_sound_index < 0xf:
            self.selected_sound_index += 1
        select_box[self.coord(self.selected_sound_index)] = 0x9
    def keypress(self, keys):
        ab_mode = None
        if keys & self.pad.K_B & self.pad.K_A:
            ab_mode = "both"
        elif keys & self.pad.K_B:
            ab_mode = "b"
        elif keys & self.pad.K_A:
            ab_mode = "a"
        else:
            ab_mode = None
        if self.mode == MODE_LIVE:
            if ab_mode and ab_mode == "b":
                if keys & self.pad.K_LEFT:
                    if self.last_keys & self.pad.K_LEFT:
                        return
                    self.selected_sound.dec_octave()
                elif keys & self.pad.K_RIGHT:
                    if self.last_keys & self.pad.K_RIGHT:
                        return
                    self.selected_sound.inc_octave()
                elif keys & self.pad.K_UP:
                    if self.last_keys & self.pad.K_UP:
                        return
                    self.selected_sound.inc_note()
                elif keys & self.pad.K_DOWN:
                    if self.last_keys & self.pad.K_DOWN:
                        return
                    self.selected_sound.dec_note()
                else:
                    self.selected_sound.update_label()
                    self.selected_sound.make_form()
            elif ab_mode and ab_mode == "a":
                pass
            elif ab_mode and ab_mode == "both":
                pass
            else:
                if keys & self.pad.K_LEFT:
                    if self.last_keys & self.pad.K_LEFT:
                        return
                    self.change_selected_sound("left")
                if keys & self.pad.K_RIGHT:
                    if self.last_keys & self.pad.K_RIGHT:
                        return
                    self.change_selected_sound("right")
                if keys & self.pad.K_UP:
                    if self.last_keys & self.pad.K_UP:
                        return
                    self.change_selected_sound("up")
                if keys & self.pad.K_DOWN:
                    if self.last_keys & self.pad.K_DOWN:
                        return
                    self.change_selected_sound("down")
        self.last_keys = keys

    def __init__(self):
        self.waves.append(Wave(self, "q", WAVE_TYPE_SQUARE, 0))
        #self.waves.append(Wave(self, "p", WAVE_TYPE_PULSE))
        self.waves.append(Wave(self, "s", WAVE_TYPE_SINE, 1))
        #self.waves.append(Wave(self, "n", WAVE_TYPE_NOISE))
        self.pad = Pad()

machine = BleepBloopMachine()

while True:
    machine.selected_wave.play()
    machine.keypress(machine.pad.get_pressed())
    machine.tick()
