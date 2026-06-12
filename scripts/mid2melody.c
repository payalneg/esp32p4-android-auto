/*
 * mid2melody — convert a Standard MIDI File into the VESC LISP melody list
 * consumed by `play-melody` in lisp/main.lisp.
 *
 * It emits a block of the exact shape that main.lisp already expects:
 *
 *     (def melody '(
 *       (330 0.124) (0 0.124) (440 0.248) ...
 *     ))
 *
 * where each entry is (freq-hz duration-seconds) and freq 0 is a rest.
 * The motor is a monophonic tone generator (foc-play-tone), so the whole
 * (possibly polyphonic, multi-track) MIDI is reduced to a single top voice:
 * at every instant we sound the highest currently-held note.
 *
 * The byte-level SMF parsing (running status, variable-length delta-times,
 * the track state machine) is done by abique/midi-parser (the submodule in
 * scripts/midi-parser). That parser deliberately does NOT decode tempo, does
 * NOT merge tracks and does NOT convert ticks to seconds — all of that
 * "musical" layer lives here.
 *
 * Build:
 *   cc -std=c11 -O2 -I scripts/midi-parser/include \
 *      scripts/midi-parser/src/midi-parser.c scripts/mid2melody.c \
 *      -lm -o scripts/mid2melody
 *
 * Usage:
 *   scripts/mid2melody song.mid [--name melody] [--track N] [--transpose S]
 *
 * With no --track it sounds the highest held note across ALL tracks, which for
 * a song split into lead+chords+bass tends to dip onto the bass during melody
 * rests. Pass --track N (0-based, see the per-track stats printed to stderr) to
 * lock onto a single lead track. --transpose S shifts every note S semitones
 * (e.g. 12 = up one octave) before the octave-fold, handy to lift a lead that
 * sits low for the motor.
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <midi-parser.h>

/* ---- tunables ------------------------------------------------------------ */
#define FREQ_LO        80.0    /* octave-fold notes into [FREQ_LO, FREQ_HI] so */
#define FREQ_HI        6000.0  /* every note stays in the motor's audible band */
#define MIN_NOTE_SEC   0.005   /* drop sub-5ms slivers (note-transition artefacts) */
#define MIN_REST_SEC   0.030   /* drop short rests (the gap from note-off/on retrigger) */
#define DEFAULT_TEMPO  500000  /* us per quarter note = 120 BPM, the SMF default */

/* ---- dynamic arrays ------------------------------------------------------ */
typedef struct { int64_t tick; int on; int note; int track; } NoteEv;
typedef struct { int64_t tick; uint32_t us_per_q; } TempoEv;

typedef struct { int64_t tick; int freq; } Seg; /* sounding freq over an interval; freq 0 = rest */

static NoteEv  *notes;   static size_t notes_n,  notes_cap;
static TempoEv *tempos;  static size_t tempos_n, tempos_cap;

static void push_note(int64_t tick, int on, int note, int track) {
    if (notes_n == notes_cap) { notes_cap = notes_cap ? notes_cap * 2 : 1024;
        notes = realloc(notes, notes_cap * sizeof *notes); }
    notes[notes_n++] = (NoteEv){ tick, on, note, track };
}
static void push_tempo(int64_t tick, uint32_t us) {
    if (tempos_n == tempos_cap) { tempos_cap = tempos_cap ? tempos_cap * 2 : 64;
        tempos = realloc(tempos, tempos_cap * sizeof *tempos); }
    tempos[tempos_n++] = (TempoEv){ tick, us };
}

static int cmp_note(const void *a, const void *b) {
    const NoteEv *x = a, *y = b;
    if (x->tick != y->tick) return x->tick < y->tick ? -1 : 1;
    return x->on - y->on;            /* note-off (0) before note-on (1) at same tick */
}
static int cmp_tempo(const void *a, const void *b) {
    const TempoEv *x = a, *y = b;
    return x->tick < y->tick ? -1 : x->tick > y->tick ? 1 : 0;
}

/* ---- tempo map: absolute tick -> absolute seconds ------------------------ */
/* cum[i] = seconds elapsed at tempos[i].tick (built once after sorting). */
static double *tempo_cum;
static int     division;          /* ticks per quarter note (metrical) */

