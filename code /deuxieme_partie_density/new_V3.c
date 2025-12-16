#include <math.h>
#include <stdio.h>

/* ---------- Structures de données ---------- */

/* Données lues dans le CSV */
struct LignePorosite {
    int    date;      /* AAAAMMJJ, ex. 19891018 */
    int    passage;   /* Nb de passages N */
    int    pression;  /* PressionMax_kPa_ (Pi) */
    double water;     /* WaterContent w */
};

/* Paramètres globaux du modèle (communs aux horizons) */
struct ParamsModele {
    double wc;
    double wmax;
    double c;
    double alpha;
    double lambda;
    double k1;
    double a;      /* paramètre 'a' pour structural_density */
    double Pref;
    double w0;
    double p0;     /* densité structurale de référence */
};

/* Paramètres spécifiques à un horizon */
struct Horizon {
    double z;    /* profondeur */
    double pa0;  /* densité apparente initiale */
};

/* ---------- Fonctions du modèle ---------- */

static double fct_wtr_cont(double wc, double wmax, double w)
{
    if (w <= wc)          return 0.0;
    if (w <= wmax)        return (w - wc) / (wmax - wc);
                          return 1.0;
}

static double nmb_passages(int N, double c)
{
    return 1.0 - exp(-c * (double)N);
}

static double pression(double Pref, double P, double alpha)
{
    return pow(P / Pref, alpha);
}

static double profondeur(double z, double lambda)
{
    return exp(-lambda * z);
}

/* contribution d'un engin (une ligne) à Δρa */
static double trafic_density_engin(const struct ParamsModele *pm,
                                   double w, int N, double P, double z)
{
    return pm->k1
           * fct_wtr_cont(pm->wc, pm->wmax, w)
           * nmb_passages(N, pm->c)
           * pression(pm->Pref, P, pm->alpha)
           * profondeur(z, pm->lambda);
}

static double structural_density(double w,
                                 const struct ParamsModele *pm)
{
    return pm->p0 - pm->a * (w - pm->w0);
}

/* Porosité multi‑engins pour un point (jour, horizon) */
static double porosity_multi(const struct Horizon *horiz,
                             const struct ParamsModele *pm,
                             const struct LignePorosite *lignes,
                             int n_lignes)
{
    double delta_total = 0.0;

    for (int i = 0; i < n_lignes; i++) {
        const struct LignePorosite *L = &lignes[i];
        delta_total += trafic_density_engin(pm,
                                            L->water,
                                            L->passage,
                                            (double)L->pression,
                                            horiz->z);
    }

    double rho_a  = horiz->pa0 + delta_total;
    double w_day  = lignes[0].water;  /* même w pour la journée */
    double rho_s  = structural_density(w_day, pm);

    if (rho_s <= 0.0) return 0.0;

    double phi = 1.0 - rho_a / rho_s;
    if (phi < 0.0) phi = 0.0;
    if (phi > 1.0) phi = 1.0;
    return phi;
}

/* ---------- Entrée / sortie CSV ---------- */

#define MAX_LIGNES 7000
#define BUF        100

static int lireLignePorosite(const char *ligne, struct LignePorosite *L)
{
    int d, p, pr;
    double w;

    int n = sscanf(ligne, "%d,%d,%d,%lf", &d, &p, &pr, &w);
    if (n != 4) return 0;

    L->date     = d;
    L->passage  = p;
    L->pression = pr;
    L->water    = w;
    return 1;
}

static int lireFichierPorosite(const char *nom,
                               struct LignePorosite *tab, int max)
{
    FILE *f = fopen(nom, "r");
    if (f == NULL) return -1;

    char buffer[BUF];

    /* sauter l'en‑tête */
    if (fgets(buffer, sizeof buffer, f) == NULL) {
        fclose(f);
        return 0;
    }

    int n = 0;
    while (fgets(buffer, sizeof buffer, f) != NULL) {
        if (n >= max) break;
        if (lireLignePorosite(buffer, &tab[n])) n++;
    }

    fclose(f);
    return n;
}

/* ---------- Programme principal ---------- */

int main(void)
{
    /* paramètres globaux du modèle */
    struct ParamsModele pm = {
        .wc     = 0.20,
        .wmax   = 0.32,
        .c      = 0.9,
        .alpha  = 0.5,
        .lambda = 4.0,
        .k1     = 0.1,
        .a      = -0.4,
        .Pref   = 169.1304,
        .w0     = 0.23,
        .p0     = 2.20
    };

    /* 7 horizons */
    const int NH = 7;
    struct Horizon H[NH] = {
        {0.05, 1.13},
        {0.10, 1.10},
        {0.15, 1.07},
        {0.20, 1.06},
        {0.25, 1.18},
        {0.30, 1.27},
        {0.35, 1.42}
    };

    struct LignePorosite lignes[MAX_LIGNES];
    int nb = lireFichierPorosite("data_porosity_full.csv",
                                 lignes, MAX_LIGNES);
    if (nb <= 0) {
        fprintf(stderr, "Erreur lecture fichier, nb = %d\n", nb);
        return 1;
    }

    FILE *fout = fopen("porosity_results.csv", "w");
    if (fout == NULL) {
        fprintf(stderr, "Impossible d'ouvrir porosity_results.csv\n");
        return 1;
    }

    fprintf(fout, "Date_interv,WaterContent,Horizon,Depth_m,Porosity\n");

    /* regroupement par date + calcul multi‑engins */
    int i = 0;
    while (i < nb) {
        int    date_courante = lignes[i].date;
        double w_courant     = lignes[i].water;

        int j = i + 1;
        while (j < nb && lignes[j].date == date_courante) j++;
        int n_lignes_jour = j - i;
        const struct LignePorosite *bloc = &lignes[i];

        for (int h = 0; h < NH; h++) {
            double phi = porosity_multi(&H[h], &pm, bloc, n_lignes_jour);

            fprintf(fout, "%d,%.6f,%d,%.4f,%.10f\n",
                    date_courante, w_courant,
                    h + 1, H[h].z, phi);
        }

        i = j;
    }

    fclose(fout);
    return 0;
}
