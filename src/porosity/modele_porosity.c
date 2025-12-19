#include <math.h>
#include <stdio.h>

/* ---------- Data structures ---------- */

/* One input record read from the CSV file */
struct PorosityRecord {
    int    date;      /* YYYYMMDD */
    int    passes;    /* number of passes N */
    int    pressure;  /* PressionMax_kPa_ (Pi) */
    double water;     /* water content w */
};

/* Global model parameters */
struct ModelParams {
    double wc;
    double wmax;
    double c;
    double alpha;
    double lambda;
    double k1;
    double a;      /* slope for rho_t(w) */
    double Pref;
    double w0;
    double p0;     /* rho_t,0: textural density at w0 */
};

/* Layer-specific parameters for one soil horizon */
struct Horizon {
    double depth_m;             /* depth in meters */
    double bulk_density_init;   /* initial bulk density (t = 0, uncompacted) */
    double bulk_density_curr;   /* current bulk density, updated over time */
};

/* ---------- Model functions ---------- */

static double water_content_factor(double wc, double wmax, double w)
{
    if (w <= wc)   return 0.0;
    if (w <= wmax) return (w - wc) / (wmax - wc);
    return 1.0;
}

static double passes_factor(int N, double c)
{
    return 1.0 - exp(-c * (double)N);
}

static double pressure_factor(double Pref, double P, double alpha)
{
    return pow(P / Pref, alpha);
}

static double depth_factor(double depth_m, double lambda)
{
    return exp(-lambda * depth_m);
}

/* Contribution of one machine (one record) to Δrho_a */
static double traffic_density_increment(const struct ModelParams *mp,
                                        double w, int N, double P, double depth_m)
{
    return mp->k1
           * water_content_factor(mp->wc, mp->wmax, w)
           * passes_factor(N, mp->c)
           * pressure_factor(mp->Pref, P, mp->alpha)
           * depth_factor(depth_m, mp->lambda);
}

/* Textural density rho_t(w) */
static double textural_density(double w, const struct ModelParams *mp)
{
    /* rho_t(w) = p0 + a * (w - w0) */
    return mp->p0 + mp->a * (w - mp->w0);
}

/* ---------- CSV input / output ---------- */

#define MAX_LINES 7000
#define BUF       100

/* Parse one CSV line into a PorosityRecord */
static int readPorosityRecord(const char *line, struct PorosityRecord *rec)
{
    int    d, passes, pressure;
    double water;

    int n = sscanf(line, "%d,%d,%d,%lf", &d, &passes, &pressure, &water);
    if (n != 4) return 0;

    rec->date     = d;
    rec->passes   = passes;
    rec->pressure = pressure;
    rec->water    = water;
    return 1;
}

/* Read the whole CSV file into an array of records */
static int readPorosityFile(const char *filename,
                            struct PorosityRecord *records, int max_records)
{
    FILE *f = fopen(filename, "r");
    if (f == NULL) return -1;

    char buffer[BUF];

    /* Skip header line */
    if (fgets(buffer, sizeof buffer, f) == NULL) {
        fclose(f);
        return 0;
    }

    int n = 0;
    while (fgets(buffer, sizeof buffer, f) != NULL) {
        if (n >= max_records) break;
        if (readPorosityRecord(buffer, &records[n])) {
            n++;
        }
    }

    fclose(f);
    return n;
}

/* ---------- Main program ---------- */

int main(int argc, char *argv[])
{
    /* Global model parameters */
    struct ModelParams mp = {
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

    /* Seven soil horizons */
    #define NH 7
    struct Horizon horizons[NH] = {
        {0.05, 1.13, 0.0},
        {0.10, 1.10, 0.0},
        {0.15, 1.07, 0.0},
        {0.20, 1.06, 0.0},
        {0.25, 1.18, 0.0},
        {0.30, 1.27, 0.0},
        {0.35, 1.42, 0.0}
    };

    /* Initialize current bulk density to initial value (uncompacted state) */
    for (int h = 0; h < NH; h++) {
        horizons[h].bulk_density_curr = horizons[h].bulk_density_init;
    }

    const char *input_csv  = (argc > 1 ? argv[1] : "results/data_porosity_full.csv");
    const char *output_csv = (argc > 2 ? argv[2] : "results/data_porosity_full_out.csv");

    struct PorosityRecord records[MAX_LINES];
    int n_records = readPorosityFile(input_csv, records, MAX_LINES);
    if (n_records <= 0) {
        fprintf(stderr, "Error reading input file, n_records = %d\n", n_records);
        return 1;
    }

    FILE *fout = fopen(output_csv, "w");
    if (fout == NULL) {
        fprintf(stderr, "Cannot open %s\n", output_csv);
        return 1;
    }

    /* Output header, adding Operation and PressionMax_kPa columns */
    fprintf(fout,
            "Date_interv,WaterContent,Horizon,Depth_m,Porosity,Operation,PressionMax_kPa\n");

    /* Group by date and compute multi-machine effect with memory */
    int i = 0;
    while (i < n_records) {
        int    current_date  = records[i].date;
        double current_water = records[i].water;

        /* Find block [i, j) for the same date */
        int j = i + 1;
        while (j < n_records && records[j].date == current_date) {
            j++;
        }
        int n_records_day = j - i;
        const struct PorosityRecord *block = &records[i];

        /* Operation indicator and maximum pressure for the day */
        int    operation_flag = 0;
        double pressure_day   = 0.0;

        if (n_records_day > 0) {
            operation_flag = 1;
            double maxP = 0.0;
            for (int k = 0; k < n_records_day; k++) {
                if ((double)block[k].pressure > maxP) {
                    maxP = (double)block[k].pressure;
                }
            }
            pressure_day = maxP;
        }

        /* For each horizon: accumulate Δrho_a, then compute porosity */
        for (int h = 0; h < NH; h++) {

            /* Sum contributions of all machines for this day at this horizon */
            double delta_rho_total = 0.0;
            for (int k = 0; k < n_records_day; k++) {
                const struct PorosityRecord *rec = &block[k];
                delta_rho_total += traffic_density_increment(&mp,
                                                             rec->water,
                                                             rec->passes,
                                                             (double)rec->pressure,
                                                             horizons[h].depth_m);
            }

            /* Update current bulk density */
            horizons[h].bulk_density_curr += delta_rho_total;

            /* Compute textural density rho_t(w) for this day */
            double rho_t = textural_density(current_water, &mp);
            if (rho_t <= 0.0) {
                rho_t = 1e-6; /* numerical safety */
            }

            /* Structural porosity for this day */
            double phi = 1.0 - horizons[h].bulk_density_curr / rho_t;
            if (phi < 0.0) phi = 0.0;
            if (phi > 1.0) phi = 1.0;

            fprintf(fout, "%d,%.6f,%d,%.4f,%.10f,%d,%.2f\n",
                    current_date, current_water,
                    h + 1, horizons[h].depth_m, phi,
                    operation_flag, pressure_day);
        }

        i = j;
    }

    fclose(fout);
    return 0;
}
