
/* Portions copyright (c) 2006 Stanford University and Simbios.
 * Contributors: Pande Group
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <sstream>

#include "../SimTKUtilities/SimTKOpenMMCommon.h"
#include "../SimTKUtilities/SimTKOpenMMLog.h"
#include "../SimTKUtilities/SimTKOpenMMUtilities.h"
#include "ReferenceLJCoulombIxn.h"
#include "ReferenceForce.h"

/**---------------------------------------------------------------------------------------

   ReferenceLJCoulombIxn constructor

   --------------------------------------------------------------------------------------- */

ReferenceLJCoulombIxn::ReferenceLJCoulombIxn( ) : cutoff(false), periodic(false) {

   // ---------------------------------------------------------------------------------------

   // static const char* methodName = "\nReferenceLJCoulombIxn::ReferenceLJCoulombIxn";

   // ---------------------------------------------------------------------------------------

}

/**---------------------------------------------------------------------------------------

   ReferenceLJCoulombIxn destructor

   --------------------------------------------------------------------------------------- */

ReferenceLJCoulombIxn::~ReferenceLJCoulombIxn( ){

   // ---------------------------------------------------------------------------------------

   // static const char* methodName = "\nReferenceLJCoulombIxn::~ReferenceLJCoulombIxn";

   // ---------------------------------------------------------------------------------------

}

  /**---------------------------------------------------------------------------------------

     Set the force to use a cutoff.

     @param distance            the cutoff distance
     @param neighbors           the neighbor list to use
     @param solventDielectric   the dielectric constant of the bulk solvent

     @return ReferenceForce::DefaultReturn

     --------------------------------------------------------------------------------------- */

  int ReferenceLJCoulombIxn::setUseCutoff( RealOpenMM distance, const OpenMM::NeighborList& neighbors, RealOpenMM solventDielectric ) {
    
    cutoff = true;
    cutoffDistance = distance;
    neighborList = &neighbors;
    krf = pow(cutoffDistance, -3.0f)*(solventDielectric-1.0f)/(2.0f*solventDielectric+1.0f);
    crf = (1.0f/cutoffDistance)*(3.0f*solventDielectric)/(2.0f*solventDielectric+1.0f);
            
    return ReferenceForce::DefaultReturn;
  }

  /**---------------------------------------------------------------------------------------

     Set the force to use periodic boundary conditions.  This requires that a cutoff has
     also been set, and the smallest side of the periodic box is at least twice the cutoff
     distance.

     @param boxSize             the X, Y, and Z widths of the periodic box

     @return ReferenceForce::DefaultReturn

     --------------------------------------------------------------------------------------- */

  int ReferenceLJCoulombIxn::setPeriodic( RealOpenMM* boxSize ) {

    assert(cutoff);
    assert(boxSize[0] >= 2.0*cutoffDistance);
    assert(boxSize[1] >= 2.0*cutoffDistance);
    assert(boxSize[2] >= 2.0*cutoffDistance);
    periodic = true;
    periodicBoxSize[0] = boxSize[0];
    periodicBoxSize[1] = boxSize[1];
    periodicBoxSize[2] = boxSize[2];
    return ReferenceForce::DefaultReturn;

  }

/**---------------------------------------------------------------------------------------

   Calculate parameters for LJ Coulomb ixn

   @param c6               c6
   @param c12              c12
   @param q1               q1 charge atom 1
   @param epsfac           epsfacSqrt ????????????
   @param parameters       output parameters:
										parameter[SigIndex]  = 0.5*( (c12/c6)**1/6 ) (sigma/2)
										parameter[EpsIndex]  = sqrt(c6*c6/c12)       (2*sqrt(epsilon))
										parameter[QIndex]    = epsfactorSqrt*q1

   @return ReferenceForce::DefaultReturn

   --------------------------------------------------------------------------------------- */

int ReferenceLJCoulombIxn::getDerivedParameters( RealOpenMM c6, RealOpenMM c12, RealOpenMM q1,
                                                 RealOpenMM epsfacSqrt,
                                                 RealOpenMM* parameters ) const {

   // ---------------------------------------------------------------------------------------

   // static const char* methodName = "\nReferenceLJCoulombIxn::getDerivedParameters";

   static const RealOpenMM zero          =  0.0;
   static const RealOpenMM one           =  1.0;
   static const RealOpenMM six           =  6.0;
   static const RealOpenMM half          =  0.5;
   static const RealOpenMM oneSixth      =  one/six;
   static const RealOpenMM oneTweleth    =  half*oneSixth;

   // ---------------------------------------------------------------------------------------

   if( c12 <= 0.0 ){

      parameters[EpsIndex] = zero;
      parameters[SigIndex] = half;

   } else {
 
      parameters[EpsIndex]    = c6*SQRT( one/c12 );
   
      parameters[SigIndex]    = POW( (c12/c6), oneSixth );
      parameters[SigIndex]   *= half;
   }

   parameters[QIndex]   = epsfacSqrt*q1;

   return ReferenceForce::DefaultReturn;
}

