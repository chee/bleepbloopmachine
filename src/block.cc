#include "block.h"

float freq[12][9] = {
    {
        16.3516,
        32.7032,
        65.40639,
        130.8128,
        261.6256,
        523.2511,
        1046.502,
        2093.005,
        4186.009,
    },
    {
        17.32391,
        34.64783,
        69.29566,
        138.5913,
        277.1826,
        554.3653,
        1108.731,
        2217.461,
        4434.922,
    },
    {
        18.35405,
        36.7081,
        73.41619,
        146.8324,
        293.6648,
        587.3295,
        1174.659,
        2349.318,
        4698.636,
    },
    {
        19.44544,
        38.89087,
        77.78175,
        155.5635,
        311.127,
        622.254,
        1244.508,
        2489.016,
        4978.032,
    },
    {
        20.60172,
        41.20344,
        82.40689,
        164.8138,
        329.6276,
        659.2551,
        1318.51,
        2637.02,
        5274.041,
    },
    {
        21.82676,
        43.65353,
        87.30706,
        174.6141,
        349.2282,
        698.4565,
        1396.913,
        2793.826,
        5587.652,
    },
    {
        23.12465,
        46.2493,
        92.49861,
        184.9972,
        369.9944,
        739.9888,
        1479.978,
        2959.955,
        5919.911,
    },
    {
        24.49971,
        48.99943,
        97.99886,
        195.9977,
        391.9954,
        783.9909,
        1567.982,
        3135.963,
        6271.927,
    },
    {
        25.95654,
        51.91309,
        103.8262,
        207.6523,
        415.3047,
        830.6094,
        1661.219,
        3322.438,
        6644.875,
    },
    {
        27.5,
        55.0,
        110.0,
        220.0,
        440.0,
        880.0,
        1760.0,
        3520.0,
        7040.0,
    },
    {
        29.13524,
        58.27047,
        116.5409,
        233.0819,
        466.1638,
        932.3275,
        1864.655,
        3729.31,
        7458.62,
    },
    {
        30.86771,
        61.73541,
        123.4708,
        246.9417,
        493.8833,
        987.7666,
        1975.533,
        3951.066,
        7902.133,
    },
};

float SoundBlock::frequency() {
	return freq[note][octave];
}
