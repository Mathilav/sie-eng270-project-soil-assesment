#include <math.h>
#include <stdio.h>

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
    return k1*fct_wtr_cont(wc, wmax, w)*nmb_passages(N,c)*pression(Pref, P, alpha)*profondeur(z, lambda);

} 

double total_BD(double pa0, double k1, double wc, double wmax, double w,int N, double c,
                double Pref, double P, double alpha,double z, double lambda){
    return trafic_density(k1,wc,wmax,w,N,c,Pref,P,alpha, z,lambda)+pa0;

}

double structural_density(double w, double w0, double a, double p0){
    return p0-a*(w-w0);

}


double porosity(double pa0, double k1, double wc, double wmax, double w,int N, double c,
                double Pref, double P, double alpha,double z, double lambda, double w0, double a, double p0){
    return 1-(total_BD(pa0, k1, wc, wmax, w, N, c, Pref, P, alpha, z, lambda)/structural_density(w, w0, a, p0));
}

// structure du csv
struct LignePorosite {
    int   date;        /* AAAAMMJJ, ex. 19891018 */
    int   passage;     // nmb de passages 
    int   pression;    /* PressionMax_kPa_ */
    double water;      /* WaterContent */
};

int lireLignePorosite(const char *ligne, struct LignePorosite *L)
{
    int d, p, pr;
    double w;

    /* Format exact de la ligne :
       Date_interv,Passage,PressionMax_kPa_,WaterContent */
    int n = sscanf(ligne, "%d,%d,%d,%lf", &d, &p, &pr, &w);
    if (n != 4) {
        return 0;   /* ligne invalide */
    }

    L->date     = d;
    L->passage  = p;
    L->pression = pr;
    L->water    = w;
    return 1;
}

#define MAX_LIGNES 7000
#define BUF        100

int lireFichierPorosite(const char *nom,
                        struct LignePorosite *tab, int max)
{
    FILE *f = fopen(nom, "r");
    if (f == NULL) return -1;

    char buffer[BUF];

    /* lire et ignorer la première ligne (header) */
    if (fgets(buffer, sizeof buffer, f) == NULL) {
        fclose(f);
        return 0;
    }

    int n = 0;
    while (fgets(buffer, sizeof buffer, f) != NULL) {
        if (n >= max) break;
        if (lireLignePorosite(buffer, &tab[n])) {
            n++;
        }
    }

    fclose(f);
    return n;   /* nombre de lignes valides lues */
}



int main(void){
    /* ----- 1) constantes globales du modèle (communes aux horizons) ----- */
    double wc    = 0.20;   /* à adapter éventuellement */
    double wmax  = 0.32;
    double c     = 0.9;
    double alpha = 0.5;
    double lambda = 4.0;
    double k1    = 0.1;
    double at    = -0.4;
    double Pref  = 169.1304;
    double w0    = 0.23;
    double p0 = 2.20; 

    /* ----- 2) paramètres spécifiques aux 7 horizons ----- */
    /* ICI tu dois mettre TES valeurs de z et de pa0 pour chaque horizon */
    const int NH = 7;
    double z_h[7]   = {0.05,0.010, 0.15, 0.20, 0.25,0.30,0.35};

    double pa0_h[7] = {1.13, 1.1,1.07,1.06,1.18,1.27,1.42}; 
    
    /* ----- 3) lecture du fichier de données journalières ----- */
    struct LignePorosite lignes[MAX_LIGNES];
    int nb = lireFichierPorosite("data_porosity_full.csv",
                                 lignes, MAX_LIGNES);
    if (nb <= 0) {
        fprintf(stderr, "Erreur lecture fichier, nb = %d\n", nb);
        return 1;
    }

    /* ----- 4) ouverture du fichier de sortie ----- */
    FILE *fout = fopen("porosity_results.csv", "w");
    if (fout == NULL) {
        fprintf(stderr, "Impossible d'ouvrir porosity_results.csv\n");
        return 1;
    }

    /* en-tête : date, water content, numéro d'horizon, profondeur, porosité */
    fprintf(fout, "Date_interv,WaterContent,Horizon,Depth_m,Porosity\n");

    /* ----- 5) double boucle : lignes du CSV × horizons ----- */
    for (int i = 0; i < nb; i++) {
        int    date    = lignes[i].date;        /* AAAAMMJJ */
        int    N       = lignes[i].passage;     /* nombre de passages */
        double P       = (double)lignes[i].pression;
        double w       = lignes[i].water;       /* water content */

        for (int h = 0; h < NH; h++) {
            double z   = z_h[h];    /* profondeur de l'horizon h */
            double pa0 = pa0_h[h];  /* densité initiale de l'horizon h */

            double phi = porosity(pa0, k1, wc, wmax,
                                  w, N, c,
                                  Pref, P, alpha,
                                  z, lambda,
                                  w0, at, p0);

            /* écriture : même date et même water content pour les 7 horizons */
            fprintf(fout, "%d,%.6f,%d,%.4f,%.10f\n",
                    date, w, h+1, z, phi);
        }
    }

    fclose(fout);
    return 0;
}
