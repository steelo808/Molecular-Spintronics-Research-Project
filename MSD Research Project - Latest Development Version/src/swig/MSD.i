/**
 * @file MSD.i
 * @author your name (you@domain.com)
 * @brief Interface file for SWIG too. Used to create high-level language bindings.
 * @version 0.1
 * @date 2022-10-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */
%module MSD

%{
	#include "../MSD.h"
%}

%include "../MSD.h"
%include "../Vector.h"
