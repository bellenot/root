// @(#)root/tmva/tmva/cnn:$Id$
// Author: Manos Stergiadis

/**********************************************************************************
 * Project: TMVA - a Root-integrated toolkit for multivariate data analysis       *
 * Package: TMVA                                                                  *
 * Class  :                                                                       *
 *                                             *
 *                                                                                *
 * Description:                                                                   *
 *      Testing the Pooling Layer on a CPU architecture                           *
 *                                                                                *
 * Authors (alphabetical):                                                        *
 *      Manos Stergiadis       <em.stergiadis@gmail.com>  - CERN, Switzerland     *
 *                                                                                *
 * Copyright (c) 2005-2015:                                                       *
 *      CERN, Switzerland                                                         *
 *      U. of Victoria, Canada                                                    *
 *      MPI-K Heidelberg, Germany                                                 *
 *      U. of Bonn, Germany                                                       *
 *                                                                                *
 * Redistribution and use in source and binary forms, with or without             *
 * modification, are permitted according to the terms listed in LICENSE           *
 * (see tmva/doc/LICENSE)                                          *
 **********************************************************************************/

////////////////////////////////////////////////////////////////////
// Testing the Downsample function                                //
////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cmath>

#include "TMVA/DNN/Architectures/Cuda.h"
#include "TestPoolingLayer.h"


int main()
{
    using Scalar_t = Double_t;

    std::cout << "Testing Downsample on the GPU:" << std::endl;

    std::cout << "Test Forward-Propagation 1: " << std::endl;
    if (!testDownsample1<TCuda<Scalar_t>>()) {
        std::cerr << "ERROR - Forward-Propagation 1 failed " << std::endl;
        return -1;
    }

    std::cout << "Test Forward-Propagation 2: " << std::endl;
    if (!testDownsample2<TCuda<Scalar_t>>()) {
        std::cerr << "ERROR - Forward-Propagation 2 failed " << std::endl;
        return -1;
    }

    std::cout << "Test Back-propagation: " << std::endl;
    if (!testBackward1<TCuda<Scalar_t>>()) {
        std::cerr << "ERROR - Back-propagation failed " << std::endl;
        return -1;
    }

    std::cout << "Test Back-propagation: " << std::endl;
    if (!testBackward2<TCuda<Scalar_t>>()) {
        std::cerr << "ERROR - Back-propagation failed " << std::endl;
        return -1;
    }
    return 0;
}
