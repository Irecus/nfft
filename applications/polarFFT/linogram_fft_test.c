/*
 * Copyright (c) 2002, 2012 Jens Keiner, Stefan Kunis, Daniel Potts
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/* $Id$ */

/**
 * \file polarFFT/linogram_fft_test.c
 * \brief NFFT-based pseudo-polar FFT and inverse.
 *
 * Computes the NFFT-based pseudo-polar FFT and its inverse.
 * \author Markus Fenn
 * \date 2006
 */
#include "config.h"

#include <math.h>
#include <stdlib.h>
#ifdef HAVE_COMPLEX_H
#include <complex.h>
#endif

#include "nfft3.h"
#include "infft.h"

#undef X
#define X(name) NFFT(name)

/**
 * \defgroup applications_polarFFT_linogramm linogram_fft_test
 * \ingroup applications_polarFFT
 * \{
 */

R GLOBAL_elapsed_time;

/** Generates the points x with weights w
 *  for the linogram grid with T slopes and R offsets.
 */
static int linogram_grid(int T, int rr, R *x, R *w)
{
  int t, r;
  R W = (R) T * (((R) rr / K(2.0)) * ((R) rr / K(2.0)) + K(1.0) / K(4.0));

  for (t = -T / 2; t < T / 2; t++)
  {
    for (r = -rr / 2; r < rr / 2; r++)
    {
      if (t < 0)
      {
        x[2 * ((t + T / 2) * rr + (r + rr / 2)) + 0] = (R) (r) / (R)(rr);
        x[2 * ((t + T / 2) * rr + (r + rr / 2)) + 1] = K(4.0) * ((R)(t) + (R)(T) / K(4.0)) / (R)(T)
            * (R)(r) / (R)(rr);
      }
      else
      {
        x[2 * ((t + T / 2) * rr + (r + rr / 2)) + 0] = -K(4.0) * ((R)(t) - (R)(T) / K(4.0)) / (R)(T)
            * (R)(r) / (R)(rr);
        x[2 * ((t + T / 2) * rr + (r + rr / 2)) + 1] = (R) r / (R)(rr);
      }
      if (r == 0)
        w[(t + T / 2) * rr + (r + rr / 2)] = K(1.0) / K(4.0) / W;
      else
        w[(t + T / 2) * rr + (r + rr / 2)] = FABS((R) r) / W;
    }
  }

  return T * rr; /** return the number of knots        */
}

/** discrete pseudo-polar FFT */
static int linogram_dft(C *f_hat, int NN, C *f, int T, int rr, int m)
{
  ticks t0, t1;
  int j, k; /**< index for nodes and freqencies   */
  X(plan) my_nfft_plan; /**< plan for the nfft-2D             */

  R *x, *w; /**< knots and associated weights     */

  int N[2], n[2];
  int M = T * rr; /**< number of knots                  */

  N[0] = NN;
  n[0] = 2 * N[0]; /**< oversampling factor sigma=2      */
  N[1] = NN;
  n[1] = 2 * N[1]; /**< oversampling factor sigma=2      */

  x = (R *) Y(malloc)((size_t)(2 * T * rr) * (sizeof(R)));
  if (x == NULL)
    return EXIT_FAILURE;

  w = (R *) Y(malloc)((size_t)(T * rr) * (sizeof(R)));
  if (w == NULL)
    return EXIT_FAILURE;

  /** init two dimensional NFFT plan */
  X(init_guru)(&my_nfft_plan, 2, N, M, n, m,
      PRE_PHI_HUT | PRE_PSI | MALLOC_X | MALLOC_F_HAT | MALLOC_F | FFTW_INIT
          | FFT_OUT_OF_PLACE,
      FFTW_MEASURE | FFTW_DESTROY_INPUT);

  /** init nodes from linogram grid*/
  linogram_grid(T, rr, x, w);
  for (j = 0; j < my_nfft_plan.M_total; j++)
  {
    my_nfft_plan.x[2 * j + 0] = x[2 * j + 0];
    my_nfft_plan.x[2 * j + 1] = x[2 * j + 1];
  }

  /** init Fourier coefficients from given image */
  for (k = 0; k < my_nfft_plan.N_total; k++)
    my_nfft_plan.f_hat[k] = f_hat[k];

  /** NFFT-2D */
  t0 = getticks();
  X(trafo_direct)(&my_nfft_plan);
  t1 = getticks();
  GLOBAL_elapsed_time = Y(elapsed_seconds)(t1, t0);

  /** copy result */
  for (j = 0; j < my_nfft_plan.M_total; j++)
    f[j] = my_nfft_plan.f[j];

  /** finalise the plans and free the variables */
  X(finalize)(&my_nfft_plan);
  Y(free)(x);
  Y(free)(w);

  return EXIT_SUCCESS;
}

