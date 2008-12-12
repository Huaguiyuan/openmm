/* -------------------------------------------------------------------------- *
 *                                   OpenMM                                   *
 * -------------------------------------------------------------------------- *
 * This is part of the OpenMM molecular simulation toolkit originating from   *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2008 Stanford University and the Authors.           *
 * Authors: Peter Eastman, Mark Friedrichs                                    *
 * Contributors:                                                              *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE  *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                     *
 * -------------------------------------------------------------------------- */

#include <cmath>
#include <limits>
#include "OpenMMException.h"
#include <sstream>

#include "BrookStreamImpl.h"
#include "BrookCalcNonbondedForceKernel.h"
#include "NonbondedForce.h"

using namespace OpenMM;
using namespace std;

const std::string BrookCalcNonbondedForceKernel::BondName = "LJ14";

/** 
 * BrookCalcNonbondedForceKernel constructor
 * 
 * @param name                      kernel name
 * @param platform                  platform
 *
 */

BrookCalcNonbondedForceKernel::BrookCalcNonbondedForceKernel( std::string name, const Platform& platform,
                                                              OpenMMBrookInterface& openMMBrookInterface, System& system ) :
                     CalcNonbondedForceKernel( name, platform ), _openMMBrookInterface( openMMBrookInterface ), _system( system ){

// ---------------------------------------------------------------------------------------

   // static const std::string methodName      = "BrookCalcNonbondedForceKernel::BrookCalcNonbondedForceKernel";

// ---------------------------------------------------------------------------------------

   _numberOfParticles                       = 0;
   _brookNonBonded                          = NULL;
   _brookBondParameters                     = NULL;

   _log                                     = NULL;

   _refForceField                           = NULL;
   _refSystem                               = NULL;
   _refOpenMMContext                        = NULL;
   _referencePlatform                       = NULL;
   _refVerletIntegrator                     = NULL;

   const BrookPlatform brookPlatform        = dynamic_cast<const BrookPlatform&> (platform);
   if( brookPlatform.getLog() != NULL ){
      setLog( brookPlatform.getLog() );
   }
      
}   

/** 
 * BrookCalcNonbondedForceKernel destructor
 * 
 */

BrookCalcNonbondedForceKernel::~BrookCalcNonbondedForceKernel( ){

// ---------------------------------------------------------------------------------------

   // static const std::string methodName      = "BrookCalcNonbondedForceKernel::BrookCalcNonbondedForceKernel";

// ---------------------------------------------------------------------------------------

   //delete _brookBondParameters;
   delete _brookNonBonded;

   // deleted w/ kernel delete? If activated, program crashes

   //delete _refForceField;
/*
   delete _refSystem;
   delete _refOpenMMContext;
   delete _referencePlatform;
   delete _refVerletIntegrator;
*/
}

/** 
 * Get log file reference
 * 
 * @return  log file reference
 *
 */

FILE* BrookCalcNonbondedForceKernel::getLog( void ) const {
   return _log;
}

/** 
 * Set log file reference
 * 
 * @param  log file reference
 *
 * @return  DefaultReturnValue
 *
 */

int BrookCalcNonbondedForceKernel::setLog( FILE* log ){
   _log = log;
   return BrookCommon::DefaultReturnValue;
}

/** 
 * Set log file reference
 * 
 * @param  log file reference
 *
 * @return  DefaultReturnValue
 *
 */

void BrookCalcNonbondedForceKernel::initialize( const System& system, const NonbondedForce& force, const std::vector<std::set<int> >& exclusions ){

// ---------------------------------------------------------------------------------------

   static const std::string methodName      = "BrookCalcNonbondedForceKernel::initialize";

// ---------------------------------------------------------------------------------------

    FILE* log                 = getLog();
    _numberOfParticles        = force.getNumParticles();
/*
    nonbondedMethod = CalcNonbondedForceKernel::NonbondedMethod(force.getNonbondedMethod());
    nonbondedCutoff = (RealOpenMM) force.getCutoffDistance();
    Vec3 boxVectors[3];
    force.getPeriodicBoxVectors(boxVectors[0], boxVectors[1], boxVectors[2]);
    periodicBoxSize[0] = (RealOpenMM) boxVectors[0][0];
    periodicBoxSize[1] = (RealOpenMM) boxVectors[1][1];
    periodicBoxSize[2] = (RealOpenMM) boxVectors[2][2];
    if (nonbondedMethod == NoCutoff)
        neighborList = NULL;
    else
        neighborList = new NeighborList();
*/
   // ---------------------------------------------------------------------------------------

   // nonbonded

   if( _brookNonBonded ){
      delete _brookNonBonded;
   }
   _brookNonBonded           = new BrookNonBonded();
   _brookNonBonded->setLog( log );

   // charge & LJ parameters

   std::vector<std::vector<double> > nonbondedParameters;
   for( int ii = 0; ii < _numberOfParticles; ii++ ){
      double charge, radius, depth;
      force.getParticleParameters( ii, charge, radius, depth );
      std::vector<double> particleParamArray;
      nonbondedParameters[ii] = particleParamArray;
      particleParamArray[0]   = radius;
      particleParamArray[1]   = depth;
      particleParamArray[2]   = charge;
   }   

   _brookNonBonded->setup( _numberOfParticles, nonbondedParameters, exclusions, getPlatform() );
   _openMMBrookInterface.setTriggerForceKernel( this );
   _openMMBrookInterface.setTriggerEnergyKernel( this );


   // echo contents

   if( log ){
      std::string contents = _brookNonBonded->getContentsString( );
      (void) fprintf( log, "%s brookNonBonded::contents\n%s", methodName.c_str(), contents.c_str() );
      (void) fflush( log );
   }

   // nonbonded 14 ixns

   initialize14Interactions( system, force );

}

