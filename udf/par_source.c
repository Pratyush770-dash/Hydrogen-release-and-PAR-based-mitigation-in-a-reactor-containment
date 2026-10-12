#include "udf.h"

real C_H2_in = 0.0;
real C_O2_in = 0.0;

/* Reads H2 and O2 concentration at the PAR inlet face every iteration */
DEFINE_ADJUST(compute_par_inlet_conc, domain)
{
    Thread *t;
    face_t f;
    real sum_h2 = 0.0, sum_o2 = 0.0, sum_area = 0.0;
    real area[ND_ND];
    real rho, yi_h2, yi_o2;
    real mw_h2 = 0.002016; /* kg/mol */
    real mw_o2 = 0.032;    /* kg/mol */

    t = Lookup_Thread(domain, 9); /* PAR_Inlet_1 zone ID -- confirm this is correct */

    begin_f_loop(f, t)
    {
        F_AREA(area, f, t);
        rho = F_R(f, t);
        yi_h2 = F_YI(f, t, 2); /* h2 = index 2 */
        yi_o2 = F_YI(f, t, 1); /* o2 = index 1 */

        sum_h2 += (rho * yi_h2 / mw_h2) * NV_MAG(area);
        sum_o2 += (rho * yi_o2 / mw_o2) * NV_MAG(area);
        sum_area += NV_MAG(area);
    }
    end_f_loop(f, t)

    if (sum_area > 0)
    {
        C_H2_in = sum_h2 / sum_area;
        C_O2_in = sum_o2 / sum_area;
    }
}
/* Applies the H2 sink using Eq. 17 (Halouane & Dehbi, 2018) */
DEFINE_SOURCE(par_h2_sink, c, t, dS, eqn)
{
    real P_abs_bar, mass_rate_g_s, source_kg_m3_s;
    real C_H2 = C_H2_in;
    real C_O2 = C_O2_in;
    real A_const, C_eff;
    real housing_volume = 0.06; /* m3 -- your box is 0.4 x 1 x 0.15, adjust if resized */

    P_abs_bar = (op_pres + C_P(c, t)) / 1e5;

    /* oxygen starvation check, per paper */
    if (C_O2 < 0.5 * C_H2)
        C_eff = 2.0 * C_O2;
    else
        C_eff = C_H2;

    if (C_eff <= C_O2)
        A_const = 1.0;
    else
        A_const = 0.6;

    /* Eq. 17: magnitude of recombination rate, g/s */
    mass_rate_g_s = A_const * (0.0137 * P_abs_bar + 0.0163) * C_eff * tanh(C_eff - 0.5);

    if (mass_rate_g_s < 0.0)
        mass_rate_g_s = 0.0; /* below threshold, tanh term goes negative -- clip to zero, no reaction yet */

    source_kg_m3_s = -(mass_rate_g_s / 1000.0) / housing_volume; /* negative = sink */

    dS[eqn] = 0.0;
    return source_kg_m3_s;
}