/** NFFT-based pseudo-polar FFT */
static int linogram_fft(C *f_hat, int NN, C *f, int T, int rr, int m)
{
  ticks t0, t1;
  int j, k; /**< index for nodes and freqencies   */
  X(plan) my_nfft_plan; /**< plan for the nfft-2D             */

  R *x, *w; /**< knots and associated weights     */

  int N[2], n[2];
  int M = T * rr; /**< number of knots                  */

  N[0] = NN;
  n[0] = 2 * N[0]; /**< oversampling factor sigma=2      */
  N[1] = NN;
  n[1] = 2 * N[1]; /**< oversampling factor sigma=2      */

  x = (R *) Y(malloc)((size_t)(2 * T * rr) * (sizeof(R)));
  if (x == NULL)
    return EXIT_FAILURE;

  w = (R *) Y(malloc)((size_t)(T * rr) * (sizeof(R)));
  if (w == NULL)
    return EXIT_FAILURE;

  /** init two dimensional NFFT plan */
  X(init_guru)(&my_nfft_plan, 2, N, M, n, m,
      PRE_PHI_HUT | PRE_PSI | MALLOC_X | MALLOC_F_HAT | MALLOC_F | FFTW_INIT
          | FFT_OUT_OF_PLACE,
      FFTW_MEASURE | FFTW_DESTROY_INPUT);

  /** init nodes from linogram grid*/
  linogram_grid(T, rr, x, w);
  for (j = 0; j < my_nfft_plan.M_total; j++)
  {
    my_nfft_plan.x[2 * j + 0] = x[2 * j + 0];
    my_nfft_plan.x[2 * j + 1] = x[2 * j + 1];
  }

  /** precompute psi, the entries of the matrix B */
  if (my_nfft_plan.flags & PRE_LIN_PSI)
    X(precompute_lin_psi)(&my_nfft_plan);

  if (my_nfft_plan.flags & PRE_PSI)
    X(precompute_psi)(&my_nfft_plan);

  if (my_nfft_plan.flags & PRE_FULL_PSI)
    X(precompute_full_psi)(&my_nfft_plan);

  /** init Fourier coefficients from given image */
  for (k = 0; k < my_nfft_plan.N_total; k++)
    my_nfft_plan.f_hat[k] = f_hat[k];

  /** NFFT-2D */
  t0 = getticks();
  X(trafo)(&my_nfft_plan);
  t1 = getticks();
  GLOBAL_elapsed_time = Y(elapsed_seconds)(t1, t0);

  /** copy result */
  for (j = 0; j < my_nfft_plan.M_total; j++)
    f[j] = my_nfft_plan.f[j];

  /** finalise the plans and free the variables */
  X(finalize)(&my_nfft_plan);
  Y(free)(x);
  Y(free)(w);

  return EXIT_SUCCESS;
}

