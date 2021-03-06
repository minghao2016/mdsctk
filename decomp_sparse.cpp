//
// 
//                This source code is part of
// 
//                        M D S C T K
// 
//       Molecular Dynamics Spectral Clustering ToolKit
// 
//                        VERSION 1.2.5
// Written by Joshua L. Phillips.
// Copyright (c) 2012-2016, Joshua L. Phillips.
// Check out http://www.cs.mtsu.edu/~jphillips/software.html for more
// information.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
// 
// If you want to redistribute modifications, please consider that
// derived work must not be called official MDSCTK. Details are found
// in the README & LICENSE files - if they are missing, get the
// official version at github.com/jlphillipsphd/mdsctk/.
// 
// To help us fund MDSCTK development, we humbly ask that you cite the
// papers on the package - you can find them in the top README file.
// 
// For more info, check our website at
// http://www.cs.mtsu.edu/~jphillips/software.html
// 
//

// Local
#include "config.h"
#include "mdsctk.h"

int main(int argc, char* argv[])
{

  const char* program_name = "decomp_sparse";
  bool optsOK = true;
  gmx::initForCommandLine(&argc,&argv);
  copyright(program_name);
  cout << "   Reads the symmetric CSC format sparse matrix from" << endl;
  cout << "   input-file, and computes the number of requested" << endl;
  cout << "   eigenvalues/vectors of the normalized laplacian" << endl;
  cout << "   using ARPACK and a gaussian kernel of width sigma." << endl;
  cout << endl;
  cout << "   Use -h or --help to see the complete list of options." << endl;
  cout << endl;

  // Option vars...
  double sigma_a;
  int nev;
  string ssm_filename;
  string evals_filename;
  string evecs_filename;
  string residuals_filename;

  // Declare the supported options.
  po::options_description cmdline_options;
  po::options_description program_options("Program options");
  program_options.add_options()
    ("help,h", "show this help message and exit")
    ("sigma,q", po::value<double>(&sigma_a), "Input:  Standard deviation of gaussian kernel (real)")
    ("nevals,n", po::value<int>(&nev), "Input:  Number of eigenvalues/vectors (int)")
    ("ssm-file,s", po::value<string>(&ssm_filename)->default_value("distances.ssm"), "Input:  Symmetric sparse matrix file (string:filename)")
    ("evals-file,v", po::value<string>(&evals_filename)->default_value("eigenvalues.dat"), "Output:  Eigenvalues file (string:filename)")
    ("evecs-file,e", po::value<string>(&evecs_filename)->default_value("eigenvectors.dat"), "Output: Eigenvectors file (string:filename)")    
    ("residuals-file,r", po::value<string>(&residuals_filename)->default_value("residuals.dat"), "Output: Residuals file (string:filename)")    
    ;
  cmdline_options.add(program_options);

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
  po::notify(vm);    

  if (vm.count("help")) {
    cout << "usage: " << program_name << " [options]" << endl;
    cout << cmdline_options << endl;
    return 1;
  }
  if (!vm.count("sigma")) {
    cout << "ERROR: --sigma not supplied." << endl;
    cout << endl;
    optsOK = false;
  }
  if (!vm.count("nevals")) {
    cout << "ERROR: --nevals not supplied." << endl;
    cout << endl;
    optsOK = false;
  }

  if (!optsOK) {
    return -1;
  }

  cout << "Running with the following options:" << endl;
  cout << "sigma =          " << sigma_a << endl;
  cout << "nevals =         " << nev << endl;
  cout << "ssm-file =       " << ssm_filename << endl;
  cout << "evals-file =     " << evals_filename << endl;
  cout << "evecs-file =     " << evecs_filename << endl;
  cout << "residuals-file = " << residuals_filename << endl;
  cout << endl;

  // Defining variables;
  int     n;   // Dimension of the problem.
  int     nnz;
  int     *irow;
  int     *pcol;
  double  *A;   // Pointer to an array that stores the lower
		// triangular elements of A.
  double  *Ax;  // Array for residual calculation
  double  residual = 0.0;
  double  max_residual = 0.0;

  // File input streams
  ifstream ssm;

  // File output streams
  ofstream eigenvalues;
  ofstream eigenvectors;
  ofstream residuals;

  // EPS
  double eps = 1.0;
  do { eps /= 2.0; } while (1.0 + (eps / 2.0) != 1.0);
  eps = sqrt(eps);

  // Open files
  ssm.open(ssm_filename.c_str());
  eigenvalues.open(evals_filename.c_str());
  eigenvectors.open(evecs_filename.c_str());
  residuals.open(residuals_filename.c_str());

  // Read symmetric CSC matrix
  ssm.read((char*) &n, (sizeof(int) / sizeof(char)));
  pcol = new int[n+1];
  ssm.read((char*) pcol, (sizeof(int) / sizeof(char)) * (n+1));
  nnz = pcol[n];
  Ax = new double[n];
  A = new double[nnz];
  irow = new int[nnz];
  ssm.read((char*) irow, (sizeof(int) / sizeof(char)) * nnz);
  ssm.read((char*) A, (sizeof(double) / sizeof(char)) * nnz);
  ssm.close();

  // Begin AFFINTY

  // Turn distances into normalized affinities...
  double *d_a = new double[n];

  // ofstream naff;
  // naff.open("sym_n_affinities.dat");

  // Make affinity matrix...
  for (int x = 0; x < nnz; x++)
    A[x] = exp(-(A[x] * A[x]) / (2.0 * sigma_a * sigma_a));

  // Calculate D_A
  for (int x = 0; x < n; x++)
    d_a[x] = 0.0;
  for (int x = 0; x < n; x++) {
    for (int y = pcol[x]; y < pcol[x+1]; y++) {
      d_a[x] += A[y];
      d_a[irow[y]] += A[y];
    }
  }
  for (int x = 0; x < n; x++)
    d_a[x] = 1.0 / sqrt(d_a[x]);

  // Normalize the affinity matrix...
  for (int x = 0; x < n; x++) {
    for (int y = pcol[x]; y < pcol[x+1]; y++) {
      A[y] *= d_a[irow[y]] * d_a[x];
    }
  }

  delete [] d_a;

  // End AFFINITY

  // ARPACK variables...
  int ido = 0;
  char bmat = 'I';
  char which[2];
  which[0] = 'L';
  which[1] = 'A';
  double tol = 0.0;
  double *resid = new double[n];
  // NOTE: Need about one order of magnitude more arnoldi vectors to
  // converge for the normalized Laplacian (according to residuals...)
  int ncv = ((10*nev+1)>n)?n:(10*nev+1);
  double *V = new double[(ncv*n)+1];
  int ldv = n;
  int *iparam = new int[12];
  iparam[1] = 1;
  iparam[3] = 100 * nev;
  iparam[4] = 1;
  iparam[7] = 1;
  int *ipntr = new int[15];
  double *workd = new double[(3*n)+1];
  int lworkl = ncv*(ncv+9);
  double *workl = new double[lworkl+1];
  int info = 0;
  int rvec = 1;
  char HowMny = 'A';
  int *lselect = new int[ncv];
  double *d = new double[nev];
  double *Z = &V[1];
  int ldz = n;
  double sigma = 0.0;

  while (ido != 99) {
    dsaupd_(&ido, &bmat, &n, which,
	    &nev, &tol, resid,
	    &ncv, &V[1], &ldv,
	    &iparam[1], &ipntr[1], &workd[1],
	    &workl[1], &lworkl, &info);
    
    if (ido == -1 || ido == 1) {
      // Matrix-vector multiplication
      sp_dsymv(n,irow,pcol,A,
	       &workd[ipntr[1]],
	       &workd[ipntr[2]]);
    }
  }
  
  dseupd_(&rvec, &HowMny, lselect,
	  d, Z, &ldz,
	  &sigma, &bmat, &n,
	  which, &nev, &tol,
	  resid, &ncv, &V[1],
	  &ldv, &iparam[1], &ipntr[1],
	  &workd[1], &workl[1],
	  &lworkl, &info);

  cout << "Number of converged eigenvalues/vectors found: "
       << iparam[5] << endl;
  
  for (int x = nev-1; x >= 0; x--) {
#ifdef DECOMP_WRITE_DOUBLE
    eigenvalues.write((char*) &d[x],(sizeof(double) / sizeof(char)));
    eigenvectors.write((char*) &Z[n*x],(sizeof(double) * n) / sizeof(char));
#else
    eigenvalues << d[x] << endl;
    for (int y = 0; y < n; y++)
      eigenvectors << Z[(n*x)+y] << " ";
    eigenvectors << endl;
#endif

    // Calculate residual...
    // Matrix-vector multiplication
    sp_dsymv(n,irow,pcol,A,
	     &Z[n*x],
	     Ax);

    double t = -d[x];
    int i = 1;
    daxpy_(&n, &t, &Z[n*x], &i, Ax, &i);
    residual = dnrm2_(&n, Ax, &i)/fabs(d[x]);
    if (residual > max_residual)
      max_residual = residual;
#ifdef DECOMP_WRITE_DOUBLE
    residuals.write((char*) &residual, sizeof(double) / sizeof(char));
#else
    residuals << residual << endl;
#endif
  }

  cout << "Max residual: " << max_residual
       << " (eps: " << eps << ")" << endl;
  if (max_residual > eps) {
    cout << "*** Sum of residuals too high (max_r > eps)!" << endl;
    cout << "*** Please, check results manually..." << endl;
  }

  eigenvalues.close();
  eigenvectors.close();
  residuals.close();

  delete [] irow;
  delete [] pcol;
  delete [] Ax;
  delete [] A;

  // ARPACK
  delete [] lselect;
  delete [] d;
  delete [] resid;
  delete [] V;
  delete [] iparam;
  delete [] ipntr;
  delete [] workd;
  delete [] workl;

  return 0;

} // main.
