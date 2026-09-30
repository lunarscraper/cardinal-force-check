/* Cardinal-Prüfprogramm für die Akai Force (armhf).
 *   probe <plugin.so>          Stufe 1: nur laden (dlopen), fehlende Bibliotheken melden
 *   probe <plugin.so> --bench  Stufe 2: Plugin instanziieren, Parameter auflisten,
 *                              10 s Audio rechnen, CPU-Zeit und Speicher messen
 * Schreibt nichts, installiert nichts, fasst MPC nicht an. */
#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct AEffect AEffect;
typedef intptr_t (*audioMasterCallback)(AEffect *, int32_t, int32_t, intptr_t, void *, float);
struct AEffect {
    int32_t magic;
    intptr_t (*dispatcher)(AEffect *, int32_t, int32_t, intptr_t, void *, float);
    void (*process)(AEffect *, float **, float **, int32_t);
    void (*setParameter)(AEffect *, int32_t, float);
    float (*getParameter)(AEffect *, int32_t);
    int32_t numPrograms, numParams, numInputs, numOutputs, flags;
    intptr_t resvd1, resvd2;
    int32_t initialDelay, realQualities, offQualities;
    float ioRatio;
    void *object, *user;
    int32_t uniqueID, version;
    void (*processReplacing)(AEffect *, float **, float **, int32_t);
    void (*processDoubleReplacing)(AEffect *, double **, double **, int32_t);
    char future[56];
};
typedef struct { int32_t type, byteSize, deltaFrames, flags; char data[16]; } VstEvent;
typedef struct {
    int32_t type, byteSize, deltaFrames, flags, noteLength, noteOffset;
    unsigned char midiData[4];
    char detune, noteOffVelocity, reserved1, reserved2;
} VstMidiEvent;
typedef struct { int32_t numEvents; intptr_t reserved; VstEvent *events[2]; } VstEvents;
typedef struct {
    double samplePos, sampleRate, nanoSeconds, ppqPos, tempo, barStartPos, cycleStartPos, cycleEndPos;
    int32_t timeSigNumerator, timeSigDenominator, smpteOffset, smpteFrameRate, samplesToNextClock, flags;
} VstTimeInfo;

enum { effOpen = 0, effClose = 1, effGetParamName = 8, effSetSampleRate = 10, effSetBlockSize = 11,
       effMainsChanged = 12, effProcessEvents = 25, effGetEffectName = 45, effGetVendorString = 47 };
enum { amVersion = 1, amGetTime = 7, amGetSampleRate = 16, amGetBlockSize = 17,
       amGetVendorString = 32, amGetProductString = 33 };

#define SR 44100
#define BLOCK 128
static VstTimeInfo g_time;

static intptr_t host(AEffect *e, int32_t op, int32_t idx, intptr_t v, void *p, float o)
{
    (void)e; (void)idx; (void)v; (void)o;
    switch (op) {
    case amVersion: return 2400;
    case amGetTime: return (intptr_t)&g_time;
    case amGetSampleRate: return SR;
    case amGetBlockSize: return BLOCK;
    case amGetVendorString: if (p) strcpy(p, "Akai"); return 1;
    case amGetProductString: if (p) strcpy(p, "MPC"); return 1;
    default: return 0;
    }
}

static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static double cpu(void) { struct timespec t; clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t); return t.tv_sec + t.tv_nsec * 1e-9; }

static long rss_mb(void)
{
    FILE *f = fopen("/proc/self/status", "r");
    char l[256];
    long kb = -1;
    if (!f) return -1;
    while (fgets(l, sizeof l, f))
        if (!strncmp(l, "VmRSS:", 6)) kb = atol(l + 6);
    fclose(f);
    return kb / 1024;
}