static void build_tempo_cum(void) {
    /* guarantee a tempo entry at tick 0 */
    if (tempos_n == 0 || tempos[0].tick != 0) push_tempo(0, DEFAULT_TEMPO);
    qsort(tempos, tempos_n, sizeof *tempos, cmp_tempo);
    tempo_cum = malloc(tempos_n * sizeof *tempo_cum);
    tempo_cum[0] = 0.0;
    for (size_t i = 1; i < tempos_n; ++i) {
        int64_t dt = tempos[i].tick - tempos[i - 1].tick;
        tempo_cum[i] = tempo_cum[i - 1] +
            (double)dt * tempos[i - 1].us_per_q / division / 1e6;
    }
}
static double tick_to_sec(int64_t tick) {
    size_t j = 0;                      /* last tempo whose tick <= `tick` */
    for (size_t i = 1; i < tempos_n && tempos[i].tick <= tick; ++i) j = i;
    return tempo_cum[j] +
        (double)(tick - tempos[j].tick) * tempos[j].us_per_q / division / 1e6;
}

static int g_transpose = 0;

static int note_to_freq(int note) {
    double f = 440.0 * pow(2.0, (note + g_transpose - 69) / 12.0);
    while (f > FREQ_HI) f /= 2.0;
    while (f > 0 && f < FREQ_LO) f *= 2.0;
    return (int)lround(f);
}