/** NFFT-based inverse pseudo-polar FFT */
static int inverse_linogram_fft(C *f, int T, int rr, C *f_hat, int NN,
    int max_i, int m)
{
  ticks t0, t1;
  int j, k; /**< index for nodes and freqencies   */
  X(plan) my_nfft_plan; /**< plan for the nfft-2D             */
  SOLVER(plan_complex) my_infft_plan; /**< plan for the inverse nfft        */

  R *x, *w; /**< knots and associated weights     */
  int l; /**< index for iterations             */

  int N[2], n[2];
  int M = T * rr; /**< number of knots                  */

  N[0] = NN;
  n[0] = 2 * N[0]; /**< oversampling factor sigma=2      */
  N[1] = NN;
  n[1] = 2 * N[1]; /**< oversampling factor sigma=2      */

  x = (R *) Y(malloc)((size_t)(2 * T * rr) * (sizeof(R)));
  if (x == NULL)
    return EXIT_FAILURE;

  w = (R *) Y(malloc)((size_t)(T * rr) * (sizeof(R)));
  if (w == NULL)
    return EXIT_FAILURE;

  /** init two dimensional NFFT plan */
  X(init_guru)(&my_nfft_plan, 2, N, M, n, m,
      PRE_PHI_HUT | PRE_PSI | MALLOC_X | MALLOC_F_HAT | MALLOC_F | FFTW_INIT
          | FFT_OUT_OF_PLACE,
      FFTW_MEASURE | FFTW_DESTROY_INPUT);

  /** init two dimensional infft plan */
  SOLVER(init_advanced_complex)(&my_infft_plan,
      (X(mv_plan_complex)*) (&my_nfft_plan), CGNR | PRECOMPUTE_WEIGHT);

  /** init nodes, given samples and weights */
  linogram_grid(T, rr, x, w);
  for (j = 0; j < my_nfft_plan.M_total; j++)
  {
    my_nfft_plan.x[2 * j + 0] = x[2 * j + 0];
    my_nfft_plan.x[2 * j + 1] = x[2 * j + 1];
    my_infft_plan.y[j] = f[j];
    my_infft_plan.w[j] = w[j];
  }

  /** precompute psi, the entries of the matrix B */
  if (my_nfft_plan.flags & PRE_LIN_PSI)
    X(precompute_lin_psi)(&my_nfft_plan);

  if (my_nfft_plan.flags & PRE_PSI)
    X(precompute_psi)(&my_nfft_plan);

  if (my_nfft_plan.flags & PRE_FULL_PSI)
    X(precompute_full_psi)(&my_nfft_plan);

  /** initialise damping factors */
  if (my_infft_plan.flags & PRECOMPUTE_DAMP)
    for (j = 0; j < my_nfft_plan.N[0]; j++)
      for (k = 0; k < my_nfft_plan.N[1]; k++)
      {
        my_infft_plan.w_hat[j * my_nfft_plan.N[1] + k] = (
            SQRT(
                POW((R)(j - my_nfft_plan.N[0] / 2), K(2.0))
                    + POW((R)(k - my_nfft_plan.N[1] / 2), K(2.0)))
                > (R)(my_nfft_plan.N[0] / 2) ? 0 : 1);
      }

  /** initialise some guess f_hat_0 */
  for (k = 0; k < my_nfft_plan.N_total; k++)
    my_infft_plan.f_hat_iter[k] = K(0.0) + II * K(0.0);

  t0 = getticks();
  /** solve the system */
  SOLVER(before_loop_complex)(&my_infft_plan);

  if (max_i < 1)
  {
    l = 1;
    for (k = 0; k < my_nfft_plan.N_total; k++)
      my_infft_plan.f_hat_iter[k] = my_infft_plan.p_hat_iter[k];
  }
  else
  {
    for (l = 1; l <= max_i; l++)
    {
      SOLVER(loop_one_step_complex)(&my_infft_plan);
    }
  }

  t1 = getticks();
  GLOBAL_elapsed_time = Y(elapsed_seconds)(t1, t0);

  /** copy result */
  for (k = 0; k < my_nfft_plan.N_total; k++)
    f_hat[k] = my_infft_plan.f_hat_iter[k];

  /** finalise the plans and free the variables */
  SOLVER(finalize_complex)(&my_infft_plan);
  X(finalize)(&my_nfft_plan);
  Y(free)(x);
  Y(free)(w);

  return EXIT_SUCCESS;
}

