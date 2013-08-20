#!/bin/bash
##
## 
##                This source code is part of
## 
##                        M D S C T K
## 
##       Molecular Dynamics Spectral Clustering ToolKit
## 
##                        VERSION 1.1.1
## Written by Joshua L. Phillips.
## Copyright (c) 2013, Joshua L. Phillips.
## check out http://github.com/douradopalmares/mdsctk/ for more information.
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
## 
## If you want to redistribute modifications, please consider that
## derived work must not be called official MDSCTK. Details are found
## in the README & LICENSE files - if they are missing, get the
## official version at <website>.
## 
## To help us fund MDSCTK development, we humbly ask that you cite
## the papers on the package - you can find them in the top README file.
## 
## For more info, check our website at http://github.com/douradopalmares/mdsctk/
## 
##

if [[ -z "${MDSCTK_HOME}" ]]; then
    echo
    echo "Please set the MDSCTK_HOME environment variable"
    echo "before running this example..."
    echo
    exit 1
fi  

## NOTE that the XTC file contains only the
## N-CA-C backbone atoms. You must make sure
## this is true for any XTC file given to
## bb_xtc_to_phipsi (you have been warned)!
XTC=trp-cage.xtc
OSXTC=trp-cage-outofsample.xtc

NTHREADS=2    ## Number of threads to use
KNN=100       ## Number of nearest neighbors to keep
NCLUSTERS=10  ## Number of clusters to extract
SCALING=12    ## (must be <= KNN) for calculating scaling factors...

## VERY IMPORTANT ---
NRES=20                        # Number of residues in the protein!
NPHIPSI=$(( (${NRES}*2) - 2 )) # Number of Phi-Psi angles
NSINCOS=$(( ${NPHIPSI}*2 ))    # Number of polar coordinates

echo "Computing Phi-Psi angles..."
${MDSCTK_HOME}/bb_xtc_to_phipsi ${XTC}

echo "Converting angles to polar coordinates..."
${MDSCTK_HOME}/phipsi_to_sincos
mv sincos.dat landmarks.dat

echo "Computing Euclidean distances between all landmark vector pairs..."
${MDSCTK_HOME}/knn_data ${NTHREADS} ${KNN} ${NSINCOS} landmarks.dat landmarks.dat

echo "Creating CSC format symmetric sparse matrix..."
${MDSCTK_HOME}/make_sysparse ${KNN}

echo "Computing Phi-Psi angles (out-of-sample)..."
${MDSCTK_HOME}/bb_xtc_to_phipsi ${OSXTC}

echo "Converting angles to polar coordinates..."
${MDSCTK_HOME}/phipsi_to_sincos

echo "Computing Euclidean distances between landmark and out-of-sample vector pairs..."
${MDSCTK_HOME}/knn_data ${NTHREADS} ${KNN} ${NSINCOS} landmarks.dat sincos.dat

echo "Creating CSC format non-symmetric sparse matrix..."
${MDSCTK_HOME}/make_gesparse ${KNN}

echo "Performing autoscaled spectral decomposition..."
${MDSCTK_HOME}/auto_decomp_sparse_nystrom ${NCLUSTERS} ${SCALING}

echo "Clustering eigenvectors..."
${MDSCTK_HOME}/kmeans.r ${NCLUSTERS}

# Generate trajectory assignment file,
# 10 trajectories of 100 frames each
# and a single remaining out-of-sample
# trajectory with 10000 frames.
Rscript \
    -e 'myout<-pipe("cat","w")' \
    -e 'write(c(sort(rep(seq(1,10),100)),rep(11,10000)),myout,ncolumns=1)' \
    -e 'close(myout)' > assignment.dat

echo "Computing replicate-cluster assignment histogram..."
${MDSCTK_HOME}/clustering_histogram.r assignment.dat clusters.dat

echo "Plotting the histogram (fails if R package 'fields' is missing)..."
${MDSCTK_HOME}/plot_histogram.r

echo "See histogram.eps for results (eg. evince histogram.eps)..."

echo "Computing normalized mutual information..."
${MDSCTK_HOME}/clustering_nmi.r | tee nmi.dat
