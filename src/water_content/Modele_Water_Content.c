#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
/*==========================================================
Importation d'un csv
==========================================================*/
/*Note sur les infos du csv Temp_prec_ray.csv 
AAAAMMJJ,RR,TNTXM,TNTXM_moy_mois,GLOT_moy_mois 
RR = prec
TNTXM = temperature moy journee 
TNTXM_moy_mois. = par mois
GLOT_moy_mois = rayonnement par mois*/

/*code inspired by sieprog.ch, exercice pointsgps, puis upgrape chatgpt*/ 

/* Structure of the table*/
struct Temp_Prec_Ray {
    int AAAAMMJJ;
    double RR;
    double TNTXM;
    double TNTXM_moy_mois;
    double GLOT_moy_mois;
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

/*==========================================================
    Exportation d'un csv 
==========================================================*/

/* Save theta values to a CSV file with Date column */
int save_theta_csv(const char *filename, const struct Temp_Prec_Ray *data, const double *theta, int n) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Cannot open output file");
        return 0;
    }

    fprintf(f, "Date,Theta\n"); // header

    for (int i = 0; i <= n; i++) {
        fprintf(f, "%d,%.10f\n", data[i].AAAAMMJJ, theta[i]);
    }

    fclose(f);
    return 1;
}

/*==========================================================
  Evapotranspiration
==========================================================*/
// double Evapotranspriation(
//     double Rs, /* a table of values of monthly solar radiations [cal /cm^2 * J]*/
//     double T /* a table of monthly temperatures in celsius*/)
//     {
//     double ETp = 0.4 * (Rs + 50) * T / (T + 15);
//     return ETp;
//     } /* serait-ce plus utile de faire un malloc ? */

/*Getting days distribution from data dates*/
int is_leap(int year)
{
    return (year % 4 == 0 && year % 100 != 0) ||
            (year % 400 == 0);
}
void split_date(int yyyymmdd, int *year, int *month, int *day)
{
    *year  = yyyymmdd / 10000;       // YYYY
    *month = (yyyymmdd / 100) % 100; // MM
    *day   = yyyymmdd % 100;         // DD
}

int days_in_month_from_date(int yyyymmdd)
    {
    int year, month, day;
    split_date(yyyymmdd, &year, &month, &day);

    static const int days_norm[12] =
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month == 2 && is_leap(year))
        return 29;

    return days_norm[month - 1];
    }

/*computiong Evap per day*/
double *Evapotranspiration_days(const struct Temp_Prec_Ray *data, int Time){
    /*input :   Temperature = double TNTXM_moy_mois
                Radiations = double GLOT_moy_mois data a priori [joule/cm^2], on veut [cal/cm^2 * J]

                (what we want to use as arg)
    output : ET_days = ETp (turc's formula) / nbr of days in a month*/
    

    double *ET_days = malloc((Time + 1) * sizeof(double));
    if (!ET_days) return NULL;

    for (int d = 0; d <= Time; d++) {

        int y = data[d].AAAAMMJJ;
        int days = days_in_month_from_date(y);

        double T = data[d].TNTXM_moy_mois;

        /* conversion [joule/cm^2] -> [cal/cm^2 * J]*/
        double R = data[d].GLOT_moy_mois / 4.184;

        // Compute monthly ET using Turc
        double ET_month = 0.4 * (R + 50) * T / (T + 15); //[cm/month]

        // Daily ET distribution
        ET_days[d] = 10 * ET_month / days; // [mm/day]
    }

    return ET_days;
}



/*==========================================================
  Infiltration (Philip)
==========================================================*/
double infiltration(double S, int t, double A1)
    {/* Imput: 
        S  : sorptivité [mm / √day]
        t  : day of evaluation
        A1 : influence gravitaire [mm / day]
        Output:
        evaluation of I at i */
    return S * sqrt(t) + A1 * t;
}

/* Renvoie un tableau dynamique → doit être free() après usage */
double *compute_infiltration(double S, int time, double A1)
{   /*Imput:
        S  : sorptivité 
        A1 : influence gravitaire
        time : maximum time of evaluation
    Output:
        I_evaluation : a table of discrete value of inflitration for each day
    
    */
    
    double *I_evaluation = malloc((time + 1) * sizeof(double));
    if (!I_evaluation) return NULL;

    I_evaluation[0] = infiltration(S, 0, A1); // infiltration at day 0

    for (int i = 1; i <= time; i++)
        I_evaluation[i] = infiltration(S, i, A1) - infiltration(S, i-1, A1); // daily infiltration

    return I_evaluation;  
    /* NE PAS free ici → on doit le faire APRES l’utilisation */}


/*==========================================================
 Water content
==========================================================*/

