#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
/*code inspired by sieprog.ch, exercice pointsgps, puis upgrape chatgpt*/ 

/*Note sur l’automation
file csv Temp_Prec_Ray extrait avec matlab -> C 
table C water content -> file csv 
file csv WaterContent -> matlab
table matlab compaction -> csv 
*/
/* Structure of the tables*/
struct Temp_Prec_Ray {
    int AAAAMMJJ;
    double RR;
    double TNTXM;
    double TNTXM_moy_mois;
    double GLOT_moy_mois;
};

struct Water_Content{
    int AAAAMMJJ;
    double Theta;
};


/* Trim newline from a string */
static void trim_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}
/* Read one CSV line into struct. Returns 1=OK, 0=bad line */
int parse_csv_line(const char *line, struct Temp_Prec_Ray *m) {
    char buf[256];
    strncpy(buf, line, sizeof(buf));
    buf[sizeof(buf)-1] = 0;

    /* Split into tokens */
    char *tok = NULL;
    char *rest = buf;

    char *fields[5];
    int nf = 0;

    while ((tok = strtok_r(rest, ",", &rest))) {
        if (nf < 5) fields[nf] = tok;
        nf++;
    }
    if (nf != 5) return 0; /* invalid column count */

    /* Validate and convert fields */
    if (strlen(fields[0]) != 8) return 0; /* expect YYYYMMDD */

    char *e;
    m->AAAAMMJJ = strtol(fields[0], &e, 10);
    if (*e) return 0;

    m->RR = strtod(fields[1], &e); if (*e) return 0;
    m->TNTXM = strtod(fields[2], &e); if (*e) return 0;
    m->TNTXM_moy_mois = strtod(fields[3], &e); if (*e) return 0;
    m->GLOT_moy_mois = strtod(fields[4], &e); if (*e) return 0;

    return 1;
}

/* Read whole CSV into dynamically-grown array */
ssize_t lireFichier(const char *filename, struct Temp_Prec_Ray **out) {
    FILE *f = fopen(filename, "r");
    if (!f) return -1;

    struct Temp_Prec_Ray *arr = NULL;
    size_t cap = 0, n = 0;

    char line[256];
    int lineno = 0;

    /* Read the first line — skip if header */
    if (fgets(line, sizeof(line), f)) {
        lineno++;
        trim_newline(line);

        /* Detect header: contains alphabetic characters */
        int header = 0;
        for (int i = 0; line[i]; i++)
            if (isalpha((unsigned char)line[i])) header = 1;

        if (!header) {
            struct Temp_Prec_Ray m;
            if (parse_csv_line(line, &m)) {
                if (n >= cap) {
                    cap = cap ? cap * 2 : 1024;
                    arr = realloc(arr, cap * sizeof(*arr));
                }
                arr[n++] = m;
            } else {
                fprintf(stderr, "Warning: bad line %d skipped\n", lineno);
            }
        }
    }

    /* Read remaining lines */
    while (fgets(line, sizeof(line), f)) {
        lineno++;
        trim_newline(line);

        struct Temp_Prec_Ray m;
        if (parse_csv_line(line, &m)) {
            if (n >= cap) {
                cap = cap ? cap * 2 : 1024;
                arr = realloc(arr, cap * sizeof(*arr));
            }
            arr[n++] = m;
        } else {
            fprintf(stderr, "Warning: bad line %d skipped\n", lineno);
        }
    }

    fclose(f);

    /* shrink memory to actual size */
    arr = realloc(arr, n * sizeof(*arr));

    *out = arr;
    return n;
}

void afficher(const struct Temp_Prec_Ray *m) {
    printf("%d  %.2f  %.2f  %.2f  %.2f\n",
           m->AAAAMMJJ,
           m->RR,
           m->TNTXM,
           m->TNTXM_moy_mois,
           m->GLOT_moy_mois);
}

int main(int argc, char *argv[]) {
    const char *filename = (argc > 1 ? argv[1] : "Temp_prec_ray.csv");

    struct Temp_Prec_Ray *meteos = NULL;
    ssize_t nb = lireFichier(filename, &meteos);

    if (nb < 0) {
        fprintf(stderr, "Erreur: impossible d'ouvrir '%s'\n", filename);
        return 1;
    }
    if (nb == 0) {
        fprintf(stderr, "Aucune ligne valide.\n");
        return 1;
    }

    printf("Lues %zd lignes.\n", nb);
    printf("Premières lignes :\n");

    for (int i = 0; i < nb && i < 10; i++)
        afficher(&meteos[i]);

    free(meteos);
    return 0;
}
