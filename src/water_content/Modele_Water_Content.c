#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

/*==========================================================
  CSV import: Temp_prec_ray.csv
  Columns: AAAAMMJJ, RR, TNTXM, TNTXM_moy_mois, GLOT_moy_mois
  RR               : daily precipitation
  TNTXM            : daily mean temperature
  TNTXM_moy_mois   : monthly mean temperature
  GLOT_moy_mois    : monthly mean solar radiation
==========================================================*/

/* Data structure for one meteorological record */
struct TempPrecRad {
    int    AAAAMMJJ;
    double RR;
    double TNTXM;
    double TNTXM_monthly_mean;
    double GLOT_monthly_mean;
};

/* Trim newline characters from a string */
static void trim_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

/* Parse one CSV line into struct. Returns 1 if OK, 0 otherwise */
int parse_csv_line(const char *line, struct TempPrecRad *m) {
    char buf[256];
    strncpy(buf, line, sizeof(buf));
    buf[sizeof(buf) - 1] = 0;

    /* Split into tokens */
    char *tok  = NULL;
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

    m->RR                 = strtod(fields[1], &e); if (*e) return 0;
    m->TNTXM              = strtod(fields[2], &e); if (*e) return 0;
    m->TNTXM_monthly_mean = strtod(fields[3], &e); if (*e) return 0;
    m->GLOT_monthly_mean  = strtod(fields[4], &e); if (*e) return 0;

    return 1;
}

/* Read whole CSV into dynamically grown array.
   Returns number of records, -1 on error. */
ssize_t read_meteo_file(const char *filename, struct TempPrecRad **out) {
    FILE *f = fopen(filename, "r");
    if (!f) return -1;

    struct TempPrecRad *arr = NULL;
    size_t cap = 0, n = 0;

    char line[256];
    int lineno = 0;

    /* Read the first line — skip if header */
    if (fgets(line, sizeof(line), f)) {
        lineno++;
        trim_newline(line);

        /* Detect header: contains alphabetic characters */
        int header = 0;
        for (int i = 0; line[i]; i++) {
            if (isalpha((unsigned char)line[i])) {
                header = 1;
                break;
            }
        }

        if (!header) {
            struct TempPrecRad m;
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

        struct TempPrecRad m;
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

    /* Shrink memory to actual size */
    if (n > 0) {
        arr = realloc(arr, n * sizeof(*arr));
    }

    *out = arr;
    return n;
}

/* Print one meteorological record to stdout */
void print_meteo(const struct TempPrecRad *m) {
    printf("%d  %.2f  %.2f  %.2f  %.2f\n",
           m->AAAAMMJJ,
           m->RR,
           m->TNTXM,
           m->TNTXM_monthly_mean,
           m->GLOT_monthly_mean);
}

/*==========================================================
  CSV export for theta
==========================================================*/

/* Save theta values to a CSV file with Date column */
int save_theta_csv(const char *filename,
                   const struct TempPrecRad *data,
                   const double *theta, int n) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Cannot open output file");
        return 0;
    }

    fprintf(f, "Date,Theta\n"); /* header */

    for (int i = 0; i < n; i++) {
        fprintf(f, "%d,%.10f\n", data[i].AAAAMMJJ, theta[i]);
    }

    fclose(f);
    return 1;
}

/*==========================================================
  Date utilities
==========================================================*/

int is_leap(int year)
{
    return (year % 4 == 0 && year % 100 != 0) ||
           (year % 400 == 0);
}

