/* Compare full bitstreams/PCM for serial, interleaved, and parallel instances, not private layouts. */
#include <lhdc/encoder/lhdc_bt.h>
#include <lhdc/decoder/decoder.h>
#include <ar/lhdc_pcm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

enum { CASES = 10, STEPS = 40, CAPACITY = 16384 };
typedef struct Output {
    int result;
    uint32_t frames, stream_bytes, pcm_bytes;
    uint8_t stream[CAPACITY], pcm[CAPACITY];
} Output;
typedef struct Instance {
    unsigned id, rate, bits, samples;
    void *encoder, *decoder, *ar_storage;
    LhdcArPcm ar;
    Output *expected;
} Instance;

/* Report instance/step failure reasons and terminate the test. */
static void fail(unsigned id, unsigned step, const char *reason) {
    fprintf(stderr, "FAIL instance=%u step=%u %s\n", id, step, reason);
    exit(1);
}

/* Release this instance's resources without affecting other active instances. */
static void close_instance(Instance *s) {
    if (s->encoder) {
        if (s->id < 4)
            lhdcBT_free_handle(s->encoder);
        else
            lhdcv5BT_free_handle(s->encoder);
    }
    if (s->decoder) {
        if (s->id < 4)
            lhdcDestroy(s->decoder);
        else
            lhdcv5_util_dec_destroy(s->decoder);
    }
    if (s->ar_storage)
        lhdc_ar_pcm_uninit(&s->ar);
    free(s->decoder);
    free(s->ar_storage);
    s->encoder = s->decoder = s->ar_storage = NULL;
}

/* Create encoder, decoder, or standalone AR instances in different formats. */
static void open_instance(Instance *s) {
    static const unsigned rates[CASES] = {48000, 96000, 44100, 48000, 44100,
                                          96000, 48000, 48000, 48000, 48000};
    static const unsigned depths[CASES] = {16, 24, 16, 24, 24, 16, 16, 24, 16, 24};
    s->rate = rates[s->id];
    s->bits = depths[s->id];
    uint32_t size = 0;
    if (s->id < 4) {
        unsigned version = s->id < 2 ? 3 : 4;
        s->encoder = lhdcBT_get_handle(version);
        if (!s->encoder || lhdcBT_init_encoder(s->encoder, s->rate, s->bits, 3, 0, 0, 660, 10))
            fail(s->id, 0, "create v4");
        s->samples = lhdcBT_get_block_Size(s->encoder);
        if (lhdc_util_dec_get_mem_req(version == 3 ? 400 : 500, &size))
            fail(s->id, 0, "size v4");
        s->decoder = calloc(1, size);
        if (!s->decoder)
            fail(s->id, 0, "allocate decoder");
        lhdcInit(s->decoder, s->bits, s->rate, 400000, version == 3 ? 400 : 500);
    } else if (s->id < 6) {
        if (lhdcv5BT_get_handle(1, &s->encoder) ||
            lhdcv5BT_init_encoder(s->encoder, s->rate, s->bits, 3, 660, 10) ||
            lhdcv5BT_get_block_Size(s->encoder, &s->samples) ||
            lhdcv5_util_dec_get_mem_req(550, s->rate, 0, 0, &size))
            fail(s->id, 0, "create v5");
        s->decoder = calloc(1, size);
        if (!s->decoder || lhdcv5_util_init_decoder(s->decoder, s->bits, s->rate, 0, 50, 0, 550))
            fail(s->id, 0, "init v5 decoder");
    } else {
        LhdcArProfile profile = s->id < 8 ? LHDC_AR_PROFILE_V4 : LHDC_AR_PROFILE_V5;
        size_t bytes = lhdc_ar_pcm_workspace_bytes(profile);
        s->ar_storage = calloc(1, bytes + 16);
        void *aligned = (void *)(((uintptr_t)s->ar_storage + 15) & ~(uintptr_t)15);
        LhdcArPcmConfig config = {s->rate, s->bits};
        if (!s->ar_storage || lhdc_ar_pcm_init(&s->ar, profile, aligned, bytes, &config))
            fail(s->id, 0, "create AR");
        s->samples = 240;
    }
}

