/* this block of code sets up and runs the model */
#include <math.h>
#include <stdlib.h>

/*Forme fct :  typeDeRetour nom(type parametre, type parametre, ...) {
    ...
} */
int Limits(void){
    /*Boundaries*/
    double qTop = -0.01; /*m/day. = precipitation (our vector data) It is negative because 
                            the direction of infiltration is downward*/
    int qBot[] = []; /* should be an empty table*/
    double psiTop= 0.0 ;  /*top boundary condition*/  
    double psiBot[] = []; /* change to [] for free drainage condition */ 

    /*Grid in space*/
    double dz = 0.05;  /* [m] increment over which we model for vertical profile*/
    double ProfileDepth = 0.3; /* [m] hight of soil we are consideration */
 /* ajouter z=np.arange(dz/2.0,ProfileDepth,dz)
n=z.size, trouver moyen de def un tableau sans def la taille avant */
    /* Grid in time*/

}



/* ENFAITE t'avais pas besoin de def les valeurs sur C...
juste tu les def sur python et ensuite tu appelles les valeurs quand tu tournes le code*/


/*Richards Model*/

int RichardsModel(psi,t,dz,n,p,vg,qTop,qBot,psiTop,psiBot){
    C = vg.Cfun(psi, p); /* a def en dehors de */




    for (int i = 0; i < n; i++) {
        dpsidt[i] = (-(q[i+1] - q[i]) / dz) / Cvec[i];
    }
    return dpsidt; 

}



#include <math.h>
#include <stdlib.h>

typedef struct {
    /* Add your parameter fields here */
} Params;

/* Function prototypes for your van Genuchten functions */
double CFun(double psi, Params *p);
double KFun(double psi, Params *p);

/* Utility: test whether a value is “empty” (Python []) */
int isEmpty(double x) {
    return isnan(x);
}

void RichardsModel(
        double *dpsidt,     // output array length n
        const double *psi,  // input array length n
        int n,
        double dz,
        Params *p,
        double qTop,        // use NAN if []
        double qBot,        // use NAN if []
        double psiTop,      // use NAN if []
        double psiBot       // use NAN if [] 
    ) {
    double C;
    double *q = (double*)calloc(n + 1, sizeof(double));
    /*des variable qu'on def avant dans le code python*/

    /* Capacitance C(psi) — assumed scalar per node */
    /* Python uses C = vg.CFun(psi,p) which returns vector C[n]
       If your CFun returns scalar, change to fill array. */

    double *Cvec = (double*)malloc(sizeof(double)*n);
    for (int i = 0; i < n; i++)
        Cvec[i] = CFun(psi[i], p);

    /* -------- Upper boundary -------- */
    if (isEmpty(qTop)) {
        double KTop = KFun(psiTop, p);
        q[n] = -KTop * (((psiTop - psi[n-1]) / dz) * 2.0 + 1.0);
    } else {
        q[n] = qTop;
    }

    /* -------- Lower boundary -------- */
    if (isEmpty(qBot)) {
        if (isEmpty(psiBot)) {
            /* Free drainage */
            double KBot = KFun(psi[0], p);
            q[0] = -KBot;
        } else {
            double KBot = KFun(psiBot, p);
            q[0] = -KBot * (((psi[0] - psiBot) / dz) * 2.0 + 1.0);
        }
    } else {
        q[0] = qBot;
    }

    /* -------- Internal nodes -------- */
    for (int i = 0; i < n - 1; i++) {
        double K1 = KFun(psi[i], p);
        double K2 = KFun(psi[i+1], p);
        double Kmid = 0.5 * (K1 + K2);

        int j = i + 1;
        q[j] = -Kmid * (((psi[i+1] - psi[i]) / dz) + 1.0);
    }

    /* -------- Continuity equation -------- */
    for (int i = 0; i < n; i++) {
        dpsidt[i] = (-(q[i+1] - q[i]) / dz) / Cvec[i];
    }

    free(q);
    free(Cvec);
    return dpsidt;
}

/* model de résolution equation diff 
*/