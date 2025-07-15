#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define SAMPLE_RATE 44100
#define VOLUME 3000       // Amplitude do som
#define DURATION 0.3      // Duração de cada nota em segundos
#define SILENCE 0.05      // Pausa entre notas (em segundos)
#define MAX_NOTES 3       // Máximo: acorde com 3 notas

// Mapeamento para 0-9 (acordes) e A-H (notas)
double note_freq(uint8_t note) {
    return 440.0 * pow(2.0, (note - 69) / 12.0);  // MIDI to Hz
}

uint8_t chord_notes(char ch, uint8_t *out) {
    switch (ch) {
        case '0': out[0]=60; out[1]=64; out[2]=67; return 3;
        case '1': out[0]=62; out[1]=65; out[2]=69; return 3;
        case '2': out[0]=64; out[1]=67; out[2]=71; return 3;
        case '3': out[0]=65; out[1]=69; out[2]=72; return 3;
        case '4': out[0]=67; out[1]=71; out[2]=74; return 3;
        case '5': out[0]=69; out[1]=72; out[2]=76; return 3;
        case '6': out[0]=71; out[1]=74; out[2]=77; return 3;
        case '7': out[0]=72; out[1]=76; out[2]=79; return 3;
        case '8': out[0]=74; out[1]=77; out[2]=81; return 3;
        case '9': out[0]=76; out[1]=79; out[2]=83; return 3;
        default: return 0;
    }
}

uint8_t letter_to_note(char ch) {
    switch (toupper((unsigned char)ch)) {
        case 'A': return 69;
        case 'B': return 71;
        case 'C': return 60;
        case 'D': return 62;
        case 'E': return 64;
        case 'F': return 65;
        case 'G': return 67;
        case 'H': return 70;
        default:  return 0;
    }
}

void write_wav_header(FILE *f, int num_samples) {
    int byte_rate = SAMPLE_RATE * 2;
    int data_size = num_samples * 2;
    int chunk_size = 36 + data_size;

    fwrite("RIFF", 1, 4, f);
    fwrite(&chunk_size, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f);

    uint32_t subchunk1_size = 16;
    uint16_t audio_format = 1;
    uint16_t num_channels = 1;
    uint16_t bits_per_sample = 16;

    fwrite(&subchunk1_size, 4, 1, f);
    fwrite(&audio_format, 2, 1, f);
    fwrite(&num_channels, 2, 1, f);
    uint32_t sr = SAMPLE_RATE;
    fwrite(&sr, 4, 1, f);
    //fwrite(&SAMPLE_RATE, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    uint16_t block_align = num_channels * bits_per_sample / 8;
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits_per_sample, 2, 1, f);

    fwrite("data", 1, 4, f);
    fwrite(&data_size, 4, 1, f);
}

int main() {
    char nome[256];
    printf("Nome do ficheiro de texto a converter: ");
    if (!fgets(nome, sizeof(nome), stdin)) return 1;
    nome[strcspn(nome, "\r\n")] = 0;
    printf("\033c\033[43;30m\n");
    FILE *in = fopen(nome, "r");
    if (!in) { perror("Erro ao abrir"); return 1; }

    FILE *out = fopen("output.wav", "wb");
    if (!out) { perror("Erro ao criar .wav"); return 1; }

    int total_samples = 0;
    int samples_per_note = (int)(DURATION * SAMPLE_RATE);
    int samples_silence = (int)(SILENCE * SAMPLE_RATE);
    int max_symbols = 10000;  // estimativa para header
    write_wav_header(out, (samples_per_note + samples_silence) * max_symbols);

    int ch;
    while ((ch = fgetc(in)) != EOF) {
        if (ch == '\n' || ch == '\r') continue;

        uint8_t notes[MAX_NOTES] = {0};
        int n = 0;

        if (isdigit(ch)) {
            n = chord_notes(ch, notes);
        } else {
            uint8_t note = letter_to_note((char)ch);
            if (note) { notes[0] = note; n = 1; }
        }

        if (n > 0) {
            for (int i = 0; i < samples_per_note; i++) {
                double t = (double)i / SAMPLE_RATE;
                double sample = 0.0;
                for (int j = 0; j < n; j++) {
                    sample += sin(2 * 3.1415927f * note_freq(notes[j]) * t);
                }
                sample /= n;  // média das notas
                int16_t val = (int16_t)(sample * VOLUME);
                fwrite(&val, sizeof(int16_t), 1, out);
            }

            // pausa
            int16_t zero = 0;
            for (int i = 0; i < samples_silence; i++)
                fwrite(&zero, sizeof(int16_t), 1, out);

            total_samples += samples_per_note + samples_silence;
        }
    }

    // Reescreve cabeçalho com número real de amostras
    fseek(out, 0, SEEK_SET);
    write_wav_header(out, total_samples);

    fclose(in);
    fclose(out);
    printf("✅  WAV criado: output.wav (%d samples)\n", total_samples);
    return 0;
}