/** Comparison of the FFTW, linogram FFT, and inverse linogram FFT */
static int comparison_fft(FILE *fp, int N, int T, int rr)
{
  ticks t0, t1;
  Z(plan) my_fftw_plan;
  C *f_hat, *f;
  int m, k;
  R t_fft, t_dft_linogram;

  f_hat = (C *) Y(malloc)(sizeof(C) * (size_t)(N * N));
  f = (C *) Y(malloc)(sizeof(C) * (size_t)((T * rr / 4) * 5));

  my_fftw_plan = Z(plan_dft_2d)(N, N, f_hat, f, FFTW_BACKWARD, FFTW_MEASURE);

  for (k = 0; k < N * N; k++)
    f_hat[k] = Y(drand48)() + II * Y(drand48)();

  t0 = getticks();
  for (m = 0; m < 65536 / N; m++)
  {
    Z(execute)(my_fftw_plan);
    /* touch */
    f_hat[2] = K(2.0) * f_hat[0];
  }
  t1 = getticks();
  GLOBAL_elapsed_time = Y(elapsed_seconds)(t1, t0);
  t_fft = (R)(N) * GLOBAL_elapsed_time / K(65536.0);

  if (N < 256)
  {
    linogram_dft(f_hat, N, f, T, rr, m);
    t_dft_linogram = GLOBAL_elapsed_time;
  }

  for (m = 3; m <= 9; m += 3)
  {
    if ((m == 3) && (N < 256))
      fprintf(fp, "%d\t&\t&\t%1.1" __FES__ "&\t%1.1" __FES__ "&\t%d\t", N, t_fft, t_dft_linogram,
          m);
    else if (m == 3)
      fprintf(fp, "%d\t&\t&\t%1.1" __FES__ "&\t       &\t%d\t", N, t_fft, m);
    else
      fprintf(fp, "  \t&\t&\t       &\t       &\t%d\t", m);

    printf("N=%d\tt_fft=%1.1" __FES__ "\tt_dft_linogram=%1.1" __FES__ "\tm=%d\t", N, t_fft,
        t_dft_linogram, m);

    linogram_fft(f_hat, N, f, T, rr, m);
    fprintf(fp, "%1.1" __FES__ "&\t", GLOBAL_elapsed_time);
    printf("t_linogram=%1.1" __FES__ "\t", GLOBAL_elapsed_time);
    inverse_linogram_fft(f, T, rr, f_hat, N, m + 3, m);
    if (m == 9)
      fprintf(fp, "%1.1" __FES__ "\\\\\\hline\n", GLOBAL_elapsed_time);
    else
      fprintf(fp, "%1.1" __FES__ "\\\\\n", GLOBAL_elapsed_time);
    printf("t_ilinogram=%1.1" __FES__ "\n", GLOBAL_elapsed_time);
  }

  fflush(fp);

  Y(free)(f);
  Y(free)(f_hat);

  return EXIT_SUCCESS;
}