static void note(AEffect *fx, int on, int n)
{
    VstMidiEvent m;
    VstEvents ev;
    memset(&m, 0, sizeof m);
    memset(&ev, 0, sizeof ev);
    m.type = 1;
    m.byteSize = sizeof m;
    m.midiData[0] = on ? 0x90 : 0x80;
    m.midiData[1] = (unsigned char)n;
    m.midiData[2] = on ? 100 : 0;
    ev.numEvents = 1;
    ev.events[0] = (VstEvent *)&m;
    fx->dispatcher(fx, effProcessEvents, 0, 0, &ev, 0);
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "Aufruf: probe <plugin.so> [--bench]\n"); return 2; }
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("== Stufe 1: Laden (dlopen)\n");
    double t0 = now();
    void *h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!h) { printf("ERGEBNIS: LADEN FEHLGESCHLAGEN\n  %s\n", dlerror()); return 1; }
    AEffect *(*entry)(audioMasterCallback) = (AEffect * (*)(audioMasterCallback)) dlsym(h, "VSTPluginMain");
    printf("ERGEBNIS: geladen in %.1f s, VSTPluginMain %s, Speicher %ld MB\n",
           now() - t0, entry ? "gefunden" : "FEHLT", rss_mb());
    if (!entry) return 1;
    if (argc < 3 || strcmp(argv[2], "--bench")) return 0;

    printf("\n== Stufe 2: Instanziieren und Benchmark\n");
    t0 = now();
    AEffect *fx = entry(host);
    if (!fx || fx->magic != 0x56737450) { printf("ERGEBNIS: kein gültiges Plugin erzeugt\n"); return 1; }
    fx->dispatcher(fx, effOpen, 0, 0, 0, 0);
    fx->dispatcher(fx, effSetSampleRate, 0, 0, 0, SR);
    fx->dispatcher(fx, effSetBlockSize, 0, BLOCK, 0, 0);
    fx->dispatcher(fx, effMainsChanged, 0, 1, 0, 0);
    char name[256] = "", vendor[256] = "";
    fx->dispatcher(fx, effGetEffectName, 0, 0, name, 0);
    fx->dispatcher(fx, effGetVendorString, 0, 0, vendor, 0);
    printf("Plugin: %s / %s, %d Ein- / %d Ausgänge, %d Parameter, Start %.1f s, Speicher %ld MB\n",
           name, vendor, fx->numInputs, fx->numOutputs, fx->numParams, now() - t0, rss_mb());
    for (int i = 0; i < fx->numParams && i < 40; i++) {
        char pn[256] = "";
        fx->dispatcher(fx, effGetParamName, i, 0, pn, 0);
        printf("  Parameter %2d: %s\n", i, pn);
    }

    int nin = fx->numInputs > 0 ? fx->numInputs : 1, nout = fx->numOutputs > 0 ? fx->numOutputs : 1;
    float **in = calloc(nin, sizeof *in), **out = calloc(nout, sizeof *out);
    for (int i = 0; i < nin; i++) in[i] = calloc(BLOCK, sizeof(float));
    for (int i = 0; i < nout; i++) out[i] = calloc(BLOCK, sizeof(float));
    g_time.sampleRate = SR;
    g_time.tempo = 145;
    g_time.timeSigNumerator = 4;
    g_time.timeSigDenominator = 4;
    g_time.flags = (1 << 1) | (1 << 9) | (1 << 10);   /* playing, ppq valid, tempo valid */

    int blocks = SR * 10 / BLOCK;
    double sumsq = 0, c0 = cpu();
    t0 = now();
    for (int b = 0; b < blocks; b++) {
        if (b % 86 == 0) note(fx, 1, 48 + (b / 86) % 12);
        if (b % 86 == 43) note(fx, 0, 48 + (b / 86) % 12);
        fx->processReplacing(fx, in, out, BLOCK);
        g_time.samplePos += BLOCK;
        g_time.ppqPos = g_time.samplePos / SR * g_time.tempo / 60.0;
        for (int i = 0; i < BLOCK; i++) sumsq += (double)out[0][i] * out[0][i];
    }
    double used = cpu() - c0, wall = now() - t0;
    printf("ERGEBNIS: 10 s Audio in %.2f s CPU (%.2f s Wanduhr) = %.0f %% eines Kerns, RMS %.4f, Speicher %ld MB\n",
           used, wall, used / 10.0 * 100.0, sqrt(sumsq / (blocks * (double)BLOCK)), rss_mb());
    printf("  (MPC läuft parallel, die Werte sind daher eher pessimistisch)\n");
    fx->dispatcher(fx, effMainsChanged, 0, 0, 0, 0);
    fx->dispatcher(fx, effClose, 0, 0, 0, 0);
    return 0;
}