double *compute_Watercontent(int Time,
    const struct Temp_Prec_Ray *data,
    double Radiation[],
    double Temperature[],
    double Sorptivity,
    double A1,
    double D, 
    double theta0,
    double theta_fc,
    double thetaR,
    double thetaS,
    double root_depth)
{   
    /* Input:
        Time: maximum time of evaluation
        Radiation: array of radiation values [J /cm^2] (formula will convert to [cal/cm^2 * J])
        Temperature: array of temperature values [°C]
        Sorptivity: sorptivity value [mm/ √day]
        A1: gravitational influence [mm/day]
        Drainage: drainage value if the soil is sat, 0 otherwise [mm/day]
        theta0: initial water content (from data)
        theta_fc: field capacity [mm^3/mm^3]
        thetaR: residual water content [mm^3/mm^3]
        thetaS: saturated water content [mm^3/mm^3]
        root_depth: depth of the root zone [cm]
       Output:
        theta: the evaluation of the water content for each day
    */

    /* Allocate result */
    double *theta = malloc((Time + 1) * sizeof(double));
    if (!theta) return NULL;

    double *I = compute_infiltration(Sorptivity, Time, A1);
    if (!I) { 
        free(theta); 
        return NULL; 
    }
    printf("Infiltration computed.,'%f'\n", I[0]);

    double *ET_days = Evapotranspiration_days(data, Time);
    if (!ET_days) {
        fprintf(stderr, "Memory allocation failed for ET_days.\n");
        free(theta);
        free(I);
        return NULL; // Return NULL instead of 1
    }
    printf("Evapotranspiration computed.'%f'\n", ET_days[0]);

    /* INITIAL VALUE */
    theta[0] = theta0;
    double Drainage = 0; /* assume theta0 =! thetaS, not saturated*/

    /* CUMULATIVE EVOLUTION */
    // for (int d = 1; d <= Time; d++) {
    //     theta[d] = theta[d-1] + (1.0 / (root_depth * 10.0)) * ( I[d] - ET_days[d] - Drainage);
    //     if (theta[d] < 0) theta[d] = 0; /* water content cannot be negative */
    //     /*set drainage at each it dep on sat or not*/
    //     if (theta[d] >= thetaS) {
    //         theta[d] = thetaS; /* saturated */
    //         Drainage = D; /* apply drainage */
    //     } else {
    //         Drainage = 0; /* no drainage */
    //     /* set ET dep on dry or not*/
    //     double stress = fmin(1.0, theta[d-1] / theta_fc);
    //     ET_days[d] *= stress; /* reduce ET if dry */
    //     }
    // }
    for (int d = 1; d <= Time; d++) {

        /* ET stress, decrease ET if dry soil*/
        double ETa = ET_days[d];
        double stress = fmin(1.0, theta[d-1] / theta_fc);
        ETa *= stress;
    
        /* Drainage only if saturated */
        double Drainage_d = (theta[d-1] >= thetaS) ? D : 0.0;
    
        /* Water balance */
        theta[d] = theta[d-1] + (I[d] - ETa - Drainage_d) / (root_depth * 10.0);
    
        /* Physical bounds */
        if (theta[d] < thetaR) theta[d] = thetaR; /* water content cannot be under the residual theta*/
        if (theta[d] > thetaS) theta[d] = thetaS; /* cannot exceed saturation */
    }

    free(I);
    free(ET_days);

    return theta;
}

int main(int argc, char *argv[]) {
    /* values for silt loam USDA / Rawls et al.*/
    double Sorptivity = 22.50; // [mm/ √day]
    double A1 = 5.00; // [mm/ day]
    double Drainage = 0.001; /* Placeholder value [mm/day]*/
    double Theta0 = 0.30; /* Initial water content [mm^3/mm^3]*/
    double theta_fc = 0.27; /* field capacity [mm^3/mm^3]*/
    double thetaR = 0.06; /* Residual water content [mm^3/mm^3]*/
    double thetaS = 0.45; /* Saturated water content [mm^3/mm^3]*/
    double root_depth = 35.0; /* Root depth [cm]*/

    const char *filename = (argc > 1 ? argv[1] : "results/Temp_prec_ray.csv");
    const char *output_csv = (argc > 2 ? argv[2] : "results/theta_output.csv");

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

    double *radiation = malloc(nb * sizeof(double));
    double *temperature = malloc(nb * sizeof(double));
    if (!radiation || !temperature) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(radiation);
        free(temperature);
        free(meteos);
        return 1;
    }

    for (int i = 0; i < nb; i++) {
        radiation[i] = meteos[i].GLOT_moy_mois;
        temperature[i] = meteos[i].TNTXM_moy_mois;
    }

    double *theta = compute_Watercontent(nb-1, meteos, radiation, temperature, Sorptivity, A1, Drainage, Theta0, theta_fc, thetaR, thetaS, root_depth);
    if (theta) {
        for (int i = 0; i < 6; i++) {
            printf("Theta[%d]: %.2f\n", i, theta[i]);
            printf("Evapotranspiration_days[%d]: %.2f\n", i, Evapotranspiration_days(meteos, nb-1)[i]);
            printf("Infiltration[%d]: %.2f\n", i, compute_infiltration(Sorptivity, nb-1, A1)[i]);
            printf("Drainage: %.2f\n", Drainage);
        }
    
        /* Save to CSV */
        if (save_theta_csv(output_csv, meteos, theta, nb - 1)) {
            printf("Theta saved to %s\n", output_csv);
        } else {
            fprintf(stderr, "Failed to save theta CSV.\n");
        }
    
        free(theta);
    }
    free(radiation);
    free(temperature);
    free(meteos);
    return 0;
}