/** test program for various parameters */
int main(int argc, char **argv)
{
  int N; /**< linogram FFT size NxN              */
  int T, rr; /**< number of directions/offsets     */
  int M; /**< number of knots of linogram grid   */
  R *x, *w; /**< knots and associated weights     */
  C *f_hat, *f, *f_direct, *f_tilde;
  int k;
  int max_i; /**< number of iterations             */
  int m;
  R temp1, temp2, E_max = K(0.0);
  FILE *fp1, *fp2;
  char filename[30];
  int logN;

  if (argc != 4)
  {
    printf("linogram_fft_test N T R \n");
    printf("\n");
    printf("N          linogram FFT of size NxN    \n");
    printf("T          number of slopes          \n");
    printf("R          number of offsets         \n");

    /** Hence, comparison of the FFTW, linogram FFT, and inverse linogram FFT */
    printf("\nHence, comparison FFTW, linogram FFT and inverse linogram FFT\n");
    fp1 = fopen("linogram_comparison_fft.dat", "w");
    if (fp1 == NULL)
      return (-1);
    for (logN = 4; logN <= 8; logN++)
      comparison_fft(fp1, (int)(1U << logN), 3 * (int)(1U << logN),
          3 * (int)(1U << (logN - 1)));
    fclose(fp1);

    return EXIT_FAILURE;
  }

  N = atoi(argv[1]);
  T = atoi(argv[2]);
  rr = atoi(argv[3]);
  printf("N=%d, linogram grid with T=%d, R=%d => ", N, T, rr);

  x = (R *) Y(malloc)((size_t)(5 * T * rr / 2) * (sizeof(R)));
  w = (R *) Y(malloc)((size_t)(5 * T * rr / 4) * (sizeof(R)));

  f_hat = (C *) Y(malloc)(sizeof(C) * (size_t)(N * N));
  f = (C *) Y(malloc)(sizeof(C) * (size_t)(5 * T * rr / 4)); /* 4/pi*log(1+sqrt(2)) = 1.122... < 1.25 */
  f_direct = (C *) Y(malloc)(sizeof(C) * (size_t)(5 * T * rr / 4));
  f_tilde = (C *) Y(malloc)(sizeof(C) * (size_t)(N * N));

  /** generate knots of linogram grid */
  M = linogram_grid(T, rr, x, w);
  printf("M=%d.\n", M);

  /** load data */
  fp1 = fopen("input_data_r.dat", "r");
  fp2 = fopen("input_data_i.dat", "r");
  if ((fp1 == NULL) || (fp2 == NULL))
    return EXIT_FAILURE;
  for (k = 0; k < N * N; k++)
  {
    fscanf(fp1, __FR__ " ", &temp1);
    fscanf(fp2, __FR__ " ", &temp2);
    f_hat[k] = temp1 + II * temp2;
  }
  fclose(fp1);
  fclose(fp2);

  /** direct linogram FFT */
  linogram_dft(f_hat, N, f_direct, T, rr, 1);
  //  linogram_fft(f_hat,N,f_direct,T,R,12);

  /** Test of the linogram FFT with different m */
  printf("\nTest of the linogram FFT: \n");
  fp1 = fopen("linogram_fft_error.dat", "w+");
  for (m = 1; m <= 12; m++)
  {
    /** fast linogram FFT */
    linogram_fft(f_hat, N, f, T, rr, m);

    /** error of fast linogram FFT */
    E_max = Y(error_l_infty_complex)(f_direct, f, M);
    //E_max=X(error_l_2_complex)(f_direct,f,M);

    printf("m=%2d: E_max = %" __FES__ "\n", m, E_max);
    fprintf(fp1, "%" __FES__ "\n", E_max);
  }
  fclose(fp1);

  /** Test of the inverse linogram FFT for different m in dependece of the iteration number*/
  for (m = 3; m <= 9; m += 3)
  {
    printf("\nTest of the inverse linogram FFT for m=%d: \n", m);
    sprintf(filename, "linogram_ifft_error%d.dat", m);
    fp1 = fopen(filename, "w+");
    for (max_i = 0; max_i <= 20; max_i += 2)
    {
      /** inverse linogram FFT */
      inverse_linogram_fft(f_direct, T, rr, f_tilde, N, max_i, m);

      /** compute maximum error */
      E_max = Y(error_l_infty_complex)(f_hat, f_tilde, N * N);
      printf("%3d iterations: E_max = %" __FES__ "\n", max_i, E_max);
      fprintf(fp1, "%" __FES__ "\n", E_max);
    }
    fclose(fp1);
  }

  /** free the variables */
  Y(free)(x);
  Y(free)(w);
  Y(free)(f_hat);
  Y(free)(f);
  Y(free)(f_direct);
  Y(free)(f_tilde);

  return EXIT_SUCCESS;
}
/* \} */