/**---------------------------------------------------------------------------------------

   Calculate LJ Coulomb pair ixn

   @param numberOfAtoms    number of atoms
   @param atomCoordinates  atom coordinates
   @param atomParameters   atom parameters                             atomParameters[atomIndex][paramterIndex]
   @param exclusions       atom exclusion indices                      exclusions[atomIndex][atomToExcludeIndex]
                           exclusions[atomIndex][0] = number of exclusions
                           exclusions[atomIndex][1-no.] = atom indices of atoms to excluded from
                           interacting w/ atom atomIndex
   @param fixedParameters  non atom parameters (not currently used)
   @param forces           force array (forces added)
   @param energyByAtom     atom energy
   @param totalEnergy      total energy

   @return ReferenceForce::DefaultReturn
      
   --------------------------------------------------------------------------------------- */
    
int ReferenceLJCoulombIxn::calculatePairIxn( int numberOfAtoms, RealOpenMM** atomCoordinates,
                                             RealOpenMM** atomParameters, int** exclusions,
                                             RealOpenMM* fixedParameters, RealOpenMM** forces,
                                             RealOpenMM* energyByAtom, RealOpenMM* totalEnergy ) const {

   if (cutoff) {
       for (int i = 0; i < (int) neighborList->size(); i++) {
           OpenMM::AtomPair pair = (*neighborList)[i];
           calculateOneIxn(pair.first, pair.second, atomCoordinates, atomParameters, forces, energyByAtom, totalEnergy);
       }
   }
   else {
       // allocate and initialize exclusion array

       int* exclusionIndices = new int[numberOfAtoms];
       for( int ii = 0; ii < numberOfAtoms; ii++ ){
          exclusionIndices[ii] = -1;
       }

       for( int ii = 0; ii < numberOfAtoms; ii++ ){

          // set exclusions

          for( int jj = 1; jj <= exclusions[ii][0]; jj++ ){
             exclusionIndices[exclusions[ii][jj]] = ii;
          }

          // loop over atom pairs

          for( int jj = ii+1; jj < numberOfAtoms; jj++ ){

             if( exclusionIndices[jj] != ii ){
                 calculateOneIxn(ii, jj, atomCoordinates, atomParameters, forces, energyByAtom, totalEnergy);
             }
          }
       }

       delete[] exclusionIndices;
   }

   return ReferenceForce::DefaultReturn;
}

  /**---------------------------------------------------------------------------------------

     Calculate LJ Coulomb pair ixn between two atoms

     @param ii               the index of the first atom
     @param jj               the index of the second atom
     @param atomCoordinates  atom coordinates
     @param atomParameters   atom parameters (charges, c6, c12, ...)     atomParameters[atomIndex][paramterIndex]
     @param forces           force array (forces added)
     @param energyByAtom     atom energy
     @param totalEnergy      total energy

     @return ReferenceForce::DefaultReturn

     --------------------------------------------------------------------------------------- */

  int ReferenceLJCoulombIxn::calculateOneIxn( int ii, int jj, RealOpenMM** atomCoordinates,
                        RealOpenMM** atomParameters, RealOpenMM** forces,
                        RealOpenMM* energyByAtom, RealOpenMM* totalEnergy ) const {

    // ---------------------------------------------------------------------------------------

    static const std::string methodName = "\nReferenceLJCoulombIxn::calculateOneIxn";

    // ---------------------------------------------------------------------------------------

    // constants -- reduce Visual Studio warnings regarding conversions between float & double

    static const RealOpenMM zero        =  0.0;
    static const RealOpenMM one         =  1.0;
    static const RealOpenMM two         =  2.0;
    static const RealOpenMM three       =  3.0;
    static const RealOpenMM six         =  6.0;
    static const RealOpenMM twelve      = 12.0;
    static const RealOpenMM oneM        = -1.0;

    static const int threeI             = 3;

    // debug flag

    static const int debug              = -1;

    static const int LastAtomIndex      = 2;

    RealOpenMM deltaR[2][ReferenceForce::LastDeltaRIndex];

    // get deltaR, R2, and R between 2 atoms

    if (periodic)
        ReferenceForce::getDeltaRPeriodic( atomCoordinates[jj], atomCoordinates[ii], periodicBoxSize, deltaR[0] );  
    else
        ReferenceForce::getDeltaR( atomCoordinates[jj], atomCoordinates[ii], deltaR[0] );  

    RealOpenMM r2        = deltaR[0][ReferenceForce::R2Index];
    RealOpenMM inverseR  = one/(deltaR[0][ReferenceForce::RIndex]);
    RealOpenMM sig       = atomParameters[ii][SigIndex] +  atomParameters[jj][SigIndex];
    RealOpenMM sig2      = inverseR*sig;
               sig2     *= sig2;
    RealOpenMM sig6      = sig2*sig2*sig2;

    RealOpenMM eps       = atomParameters[ii][EpsIndex]*atomParameters[jj][EpsIndex];
    RealOpenMM dEdR      = eps*( twelve*sig6 - six )*sig6;
               if (cutoff)
                   dEdR += atomParameters[ii][QIndex]*atomParameters[jj][QIndex]*(inverseR-2.0f*krf*r2);
               else
                   dEdR += atomParameters[ii][QIndex]*atomParameters[jj][QIndex]*inverseR;
               dEdR     *= inverseR*inverseR;
         
    // accumulate forces

    for( int kk = 0; kk < 3; kk++ ){
       RealOpenMM force  = dEdR*deltaR[0][kk];
       forces[ii][kk]   += force;
       forces[jj][kk]   -= force;
    }

    RealOpenMM energy = 0.0;

    // accumulate energies

    if( totalEnergy || energyByAtom ) {
        if (cutoff)
            energy = atomParameters[ii][QIndex]*atomParameters[jj][QIndex]*(inverseR+krf*r2-crf);
        else
            energy = atomParameters[ii][QIndex]*atomParameters[jj][QIndex]*inverseR;
        energy += eps*(sig6-one)*sig6;
        if( totalEnergy )
           *totalEnergy += energy;
        if( energyByAtom ){
           energyByAtom[ii] += energy;
           energyByAtom[jj] += energy;
        }
    }

    // debug 

    if( debug == ii ){
       static bool printHeader = false;
       std::stringstream message;
       message << methodName;
       message << std::endl;
       int pairArray[2] = { ii, jj };
       if( !printHeader  ){  
          printHeader = true;
          message << std::endl;
          message << methodName.c_str() << " a0 k [c q p s] r1 r2  angle dt rp p[] dot cosine angle dEdR*r F[]" << std::endl;
       }   

       message << std::endl;
       for( int kk = 0; kk < 2; kk++ ){
          message << " Atm " << pairArray[kk] << " [" << atomCoordinates[pairArray[kk]][0] << " " << atomCoordinates[pairArray[kk]][1] << " " << atomCoordinates[pairArray[kk]][2] << "] ";
       }
       message << std::endl << " Delta:";
       for( int kk = 0; kk < (LastAtomIndex - 1); kk++ ){
          message << " [";
          for( int jj = 0; jj < ReferenceForce::LastDeltaRIndex; jj++ ){
             message << deltaR[kk][jj] << " ";
          }
          message << "]";
       }
       message << std::endl;

       for( int kk = 0; kk < 2; kk++ ){
          message << " p" << pairArray[kk] << " [";
          message << atomParameters[pairArray[kk]][0] << " " << atomParameters[pairArray[kk]][1] << " " << atomParameters[pairArray[kk]][2];
          message << "]";
       }
      message << std::endl;

       message << " dEdR=" << dEdR;
       message << " E=" << energy << " force factors: ";
       message << "F=compute force; f=cumulative force";

       message << std::endl << "  ";
       message << " f" << ii << "[";
       SimTKOpenMMUtilities::formatRealStringStream( message, deltaR[0], threeI, dEdR );
       message << "]";

       for( int kk = 0; kk < 2; kk++ ){
          message << " F" <<  pairArray[kk] << " [";
          SimTKOpenMMUtilities::formatRealStringStream( message, forces[pairArray[kk]], threeI );
          message << "]";
       }

       SimTKOpenMMLog::printMessage( message );
    }
    return ReferenceForce::DefaultReturn;
  }

