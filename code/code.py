import board
import time
import displayio
import gamepadshift
import adafruit_imageload

MODE_LIVE = 0
MODE_MENU1 = 1
MODE_MENU2 = 2

SOUND_TYPE_SQUARE = 0
SOUND_TYPE_PULSE = 1
SOUND_TYPE_SINE = 2
SOUND_TYPE_NOISE = 3

class PitchEnvelope():
    pass

class AmpEnvelope():
    pass

class SoundBlock():
    index = -1
    note = None
    intervals = [0, 0, 0]
    interval_mode = 0
    octave = 3
    pitch_env = PitchEnvelope()
    amp_env = AmpEnvelope()
    delay = 0
    pan = 0
    def __init__(self, index):
        self.index = index

class Sound():
    name = None
    type = None
    blocks = []
    rate = 8 # 8 for normal, 4 for half, 2 for quarter, 1 for eigth

    def __init__(self, machine, name, type):
        self.machine = machine
        self.name = name
        self.type = type
        for i in range(16):
            self.blocks.append(SoundBlock(i))
    @property
    def playing_block(self):
        return self.machine.current_tick // self.rate

class BleepBloopMachine():
    mode = MODE_LIVE
    bpm = 120
    current_tick = 0
    max_tick = 127
    selected_live_block = 0
    selected_menu1_control = 0
    selected_menu2_control = 0
    current_sound_index = 0
    sounds = []
    @property
    def tick_length(self):
        return self.bpm / self.max_tick / 16
    @property
    def current_sound(self):
        return self.sounds[self.current_sound_index]
    def tick(self):
        time.sleep(self.tick_length)
        self.current_tick += 1
        if self.current_tick > 127:
            self.current_tick = 0
    def __init__(self):
        self.sounds.append(Sound(self, "q", SOUND_TYPE_SQUARE))
        self.sounds.append(Sound(self, "p", SOUND_TYPE_PULSE))
        self.sounds.append(Sound(self, "s", SOUND_TYPE_SINE))
        self.sounds.append(Sound(self, "n", SOUND_TYPE_NOISE))

machine = BleepBloopMachine()

display = board.DISPLAY

colorsheet, palette = adafruit_imageload.load(
    "colors.bmp",
    bitmap=displayio.Bitmap,
    palette=displayio.Palette,
)
grid = displayio.TileGrid(
    colorsheet,
    pixel_shader=palette,
    width=10,
    height=8,
    tile_width=16,
    tile_height=16,
)
grid_group = displayio.Group()
grid_group.append(grid)
display.show(grid_group)

sound = machine.current_sound
while True:
    sound = machine.current_sound
    for y in range(0, 4):
        for x in range(0, 4):
            color = 1
            if x == sound.playing_block % 4 and y == sound.playing_block // 4:
                color = 2
            grid[x + 3, y + 2] = color
            print(sound.playing_block)
            print(x, y, sound.playing_block / 4)
    machine.tick()
