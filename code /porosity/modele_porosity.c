#include <math.h>
#include <stdio.h>

/* ---------- Structures de données ---------- */

/* Données lues dans le CSV */
struct LignePorosite {
    int    date;      /* AAAAMMJJ */
    int    passage;   /* Nb de passages N */
    int    pression;  /* PressionMax_kPa_ (Pi) */
    double water;     /* WaterContent w */
};

/* Paramètres globaux du modèle */
struct ParamsModele {
    double wc;
    double wmax;
    double c;
    double alpha;
    double lambda;
    double k1;
    double a;      /* pente pour rho_t(w) */
    double Pref;
    double w0;
    double p0;     /* rho_t,0 : densité texturale à w0 */
};

/* Paramètres spécifiques à un horizon */
struct Horizon {
    double z;      /* profondeur */
    double pa0;    /* densité apparente initiale (t=0, non roulé) */
    double rho_a;  /* densité apparente courante (mise à jour dans le temps) */
};

/* ---------- Fonctions du modèle ---------- */

static double fct_wtr_cont(double wc, double wmax, double w)
{
    if (w <= wc)   return 0.0;
    if (w <= wmax) return (w - wc) / (wmax - wc);
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

/* densité texturale rho_t(w) */
static double rho_texturale(double w, const struct ParamsModele *pm)
{
    /* rho_t(w) = p0 + a * (w - w0) */
    return pm->p0 + pm->a * (w - pm->w0);
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
        {0.05, 1.13, 0.0},
        {0.10, 1.10, 0.0},
        {0.15, 1.07, 0.0},
        {0.20, 1.06, 0.0},
        {0.25, 1.18, 0.0},
        {0.30, 1.27, 0.0},
        {0.35, 1.42, 0.0}
    };

    /* initialiser rho_a courant = pa0 (état initial) */
    for (int h = 0; h < NH; h++) {
        H[h].rho_a = H[h].pa0;
    }

    struct LignePorosite lignes[MAX_LIGNES];
    int nb = lireFichierPorosite("data_porosity_full.csv",
                                 lignes, MAX_LIGNES);
    if (nb <= 0) {
        fprintf(stderr, "Erreur lecture fichier, nb = %d\n", nb);
        return 1;
    }

    FILE *fout = fopen("data_porosity_full_out.csv", "w");
    if (fout == NULL) {
        fprintf(stderr, "Impossible d'ouvrir data_porosity_full_out.csv\n");
        return 1;
    }

    /* ajout des colonnes Operation et PressionMax_kPa */
    fprintf(fout,
            "Date_interv,WaterContent,Horizon,Depth_m,Porosity,Operation,PressionMax_kPa\n");

    /* regroupement par date + calcul multi‑engins avec mémoire */
    int i = 0;
    while (i < nb) {
        int    date_courante = lignes[i].date;
        double w_courant     = lignes[i].water;

        int j = i + 1;
        while (j < nb && lignes[j].date == date_courante) j++;
        int n_lignes_jour = j - i;
        const struct LignePorosite *bloc = &lignes[i];

        /* opération agricole + pression max du jour */
        int operation = 0;
        double pression_jour = 0.0;

        if (n_lignes_jour > 0) {
            operation = 1;
            double maxP = 0.0;
            for (int k = 0; k < n_lignes_jour; k++) {
                if ((double)bloc[k].pression > maxP) {
                    maxP = (double)bloc[k].pression;
                }
            }
            pression_jour = maxP;
        }

        /* pour chaque horizon : ajouter Δrho_a du jour à rho_a courant, puis calculer phi */
        for (int h = 0; h < NH; h++) {

            /* somme des contributions des engins du jour à cet horizon */
            double delta_total = 0.0;
            for (int k = 0; k < n_lignes_jour; k++) {
                const struct LignePorosite *L = &bloc[k];
                delta_total += trafic_density_engin(&pm,
                                                    L->water,
                                                    L->passage,
                                                    (double)L->pression,
                                                    H[h].z);
            }

            /* mise à jour de la densité apparente courante */
            H[h].rho_a += delta_total;

            /* calcul de rho_t(w) pour ce jour */
            double rho_t = rho_texturale(w_courant, &pm);
            if (rho_t <= 0.0) rho_t = 1e-6; /* sécurité numérique */

            /* porosité structurale du jour */
            double phi = 1.0 - H[h].rho_a / rho_t;
            if (phi < 0.0) phi = 0.0;
            if (phi > 1.0) phi = 1.0;

            fprintf(fout, "%d,%.6f,%d,%.4f,%.10f,%d,%.2f\n",
                    date_courante, w_courant,
                    h + 1, H[h].z, phi,
                    operation, pression_jour);
        }

        i = j;
    }

    fclose(fout);
    return 0;
}
