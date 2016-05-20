#include "FastSimulation/Propagation/interface/HelixTrajectory.h"
#include "FastSimulation/Geometry/interface/BarrelLayer.h"
#include "FastSimulation/Geometry/interface/ForwardLayer.h"
#include "FastSimulation/Particle/interface/RawParticle.h"
#include <math.h>

// helix phi definition
// ranges from -PI to PI
// PI corresponds to the positive x direction
// PI increases counterclockwise

fastsim::HelixTrajectory::HelixTrajectory(const RawParticle & particle,double magneticFieldZ)
    : Trajectory(particle)
    , radius_( abs(momentum_.Pt() / ( speedOfLight_ * 1e-4 * magneticFieldZ * particle.charge() )))
    , phi0_(std::atan2(momentum_.Px(),momentum_.Py()) - (momentum_.Px() > 0 || momentum_.Py() > 0 ? M_PI/2 : -M_PI/2 ))  // something wrong here, should depend on direction of magnetic field and charge!
    , centerX_(position_.x() + radius_*std::cos(phi0_))
    , centerY_(position_.y() - radius_*std::sin(phi0_))
    , centerR_(centerX_*centerX_ + centerY_*centerY_)
    , minR_(centerR_ - radius_)
    , maxR_(centerR_ + radius_)
    , phiSpeed_( momentum_.Pt() / momentum_.E() * speedOfLight_ / radius_ * (particle.charge() > 0 ? 1. : -1) * (magneticFieldZ > 0 ? 1. : -1 ) ) // make sure you have the right signs here!
{}

bool fastsim::HelixTrajectory::crosses(const BarrelLayer & layer) const
{
    return (minR_ < layer.getRadius() && maxR_ > layer.getRadius());
}

bool fastsim::HelixTrajectory::crossesMaterial(const ForwardLayer & layer) const
{
    return (minR_ < layer.getMaxMaterialR() && maxR_ > layer.getMinMaterialR());
}

double fastsim::HelixTrajectory::nextCrossingTimeC(const BarrelLayer & layer) const
{
    //
    // solve the following equation for sin(phi)
    // (x^2 + y^2 = R_L^2)     (1)      the layer 
    // x = x_c + R_H*cos(phi)  (2)      the helix in the xy plane
    // y = y_c + R_H*sin(phi)  (3)      the helix in the xy plane
    // with
    // R_L: the radius of the layer
    // x_c,y_c the center of the helix in xy plane
    // R_H, the radius of the helix
    // phi, the phase of the helix
    //
    // substitute (2) and (3) in (1)
    // =>
    //   x_c^2 + 2*x_c*R_H*cos(phi) + R_H^2*cos^2(phi)
    // + y_c^2 + 2*y_c*R_H*sin(phi) + R_H^2*sin^2(phi)
    // = R_L^2
    // =>
    // (x_c^2 + y_c^2 + R_H^2 - R_L^2) + (2*y_c*R_H)*sin(phi) = -(2*x_c*R_H)*cos(phi)
    //
    // rewrite
    //               E                 +       F    *sin(phi) = -     G     *cos(phi)
    // =>
    // E^2 + 2*E*F*sin(phi) + F^2*sin^2(phi) = G^2*(1-sin^2(phi))
    // rearrange
    // sin^2(phi)*(F^2 + G^2) + sin(phi)*(2*E*F) + (E^2 - G^2) = 0
    //
    // rewrite
    // sin^2(phi)*     a      + sin(phi)*   b    +      c      = 0
    // => sin(phi) = (b +/- sqrt(b^2 - 4*ac)) / (2*a)
    // with
    // a = F^2 + G^2
    // b = 2*E*F
    // c = E^2 - G^2
    //
    // TODO: try to find the solution with less operations

    double E = centerX_*centerX_ + centerY_*centerY_ + radius_*radius_ - layer.getRadius()*layer.getRadius();
    double F = 2*centerY_*radius_;
    double G = 2*centerX_*radius_;

    double a = F*F + G*G;
    double b = 2*F*G;
    double c = E*E - G*G;
    
    double delta = b*b - 4*a*c;
    // case of no solution
    if(delta < 0)
    {
	return -1.;
    }
    double sqrtDelta = sqrt(delta);
    double phi1 = std::asin((-b - sqrtDelta)/ 2. / a);
    double phi2 = std::asin((-b + sqrtDelta)/ 2. / a);
    // asin is ambiguous, make sure to have the right solution
    // TODO: make sure this condition is correct
    if( (centerR_ > layer.getRadius() && centerX_ > 0.) || 
	(centerR_ < layer.getRadius() && centerX_ < 0.) )
    {
	phi1 = M_PI - phi1;
	phi2 = M_PI - phi2;
    }

    // find the corresponding times
    // make sure they are positive
    double t1 = (phi1 - phi0_)/phiSpeed_;
    if(t1 < 0)
    {
	t1 += 2*M_PI/phiSpeed_;
    }
    double t2 = (phi1 - phi0_)/phiSpeed_;
    if(t2 < 0)
    {
	t2 += 2*M_PI/phiSpeed_;
    }
    
    // not sure if we should stick to this t*c strategy...
    return std::min(t1,t2)*fastsim::Trajectory::speedOfLight_;
    
}

void fastsim::HelixTrajectory::move(double deltaTimeC)
{
    double deltaT = deltaTimeC/speedOfLight_;
    double positionPhi = phi0_ + phiSpeed_*deltaT;
    position_.SetXYZT(
	position_.x() + radius_*std::cos(positionPhi),
	position_.y() + radius_*std::cos(positionPhi),
	position_.z() + momentum_.pz()/momentum_.E()*deltaTimeC,
	position_.t() + deltaT);
    double momentumPhi = positionPhi  - (momentum_.Px() > 0 || momentum_.Py() > 0 ? M_PI/2 : -M_PI/2 ); // something wrong here, should depend on direction of magnetic field and charge!
    momentum_.SetXYZT(
	momentum_.Pt()*std::cos(momentumPhi),
	momentum_.Pt()*std::sin(momentumPhi),
	momentum_.Pz(),
	momentum_.E());
}