int main(int argc, char **argv) {
    const char *path = NULL, *name = "melody";
    int want_track = -1;            /* -1 = all tracks (highest note wins) */
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--name") && i + 1 < argc) name = argv[++i];
        else if (!strcmp(argv[i], "--track") && i + 1 < argc) want_track = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--transpose") && i + 1 < argc) g_transpose = atoi(argv[++i]);
        else if (argv[i][0] != '-') path = argv[i];
    }
    if (!path) {
        fprintf(stderr, "usage: %s song.mid [--name NAME] [--track N] [--transpose S]\n", argv[0]);
        return 2;
    }

    /* slurp the whole file */
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return 1; }
    fseek(f, 0, SEEK_END); long fsz = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc(fsz);
    if (fread(buf, 1, fsz, f) != (size_t)fsz) { perror("fread"); return 1; }
    fclose(f);

    struct midi_parser p = { .state = MIDI_PARSER_INIT, .in = buf, .size = (int32_t)fsz };

    int64_t abs_tick = 0;
    int cur_track = -1;
    int tk_count[64] = {0}, tk_lo[64], tk_hi[64];
    for (int i = 0; i < 64; ++i) { tk_lo[i] = 127; tk_hi[i] = 0; }
    enum midi_parser_status st;
    while ((st = midi_parse(&p)) >= 0) {
        switch (st) {
        case MIDI_PARSER_HEADER:
            division = p.header.time_division;
            if (division <= 0) {            /* high bit set => SMPTE timing */
                fprintf(stderr, "error: SMPTE time division (0x%04x) not supported\n",
                        (uint16_t)division);
                return 1;
            }
            fprintf(stderr, "MIDI: %s, %d track(s), %d ticks/quarter\n",
                    midi_file_format_name(p.header.format),
                    p.header.tracks_count, division);
            break;
        case MIDI_PARSER_TRACK:             /* new track => restart absolute time */
            abs_tick = 0;
            ++cur_track;
            break;
        case MIDI_PARSER_TRACK_MIDI:
            abs_tick += p.vtime;
            if (p.midi.status == MIDI_STATUS_NOTE_ON && p.midi.param2 > 0) {
                push_note(abs_tick, 1, p.midi.param1, cur_track);
                if (cur_track >= 0 && cur_track < 64) {
                    int nt = p.midi.param1 & 127;
                    tk_count[cur_track]++;
                    if (nt < tk_lo[cur_track]) tk_lo[cur_track] = nt;
                    if (nt > tk_hi[cur_track]) tk_hi[cur_track] = nt;
                }
            } else if (p.midi.status == MIDI_STATUS_NOTE_OFF ||
                     (p.midi.status == MIDI_STATUS_NOTE_ON && p.midi.param2 == 0))
                push_note(abs_tick, 0, p.midi.param1, cur_track);
            break;
        case MIDI_PARSER_TRACK_META:
            abs_tick += p.vtime;
            if (p.meta.type == MIDI_META_SET_TEMPO && p.meta.length == 3)
                push_tempo(abs_tick, (p.meta.bytes[0] << 16) |
                                     (p.meta.bytes[1] << 8) | p.meta.bytes[2]);
            break;
        case MIDI_PARSER_TRACK_SYSEX:
            abs_tick += p.vtime;
            break;
        default: break;
        }
    }
    if (st == MIDI_PARSER_ERROR)
        fprintf(stderr, "warning: parse error before EOF — output may be truncated\n");

    for (int t = 0; t <= cur_track && t < 64; ++t)
        if (tk_count[t])
            fprintf(stderr, "  track %d: %d notes, pitch %d..%d%s\n", t, tk_count[t],
                    tk_lo[t], tk_hi[t], t == want_track ? "  <- selected" : "");

    if (want_track >= 0) {              /* keep only the chosen track */
        size_t k = 0;
        for (size_t i = 0; i < notes_n; ++i)
            if (notes[i].track == want_track) notes[k++] = notes[i];
        notes_n = k;
    }
    if (notes_n == 0) { fprintf(stderr, "error: no notes found\n"); return 1; }

    build_tempo_cum();
    qsort(notes, notes_n, sizeof *notes, cmp_note);

    /* sweep the timeline; the top held note sounds over each interval */
    Seg   *segs = NULL; size_t segs_n = 0, segs_cap = 0;
    int    held[128] = {0};
    int64_t prev_tick = notes[0].tick;     /* skip any leading silence */
    int    prev_top = -1;
    for (size_t i = 0; i < notes_n; ) {
        int64_t t = notes[i].tick;
        if (t > prev_tick) {
            if (segs_n == segs_cap) { segs_cap = segs_cap ? segs_cap * 2 : 1024;
                segs = realloc(segs, segs_cap * sizeof *segs); }
            segs[segs_n++] = (Seg){ prev_tick, prev_top < 0 ? 0 : note_to_freq(prev_top) };
            prev_tick = t;
        }
        for (; i < notes_n && notes[i].tick == t; ++i) {
            int nt = notes[i].note & 127;
            if (notes[i].on) held[nt]++;
            else if (held[nt] > 0) held[nt]--;
        }
        prev_top = -1;
        for (int n = 127; n >= 0; --n) if (held[n]) { prev_top = n; break; }
    }
    /* terminator seg so the last real segment gets a duration */
    if (segs_n == segs_cap) segs = realloc(segs, (segs_cap = segs_cap + 1) * sizeof *segs);
    segs[segs_n++] = (Seg){ prev_tick, 0 };

    /* convert to (freq, dur-seconds), drop slivers/short rests, merge equal freqs */
    typedef struct { int freq; double dur; } Out;
    Out *out = malloc(segs_n * sizeof *out); size_t out_n = 0;
    for (size_t i = 0; i + 1 < segs_n; ++i) {
        double dur = tick_to_sec(segs[i + 1].tick) - tick_to_sec(segs[i].tick);
        int freq = segs[i].freq;
        if (dur < MIN_NOTE_SEC) continue;
        if (freq == 0 && dur < MIN_REST_SEC) continue;
        if (out_n && out[out_n - 1].freq == freq) out[out_n - 1].dur += dur;  /* sustain */
        else out[out_n++] = (Out){ freq, dur };
    }

    /* emit the LISP block */
    double total = 0; for (size_t i = 0; i < out_n; ++i) total += out[i].dur;
    printf("; Generated by scripts/mid2melody from %s\n", path);
    printf("; %zu notes, %.1f s. Monophonic top-voice reduction.\n", out_n, total);
    printf("(def %s '(\n ", name);
    for (size_t i = 0; i < out_n; ++i) {
        printf(" (%d %.3f)", out[i].freq, out[i].dur);
        if ((i + 1) % 6 == 0 && i + 1 < out_n) printf("\n ");
    }
    printf("\n))\n");
    return 0;
}