/** 
 * Initialize the kernel, setting up the values of all the force field parameters.
 * 
 * @param system                    System reference
 * @param force                     HarmonicLJ14Force reference
 *
 */

void BrookCalcNonbondedForceKernel::initialize14Interactions( const System& system, const NonbondedForce& force ){

// ---------------------------------------------------------------------------------------

   static const std::string methodName      = "BrookCalcNonbondedForceKernel::initialize14Interactions";

// ---------------------------------------------------------------------------------------

   FILE* log                 = getLog();

   // ---------------------------------------------------------------------------------------

   // create _brookBondParameters object containing particle indices/parameters

   int numberOf14Forces         = force.getNumNonbonded14();

   //delete _brookBondParameters;
   _brookBondParameters = new BrookBondParameters( BondName, NumberOfParticlesInBond, NumberOfParametersInBond, numberOf14Forces, getLog() );

   for( int ii = 0; ii < numberOf14Forces; ii++ ){

      int particle1, particle2;
      double  charge, radius, depth;

      int particles[NumberOfParticlesInBond];
      double parameters[NumberOfParametersInBond];

      force.getNonbonded14Parameters( ii, particle1, particle2,  charge, radius, depth ); 
      particles[0]    = particle1;
      particles[1]    = particle2;
 
      parameters[0]   = charge;
      parameters[1]   = radius;
      parameters[2]   = depth;

      _brookBondParameters->setBond( ii, particles, parameters );
   }   
   _openMMBrookInterface.setNonBonded14ForceParameters( _brookBondParameters );

   if( log ){
      std::string contents = _brookBondParameters->getContentsString( ); 
      (void) fprintf( log, "%s contents:\n%s", methodName.c_str(), contents.c_str() );
      (void) fflush( log );
   }

   // ---------------------------------------------------------------------------------------
    
}
/** 
 * Execute the kernel to calculate the nonbonded forces
 * 
 * @param context OpenMMContextImpl context
 *
 */

void BrookCalcNonbondedForceKernel::executeForces( OpenMMContextImpl& context ){

// ---------------------------------------------------------------------------------------

   static const std::string methodName      = "BrookCalcNonbondedForceKernel::executeForces";

// ---------------------------------------------------------------------------------------

   if( _openMMBrookInterface.getTriggerForceKernel() == this ){
      _openMMBrookInterface.computeForces( context );
   }   

   // ---------------------------------------------------------------------------------------
}

/**
 * Execute the kernel to calculate the energy.
 * 
 * @param context OpenMMContextImpl context
 *
 * @return  potential energy due to the NonbondedForce
 * Currently always return 0.0 since energies not calculated on gpu
 *
 */

double BrookCalcNonbondedForceKernel::executeEnergy( OpenMMContextImpl& context ){

// ---------------------------------------------------------------------------------------

   //static const std::string methodName      = "BrookCalcNonbondedForceKernel::executeEnergy";

// ---------------------------------------------------------------------------------------

   if( _openMMBrookInterface.getTriggerEnergyKernel() == this ){
      return (double) _openMMBrookInterface.computeEnergy( context );
   } else {
      return 0.0;
   }   

}

/**
 * Get reference Context
 * 
 * @param numberOfParticles  number of particles
 *
 * @return  OpenMMContext
 *
 */

OpenMMContext* BrookCalcNonbondedForceKernel::getReferenceOpenMMContext( int numberOfParticles ){

// ---------------------------------------------------------------------------------------

   //static const std::string methodName      = "BrookCalcNonbondedForceKernel::getReferenceOpenMMContext";

// ---------------------------------------------------------------------------------------

   if( _refOpenMMContext == NULL ){
      
      _referencePlatform       = new ReferencePlatform();
      _refSystem               = new System( numberOfParticles, 0 ); 
		_refVerletIntegrator     = new VerletIntegrator( 0.01 );

      _refSystem->addForce( _refForceField );

      _refOpenMMContext        = new OpenMMContext( *_refSystem, *_refVerletIntegrator, *_referencePlatform );
   }

   return _refOpenMMContext;

}