/* Generate schedule-independent PCM and compare complete bitstream/decoded/effects output batches. */
static void step_instance(Instance *s, unsigned step, int record) {
    if (step == 19 && (s->id & 1)) {
        close_instance(s);
        open_instance(s);
    }
    uint8_t input[8192] = {0}, decoded[CAPACITY] = {0};
    unsigned input_bytes = s->samples * 2 * (s->bits / 8);
    uint32_t random = 7919u * (s->id + 1) + 104729u * step;
    for (unsigned i = 0; i < input_bytes; ++i) {
        random = random * 1664525u + 1013904223u;
        input[i] = step % 7 ? (uint8_t)(random >> 24) : 0;
    }
    Output *out = calloc(1, sizeof *out);
    if (!out)
        fail(s->id, step, "allocate record");
    if (s->id < 4)
        out->result =
            lhdcBT_encodeV3(s->encoder, input, out->stream, &out->stream_bytes, &out->frames);
    else if (s->id < 6)
        out->result = lhdcv5BT_encode(s->encoder, input, input_bytes, out->stream, CAPACITY,
                                      &out->stream_bytes, &out->frames);
    else {
        out->result = lhdc_ar_pcm_process(&s->ar, input, out->pcm, input_bytes);
        out->pcm_bytes = input_bytes;
    }
    if (out->result < 0 || out->stream_bytes > CAPACITY)
        fail(s->id, step, "encode");
    unsigned position = 0;
    while (position < out->stream_bytes) {
        uint32_t length = 0, written = 0;
        if (s->id < 4) {
            LhdcV4FrameInfo info;
            if (!lhdcFetchFrameInfo(s->decoder, out->stream + position, &info))
                fail(s->id, step, "parse v4");
            length = info.frame_len;
        } else {
            LhdcV5FrameInfo info;
            if (lhdcv5_util_dec_fetch_frame_info(s->decoder, out->stream + position,
                                                 out->stream_bytes - position, &info))
                fail(s->id, step, "parse v5");
            length = info.frame_len;
        }
        if (!length || length > out->stream_bytes - position)
            fail(s->id, step, "frame length");
        if (s->id < 4)
            written = lhdcDecodeProcess(s->decoder, decoded, out->stream + position, length);
        else if (lhdcv5_util_dec_process(s->decoder, decoded, out->stream + position, length,
                                         &written))
            fail(s->id, step, "decode v5");
        if (!written || written + out->pcm_bytes > CAPACITY)
            fail(s->id, step, "decoded bytes");
        memcpy(out->pcm + out->pcm_bytes, decoded, written);
        out->pcm_bytes += written;
        position += length;
    }
    if (record)
        s->expected[step] = *out;
    else if (memcmp(&s->expected[step], out, sizeof *out))
        fail(s->id, step, "cross-instance interference");
    free(out);
}

#ifdef _WIN32
/* Run the specified instance in a thread, check output, and maintain its state. */
static DWORD WINAPI worker(void *ptr) {
#else
/* Run the specified instance in a thread, check output, and maintain its state. */
static void *worker(void *ptr) {
#endif
    Instance *s = ptr;
    open_instance(s);
    for (unsigned step = 0; step < STEPS; ++step)
        step_instance(s, step, 0);
    close_instance(s);
    return 0;
}

/* Compare encoded bytes/PCM across serial, interleaved, and parallel instance execution. */
int main(void) {
    Instance states[CASES] = {0};
    for (unsigned i = 0; i < CASES; ++i) {
        states[i].id = i;
        states[i].expected = calloc(STEPS, sizeof(Output));
        if (!states[i].expected)
            return 2;
        open_instance(&states[i]);
        for (unsigned step = 0; step < STEPS; ++step)
            step_instance(&states[i], step, 1);
        close_instance(&states[i]);
    }
    for (unsigned i = 0; i < CASES; ++i)
        open_instance(&states[i]);
    for (unsigned step = 0; step < STEPS; ++step)
        for (unsigned i = 0; i < CASES; ++i)
            step_instance(&states[i], step, 0);
    for (unsigned i = 0; i < CASES; ++i)
        close_instance(&states[i]);
#ifdef _WIN32
    HANDLE threads[CASES];
    for (unsigned i = 0; i < CASES; ++i) {
        threads[i] = CreateThread(NULL, 0, worker, &states[i], 0, NULL);
        if (!threads[i])
            return 3;
    }
    WaitForMultipleObjects(CASES, threads, TRUE, INFINITE);
    for (unsigned i = 0; i < CASES; ++i)
        CloseHandle(threads[i]);
#else
    pthread_t threads[CASES];
    for (unsigned i = 0; i < CASES; ++i)
        if (pthread_create(&threads[i], NULL, worker, &states[i]))
            return 3;
    for (unsigned i = 0; i < CASES; ++i)
        pthread_join(threads[i], NULL);
#endif
    for (unsigned i = 0; i < CASES; ++i)
        free(states[i].expected);
    printf(
        "PASS %u instances, %u calls per schedule, serial/interleaved/parallel exact streams and PCM\n",
        CASES, CASES * STEPS);
    return 0;
}
