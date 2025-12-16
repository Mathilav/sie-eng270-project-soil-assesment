// #include <math.h>

// double water_sensibility( double w_t, double w_opt, double beta){
//     double water = exp(beta*pow(w_t-w_opt, 2));
//     return water;
// }

// double TICI(int Ngt, double a, double b, double Wref, double Wg, double Pref, double Pg, 
//     double w_t, double w_opt, double beta){
//     double TICI_t = Ngt*pow(Wg/Wref, a)*pow(Pg/Pref,2)*water_sensibility(w_t, w_opt, beta); 
//     return TICI_t; 

// }

// double var_BD_eau( double coef_sens, double w_t, double w_ref){
//     double Var_BD = coef_sens*(w_t-w_ref);
//     return Var_BD; 
// }

#include <math.h>
#include <stdio.h>

/* 1. Sensibilité à l'eau : f(w_t) = exp( -beta * (w_t - w_opt)^2 ) */
double water_sensitivity(double w_t, double w_opt, double beta)
{
    double diff = w_t - w_opt;
    return exp( -beta * diff * diff );
}

/* 2. Contribution TICI pour une opération :
   TICI_g,t = Ngt * (Wg/Wref)^a * (Pg/Pref)^b * f(w_t)
*/
double TICI(int Ngt, double a, double b, double Wref, double Wg, double Pref, double Pg,
            double w_t, double w_opt, double beta)
{
    double f_w      = water_sensitivity(w_t, w_opt, beta);
    double load_fac = pow(Wg / Wref, a);
    double press_fac= pow(Pg / Pref, b);
    double TICI_t   = Ngt * load_fac * press_fac * f_w;
    return TICI_t;
}

/* 3. Variation de BD due à l'eau :
   DeltaBD_eau = coef_sens * (w_t - w_ref)
*/
double var_BD_eau(double coef_sens, double w_t, double w_ref)
{
    return coef_sens * (w_t - w_ref);
}

/* 4. Variation de BD due au trafic :
   DeltaBD_trafic = k * TICI_interval
*/
double var_BD_trafic(double k, double TICI_interval)
{
    return k * TICI_interval;
}

/* 5. Mise à jour récursive de BD :
   BD_t = BD_prev + DeltaBD_eau + DeltaBD_trafic
*/
double update_BD(double BD_prev,
                 double coef_sens,  /* gamma */
                 double w_t,
                 double w_ref,
                 double k,
                 double TICI_interval)
{
    double dBD_eau    = var_BD_eau(coef_sens, w_t, w_ref);
    double dBD_trafic = var_BD_trafic(k, TICI_interval);
    return BD_prev + dBD_eau + dBD_trafic;
}

int main(){
// double BD_prev = 1.35;      // BD au pas de temps précédent
// double gamma   = -0.02;     // coef_sens
// double w_ref   = 20.0;
// double w_t     = 24.0;
// double k       = 0.05;

// double Wref = 8.0, Pref = 150.0;
// double Wg = 8.0, Pg = 150.0;
// int Ngt = 1;
// double w_opt = 25.0;
// double Delta_w = 1.25;
// double beta = 1.0 / (Delta_w * Delta_w);

// double TICI_interval = TICI(Ngt, 1.0, 1.0, Wref, Wg, Pref, Pg,
//                             w_t, w_opt, beta);

// double BD_t = update_BD(BD_prev, gamma, w_t, w_ref, k, TICI_interval);

// printf("%f\n", BD_t);

// return 0;

// constantes déterminées à partir de moyennes ou de la documentation scientifique 
double w_opt = 0.23; // tester différentes valeurs entre 0,22 et 0,25 faire attention à la justification
double beta = 0.0; //à completer
double Wref = 0.0;//à compléter 
double Pref = 0.0; //à compléter 
double w_ref = 0.0; // à compléter avec la moyenne de water content  
int a = 1; 
int b = 1;
double coef_sens = 0.0; //à compléter 
double k = 0.0;  //à compléter 

}



