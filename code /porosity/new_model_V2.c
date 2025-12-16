#include <math.h>
#include <stdio.h>

/* --- structure du csv : à mettre AVANT porosity_multi --- */
struct LignePorosite {
    int   date;      /* AAAAMMJJ, ex. 19891018 */
    int   passage;   /* nmb de passages */
    int   pression;  /* PressionMax_kPa_ */
    double water;    /* WaterContent */
};

/* --- tes fonctions de modèle inchangées --- */

double fct_wtr_cont(double wc, double wmax, double w){
    double f = 0;
    if (w > wc && w <= wmax) {
        f = (w - wc) / (wmax - wc);
    } else if (w > wmax) {
        f = 1.0;
    }
    return f;
}

double nmb_passages(int N, double c){
    return 1-exp(-c*N);
}

double pression(double Pref, double P, double alpha){
    return pow(P/Pref, alpha);
}

double profondeur(double z, double lambda){
    return exp(-lambda*z);
}

double trafic_density(double k1, double wc, double wmax, double w,int N, double c,
                      double Pref, double P, double alpha,double z, double lambda){
    return k1*fct_wtr_cont(wc, wmax, w)
           * nmb_passages(N,c)
           * pression(Pref, P, alpha)
           * profondeur(z, lambda);
}

double total_BD(double pa0, double k1, double wc, double wmax, double w,int N, double c,
                double Pref, double P, double alpha,double z, double lambda){
    return trafic_density(k1,wc,wmax,w,N,c,Pref,P,alpha, z,lambda) + pa0;
}

double structural_density(double w, double w0, double a, double p0){
    return p0 - a*(w-w0);
}

double porosity(double pa0, double k1, double wc, double wmax, double w,int N, double c,
                double Pref, double P, double alpha,double z, double lambda, double w0, double a, double p0){
    return 1 - total_BD(pa0, k1, wc, wmax, w, N, c, Pref, P, alpha, z, lambda)
               / structural_density(w, w0, a, p0);
}

/* --- nouvelle fonction multi-engins, maintenant APRÈS la struct --- */
double porosity_multi(double pa0,
                      double k1, double wc, double wmax,
                      double c, double Pref, double alpha,
                      double z, double lambda,
                      double w0, double a, double p0,
                      const struct LignePorosite *lignes,
                      int n_lignes)
{
    double delta_total = 0.0;

    for (int i = 0; i < n_lignes; i++) {
        int    N = lignes[i].passage;
        double P = (double)lignes[i].pression;
        double w = lignes[i].water;

        double delta_i = trafic_density(k1, wc, wmax, w,
                                        N, c,
                                        Pref, P, alpha,
                                        z, lambda);
        delta_total += delta_i;
    }

    double rho_a  = pa0 + delta_total;
    double w_day  = lignes[0].water;
    double rho_s  = structural_density(w_day, w0, a, p0);

    if (rho_s <= 0.0) return 0.0;

    double phi = 1.0 - rho_a / rho_s;
    if (phi < 0.0) phi = 0.0;
    if (phi > 1.0) phi = 1.0;
    return phi;
}

/* --- fonctions d’IO inchangées --- */

#define MAX_LIGNES 7000
#define BUF        100

int lireLignePorosite(const char *ligne, struct LignePorosite *L)
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

int lireFichierPorosite(const char *nom,
                        struct LignePorosite *tab, int max)
{
    FILE *f = fopen(nom, "r");
    if (f == NULL) return -1;

    char buffer[BUF];

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

/* --- main inchangé sauf regroupement par date --- */

int main(void){
    double wc    = 0.20;
    double wmax  = 0.32;
    double c     = 0.9;
    double alpha = 0.5;
    double lambda = 4.0;
    double k1    = 0.1;
    double at    = -0.4;
    double Pref  = 169.1304;
    double w0    = 0.23;
    double p0    = 2.20;

    const int NH = 7;
    double z_h[7]   = {0.05, 0.10, 0.15, 0.20, 0.25, 0.30, 0.35};
    double pa0_h[7] = {1.13, 1.10, 1.07, 1.06, 1.18, 1.27, 1.42};

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

    int i = 0;
    while (i < nb) {
        int    date_courante = lignes[i].date;
        double w_courant     = lignes[i].water;

        int j = i + 1;
        while (j < nb && lignes[j].date == date_courante) j++;
        int n_lignes_jour = j - i;
        const struct LignePorosite *bloc = &lignes[i];

        for (int h = 0; h < NH; h++) {
            double z   = z_h[h];
            double pa0 = pa0_h[h];

            double phi = porosity_multi(pa0,
                                        k1, wc, wmax,
                                        c, Pref, alpha,
                                        z, lambda,
                                        w0, at, p0,
                                        bloc, n_lignes_jour);

            fprintf(fout, "%d,%.6f,%d,%.4f,%.10f\n",
                    date_courante, w_courant, h+1, z, phi);
        }

        i = j;
    }

    fclose(fout);
    return 0;
}