void split_date(int yyyymmdd, int *year, int *month, int *day)
{
    *year  = yyyymmdd / 10000;       /* YYYY */
    *month = (yyyymmdd / 100) % 100; /* MM   */
    *day   = yyyymmdd % 100;         /* DD   */
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

/*==========================================================
  Evapotranspiration (Turc, daily distribution)
==========================================================*/

/* Compute daily potential ET from monthly mean radiation and temperature */
double *Evapotranspiration_days(const struct TempPrecRad *data, int n_days)
{
    /* Output: ET_days[d] = ETp_month / days_in_month, converted to mm/day */

    double *ET_days = malloc((n_days + 1) * sizeof(double));
    if (!ET_days) return NULL;

    for (int d = 0; d <= n_days; d++) {

        int days = days_in_month_from_date(data[d].AAAAMMJJ);

        double T = data[d].TNTXM_monthly_mean;

        /* Convert [J/cm^2] -> [cal/cm^2] */
        double R = data[d].GLOT_monthly_mean / 4.184;

        /* Monthly ET using Turc (cm/month) */
        double ET_month = 0.4 * (R + 50.0) * T / (T + 15.0);

        /* Daily ET distribution in mm/day */
        ET_days[d] = 10.0 * ET_month / days;
    }

    return ET_days;
}

/*==========================================================
  Infiltration (Philip)
==========================================================*/

double infiltration(double S, int t, double A1)
{
    /* I(t) = S * sqrt(t) + A1 * t */
    return S * sqrt((double)t) + A1 * (double)t;
}

/* Return dynamic array of daily infiltration increments.
   Caller must free(). */
double *compute_infiltration(double S, int time, double A1)
{
    double *I_evaluation = malloc((time + 1) * sizeof(double));
    if (!I_evaluation) return NULL;

    /* Infiltration at day 0 */
    I_evaluation[0] = infiltration(S, 0, A1);

    /* Daily increments: I(t) - I(t-1) */
    for (int i = 1; i <= time; i++) {
        I_evaluation[i] = infiltration(S, i, A1) - infiltration(S, i - 1, A1);
    }

    return I_evaluation;
}

/*==========================================================
  Water content
==========================================================*/

double *compute_Watercontent(int Time,
    const struct TempPrecRad *data,
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
    /* Compute daily soil water content over [0..Time].
       Radiation and Temperature arrays are currently not used directly,
       because ET is recomputed from data (GLOT, TNTXM). */

    double *theta = malloc((Time + 1) * sizeof(double));
    if (!theta) return NULL;

    double *I = compute_infiltration(Sorptivity, Time, A1);
    if (!I) {
        free(theta);
        return NULL;
    }
    printf("Infiltration computed (example I[0] = %f)\n", I[0]);

    double *ET_days = Evapotranspiration_days(data, Time);
    if (!ET_days) {
        fprintf(stderr, "Memory allocation failed for ET_days.\n");
        free(theta);
        free(I);
        return NULL;
    }
    printf("Evapotranspiration computed (example ET_days[0] = %f)\n", ET_days[0]);

    /* Initial condition */
    theta[0] = theta0;

    /* Daily water balance with ET stress and drainage */
    for (int d = 1; d <= Time; d++) {

        /* ET stress: reduce ET if soil is dry */
        double ETa = ET_days[d];
        double stress = fmin(1.0, theta[d - 1] / theta_fc);
        ETa *= stress;

        /* Drainage only if saturated */
        double Drainage_d = (theta[d - 1] >= thetaS) ? D : 0.0;

        /* Water balance in root zone */
        theta[d] = theta[d - 1] +
                   (I[d] - ETa - Drainage_d) / (root_depth * 10.0);

        /* Physical bounds */
        if (theta[d] < thetaR) theta[d] = thetaR;
        if (theta[d] > thetaS) theta[d] = thetaS;
    }

    free(I);
    free(ET_days);

    return theta;
}

/*==========================================================
  Main
==========================================================*/

int main(int argc, char *argv[]) {
    /* Parameters for silt loam (USDA / Rawls et al.) */
    double Sorptivity = 22.50;   /* [mm / sqrt(day)] */
    double A1         = 5.00;    /* [mm / day]       */
    double Drainage   = 0.001;  /* placeholder [mm/day] */
    double Theta0     = 0.30;    /* initial water content [m3/m3] */
    double theta_fc   = 0.27;    /* field capacity [m3/m3] */
    double thetaR     = 0.06;    /* residual water content [m3/m3] */
    double thetaS     = 0.45;    /* saturated water content [m3/m3] */
    double root_depth = 35.0;    /* root depth [cm] */

    const char *filename   = (argc > 1 ? argv[1] : "results/Temp_prec_ray.csv");
    const char *output_csv = (argc > 2 ? argv[2] : "results/theta_output.csv");

    struct TempPrecRad *meteos = NULL;
    ssize_t nb = read_meteo_file(filename, &meteos);

    if (nb < 0) {
        fprintf(stderr, "Error: cannot open '%s'\n", filename);
        return 1;
    }
    if (nb == 0) {
        fprintf(stderr, "No valid lines in input file.\n");
        return 1;
    }

    printf("Read %zd lines.\n", nb);
    printf("First lines:\n");
    for (int i = 0; i < nb && i < 10; i++) {
        print_meteo(&meteos[i]);
    }

    double *radiation   = malloc(nb * sizeof(double));
    double *temperature = malloc(nb * sizeof(double));
    if (!radiation || !temperature) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(radiation);
        free(temperature);
        free(meteos);
        return 1;
    }

    for (int i = 0; i < nb; i++) {
        radiation[i]   = meteos[i].GLOT_monthly_mean;
        temperature[i] = meteos[i].TNTXM_monthly_mean;
    }

    double *theta = compute_Watercontent((int)nb - 1,
                                         meteos,
                                         radiation,
                                         temperature,
                                         Sorptivity,
                                         A1,
                                         Drainage,
                                         Theta0,
                                         theta_fc,
                                         thetaR,
                                         thetaS,
                                         root_depth);
    if (theta) {
        for (int i = 0; i < 6 && i < nb; i++) {
            printf("Theta[%d]: %.2f\n", i, theta[i]);
        }

        /* Save to CSV */
        if (save_theta_csv(output_csv, meteos, theta, (int)nb)) {
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
