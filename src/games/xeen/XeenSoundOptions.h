#ifndef MMODERN_XEEN_SOUND_OPTIONS_H
#define MMODERN_XEEN_SOUND_OPTIONS_H
namespace mmodern {
// Party values, independent of the program's song buffer and playback state.
struct XeenSoundOptions {
    bool efx=true, music=true, text=false, speech=true;
    void panelRedraw() noexcept { if(!speech)text=true; }
    void toggle(unsigned button) noexcept {
        switch(button) {
        case 0: efx=!efx;break;
        case 1: music=!music;break;
        case 6: text=!text;break;
        case 7: speech=!speech;break;
        }
        panelRedraw();
    }
    friend bool operator==(XeenSoundOptions a,XeenSoundOptions b) noexcept {
        return a.efx==b.efx && a.music==b.music && a.text==b.text && a.speech==b.speech;
    }
    friend bool operator!=(XeenSoundOptions a,XeenSoundOptions b) noexcept {return !(a==b);}
};
}
#endif
