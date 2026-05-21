#ifndef HEATMAP_H
#define HEATMAP_H

#define HEATMAP_FILE "/.typecode/heatmap.txt"

typedef struct {
    int counts[128];   /* ошибки по ASCII коду символа */
} Heatmap;

extern Heatmap global_heatmap;

void heatmap_record_error(Heatmap *h, char expected, char typed);
void heatmap_load(Heatmap *h);
void heatmap_save(const Heatmap *h);
void heatmap_draw_screen(const Heatmap *h);
void heatmap_reset(Heatmap *h);

#endif
